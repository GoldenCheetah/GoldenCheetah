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
#include "Fortius.h"

// Abstract base class for Realtime device controllers

#ifndef _GC_FortiusController_h
#define _GC_FortiusController_h 1

class FortiusController : public RealtimeController
{
    Q_OBJECT

public:
    static const int calibrationDurationLimit_s = 60;

    FortiusController (TrainSidebar *, DeviceConfiguration *);

    Fortius *myFortius;               // the device itself

    int start() override;
    int restart() override;           // restart after paused
    int pause() override;             // pauses data collection, inbound telemetry is discarded
    int stop() override;              // stops data collection thread

    bool find() override;
    bool discover(QString) override;  // tell if a device is present at port passed


    // telemetry push pull
    bool doesPush() override, doesPull() override, doesLoad() override;
    void getRealtimeData(RealtimeData &rtData) override;
    void pushRealtimeData(RealtimeData &rtData) override;
    void setLoad(double watts) override;
    void setGradientWithSimState(double gradient, double force_N, double speed_kph) override;
    void setMode(int) override;
    void setWeight(double kg) override;

    // calibration
    uint8_t  getCalibrationType() override;
    double   getCalibrationTargetSpeed() override;
    uint8_t  getCalibrationState() override;
    void     setCalibrationState(uint8_t state) override;
    uint16_t getCalibrationZeroOffset() override;
    void     resetCalibrationState() override;
};

#endif // _GC_FortiusController_h

