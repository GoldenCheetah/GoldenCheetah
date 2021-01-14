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

#include "FortiusController.h"
#include "Fortius.h"
#include "RealtimeData.h"

FortiusController::FortiusController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myFortius = new Fortius (parent);
    myFortius->m_calibrationState = CALIBRATION_STATE_IDLE;
}


int
FortiusController::start()
{
    return myFortius->start();
}


int
FortiusController::restart()
{
    return myFortius->restart();
}


int
FortiusController::pause()
{
    return myFortius->pause();
}


int
FortiusController::stop()
{
    return myFortius->stop();
}

bool
FortiusController::find()
{
    return myFortius->find(); // needs to find either unconfigured or configured device
}

bool
FortiusController::discover(QString) {return false; } // NOT IMPLEMENTED YET


bool FortiusController::doesPush() { return false; }
bool FortiusController::doesPull() { return true; }
bool FortiusController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
FortiusController::getRealtimeData(RealtimeData &rtData)
{
    if(!myFortius->isRunning())
    {
        emit setNotification(tr("Cannot Connect to Fortius"), 2);
        parent->Stop(1);
        return;
    }

    // get latest telemetry
    Fortius::DeviceTelemetry telemetry;
    myFortius->getTelemetry(telemetry);

    //
    // PASS BACK TELEMETRY
    //
    rtData.setWatts(telemetry.Power_W);
    rtData.setHr(telemetry.HeartRate);
    rtData.setCadence(telemetry.Cadence);
    rtData.setSpeed(Fortius::ms_to_kph(telemetry.Speed_ms));


    // post processing, probably not used
    // since its used to compute power for
    // non-power devices, but we may add other
    // calculations later that might apply
    // means we could calculate power based
    // upon speed even for a Fortius!
    processRealtimeData(rtData);

    //
    // BUTTONS
    //

    // ignore other buttons if calibrating
    if (parent->calibrating) return;


    // ADJUST LOAD
    if (telemetry.Buttons & Fortius::FT_PLUS)   parent->Higher();
    if (telemetry.Buttons & Fortius::FT_MINUS)  parent->Lower();

    // LAP/INTERVAL
    if (telemetry.Buttons & Fortius::FT_ENTER)  parent->newLap();

    // CANCEL
    if (telemetry.Buttons & Fortius::FT_CANCEL) parent->Stop(0);


    // Ensure we set the UI load to the actual setpoint from the fortius (as it will clamp)
    rtData.setLoad(myFortius->getLoad());
    rtData.setSlope(myFortius->getGradient());
}

void FortiusController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void
FortiusController::setLoad(double load)
{
    myFortius->setLoad(load);
}

void
FortiusController::setGradientWithSimState(double gradient, double targetForce_N, double speed_kph)
{
    myFortius->setGradientWithSimState(gradient, targetForce_N, speed_kph);
}

void
FortiusController::setMode(int mode)
{
    if (mode == RT_MODE_ERGO) mode = Fortius::FT_ERGOMODE;
    else if (mode == RT_MODE_SPIN) mode = Fortius::FT_SSMODE;
    else mode = Fortius::FT_IDLE;

    myFortius->setMode(mode);
}

void
FortiusController::setWeight(double weight)
{
    myFortius->setWeight(weight);
}


// Calibration

uint8_t
FortiusController::getCalibrationType()
{
    return CALIBRATION_TYPE_FORTIUS;
}

double
FortiusController::getCalibrationTargetSpeed()
{
    return 20; // kph
}

uint8_t
FortiusController::getCalibrationState()
{
    return myFortius->m_calibrationState;
}

void
FortiusController::setCalibrationState(uint8_t state)
{
    myFortius->m_calibrationState = state;
    switch (state)
    {
    case CALIBRATION_STATE_IDLE:
        myFortius->setMode(Fortius::FT_IDLE);
        break;

    case CALIBRATION_STATE_PENDING:
        myFortius->setMode(Fortius::FT_CALIBRATE);
        myFortius->m_calibrationState = CALIBRATION_STATE_REQUESTED;
        break;

    default:
        break;
    }
}

// I don't know if this is the right hook to use
// or if I should be adding a new, better suited function
uint16_t
FortiusController::getCalibrationZeroOffset()
{
    switch (myFortius->m_calibrationState)
    {
        // Waiting for use to kick pedal...
        case CALIBRATION_STATE_REQUESTED:
        {
            Fortius::DeviceTelemetry telemetry;
            myFortius->getTelemetry(telemetry);

            if (telemetry.Speed_ms > Fortius::kph_to_ms(19.9))
            {
                myFortius->m_calibrationValues.reset();
                myFortius->m_calibrationState = CALIBRATION_STATE_STARTING;
            }
            return 0;
        }

        // Calibration starting, waiting until we have enough values
        case CALIBRATION_STATE_STARTING:
        {
            // Get current value and push onto the list of recent values
            Fortius::DeviceTelemetry telemetry;
            myFortius->getTelemetry(telemetry);

            const double latest = telemetry.Force_N;
            myFortius->m_calibrationValues.update(latest);

            // unexpected resistance (pedalling) will cause calibration to terminate
            if (latest > 0)
            {
                myFortius->m_calibrationState = CALIBRATION_STATE_FAILURE;
                return 0;
            }

            if (myFortius->m_calibrationValues.is_full())
            {
                myFortius->m_calibrationState = CALIBRATION_STATE_STARTED;
                myFortius->m_calibrationStarted = time(nullptr);
            }
            return Fortius::N_to_rawForce(latest);
        }

        // Calibration started, runs until standard deviation is below some threshold
        case CALIBRATION_STATE_STARTED:
        {
            // Get current value and push onto the list of recent values
            Fortius::DeviceTelemetry telemetry;
            myFortius->getTelemetry(telemetry);

            const double latest = telemetry.Force_N;
            myFortius->m_calibrationValues.update(latest);

            // unexpected resistance (pedalling) will cause calibration to terminate
            if (latest > 0)
            {
                myFortius->m_calibrationState = CALIBRATION_STATE_FAILURE;
                return 0;
            }

            // calculate the average
            const double mean   = myFortius->m_calibrationValues.mean();
            const double stddev = myFortius->m_calibrationValues.stddev();

            // wait until stddev within threshold
            // perhaps this isn't the best numerical solution to detect settling
            // but it's better than the previous attempt which was based on diff(min/max)
            // I'd prefer a tighter threshold, eg 0.02
            // but runtime would be too long for users, especially from cold
            static const double stddev_threshold = 0.05;

            // termination (settling) conditions
            if (stddev < stddev_threshold
                || (time(nullptr) - myFortius->m_calibrationStarted) > calibrationDurationLimit_s)
            {
                // accept the current average as the final valibration value
                myFortius->setBrakeCalibrationForce(-mean);
                myFortius->m_calibrationState = CALIBRATION_STATE_SUCCESS;
                myFortius->setMode(Fortius::FT_IDLE);

                // TODO... just because we have determined the current value
                // doesn't mean it's "right"... TTS4 had a red-green bar and
                // plotted the value on that bar, and user was to adjust wheel
                // pressure if it was outside the green range.
                // There were no units. We should probably look at that...
            }

            // Need to return a uint16_t, and TrainSidebar displays to user as raw value
            return Fortius::N_to_rawForce(-(myFortius->m_calibrationValues.is_full() ? mean : latest));
        }

        case CALIBRATION_STATE_SUCCESS:
            return Fortius::N_to_rawForce(myFortius->getBrakeCalibrationForce());

        default:
            return 0;
    }
}

void
FortiusController::resetCalibrationState()
{
    myFortius->m_calibrationState = CALIBRATION_STATE_IDLE;
    myFortius->setMode(Fortius::FT_IDLE);
}
