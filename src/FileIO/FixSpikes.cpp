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
#include "HelpWhatsThis.h"
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
        QLabel* avgWindowLabel;
        QDoubleSpinBox *avgWindow;
        QCheckBox *dropOuts;

    public:
        FixSpikesConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixPowerSpikes));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            maxLabel = new QLabel(tr("Max Watts"));
            varianceLabel = new QLabel(tr("Watt Variance"));
            avgWindowLabel = new QLabel(tr("Avg Window (secs)"));

            max = new QDoubleSpinBox();
            max->setMaximum(9999.99);
            max->setMinimum(0);
            max->setSingleStep(1);

            variance = new QDoubleSpinBox();
            variance->setMaximum(9999);
            variance->setMinimum(0);
            variance->setSingleStep(10);

            avgWindow = new QDoubleSpinBox();
            avgWindow->setMaximum(61);
            avgWindow->setMinimum(5);
            avgWindow->setSingleStep(1);

            dropOuts = new QCheckBox(tr("Fix Dropouts"));

            layout->addWidget(maxLabel);
            layout->addWidget(max);
            layout->addWidget(varianceLabel);
            layout->addWidget(variance);
            layout->addWidget(avgWindowLabel);
            layout->addWidget(avgWindow);
            layout->addWidget(dropOuts);

            layout->addStretch();
        }

        //~FixSpikesConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Power meters will occasionally report erroneously "
                " high values for power. For crank based "
                "power meters such as SRM and Quarq this is "
                "caused by an erroneous cadence reading "
                "as a result of triggering a reed switch "
                "whilst pushing off.\n\n"
                "This function will look for spikes/anomalies "
                "in power data and replace the erroneous data "
                "by smoothing/interpolating the data from either "
                "side of the point in question.\n\n"
                "It takes the following parameters:\n\n"
                "Absolute Max - this defines an absolute value "
                "for watts, and will smooth any values above this "
                "absolute value that have been identified as being "
                "anomalies (i.e. at odds with the data surrounding it)\n\n"
                "Watt Variance - data values that differ by more "
                "than this variance wattage from the rolling average "
                "preceding the spike will be considered anomalous/spikes.\n\n"
                "Avg Window - defines the duration of the rolling average window (default 30sec).\n\n"
                "Fix Dropouts - data values that differ by less "
                "than the variance wattage from the rolling average "
                "preceding the spike will be considered anomalous/dropouts.\n\n")));
        }

        void readConfig() {
            double tol = appsettings->value(NULL, GC_DPFS_MAX, "200").toDouble();
            double stop = appsettings->value(NULL, GC_DPFS_VARIANCE, "20").toDouble();
            int wtime = appsettings->value(NULL, GC_DPFS_WINDOWTIME, "30").toDouble();
            bool fixneg = appsettings->value(NULL, GC_DPFS_DROPOUTS, false).toBool();
            max->setValue(tol);
            variance->setValue(stop);
            avgWindow->setValue(wtime);
            dropOuts->setChecked(fixneg);
        }

        void saveConfig() {
            appsettings->setValue(GC_DPFS_MAX, max->value());
            appsettings->setValue(GC_DPFS_VARIANCE, variance->value());
            appsettings->setValue(GC_DPFS_WINDOWTIME, avgWindow->value());
            appsettings->setValue(GC_DPFS_DROPOUTS, dropOuts->isChecked());
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
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixSpikesConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix Power Spikes"));
        }
};

static bool fixSpikesAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix Power Spikes"), new FixSpikes());

bool
FixSpikes::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // does this ride have power?
    if (ride->areDataPresent()->watts == false) return false;

    // get settings
    double variance, max, avgWindowTime;
    bool fixdropOuts;
    if (config == NULL) { // being called automatically
        max = appsettings->value(NULL, GC_DPFS_MAX, "200").toDouble();
        variance = appsettings->value(NULL, GC_DPFS_VARIANCE, "20").toDouble();
        avgWindowTime = appsettings->value(NULL, GC_DPFS_WINDOWTIME, "30").toDouble();
        fixdropOuts = appsettings->value(NULL, GC_DPFS_DROPOUTS, false).toBool();
    } else { // being called manually
        max = ((FixSpikesConfig*)(config))->max->value();
        variance = ((FixSpikesConfig*)(config))->variance->value();
        avgWindowTime = ((FixSpikesConfig*)(config))->avgWindow->value();
        fixdropOuts = ((FixSpikesConfig*)(config))->dropOuts->isChecked();
    }
   
    int windowsize = avgWindowTime / ride->recIntSecs();

    // We use a window size (default 30 secs) to find spikes
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

        // An entry is a fixup candidate only if its variance is high AND it is above a concerning power level.
        double y = outliers->getYForRank(i);
        if ((fixdropOuts && (fabs(outliers->getDeviationForRank(i)) < variance)) ||
            (!fixdropOuts && (outliers->getDeviationForRank(i) < variance)) || y < max)
            continue;

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

    delete outliers;

    ride->setTag("Spikes", QString("%1").arg(spikes));
    ride->setTag("Spike Time", QString("%1").arg(spiketime));

    if (spikes) return true;
    else return false;
}
