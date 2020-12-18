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

#include "Fortius.h"

#include <Core/Settings.h>
#define TRAIN_FORTIUSCALIBRATION       "<global-trainmode>train/fortiuscalibration"
// could (should?) be defined in Core/Settings.h
// but they are not currently used anywhere other than Fortius.cpp

#include <QtCore/qendian.h>

class Lock
{
    QMutex& mutex;
public:
    Lock(QMutex& m) : mutex(m) { mutex.lock(); }
    ~Lock() { mutex.unlock(); }
};

/* ----------------------------------------------------------------------
 * CONSTRUCTOR/DESRTUCTOR
 * ---------------------------------------------------------------------- */
Fortius::Fortius(QObject *parent) : QThread(parent)
{
    this->parent = parent;

    _device.Force_N = _device.Power_W = _device.HeartRate = _device.Cadence = _device.Speed_ms = 0.00;
    deviceStatus=0;

    _control.mode                         = FT_IDLE;
    _control.brakeCalibrationFactor       = 0.0;
    _control.brakeCalibrationForce_N      = 0.0; // loaded from settings on connect, or set upon calibration
    _control.targetPower_W                = 100.0;
    _control.gradient                     = 2.0;
    _control.weight_kg                    = 77;
    _control.powerScaleFactor             = 1.0;

    // for interacting over the USB port
    usb2 = new LibUsb(TYPE_FORTIUS);
}

Fortius::~Fortius()
{
    delete usb2;
}

/* ----------------------------------------------------------------------
 * SET
 * ---------------------------------------------------------------------- */
void Fortius::setMode(int mode)
{
    Lock lock(pvars);
    _control.mode = mode;
}

// Alters the relationship between brake setpoint at load.
void Fortius::setBrakeCalibrationFactor(double brakeCalibrationFactor)
{
    Lock lock(pvars);
    _control.brakeCalibrationFactor = brakeCalibrationFactor;
}

// output power adjusted by this value so user can compare with hub or crank based readings
void Fortius::setPowerScaleFactor(double powerScaleFactor)
{
    if (powerScaleFactor < 0.8) powerScaleFactor = 0.8;
    if (powerScaleFactor > 1.2) powerScaleFactor = 1.2;

    Lock lock(pvars);
    _control.powerScaleFactor = powerScaleFactor;
}

// User weight used by brake in slope mode
void Fortius::setWeight(double weight_kg)
{
    // need to apply range as same byte used to signify erg mode
    if (weight_kg < 50)  weight_kg = 50;
    if (weight_kg > 120) weight_kg = 120;

    Lock lock(pvars);
    _control.weight_kg = weight_kg;
}

void Fortius::setBrakeCalibrationForce(double force_N)
{
    // save raw calibration value
    appsettings->setValue(TRAIN_FORTIUSCALIBRATION, static_cast<int>(N_to_rawForce(force_N)));

    Lock lock(pvars);
    _control.brakeCalibrationForce_N = force_N;
}

// Load in watts when in power mode
void Fortius::setLoad(double targetPower_W)
{
    Lock lock(pvars);
    _control.targetPower_W = targetPower_W;
}

// Resistance in newtons when implementing 'slope' mode
void Fortius::setGradientWithSimState(double gradient, double targetForce_N, double speed_kph)
{
    Lock lock(pvars);
    _control.targetForce_N = targetForce_N;
    _control.simSpeed_ms   = kph_to_ms(speed_kph); // for aligning simulator and trainer speeds
    _control.gradient      = gradient;
}

/* ----------------------------------------------------------------------
 * GET
 * ---------------------------------------------------------------------- */
void Fortius::getTelemetry(DeviceTelemetry& copy)
{
    Lock lock(pvars);
    copy = _device;

    // work around to ensure controller doesn't miss button press.
    // The run thread will only set the button bits, they don't get
    // reset until the ui reads the device state
    _device.Buttons = 0;
}

int Fortius::getMode() const
{
    Lock lock(pvars);
    return _control.mode;
}

double Fortius::getLoad() const
{
    Lock lock(pvars);
    return _control.targetPower_W;
}

double Fortius::getGradient() const
{
    Lock lock(pvars);
    return _control.gradient;
}

double Fortius::getWeight() const
{
    Lock lock(pvars);
    return _control.weight_kg;
}

double Fortius::getBrakeCalibrationForce() const
{
    Lock lock(pvars);
    return _control.brakeCalibrationForce_N;
}

double Fortius::getBrakeCalibrationFactor() const
{
    Lock lock(pvars);
    return _control.brakeCalibrationFactor;
}

double Fortius::getPowerScaleFactor() const
{
    Lock lock(pvars);
    return _control.powerScaleFactor;
}



/* ----------------------------------------------------------------------
 * EXECUTIVE FUNCTIONS
 *
 * start() - start/re-start reading telemetry in a thread
 * stop() - stop reading telemetry and terminates thread
 * pause() - discards inbound telemetry (ignores it)
 *
 *
 * THE MEAT OF THE CODE IS IN RUN() IT IS A WHILE LOOP CONSTANTLY
 * READING TELEMETRY AND ISSUING CONTROL COMMANDS WHILST UPDATING
 * MEMBER VARIABLES AS TELEMETRY CHANGES ARE FOUND.
 *
 * run() - bg thread continuosly reading/writing the device port
 *         it is kicked off by start and then examines status to check
 *         when it is time to pause or stop altogether.
 * ---------------------------------------------------------------------- */
int
Fortius::start()
{
    {
        Lock lock(pvars);
        deviceStatus = FT_RUNNING;

        // Lead raw calibration value, and convert to N
        const double raw_saved_calibration = appsettings->value(this, TRAIN_FORTIUSCALIBRATION, 0x0410).toInt();
        _control.brakeCalibrationForce_N = rawForce_to_N(raw_saved_calibration);
    }

    QThread::start();
    return 0;
}

int Fortius::restart()
{
    // get current status
    Lock lock(pvars);

    int status = deviceStatus;

    // what state are we in anyway?
    if (status & FT_RUNNING && status & FT_PAUSED)
    {
        status &= ~FT_PAUSED;
        deviceStatus = status;
        return 0; // ok its running again!
    }
    return 2;
}

int Fortius::stop()
{
    // what state are we in anyway?
    Lock lock(pvars);
    deviceStatus = 0; // Terminate it!
    return 0;
}

int Fortius::pause()
{
    Lock lock(pvars);

    // get current status
    int status = deviceStatus;

    if (status & FT_PAUSED) return 2; // already paused you muppet!
    else if (!(status & FT_RUNNING)) return 4; // not running anyway, fool!
    else {
        // ok we're running and not paused so lets pause
        status |= FT_PAUSED;
        deviceStatus = status;

        return 0;
    }
}

// used by thread to set variables and emit event if needed
// on unexpected exit
int Fortius::quit(int code)
{
    // event code goes here!
    exit(code);
    return 0; // never gets here obviously but shuts up the compiler!
}

/*----------------------------------------------------------------------
 * THREADED CODE - READS TELEMETRY AND SENDS COMMANDS TO KEEP FORTIUS ALIVE
 *----------------------------------------------------------------------*/
void Fortius::run()
{

    // newly read values - compared against cached values
    bool isDeviceOpen = false;

    // ui controller state
    int curstatus;

    // local copy of variables for telemetry, copied to fields on each brake update
    DeviceTelemetry cur;
    getTelemetry(cur);

    uint8_t pedalSensor = 0;                // 1 when using is cycling else 0, fed back to brake although appears unnecessary

    // open the device
    if (openPort()) {
        quit(2);
        return; // open failed!
    } else {
        isDeviceOpen = true;
        sendCommand_OPEN();
    }

    QTime timer;
    timer.start();

    while(1) {

        if (isDeviceOpen == true) {

            int rc = sendRunCommand(cur.Speed_ms, pedalSensor) ;
            if (rc < 0) {
                qDebug() << "usb write error " << rc;
                // send failed - ouch!
                closePort(); // need to release that file handle!!
                quit(4);
                return; // couldn't write to the device
            }

            int actualLength = readMessage();
            if (actualLength < 0) {
                qDebug() << "usb read error " << actualLength;
            }

            //----------------------------------------------------------------
            // UPDATE BASIC TELEMETRY (HR, CAD, SPD et al)
            // The data structure is very simple, no bit twiddling needed here
            //----------------------------------------------------------------
            // The first 12 bytes are almost always identical, with buf[10] being
            // exceptionally 40 instead of 50:
            //
            // 20 9f 00 00 08 01 00 00 07 00 50 bd
            //
            // buf[12] is heart rate
            // buf[13] is buttons
            // buf[14, 15] change from time to time, 00 00 or 00 01 (controller status?)
            // buf[16, 17] remain around 6d 02
            // buf[18, 19] is the steering
            // buf[20 - 27] remain constant at d0 07 d0 07 03 13 02 00
            // buf[28 - 31] is the distance
            // buf[32, 33] is the speed
            // buf[34] varies around 2c
            // buf[35] jumps mostly between 02 and 0e.
            // buf[36, 37] vary with the speed but go to 0 more quickly than the speed or power
            // buf[38, 39] is the torque output of the cyclist
            // buf[40, 41] also vary somewhat with speed
            // buf[42] pedal sensor, normally 0, 1 when the pedal passes near the sensor
            // buf[43] remains 0
            // buf[44, 45] cadence
            // buf[46] is 0x02 when active, 0x00 at the end
            // buf[47] varies even when the system is idle

            if (actualLength >= 24) {
                cur.HeartRate = buf[12];
                cur.Buttons   = buf[13];
                cur.Steering  = buf[18] | (buf[19] << 8);
            }

            if (actualLength >= 48) {
                // brake status status&0x04 == stopping wheel
                //              status&0x01 == brake on
                //curBrakeStatus = buf[46?];

                // pedal sensor is 0x01 when crank passes sensor
                pedalSensor   = buf[42];

                // UNUSED curDistance = (buf[28] | (buf[29] << 8) | (buf[30] << 16) | (buf[31] << 24)) / 16384.0;

                cur.Cadence   = buf[44];

                // speed

                cur.Speed_ms  = rawSpeed_to_ms(qFromLittleEndian<quint16>(&buf[32]));

                // Power is torque * wheelspeed - adjusted by device resistance factor.
                cur.Force_N   = rawForce_to_N(qFromLittleEndian<qint16>(&buf[38]));
                cur.Power_W   = cur.Force_N * cur.Speed_ms;
                cur.Power_W  *= getPowerScaleFactor(); // apply scale factor

                if (cur.Power_W < 0.0) cur.Power_W = 0.0;  // brake power can be -ve when coasting.
            }

            if (actualLength >= 24) // ie, if either of the two blocks above were executed
            {
                // update public fields
                Lock lock(pvars);
                _device = cur;
            }
        }

        //----------------------------------------------------------------
        // LISTEN TO GUI CONTROL COMMANDS
        //----------------------------------------------------------------
        {
            Lock lock(pvars);
            curstatus = deviceStatus;
        }

        /* time to shut up shop */
        if (!(curstatus & FT_RUNNING)) {
            // time to stop!

            // Once is not enough...
            // edgecase:
            // - calibrate
            // - while motor running hit stop, it keeps running
            // - then hit disconnect, it KEEPS running
            // - unless you call twice, as below
            sendCommand_IDLE();
            msleep(100);
            sendCommand_IDLE();

            closePort(); // need to release that file handle!!
            quit(0);
            return;
        }

        if ((curstatus & FT_PAUSED) && isDeviceOpen == true) {

            closePort();
            isDeviceOpen = false;

        } else if (!(curstatus & FT_PAUSED) && (curstatus & FT_RUNNING) && isDeviceOpen == false) {

            if (openPort()) {
                quit(2);
                return; // open failed!
            }
            isDeviceOpen = true;
            sendCommand_OPEN();

            timer.restart();
        }


        // The controller updates faster than the brake. Setting this to a low value (<50ms) increases the frequency of controller
        // only packages (24byte). Tacx software uses 100ms.
        msleep(50);
    }
}

/* ----------------------------------------------------------------------
 * HIGH LEVEL DEVICE IO AND COMMAND ENCODING ROUTINES
 *
 * sendRunCommand(int) - update brake setpoint
 *
 * sendCommmand_OPEN()          - used to start device
 * sendCommmand_CLOSE()         - put device in idle mode
 * sendCommmand_RESISTANCE(...) - set trainer resistance and flywheel
 *                              - common to ERGO and SLOPE modes
 * sendCommmand_CALIBRATE(...)  - put trainer in calibration mode
 * sendCommmand_GENERIC(...)    - generic message to control trainer
 *                              - common to all but Command_OPEN() above
 *
 * ---------------------------------------------------------------------- */

namespace {
    double UpperForceLimit(double v, double F) // m/s and N
    {
        // We think the trainer has an inherent, absolute-maximum Force limit
        // See: https://www.qbp.com/diagrams/TechInfo/Tacx/MA9571.pdf
        // Here we could assume Fortius is like i-Genius, and has ~118N max force
        // Instead, we will set conservative value
        static const double F_absmax = 100.;

        // Below a certain speed, max force is no longer constant
        // In other words, at low speeds, max power is no longer linear

        // The proposed low-speed model is:
        //   F_max_v = v^2 * Q
        // where:
        //   F_max_v is the maximum force we should command at speed v
        //       v^2 is the square of the velocity (v in m/s)
        //         Q is some constant derived empirically
        static const double Q = 8.; // model suggests 10, 8 is conservative

        const double F_max_v = v*v * Q;

        // We must never return >F_absmax
        // We must never return >F_max_v, for current value of v
        return std::min({F, F_max_v, F_absmax});
    }
}

int Fortius::sendRunCommand(double deviceSpeed_ms, int16_t pedalSensor)
{
    pvars.lock();
    const ControlParameters c = _control; // local copy
    pvars.unlock();

    // Depending on mode, send the appropriate command to the trainer
    int rc = 0;
    switch (c.mode)
    {
        case FT_IDLE:
            rc = sendCommand_OPEN();
            break;

        case FT_CALIBRATE:
            rc = sendCommand_CALIBRATE(kph_to_ms(20));
            break;

        case FT_ERGOMODE:
            {
                // Calculate force requires, given power set-point and current device speed
                // Note: avoid divide by zero
                const double targetForce_N = c.targetPower_W / std::max(0.1, deviceSpeed_ms);

                // Ensure that load never exceeds physical limit of device using UpperForceLimit
                const double limitedTargetForce_N = UpperForceLimit(deviceSpeed_ms, targetForce_N);

                // Send to trainer
                rc = sendCommand_RESISTANCE(limitedTargetForce_N, pedalSensor, 0x0a /*10kg flywheel*/);
            }
            break;

        case FT_SSMODE:
            {
                // Simply use targetForce_N (aka resistanceNewtons) provided by sim in setGradientWithSimState()

                // TODO wheel speed matching
                // First propsal is:
                //     const double ratio         = 1 + (deviceSpeed_ms - c.simSpeed_ms) / c.simSpeed_ms;
                //     const double targetForce_N = c.targetForce_N * ratio*ratio*ratio;
                //     const double weight_kg     = some_f(c.weight_kg, deviceSpeed_ms, c.simSpeed_ms)

                // Ensure that load never exceeds physical limit of device using UpperForceLimit
                const double limitedTargetForce_N = UpperForceLimit(deviceSpeed_ms, c.targetForce_N);

                // Send to trainer
                rc = sendCommand_RESISTANCE(limitedTargetForce_N, pedalSensor, c.weight_kg);
            }
            break;

        default:
            break;
    }
    return rc;
}


// Outbound control message has the format:
// Byte          Value / Meaning
// 0             0x01 CONSTANT
// 1             0x08 CONSTANT
// 2             0x01 CONSTANT
// 3             0x00 CONSTANT
// 4             Brake Value - Lo Byte
// 5             Brake Value - Hi Byte
// 6             Echo cadence sensor
// 7             0x00 -- UNKNOWN
// 8             0x02 -- 0 - idle, 2 - Active, 3 - Calibration
// 9             0x52 -- Mode 0a = ergo, weight for slope mode (48 = 72kg), 52 = idle (in conjunction with byte 8)
// 10            Calibration Value - Lo Byte
// 11            Calibration High - Hi Byte

int Fortius::sendCommand_OPEN()
{
    static const uint8_t command[4] = {0x02,0x00,0x00,0x00};
    return rawWrite(command, 4);
}

int Fortius::sendCommand_GENERIC(uint8_t mode, double rawForce, uint8_t pedecho, uint8_t weight_kg, uint16_t calibration)
{
    uint8_t command[12] = { 0x01, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    const double resistance = std::min<double>(SHRT_MAX, rawForce);
    qToLittleEndian<int16_t>(resistance, &command[4]);

    command[6] = pedecho;
    command[8] = mode;
    command[9] = weight_kg;

    qToLittleEndian<int16_t>(calibration, &command[10]);
    return rawWrite(command, 12);
}

int Fortius::sendCommand_IDLE()
{
    return sendCommand_GENERIC(FT_MODE_IDLE, 0, 0, 0x52 /* flywheel enabled at 82 kg */, 0);
}

int Fortius::sendCommand_RESISTANCE(double force_N, uint8_t pedecho, uint8_t weight_kg)
{
    const double brakeCalibrationFactor = getBrakeCalibrationFactor(); // thread-safe
    const double brakeCalibrationForce_N = getBrakeCalibrationForce(); // thread-safe
    const double calibration = (130 * brakeCalibrationFactor) + N_to_rawForce(brakeCalibrationForce_N);

    return sendCommand_GENERIC(FT_MODE_ACTIVE, N_to_rawForce(force_N), pedecho, weight_kg, calibration);
}

int Fortius::sendCommand_CALIBRATE(double speed_ms)
{
    return sendCommand_GENERIC(FT_MODE_CALIBRATE, ms_to_rawSpeed(speed_ms), 0, 0, 0);
}


/* ----------------------------------------------------------------------
 * LOW LEVEL DEVICE IO ROUTINES - PORT TO QIODEVICE REQUIRED BEFORE COMMIT
 *
 *
 * readMessage() - reads an inbound message
 * openPort()    - opens serial device and configures it
 * closePort()   - closes serial device and releases resources
 * rawRead()     - non-blocking read of inbound data
 * rawWrite()    - non-blocking write of outbound data
 * discover()    - check if a ct is attached to the port specified
 * ---------------------------------------------------------------------- */
int Fortius::readMessage()
{
    int rc;

    rc = rawRead(buf, 64);
    //qDebug() << "usb status " << rc;
    return rc;
}

int Fortius::closePort()
{
    usb2->close();
    return 0;
}

bool Fortius::find()
{
    int rc;
    rc = usb2->find();
    //qDebug() << "usb status " << rc;
    return rc;
}

int Fortius::openPort()
{
    int rc;
    // on windows we try on USB2 then on USB1 then fail...
    rc = usb2->open();
    //qDebug() << "usb status " << rc;
    return rc;
}

int Fortius::rawWrite(const uint8_t *bytes, int size) // unix!!
{
    return usb2->write((char *)bytes, size, FT_USB_TIMEOUT);
}

int Fortius::rawRead(uint8_t bytes[], int size)
{
    return usb2->read((char *)bytes, size, FT_USB_TIMEOUT);
}

// check to see of there is a port at the device specified
// returns true if the device exists and false if not
bool Fortius::discover(QString)
{
    return true;
}
