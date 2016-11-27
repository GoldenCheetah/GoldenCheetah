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
#include "kdri.h"

// Abstract base class for Realtime device controllers

#ifndef _GC_KettlerBluetoothController_h
#define _GC_KettlerBluetoothController_h 1

class KettlerBluetoothController : public RealtimeController
{
    Q_DECLARE_TR_FUNCTIONS(KettlerBluetoothController)

public:
    KettlerBluetoothController (TrainSidebar *, DeviceConfiguration *);
    virtual ~KettlerBluetoothController ();

    KdriConnection* m_connection;               // the connection to the device
    DeviceConfiguration devConf;
    DeviceConfiguration* devConfPtr;

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread

    // for querying devices (bluetooth serial port) and connecting to one of them
    bool canDoInquiry();                             // can there be multiple devices for this device type that can be scanned for?
    QList<DeviceConfiguration> doInquiry();     // scans for devices in range
    bool connectToDevice();
    bool isConnected();

    bool ensureConnection(bool tryConnect);

    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void pushRealtimeData(RealtimeData &rtData);

    void setLoad(double);
    void setGradient(double);
    void setMode(int mode);
};

#endif // _GC_KettlerBluetoothController_h
