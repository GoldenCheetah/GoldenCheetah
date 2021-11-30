/*
 * Copyright (c) 2015 Damien Grauser
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
class FixSmO2;
class FixSmO2Config : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixSmO2Config)

    friend class ::FixSmO2;
    protected:
        QHBoxLayout *layout;
        QCheckBox *fixSmO2Box, *fixtHbBox;
        QLabel *maxtHbLabel;
        QDoubleSpinBox *maxtHbInput;

    public:
        FixSmO2Config(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixSmO2));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            fixSmO2Box = new QCheckBox(tr("Fix SmO2"));
            fixtHbBox = new QCheckBox(tr("Fix tHb"));
            maxtHbLabel = new QLabel(tr("Max. tHb"));

            maxtHbInput = new QDoubleSpinBox();
            maxtHbInput->setMaximum(100);
            maxtHbInput->setMinimum(0);
            maxtHbInput->setSingleStep(1);
            maxtHbInput->setDecimals(2);

            layout->addWidget(fixSmO2Box);
            layout->addWidget(fixtHbBox);
            layout->addWidget(maxtHbLabel);
            layout->addWidget(maxtHbInput);

            layout->addStretch();

        }

        //~FixSmO2Config() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Occasionally SmO2 (%) and/or tHb (%) will erroneously "
                           "report missing or high values (SmO2: 0% or >100% / tHb: 0% or > max. tHb parameter). \n\n "
                           "This function will look for those anomalies "
                           "in SmO2 and tHb data and depending on the configuration replace the erroneous data "
                           "by smoothing/interpolating the data from either "
                           "side of the 3 points in question. It takes the following parameters:\n\n"
                           "Fix SmO2 - check to fix anomalies in SmO2 data \n"
                           "Fix tHb - check to fix anomalies in tHb data \n"
                           "Max. tHb - any tHb above is considered an outlier \n")));


        }

        void readConfig() {
            bool fixSmO2 = appsettings->value(NULL, GC_MOXY_FIX_SMO2, Qt::Checked).toBool();
            bool fixtHb = appsettings->value(NULL, GC_MOXY_FIX_THB, Qt::Checked).toBool();
            double maxtHb = appsettings->value(NULL, GC_MOXY_FIX_THB_MAX, "25.0").toDouble();
            fixSmO2Box->setCheckState(fixSmO2 ? Qt::Checked : Qt::Unchecked);
            fixtHbBox->setCheckState(fixtHb ? Qt::Checked : Qt::Unchecked);
            maxtHbInput->setValue(maxtHb);


        }
        void saveConfig() {
            appsettings->setValue(GC_MOXY_FIX_SMO2, fixSmO2Box->checkState());
            appsettings->setValue(GC_MOXY_FIX_THB, fixtHbBox->checkState());
            appsettings->setValue(GC_MOXY_FIX_THB_MAX, maxtHbInput->value());
        }

};


class FixSmO2 : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixSmO2)

    public:
        FixSmO2() {}
        ~FixSmO2() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixSmO2Config(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix SmO2 and/or tHb Anomaly"));
        }
};

static bool fixSmO2Added = DataProcessorFactory::instance().registerProcessor(QString("Fix SmO2/tHb Anomaly"), new FixSmO2());

bool
FixSmO2::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)
    
    // get settings
    bool fixSmO2;
    bool fixtHb;
    double maxtHb;


    if (config == NULL) { // being called automatically
        fixSmO2 = appsettings->value(NULL, GC_MOXY_FIX_SMO2, Qt::Checked).toBool();
        fixtHb = appsettings->value(NULL, GC_MOXY_FIX_THB, Qt::Checked).toBool();
        maxtHb = appsettings->value(NULL, GC_MOXY_FIX_THB_MAX, "50.0").toDouble();

    } else { // being called manually
        fixSmO2 = ((FixSmO2Config*)(config))->fixSmO2Box->isChecked();
        fixtHb = ((FixSmO2Config*)(config))->fixtHbBox->isChecked();
        maxtHb = ((FixSmO2Config*)(config))->maxtHbInput->value();
    }

    int smO2spikes = 0;
    int tHbspikes = 0;

    // we have SmO2 and want to fix it
    if (ride->areDataPresent()->smo2 && fixSmO2) {

        // get settings
        double min = 0;
        double max = 100;

        ride->command->startLUW("Fix SmO2 in Recording");

        for (int i=0; i<ride->dataPoints().count(); i++) {

            // the min/max range value we are willing to accept?
            if (ride->dataPoints()[i]->smo2 > min && ride->dataPoints()[i]->smo2 < max) continue;

            // Value is 0 or > 100

            // which one is it
            double left=0.0, right=0.0;

            if (i >= 0)  {
                int nb = 0;
                for (int j=1; j<4 && i-j >= 0; j++) {
                    if (ride->dataPoints()[i-j]->smo2 > min && ride->dataPoints()[i-j]->smo2 < max) {
                        left += ride->dataPoints()[i-j]->smo2;
                        nb++;
                    }
                }
                if (nb > 0)
                    left = left / nb;
            }
            if (i < (ride->dataPoints().count()))  {
                int nb = 0;
                for (int j = 1; j < 4 && i + j < ride->dataPoints().count(); j++) {
                    if (ride->dataPoints()[i + j]->smo2 > min && ride->dataPoints()[i + j]->smo2 < max) {
                        right += ride->dataPoints()[i + j]->smo2;
                        nb++;
                    }
                }
                if (nb > 0)
                    right = right / nb;
            }

            if (left != 0.0 && right != 0.0 && (left+right)/2.0 != ride->dataPoints()[i]->smo2) {
                smO2spikes++;
                ride->command->setPointValue(i, RideFile::smo2, (left+right)/2.0);

            }
        }
        ride->command->endLUW();

    } // end smO2

    // we have tHb and want to fix it
    if (ride->areDataPresent()->thb && fixtHb) {

        // get settings
        double min = 0;
        double max = maxtHb;

        ride->command->startLUW("Fix tHb in Recording");

        for (int i=0; i<ride->dataPoints().count(); i++) {

            // the min/max range value we are willing to accept?
            if (ride->dataPoints()[i]->thb > min && ride->dataPoints()[i]->thb < max) continue;

            // Value is 0 or > maxtHb

            // which one is it
            double left=0.0, right=0.0;

            if (i >= 0)  {
                int nb = 0;
                for (int j=1; j<4 && i-j >= 0; j++) {
                    if (ride->dataPoints()[i-j]->thb>min && ride->dataPoints()[i-j]->thb<max) {
                        left += ride->dataPoints()[i-j]->thb;
                        nb++;
                    }
                }
                if (nb > 0)
                    left = left / nb;
            }
            if (i < (ride->dataPoints().count()))  {
                int nb = 0;
                for (int j = 1; j < 4 && i + j < ride->dataPoints().count();
                     j++) {
                    if (ride->dataPoints()[i + j]->thb > min &&
                            ride->dataPoints()[i + j]->thb < max) {
                        right += ride->dataPoints()[i + j]->thb;
                        nb++;
                    }
                }
                if (nb > 0)
                    right = right / nb;
            }

            if (left != 0.0 && right != 0.0 && (left+right)/2.0 != ride->dataPoints()[i]->thb) {
                tHbspikes++;
                ride->command->setPointValue(i, RideFile::thb, (left+right)/2.0);

            }
        }
        ride->command->endLUW();


    } // end tHb


    if (smO2spikes > 0 || tHbspikes > 0) return true;
    else return false;
}
