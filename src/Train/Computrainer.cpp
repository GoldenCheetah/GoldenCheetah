/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net),
 *                    Mark Liversedge (liversedge@gmail.com)
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

// ISSUES
// ======
// 1. SpinScan decoding is not implemented (but erg and slope mode are)
// 2. Some C-style casts used for expediency

#include "Computrainer.h"

const static uint8_t ergo_command[56] = {
//                        Ergo            various
//      crc     -     -   mode  cmd   val   bits
//      ----  ----  ----  ----  ----  ----  ----
        0x6D, 0x00, 0x00, 0x0A, 0x08, 0x00, 0xE0,
        0x65, 0x00, 0x00, 0x0A, 0x10, 0x00, 0xE0,
        0x00, 0x00, 0x00, 0x0A, 0x18, 0x5D, 0xC1,
        0x33, 0x00, 0x00, 0x0A, 0x24, 0x1E, 0xE0,
        0x6A, 0x00, 0x00, 0x0A, 0x2C, 0x5F, 0xE0,
        0x41, 0x00, 0x00, 0x0A, 0x34, 0x00, 0xE0,
        0x2D, 0x00, 0x00, 0x0A, 0x38, 0x10, 0xC2,
        0x03, 0x00, 0x00, 0x0A, 0x40, 0x32, 0xE0    // set LOAD command
};

const static uint8_t ss_command[56] = {
//                      Spinscan           various
//      crc     -     -   mode  cmd   val   bits
//      ----  ----  ----  ----  ----  ----  ----
        0x61, 0x00, 0x00, 0x16, 0x08, 0x00, 0xE0,   // set GRADIENT command
        0x59, 0x00, 0x00, 0x16, 0x10, 0x00, 0xE0,   // set WINDSPEED (?)
        0x74, 0x00, 0x00, 0x16, 0x18, 0x5D, 0xC1,
        0x27, 0x00, 0x00, 0x16, 0x24, 0x1E, 0xE0,
        0x5E, 0x00, 0x00, 0x16, 0x2C, 0x5F, 0xE0,
        0x35, 0x00, 0x00, 0x16, 0x34, 0x00, 0xE0,
        0x21, 0x00, 0x00, 0x16, 0x38, 0x10, 0xC2,
        0x29, 0x00, 0x00, 0x16, 0x40, 0x00, 0xE0
};

const static uint8_t rrc_command[56] = {

        0x31, 0x00, 0x00, 0x0e, 0x40, 0x00, 0xe0,
        0x69, 0x00, 0x00, 0x0e, 0x08, 0x00, 0xe0,
        0x61, 0x00, 0x00, 0x0e, 0x10, 0x00, 0xe0,
        0x78, 0x00, 0x00, 0x0e, 0x18, 0x61, 0xc1,
        0x4d, 0x00, 0x00, 0x0e, 0x24, 0x00, 0xe0,
        0x6e, 0x00, 0x00, 0x0e, 0x2c, 0x57, 0xc1,
        0x3d, 0x00, 0x00, 0x0e, 0x34, 0x00, 0xe0,
        0x23, 0x00, 0x00, 0x0e, 0x38, 0x16, 0xc2

};    

/* ----------------------------------------------------------------------
 * CONSTRUCTOR/DESRTUCTOR
 * ---------------------------------------------------------------------- */
Computrainer::Computrainer(QObject *parent,  QString devname) : QThread(parent)
{

    devicePower = deviceHeartRate = deviceCadence = deviceSpeed = deviceRRC = 0.00;
    for (int i=0; i<24; spinScan[i++] = 0) ;
    mode = DEFAULT_MODE;
    load = DEFAULT_LOAD;
    gradient = DEFAULT_GRADIENT;
    deviceCalibrated = false;
    deviceHRConnected = false;
    deviceCADConnected = false;
    setDevice(devname);
    deviceStatus=0;
    this->parent = parent;

    /* 56 byte control sequence, composed of 8 command packets
     * where the last packet sets the load. The first byte
     * is a CRC for the value being issued (e.g. Load in WATTS)
     *
     * these members are modified as load / gradient are set
     */
    memcpy(ERGO_Command, ergo_command, 56);
    memcpy(SS_Command, ss_command, 56);
}

Computrainer::~Computrainer()
{
}

/* ----------------------------------------------------------------------
 * SET
 * ---------------------------------------------------------------------- */
void Computrainer::setDevice(QString devname)
{
    // if not null, replace existing if set, otherwise set
    deviceFilename = devname;
}

void Computrainer::setMode(int mode, double load, double gradient)
{
    pvars.lock();
    this->mode = mode;
    this->load = load;
    this->gradient = gradient;
    pvars.unlock();
}

void Computrainer::setLoad(double load)
{
    pvars.lock();
    if (load > 1500) load = 1500;
    if (load < 50) load = 50;
    this->load = load;
    pvars.unlock();
}

void Computrainer::setGradient(double gradient)
{
    pvars.lock();
    this->gradient = gradient;
    pvars.unlock();
}


/* ----------------------------------------------------------------------
 * GET
 * ---------------------------------------------------------------------- */
bool Computrainer::isHRConnected()
{
    bool tmp;
    pvars.lock();
    tmp = deviceHRConnected;
    pvars.unlock();

    return tmp;
}

bool Computrainer::isCADConnected()
{
    bool tmp;
    pvars.lock();
    tmp = deviceCADConnected;
    pvars.unlock();

    return tmp;
}

bool Computrainer::isCalibrated()
{
    bool tmp;
    pvars.lock();
    tmp = deviceCalibrated;
    pvars.unlock();

    return tmp;
}

void Computrainer::getTelemetry(double &power, double &heartrate, double &cadence, double &speed,
                  double &RRC, bool &calibration, int &buttons, uint8_t *ss, int &status)
{

    pvars.lock();
    power = devicePower;
    heartrate = deviceHeartRate;
    cadence = deviceCadence;
    speed = deviceSpeed;
    RRC = deviceRRC;
    calibration = deviceCalibrated;
    buttons = deviceButtons;
    status = deviceStatus;
    memcpy((void*)ss, (void*)spinScan, 24);

    // work around to ensure controller doesn't miss button press. 
    // The run thread will only set the button bits, they don't get
    // reset until the ui reads the device state
    //  Borrowed from: Fortius.cpp 
    deviceButtons = 0; 
    pvars.unlock();
}

void Computrainer::getSpinScan(double spinData[])
{
    pvars.lock();
    for (int i=0; i<24; spinData[i] = this->spinScan[i]) ;
    pvars.unlock();
}

int Computrainer::getMode()
{
    int  tmp;
    pvars.lock();
    tmp = mode;
    pvars.unlock();
    return tmp;
}

double Computrainer::getLoad()
{
    double tmp;
    pvars.lock();
    tmp = load;
    pvars.unlock();
    return tmp;
}

double Computrainer::getGradient()
{
    double tmp;
    pvars.lock();
    tmp = gradient;
    pvars.unlock();
    return tmp;
}

/*----------------------------------------------------------------------
 * COMPUTRAINER PROTOCOL DECODE/ENCODE ROUTINES
 *                           (The clever stuff)
 *
 * Many thanks to Stephan Mantler for sharing some of his initial
 * findings and source code.
 *
 *                             -- NOTE --
 *
 *    ALTHOUGH THESE ROUTINES READ AND WRITE FROM/TO THE LOW LEVEL
 *    IO BUFFERS THEY *DO NOT* WRITE TO THE CLASS MEMBERS DIRECTLY
 *    THIS IS TO MINIMISE RACE CONDITIONS. THE EXECUTIVE FUNCTIONS
 *    ESPECIALLY run() CONTROL ACCESS TO THESE AND CACHE VALUES IN
 *    LOCAL VARS TO MINIMISE THIS.
 *
 *        ** IF YOU MODIFY THIS CODE PLEASE BEAR THIS IN MIND **
 *
 * The protocol definition and reference code has been put up at
 * https://www.sourceforge.net/projects/ctmac
 *
 * void prepareCommand()    - sets up the command packet according to current settings
 * int calcCRC(double value)       - calculates the checksum for the current command
 * int unpackTelemetry()  - unpacks from CT protocol layout to byte layout
 *                          returns non-zero if unknown message type
 *
 *---------------------------------------------------------------------- */

int
Computrainer::start()
{
    QThread::start();
    return 0;
}

void Computrainer::prepareCommand(int mode, double value)
{
    // prepare the control message according to the current mode and gradient/load
    int load, gradient, crc;

    switch (mode) {

    case CT_ERGOMODE :
                load = (int)value;
                crc = calcCRC(load);

                // BYTE 0 - 49 is b0, 53 is b4, 54 is b5, 55 is b6
                ERGO_Command[49] = crc >> 1; // set byte 0

                // BYTE 4 - command and highbyte
                ERGO_Command[53]  = 0x40; // set command
                ERGO_Command[53] |= (load&(2048+1024+512)) >> 9;

                // BYTE 5 - low 7
                ERGO_Command[54] = 0;
                ERGO_Command[54] |= (load&(128+64+32+16+8+4+2)) >> 1;

                // BYTE 6 - sync + z set
                ERGO_Command[55] = 128+64;

                // low bit of supplement in bit 6 (32)
                ERGO_Command[55] |= crc & 1 ? 32 : 0;
                // Bit 2 (0x02) is low bit of high byte in load (bit 9 0x256)
                ERGO_Command[55] |= (load&256) ? 2 : 0;
                // Bit 1 (0x01) is low bit of low byte in load (but 1 0x01)
                ERGO_Command[55] |= load&1;

                break;

    case CT_SSMODE :

                if (value < -9.9) gradient = -99;
                else if (value > 15) gradient = 150;
                else gradient = value *10;        // gradient is passed as an integer

                // negative gradients use two's complement with bit 2048 set
                if (gradient < 0) {
                        gradient *= -1;                // make it positive
                        gradient &= 1024+512+256+128+64+32+16+8+4+2+1; // the number
                        gradient = ~gradient;        // two's complement
                }

                crc = calcCRC(gradient);

                // BYTE 0 - 49 is b0, 53 is b4, 54 is b5, 55 is b6
                SS_Command[0] = crc >> 1; // set byte 0

                // BYTE 4 - command and highbyte
                SS_Command[4]  = 0x08; // set command - slope set
                SS_Command[4] |= (gradient&(2048+1024+512)) >> 9;

                // BYTE 5 - low 7
                SS_Command[5] = 0;
                SS_Command[5] |= (gradient&(128+64+32+16+8+4+2)) >> 1;

                // BYTE 6 - sync + z set
                SS_Command[6] = 128+64;

                // low bit of supplement in bit 6 (32)
                SS_Command[6] |= crc & 1 ? 32 : 0;
                // Bit 2 (0x02) is low bit of high byte in load (bit 9 0x256)
                SS_Command[6] |= (gradient&256) ? 2 : 0;
                // Bit 1 (0x01) is low bit of low byte in load (but 1 0x01)
                SS_Command[6] |= gradient&1;
                break;

    }
}

// thanks to Sean Rhea for working this one out!
int Computrainer::calcCRC(int value)
{
    return (0xff & (107 - (value & 0xff) - (value >> 8)));
}


// funny, just a few lines of code. oh the pain to get this working :-)
void Computrainer::unpackTelemetry(int &ss1, int &ss2, int &ss3, int &buttons, int &type, int &value8, int &value12)
{
    static uint8_t ss[24];
    static int pos=0;

    // inbound data is in the 7 byte array Computrainer::buf[]
    // for code clarity they hjave been put into these holdiing
    // variables. the overhead is minimal and makes the code a
    // lot easier to decipher! :-)

    short s1 = buf[0]; // ss data
    short s2 = buf[1]; // ss data
    short s3 = buf[2]; // ss data
    short bt = buf[3]; // button data
    short b1 = buf[4]; // message and value
    short b2 = buf[5]; // value
    short b3 = buf[6]; // the dregs (sync, z and lsb for all the others)

    // ss vars
    ss1 = s1<<1 | (b3&32)>>5;
    ss2 = s2<<1 | (b3&16)>>4;
    ss3 = s3<<1 | (b3&8)>>3;

    // swap nibbles, is this really right?
    ss1 = ((ss1&15)<<4) | (ss1&240)>>4;
    ss2 = ((ss2&15)<<4) | (ss2&240)>>4;
    ss3 = ((ss3&15)<<4) | (ss3&240)>>4;

    // buttons
    buttons = bt<<1 | (b3&4)>>2;

    // 4-bit message type
    type = (b1&120)>>3;

    // 8 bit value
    value8 = (b2&~128)<<1 | (b3&1); // 8 bit values

    // 12 bit value
    value12 = value8 | (b1&7)<<9 | (b3&2)<<7;

    if (buttons&64) {
        memcpy((uint8_t*)spinScan, (uint8_t*)ss+3, 21);
        memcpy((uint8_t*)spinScan+21, (uint8_t*)ss, 3);
        //for (pos=0; pos<24; pos++) fprintf(stderr, "%d, ", ss[pos]);
        //fprintf(stderr, "\n");
        pos=0;
    }
    if (ss1 || ss2 || ss3) {

        // we drop the msb and do a ones compliment, but
        // that looks eerily like a signed byte.
        // suspect there is more hidden in there?
        // but when decoded as a signed byte the numbers
        // are all over the place. investigate further!
        ss[pos++] = 127^(ss1&127);
        ss[pos++] = 127^(ss2&127);
        ss[pos++] = 127^(ss3&127);
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
int Computrainer::restart()
{
    int status;

    // get current status
    pvars.lock();
    status = this->deviceStatus;
    pvars.unlock();
    // what state are we in anyway?
    if (status&CT_RUNNING && status&CT_PAUSED) {
            status &= ~CT_PAUSED;
            pvars.lock();
            this->deviceStatus = status;
            pvars.unlock();
            return 0; // ok its running again!
    }
    return 2;
}

int Computrainer::stop()
{
    // what state are we in anyway?
    pvars.lock();
    deviceStatus = 0; // Terminate it!
    pvars.unlock();
    return 0;
}

int Computrainer::pause()
{
    int status;

    // get current status
    pvars.lock();
    status = this->deviceStatus;
    pvars.unlock();

    if (status&CT_PAUSED) return 2; // already paused you muppet!
    else if (!(status&CT_RUNNING)) return 4; // not running anyway, fool!
    else {

            // ok we're running and not paused so lets pause
            status |= CT_PAUSED;
            pvars.lock();
            this->deviceStatus = status;
            pvars.unlock();

            return 0;
    }
}

// used by thread to set variables and emit event if needed
// on unexpected exit
int Computrainer::quit(int code)
{
    // event code goes here!
    exit(code);
    return 0; // never gets here obviously but shuts up the compiler!
}

/*----------------------------------------------------------------------
 * THREADED CODE - READS TELEMETRY AND SENDS COMMANDS TO KEEP CT ALIVE
 *----------------------------------------------------------------------*/
void Computrainer::run()
{

    // locally cached settings - only update main class variables
    //                           when they change

    int cmds=0;            // count loops with no command sent

    // holders for unpacked telemetry
    int ss1,ss2,ss3, buttons, type, value8, value12;

    // newly read values - compared against cached values
    int newmode;
    double newload, newgradient;
    double newspeed, newRRC;
    bool newhrconnected, newcadconnected;
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
    double curSpeed;                      // current speed in KPH
    double curRRC;                        // calibrated Rolling Resistance
    bool curhrconnected;                  // is HR sensor connected?
    bool curcadconnected;                 // is CAD sensor connected?
    int curButtons;                       // Button status


    // initialise local cache & main vars
    pvars.lock();
    this->deviceStatus = CT_RUNNING;
    curmode = this->mode;
    curload = this->load;
    curgradient = this->gradient;
    curPower = this->devicePower = 0;
    curHeartRate = this->deviceHeartRate = 0;
    curCadence = this->deviceCadence = 0;
    curSpeed = this->deviceSpeed = 0;
    curButtons = this->deviceButtons = 0;
    curRRC = this->deviceRRC = 0;
    this->deviceCalibrated = false;
    curhrconnected = false;
    this->deviceHRConnected = false;
    curcadconnected = false;
    this->deviceCADConnected = false;
    pvars.unlock();


    // open the device
    if (openPort()) {
        quit(2);
        return; // open failed!
    } else {
        isDeviceOpen = true;
    }

    // send first command to get computrainer ready
    prepareCommand(curmode, curmode == CT_ERGOMODE ? curload : curgradient);
    if (sendCommand(curmode) == -1) {
        // send failed - ouch!
        closePort(); // need to release that file handle!!
        quit(4);
        return; // couldn't write to the device
    }


    while(1) {

        if (isDeviceOpen == true) {

            if (readMessage() > 0) {

                //----------------------------------------------------------------
                // UPDATE BASIC TELEMETRY (HR, CAD, SPD et al)
                //----------------------------------------------------------------

               unpackTelemetry(ss1, ss2, ss3, buttons, type, value8, value12);

               switch (type) {
                    case CT_HEARTRATE :
                        if (value8 != curHeartRate) {
                            curHeartRate = value8;
                            pvars.lock();
                            this->deviceHeartRate = curHeartRate;
                            pvars.unlock();
                        }
                        break;

                    case CT_POWER :
                        if (value12 != curPower) {
                            curPower = value12;
                            pvars.lock();
                            this->devicePower = curPower;
                            pvars.unlock();
                        }
                        break;

                    case CT_CADENCE :
                        if (value8 != curCadence) {
                            curCadence = value8;
                            pvars.lock();
                            this->deviceCadence = curCadence;
                            pvars.unlock();
                        }
                        break;

                    case CT_SPEED :
                        value12  *=36;      // convert from mps to kph
                        value12  *=9;
                        value12  /=10;      // it seems that compcs takes off 10% ????
                        newspeed = value12;
                        newspeed  /= 1000;
                        if (newspeed != curSpeed) {
                            pvars.lock();
                            this->deviceSpeed = curSpeed = newspeed;
                            pvars.unlock();
                        }
                        break;

                    case CT_RRC :
                        //newcalibrated = value12&2048 ? true : false;
                        newRRC = value12&~2048; // only use 11bits
                        newRRC /= 256;

                        if (newRRC != curRRC) {
                            pvars.lock();
                            this->deviceRRC = curRRC = newRRC;
                            pvars.unlock();
                        }
                        break;

                    case CT_SENSOR :
                        newcadconnected = value12&2048 ? true : false;
                        newhrconnected  = value12&1024 ? true : false;

                        if (newhrconnected != curhrconnected || newcadconnected != curcadconnected) {
                            pvars.lock();
                            this->deviceHRConnected=curhrconnected=newhrconnected;
                            this->deviceCADConnected=curcadconnected=newcadconnected;
                            pvars.unlock();
                        }
                        break;

                    default :
                        break;
                }

            //----------------------------------------------------------------
            // UPDATE BUTTONS
            //----------------------------------------------------------------
            if (buttons != curButtons) {
                // let the gui workout what the deal is with silly button values!
                pvars.lock();
			  this->deviceButtons |= buttons; // Borrowed from Fortius.cpp: workaround to ensure controller doesn't miss button pushes
                pvars.unlock();
            }

            //----------------------------------------------------------------
            // UPDATE SSCAN
            //----------------------------------------------------------------
            /* not yet implemented */

            } else {
                // no data
                // how long to sleep for ... mmm save CPU cycles vs
                //                           data overflow ?
                CTsleeper::msleep (100); // lets try a tenth of a second
            }

        }

        //----------------------------------------------------------------
        // LISTEN TO GUI CONTROL COMMANDS
        //----------------------------------------------------------------
        pvars.lock();
        curstatus = this->deviceStatus;
        newmode = this->mode;
        newload = this->load;
        curgradient = newgradient = this->gradient;
        pvars.unlock();

        /* time to shut up shop */
        if (!(curstatus&CT_RUNNING)) {
            // time to stop!
            closePort(); // need to release that file handle!!
            quit(0);
            return;
        }

        if ((curstatus&CT_PAUSED) && isDeviceOpen == true) {
            closePort();
            isDeviceOpen = false;

        } else if (!(curstatus&CT_PAUSED) && (curstatus&CT_RUNNING) && isDeviceOpen == false) {

            if (openPort()) {
                quit(2);
                return; // open failed!
            }
            isDeviceOpen = true;

            // send first command to get computrainer ready
            prepareCommand(curmode, curmode == CT_ERGOMODE ? curload : curgradient);
            if (sendCommand(curmode) == -1) {
                // send failed - ouch!
                closePort(); // need to release that file handle!!
                quit(4);
                return; // couldn't write to the device
            }
        }

        //----------------------------------------------------------------
        // KEEP THE COMPUTRAINER CONTROL ALIVE
        //----------------------------------------------------------------
        if (isDeviceOpen == true && !(cmds%10)) {
            cmds=1;
            curmode = newmode;
            curload = newload;
            curgradient = newgradient;

            prepareCommand(curmode, curmode == CT_ERGOMODE ? curload : curgradient);
            if (sendCommand(curmode) == -1) {
                    // send failed - ouch!
                    closePort(); // need to release that file handle!!
                    quit(4);
                    cmds=20;
                    return; // couldn't write to the device
            }
        } else {
            cmds++;
        }
    }
}

void CTsleeper::msleep(unsigned long msecs)
{
    QThread::msleep(msecs);
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
int Computrainer::sendCommand(int mode)      // writes a command to the device
{
    switch (mode) {

        case CT_ERGOMODE :
            return rawWrite(ERGO_Command, 56);
            break;

        case CT_SSMODE :
            return rawWrite(SS_Command, 56);
            break;

        case CT_CALIBRATE :
            return rawWrite(const_cast<uint8_t*>(rrc_command), 56);
            break;


        default :
            return -1;
            break;
    }
}

int Computrainer::readMessage()
{
    int rc;

    if ((rc = rawRead(buf, 7)) > 0 && (buf[6]&128) == 0) {

        // we got something but need to sync
        while ((buf[6]&128) == 0 && rc > 0) {
            rc = rawRead(&buf[6], 1);
        }

        // at this point we are synced, we may have a dodgy
        // record but that is fair enough if we were out of
        // sync anyway - the alternative is to keep going
        // until we get a good message and that will
        // lead to bigger issues (plus we may have a hw
        // problem anyway).
        //
        // From experience, the need to sync is quite rare
        // on a normally configured and working system

    }
    return rc;
}

int Computrainer::closePort()
{
#ifdef WIN32
    return (int)!CloseHandle(devicePort);
#else
    tcflush(devicePort, TCIOFLUSH); // clear out the garbage
    return close(devicePort);
#endif
}

int Computrainer::openPort()
{
#ifndef WIN32

    // LINUX AND MAC USES TERMIO / IOCTL / STDIO

#if defined(Q_OS_MACX)
    int ldisc=TTYDISC;
#else
    int ldisc=N_TTY; // LINUX
#endif

    if ((devicePort=open(deviceFilename.toLatin1(),O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) return errno;

    tcflush(devicePort, TCIOFLUSH); // clear out the garbage

    if (ioctl(devicePort, TIOCSETD, &ldisc) == -1) return errno;

    // get current settings for the port
    tcgetattr(devicePort, &deviceSettings);

    // set raw mode i.e. ignbrk, brkint, parmrk, istrip, inlcr, igncr, icrnl, ixon
    //                   noopost, cs8, noecho, noechonl, noicanon, noisig, noiexn
    cfmakeraw(&deviceSettings);
    cfsetspeed(&deviceSettings, B2400);

    // further attributes
    deviceSettings.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ICANON | ISTRIP | IXON | IXOFF | IXANY);
    deviceSettings.c_iflag |= IGNPAR;
    deviceSettings.c_cflag &= (~CSIZE & ~CSTOPB);
    deviceSettings.c_oflag=0;

#if defined(Q_OS_MACX)
    deviceSettings.c_cflag &= (~CCTS_OFLOW & ~CRTS_IFLOW); // no hardware flow control
    deviceSettings.c_cflag |= (CS8 | CLOCAL | CREAD | HUPCL);
#else
    deviceSettings.c_cflag &= (~CRTSCTS); // no hardware flow control
    deviceSettings.c_cflag |= (CS8 | CLOCAL | CREAD | HUPCL);
#endif
    deviceSettings.c_lflag=0;
    deviceSettings.c_cc[VSTART] = 0x11;    
    deviceSettings.c_cc[VSTOP]  = 0x13;  
    deviceSettings.c_cc[VEOF]   = 0x20; 
    deviceSettings.c_cc[VMIN]=0;
    deviceSettings.c_cc[VTIME]=0;

    // set those attributes
    if(tcsetattr(devicePort, TCSANOW, &deviceSettings) == -1) return errno;
    tcgetattr(devicePort, &deviceSettings);

    tcflush(devicePort, TCIOFLUSH); // clear out the garbage
#else
    // WINDOWS USES SET/GETCOMMSTATE AND READ/WRITEFILE

    COMMTIMEOUTS timeouts; // timeout settings on serial ports

    // if deviceFilename references a port above COM9
    // then we need to open "\\.\COMX" not "COMX"
    QString portSpec;
    int portnum = deviceFilename.midRef(3).toString().toInt();
    if (portnum < 10)
       portSpec = deviceFilename;
    else
       portSpec = "\\\\.\\" + deviceFilename;
    wchar_t deviceFilenameW[32]; // \\.\COM32 needs 9 characters, 32 should be enough?
    MultiByteToWideChar(CP_ACP, 0, portSpec.toLatin1(), -1, (LPWSTR)deviceFilenameW,
                    sizeof(deviceFilenameW));

    // win32 commport API
    devicePort = CreateFile (deviceFilenameW, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (devicePort == INVALID_HANDLE_VALUE) return -1;

    if (GetCommState (devicePort, &deviceSettings) == false) return -1;

    // so we've opened the comm port lets set it up for
    deviceSettings.BaudRate = CBR_2400;
    deviceSettings.fParity = NOPARITY;
    deviceSettings.ByteSize = 8;
    deviceSettings.StopBits = ONESTOPBIT;
    deviceSettings.XonChar = 11;
    deviceSettings.XoffChar = 13;
    deviceSettings.EofChar = 0x0;
    deviceSettings.ErrorChar = 0x0;
    deviceSettings.EvtChar = 0x0;
    deviceSettings.fBinary = true;
    deviceSettings.fOutX = 0;
    deviceSettings.fInX = 0;
    deviceSettings.XonLim = 0;
    deviceSettings.XoffLim = 0;
    deviceSettings.fRtsControl = RTS_CONTROL_ENABLE;
    deviceSettings.fDtrControl = DTR_CONTROL_ENABLE;
    deviceSettings.fOutxCtsFlow = FALSE; //TRUE;

    if (SetCommState(devicePort, &deviceSettings) == false) {
        CloseHandle(devicePort);
        return -1;
    }

    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 1000;
    timeouts.ReadTotalTimeoutMultiplier = 50;
    timeouts.WriteTotalTimeoutConstant = 2000;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(devicePort, &timeouts);

#endif

    // success
    return 0;
}

int Computrainer::rawWrite(uint8_t *bytes, int size) // unix!!
{
    int rc=0;

#ifdef WIN32
    DWORD cBytes;
    rc = WriteFile(devicePort, bytes, size, &cBytes, NULL);
    if (!rc) return -1;
    return rc;

#else
    int ibytes;
    ioctl(devicePort, FIONREAD, &ibytes);

    // timeouts are less critical for writing, since vols are low
    rc= write(devicePort, bytes, size);

    // but it is good to avoid buffer overflow since the
    // computrainer microcontroller has almost no RAM
    if (rc != -1) tcdrain(devicePort); // wait till its gone.

    ioctl(devicePort, FIONREAD, &ibytes);
#endif

    return rc;

}

int Computrainer::rawRead(uint8_t bytes[], int size)
{
    int rc=0;

#ifdef WIN32
Q_UNUSED(size);
    // Readfile deals with timeouts and readyread issues
    DWORD cBytes;
    rc = ReadFile(devicePort, bytes, 7, &cBytes, NULL);
    if (rc) return (int)cBytes;
    else return (-1);

#else

    int timeout=0, i=0;
    uint8_t byte;

    // read one byte at a time sleeping when no data ready
    // until we timeout waiting then return error
    for (i=0; i<size; i++) {
        timeout =0;
        rc=0;
        while (rc==0 && timeout < CT_READTIMEOUT) {
            rc = read(devicePort, &byte, 1);
            if (rc == -1) return -1; // error!
            else if (rc == 0) {
                msleep(50); // sleep for 1/20th of a second
                timeout += 50;
            } else {
                bytes[i] = byte;
            }
        }
        if (timeout >= CT_READTIMEOUT) return -1; // we timed out!
    }

    return i;

#endif
}


// check to see of there is a port at the device specified
// returns true if the device exists and false if not
bool Computrainer::discover(QString filename)
{
    uint8_t *greeting = (uint8_t *)"RacerMate";
    uint8_t handshake[7];
    int rc;

    if (filename.isEmpty()) return false; // no null filenames thanks

    // lets set the port
    setDevice(filename);

    // lets open it
    openPort();

    // send a probe
    if ((rc=rawWrite(greeting, 9)) == -1) {
        closePort();
        return false;
    }

    // did we get something back from the device?
    if ((rc=rawRead(handshake, 6)) < 6) {
        closePort();
        return false;
    }

    closePort();

    handshake[6] = '\0';
    if (strcmp((char *)handshake, "LinkUp")) return false;
    else return true;
}
