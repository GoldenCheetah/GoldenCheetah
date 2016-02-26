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
#include "DeviceConfiguration.h"
#include "QuarqdClient.h"
#include "ConfigDialog.h"

// Abstract base class for Realtime device controllers


#ifndef _GC_ANTplusController_h
#define _GC_ANTplusController_h 1

class ANTplusController : public RealtimeController
{

public:
    ANTplusController (TrainTool *parent =0, DeviceConfiguration *dc =0);

    QuarqdClient *myANTplus;               // the device itself

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    bool discover(QString);
                                                // port is specified as ipname:port e.g. 192.168.2.1:8168
    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void pushRealtimeData(RealtimeData &rtData);
    void setLoad(double) { return; }
};

#endif // _GC_ANTplusController_h

