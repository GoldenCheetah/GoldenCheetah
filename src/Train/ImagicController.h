#ifndef IMAGICCONTROLLER_H
#define IMAGICCONTROLLER_H 1
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
#include "Imagic.h"

#include <QMessageBox>


class ImagicController : public RealtimeController
{
    Q_OBJECT

public:
    ImagicController (TrainSidebar *, DeviceConfiguration *);

    Imagic *myImagic;               // the device itself
    int steerCalibrate = 0;         // Steering detection
    int steerActive = 0;
    int steerStraight = 128;
    float steerDrift = 128;
    int pressCount = 0;
    int noPressCount = 0;

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread

    bool find();
    bool discover(QString);              // tell if a device is present at port passed


    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void pushRealtimeData(RealtimeData &rtData);
    void setLoad(double);
    void setGradient(double);
    void setMode(int);
    void setWeight(double);
};


#endif // IMAGICCONTROLLER_H
