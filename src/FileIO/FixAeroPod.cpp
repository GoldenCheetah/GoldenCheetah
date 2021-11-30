/*
 * Copyright (c) 2018 Damien.Grauser@pev-geneve.ch
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
#include <QCheckBox>

// Config widget used by the Preferences/Options config panes
class FixAeroPod;
class FixAeroPodConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixAeroPodConfig)

    friend class ::FixAeroPod;
    protected:
        QHBoxLayout *layout;
        QLabel *paLabel;
        QLabel *percentLabel;
        QLineEdit *pa;
        QCheckBox *hrConv;

    public:
        FixAeroPodConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixMoxy));

	    layout = new QHBoxLayout(this);

	    layout->setContentsMargins(0,0,0,0);
	    setContentsMargins(0,0,0,0);
        //paLabel = new QLabel(tr("Field Adjustment"));

        //hrConv = new QCheckBox(tr("Heartrate to XData.CdA"));

        //layout->addWidget(paLabel);
        //layout->addWidget(hrConv);
	    layout->addStretch();
	}

        //~FixAeroPodConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("When recording with an iBike AeroPod (in HR"
                           " mode) the CdA data is sent as HR. This tool will"
                           " update the activity file to move the values from HR"
                           " into the XData.CdA series."
                           )));
        }
        void readConfig() {
        //bool isHr = appsettings->value(NULL, GC_HR2XDATACDA, Qt::Checked).toBool();
        //hrConv->setCheckState(isHr ? Qt::Checked : Qt::Unchecked);
	    
	}
        void saveConfig() {
        //appsettings->setValue(GC_CAD2SMO2, hrConv->checkState());
	}
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixAeroPod : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixAeroPod)

    public:
        FixAeroPod() {}
        ~FixAeroPod() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixAeroPodConfig(parent);
        }

        // Localized Name
        QString name() {
            return tr("Set XData.CdA from HR");
        }
};

static bool FixAeroPodAdded = DataProcessorFactory::instance().registerProcessor(QString("FixAeroPod"), new FixAeroPod());

bool
FixAeroPod::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    bool isHr = true;

    if (config == NULL) { // being called automatically
        //isHr = appsettings->value(NULL, GC_HR2XDATACDA, Qt::Checked).toBool();
    } else { // being called manually
        //isHr = ((FixMoxyConfig*)(config))->cadConv->checkState();
    }

    // does this ride have power?
    if (ride->areDataPresent()->hr == false)
        return false;

    bool createXData = false;
    int index = -1;
    XDataSeries *aero = ride->xdata("AERO");
    if (aero == NULL) {
        createXData = true;
        aero = new XDataSeries();
        aero->name = "AERO";

        ride->addXData("AERO", aero);
    }

    index = aero->valuename.indexOf("CdA");

    if (index==-1) {
        aero->valuename << "CdA";
        aero->unitname << "m^2";

        index = aero->valuename.length()-1;
    }



    // apply the change
    ride->command->startLUW("Fix AeroPod CdA");
    for (int i=0; i<ride->dataPoints().count(); i++) {
        RideFilePoint *point = ride->dataPoints()[i];

        if (isHr) {
            if (createXData) {
                ride->command->setXDataPointValue("AERO", i, 0, point->secs);
                ride->command->setXDataPointValue("AERO", i, 1, point->km);
            }
            ride->command->setXDataPointValue("AERO", i, index+2, point->hr*4/1000.0);
            ride->command->setPointValue(i, RideFile::hr, 0.00f);
        }
    }

    // shift the data present flags
    if (isHr) {
        ride->command->setDataPresent(RideFile::hr, false);
    }

    ride->command->endLUW();

    return true;
}
