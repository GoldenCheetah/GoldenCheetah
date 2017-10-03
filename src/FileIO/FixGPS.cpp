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
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include "FixGPS.h"
#include <algorithm>
#include <QVector>

// Config widget used by the Preferences/Options config panes
class FixGPS;
class FixGPSConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixGPSConfig)
    friend class ::FixGPS;
    protected:
        QHBoxLayout *layout;
        QCheckBox *adjustSpeed;
    public:
        // there is no config
        FixGPSConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixGPSErrors));

            layout = new QHBoxLayout(this);
            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            adjustSpeed = new QCheckBox(tr("Adjust distance and speed"));
            layout->addWidget(adjustSpeed);
            layout->addStretch();
        }

        QString explain() {
            return(QString(tr("Remove GPS errors and interpolate positional "
                           "data where the GPS device did not record any data, "
                           "or the data that was recorded is invalid.")));
        }

        void readConfig() {
            bool SetSpeed = appsettings->value(NULL, GC_DPGPS_SPEED, Qt::Unchecked).toBool();
            adjustSpeed->setCheckState(SetSpeed ? Qt::Checked : Qt::Unchecked);
        }
        void saveConfig() {
            appsettings->setValue(GC_DPGPS_SPEED, adjustSpeed->checkState());
        }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixGPS : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixGPS)

    public:
        FixGPS() {}
        ~FixGPS() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig*settings=0, QString op="");

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixGPSConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix GPS errors"));
        }
};

static bool fixGPSAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix GPS errors"), new FixGPS());

bool
FixGPS::postProcess(RideFile *ride, DataProcessorConfig *config, QString op)
{
    Q_UNUSED(op)

    // ignore null or files without GPS data
    if (!ride || ride->areDataPresent()->lat == false || ride->areDataPresent()->lon == false)
        return false;

    bool setSpeed;

    if (config == NULL) { // beging called automatically
        setSpeed = appsettings->value(NULL, GC_DPGPS_SPEED, Qt::Unchecked).toBool();
    }
    else { // being called manually
        setSpeed = (bool) ((FixGPSConfig*)(config))->adjustSpeed->checkState();
    }

    ride->command->startLUW("Fix GPS Errors");

    int errors=0;
    int lastgood = -1;  // where did we last have decent GPS data?
    for (int i=0; i<ride->dataPoints().count(); i++) {
        // is this one decent?
        if (ride->dataPoints()[i]->lat && ride->dataPoints()[i]->lat >= double(-90) && ride->dataPoints()[i]->lat <= double(90) &&
            ride->dataPoints()[i]->lon && ride->dataPoints()[i]->lon >= double(-180) && ride->dataPoints()[i]->lon <= double(180)) {

            if (lastgood != -1 && (lastgood+1) != i) {
                // interpolate from last good to here
                // then set last good to here
                double deltaLat = (ride->dataPoints()[i]->lat - ride->dataPoints()[lastgood]->lat) / double(i-lastgood);
                double deltaLon = (ride->dataPoints()[i]->lon - ride->dataPoints()[lastgood]->lon) / double(i-lastgood);

                double _dist, _old_dist, _avg_speed;

                if (setSpeed) {
                    _old_dist = ride->dataPoints()[lastgood]->km - ride->dataPoints()[i]->km;
                    if (deltaLat == 0.0 && deltaLon == 0.0){
                        _dist = 0.0;
                    }
                    else {
                        _dist = sin(_deg2rad(ride->dataPoints()[lastgood]->lat)) * sin(_deg2rad(ride->dataPoints()[i]->lat));
                        _dist += cos(_deg2rad(ride->dataPoints()[lastgood]->lat)) * cos(_deg2rad(ride->dataPoints()[i]->lat)) * cos(_deg2rad(deltaLon));
                        _dist = acos(_dist) * 6371;
                    }
                    _avg_speed = _dist / (ride->dataPoints()[i]->secs - ride->dataPoints()[lastgood]->secs) * 3600;
                    _dist /= double(i-lastgood);
                }

                for (int j=lastgood+1; j<i; j++) {
                    if (setSpeed) {
                        ride->command->setPointValue(j, RideFile::kph, _avg_speed);
                        ride->command->setPointValue(j, RideFile::km, ride->dataPoints()[lastgood]->km + (double(j-lastgood)*_dist));
                    }
                    ride->command->setPointValue(j, RideFile::lat, ride->dataPoints()[lastgood]->lat + (double(j-lastgood)*deltaLat));
                    ride->command->setPointValue(j, RideFile::lon, ride->dataPoints()[lastgood]->lon + (double(j-lastgood)*deltaLon));
                    errors++;
                }
                if (setSpeed && errors>0) {
                    for (int j=i; j<ride->dataPoints().count(); j++) {
                        ride->command->setPointValue(j, RideFile::km, ride->dataPoints()[j]->km + _dist - _old_dist);
                    }
                }
            } else if (lastgood == -1) {
                // fill to front
                for (int j=0; j<i; j++) {
                    ride->command->setPointValue(j, RideFile::lat, ride->dataPoints()[i]->lat);
                    ride->command->setPointValue(j, RideFile::lon, ride->dataPoints()[i]->lon);
                    errors++;
                }
            }
            lastgood = i;
        }
    }

    // fill to end...
    if (lastgood != -1 && lastgood != (ride->dataPoints().count()-1)) {
       // fill from lastgood to end with lastgood
        for (int j=lastgood+1; j<ride->dataPoints().count(); j++) {
            ride->command->setPointValue(j, RideFile::lat, ride->dataPoints()[lastgood]->lat);
            ride->command->setPointValue(j, RideFile::lon, ride->dataPoints()[lastgood]->lon);
            errors++;
        }
    } 
    ride->command->endLUW();

    if (errors) {
        ride->setTag("GPS errors", QString("%1").arg(errors));
        return true;
    } else
        return false;
}
