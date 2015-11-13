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
class FixMoxy;
class FixMoxyConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixMoxyConfig)

    friend class ::FixMoxy;
    protected:
        QHBoxLayout *layout;
        QLabel *paLabel;
        QLabel *percentLabel;
        QLineEdit *pa;

    public:
        FixMoxyConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixMoxy));
        }

        //~FixMoxyConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("When recording from the Moxy or BSX Insight in Speed and"
                           " cadence mode the SmO2 and tHb data is sent as"
                           " cadence and speed respectively. This tool will"
                           " update the activity file to move the values from speed"
                           " and cadence into the Moxy series."
                           )));
        }

        void readConfig() { }
        void saveConfig() { }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixMoxy : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixMoxy)

    public:
        FixMoxy() {}
        ~FixMoxy() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixMoxyConfig(parent);
        }

        // Localized Name
        QString name() {
            return tr("Set SmO2/tHb from Speed and Cadence");
        }
};

static bool FixMoxyAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix Moxy"), new FixMoxy());

bool
FixMoxy::postProcess(RideFile *ride, DataProcessorConfig *config=0)
{
    Q_UNUSED(config);

    // does this ride have power?
    if (ride->areDataPresent()->kph == false || ride->areDataPresent()->cad == false) return false;

    // apply the change
    ride->command->startLUW("Fix Moxy");
    for (int i=0; i<ride->dataPoints().count(); i++) {
        RideFilePoint *point = ride->dataPoints()[i];

        ride->command->setPointValue(i, RideFile::smo2, point->cad);
        ride->command->setPointValue(i, RideFile::thb, point->kph);
        ride->command->setPointValue(i, RideFile::cad, 0.00f);
        ride->command->setPointValue(i, RideFile::kph, 0.00f);
    }

    // shift the data present flags
    ride->command->setDataPresent(RideFile::smo2, true);
    ride->command->setDataPresent(RideFile::thb, true);
    ride->command->setDataPresent(RideFile::cad, false);
    ride->command->setDataPresent(RideFile::kph, false);
    ride->command->endLUW();

    return true;
}
