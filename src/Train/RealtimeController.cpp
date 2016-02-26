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

    int level = parent->currentLevel(); // trainer level in device units.
    //convert to 0 based integer.
    if (dc->levels)
    {
        level = (level - dc->levelstart)/dc->levelstep;
        if (level > dc->levels)
            level = 0;
    }

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
    case 7 : //MINOURA V100
        {
        const double pol[7][4]={
            0.0000, 0.0557,1.231, - 3.7143,
            0.0004, 0.057, 1.7797,- 5.0714,
            -0.0007, 0.1348,1.581, - 3.3571,
            -0.0011, 0.1433,2.8808,- 8.1429,
            -0.00173,0.1825,3.4036,- 10.00,
            -0.0023, 0.2067,3.8906,- 11.214,
            -0.0036, 0.2815,3.4978,- 9.7857
        };

        double V = rtData.getSpeed();
        rtData.setWatts(pol[level][0]*pow(V, 3) + pol[level][1]*pow(V,2) + (pol[level][2]*V) + pol[level][3]);
        }
        break;


    case 8 : //MINOURA V270/v130
        {
        const double pol[7][6]={
            3.46907993967e-07,-3.38406691348e-05,-6.92787604552e-05,0.122555189908,0.642516796929,-0.0859728506788,
            3.74057315234e-07,-1.7235705471e-05, -0.00251391745509, 0.237884615385,0.704304812834,-0.056561085973,
            -5.58069381599e-07,0.000123625394214,-0.0103757370081,0.434414507062,0.278777594954,-0.00678733031682,
            2.32277526395e-07,3.96681749623e-05,-0.00805837789661,0.439146098999,1.00085150144,-0.386877828054,
            -3.34841628959e-07,0.000114219114219,-0.0117029686,0.52033045386,1.10649184149,-0.364253393665,
            -9.35143288085e-07,0.000193281228575,-0.0153105032223,0.587825311943,1.16395173454,-0.472850678733,
            -1.84615384615e-06,0.000338955162485,-0.0237725215961,0.782320032908,0.538329905388,-0.628959276017
        };

        double V = rtData.getSpeed();
        rtData.setWatts(pol[level][0]*pow(V, 5) +
                pol[level][1]*pow(V, 4) +
                pol[level][2]*pow(V, 3) +
                pol[level][3]*pow(V, 2) +
                pol[level][4]*V +
                pol[level][5]);
        }
        break;



    case 9 : //SARIS POWERBEAM PRO
        {
        double V = rtData.getSpeed();
        // 21 = 0.0008x^3 + 0.145x^2 + 2.5299x + 14.641 where x = speed in kph
        rtData.setWatts(0.0008*pow(V, 3) + 0.145*pow(V, 2) + (2.5299*V) + 14.641);
        }
        break;

    case 10 : //  TACX SATORI
        {
        const double fac[5][2]={
            3.9,-19.5,
            6.66,-52.3,
            9.43,-43.65,
            13.73,-51.15,
            17.7,-76.0
        };
        double V = rtData.getSpeed();
        rtData.setWatts((fac[level][0]* V) + fac[level][1]);
        }
        break;

    case 11 : //  TACX FLOW
        {
        const double fac[5][2]={
            7.75,-47.27,
            9.51,-66.69,
            11.03,-71.59,
            12.81,-95.05,
            14.37,-102.43
        };
        double V = rtData.getSpeed();
        rtData.setWatts((fac[level][0] * V) + fac[level][1]);
        }
        break;

    case 12 : //  TACX BLUE TWIST
        {
        const double fac[4][2]={
            3.2,-24.0,
            6.525,-46.5,
            9.775,-66.5,
            13.075,-89.5
        };
        double V = rtData.getSpeed();
        rtData.setWatts((fac[level][0] * V) + fac[level][1]);
        }
        break;

    case 13 : //  TACX BLUE MOTION
        {
        const double fac[5][2]={
            5.225,-36.5,
            8.25,-53.0,
            11.45,-74.0,
            14.45,-89.0,
            17.575,-110.5
        };
        double V = rtData.getSpeed();
        rtData.setWatts((fac[level][0] * V) + fac[level][1]);
        }
        break;
    
    case 14 : // ELITE SUPERCRONO POWER MAG
        {
        // Power curve provided by extraction from SportsTracks plugin
        double pol[8][4]={
            -0.000803192769148186,0.17689196198325,3.62446277061515,- 1.16783216783223,
            -0.00590735326986424,0.442531768374482,3.54843470904764,- 0.363636363636395,
            -0.00917194323478923,0.614352424962992,5.08762781732785,- 1.48951048951047,
            -0.0150015681721553,0.880112976720764,5.16903286351279, - 1.7342657342657,
            -0.0172621671756449,1.0207209560583,6.23730215622854,- 3.18881118881126,
            -0.0195227661791347,1.15505017633569,7.47138264900755, - 4.18881118881114,
            -0.0222497351776137,1.2917943039439,8.74972948026508,- 5.11888111888112,
            -0.0255078477814972,1.42902141301828,10.2050166192824,- 6.48951048951042
        };

        double V = rtData.getSpeed() * MILES_PER_KM;
        // Power curve provided by extraction from SportsTracks plugin
        rtData.setWatts(pol[level][0]*pow(V, 3) + pol[level][1]*pow(V,2) + (pol[level][2]*V) + pol[level][3]);
        }
        break;

    case 15: //ELITE TURBO MUIN 2013
        {
        double V = rtData.getSpeed();
        // Power curve fit from data collected by Ray Maker at dcrainmaker.com
        rtData.setWatts(2.30615942 * V -0.28395558 * pow(V,2) + 0.02099661 * pow(V,3));
        }
        break;
    case 16: // ELITE QUBO POWER FLUID
        {
        double V = rtData.getSpeed();
        // Power curve fit from powercurvesensor
        //     f(x) = 4.31746 * x -2.59259e-002 * x^2 +  9.41799e-003 * x^3
        rtData.setWatts(4.31746 * V - 2.59259e-002 * pow(V, 2) + 9.41799e-003 * pow(V, 3));
        }
        break;

    case 17: // ELITE Qubo Digital
        {
        double pol[16][3]={
            0.039,3.91,-9,
            0.051,3.91,-10,
            0.069,3.59,-5,
            0.072,4.58,-13,
            0.075,5.95,-23,
            0.066,7.86,-37,
            0.071,9.71,-48,
            0.067,11.77,-63,
            0.063,14.11,-78,
            0.091,14.91,-78,
            0.099,16.45,-88,
            0.118,16.86,-86,
            0.124,18.04,-92,
            0.145,18.13,-89,
            0.179,17.35,-82,
            0.207,16.1,-76
        };
        double v = rtData.getSpeed();
        rtData.setWatts(pol[level][0]*pow(V,2) + (pol[level][1]*V) + pol[level][2]);
        }
        break;

    case 18: // ELITE ARION MAG
        {
        double pol[3][2]={
            1.217110021,3.335794377,
            1.206592577,4.362485081,
            1.206984321,6.374459698
        };
        double v = rtData.getSpeed();
        rtData.setWatts(pow(v,pol[level][0] ) * pol[level][1]);
        }
        break;

    case 19: // CYCLOPS MAGNETO PRO (ROAD)
        {
        double V = rtData.getSpeed();
        //     Watts = 6.0f + (-0.93 * speed) + (0.275 * speed^2) + (-0.00175 * speed^3)
        rtData.setWatts(6.0f + (-0.93f * V) + (0.275f * pow(V, 2)) + (-0.00175f * pow(V, 3)));
        }
        break;

    case 20: // Blackburn Tech Fluid
        {
        double v = rtData.getSpeed();
        rtData.setWatts(6.758241758241894 - 1.9995004995004955 * v + 0.24165834165834146 * pow(v, 2));
        }
        break;

    default : // unknown - do nothing
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
