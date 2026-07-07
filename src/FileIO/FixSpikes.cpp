/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 * Median Filter Copyright (c) 2021 Paul Johnson (paulj49457@gmail.com)
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
#include "Colors.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

// Config widget used by the Preferences/Options config panes
class FixSpikes;
class FixSpikesConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixSpikesConfig)

    friend class ::FixSpikes;
    protected:
        QDoubleSpinBox *max,
                       *variance;
        QSpinBox* medWinSize;
        QCheckBox* algo;

    public:
        FixSpikesConfig(QWidget *parent) : DataProcessorConfig(parent) {
            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixPowerSpikes));

            QFormLayout *layout = newQFormLayout(this);
            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            algo = new QCheckBox(tr("Median"));

            max = new QDoubleSpinBox();
            max->setMaximum(9999.99);
            max->setMinimum(0);
            max->setSingleStep(1);
            max->setSuffix(" " + tr("W"));

            variance = new QDoubleSpinBox();
            variance->setMaximum(9999);
            variance->setMinimum(0);
            variance->setSingleStep(5);
            variance->setSuffix(" " + tr("W"));

            medWinSize = new QSpinBox();
            medWinSize->setMaximum(21);
            medWinSize->setMinimum(5);
            medWinSize->setSingleStep(2);
            medWinSize->setValue(13);
            medWinSize->setSuffix(" " + tr("Points"));

            // Ensure only odd numbers are set for the median window
            connect(medWinSize, QOverload<int>::of(&QSpinBox::valueChanged), this,
                [=](int i) {(i % 2) ? medWinSize->setValue(i) : medWinSize->setValue(i + 1); });

            layout->addRow("", algo);
            layout->addRow(tr("Max"), max);
            layout->addRow(tr("Variance"), variance);
            layout->addRow(tr("Window"), medWinSize);
        }

        //~FixSpikesConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        void readConfig() {
            double tol = appsettings->value(NULL, GC_DPFS_MAX, "200").toDouble();
            double stop = appsettings->value(NULL, GC_DPFS_VARIANCE, "10").toDouble();
            bool medAlgo = appsettings->value(NULL, GC_DPFS_MEDALGO, Qt::Unchecked).toBool();
            int mWin = appsettings->value(NULL, GC_DPFS_MEDWINSIZ, "13").toInt();

            max->setValue(tol);
            variance->setValue(stop);
            medWinSize->setValue(mWin);
            algo->setCheckState(medAlgo ? Qt::Checked : Qt::Unchecked);
        }

        void saveConfig() {
            appsettings->setValue(GC_DPFS_MAX, max->value());
            appsettings->setValue(GC_DPFS_VARIANCE, variance->value());
            appsettings->setValue(GC_DPFS_MEDWINSIZ, medWinSize->value());
            appsettings->setValue(GC_DPFS_MEDALGO, algo->checkState());
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
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op) override;

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) const override {
            Q_UNUSED(ride);
            return new FixSpikesConfig(parent);
        }

        // Localized Name
        QString name() const override {
            return tr("Fix Power Spikes");
        }

        QString id() const override {
            return "::FixSpikes";
        }

        QString legacyId() const override {
            return "Fix Power Spikes";
        }

        QString explain() const override {
            return tr("Power meters will occasionally report erroneously high values "
                      "for power. For crank based power meters such as SRM and Quarq this "
                      "is caused by an erroneous cadence reading as a result of triggering "
                      "a reed switch whilst pushing off.\n\n"
                      "This function provides two algorithms that look for spikes/anomalies "
                      "in power data and replace the erroneous data by: \n\n"
                      "i) Replacing the point in question with smoothed/interpolated data from "
                      "either side of the point in question, it takes the following parameters:\n\n"
                      "Absolute Max (Watts)- this defines an absolute value for watts, and will "
                      "smooth any values above this absolute value that have been identified "
                      "as being anomalies (i.e. at odds with the data surrounding it)\n\n"
                      "Variance (Watts) - This determines the threshold beyond which a data point "
                      "will be smoothed/interpolated, if the difference between the data point "
                      "value and the 30 second rolling average wattage prior to the spike exceeds "
                      "this parameter.\n\n"
                      "ii) Replacing the point in question with the median value of a window "
                      "centred upon the erronous data point. This approach is robust to local "
                      "outliers, and preserves sharp edges, it takes the following parameters:\n\n"
                      "Window Size - this defines the number of neighbouring points used to "
                      "determine a median value; the window size is always odd to ensure we "
                      "have a central median value.\n\n"
                      "Variance (Watts) - Determines the threshold beyond which a data point will "
                      "be fixed, if the difference between the data point value and the median "
                      "value exceeds this parameter.\n\n");
        }
};

static bool fixSpikesAdded = DataProcessorFactory::instance().registerProcessor(new FixSpikes());

bool
FixSpikes::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // does this ride have power?
    if (ride->areDataPresent()->watts == false) return false;

    // Keep track of the power outliers
    int spikes = 0;
    double spiketime = 0.0;

    bool medAlgo;
    double variance, max;
    int medianWinSize; // this nummber must be odd to align the centre of the median window with the point being tested/corrected

    if (config == NULL) { // being called automatically
        medAlgo = appsettings->value(NULL, GC_DPFS_MEDALGO, Qt::Unchecked).toBool();
        max = appsettings->value(NULL, GC_DPFS_MAX, "200").toDouble();
        variance = appsettings->value(NULL, GC_DPFS_VARIANCE, "10").toInt();
        medianWinSize = appsettings->value(NULL, GC_DPFS_MEDWINSIZ, "13").toInt();
    }
    else {// being called manually
        medAlgo = ((FixSpikesConfig*)(config))->algo->isChecked();
        max = ((FixSpikesConfig*)(config))->max->value();
        variance = ((FixSpikesConfig*)(config))->variance->value();
        medianWinSize = ((FixSpikesConfig*)(config))->medWinSize->value();
    }

    if (!medAlgo) {

        int windowsize = 30 / ride->recIntSecs();

        // We use a window size of 30s to find spikes
        // if the ride is shorter, don't bother
        // is no way of post processing anyway (e.g. manual workouts)
        if (windowsize > ride->dataPoints().count()) return false;

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
            if (   outliers->getDeviationForRank(i) < variance
                || y < max)
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

        delete outliers;

    } else {

        // We use a median window to find spikes if the ride is
        // shorter than the window don't bother, as there is no way
        // of post processing anyway (e.g. manual workouts)
        if (medianWinSize > ride->dataPoints().count()) return false;

        double* data = new double[medianWinSize];
        int halfMedianWin = medianWinSize / 2;

        ride->command->startLUW("Fix Spikes Median in Recording");
        int numDataPnts = ride->dataPoints().count();
        for (int dataPntPosn = 0; dataPntPosn < ride->dataPoints().count(); dataPntPosn++) {

            double wattsAtPnt = ride->dataPoints()[dataPntPosn]->watts;

            // load median window with values
            for (int medianWin = 0; medianWin < medianWinSize; medianWin++) {

                int dp = dataPntPosn + medianWin - halfMedianWin;

                // Load the median window...

                if (dp < 0) {
                    // At the beginning of the ride, the left-hand side of the median window doesn't align with any ride data, it's
                    // somewhat arbitrary how to pad this data, but choosing a single data point to replicate runs the risk of
                    // skewing the median filter so choose some reasonably close ride data to avoid this scenario.
                    data[medianWin] = ride->dataPoints()[dataPntPosn + medianWin + halfMedianWin + 1]->watts;
                }
                else if (dp > numDataPnts - 1) {
                    // Again at the end of the ride, the right-hand side of the median window doesn't align with any ride data, it's
                    // best to avoid a single data point to replicate as this runs the risk of skewing the median filter
                    // so choose some reasonably close ride data to avoid this scenario.
                    data[medianWin] = ride->dataPoints()[dataPntPosn - halfMedianWin - (medianWinSize - medianWin)]->watts;
                }
                else {
                    // The Median window lies completely within the ride data.
                    data[medianWin] = ride->dataPoints()[dp]->watts;
                }
            }

            // Sort entries into ascending numerical order. 
            gsl_sort(data, 1, medianWinSize);

            double medianVal = gsl_stats_median_from_sorted_data(data, 1, medianWinSize);

            // An entry is a fixup candidate if it differs by more than the variance threshold.
            if (fabs(medianVal - wattsAtPnt) < variance) continue; // Note: Only works for two positive numbers.

            // Record spike
            spikes++;
            spiketime += ride->recIntSecs();

            // Fix data point
            ride->command->setPointValue(dataPntPosn, RideFile::watts, medianVal);
        }

        delete[] data;
    }

    ride->command->endLUW();

    ride->setTag("Spikes", QString("%1").arg(spikes));
    ride->setTag("Spike Time", QString("%1").arg(spiketime));

    if (spikes) return true;
    else return false;
}
