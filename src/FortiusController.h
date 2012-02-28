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

#ifndef _GC_FortiusController_h
#define _GC_FortiusController_h 1

#include "GoldenCheetah.h"
#include "RealtimeController.h"

#include <QThread>

class Fortius;

class FortiusController : public RealtimeController
{
    Q_OBJECT
public:
    FortiusController (TrainTool *traintool, DeviceConfiguration *);
    virtual ~FortiusController();

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread

    bool find();
    bool discover(DeviceConfiguration *);              // tell if a device is present at port passed

    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void setLoad(double);
    void setGradient(double);
    void setMode(int);

private Q_SLOTS:
    void receiveTelemetry(double power, double heartrate, double cadence, double speed);
    void receiveError(int error);

    void receiveUpButtonPushed();
    void receiveDownButtonPushed();
    void receiveCancelButtonPushed();
    void receiveEnterButtonPushed();

private:
    enum FortiusState {
	Running,
	Stopped
    };

    Fortius* fortius;
    QThread fortiusThread;
    FortiusState _state;
    double _load;
    double _power;
    double _heartrate;
    double _cadence;
    double _speed;
};

#endif // _GC_FortiusController_h

