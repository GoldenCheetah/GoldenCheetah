/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2016 Alejandro Martinez (amtriathlon@gmail.com)
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
#include "LocationInterpolation.h"
#include <algorithm>
#include <QVector>

#ifndef MATHCONST_PI
#define MATHCONST_PI 		    3.141592653589793238462643383279502884L /* pi */
#endif

// Config widget used by the Preferences/Options config panes
class FixRunningPower;
class FixRunningPowerConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixRunningPowerConfig)

    friend class ::FixRunningPower;
    protected:
        QVBoxLayout *layoutV;
        QHBoxLayout *layout;
        QHBoxLayout *windLayout;
        QLabel *equipWeightLabel;
        QDoubleSpinBox *equipWeight;
        QLabel *draftMLabel;
        QDoubleSpinBox *draftM;
        QLabel *windSpeedLabel;
        QDoubleSpinBox *windSpeed;
        QLabel *windHeadingLabel;
        QDoubleSpinBox *windHeading;

    public:
        FixRunningPowerConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_EstimatePowerValues));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            equipWeightLabel = new QLabel(tr("Equipment Weight (kg)"));
            draftMLabel = new QLabel(tr("Draft mult."));
            windSpeedLabel = new QLabel(tr("Wind (kph)"));
            windHeadingLabel = new QLabel(tr(", direction"));

            equipWeight = new QDoubleSpinBox();
            equipWeight->setMaximum(9.9);
            equipWeight->setMinimum(0);
            equipWeight->setSingleStep(0.1);
            equipWeight->setDecimals(1);

            draftM = new QDoubleSpinBox();
            draftM->setMaximum(1.0);
            draftM->setMinimum(0.1);
            draftM->setSingleStep(0.1);
            draftM->setDecimals(1);

            windSpeed = new QDoubleSpinBox();
            windSpeed->setMaximum(99.9);
            windSpeed->setMinimum(0);
            windSpeed->setSingleStep(0.1);
            windSpeed->setDecimals(1);

            windHeading = new QDoubleSpinBox();
            windHeading->setMaximum(180.0);
            windHeading->setMinimum(-179.0);
            windHeading->setSingleStep(1);
            windHeading->setDecimals(0);

            layout->addWidget(equipWeightLabel);
            layout->addWidget(equipWeight);
            layout->addWidget(draftMLabel);
            layout->addWidget(draftM);
            layout->addWidget(windSpeedLabel);
            layout->addWidget(windSpeed);
            layout->addWidget(windHeadingLabel);
            layout->addWidget(windHeading);

            layout->addStretch();

        }

        //~FixRunningPowerConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Derive estimated running power data based on "
                              "speed/elevation/weight etc using di Prampero "
                              "coefficcients\n\n"
                              "Equipment Weight parameter is added to athlete's"
                              " weight to compound total mass, it should "
                              "include apparel, shoes, etc\n\n"
                              "Draft Mult. parameter is the multiplier "
                              "to adjust for drafting, 1 is no drafting "
                              " and 0.7 seems legit for drafting in a group\n\n"
                              "wind speed shall be indicated in kph\n"
                              "wind direction (origin) unit is degrees "
                              "from -179 to +180 (-90=W, 0=N, 90=E, 180=S)\n"
                              "Note: when the file already contains wind data, "
                              "it will be overridden if wind speed is set\n\n"
                              "The activity has to be a Run with Speed and "
                              "Altitude.")));
        }

        void readConfig() {
            double MEquip = appsettings->value(NULL, GC_DPRP_EQUIPWEIGHT, "0.7").toDouble();
            equipWeight->setValue(MEquip);
            double Draft = appsettings->value(NULL, GC_DPDR_DRAFTM, "1.0").toDouble();
            draftM->setValue(Draft);
            windSpeed->setValue(0.0);
            windHeading->setValue(0.0);
        }

        void saveConfig() {
            appsettings->setValue(GC_DPRP_EQUIPWEIGHT, equipWeight->value());
            appsettings->setValue(GC_DPDR_DRAFTM, draftM->value());
        }
};


// RideFile Dataprocessor -- Derive estimated running power data based on
//                           speed/elevation/weight with di Prampero coeff.
//
class FixRunningPower : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixRunningPower)

    public:
        FixRunningPower() {}
        ~FixRunningPower() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixRunningPowerConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Estimate Running Power"));
        }
};

static bool FixRunningPowerAdded = DataProcessorFactory::instance().registerProcessor(QString("Estimate Running Power"), new FixRunningPower());

bool
FixRunningPower::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // get settings
    double MEquip; // Equipment weight kg
    double DraftM;  // Drafting Multiplier
    double windSpeed; // wind speed
    double windHeading; //wind direction
    if (config == NULL) { // being called automatically
        MEquip = appsettings->value(NULL, GC_DPRP_EQUIPWEIGHT, "0.7").toDouble();
        DraftM = appsettings->value(NULL, GC_DPDR_DRAFTM, "1.0").toDouble();
        windSpeed = 0.0;
        windHeading = 0.0;
    } else { // being called manually
        MEquip = ((FixRunningPowerConfig*)(config))->equipWeight->value();
        DraftM = ((FixRunningPowerConfig*)(config))->draftM->value();
        windSpeed = ((FixRunningPowerConfig*)(config))->windSpeed->value();                // kph
        windHeading = ((FixRunningPowerConfig*)(config))->windHeading->value() / 180 * MATHCONST_PI; // rad
    }

    // if not a run do nothing !
    if (!ride->isRun()) return false;

    // if called automatically and power already present, do nothing !
    if (!config && ride->areDataPresent()->watts) return false;

    // no dice if we don't have alt and speed
    if (!ride->areDataPresent()->alt || !ride->areDataPresent()->kph) return false;

    // Power Estimation Constants (mostly constants...)
    double H = ride->getHeight(); //Height in m
    double M = ride->getWeight(); //Weight kg
    double Mtotal = M + MEquip;
    double T = 15; // Temp degC if not in ride data
    double W = 0;  // headwind (from records or based on wind parameters entered manually)
    double bearing = 0.0; //runner direction used to compute headwind
    double Cx = 0.9; // Axial force coefficient
    double FA = 0.226 * 0.2025 * pow(H, 0.725) * pow(M, 0.425); // Frontal Area
    double Cr = 3.72; // di Prampero cost of flat running
    double eff = 0.228; // di Prampero efficiency (metabolic to external)

    // apply the change
    ride->command->startLUW("Estimate Running Power");

    if (ride->areDataPresent()->slope) {
        for (int i=0; i<ride->dataPoints().count(); i++) {
            RideFilePoint *p = ride->dataPoints()[i];

            // compute bearing in order to calculate headwind
            if (i>=1)
            {
                RideFilePoint *prevPoint = ride->dataPoints()[i-1];

                // ensure a movement occurred and valid lat/lon in order to compute cyclist direction
                if (prevPoint->lat != p->lat || prevPoint->lon != p->lon)
                {
                    geolocation prevLoc(prevPoint->lat, prevPoint->lon, prevPoint->alt);
                    geolocation loc(p->lat, p->lon, p->alt);

                    if (prevLoc.IsReasonableGeoLocation() && loc.IsReasonableGeoLocation())
                    {
                        bearing = prevLoc.BearingTo(loc);
                    }
                }
            }
            // else keep previous bearing (or 0 at the beginning)

            // wind parameters to be considered
            if (windSpeed || windHeading) // if wind parameters were entered manually then use it and override rideFile headwind
                W = cos(bearing - windHeading) * windSpeed * 0.27777777777778; // Absolute wind speed relative to cyclist orientation (m/s)
            else if (ride->areDataPresent()->headwind) //  otherwise use headwind from rideFile records (typ. weather forecast included in FIT files)
                W = (p->headwind - p->kph) * 0.27777777777778; // Compute absolute wind speed relative to cyclist orientation (m/s)
            else
                W = 0.0; //otherwise assume no wind

            double V = p->kph * 0.27777777777778; // Runner speed m/s
            // if the file has temperature, use it
            if (ride->areDataPresent()->temp) T = p->temp;
            // Air Density based on altitude and temperature
            double rho = 353 * exp(-p->alt * .0001253) / (273 + T);
            double vw=V+W; // Wind speed against runner = speed + wind speed
            double Slope = atan(p->slope * .01);
            double forw = Cr * Mtotal * eff * V;
            double aero = 0.5 * rho * Cx * FA * DraftM * (vw*vw) * V;
            double grav = 9.81 * Mtotal * sin(Slope) * V;
            double chgV = (p->kphd > 1 ? 1 : p->kphd) * Mtotal * V;

            // Power = moving forward + aerodynamic + gravity + change of speed
            double watts = forw + aero + grav + chgV;
            ride->command->setPointValue(i, RideFile::watts, watts > 0 ? (watts > 1000 ? 1000 : watts) : 0);
        }

        int smoothPoints = 3;
        // initialise rolling average
        double rtot = 0;
        for (int i=smoothPoints; i>0 && ride->dataPoints().count()-i >=0; i--) {
            rtot += ride->dataPoints()[ride->dataPoints().count()-i]->watts;
        }

        // now run backwards setting the rolling average
        for (int i=ride->dataPoints().count()-1; i>=smoothPoints; i--) {
            double here = ride->dataPoints()[i]->watts;
            ride->dataPoints()[i]->watts = rtot / smoothPoints;
            if (ride->dataPoints()[i]->watts<0) ride->dataPoints()[i]->watts = 0;
                rtot -= here;
                rtot += ride->dataPoints()[i-smoothPoints]->watts;
        }
        ride->setDataPresent(ride->watts, true);
    }

    ride->command->endLUW();

    return true;
}
