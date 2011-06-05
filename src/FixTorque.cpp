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
class FixTorque;
class FixTorqueConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixTorqueConfig)

    friend class ::FixTorque;
    protected:
        QHBoxLayout *layout;
        QLabel *taLabel;
        QLineEdit *ta;

    public:
        FixTorqueConfig(QWidget *parent) : DataProcessorConfig(parent) {

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            taLabel = new QLabel(tr("Torque Adjust"));

            ta = new QLineEdit();

            layout->addWidget(taLabel);
            layout->addWidget(ta);
            layout->addStretch();
        }

        //~FixTorqueConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Adjusting torque values allows you to "
                           "uplift or degrade the torque values when the calibration "
                           "of your power meter was incorrect. It "
                           "takes a single parameter:\n\n"
                           "Torque Adjust - this defines an absolute value "
                           "in poinds per square inch or newton meters to "
                           "modify values by. Negative values are supported. (e.g. enter \"1.2 nm\" or "
                           "\"-0.5 pi\").")));
        }

        void readConfig() {
            boost::shared_ptr<QSettings> settings = GetApplicationSettings();
            ta->setText(settings->value(GC_DPTA, "0 nm").toString());
        }

        void saveConfig() {
            boost::shared_ptr<QSettings> settings = GetApplicationSettings();
            settings->setValue(GC_DPTA, ta->text());
        }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixTorque : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixTorque)

    public:
        FixTorque() {}
        ~FixTorque() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixTorqueConfig(parent);
        }
        // Localized Name
        QString name() {
            return (tr("Adjust Torque Values"));
        }
};

static bool fixTorqueAdded = DataProcessorFactory::instance().registerProcessor((QString("Adjust Torque Values")), new FixTorque());

bool
FixTorque::postProcess(RideFile *ride, DataProcessorConfig *config=0)
{
    // does this ride have torque?
    if (ride->areDataPresent()->nm == false) return false;

    // Lets do it then!
    QString ta;
    double nmAdjust;

    if (config == NULL) { // being called automatically
        boost::shared_ptr<QSettings> settings = GetApplicationSettings();
        ta = settings->value(GC_DPTA, "0 nm").toString();
    } else { // being called manually
        ta = ((FixTorqueConfig*)(config))->ta->text();
    }

    // patrick's torque adjustment code
    bool pi = ta.endsWith("pi", Qt::CaseInsensitive);
    if (pi || ta.endsWith("nm", Qt::CaseInsensitive)) {
        nmAdjust = ta.left(ta.length() - 2).toDouble();
        if (pi) {
            nmAdjust *= 0.11298482933;
        }
    } else {
        nmAdjust = ta.toDouble();
    }

    // no adjustment required
    if (nmAdjust == 0) return false;

    // apply the change
    ride->command->startLUW("Adjust Torque");
    for (int i=0; i<ride->dataPoints().count(); i++) {
        RideFilePoint *point = ride->dataPoints()[i];

      if (point->nm != 0) {
            double newnm = point->nm + nmAdjust;
            ride->command->setPointValue(i, RideFile::watts, point->watts * (newnm / point->nm));
            ride->command->setPointValue(i, RideFile::nm, newnm);
        }
    }
    ride->command->endLUW();

    double currentta = ride->getTag("Torque Adjust", "0.0").toDouble();
    ride->setTag("Torque Adjust", QString("%1 nm").arg(currentta + nmAdjust));

    return true;
}
