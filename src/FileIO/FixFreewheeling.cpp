/*
 * Copyright (c) 2015 Mark Liversedge
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
class FixFreewheeling;
class FixFreewheelingConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixFreewheelingConfig)

    friend class ::FixFreewheeling;
    protected:

    public:
        FixFreewheelingConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixFreewheeling));
        }

        //~FixFreewheelingConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("ANT+ crank based power meters will send "
                           " 3 duplicate values for power and cadence when the "
                           " rider starts to freewheel. The duplicates should "
                           " not be retained. This tool removes the duplicates."
                           )));
        }

        void readConfig() { }
        void saveConfig() { }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixFreewheeling : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixFreewheeling)

    public:
        FixFreewheeling() {}
        ~FixFreewheeling() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixFreewheelingConfig(parent);
        }

        // Localized Name
        QString name() {
            return tr("Fix freewheeling power/cadence.");
        }
};

static bool FixFreewheelingAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix Freewheeling"), new FixFreewheeling());

bool
FixFreewheeling::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    // does this ride have power and cadence ?
    if (ride->areDataPresent()->watts == false || ride->areDataPresent()->cad == false) return false;

    // apply the change
    ride->command->startLUW("Fix Freewheeling");
    RideFilePoint *last = NULL;
    for (int i=0; i<ride->dataPoints().count(); i++) {

        RideFilePoint *point = ride->dataPoints()[i];

        // ooh we just started freewheeling
        if (i > 2 && point->cad == 0 && point->watts == 0 && last->cad != 0 && last->watts != 0) {

            // check there are 3 duplicated values
            if (ride->dataPoints()[i-3]->watts == ride->dataPoints()[i-2]->watts &&
                ride->dataPoints()[i-3]->cad == ride->dataPoints()[i-2]->cad &&
                ride->dataPoints()[i-2]->watts == ride->dataPoints()[i-1]->watts &&
                ride->dataPoints()[i-2]->cad == ride->dataPoints()[i-1]->cad) {

                // set previous 2 back to zero
                ride->command->setPointValue(i-2, RideFile::cad, 0.00f);
                ride->command->setPointValue(i-1, RideFile::cad, 0.00f);
                
                ride->command->setPointValue(i-2, RideFile::watts, 0.00f);
                ride->command->setPointValue(i-1, RideFile::watts, 0.00f);
            }

        }

        // remember the last one
        last = point;
    }

    // process LOW
    ride->command->endLUW();

    return true;
}
