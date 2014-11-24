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

//
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

// Encoded Calibration is 130 x Calibration Value + 1040 so calibration of zero gives 0x0410

const static uint8_t ergo_command[12] = {
     // 0     1     2     3     4     5     6     7    8      9     10    11
        0x01, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0a, 0x10, 0x04
};

const static uint8_t slope_command[12] = {
     // 0     1     2     3     4     5     6     7    8      9     10    11
        0x01, 0x08, 0x01, 0x00, 0x6c, 0x01, 0x00, 0x00, 0x02, 0x48, 0x10, 0x04
};


    
/* ----------------------------------------------------------------------
 * CONSTRUCTOR/DESRTUCTOR
 * ---------------------------------------------------------------------- */
Fortius::Fortius(QObject *parent) : QThread(parent)
{
    
    devicePower = deviceHeartRate = deviceCadence = deviceSpeed = 0.00;
    mode = FT_IDLE;
    load = DEFAULT_LOAD;
    gradient = DEFAULT_GRADIENT;
    weight = DEFAULT_WEIGHT;
    brakeCalibrationFactor = DEFAULT_CALIBRATION;
    powerScaleFactor = DEFAULT_SCALING;
    deviceStatus=0;
    this->parent = parent;

    /* 12 byte control sequence, composed of 8 command packets
     * where the last packet sets the load. The first byte
     * is a CRC for the value being issued (e.g. Load in WATTS)
     *
     * these members are modified as load / gradient are set
     */
    memcpy(ERGO_Command, ergo_command, 12);
    memcpy(SLOPE_Command, slope_command, 12);

    // for interacting over the USB port
    usb2 = new LibUsb(TYPE_FORTIUS);
}

Fortius::~Fortius()
{
}

/* ----------------------------------------------------------------------
 * SET
 * ---------------------------------------------------------------------- */
void Fortius::setMode(int mode)
{
    pvars.lock();
    this->mode = mode;
    pvars.unlock();
}

// Alters the relationship between brake setpoint at load.
void Fortius::setBrakeCalibrationFactor(double brakeCalibrationFactor)
{
    pvars.lock();
    this->brakeCalibrationFactor = brakeCalibrationFactor;
    pvars.unlock();
}

// output power adjusted by this value so user can compare with hub or crank based readings
void Fortius::setPowerScaleFactor(double powerScaleFactor)
{
    if (weight < 0.8) weight = 0.8;
    if (weight > 1.2) weight = 1.2;
    
    pvars.lock();
    this->powerScaleFactor = powerScaleFactor;
    pvars.unlock();
}

// User weight used by brake in slope mode
void Fortius::setWeight(double weight)
{
    // need to apply range as same byte used to signify erg mode
    if (weight < 50) weight = 50;
    if (weight > 120) weight = 120;
        
    pvars.lock();
    this->weight = weight;
    pvars.unlock();
}

// Load in watts when in power mode
void Fortius::setLoad(double load)
{
    // we can only do 50-1000w on a Fortius
    if (load > 1000) load = 1000;
    if (load < 50) load = 50;
        
    pvars.lock();
    this->load = load;
    pvars.unlock();
}

// Load as slope % when in slope mode
void Fortius::setGradient(double gradient)
{
    if (gradient > 20) gradient = 20;
    if (gradient < -5) gradient = -5;
    
    pvars.lock();
    this->gradient = gradient;
    pvars.unlock();
}


/* ----------------------------------------------------------------------
 * GET
 * ---------------------------------------------------------------------- */
void Fortius::getTelemetry(double &power, double &heartrate, double &cadence, double &speed, double &distance, int &buttons, int &steering, int &status)
{

    pvars.lock();
    power = devicePower;
    heartrate = deviceHeartRate;
    cadence = deviceCadence;
    speed = deviceSpeed;
    distance = deviceDistance;
    buttons = deviceButtons;
    steering = deviceSteering;
    status = deviceStatus;
    
    // work around to ensure controller doesn't miss button press. 
    // The run thread will only set the button bits, they don't get
    // reset until the ui reads the device state
    deviceButtons = 0; 
    pvars.unlock();
}

int Fortius::getMode()
{
    int  tmp;
    pvars.lock();
    tmp = mode;
    pvars.unlock();
    return tmp;
}

double Fortius::getLoad()
{
    double tmp;
    pvars.lock();
    tmp = load;
    pvars.unlock();
    return tmp;
}

double Fortius::getGradient()
{
    double tmp;
    pvars.lock();
    tmp = gradient;
    pvars.unlock();
    return tmp;
}

double Fortius::getWeight()
{
    double tmp;
    pvars.lock();
    tmp = weight;
    pvars.unlock();
    return tmp;
}

double Fortius::getBrakeCalibrationFactor()
{
    double tmp;
    pvars.lock();
    tmp = brakeCalibrationFactor;
    pvars.unlock();
    return tmp;
}

double Fortius::getPowerScaleFactor()
{
    double tmp;
    pvars.lock();
    tmp = powerScaleFactor;
    pvars.unlock();
    return tmp;
}

int
Fortius::start()
{
    pvars.lock();
    this->deviceStatus = FT_RUNNING;
    pvars.unlock();

    QThread::start();
    return 0;
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
int Fortius::restart()
{
    int status;

    // get current status
    pvars.lock();
    status = this->deviceStatus;
    pvars.unlock();
    // what state are we in anyway?
    if (status&FT_RUNNING && status&FT_PAUSED) {
        status &= ~FT_PAUSED;
        pvars.lock();
        this->deviceStatus = status;
        pvars.unlock();
        return 0; // ok its running again!
    }
    return 2;
}

int Fortius::stop()
{
    // what state are we in anyway?
    pvars.lock();
    deviceStatus = 0; // Terminate it!
    pvars.unlock();
    return 0;
}

int Fortius::pause()
{
    int status;

    // get current status
    pvars.lock();
    status = this->deviceStatus;
    pvars.unlock();

    if (status&FT_PAUSED) return 2; // already paused you muppet!
    else if (!(status&FT_RUNNING)) return 4; // not running anyway, fool!
    else {

        // ok we're running and not paused so lets pause
        status |= FT_PAUSED;
        pvars.lock();
        this->deviceStatus = status;
        pvars.unlock();

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

    // variables for telemetry, copied to fields on each brake update
    double curPower;                      // current output power in Watts
    double curHeartRate;                  // current heartrate in BPM
    double curCadence;                    // current cadence in RPM
    double curSpeed;                      // current speed in KPH
    // UNUSED double curDistance;                    // odometer?
    int curButtons;                       // Button status
    int curSteering;                    // Angle of steering controller
    // UNUSED int curStatus;
    uint8_t pedalSensor;                // 1 when using is cycling else 0, fed back to brake although appears unnecessary

    // we need to average out power for the last second
    // since we get updates every 10ms (100hz)
    int powerhist[10];     // last 10 values received
    int powertot=0;        // running total
    int powerindex=0;      // index into the powerhist array
    for (int i=0; i<10; i++) powerhist[i]=0; 
                                        
    // initialise local cache & main vars
    pvars.lock();
    // UNUSED curStatus = this->deviceStatus;
    curPower = this->devicePower = 0;
    curHeartRate = this->deviceHeartRate = 0;
    curCadence = this->deviceCadence = 0;
    curSpeed = this->deviceSpeed = 0;
    // UNUSED curDistance = this->deviceDistance = 0;
    curSteering = this->deviceSteering = 0;
    curButtons = this->deviceButtons = 0;
    pedalSensor = 0;
    pvars.unlock();


    // open the device
    if (openPort()) {
        quit(2);
        return; // open failed!
    } else {
        isDeviceOpen = true;
        sendOpenCommand();
    }

    QTime timer;
    timer.start();

    while(1) {

        if (isDeviceOpen == true) {

            int actualLength = readMessage();
            if (actualLength >= 24) {

                //----------------------------------------------------------------
                // UPDATE BASIC TELEMETRY (HR, CAD, SPD et al)
                // The data structure is very simple, no bit twiddling needed here
                //----------------------------------------------------------------

                // buf[14] changes from time to time (controller status?)
                
                // buttons
                curButtons = buf[13];

                // steering angle
                curSteering = buf[18] | (buf[19] << 8);
                
                // update public fields
                pvars.lock();
                deviceButtons |= curButtons;    // workaround to ensure controller doesn't miss button pushes
                deviceSteering = curSteering;
                pvars.unlock();
            }
            if (actualLength >= 48) {
                // brake status status&0x04 == stopping wheel
                //              status&0x01 == brake on
                //curBrakeStatus = buf[42];
                
                // pedal sensor is 0x01 when cycling
                pedalSensor = buf[46];
                
                // UNUSED curDistance = buf[28] | (buf[29] << 8) | (buf[30] << 16) | (buf[31] << 24);

                // cadence - confirmed correct
                curCadence = buf[44];

                // speed
                curSpeed = 1.3f * (double)(qFromLittleEndian<quint16>(&buf[32])) / (3.6f * 100.00f);

                // power - changed scale from 10 to 13, seems correct in erg mode, slope mode needs work
                curPower = qFromLittleEndian<qint16>(&buf[38])/13;
                if (curPower < 0.0) curPower = 0.0;  // brake power can be -ve when coasting. 
                
                // average power over last 1s
                powertot += curPower;
                powertot -= powerhist[powerindex];
                powerhist[powerindex] = curPower;

                curPower = powertot / 10;
                powerindex = (powerindex == 9) ? 0 : powerindex+1; 

                curPower *= powerScaleFactor; // apply scale factor
                
                // heartrate - confirmed correct
                curHeartRate = buf[12];

                // update public fields
                pvars.lock();
                deviceSpeed = curSpeed;
                deviceCadence = curCadence;
                deviceHeartRate = curHeartRate;
                devicePower = curPower;
                pvars.unlock();
            }
        }

        //----------------------------------------------------------------
        // LISTEN TO GUI CONTROL COMMANDS
        //----------------------------------------------------------------
        pvars.lock();
        curstatus = this->deviceStatus;
        pvars.unlock();

        /* time to shut up shop */
        if (!(curstatus&FT_RUNNING)) {
            // time to stop!
            
            sendCloseCommand();
            
            closePort(); // need to release that file handle!!
            quit(0);
            return;
        }

        if ((curstatus&FT_PAUSED) && isDeviceOpen == true) {
        
            closePort();
            isDeviceOpen = false;

        } else if (!(curstatus&FT_PAUSED) && (curstatus&FT_RUNNING) && isDeviceOpen == false) {

            if (openPort()) {
                quit(2);
                return; // open failed!
            }
            isDeviceOpen = true;        
            sendOpenCommand();
                        
            timer.restart();
        }

        //----------------------------------------------------------------
        // KEEP THE FORTIUS CONNECTION ALIVE
        //----------------------------------------------------------------
        if (isDeviceOpen == true) {

            if (sendRunCommand(pedalSensor) < 0) {
                // send failed - ouch!
                closePort(); // need to release that file handle!!
                quit(4);
                return; // couldn't write to the device
            }
        }
        
        // The controller updates faster than the brake. Setting this to a low value (<50ms) increases the frequency of controller
        // only packages (24byte).
        msleep(10);
    }
}

/* ----------------------------------------------------------------------
 * HIGH LEVEL DEVICE IO ROUTINES
 *
 * sendOpenCommand() - initialises training session
 * sendCloseCommand() - finalises training session
 * sendRunCommand(int) - update brake setpoint
 *
 * ---------------------------------------------------------------------- */
int Fortius::sendOpenCommand()
{
    uint8_t open_command[] = {0x02,0x00,0x00,0x00};
    
    return rawWrite(open_command, 4);
}

int Fortius::sendCloseCommand()
{
    uint8_t close_command[] = {0x01,0x08,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x52,0x10,0x04};
    
    return rawWrite(close_command, 12);
}

int Fortius::sendRunCommand(int16_t pedalSensor)
{
    pvars.lock();
    int mode = this->mode;
    int16_t gradient = (int16_t)this->gradient;
    int16_t load = (int16_t)this->load;
    unsigned int weight = (unsigned int)this->weight;
    int16_t brakeCalibrationFactor = (int16_t)this->brakeCalibrationFactor;
    pvars.unlock();
    
    if (mode == FT_ERGOMODE)
    {
        qToLittleEndian<int16_t>(13 * load, &ERGO_Command[4]);
        ERGO_Command[6] = pedalSensor;
        
        qToLittleEndian<int16_t>(130 * brakeCalibrationFactor + 1040, &ERGO_Command[10]);
                
        return rawWrite(ERGO_Command, 12);
    }
    else if (mode == FT_SSMODE)
    {
        // Tacx driver appears to add an offset to create additional load at
        // zero slope, also seems to be slightly dependent on weight but have
        // ignored this for now.
        qToLittleEndian<int16_t>(1300 * gradient + 507, &SLOPE_Command[4]);
        SLOPE_Command[6] = pedalSensor;
        SLOPE_Command[9] = weight;
        
        qToLittleEndian<int16_t>(130 * brakeCalibrationFactor + 1040, &SLOPE_Command[10]);
        
        return rawWrite(SLOPE_Command, 12);
    }
    else if (mode == FT_IDLE)
    {
        return sendOpenCommand();
    }
    else if (mode == FT_CALIBRATE)
    {
        // Not yet implemented, easy enough to start calibration but appears that the calibration factor needs
        // to be calculated by observing the brake power and speed after calibration starts (i.e. it's not returned
        // by the brake).
    }
    
    return 0;
}

/* ----------------------------------------------------------------------
 * LOW LEVEL DEVICE IO ROUTINES - PORT TO QIODEVICE REQUIRED BEFORE COMMIT
 *
 *
 * readMessage()        - reads an inbound message
 * openPort() - opens serial device and configures it
 * closePort() - closes serial device and releases resources
 * rawRead() - non-blocking read of inbound data
 * rawWrite() - non-blocking write of outbound data
 * discover() - check if a ct is attached to the port specified
 * ---------------------------------------------------------------------- */
int Fortius::readMessage()
{
    int rc;

    rc = rawRead(buf, 64);
    return rc;
}

int Fortius::closePort()
{
    usb2->close();
    return 0;
}

bool Fortius::find()
{
    return usb2->find();
}

int Fortius::openPort()
{
    // on windows we try on USB2 then on USB1 then fail...
    return usb2->open();
}

int Fortius::rawWrite(uint8_t *bytes, int size) // unix!!
{
    return usb2->write((char *)bytes, size);
}

int Fortius::rawRead(uint8_t bytes[], int size)
{
    return usb2->read((char *)bytes, size);
}

// check to see of there is a port at the device specified
// returns true if the device exists and false if not
bool Fortius::discover(QString)
{
    return true;
}
