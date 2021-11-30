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
class FixDeriveTorque;
class FixDeriveTorqueConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixDeriveTorqueConfig)

    friend class ::FixDeriveTorque;
    protected:
        //QHBoxLayout *layout;
        //QLabel *taLabel;
        //QLineEdit *ta;

    public:
        FixDeriveTorqueConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_AddTorqueValues));

            //layout = new QHBoxLayout(this);

            //layout->setContentsMargins(0,0,0,0);
            //setContentsMargins(0,0,0,0);

            //taLabel = new QLabel(tr("Torque Adjust"));

            //ta = new QLineEdit();

            //layout->addWidget(taLabel);
            //layout->addWidget(ta);
            //layout->addStretch();
        }

        //~FixDeriveTorqueConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Derive torque when power and cadence data is available.")));
        }

        void readConfig() {
            //ta->setText(appsettings->value(NULL, GC_DPTA, "0 nm").toString());
        }

        void saveConfig() {
            //appsettings->setValue(GC_DPTA, ta->text());
        }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixDeriveTorque : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixDeriveTorque)

    public:
        FixDeriveTorque() {}
        ~FixDeriveTorque() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixDeriveTorqueConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Add Torque Values"));
        }
};

static bool FixDeriveTorqueAdded = DataProcessorFactory::instance().registerProcessor(QString("Add Torque Values"), new FixDeriveTorque());

bool
FixDeriveTorque::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    static const double PI = 3.1415927f;

    // if its already there do nothing !
    if (ride->areDataPresent()->nm) return false;

    // no dice if we don't have power and cadence
    if (!ride->areDataPresent()->watts || !ride->areDataPresent()->cad) return false;

    // apply the change
    bool changed=false;

    for (int i=0; i< ride->dataPoints().count(); i++) {
   
        RideFilePoint *p = ride->dataPoints()[i]; 

        // Estimate Power if not in data
        if (p->cad > 0 && p->watts > 0) {

            // ok, lets start a luw if not already changed
            if (!changed) {
                changed = true;
                ride->command->startLUW("Add Torque Values");
            }

            double torque = p->watts * 60 / ( 2 * PI * p->cad);
            ride->command->setPointValue(i, RideFile::nm, torque);
        }
    }

    if (changed) {
        ride->setDataPresent(ride->nm, true);
        ride->command->endLUW();
        return true;
    }

    return false;
}
