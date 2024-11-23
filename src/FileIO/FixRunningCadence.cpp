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
class FixRunningCadence;
class FixRunningCadenceConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixRunningCadenceConfig)

    friend class ::FixRunningCadence;
    protected:
        QHBoxLayout *layout;

    public:
        FixRunningCadenceConfig(QWidget *parent) : DataProcessorConfig(parent) {




        }

        //~FixRunningCadenceConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        void readConfig() { }
        void saveConfig() { }

};


class FixRunningCadence : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixRunningCadence)

    public:
        FixRunningCadence() {}
        ~FixRunningCadence() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op) override;

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) const override {
            Q_UNUSED(ride);
            return new FixRunningCadenceConfig(parent);
        }

        // Localized Name
        QString name() const override {
            return (tr("Fix Running Cadence"));
        }

        QString id() const override {
            return "::FixRunningCadence";
        }

        QString legacyId() const override {
            return "Fix Running Cadence";
        }

        QString explain() const override {
            return tr("Some file report cadence in steps per minutes.\n"
                      "This tools convert to revolutions or cycles per minute");
        }
};

static bool FixRunningCadenceAdded = DataProcessorFactory::instance().registerProcessor(new FixRunningCadence());

bool
FixRunningCadence::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    // does this ride have cadence?
    if (ride->areDataPresent()->cad == false && ride->areDataPresent()->rcad == false) return false;

    ride->command->startLUW("Fix Running Cadence");

    for (int i=0; i<ride->dataPoints().count(); i++) {
        RideFilePoint *p = ride->dataPoints()[i];

        if (p->cad > 0)
            p->cad = p->cad / 2;
        if (p->rcad > 0)
            p->rcad = p->rcad / 2;
    }
    ride->command->endLUW();

    return true;
}
