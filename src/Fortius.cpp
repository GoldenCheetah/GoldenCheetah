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
// 8             0x02 -- 0 - idle 2 - Active
// 9             0x52 -- Mode (?) 52 = idle, 0a = ergo, 48 = slope. 
// 10            0x10 -- UNKNOWN - CONSTANT?
// 11            0x04 -- UNKNOWN - CONSTANT?

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
    mode = FT_ERGOMODE;
    load = DEFAULT_LOAD;
    gradient = DEFAULT_GRADIENT;
    deviceStatus=0;
    this->parent = parent;

    /* 56 byte control sequence, composed of 8 command packets
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
void Fortius::setMode(int mode, double load, double gradient)
{
    pvars.lock();
    this->mode = mode;
    this->load = load;
    this->gradient = gradient;
    pvars.unlock();
}

void Fortius::setLoad(double load)
{
    pvars.lock();
    // we can only do 50-800w on a Fortius
    if (load > 1000) load = 1000;
    if (load < 50) load = 50;
    this->load = load;
    pvars.unlock();
}

void Fortius::setGradient(double gradient)
{
    pvars.lock();
    this->gradient = gradient;
    pvars.unlock();
}


/* ----------------------------------------------------------------------
 * GET
 * ---------------------------------------------------------------------- */
void Fortius::getTelemetry(double &power, double &heartrate, double &cadence, double &speed, int &buttons, int &status)
{

    pvars.lock();
    power = devicePower;
    heartrate = deviceHeartRate;
    cadence = deviceCadence;
    speed = deviceSpeed;
    buttons = deviceButtons;
    status = deviceStatus;
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

int
Fortius::start()
{
    QThread::start();
    return 0;
}

void Fortius::prepareCommand(int mode, double value)
{
    // prepare the control message according to the current mode and gradient/load
    // we do not put the brake on for speeds less than 10kph to ensure it does
    // not burn out
    int16_t encoded;

    switch (mode) {

    case FT_ERGOMODE :

                     if (deviceSpeed < 10) encoded =0;
                     else encoded = 10 * value;
                     qToLittleEndian<int16_t>(encoded, &ERGO_Command[4]); // little endian
                     break;

    case FT_SSMODE : 
                     if (deviceSpeed < 10) encoded =0;
                     else encoded = 650 * value;
                     qToLittleEndian<int16_t>(encoded, &SLOPE_Command[4]); // little endian
                     break;

    }
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
    double newload, newgradient;
    bool isDeviceOpen = false;

    // Cached current values
    // when new values are received from the device
    // if they differ from current values we update
    // otherwise do nothing
    int curmode, curstatus;
    double curload, curgradient;
    double curPower;                      // current output power in Watts
    double curHeartRate;                  // current heartrate in BPM
    double curCadence;                    // current cadence in RPM
    double curSpeed;                      // current speef in KPH
    int curButtons;                       // Button status

    // we need to average out power for the last second
    // since we get updates every 16ms (60hz)
    int powerhist[16];     // last 60s values received
    int powertot=0;        // running total
    int powerindex=0;      // index into the powerhist array
    for (int i=0; i<16; i++) powerhist[i]=0;

    // initialise local cache & main vars
    pvars.lock();
    this->deviceStatus = FT_RUNNING;
    curmode = this->mode;
    curload = this->load;
    curgradient = this->gradient;
    curPower = this->devicePower = 0;
    curHeartRate = this->deviceHeartRate = 0;
    curCadence = this->deviceCadence = 0;
    curSpeed = this->deviceSpeed = 0;
    curButtons = this->deviceButtons;
    curButtons = 0;
    this->deviceButtons = 0;
    pvars.unlock();


    // open the device
    if (openPort()) {
        quit(2);
        return; // open failed!
    } else {
        isDeviceOpen = true;
    }

    // send first command
    prepareCommand(curmode, curmode == FT_ERGOMODE ? curload : curgradient);
    if (sendCommand(curmode) == -1) {

        // send failed - ouch!
        closePort(); // need to release that file handle!!
        quit(4);
        return; // couldn't write to the device
    }


    QTime timer;
    timer.start();

    while(1) {

        if (isDeviceOpen == true) {

            if (readMessage() > 0) {

                msleep(60); //slow down - need to wait for previous interrupt to clear
                            //            before reading. Not sure why, but solves issues
                            //            when working on OSX, and possibly Windows

                //----------------------------------------------------------------
                // UPDATE BASIC TELEMETRY (HR, CAD, SPD et al)
                // The data structure is very simple, no bit twiddling needed here
                //----------------------------------------------------------------

                // buttons
                curButtons = buf[13];

                // brake status status&0x04 == stopping wheel
                //              status&0x01 == brake on
                // buf[42];

                // cadence
                curCadence = buf[44];

                // speed
                curSpeed = (double)(qFromLittleEndian<quint16>(&buf[32])) / 360.00f;

                // power
                int power = qFromLittleEndian<qint16>(&buf[38])/10;

                powertot += power;
                powertot -= powerhist[powerindex];
                powerhist[powerindex] = power;

                curPower = powertot / 16;
                powerindex = (powerindex == 15) ? 0 : powerindex+1;
                
                // heartrate
                curHeartRate = buf[12];

            } else {

                // no data available!? It should block in libusb.. but lets sleep anyway
                msleep (50);
            }

        }

        //----------------------------------------------------------------
        // LISTEN TO GUI CONTROL COMMANDS
        //----------------------------------------------------------------
        pvars.lock();
        curstatus = this->deviceStatus;
        load = curload = newload = this->load;
        gradient = curgradient = newgradient = this->gradient;

        // whilst we are here lets update the values
        deviceButtons = curButtons;
        deviceSpeed = curSpeed;
        deviceCadence = curCadence;
        deviceHeartRate = curHeartRate;
        devicePower = curPower;

        pvars.unlock();

        /* time to shut up shop */
        if (!(curstatus&FT_RUNNING)) {
            // time to stop!
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
            timer.restart();

            // reset smoothing.
            powertot = 0;
            powerindex = 0;
            for (int i=0; i<16; i++) powerhist[i]=0;

            // send first command to get fortius ready
            prepareCommand(curmode, curmode == FT_ERGOMODE ? curload : curgradient);
            if (sendCommand(curmode) == -1) {
                // send failed - ouch!
                closePort(); // need to release that file handle!!
                quit(4);
                return; // couldn't write to the device
            }
        }

        //----------------------------------------------------------------
        // KEEP THE FORTIUS CONNECTION ALIVE
        //----------------------------------------------------------------
        if (isDeviceOpen == true) {

            prepareCommand(curmode, curmode == FT_ERGOMODE ? curload : curgradient);
            if (sendCommand(curmode) == -1) {
                    // send failed - ouch!
                    closePort(); // need to release that file handle!!
                    quit(4);
                    return; // couldn't write to the device
            }
        }
    }
}

/* ----------------------------------------------------------------------
 * LOW LEVEL DEVICE IO ROUTINES - PORT TO QIODEVICE REQUIRED BEFORE COMMIT
 *
 *
 * HIGH LEVEL IO
 * int sendCommand()        - writes a command to the device
 * int readMessage()        - reads an inbound message
 *
 * LOW LEVEL IO
 * openPort() - opens serial device and configures it
 * closePort() - closes serial device and releases resources
 * rawRead() - non-blocking read of inbound data
 * rawWrite() - non-blocking write of outbound data
 * discover() - check if a ct is attached to the port specified
 * ---------------------------------------------------------------------- */
int Fortius::sendCommand(int mode)      // writes a command to the device
{
    switch (mode) {

        case FT_ERGOMODE :
            return rawWrite(ERGO_Command, 12);
            break;

        case FT_SSMODE :
            return rawWrite(SLOPE_Command, 12);
            break;

        default :
            return -1;
            break;
    }
}

int Fortius::readMessage()
{
    int rc;

    rc = rawRead(buf, 48);
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
    int rc=0;

    rc = usb2->write((char *)bytes, size);

    if (!rc) rc = -1; // return -1 if nothing written
    return rc;
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
