/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRWFApiY; without even the implied warranty of MERCHWFApiABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef gc_WFApi_h
#define gc_WFApi_h

// GC 
#include "GoldenCheetah.h"
#include "Settings.h"
#include "RealtimeData.h"
#include "DeviceConfiguration.h"
#include "RealtimeData.h"

// QT
#include <QPointer>
#include <QObject>

// global singleton
class WFApi;
extern WFApi *_gc_wfapi;

// declare the provate implementation for objc
#ifdef __OBJC__
@class WFBridge;
#else
// irritating dependencies
#endif

class WFApi : public QObject // QOBject for signals
{
   Q_OBJECT

public:
    WFApi(); // single instance
    ~WFApi();

    static WFApi *getInstance() { if (_gc_wfapi) return _gc_wfapi;
                                  else return (_gc_wfapi = new WFApi); } // singleton

    // what version?
    QString apiVersion();

    // status of BTLE
    bool hasBTLESupport();
    bool isBTLEEnabled();
    bool enableBTLE(bool enable, bool bondingmode);
    bool isCommunicationHWReady();

    // current state
    int currentState();

    // scan
    bool discoverDevicesOfType(int eSensorType, int eNetworkType, int timeout);
    int deviceCount();
    QString deviceUUID(int); // return the UUID for device n

    // connect and disconnect
    bool connectDevice(int n); // connect the device n
    bool disconnectDevice();   // disconnect

    // has data?
    bool hasData();
    void getRealtimeData(RealtimeData *p);

signals:
    void currentStateChanged(int); // hardware conncector state changed
    int discoveredDevices(int,bool);
    void connectionHasData();

public slots:

    // connecting...
    void stateChanged();
    void connectionState(int status);
    void connectionTimeout();

    void connectedSensor(void*);
    void didDiscoverDevices(int count, bool finished);
    void disconnectedSensor(void*);
    void hasFirmwareUpdateAvalableForConnection();
    void connectorHasData();

signals:

public slots:

public:

    // the native api bridge -- private implementation in
    // WFApi.mm -- bridge between the QT/C++ world and the
    // WF/Objc world
#ifdef __OBJC__
    WFBridge *wf; // when included in objc sources
#else /* __OBJC__ */
    void *wf;       // when included in C++ sources
#endif /* __OBJC__ */
};

//
// We need to define the constants from the WF API headers because they
// assume you are building with OBJ C. And fail to compile with gcc/g++
//

// Hardware Connector states
typedef enum
{
    /** The fisica device is not physically connected to the iPod. */
    WFAPI_HWCONN_STATE_NOT_CONNECTED                = 0,
    /** The fisica device is physically connected to the iPod. */
    WFAPI_HWCONN_STATE_CONNECTED                    = 0x01,
    /** The fisica device is connected and communication is established (norml operating mode). */
    WFAPI_HWCONN_STATE_ACTIVE                       = 0x02,
    /** The fisica device is performing a reset operation. */
    WFAPI_HWCONN_STATE_RESET                        = 0x04,
    /** The BTLE hardware is enabled. */
    WFAPI_HWCONN_STATE_BT40_ENABLED                 = 0x08,
    /** The BTLE controller is in bonding mode. */
    WFAPI_HWCONN_STATE_BT_BONDING_MODE              = 0x10,
    
} WFConnState;

#endif
