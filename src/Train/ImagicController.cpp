/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRAfNTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ImagicController.h"
#include "Imagic.h"
#include "RealtimeData.h"

ImagicController::ImagicController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myImagic = new Imagic (parent);
}


int
ImagicController::start()
{
    return myImagic->start();
}


int
ImagicController::restart()
{
    return myImagic->restart();
}


int
ImagicController::pause()
{
    return myImagic->pause();
}


int
ImagicController::stop()
{
    return myImagic->stop();
}

bool
ImagicController::find()
{
    return myImagic->find(); // needs to find either unconfigured or configured device
}

bool
ImagicController::discover(QString) {return false; } // NOT IMPLEMENTED YET


bool ImagicController::doesPush() { return false; }
bool ImagicController::doesPull() { return true; }
bool ImagicController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button and steering
 * status too and act accordingly.
 *
 */
void
ImagicController::getRealtimeData(RealtimeData &rtData)
{
    // Added Distance and Steering here but yet to RealtimeData

    int Buttons, Status, Steering;
    double Power, HeartRate, Cadence, Speed, Distance;

    if(!myImagic->isRunning())
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Imagic"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(1);
        parent->Disconnect();
        return;
    }
    // get latest telemetry
    myImagic->getTelemetry(Power, HeartRate, Cadence, Speed, Distance, Buttons, Steering, Status);

    //
    // PASS BACK TELEMETRY
    //
    rtData.setWatts(Power);
    rtData.setHr(HeartRate);
    rtData.setCadence(Cadence);
    rtData.setSpeed(Speed);


    // For now, all the power calculations are contained
    // here, so we don't strictly need to call this. It might be useful
    // in the future though as our existing power calculations
    // leave something to be desired at low speeds.
    processRealtimeData(rtData);

    //
    // Handlebar controls
    //

    // Ignore controls if calibrating
    if (parent->calibrating) return;

    // Check for button presses
    if (Buttons == 0xf0) {
        if (noPressCount < 100) ++noPressCount;
        pressCount = 0;
    }
    else {
        if (pressCount < 100) ++pressCount;
        // UP or DOWN
        // Adjusts intensity
        if ((Buttons&IM_PLUS) == 0 && (noPressCount > 0 || pressCount > 5)) parent->Higher();
        if ((Buttons&IM_MINUS) == 0 && (noPressCount > 0 || pressCount > 5)) parent->Lower();

        // START
        // If not running, start. If paused, restart. Otherwise start new lap
        if ((Buttons&IM_ENTER) == 0 && noPressCount > 4) {
            if (!parent->context->isRunning || parent->context->isPaused) parent->Start();
            else  parent->newLap();
        }

        // CANCEL
        // Press once to pause, press again (while paused) to stop
        if ((Buttons&IM_CANCEL) == 0 && noPressCount > 4) {
            if (parent->context->isRunning) {
                if (parent->context->isPaused) parent->Stop(0);
                else  parent->Start();
            }
        }
        noPressCount = 0;
    }

    //
    // Steering left/right to scroll train view up/down
    //
    if (steerCalibrate > 50 && steerActive < 75) {
        // Steering is calibrated and seems to be within bounds
        if (Steering > (steerStraight+10)) {
            if (Steering > (steerStraight+20))
                parent->steerScroll(+2);
            else
                parent->steerScroll(+1);
            ++steerActive;
        }
        else if (Steering < (steerStraight-10)) {
            if (Steering < (steerStraight-20))
                parent->steerScroll(-2);
            else
                parent->steerScroll(-1);
            ++steerActive;
        }
        else {
            steerActive = 0;
            // Allow for drift in "Straight ahead" value
            steerDrift = ((steerDrift * 9.0f) + Steering) / 10.0f;
            if ((steerDrift - (float)steerStraight) > 1.0f || (steerDrift - (float)steerStraight) < -1.0f)
                steerStraight = (int)steerDrift;
        }
    }
    // Steering requires calibration
    else {
         // If we seem to be steering constantly left or right - recalibrate
         if (steerActive >= 75) {
             steerCalibrate = 0;
             steerActive = 0;
             steerStraight = 128;
             parent->steerScroll(0);
         }
         // Take average steering value over ~ 10secs and use that as new "straight ahead" value
         if (Steering > 0) {
             ++steerCalibrate;
             steerStraight = steerDrift = (steerStraight*steerCalibrate + Steering) / (steerCalibrate + 1);
         }
    }

    // Ensure we set the UI load to the actual setpoint from the imagic (as it will clamp)
    rtData.setLoad(myImagic->getLoad());
    rtData.setSlope(myImagic->getGradient());
}

void ImagicController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void
ImagicController::setLoad(double load)
{
    myImagic->setLoad(load);
}

void
ImagicController::setGradient(double grade)
{
    myImagic->setGradient(grade);
}

void
ImagicController::setMode(int mode)
{
    if (mode == RT_MODE_ERGO) mode = IM_ERGOMODE;
    else if (mode == RT_MODE_SPIN) mode = IM_SSMODE;
    else mode = IM_IDLE;

    myImagic->setMode(mode);
}

void
ImagicController::setWeight(double weight)
{
    myImagic->setWeight(weight);
}

