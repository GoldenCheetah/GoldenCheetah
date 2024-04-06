/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com),
 *               2018 Florian Nairz (nairz.florian@gmail.com)
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
#include "Daum.h"

#ifndef _GC_DaumController_h
#define _GC_DaumController_h 1

class DaumController : public RealtimeController
{
    Q_OBJECT

public:
    DaumController (TrainSidebar *, DeviceConfiguration *);

    // device control
    int start();            // start capturing
    int restart();          // restart after paused
    int pause();            // pauses data collection, inbound telemetry is discarded
    int stop();             // stops data collection thread

    // device discovery
    bool discover(QString); // tell if a device is present at port passed

    // telemetry: we can pull data and set load
    bool doesPull() { return true; }
    bool doesLoad() { return true; }

    // get telemetry data
    void getRealtimeData(RealtimeData &rtData);

    void setLoad(double);
    void setMode(int) { return; }

private:
    Daum daumDevice_;     // device instance
};

#endif // _GC_DaumController_h

