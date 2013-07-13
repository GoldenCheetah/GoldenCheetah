/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_BT40_h
#define _GC_BT40_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QFile>
#include "RealtimeController.h"
#include "TrainSidebar.h"
#include "DeviceConfiguration.h"

#include "WFApi.h"

class BT40 : public QThread
{
    Q_OBJECT

public:
    BT40(QObject *parent=0, DeviceConfiguration * devConf=0);       // pass device
    ~BT40();

    QObject *parent;

    // HIGH-LEVEL FUNCTIONS
    int start();                                // Calls QThread to start
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    int quit(int error);                        // called by thread before exiting
    bool discover(QString deviceFilename);        // confirm CT is attached to device

    // SET
    void setDevice(QString deviceFilename);       // setup the device filename
    void setLoad(double load);                  // set the load to generate in ERGOMODE
    void setGradient(double gradient);          // set the load to generate in SSMODE
    void setMode(int mode,
        double load=100,               // set mode to CT_ERGOMODE or CT_SSMODE
        double gradient=1);

    bool find();
    int connectBT40();
    int disconnectBT40();

    int getMode();
    double getGradient();
    double getLoad();
    void getRealtimeData(RealtimeData &rtData);

    QString id() { return deviceUUID; }

signals:
    void foundDevice(QString uuid, int type);

private slots:
    void discoveredDevices(int,bool);

private:
    void run();                                 // called by start to kick off the CT comtrol thread

    // device configuration
    DeviceConfiguration *devConf;

    // Mutex for controlling accessing private data
    QMutex pvars;

    bool scanned, running, connected;

    volatile int mode;
    volatile double load;
    volatile double slope;

    QString deviceUUID;
    int sd; // sensor descriptor aka an index into the connections array
            // mimics the fd index used by open/close syscalls.
    RealtimeData rt;

    void *pool;
};

#endif // _GC_BT40_h

