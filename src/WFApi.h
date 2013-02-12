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

    // we don't use the WF API header "hardware_connector_types.h" because it
    // references Objc types (NSUInteger) that fail when compiling C++.
    // Instead we redeclare them here, but only the ones we absolutely need
    // and hopefully they won't change too often

    // current state
    typedef enum {
        WFAPI_HWCONN_STATE_NOT_CONNECTED                = 0,
        WFAPI_HWCONN_STATE_CONNECTED                    = 0x01,
        WFAPI_HWCONN_STATE_ACTIVE                       = 0x02,
        WFAPI_HWCONN_STATE_RESET                        = 0x04,
        WFAPI_HWCONN_STATE_BT40_ENABLED                 = 0x08,
        WFAPI_HWCONN_STATE_BT_BONDING_MODE              = 0x10 } _state;
    int currentState();

    // connection state
    typedef enum {
        WF_SENSOR_CONNECTION_STATUS_IDLE = 0,
        WF_SENSOR_CONNECTION_STATUS_CONNECTING = 1,
        WF_SENSOR_CONNECTION_STATUS_CONNECTED = 2,
        WF_SENSOR_CONNECTION_STATUS_INTERRUPTED = 3,
        WF_SENSOR_CONNECTION_STATUS_DISCONNECTING = 4 } _connstate;
    int connectionStatus(int sd);
    bool isConnected(int sd);

    // all the sensor types just the types we need
    enum {
        WF_SENSORTYPE_NONE                           = 0,
        WF_SENSORTYPE_BIKE_POWER                     = 0x00000001, // kickr, inride etc
        WF_SENSORTYPE_BIKE_SPEED                     = 0x00000002,
        WF_SENSORTYPE_BIKE_CADENCE                   = 0x00000004,
        WF_SENSORTYPE_BIKE_SPEED_CADENCE             = 0x00000008,
        WF_SENSORTYPE_FOOTPOD                        = 0x00000010,
        WF_SENSORTYPE_HEARTRATE                      = 0x00000020,
        WF_SENSORTYPE_WEIGHT_SCALE                   = 0x00000040,
        WF_SENSORTYPE_ANT_FS                         = 0x00000080,
        WF_SENSORTYPE_LOCATION                       = 0x00000100,
        WF_SENSORTYPE_CALORIMETER                    = 0x00000200,
        WF_SENSORTYPE_GEO_CACHE                      = 0x00000400,
        WF_SENSORTYPE_FITNESS_EQUIPMENT              = 0x00000800,
        WF_SENSORTYPE_MULTISPORT_SPEED_DISTANCE      = 0x00001000,
        WF_SENSORTYPE_PROXIMITY                      = 0x00002000,
        WF_SENSORTYPE_HEALTH_THERMOMETER             = 0x00004000,
        WF_SENSORTYPE_BLOOD_PRESSURE                 = 0x00008000,
        WF_SENSORTYPE_BTLE_GLUCOSE                   = 0x00010000,
        WF_SENSORTYPE_GLUCOSE                        = 0x00020000,
        WF_SENSORTYPE_DISPLAY                        = 0x00800000  } _sensortypes; // rflkt

    const QString sensorDescription(int id, int sub) const {
        QString returning(tr("Unknown"));
        switch (id) {
        case 0x0 : returning = tr("None"); break;
        case 0x1 : 
            {
                switch(sub) {

                    case  0:
                    default:
                        returning = tr("Power Meter");
                        break;

                    case 1:
                        returning = tr("Wahoo KICKR trainer");
                        break;

                    case 2:
                        returning = tr("Stage ONE Crank Power Meter");
                        break;

                    case 3:
                        returning = tr("Kurt Kinetic InRide Power Meter");
                        break;
                }
            }
            break;

        case 0x2 : returning = tr("Bike Speed"); break;
        case 0x4 : returning = tr("Bike Cadence"); break;
        case 0x8 : returning = tr("Speed and Cadence"); break;
        case 0x10 : returning = tr("FootPod"); break;
        case 0x20 : returning = tr("Heart Rate"); break;
        case 0x800000 : returning = tr("RFKLT Display"); break;
        }
        return returning;
    }


    // subtypes -- esp. power
    enum {
        WF_SENSOR_SUBTYPE_UNSPECIFIED                = 0,
        WF_SENSOR_SUBTYPE_BIKE_POWER_KICKR           = 1,
        WF_SENSOR_SUBTYPE_BIKE_POWER_STAGE_ONE       = 2,
        WF_SENSOR_SUBTYPE_BIKE_POWER_IN_RIDE         = 3 } _sensorsubtype;

    // scan
    bool discoverDevicesOfType(int eSensorType);
    int deviceCount();
    QString deviceUUID(int); // return the UUID for device n
    int deviceType(int); // return the device type
    int deviceSubType(int); // return the subtype found

    // connect and disconnect
    int connectDevice(QString uuid); // connect the device n
    bool disconnectDevice(int sd);   // disconnect

    // has data?
    bool hasData(int sd);
    void getRealtimeData(int sd, RealtimeData *p);

    // set slope or ergo mode
    void setSlopeMode(int sd);
    void setErgoMode(int sd);

    // set resistance slope or load
    void setSlope(int sd, double slope);
    void setLoad(int sd, int watts);

    // NOTE: There is an application wide NSAutoreleasePool maintained
    //       in cocoa initialiser, but it is only to support activity on
    //       the main thread.
    //       The application code (e.g. Kickr.cpp) needs to get and free a
    //       pool for each thread, this is why we have a getPool/freePool
    //       method in WFApi, but never allocate a pool ourselves.
    void *getPool();
    void freePool(void*);

signals:
    void currentStateChanged(int); // hardware conncector state changed
    void connectionStateChanged(int status);
    void discoveredDevices(int,bool);
    void discoverFinished();
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
    QVector <void *> connections;
};
#endif
