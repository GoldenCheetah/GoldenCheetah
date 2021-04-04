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
    myFortius = new Fortius (this);
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
    // Added Distance and Steering here but yet to RealtimeData

    int Buttons, Status, Steering;
    double Power, Force_N, HeartRate, Cadence, Speed, Distance;

    if(!myFortius->isRunning())
    {
        emit setNotification(tr("Cannot Connect to Fortius"), 2);
        parent->Stop(1);
        return;
    }
    // get latest telemetry
    myFortius->getTelemetry(Power, Force_N, HeartRate, Cadence, Speed, Distance, Buttons, Steering, Status);

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
    // upon speed even for a Fortius!
    processRealtimeData(rtData);

    //
    // BUTTONS
    //

    // ignore other buttons if calibrating
    if (parent->calibrating) return;

    // ADJUST LOAD
    if ((Buttons&FT_PLUS)) parent->Higher();
    
    if ((Buttons&FT_MINUS)) parent->Lower();

    // LAP/INTERVAL
    if (Buttons&FT_ENTER) parent->newLap();

    // CANCEL
    if (Buttons&FT_CANCEL) parent->Stop(0);

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
FortiusController::setGradient(double grade)
{
    myFortius->setGradient(grade);
}

void
FortiusController::setMode(int mode)
{
    if (mode == RT_MODE_ERGO) mode = FT_ERGOMODE;
    else if (mode == RT_MODE_SPIN) mode = FT_SSMODE;
    else mode = FT_IDLE;
    
    myFortius->setMode(mode);
}

void
FortiusController::setWeight(double weight)
{
    myFortius->setWeight(weight);

}

void
FortiusController::setWindSpeed(double speed)
{
    myFortius->setWindSpeed(speed);
}

void
FortiusController::setRollingResistance(double rollingResistance)
{
    myFortius->setRollingResistance(rollingResistance);
}

void
FortiusController::setWindResistance(double windResistance)
{
    myFortius->setWindResistance(windResistance);
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
    return m_calibrationState;
}

void
FortiusController::setCalibrationState(uint8_t state)
{
    m_calibrationState = state;
    switch (state)
    {
    case CALIBRATION_STATE_IDLE:
        myFortius->setMode(FT_IDLE);
        break;

    case CALIBRATION_STATE_PENDING:
        myFortius->setMode(FT_CALIBRATE);
        m_calibrationState = CALIBRATION_STATE_REQUESTED;
        break;

    default:
        break;
    }
}

uint16_t
FortiusController::getCalibrationZeroOffset()
{
    switch (m_calibrationState)
    {
        // Waiting for user to kick pedal, or speed to decay...
        case CALIBRATION_STATE_REQUESTED:
        {
            int Buttons, Status, Steering;
            double Power, Force_N, HeartRate, Cadence, SpeedKmh, Distance;
            myFortius->getTelemetry(Power, Force_N, HeartRate, Cadence, SpeedKmh, Distance, Buttons, Steering, Status);

            // Once we're at target speed (only takes a second or so), move on to next stage
            if (SpeedKmh > 0.995 * getCalibrationTargetSpeed() && SpeedKmh < 1.005 * getCalibrationTargetSpeed()) // Within 0.005 of the target speed
            {
                m_calibrationValues.reset();
                m_calibrationState = CALIBRATION_STATE_STARTING;
            }

            // Return value to determine message to user in the GUI
            //  -  0 instructs user to "Give the pedal a kick"
            //  - >0 instructs user to "Allow wheel speed to settle"
            return SpeedKmh;
        }

        // Calibration starting, waiting until we have enough values
        case CALIBRATION_STATE_STARTING:
        {
            // Get current force value
            int Buttons, Status, Steering;
            double Power, Force_N, HeartRate, Cadence, SpeedKmh, Distance;
            myFortius->getTelemetry(Power, Force_N, HeartRate, Cadence, SpeedKmh, Distance, Buttons, Steering, Status);

            // unexpected drop in wheel speed will cause calibration to terminate
            if (SpeedKmh < 0.9 * getCalibrationTargetSpeed()) // 10% underspeed would be considered an error case
            {
                m_calibrationState = CALIBRATION_STATE_FAILURE;
                return 0;
            }

            // Push onto the list of recent values
            m_calibrationValues.update(Force_N);

            // If we have enough values, move on to next stage
            if (m_calibrationValues.is_full())
            {
                m_calibrationState = CALIBRATION_STATE_STARTED;
                m_calibrationStarted = time(nullptr);
            }

            // Return value for presentation in the GUI
            // Need to return a uint16_t, use +ve rawForce
            return Fortius::N_to_rawForce(-Force_N);
        }

        // Calibration started, runs until standard deviation is below some threshold
        case CALIBRATION_STATE_STARTED:
        {
            // Get current force value
            int Buttons, Status, Steering;
            double Power, Force_N, HeartRate, Cadence, SpeedKmh, Distance;
            myFortius->getTelemetry(Power, Force_N, HeartRate, Cadence, SpeedKmh, Distance, Buttons, Steering, Status);

            // Push onto the list of recent values
            m_calibrationValues.update(Force_N);

            // unexpected resistance (pedalling) will cause calibration to terminate
            if (Force_N > 0)
            {
                m_calibrationState = CALIBRATION_STATE_FAILURE;
                return 0;
            }

            // unexpected drop in wheel speed will cause calibration to terminate
            if (SpeedKmh < 0.9 * getCalibrationTargetSpeed()) // 10% underspeed would be considered an error case
            {
                m_calibrationState = CALIBRATION_STATE_FAILURE;
                return 0;
            }

            // calculate the average
            const double mean   = m_calibrationValues.mean();
            const double stddev = m_calibrationValues.stddev();

            // wait until stddev within threshold
            // perhaps this isn't the best numerical solution to detect settling
            // but it's better than the previous attempt which was based on diff(min/max)
            // I'd prefer a tighter threshold, eg 0.02
            // but runtime would be too long for users, especially from cold
            static const double stddev_threshold = 0.05;

            // termination (settling) conditions
            const time_t calibrationDuration = time(nullptr) - m_calibrationStarted;
            if (stddev < stddev_threshold || calibrationDuration > calibrationDurationLimit_s)
            {
                // TODO... just because we have determined the current value
                // doesn't mean it's "right"... TTS4 had a red-green bar and
                // plotted the value on that bar, and user was to adjust wheel
                // pressure if it was outside the green range.

                // Source: https://github.com/totalreverse/ttyT1941/wiki
                //   ; If the 'gauge' of the calibration screen is perfect in the
                //   ; "green" middle of the calibration bar, then TTS4 sets the
                //   ; calibration value to 0x0492 (= 90*13) 


                // accept the current average as the final valibration value
                // Note: call setBrakeCalibrationForce() with +ve value
                myFortius->setBrakeCalibrationForce(-mean);

                // complete calibration
                m_calibrationState = CALIBRATION_STATE_SUCCESS;
                myFortius->setMode(FT_IDLE);
            }

            // Return value for presentation in the GUI
            // Need to return a uint16_t, use +ve rawForce
            return Fortius::N_to_rawForce(-mean);
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
    m_calibrationState = CALIBRATION_STATE_IDLE;
    myFortius->setMode(FT_IDLE);
}

