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
#include <algorithm>
#include <QVector>

// Config widget used by the Preferences/Options config panes
class FixDerivePower;
class FixDerivePowerConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixDerivePowerConfig)

    friend class ::FixDerivePower;
    protected:
        //QHBoxLayout *layout;
        //QLabel *taLabel;
        //QLineEdit *ta;

    public:
        FixDerivePowerConfig(QWidget *parent) : DataProcessorConfig(parent) {

            //layout = new QHBoxLayout(this);

            //layout->setContentsMargins(0,0,0,0);
            //setContentsMargins(0,0,0,0);

            //taLabel = new QLabel(tr("Torque Adjust"));

            //ta = new QLineEdit();

            //layout->addWidget(taLabel);
            //layout->addWidget(ta);
            //layout->addStretch();
        }

        //~FixDerivePowerConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Derive estimated power data based on speed/elevation/weight etc")));
        }

        void readConfig() {
            //ta->setText(appsettings->value(NULL, GC_DPTA, "0 nm").toString());
        }

        void saveConfig() {
            //appsettings->setValue(GC_DPTA, ta->text());
        }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
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
    Q_UNUSED(config);

    // if its already there do nothing !
    if (ride->areDataPresent()->watts) return false;

    // no dice if we don't have alt and speed
    if (!ride->areDataPresent()->alt || !ride->areDataPresent()->kph) return false;

    // Power Estimation Constants
    double hRider = 1.7 ; //Height in m
    double M = ride->getWeight(); //Weight kg
    double MBik = 9.5; //Bike weight kg
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
    double CrV = 0.0031;
    double ATire = 0.031;
    double CrEff = CrV;
    double adipos = sqrt(M/(hRider*750));
    double CwaBike = afCdBike * (afCATireV * ATire + afCATireH * ATire + afAFrame);
    //qDebug()<<"CwaBike="<<CwaBike<<", afCdBike="<<afCdBike<<", afCATireV="<<afCATireV<<", ATire="<<ATire<<", afCATireH="<<afCATireH<<", afAFrame="<<afAFrame;

    // apply the change
    ride->command->startLUW("Estimate Power");

    // last point looked at
    RideFilePoint *lastP = NULL;

    if (!ride->areDataPresent()->slope && ride->areDataPresent()->alt && ride->areDataPresent()->km) {
        for (int i=0; i<ride->dataPoints().count(); i++) {
            RideFilePoint *p = ride->dataPoints()[i];
            // Slope
            // If there is no slope data then it can be derived
            // from distanct and altitude
            if (lastP) {
                double deltaDistance = (p->km - lastP->km) * 1000;
                double deltaAltitude = p->alt - lastP->alt;
                if (deltaDistance>0) {
                    ride->command->setPointValue(i, RideFile::slope, (deltaAltitude / deltaDistance) * 100);
                } else {
                    ride->command->setPointValue(i, RideFile::slope, 0);
                }
                if (p->slope > 20 || p->slope < -20) {
                    ride->command->setPointValue(i, RideFile::slope, lastP->slope);
                }
            }
            // last point
            lastP = p;
        }

        // Smooth the slope if it has been derived
        int smoothPoints = 10;
        // initialise rolling average
        double rtot = 0;
        for (int i=smoothPoints; i>0 && ride->dataPoints().count()-i >=0; i--) {
            rtot += ride->dataPoints()[ride->dataPoints().count()-i]->slope;
        }

        // now run backwards setting the rolling average
        for (int i=ride->dataPoints().count()-1; i>=smoothPoints; i--) {
            double here = ride->dataPoints()[i]->slope;
            ride->dataPoints()[i]->slope = rtot / smoothPoints;
            rtot -= here;
            rtot += ride->dataPoints()[i-smoothPoints]->slope;
        }
        ride->setDataPresent(ride->slope, true);
    }

    if (ride->areDataPresent()->slope && ride->areDataPresent()->alt
     && ride->areDataPresent()->km) {
        for (int i=0; i<ride->dataPoints().count(); i++) {
            RideFilePoint *p = ride->dataPoints()[i];
            // Estimate Power if not in data
            if (p->cad > 0) {
                if (ride->areDataPresent()->temp) T = p->temp;
                double Slope = atan(p->slope * .01);
                double V = p->kph * 0.27777777777778; //Speed m/s
                double CrDyn = 0.1 * cos(Slope);

                double CwaRider, Ka;
                double Frg = 9.81 * (MBik + M) * (CrEff * cos(Slope) + sin(Slope));

                double vw=V+W;
                double cad = ride->areDataPresent()->cad ? p->cad : 85.00;

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
