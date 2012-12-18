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
#include "ANT.h"
#include "ANTLogger.h"
#include "ConfigDialog.h"

// Abstract base class for Realtime device controllers

#ifndef _GC_ANTlocalController_h
#define _GC_ANTlocalController_h 1

class ANTlocalController : public RealtimeController
{
    Q_OBJECT

public:
    ANTlocalController (TrainTool *parent =0, DeviceConfiguration *dc =0);

    ANT *myANTlocal;               // the device itself

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread

    int channels() { return myANTlocal->channelCount(); }
    void setChannel(int channel, int device_number, int device_type) {
        myANTlocal->setChannel(channel,device_number,device_type); // using QQueue
    }
    double channelValue(int channel) { return myANTlocal->channelValue(channel); }
    double channelValue2(int channel) { return myANTlocal->channelValue2(channel); }

    bool find();
    bool discover(QString name);
    void setDevice(QString);

    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void pushRealtimeData(RealtimeData &rtData);
    void setLoad(double) { return; }

signals:
    void foundDevice(int channel, int device_number, int device_id); // channelInfo
    void lostDevice(int channel);            // dropInfo
    void searchTimeout(int channel);         // searchTimeount

private:
    QQueue<setChannelAtom> channelQueue;
    ANTLogger logger;

    
};

#endif // _GC_ANTlocalController_h
