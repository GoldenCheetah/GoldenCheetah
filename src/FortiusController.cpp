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

#include "FortiusController.h"
#include "Fortius.h"
#include "RealtimeData.h"

#include <QMetaObject>

static const int DEFAULT_LOAD = 100.0;
static const int DEFAULT_GRADIENT = 0.0;

FortiusController::FortiusController(TrainTool *parent,  DeviceConfiguration *dc) :
    RealtimeController(parent, dc),
    fortius(new Fortius(Fortius::ErgoMode, DEFAULT_LOAD, DEFAULT_GRADIENT)),
    _state(Stopped),
    _load(DEFAULT_LOAD), _power(0.0), _heartrate(0.0), _cadence(0.0), _speed(0.0)
{
    connect(fortius, SIGNAL(signalTelemetry(double,double,double,double)),
	    SLOT(receiveTelemetry(double,double,double,double)));
    connect(fortius, SIGNAL(error(int)), SLOT(receiveError(int)));
    connect(fortius, SIGNAL(upButtonPushed()), SLOT(receiveUpButtonPushed()));
    connect(fortius, SIGNAL(downButtonPushed()), SLOT(receiveDownButtonPushed()));
    connect(fortius, SIGNAL(cancelButtonPushed()), SLOT(receiveCancelButtonPushed()));
    connect(fortius, SIGNAL(enterButtonPushed()), SLOT(receiveEnterButtonPushed()));


}

FortiusController::~FortiusController()
{
    stop();
    delete fortius;
}

int FortiusController::start()
{
    fortiusThread.start();
    fortius->moveToThread(&fortiusThread);

    QMetaObject::invokeMethod(fortius, "start");

    _state = Running;
    return 0;
}


int FortiusController::restart()
{
    QMetaObject::invokeMethod(fortius, "restart");

    _state = Running;
    return 0;
}


int FortiusController::pause()
{
    QMetaObject::invokeMethod(fortius, "pause");

    return 0;
}


int FortiusController::stop()
{
    if (_state == Running)
    {
        QMetaObject::invokeMethod(fortius, "stop");

        _state = Stopped;
        fortiusThread.quit();
        fortiusThread.wait();
    }
    return 0;
}

bool FortiusController::find()
{
    return Fortius::find();
}

bool FortiusController::discover(DeviceConfiguration *) {return false; } // NOT IMPLEMENTED YET


bool FortiusController::doesPush() { return false; }
bool FortiusController::doesPull() { return true; }
bool FortiusController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void FortiusController::getRealtimeData(RealtimeData &rtData)
{
    if (_state == Running) {
	rtData.setWatts(_power);
	rtData.setHr(_heartrate);
	rtData.setCadence(_cadence);
	rtData.setSpeed(_speed);
	rtData.setLoad(_load);
	// post processing, probably not used
	// since its used to compute power for
	// non-power devices, but we may add other
	// calculations later that might apply
	// means we could calculate power based
	// upon speed even for a Fortius!
	processRealtimeData(rtData);
    } else {
	QMessageBox msgBox;
	msgBox.setText("Cannot Connect to Fortius");
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.exec();
	parent->Stop(1);
    }
}

void FortiusController::setLoad(double load)
{
    _load = load;
    QMetaObject::invokeMethod(fortius, "changeLoad", Q_ARG(double, load));
}

void FortiusController::setGradient(double grade)
{
    QMetaObject::invokeMethod(fortius, "changeGradient", Q_ARG(double, grade));
}

void FortiusController::setMode(int mode)
{
    if (mode == RT_MODE_ERGO)
        QMetaObject::invokeMethod(fortius, "useErgoMode");
    if (mode == RT_MODE_SPIN)
        QMetaObject::invokeMethod(fortius, "useSlopeMode");
}

void FortiusController::receiveTelemetry(double power, double heartrate,
					 double cadence, double speed)
{
    _power = power;
    _heartrate = heartrate;
    _cadence = cadence;
    _speed = speed;
}

void FortiusController::receiveError(int) {
    _state = Stopped;
}

void FortiusController::receiveUpButtonPushed()
{
    parent->Higher();
}

void FortiusController::receiveDownButtonPushed()
{
    parent->Lower();
}

void FortiusController::receiveEnterButtonPushed()
{
    parent->newLap();
}

void FortiusController::receiveCancelButtonPushed()
{
    parent->Stop();
}
