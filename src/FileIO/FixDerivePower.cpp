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
#include "HelpWhatsThis.h"
#include "LocationInterpolation.h"
#include <algorithm>
#include <QVector>

#ifndef MATHCONST_PI
#define MATHCONST_PI 		    3.141592653589793238462643383279502884L /* pi */
#endif

// Config widget used by the Preferences/Options config panes
class FixDerivePower;
class FixDerivePowerConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixDerivePowerConfig)

    friend class ::FixDerivePower;
    protected:
        QVBoxLayout *layoutV;
        QHBoxLayout *layout;
        QHBoxLayout *windLayout;
        QLabel *bikeWeightLabel;
        QDoubleSpinBox *bikeWeight;
        QLabel *crrLabel;
        QDoubleSpinBox *crr;
        QLabel *cdALabel;
        QDoubleSpinBox *cdA;
        QLabel *draftMLabel;
        QDoubleSpinBox *draftM;
        QLabel *windSpeedLabel;
        QDoubleSpinBox *windSpeed;
        QLabel *windHeadingLabel;
        QDoubleSpinBox *windHeading;

    public:
        FixDerivePowerConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_EstimatePowerValues));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            bikeWeightLabel = new QLabel(tr("Bike Weight (kg)"));
            crrLabel = new QLabel(tr("Crr"));
            cdALabel = new QLabel(tr("CdA"));
            draftMLabel = new QLabel(tr("Draft mult."));
            windSpeedLabel = new QLabel(tr("Wind (kph)"));
            windHeadingLabel = new QLabel(tr(", direction"));

            bikeWeight = new QDoubleSpinBox();
            bikeWeight->setMaximum(99.9);
            bikeWeight->setMinimum(0);
            bikeWeight->setSingleStep(0.1);
            bikeWeight->setDecimals(1);

            crr = new QDoubleSpinBox();
            crr->setMaximum(0.0999);
            crr->setMinimum(0);
            crr->setSingleStep(0.0001);
            crr->setDecimals(4);

            cdA = new QDoubleSpinBox();
            cdA->setMaximum(0.999);
            cdA->setMinimum(0);
            cdA->setSingleStep(0.001);
            cdA->setDecimals(3);

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

            layout->addWidget(bikeWeightLabel);
            layout->addWidget(bikeWeight);
            layout->addWidget(crrLabel);
            layout->addWidget(crr);
            layout->addWidget(cdALabel);
            layout->addWidget(cdA);
            layout->addWidget(draftMLabel);
            layout->addWidget(draftM);
            layout->addWidget(windSpeedLabel);
            layout->addWidget(windSpeed);
            layout->addWidget(windHeadingLabel);
            layout->addWidget(windHeading);

            layout->addStretch();

        }

        //~FixDerivePowerConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Derive estimated power data based on "
                              "speed/elevation/weight etc\n\n"
                              "Bike Weight parameter is added to athlete's "
                              "weight to compound total mass, it should "
                              "include apparel, shoes, etc\n\n"
                              "CRR parameter is the coefficient of rolling "
                              "resistance, it depends on tires and surface\n\n"
                              "CdA parameter is the effective frontal area "
                              "in m^2, it depends on position and equipment. "
                              "If 0 estimated from anthropometric data\n\n"
                              "Draft Mult. parameter is the multiplier "
                              "to adjust for drafting, 1 is no drafting "
                              " and 0.7 seems legit for drafting in a group\n\n"
                              "wind speed shall be indicated in kph\n"
                              "wind direction (origin) unit is degrees "
                              "from -179 to +180 (-90=W, 0=N, 90=E, 180=S)\n"
                              "Note: when the file already contains wind data, "
                              "it will be overridden if wind speed is set\n\n"
                              "The activity has to be a Ride with Speed and "
                              "Altitude.\n\n"
                              "Warning: the accuracy of power estimation can be "
                              "too low to be of practical use for power analysis "
                              "of general outdoor rides using typical GPS data. "
                              "A power meter is recommended.")));
        }

        void readConfig() {
            double MBik = appsettings->value(NULL, GC_DPDP_BIKEWEIGHT, "9.5").toDouble();
            bikeWeight->setValue(MBik);
            double Crr = appsettings->value(NULL, GC_DPDP_CRR, "0.0031").toDouble();
            crr->setValue(Crr);
            double CdA = appsettings->value(NULL, GC_DPDP_CDA, "0.0").toDouble();
            cdA->setValue(CdA);
            double Draft = appsettings->value(NULL, GC_DPDP_DRAFTM, "1.0").toDouble();
            draftM->setValue(Draft);
            windSpeed->setValue(0.0);
            windHeading->setValue(0.0);
        }

        void saveConfig() {
            appsettings->setValue(GC_DPDP_BIKEWEIGHT, bikeWeight->value());
            appsettings->setValue(GC_DPDP_CRR, crr->value());
            appsettings->setValue(GC_DPDP_CDA, cdA->value());
            appsettings->setValue(GC_DPDP_DRAFTM, draftM->value());
        }
};


// RideFile Dataprocessor -- Derive estimated power data based on
//                           speed/elevation/weight etc
//
class FixDerivePower : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixDerivePower)

    public:
        FixDerivePower() {}
        ~FixDerivePower() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixDerivePowerConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Estimate Power Values"));
        }
};

static bool FixDerivePowerAdded = DataProcessorFactory::instance().registerProcessor(QString("Estimate Power Values"), new FixDerivePower());

bool
FixDerivePower::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // get settings
    double MBik; // Bike weight kg
    double CrV;  // Coefficient of Rolling Resistance
    double CdA;  // Effective frontal area
    double DraftM; // Drafting multiplier
    double windSpeed; // wind speed
    double windHeading; //wind direction
    if (config == NULL) { // being called automatically
        MBik = appsettings->value(NULL, GC_DPDP_BIKEWEIGHT, "9.5").toDouble();
        CrV = appsettings->value(NULL, GC_DPDP_CRR, "0.0031").toDouble();
        CdA = appsettings->value(NULL, GC_DPDP_CDA, "0.0").toDouble();
        DraftM = appsettings->value(NULL, GC_DPDP_DRAFTM, "1.0").toDouble();
        windSpeed = 0.0;
        windHeading = 0.0;
    } else { // being called manually
        MBik = ((FixDerivePowerConfig*)(config))->bikeWeight->value();
        CrV = ((FixDerivePowerConfig*)(config))->crr->value();
        CdA = ((FixDerivePowerConfig*)(config))->cdA->value();
        DraftM = ((FixDerivePowerConfig*)(config))->draftM->value();
        windSpeed = ((FixDerivePowerConfig*)(config))->windSpeed->value();                // kph
        windHeading = ((FixDerivePowerConfig*)(config))->windHeading->value() / 180 * MATHCONST_PI; // rad
    }
    bool CdANotSet = (CdA == 0.0);

    // Do nothing for swims and runs
    if (ride->isSwim() || ride->isRun()) return false;

    // if called automatically and power already present, do nothing !
    if (!config && ride->areDataPresent()->watts) return false;

    // no dice if we don't have alt and speed
    if (!ride->areDataPresent()->alt || !ride->areDataPresent()->kph) return false;

    // Power Estimation Constants (mostly constants...)
    double hRider = ride->getHeight(); //Height in m
    double M = ride->getWeight() + MBik; //Total Mass kg
    double T = 15; // Temp degC if not in ride data
    double W = 0;  // headwind (from records or based on wind parameters entered manually)
    double bearing = 0.0; //cyclist direction used to compute headwind
    double cCad=.002;
    double afCd = 0.62;
    double afSin = 0.89;
    double afCm = 1.025;
    double afCdBike = 1.2;
    double afCATireV = 1.1;
    double afCATireH = 0.9;
    double afAFrame = 0.048;
    double ATire = 0.031;
    double CrEff = CrV;
    double adipos = sqrt(M/(hRider*750));
    double CwaBike = afCdBike * (afCATireV * ATire + afCATireH * ATire + afAFrame);
    //qDebug()<<"CwaBike="<<CwaBike<<", afCdBike="<<afCdBike<<", afCATireV="<<afCATireV<<", ATire="<<ATire<<", afCATireH="<<afCATireH<<", afAFrame="<<afAFrame;

    // apply the change
    ride->command->startLUW("Estimate Power");

    if (ride->areDataPresent()->slope) {
        for (int i=0; i<ride->dataPoints().count(); i++) {
            RideFilePoint *p = ride->dataPoints()[i];

            // compute bearing in order to calculate headwind
            if (i>=1)
            {
                RideFilePoint *prevPoint = ride->dataPoints()[i-1];

                // ensure a movement occurred and valid lat/lon in order to compute cyclist direction
                if ( prevPoint->lat != p->lat || prevPoint->lon != p->lon ) 
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

            // Estimate Power if not in data
            double cad = ride->areDataPresent()->cad ? p->cad : 85.00;
            if (cad > 0) {
                if (ride->areDataPresent()->temp) T = p->temp;
                double Slope = atan(p->slope * .01);
                double V = p->kph * 0.27777777777778; // Cyclist speed m/s
                double CrDyn = 0.1 * cos(Slope);

                double Ka;
                double Frg = 9.81 * M * (CrEff * cos(Slope) + sin(Slope));

                double vw=V+W; // Wind speed against cyclist = cyclist speed + wind speed

                if (CdANotSet) {
                    double CwaRider = (1 + cad * cCad) * afCd * adipos * (((hRider - adipos) * afSin) + adipos);
                    CdA = CwaRider + CwaBike;
                }
                Ka = 176.5 * exp(-p->alt * .0001253) * CdA * DraftM / (273 + T);
                //qDebug()<<"acc="<<p->kphd<<" , V="<<V<<" , m="<<M<<" , Pa="<<(p->kphd > 1 ? 1 : p->kphd)*V*M;
                double watts = (afCm * V * (Ka * (vw * vw) + Frg + V * CrDyn)) + (p->kphd > 1 ? 1 : p->kphd)*V*M;
                ride->command->setPointValue(i, RideFile::watts, watts > 0 ? (watts > 1000 ? 1000 : watts) : 0);
                // qDebug() << "watts = "<<p->watts;
                // qDebug() << "  " << afCm * V * Ka * (vw * vw) << " = afCm(=" << afCm << ") * V(=" << V << ") * Ka(="<<Ka<<") * (vw^2(=" << V+W << "^2))";
                // qDebug() << "  " << afCm * V * Frg << " = afCm * V * Frg(=" << Frg << ")";
                // qDebug() << "  " << afCm * V * V * CrDyn << " = afCm * V^2 * CrDyn(=" << CrDyn << ")";
                // qDebug() << "  " << p->kphd*V*M << " = kphd(=" << p->kphd << ") * V * M(=" << M << ")";
                // qDebug() << "    Ka="<<Ka<<", CwaRi="<<CwaRider<<", slope="<<p->slope<<", v="<<p->kph<<" Cwa="<<(CwaRider + CwaBike);
            } else {
                ride->command->setPointValue(i, RideFile::watts, 0);
            }
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
