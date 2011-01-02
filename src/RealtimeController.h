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


// Abstract base class for Realtime device controllers
#include "RealtimeData.h"
#include "RealtimeWindow.h"

#ifndef _GC_RealtimeController_h
#define _GC_RealtimeController_h 1

#define DEVICE_ERROR 1
#define DEVICE_OK 0

class RealtimeController
{
public:
    RealtimeWindow *parent;                     // for push devices

    RealtimeController (RealtimeWindow *parent, DeviceConfiguration *dc = 0);
    virtual ~RealtimeController() {}

    virtual int start();
    virtual int restart();                              // restart after paused
    virtual int pause();                                // pauses data collection, inbound telemetry is discarded
    virtual int stop();                                 // stops data collection thread
    virtual bool discover(char *pathname);              // tell if a device is present at port passed

    // push or pull telemetry
    virtual bool doesPush();                    // this device is a push device (e.g. Quarq)
    virtual bool doesPull();                    // this device is a pull device (e.g. CT)
    virtual bool doesLoad();                    // this device can generate Load

    // will update the realtime data with current data (only called for doesPull devices)
    virtual void getRealtimeData(RealtimeData &rtData); // update realtime data with current values
    virtual void pushRealtimeData(RealtimeData &rtData); // update realtime data with current values

    // only relevant for Computrainer like devices
    virtual void setLoad(double) { return; }
    virtual void setGradient(double) { return; }
    virtual void setMode(int) { return; }

    // post process, based upon device configuration
    void processRealtimeData(RealtimeData &rtData);
    void processSetup();

private:
    DeviceConfiguration *dc;
    DeviceConfiguration devConf;
};

#endif // _GC_RealtimeController_h

