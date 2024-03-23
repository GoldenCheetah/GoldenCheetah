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
#include "GoldenCheetah.h"

#include "RealtimeController.h"
#include "Computrainer.h"

// Abstract base class for Realtime device controllers

#ifndef _GC_ComputrainerController_h
#define _GC_ComputrainerController_h 1

class ComputrainerController : public RealtimeController
{
    Q_OBJECT

public:
    ComputrainerController (TrainSidebar *, DeviceConfiguration *);

    Computrainer *myComputrainer;               // the device itself

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    bool discover(QString);                     // tell if a device is present at port passed


    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void pushRealtimeData(RealtimeData &rtData);
    void setLoad(double);
    void setGradient(double);
    void setMode(int);

    // calibration
    uint8_t  getCalibrationType() { return CALIBRATION_TYPE_COMPUTRAINER; }

private:
    bool f1Depressed;
    bool f2Depressed;
    bool f3Depressed;
};

#endif // _GC_ComputrainerController_h

