/*
 * Copyright (c) 2015 Alejandro Martinez (amtriathlon@gmail.com)
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
#include "Colors.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>

#include <cmath>

// Config widget used by the Preferences/Options config panes
class FixSpeed;
class FixSpeedConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixSpeedConfig)

    friend class ::FixSpeed;
    protected:
        QSpinBox *ma;

    public:
        FixSpeedConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixSpeed));

            QFormLayout *layout = newQFormLayout(this);
            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            ma = new QSpinBox();
            ma->setMaximum(600);
            ma->setMinimum(0);
            ma->setSingleStep(1);
            ma->setSuffix(" " + tr("s"));

            layout->addRow(tr("Moving Average"), ma);
        }

        //~FixSpeedConfig() {}  // deliberately not declared since Qt will
                                // delete the widget and its children when
                                // the config pane is deleted

        void readConfig() {
            ma->setValue(appsettings->value(NULL, GC_DPFV_MA, 1).toInt());
        }

        void saveConfig() {
            appsettings->setValue(GC_DPFV_MA, ma->value());
        }
};


// RideFile Dataprocessor -- used to compute Speed from Distance
//                           travelled during each sample
//                           to ensure consistency.
//
class FixSpeed : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixSpeed)

    public:
        FixSpeed() {}
        ~FixSpeed() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op) override;

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) const override {
            Q_UNUSED(ride);
            return new FixSpeedConfig(parent);
        }

        // Localized Name
        QString name() const override {
            return tr("Fix Speed from Distance");
        }

        QString id() const override {
            return "::FixSpeed";
        }

        QString legacyId() const override {
            return "Fix Speed from Distance";
        }

        QString explain() const override {
            return tr("Some devices report sample speed not matching the "
                      "change of distance, for example when using a footpod "
                      "for speed and GPS for distance.\n"
                      "This tool replaces the existing speed channel, or "
                      "creates a new one if not present, based on travelled distance\n\n"
                      "Moving Average Seconds parameter allows to set the "
                      "seconds of the MA filter to smooth speed spikes");
        }
};

static bool FixSpeedAdded = DataProcessorFactory::instance().registerProcessor(new FixSpeed());

bool
FixSpeed::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // get settings
    int ma;
    if (config == NULL) { // being called automatically
        ma = appsettings->value(NULL, GC_DPFV_MA, 1).toInt();
    } else { // being called manually
        ma = ((FixSpeedConfig*)(config))->ma->value();
    }

    // no dice if we don't have Distance
    if (!ride->areDataPresent()->km) return false;

    if(ride->recIntSecs() == 0) return false;

    int rollingwindowsize = ma / ride->recIntSecs();
    if (rollingwindowsize < 1) rollingwindowsize = 1;
    QVector<double> rolling(rollingwindowsize);
    int index = 0;
    double sum = 0;

    // apply the change
    bool changed=false;

    double secs = 0.0;
    double km = 0.0;
    for (int i=0; i< ride->dataPoints().count(); i++) {

        RideFilePoint *p = ride->dataPoints()[i];

        // Estimate Speed from travelled distance
        double kph = p->kph;
        if (p->secs - secs > 0) kph = 3600 * (p->km - km) / (p->secs - secs);
        else if (p->secs == 0) kph = 0;

        // compute rolling average for rollingwindowsize seconds
        sum += kph;
        sum -= rolling[index];
        rolling[index] = kph;
        kph = sum/std::min(i+1, rollingwindowsize);
        // move index on/round
        index = (index >= rollingwindowsize-1) ? 0 : index+1;

        // If different enough, update
        if (std::abs(kph - p->kph) > 10e-6) {
            // ok, lets start a luw if not already changed
            if (!changed) {
                changed = true;
                ride->command->startLUW("Fix Speed from Distance");
            }
            ride->command->setPointValue(i, RideFile::kph, kph);
        }

        // update accumulated time and distance
        secs = p->secs;
        km = p->km;
    }

    if (changed || !ride->areDataPresent()->kph) {
        ride->setDataPresent(ride->kph, true);
        ride->command->endLUW();
        return true;
    }

    return false;
}
