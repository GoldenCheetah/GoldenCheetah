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
#include <algorithm>
#include <QVector>

// Config widget used by the Preferences/Options config panes
class FixGPS;
class FixGPSConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixGPSConfig)

    friend class ::FixGPS;
    protected:
    public:
        // there is no config
        FixGPSConfig(QWidget *parent) : DataProcessorConfig(parent) {}

        QString explain() {
            return(QString(tr("Remove GPS errors and interpolate positional "
                           "data where the GPS device did not record any data, "
                           "or the data that was recorded is invalid.")));
        }

        void readConfig() {}
        void saveConfig() {}
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
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixGPSConfig(parent);
        }
        // Localized Name
        QString name() {
            return (tr("Fix GPS errors"));
        }
};

static bool fixGPSAdded = DataProcessorFactory::instance().registerProcessor((QString("Fix GPS errors")), new FixGPS());

bool
FixGPS::postProcess(RideFile *ride, DataProcessorConfig *)
{
    // ignore null or files without GPS data
    if (!ride || ride->areDataPresent()->lat == false || ride->areDataPresent()->lon == false)
        return false;

    int errors=0;

    ride->command->startLUW("Fix GPS Errors");

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
                for (int j=lastgood+1; j<i; j++) {
                    ride->command->setPointValue(j, RideFile::lat, ride->dataPoints()[lastgood]->lat + (double(j-lastgood)*deltaLat));
                    ride->command->setPointValue(j, RideFile::lon, ride->dataPoints()[lastgood]->lon + (double(j-lastgood)*deltaLon));
                    errors++;
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
    } else {
        // they are all bad!!
        // XXX do nothing?
    }
    ride->command->endLUW();

    if (errors) {
        ride->setTag("GPS errors", QString("%1").arg(errors));
        return true;
    } else
        return false;
}
