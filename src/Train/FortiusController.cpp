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
    double Power, Resistance, HeartRate, Cadence, Speed, Distance;

    if(!myFortius->isRunning())
    {
        emit setNotification(tr("Cannot Connect to Fortius"), 2);
        parent->Stop(1);
        return;
    }
    // get latest telemetry
    myFortius->getTelemetry(Power, Resistance, HeartRate, Cadence, Speed, Distance, Buttons, Steering, Status);

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
FortiusController::setWindSpeed(double ws)
{
    myFortius->setWindSpeed(ws);
}

void
FortiusController::setRollingResistance(double rr)
{
    myFortius->setRollingResistance(rr);
}

void
FortiusController::setWindResistance(double wr)
{
    myFortius->setWindResistance(wr);
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
    return 20;
}

uint8_t
FortiusController::getCalibrationState()
{
    return calibrationState;
}

void
FortiusController::setCalibrationState(uint8_t state)
{
    calibrationState = state;
    switch (state)
    {
    case CALIBRATION_STATE_IDLE:
        myFortius->setMode(FT_IDLE);
        break;

    case CALIBRATION_STATE_PENDING:
        myFortius->setMode(FT_CALIBRATE);
        calibrationState = CALIBRATION_STATE_STARTING;
        break;

    default:
        break;
    }
}

uint16_t
FortiusController::getCalibrationZeroOffset()
{
    static int final_calibration_value = 0;

    switch (calibrationState)
    {
        // Waiting for use to kick pedal...
        case CALIBRATION_STATE_STARTING:
        {
            final_calibration_value = 0;
            if (readCurrentSpeedValue() > 19.9)
            {
                calibrationState = CALIBRATION_STATE_STARTED;
            }
            return 0;
        }

        // Calibration running
        case CALIBRATION_STATE_STARTED:
        {
            // keep a note of the last N calibration values
            // keep running calibration until the last N values differ by less than some threshold M
            static const int num_samples = 20; // N
            static const int difference_threshold = 10; // M
            static std::list<int> calibration_values;
           
            // Get current value and push onto the list of recent values 
            double calibration_value = readCurrentCalibrationValue();
            calibration_values.push_back(calibration_value);

            // Only do anything if we have a sufficient number of samples
            if (calibration_values.size() > num_samples)
            {
                // maintain a constant number of samples
                calibration_values.pop_front();

                // unexpected resistance (pedalling) will cause calibration to terminate
                if (calibration_value > 0)
                {
                    calibrationState = CALIBRATION_STATE_FAILURE;
                    return 0;
                }

                // calculate the difference between the minimum and maximum calibration values of those recently received
                const int min_calibration_value = *std::min_element(calibration_values.begin(), calibration_values.end());
                const int max_calibration_value = *std::max_element(calibration_values.begin(), calibration_values.end());
                if (abs(min_calibration_value - max_calibration_value) < difference_threshold) // termination (settling) condition
                {
                    // Difference between min and max is within threshold, stop here.
                    calibrationState = CALIBRATION_STATE_SUCCESS;

                    // accept the average as the final valibration value
                    final_calibration_value = std::accumulate(calibration_values.begin(), calibration_values.end(), 0.0) / calibration_values.size();
                    calibration_values.clear();

                    // Tell the trainer object what the calibration value is
                    myFortius->setCalibrationValue(-final_calibration_value);
                    myFortius->setMode(FT_IDLE);

                    return final_calibration_value;
                }
            }
            return calibration_value; 
        }

        case CALIBRATION_STATE_SUCCESS:
            // return the last determined calibration value to be displayed to user in GUI
            return final_calibration_value;

        default:
            return 0;
    }
}

void
FortiusController::resetCalibrationState()
{
    calibrationState = CALIBRATION_STATE_IDLE;
    myFortius->setMode(FT_IDLE);
}

// FIXME dirty pair of functions - refactor
double
FortiusController::readCurrentCalibrationValue()
{
    double power, resistance, heartrate, cadence, speed, distance;
    int buttons, steering, status;
    myFortius->getTelemetry(power, resistance, heartrate, cadence, speed, distance, buttons, steering, status);
    return resistance;
}

double
FortiusController::readCurrentSpeedValue()
{
    double power, resistance, heartrate, cadence, speed, distance;
    int buttons, steering, status;
    myFortius->getTelemetry(power, resistance, heartrate, cadence, speed, distance, buttons, steering, status);
    return speed;
}
