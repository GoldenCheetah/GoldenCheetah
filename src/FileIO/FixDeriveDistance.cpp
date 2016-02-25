/*
 * Copyright (c) 2015 Damien Grauser (Damien.Grauser@gmail.com)
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
#include "Units.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>

#define pi 3.14159265358979323846

// Config widget used by the Preferences/Options config panes
class FixDeriveDistance;
class FixDeriveDistanceConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixDeriveDistanceConfig)

    friend class ::FixDeriveDistance;
    protected:
        QHBoxLayout *layout;
        QLabel *bikeWeightLabel;
        QDoubleSpinBox *bikeWeight;
        QLabel *crrLabel;
        QDoubleSpinBox *crr;

    public:
        FixDeriveDistanceConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_EstimateDistanceValues));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            layout->addStretch();
        }

        //~FixDeriveDistanceConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        void readConfig() {
        }

        void saveConfig() {
        }

        QString explain() {
            return(QString(tr("Derive distance based on "
                              "GPS position\n\n"
                              "Some unit doesn't record distance without "
                              "a speedometer but record position "
                              "(eg Timex Cycle Trainer)\n\n")));
        }

};


// RideFile Dataprocessor -- Derive estimated distance based on GPS position
//
class FixDeriveDistance : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixDeriveDistance)

    public:
        FixDeriveDistance() {}
        ~FixDeriveDistance() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixDeriveDistanceConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Estimate Distance Values"));
        }
};

static bool FixDeriveDistanceAdded = DataProcessorFactory::instance().registerProcessor(QString("Estimate Distance Values"), new FixDeriveDistance());

double _deg2rad(double deg) {
  return (deg * pi / 180);
}

bool
FixDeriveDistance::postProcess(RideFile *ride, DataProcessorConfig *config=0)
{
    Q_UNUSED(config);

    // if its already there do nothing !
    if (ride->areDataPresent()->km) return false;

    // no dice if we don't have alt and speed
    if (!ride->areDataPresent()->lat) return false;

    // apply the change
    ride->command->startLUW("Estimate Distance");

    if (ride->areDataPresent()->lat) {
        double km = 0.0;
        double lastLat = 0;
        double lastLon = 0;

        for (int i=0; i<ride->dataPoints().count(); i++) {
            RideFilePoint *p = ride->dataPoints()[i];

            double _theta, _dist;
            _theta = lastLon - p->lon;
            if (lastLat == 0.0 || lastLon == 0.0 || p->lat == 0.0 || p->lon == 0.0 || (_theta == 0.0 && (lastLat - p->lat) == 0.0)) {
                 _dist = 0;
            }
            else {
                _dist = sin(_deg2rad(lastLat)) * sin(_deg2rad(p->lat)) + cos(_deg2rad(lastLat)) * cos(_deg2rad(p->lat)) * cos(_deg2rad(_theta));
                _dist = acos(_dist) * 6371;
            }
            if (std::isnan(_dist) == false && _dist<0.05)
                km += _dist;
            lastLat = p->lat;
            lastLon = p->lon;

            ride->command->setPointValue(i, RideFile::km, km);
        }

        ride->setDataPresent(ride->km, true);

        // update the change history
        QString log = ride->getTag("Change History", "");
        log +=  tr("Derive Distance from GPS on ");
        log +=  QDateTime::currentDateTime().toString() + ":";
        log += '\n' + ride->command->changeLog();
        ride->setTag("Change History", log);
    }

    ride->command->endLUW();

    return true;
}

