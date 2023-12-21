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
#include "RemoteControl.h"

#include <QMessageBox>

// Abstract base class for Realtime device controllers

#ifndef _GC_ANTlocalController_h
#define _GC_ANTlocalController_h 1

class ANTlocalController : public RealtimeController
{
    Q_OBJECT

public:
    ANTlocalController (TrainSidebar *parent =0, DeviceConfiguration *dc =0);

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

    // now with the kickr we can control trainers
    void setLoad(double);
    void setGradient(double);
    void setMode(int);

    // passing calibration state between trainer and TrainSidebar
    uint8_t  getCalibrationType();
    double   getCalibrationTargetSpeed();
    uint8_t  getCalibrationState();
    void     setCalibrationState(uint8_t);
    uint16_t getCalibrationSpindownTime();
    uint16_t getCalibrationZeroOffset();
    uint16_t getCalibrationSlope();
    void     resetCalibrationState();

signals:
    void foundDevice(int channel, int device_number, int device_id); // channelInfo
    void lostDevice(int channel);            // dropInfo
    void searchTimeout(int channel);         // searchTimeount

    // signal instantly on data receipt for R-R data
    // made a special case to support HRV tool without complication
    void rrData(uint16_t  measurementTime, uint8_t heartrateBeats, uint8_t instantHeartrate);

    void posData(uint8_t position);

    // signal for passing remote control commands to train view
    void remoteControl(uint16_t command);

public slots:
    // slot for receiving & translating ANT remote control commands
    void antRemoteControl(uint16_t command);

private:
    QQueue<setChannelAtom> channelQueue;
    ANTLogger *logger;

};

#endif // _GC_ANTlocalController_h
