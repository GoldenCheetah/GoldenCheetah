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

ComputrainerController::ComputrainerController(RealtimeWindow *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myComputrainer = new Computrainer (parent, dc->portSpec);
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
ComputrainerController::discover(DeviceConfiguration *) {return false; } // NOT IMPLEMENTED YET


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
    double Power, HeartRate, Cadence, Speed, RRC, Load;

    if(!myComputrainer->isRunning())
    {
        QMessageBox msgBox;
        msgBox.setText("Cannot Connect to Computrainer");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(1);
        return;
    }
    // get latest telemetry
    myComputrainer->getTelemetry(Power, HeartRate, Cadence, Speed,
                        RRC, calibration, Buttons, Status);

    //
    // PASS BACK TELEMETRY
    //
    rtData.setWatts(Power);
    rtData.setHr(HeartRate);
    rtData.setCadence(Cadence);
    rtData.setSpeed(Speed);

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

    // ADJUST LOAD
    Load = myComputrainer->getLoad();
    if ((Buttons&CT_PLUS) && !(Buttons&CT_F3)) {
            parent->Higher();
    }
    if ((Buttons&CT_MINUS) && !(Buttons&CT_F3)) {
            parent->Lower();
    }
    rtData.setLoad(Load);

    // FFWD/REWIND
    if ((Buttons&CT_PLUS) && (Buttons&CT_F3)) {
           parent->FFwd();
    }
    if ((Buttons&CT_MINUS) && (Buttons&CT_F3)) {
           parent->Rewind();
    }


    // LAP/INTERVAL
    if (Buttons&CT_F1 && !(Buttons&CT_F3)) {
        parent->newLap();
    }
    if ((Buttons&CT_F1) && (Buttons&CT_F3)) {
           parent->FFwdLap();
    }

    // if Buttons == 0 we just pressed stop!
    if (Buttons&CT_RESET) {
        parent->Stop(0);
    }

    // displaymode
    if (Buttons&CT_F2) {
        parent->nextDisplayMode();
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
    myComputrainer->setMode(mode);
}
