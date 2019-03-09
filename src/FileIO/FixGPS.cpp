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
#include <algorithm>
#include <QVector>
#include "LocationInterpolation.h"

// Config widget used by the Preferences/Options config panes
class FixGPS;
class FixGPSConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixGPSConfig)
    friend class ::FixGPS;
    protected:
    public:
        // there is no config
        FixGPSConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixGPSErrors));
        }

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

bool IsReasonableGeoLocation(geolocation *ploc) {
    return  (ploc->Lat()  && ploc->Lat()  >= double(-90)  && ploc->Lat()  <= double(90) &&
             ploc->Long() && ploc->Long() >= double(-180) && ploc->Long() <= double(180) &&
             ploc->Alt() >= -1000 && ploc->Alt() < 10000);
}

bool
FixGPS::postProcess(RideFile *ride, DataProcessorConfig *config, QString op)
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    // ignore null or files without GPS data
    if (!ride || ride->areDataPresent()->lat == false || ride->areDataPresent()->lon == false)
        return false;

    // Interpolate altitude if its available.
    bool fHasAlt = ride->areDataPresent()->alt;

    int errors=0;

    ride->command->startLUW("Fix GPS Errors");

    GeoPointInterpolator gpi;
    int ii = 0; // interpolation input index

    for (int i=0; i<ride->dataPoints().count(); i++) {
        const RideFilePoint * pi = (ride->dataPoints()[i]);
        geolocation curLoc(pi->lat, pi->lon, fHasAlt ? pi->alt : 0.0);

        // Activate interpolation if this sample isn't reasonable.
        if (!IsReasonableGeoLocation(&curLoc))
        {
            double km = pi->km;

            // Feed interpolator until it has samples that span current distance.
            while (gpi.WantsInput(km)) {
                if (ii < ride->dataPoints().count()) {
                    const RideFilePoint * pii = (ride->dataPoints()[ii]);
                    geolocation geo(pii->lat, pii->lon, fHasAlt ? pii->alt : 0.0);

                    // Only feed reasonable locations to interpolator
                    if (IsReasonableGeoLocation(&geo)) {
                        gpi.Push(pii->km, geo);
                    }
                    ii++;
                }
                else {
                    gpi.NotifyInputComplete();
                    break;
                }
            }

            geolocation interpLoc = gpi.Interpolate(km);

            ride->command->setPointValue(i, RideFile::lat, interpLoc.Lat());
            ride->command->setPointValue(i, RideFile::lon, interpLoc.Long());

            if (fHasAlt) {
                ride->command->setPointValue(i, RideFile::alt, interpLoc.Alt());
            }

            errors++;
        }
    }

    ride->command->endLUW();

    if (errors) {
        ride->setTag("GPS errors", QString("%1").arg(errors));
        return true;
    }

    return false;
}
