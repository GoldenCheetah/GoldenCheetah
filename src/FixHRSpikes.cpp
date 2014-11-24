/*
 * Copyright (c) 2013 Jaime Jofre (jaimeajofre@hotmail.com)
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 * Based on Mark Liversedge's FixGPS.cpp
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
class FixHRSpikes;
class FixHRSpikesConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixHRSpikesConfig)

    friend class ::FixHRSpikes;
    protected:
        QHBoxLayout *layout;
        QLabel *maxLabel, *varianceLabel;
        QDoubleSpinBox *max,
                       *variance;

    public:
        FixHRSpikesConfig(QWidget *parent) : DataProcessorConfig(parent) {

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            maxLabel = new QLabel(tr("Max"));

            max = new QDoubleSpinBox();
            max->setMaximum(220.00);
            max->setMinimum(0);
            max->setSingleStep(1);

            layout->addWidget(maxLabel);
            layout->addWidget(max);
            layout->addStretch();
        }

        //~FixHRSpikesConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Occasionally heart rate sensors will erroneously "
                           "report high values for heart rate or drop out (0). "
                           "This function will look for spikes and dropouts "
                           "in heart rate data and replace the erroneous data "
                           "by interpolating the data from either "
                           "side of the point in question\n\n"
                           "It takes the following parameter:\n\n"
                           "Absolute Max - this defines an absolute value "
                           "for heart rates, and will smooth any values above this "
                           "absolute value that have been identified as being "
			   "anomalies (i.e. at odds with the data surrounding it).")));
        }

        void readConfig() {
            double tol = appsettings->value(NULL, GC_DPFHRS_MAX, "220").toDouble();
            max->setValue(tol);
        }

        void saveConfig() {
            appsettings->setValue(GC_DPFHRS_MAX, max->value());
        }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixHRSpikes : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixHRSpikes)

    public:
        FixHRSpikes() {}
        ~FixHRSpikes() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixHRSpikesConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix HR Spikes"));
        }
};

static bool fixHRSpikesAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix HR Spikes"), new FixHRSpikes());

bool
FixHRSpikes::postProcess(RideFile *ride, DataProcessorConfig *config=0)
{
    // does this ride have heart rate data?
    if (ride->areDataPresent()->hr == false) return false;

    // get settings
    double max;
    if (config == NULL) { // being called automatically
        max = appsettings->value(NULL, GC_DPFHRS_MAX, "200").toDouble();
    } else { // being called manually
        max = ((FixHRSpikesConfig*)(config))->max->value();
    }

    // Find the HR outliers
    int spikes = 0;
    double spiketime = 0.0;

    ride->command->startLUW("Fix Spikes in Recording"); // Start LogicalUnitOfWork

    int lastgood = -1;  // where did we last have decent HR data?
    for (int i=0; i<ride->dataPoints().count(); i++) {
      // If we have a non-zero HR that is not above the specified MAX
      if(ride->dataPoints()[i]->hr > 0 and ride->dataPoints()[i]->hr <= max) {
	if (lastgood != -1 && (lastgood+1) != i) {
	  // interpolate from last good to here
	  double deltaHR = (ride->dataPoints()[i]->hr - ride->dataPoints()[lastgood]->hr) / double(i-lastgood);

	  for (int j=lastgood+1; j<i; j++) {
	    // Round as fractional HR is not very useful
	    ride->command->setPointValue(j, RideFile::hr, ride->dataPoints()[lastgood]->hr + round(double(j-lastgood)*deltaHR));
	    spikes++;
	  }
	} else if (lastgood == -1) {
	  // fill to front
	  for (int j=0; j<i; j++) {
	    ride->command->setPointValue(j, RideFile::hr, ride->dataPoints()[i]->hr);
	    spikes++;
	  }
	}
	lastgood = i;		// Set lastgood to here
        spiketime += ride->recIntSecs();
      }
    }
    // fill to end...
    if (lastgood != -1 && lastgood != (ride->dataPoints().count()-1)) {
       // fill from lastgood to end with lastgood
        for (int j=lastgood+1; j<ride->dataPoints().count(); j++) {
            ride->command->setPointValue(j, RideFile::hr, ride->dataPoints()[lastgood]->hr);
            spikes++;
        }
    }

    ride->command->endLUW();	// End of LogicalUnitOfWork

    ride->setTag("Spikes", QString("%1").arg(spikes));
    ride->setTag("Spike Time", QString("%1").arg(spiketime));

    if (spikes) return true;
    else return false;
}
