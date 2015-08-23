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
#include <algorithm>
#include <QVector>

// Config widget used by the Preferences/Options config panes
class FixDerivePower;
class FixDerivePowerConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixDerivePowerConfig)

    friend class ::FixDerivePower;
    protected:
        QHBoxLayout *layout;
        QLabel *bikeWeightLabel;
        QDoubleSpinBox *bikeWeight;
        QLabel *crrLabel;
        QDoubleSpinBox *crr;

    public:
        FixDerivePowerConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_EstimatePowerValues));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            bikeWeightLabel = new QLabel(tr("Bike Weight (kg)"));
            crrLabel = new QLabel(tr("Crr"));

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

            layout->addWidget(bikeWeightLabel);
            layout->addWidget(bikeWeight);
            layout->addWidget(crrLabel);
            layout->addWidget(crr);
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
                              "resistance, it depends on tires and surface")));
        }

        void readConfig() {
            double MBik = appsettings->value(NULL, GC_DPDP_BIKEWEIGHT, "9.5").toDouble();
            bikeWeight->setValue(MBik);
            double Crr = appsettings->value(NULL, GC_DPDP_CRR, "0.0031").toDouble();
            crr->setValue(Crr);
        }

        void saveConfig() {
            appsettings->setValue(GC_DPDP_BIKEWEIGHT, bikeWeight->value());
            appsettings->setValue(GC_DPDP_CRR, crr->value());
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
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixDerivePowerConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Estimate Power Values"));
        }
};

static bool FixDerivePowerAdded = DataProcessorFactory::instance().registerProcessor(QString("Estimate Power Values"), new FixDerivePower());

bool
FixDerivePower::postProcess(RideFile *ride, DataProcessorConfig *config=0)
{
    // get settings
    double MBik; // Bike weight kg
    double CrV;  // Coefficient of Rolling Resistance
    if (config == NULL) { // being called automatically
        MBik = appsettings->value(NULL, GC_DPDP_BIKEWEIGHT, "9.5").toDouble();
        CrV = appsettings->value(NULL, GC_DPDP_CRR, "0.0031").toDouble();
    } else { // being called manually
        MBik = ((FixDerivePowerConfig*)(config))->bikeWeight->value();
        CrV = ((FixDerivePowerConfig*)(config))->crr->value();
    }

    // if its already there do nothing !
    if (ride->areDataPresent()->watts) return false;

    // no dice if we don't have alt and speed
    if (!ride->areDataPresent()->alt || !ride->areDataPresent()->kph) return false;

    // Power Estimation Constants
    double hRider = ride->getHeight(); //Height in m
    double M = ride->getWeight(); //Weight kg
    double T = 15; //Temp degC in not in ride data
    double W = 0; //Assume no wind
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

    if (ride->areDataPresent()->slope && ride->areDataPresent()->alt
     && ride->areDataPresent()->km) {
        for (int i=0; i<ride->dataPoints().count(); i++) {
            RideFilePoint *p = ride->dataPoints()[i];
            // Estimate Power if not in data
            double cad = ride->areDataPresent()->cad ? p->cad : 85.00;
            if (cad > 0) {
                if (ride->areDataPresent()->temp) T = p->temp;
                double Slope = atan(p->slope * .01);
                double V = p->kph * 0.27777777777778; //Speed m/s
                double CrDyn = 0.1 * cos(Slope);

                double CwaRider, Ka;
                double Frg = 9.81 * (MBik + M) * (CrEff * cos(Slope) + sin(Slope));

                double vw=V+W;

                CwaRider = (1 + cad * cCad) * afCd * adipos * (((hRider - adipos) * afSin) + adipos);
                Ka = 176.5 * exp(-p->alt * .0001253) * (CwaRider + CwaBike) / (273 + T);
                //qDebug()<<"acc="<<p->kphd<<" , V="<<V<<" , m="<<M<<" , Pa="<<(p->kphd > 1 ? 1 : p->kphd*V*M);
                double watts = (afCm * V * (Ka * (vw * vw) + Frg + V * CrDyn))+(p->kphd > 1 ? 1 : p->kphd*V*M);
                ride->command->setPointValue(i, RideFile::watts, watts > 0 ? (watts > 1000 ? 1000 : watts) : 0);
                //qDebug()<<"w="<<p->watts<<", Ka="<<Ka<<", CwaRi="<<CwaRider<<", slope="<<p->slope<<", v="<<p->kph<<" Cwa="<<(CwaRider + CwaBike);
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
