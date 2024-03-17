/*
 * Copyright (c) 2014 Jon Beverley (jon@csdl.biz)
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
class FixPower;
class FixPowerConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixPowerConfig)

    friend class ::FixPower;
    protected:
        QHBoxLayout *layout;
        QLabel *relPaLabel, *absPaLabel;
        QLabel *percentLabel,*wattLabel;
        QLineEdit *paRel, *paAbs;

    public:
        FixPowerConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_AdjustPowerValues));

            layout = new QHBoxLayout(this);
            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            relPaLabel = new QLabel(tr("Percent Adjustment"));
            paRel = new QLineEdit();
            percentLabel = new QLabel(tr("%"));

            absPaLabel = new QLabel(tr("Fix Adjustment"));
            paAbs = new QLineEdit();
            wattLabel = new QLabel(tr("watt"));

            layout->addWidget(relPaLabel);
            layout->addWidget(paRel);
            layout->addWidget(percentLabel);
            layout->addStretch();

            layout->addWidget(absPaLabel);
            layout->addWidget(paAbs);
            layout->addWidget(wattLabel);


        }

        //~FixPowerConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Adjusting power values allows you to "
                           "uplift or degrade the power values by a "
                           "percentage and/or a fix value. It takes two parameters:\n\n"
                           "Percent Adjustment - this defines percentage "
                           " to modify values by. Negative values are supported.\n\n"
                           "Fix Adjustment - this defines an fix amount "
                           " to modify values by. Negative values are supported.\n\n"
                           "If both parameters are given, first the relative adjustment "
                           "takes place, then the fix value adjustment is applied on the result."
                           )));
        }

        void readConfig() {
            paRel->setText(appsettings->value(NULL, GC_DPPA, "0").toString());
            paAbs->setText(appsettings->value(NULL, GC_DPPA_ABS, "0").toString());
        }

        void saveConfig() {
            appsettings->setValue(GC_DPPA, paRel->text());
            appsettings->setValue(GC_DPPA_ABS, paAbs->text());

        }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixPower : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixPower)

    public:
        FixPower() {}
        ~FixPower() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixPowerConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Adjust Power Values"));
        }
};

static bool FixPowerAdded = DataProcessorFactory::instance().registerProcessor(QString("Adjust Power Values"), new FixPower());

bool
FixPower::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // Lets do it then!
    QString tpRel, tpAbs;
    double percentageAdjust = 0;
    double absoluteAdjust = 0;

    if (config == NULL) { // being called automatically
        tpRel = appsettings->value(NULL, GC_DPPA, "0").toString();
        tpAbs = appsettings->value(NULL, GC_DPPA_ABS, "0").toString();
    } else { // being called manually
        tpRel = ((FixPowerConfig*)(config))->paRel->text();
        tpAbs = ((FixPowerConfig*)(config))->paAbs->text();

    }

    percentageAdjust = tpRel.toDouble();
    absoluteAdjust = tpAbs.toDouble();

    // does this ride have power?
    if (ride->areDataPresent()->watts == false) return false;

    // no adjustment required
    if ((percentageAdjust == 0) && (absoluteAdjust == 0)) return false;

    // apply the change
    ride->command->startLUW("Adjust Power");
    for (int i=0; i<ride->dataPoints().count(); i++) {
        RideFilePoint *point = ride->dataPoints()[i];
        double newWatts = point->watts;
        // only add/adjust if we have a value > 0
        if (point->watts != 0 && percentageAdjust != 0) {
            newWatts += (newWatts * (percentageAdjust / 100));
        }
        // only add/adjust if we have a value > 0
        if (point->watts != 0 && absoluteAdjust != 0) {
            newWatts += absoluteAdjust;
        }
        if (newWatts != point->watts) {
           ride->command->setPointValue(i, RideFile::watts, newWatts);
        }

    }
    ride->command->endLUW();

    double currentta = ride->getTag("Power Adjust", "0.0").toDouble();
    ride->setTag("Power Adjust", QString("%1").arg(currentta + percentageAdjust));
    double currenttaAbs = ride->getTag("Power Adjust fix", "0.0").toDouble();
    ride->setTag("Power Adjust fix", QString("%1").arg(currenttaAbs + absoluteAdjust));


    return true;
}
