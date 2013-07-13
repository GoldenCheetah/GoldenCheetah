/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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
#include "ConfigDialog.h"

#include "BT40.h"

#ifndef _GC_BT40Controller_h
#define _GC_BT40Controller_h 1

class BT40Controller : public RealtimeController
{
    Q_OBJECT

public:
    BT40Controller (TrainSidebar *parent =0, DeviceConfiguration *dc =0);
    ~BT40Controller();

    BT40 *myBT40;               // the device itself

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread

    bool find();
    bool discover(QString name);
    void setDevice(QString);

    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void pushRealtimeData(RealtimeData &rtData);

    void setLoad(double x) { myBT40->setLoad(x); }
    void setGradient(double x) { myBT40->setGradient(x); }
    void setMode(int x) { myBT40->setMode(x); }

    QString id() { return myBT40->id(); }

signals:
    void foundDevice(QString uuid, int type);

private:
    
};

#endif // _GC_BT40Controller_h
