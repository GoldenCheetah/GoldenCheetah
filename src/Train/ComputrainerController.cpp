/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "ComputrainerController.h"
#include "Computrainer.h"
#include "RealtimeData.h"

ComputrainerController::ComputrainerController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myComputrainer = new Computrainer (parent, dc ? dc->portSpec : ""); // we may get NULL passed when configuring
    f1Depressed = false;
    f2Depressed = false;
    f3Depressed = false;
}


int
ComputrainerController::start()
{
    return myComputrainer->start();
}


int
ComputrainerController::restart()
{
    return myComputrainer->restart();
}


int
ComputrainerController::pause()
{
    return myComputrainer->pause();
}


int
ComputrainerController::stop()
{
    return myComputrainer->stop();
}


bool
ComputrainerController::discover(QString name)
{
    return myComputrainer->discover(name);  // go probe it...
}


bool ComputrainerController::doesPush() { return false; }
bool ComputrainerController::doesPull() { return true; }
bool ComputrainerController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
ComputrainerController::getRealtimeData(RealtimeData &rtData)
{
    int Buttons, Status;
    bool calibration;
    double Power, HeartRate, Cadence, Speed, RRC, Load, Gradient;
    uint8_t ss[24];

    if(!myComputrainer->isRunning())
    {
        emit setNotification(tr("Cannot Connect to Computrainer"), 2);
        parent->Stop(1);
        return;
    }

    // get latest telemetry
    myComputrainer->getTelemetry(Power, HeartRate, Cadence, Speed,
                        RRC, calibration, Buttons, ss, Status);

	// Check CT if F3 has been pressed for Calibration mode FIRST before we do anything else
    if (Buttons&CT_F3) {
        // We're only interested in the act of pressing the button, not it being held down
        if (f3Depressed == false) {
            f3Depressed = true;
            parent->Calibrate();
        }
    } else {
        f3Depressed = false; // It has been released
    }

    // ignore other buttons and anything else if calibrating
    if (parent->calibrating) return;

    //
    // PASS BACK TELEMETRY
    //
    rtData.setWatts(Power);
    rtData.setHr(HeartRate);
    rtData.setCadence(Cadence);
    rtData.setSpeed(Speed);

    memcpy(rtData.spinScan, ss, 24);

    // post processing, probably not used
    // since its used to compute power for
    // non-power devices, but we may add other
    // calculations later that might apply
    // means we could calculate power based
    // upon speed even for CT!
    processRealtimeData(rtData);

    //
    // BUTTONS
    //

    // ADJUST LOAD & GRADIENT
    Load = myComputrainer->getLoad();
    Gradient = myComputrainer->getGradient();
	// the calls to the parent will determine which mode we are on (ERG/SPIN) and adjust load/slop appropriately
    if (Buttons&CT_PLUS) {
        parent->Higher();
    }
    if (Buttons&CT_MINUS) {
        parent->Lower();
    }
    rtData.setLoad(Load);
	rtData.setSlope(Gradient);

    // Start/Pause
    if (Buttons&CT_F1) {
        // We're only interested in the act of pressing the button, not it being held down
        if (f1Depressed == false) {
            f1Depressed = true;
            parent->Start();
        }
    } else {
        f1Depressed = false; // It has been released
    }

    // LAP/INTERVAL
    if (Buttons&CT_F2) {
        // We're only interested in the act of pressing the button, not it being held down
        if (f2Depressed == false) {
            f2Depressed = true;
            parent->newLap();
        }
    } else {
        f2Depressed = false; // It has been released
    }

    // if Buttons == 0 we just pressed stop!
    if (Buttons&CT_RESET) {
        parent->Stop(0);
    }

}

void ComputrainerController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void
ComputrainerController::setLoad(double load)
{
    myComputrainer->setLoad(load);
}

void
ComputrainerController::setGradient(double grade)
{
    myComputrainer->setGradient(grade);
}
void
ComputrainerController::setMode(int mode)
{
    if (mode == RT_MODE_ERGO) mode = CT_ERGOMODE;
    if (mode == RT_MODE_SPIN) mode = CT_SSMODE;
    if (mode == RT_MODE_CALIBRATE) mode = CT_CALIBRATE;
    myComputrainer->setMode(mode);
}
