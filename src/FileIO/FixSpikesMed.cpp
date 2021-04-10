/*
 *  * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 *  * Copyright (c) 2021 Paul Johnson ( paulj49457@gmail.com )
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
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

 // Config widget used by the Preferences/Options config panes
class FixSpikesMed;
class FixSpikesMedConfig : public DataProcessorConfig {
	Q_DECLARE_TR_FUNCTIONS(FixSpikesMedConfig)

		friend class ::FixSpikesMed;
protected:
	QHBoxLayout* layout;
	QLabel* medWinLabel, * varianceLabel;
	QSpinBox* medWinSize, * variance;

public:
	FixSpikesMedConfig(QWidget* parent) : DataProcessorConfig(parent) {

		HelpWhatsThis* help = new HelpWhatsThis(parent);
		parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixPowerSpikesMed));

		layout = new QHBoxLayout(this);

		layout->setContentsMargins(0, 0, 0, 0);
		setContentsMargins(0, 0, 0, 0);

		medWinLabel = new QLabel(tr("Median Window Size"));
		varianceLabel = new QLabel(tr("Variance (Watts)"));

		medWinSize = new QSpinBox();
		medWinSize->setMaximum(21);
		medWinSize->setMinimum(5);
		medWinSize->setSingleStep(2);
		medWinSize->setValue(13);

		// Ensure only odd numbers are set for the median window
		connect(medWinSize, QOverload<int>::of(&QSpinBox::valueChanged),
			[=](int i) {(i % 2) ? medWinSize->setValue(i) : medWinSize->setValue(i + 1); });

		variance = new QSpinBox();
		variance->setMaximum(100);
		variance->setMinimum(0);
		variance->setSingleStep(1);
		variance->setValue(10);

		layout->addWidget(medWinLabel);
		layout->addWidget(medWinSize);
		layout->addWidget(varianceLabel);
		layout->addWidget(variance);

		layout->addStretch();
	}

	//~FixSpikesMedConfig() {} // deliberately not declared since Qt will delete
						  // the widget and its children when the config pane is deleted


	QString explain() {
		return(QString(tr("Power meters will occasionally report "
			"erroneously high values for power. For crank based "
			"power meters such as SRM and Quarq this is caused "
			"by an erroneous cadence reading as a result of "
			"triggering a reed switch whilst pushing off.\n\n"

			"This function applies median filtering which is "
			"considered robust to local outliers, and preserves "
			"sharp edges while at the same time removing signal "
			"noise.\n\n"

			"This function will search for spikes/anomalies in the "
			"power data and replace the erroneous data which differs "
			"from the median value of a window centred upon the "
			"erronous data point.\n\n"

			"It takes the following parameters:\n\n"

			"Median Window Size - this defines the number of "
			"neighbouring points used to determine a median "
			"value; the window size is always odd to ensure "
			"we have a central median value.\n\n"

			"Variance Watts - Determines the threshold beyond which a "
			"data point will be fixed, if the difference between the "
			"data point value and the median value exceeds this parameter.\n\n")));
	}

	void readConfig() {
		int mWin = appsettings->value(NULL, GC_DPFSM_MED_WIN, "13").toInt();
		int mVar = appsettings->value(NULL, GC_DPFSM_MED_VARIANCE, "10").toInt();
		medWinSize->setValue(mWin);
		variance->setValue(mVar);
	}

	void saveConfig() {
		appsettings->setValue(GC_DPFSM_MED_WIN, medWinSize->value());
		appsettings->setValue(GC_DPFSM_MED_VARIANCE, variance->value());
	}
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixSpikesMed : public DataProcessor {
	Q_DECLARE_TR_FUNCTIONS(FixSpikesMed)

public:
	FixSpikesMed() {}
	~FixSpikesMed() {}

	// the processor
	bool postProcess(RideFile*, DataProcessorConfig* config, QString op);

	// the config widget
	DataProcessorConfig* processorConfig(QWidget* parent, const RideFile* ride = NULL) {
		Q_UNUSED(ride);
		return new FixSpikesMedConfig(parent);
	}

	// Localized Name
	QString name() {
		return (tr("Fix Power Spikes Median"));
	}
};

static bool FixSpikesMedAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix Power Spikes Median"), new FixSpikesMed());

bool
FixSpikesMed::postProcess(RideFile* ride, DataProcessorConfig* config = 0, QString op = "") {
	Q_UNUSED(op)

		// does this ride have power?
		if (ride->areDataPresent()->watts == false) return false;

	// get settings
	double variance;
	int medianWinSize; // this nummber must be odd to align the centre of the median window with the point being tested/corrected

	if (config == NULL) { // being called automatically
		medianWinSize = appsettings->value(NULL, GC_DPFSM_MED_WIN, "13").toInt();
		variance = appsettings->value(NULL, GC_DPFSM_MED_VARIANCE, "10").toInt();
	}
	else { // being called manually
		medianWinSize = ((FixSpikesMedConfig*)(config))->medWinSize->value();
		variance = ((FixSpikesMedConfig*)(config))->variance->value();
	}

	// We use a median window to find spikes if the ride is
	// shorter than the window don't bother, as there is no way
	// of post processing anyway (e.g. manual workouts)
	if (medianWinSize > ride->dataPoints().count()) return false;

	// Keep track of the power outliers
	int spikes = 0;
	double spiketime = 0.0;

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

	ride->command->endLUW();

	delete[] data;

	ride->setTag("Spikes", QString("%1").arg(spikes));
	ride->setTag("Spike Time", QString("%1").arg(spiketime));

	return (bool)spikes;
}
