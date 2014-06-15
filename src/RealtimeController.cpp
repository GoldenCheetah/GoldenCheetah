/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "RealtimeController.h"
#include "TrainSidebar.h"
#include "RealtimeData.h"
#include "Units.h"

// Abstract base class for Realtime device controllers

RealtimeController::RealtimeController(TrainSidebar *parent, DeviceConfiguration *dc) : parent(parent), dc(dc)
{
    if (dc != NULL)
    {
        // Save a copy of the dc
        devConf = *dc;
        this->dc = &devConf;
    } else {
        this->dc = NULL;
    }

    // setup algorithm
    processSetup();
}

int RealtimeController::start() { return 0; }
int RealtimeController::restart() { return 0; }
int RealtimeController::pause() { return 0; }
int RealtimeController::stop() { return 0; }
bool RealtimeController::find() { return false; }
bool RealtimeController::discover(QString) { return false; }
bool RealtimeController::doesPull() { return false; }
bool RealtimeController::doesPush() { return false; }
bool RealtimeController::doesLoad() { return false; }
void RealtimeController::getRealtimeData(RealtimeData &) { }
void RealtimeController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void
RealtimeController::processRealtimeData(RealtimeData &rtData)
{
    if (!dc) return; // no config

    // setup the algorithm or lookup tables
    // for the device postprocessing type
    switch(dc->postProcess) {
    case 0 : // nothing!
        break;
    case 1 : // Kurt Kinetic - Cyclone
        {
        double mph = rtData.getSpeed() * MILES_PER_KM;
        // using the algorithm from http://www.kurtkinetic.com/powercurve.php
        rtData.setWatts((6.481090) * mph + (0.020106) * (mph*mph*mph));
        }
        break;
    case 2 : // Kurt Kinetic - Road Machine
        {
        double mph = rtData.getSpeed() * MILES_PER_KM;
        // using the algorithm from http://www.kurtkinetic.com/powercurve.php
        rtData.setWatts((5.244820) * mph + (0.019168) * (mph*mph*mph));
        }
        break;
    case 3 : // Cyclops Fluid 2
        {
        double mph = rtData.getSpeed() * MILES_PER_KM;
        // using the algorithm from:
        // http://thebikegeek.blogspot.com/2009/12/while-we-wait-for-better-and-better.html
        rtData.setWatts((0.0115*(mph*mph*mph)) - ((0.0137)*(mph*mph)) + ((8.9788)*(mph)));
        }
        break;
    case 4 : // BT-ATS - BT Advanced Training System
        {
        //        v is expressed in revs/second
        double v = rtData.getWheelRpm()/60.0;
        // using the algorithm from Steven Sansonetti of BT:
        //  This is a 3rd order polynomial, where P = av3 + bv2 + cv + d
        //  where:
                double a =       2.90390167E-01; // ( 0.290390167)
                double b =     - 4.61311774E-02; // ( -0.0461311774)
                double c =       5.92125507E-01; // (0.592125507)
                double d =       0.0;
        rtData.setWatts(a*v*v*v + b*v*v +c*v + d);
        }
        break;

    case 5 : // Lemond Revolution
        {
        double V = rtData.getSpeed() * 0.277777778;
        // Tom Anhalt spent a lot of time working this all out
        // for the data / analysis see: http://wattagetraining.com/forum/viewtopic.php?f=2&t=335
        rtData.setWatts((0.21*pow(V,3))+(4.25*V));
        }
        break;

    case 6 : // 1UP USA
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(25.00 + (2.65f*V) - (0.42f*pow(V,2)) + (0.058f*pow(V,3)));
        }
        break;

    // MINOURA - Has many gears
    case 7 : //MINOURA V100 on H
        {
        double V = rtData.getSpeed();
        // 7 = V100 on H: y = -0.0036x^3 + 0.2815x^2 + 3.4978x - 9.7857 
        rtData.setWatts(pow(-0.0036*V, 3) + pow(0.2815*V,2) + (3.4978*V) - 9.7857);
        }
        break;

    case 8 : //MINOURA V100 on 5
        {
        double V = rtData.getSpeed();
        // 8 = V100 on 5: y = -0.0023x^3 + 0.2067x^2 + 3.8906x - 11.214 
        rtData.setWatts(pow(-0.0023*V, 3) + pow(0.2067*V,2) + (3.8906*V) - 11.214);
        }
        break;

    case 9 : //MINOURA V100 on 4
        {
        double V = rtData.getSpeed();
        // 9 = V100 on 4: y = -0.00173x^3 + 0.1825x^2 + 3.4036x - 10 
        rtData.setWatts(pow(-0.00173*V, 3) + pow(0.1825*V,2) + (3.4036*V) - 10.00);
        }
        break;

    case 10 : //MINOURA V100 on 3
        {
        double V = rtData.getSpeed();
        // 10 = V100 on 3: y = -0.0011x^3 + 0.1433x^2 + 2.8808x - 8.1429 
        rtData.setWatts(pow(-0.0011*V, 3) + pow(0.1433*V,2) + (2.8808*V) - 8.1429);
        }
        break;

    case 11 : //MINOURA V100 on 2
        {
        double V = rtData.getSpeed();
        // 11 = V100 on 2: y = -0.0007x^3 + 0.1348x^2 + 1.581x - 3.3571 
        rtData.setWatts(pow(-0.0007*V, 3) + pow(0.1348*V,2) + (1.581*V) - 3.3571);
        }
        break;

    case 12 : //MINOURA V100 on 1
        {
        double V = rtData.getSpeed();
        // 12 = V100 on 1: y = 0.0004x^3 + 0.057x^2 + 1.7797x - 5.0714 
        rtData.setWatts(pow(0.0004*V, 3) + pow(0.057*V,2) + (1.7797*V) - 5.0714);
        }
        break;

    case 13 : //MINOURA V100 on L
        {
        double V = rtData.getSpeed();
        // 13 = V100 on L: y = 0.0557x^2 + 1.231x - 3.7143 
        rtData.setWatts(pow(0.0557*V, 2) + (1.231*V) - 3.7143);
        }
        break;

    case 14 : //SARIS POWERBEAM PRO
        {
        double V = rtData.getSpeed();
        // 14 = 0.0008x^3 + 0.145x^2 + 2.5299x + 14.641 where x = speed in kph
        rtData.setWatts(pow(0.0008*V, 3) + pow(0.145*V, 2) + (2.5299*V) + 14.641);
        }
        break;

    case 15 : //  TACX SATORI SETTING 2
        {
        double V = rtData.getSpeed();
        double slope = 3.9;
        double intercept = -19.5;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 16 : //  TACX SATORI SETTING 4
        {
        double V = rtData.getSpeed();
        double slope = 6.66;
        double intercept = -52.3;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 17 : //  TACX SATORI SETTING 6
        {
        double V = rtData.getSpeed();
        double slope = 9.43;
        double intercept = -43.65;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 18 : //  TACX SATORI SETTING 8
        {
        double V = rtData.getSpeed();
        double slope = 13.73;
        double intercept = -51.15;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 19 : //  TACX SATORI SETTING 10
        {
        double V = rtData.getSpeed();
        double slope = 17.7;
        double intercept = -76.0;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 20 : //  TACX FLOW SETTING 0
        {
        double V = rtData.getSpeed();
        double slope = 7.75;
        double intercept = -47.27;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 21 : //  TACX FLOW SETTING 2
        {
        double V = rtData.getSpeed();
        double slope = 9.51;
        double intercept = -66.69;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 22 : //  TACX FLOW SETTING 4
        {
        double V = rtData.getSpeed();
        double slope = 11.03;
        double intercept = -71.59;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 23 : //  TACX FLOW SETTING 6
        {
        double V = rtData.getSpeed();
        double slope = 12.81;
        double intercept = -95.05;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 24 : //  TACX FLOW SETTING 8
        {
        double V = rtData.getSpeed();
        double slope = 14.37;
        double intercept = -102.43;
        rtData.setWatts((slope * V) + intercept);
        }
        break;
        
    case 25 : //  TACX BLUE TWIST SETTING 1
        {
        double V = rtData.getSpeed();
        double slope = 3.2;
        double intercept = -24.0;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 26 : //  TACX BLUE TWIST SETTING 3
        {
        double V = rtData.getSpeed();
        double slope = 6.525;
        double intercept = -46.5;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 27 : //  TACX BLUE TWIST SETTING 5
        {
        double V = rtData.getSpeed();
        double slope = 9.775;
        double intercept = -66.5;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 28 : //  TACX BLUE TWIST SETTING 7
        {
        double V = rtData.getSpeed();
        double slope = 13.075;
        double intercept = -89.5;
        rtData.setWatts((slope * V) + intercept);
        }
        break;
        
    case 29 : //  TACX BLUE MOTION SETTING 2
        {
        double V = rtData.getSpeed();
        double slope = 5.225;
        double intercept = -36.5;
        rtData.setWatts((slope * V) + intercept);
        }
        break;
    
    case 30 : //  TACX BLUE MOTION SETTING 4
        {
        double V = rtData.getSpeed();
        double slope = 8.25;
        double intercept = -53.0;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 31 : //  TACX BLUE MOTION SETTING 6
        {
        double V = rtData.getSpeed();
        double slope = 11.45;
        double intercept = -74.0;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 32 : //  TACX BLUE MOTION SETTING 8
        {
        double V = rtData.getSpeed();
        double slope = 14.45;
        double intercept = -89.0;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    case 33 : //  TACX BLUE MOTION SETTING 10
        {
        double V = rtData.getSpeed();
        double slope = 17.575;
        double intercept = -110.5;
        rtData.setWatts((slope * V) + intercept);
        }
        break;

    default : // unknown - do nothing
        break;

    case 34 : // ELITE SUPERCRONO POWER MAG LEVEL 1
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pow(-0.000803192769148186*V, 3) + pow(0.17689196198325*V,2) + (3.62446277061515*V) - 1.16783216783223);
        }
        break;

    case 35 : // ELITE SUPERCRONO POWER MAG LEVEL 2
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pow(-0.00590735326986424*V, 3) + pow(0.442531768374482*V,2) + (3.54843470904764*V) - 0.363636363636395);
        }
        break;

    case 36 : // ELITE SUPERCRONO POWER MAG LEVEL 3
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pow(-0.00917194323478923*V, 3) + pow(0.614352424962992*V,2) + (5.08762781732785*V) - 1.48951048951047);
        }
        break;

    case 37 : // ELITE SUPERCRONO POWER MAG LEVEL 4
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pow(-0.0150015681721553*V, 3) + pow(0.880112976720764*V,2) + (5.16903286351279*V) - 1.7342657342657);
        }
        break;

    case 38 : // ELITE SUPERCRONO POWER MAG LEVEL 5
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pow(-0.0172621671756449*V, 3) + pow(1.0207209560583*V,2) + (6.23730215622854*V) - 3.18881118881126);
        }
        break;

    case 39 : // ELITE SUPERCRONO POWER MAG LEVEL 6
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pow(-0.0195227661791347*V, 3) + pow(1.15505017633569*V,2) + (7.47138264900755*V) - 4.18881118881114);
        }
        break;

    case 40 : // ELITE SUPERCRONO POWER MAG LEVEL 7
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pow(-0.0222497351776137*V, 3) + pow(1.2917943039439*V,2) + (8.74972948026508*V) - 5.11888111888112);
        }
        break;

    case 41 : // ELITE SUPERCRONO POWER MAG LEVEL 8
        {
        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pow(-0.0255078477814972*V, 3) + pow(1.42902141301828*V,2) + (10.2050166192824*V) - 6.48951048951042);
        }
        break;
    case 42:
        {
        double V = rtData.getSpeed();
        // Power curve fit from powercurvesensor
        //     f(x) = 4.31746 * x -2.59259e-002 * x^2 +  9.41799e-003 * x^3
        rtData.setWatts(4.31746 * V - 2.59259e-002 * pow(V, 2) + 9.41799e-003 * pow(V, 3));
        }
        break;
    }
}

// for future devices, we may need to setup algorithmic tables etc
void
RealtimeController::processSetup()
{
    if (!dc) return; // no config

    // setup the algorithm or lookup tables
    // for the device postProcessing type
    switch(dc->postProcess) {
    case 0 : // nothing!
        break;
    case 1 : // TODO Kurt Kinetic - use an algorithm...
    case 2 : // TODO Kurt Kinetic - use an algorithm...
        break;
    case 3 : // TODO Cyclops Fluid 2 - use an algorithm
        break;
    case 4 : // TODO BT-ATS - BT Advanced Training System - use an algorithm
        break;
    default : // unknown - do nothing
        break;
    }
}
