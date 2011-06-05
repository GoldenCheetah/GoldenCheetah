/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "DataProcessor.h"
#include "LTMOutliers.h"
#include "Settings.h"
#include "Units.h"
#include <algorithm>
#include <QVector>

// Config widget used by the Preferences/Options config panes
class FixSpikes;
class FixSpikesConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixSpikesConfig)

    friend class ::FixSpikes;
    protected:
        QHBoxLayout *layout;
        QLabel *maxLabel, *varianceLabel;
        QDoubleSpinBox *max,
                       *variance;

    public:
        FixSpikesConfig(QWidget *parent) : DataProcessorConfig(parent) {

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            maxLabel = new QLabel(tr("Max"));
            varianceLabel = new QLabel(tr("Variance"));

            max = new QDoubleSpinBox();
            max->setMaximum(9999.99);
            max->setMinimum(0);
            max->setSingleStep(1);

            variance = new QDoubleSpinBox();
            variance->setMaximum(9999);
            variance->setMinimum(0);
            variance->setSingleStep(50);

            layout->addWidget(maxLabel);
            layout->addWidget(max);
            layout->addWidget(varianceLabel);
            layout->addWidget(variance);
            layout->addStretch();
        }

        //~FixSpikesConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Occasionally power meters will erroneously "
                           "report high values for power. For crank based "
                           "power meters such as SRM and Quarq this is "
                           "caused by an erroneous cadence reading "
                           "as a result of triggering a reed switch "
                           "whilst pushing off\n\n"
                           "This function will look for spikes/anomalies "
                           "in power data and replace the erroneous data "
                           "by smoothing/interpolating the data from either "
                           "side of the point in question\n\n"
                           "It takes the following parameters:\n\n"
                           "Absolute Max - this defines an absolute value "
                           "for watts, and will smooth any values above this "
                           "absolute value that have been identified as being "
                           "anomalies (i.e. at odds with the data surrounding it)\n\n"
                           "Variance (%) - this will smooth any values which "
                           "are higher than this percentage of the rolling "
                           "average wattage for the 30 seconds leading up "
                           "to the spike.")));
        }

        void readConfig() {
            boost::shared_ptr<QSettings> settings = GetApplicationSettings();
            double tol = settings->value(GC_DPFS_MAX, "1500").toDouble();
            double stop = settings->value(GC_DPFS_VARIANCE, "1000").toDouble();
            max->setValue(tol);
            variance->setValue(stop);
        }

        void saveConfig() {
            boost::shared_ptr<QSettings> settings = GetApplicationSettings();
            settings->setValue(GC_DPFS_MAX, max->value());
            settings->setValue(GC_DPFS_VARIANCE, variance->value());
        }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixSpikes : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixSpikes)

    public:
        FixSpikes() {}
        ~FixSpikes() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixSpikesConfig(parent);
        }
        // Localized Name
        QString name() {
            return (tr("Fix Power Spikes"));
        }
};

static bool fixSpikesAdded = DataProcessorFactory::instance().registerProcessor((QString("Fix Power Spikes")), new FixSpikes());

bool
FixSpikes::postProcess(RideFile *ride, DataProcessorConfig *config=0)
{
    // does this ride have power?
    if (ride->areDataPresent()->watts == false) return false;

    // get settings
    double variance, max;
    if (config == NULL) { // being called automatically
        boost::shared_ptr<QSettings> settings = GetApplicationSettings();
        max = settings->value(GC_DPFS_MAX, "1500").toDouble();
        variance = settings->value(GC_DPFS_VARIANCE, "1000").toDouble();
    } else { // being called manually
        max = ((FixSpikesConfig*)(config))->max->value();
        variance = ((FixSpikesConfig*)(config))->variance->value();
    }

    int windowsize = 30 / ride->recIntSecs();

    // We use a window size of 30s to find spikes
    // if the ride is shorter, don't bother
    // is no way of post processing anyway (e.g. manual workouts)
    if (windowsize > ride->dataPoints().count()) return false;

    // Find the power outliers

    int spikes = 0;
    double spiketime = 0.0;

    // create a data array for the outlier algorithm
    QVector<double> power;
    QVector<double> secs;

    foreach (RideFilePoint *point, ride->dataPoints()) {
        power.append(point->watts);
        secs.append(point->secs);
    }

    LTMOutliers *outliers = new LTMOutliers(secs.data(), power.data(), power.count(), windowsize, false);
    ride->command->startLUW("Fix Spikes in Recording");
    for (int i=0; i<secs.count(); i++) {

        // is this over variance threshold?
        if (outliers->getDeviationForRank(i) < variance) break;

        // ok, so its highly variant but is it over
        // the max value we are willing to accept?
        if (outliers->getYForRank(i) < max) continue;

        // Houston, we have a spike
        spikes++;
        spiketime += ride->recIntSecs();

        // which one is it
        int pos = outliers->getIndexForRank(i);
        double left=0.0, right=0.0;

        if (pos > 0) left = ride->dataPoints()[pos-1]->watts;
        if (pos < (ride->dataPoints().count()-1)) right = ride->dataPoints()[pos+1]->watts;

        ride->command->setPointValue(pos, RideFile::watts, (left+right)/2.0);
    }
    ride->command->endLUW();

    ride->setTag("Spikes", QString("%1").arg(spikes));
    ride->setTag("Spike Time", QString("%1").arg(spiketime));

    if (spikes) return true;
    else return false;
}
