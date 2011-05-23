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
#include "RealtimeWindow.h"
#include "RealtimeData.h"
#include "Units.h"

// Abstract base class for Realtime device controllers

RealtimeController::RealtimeController(RealtimeWindow *parent, DeviceConfiguration *dc) : parent(parent), dc(dc)
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
bool RealtimeController::discover(char *) { return false; }
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
        rtData.setWatts((5.244820) * mph + (0.01968) * (mph*mph*mph));
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
