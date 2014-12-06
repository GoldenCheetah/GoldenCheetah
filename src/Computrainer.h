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


// I have consciously avoided putting things like data logging, lap marking,
// intervals or any load management functions in this class. It is restricted
// to controlling an reading telemetry from the device
//
// I expect higher order classes to implement such functions whilst
// other devices (e.g. ANT+ devices) may be implemented with the same basic
// interface
//
// I have avoided a base abstract class at this stage since I am uncertain
// what core methods would be required by say, ANT+ or Tacx devices


#ifndef _GC_Computrainer_h
#define _GC_Computrainer_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QFile>
#include "RealtimeController.h"

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#else
#include <termios.h> // unix!!
#include <unistd.h> // unix!!
#include <sys/ioctl.h>
#ifndef N_TTY // for OpenBSD, this is a hack XXX
#define N_TTY 0
#endif
#endif

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>


/* Some CT Microcontroller / Protocol Constants */

/* read timeouts in microseconds */
#define CT_READTIMEOUT    1000
#define CT_WRITETIMEOUT   2000

// message type
#define CT_SPEED        0x01
#define CT_POWER        0x02
#define CT_HEARTRATE    0x03
#define CT_CADENCE      0x06
#define CT_RRC          0x09
#define CT_SENSOR       0x0b

// buttons
#define CT_RESET        0x01
#define CT_F1           0x02
#define CT_F3           0x04
#define CT_PLUS         0x08
#define CT_F2           0x10
#define CT_MINUS        0x20
#define CT_SSS          0x40    // spinscan sync is not a button!
#define CT_NONE         0x80

/* Device operation mode */
#define CT_ERGOMODE    0x01
#define CT_SSMODE      0x02
#define CT_CALIBRATE   0x04

/* UI operation mode */
#define UI_MANUAL 0x01  // using +/- keys to adjust
#define UI_ERG    0x02  // running an erg file!

/* Control status */
#define CT_RUNNING 0x01
#define CT_PAUSED  0x02

/* default operation mode */
#define DEFAULT_MODE        CT_ERGOMODE
#define DEFAULT_LOAD        100.00
#define DEFAULT_GRADIENT    2.00


class Computrainer : public QThread
{

public:
    Computrainer(QObject *parent=0, QString deviceFilename=0);       // pass device
    ~Computrainer();

    QObject *parent;

    // HIGH-LEVEL FUNCTIONS
    int start();                                // Calls QThread to start
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    int quit(int error);                        // called by thread before exiting
    bool discover(QString deviceFilename);        // confirm CT is attached to device

    // SET
    void setDevice(QString deviceFilename);       // setup the device filename
    void setLoad(double load);                  // set the load to generate in ERGOMODE
    void setGradient(double gradient);          // set the load to generate in SSMODE
    void setMode(int mode,
        double load=DEFAULT_LOAD,               // set mode to CT_ERGOMODE or CT_SSMODE
        double gradient=DEFAULT_GRADIENT);


    // GET TELEMETRY AND STATUS
    // direct access to class variables is not allowed because we need to use wait conditions
    // to sync data read/writes between the run() thread and the main gui thread
    bool isCalibrated();
    bool isHRConnected();
    bool isCADConnected();
    void getTelemetry(double &Power, double &HeartRate, double &Cadence, double &Speed,
                        double &RRC, bool &calibration, int &Buttons, uint8_t *ss, int &Status);
    void getSpinScan(double spinData[]);
    int getMode();
    double getGradient();
    double getLoad();

private:
    void run();                                 // called by start to kick off the CT comtrol thread

    // 56 bytes comprise of 8 7byte command messages, where
    // the last is the set load / gradient respectively
    uint8_t ERGO_Command[56],
            SS_Command[56];

    // Utility and BG Thread functions
    int openPort();
    int closePort();

    // Protocol encoding
    void prepareCommand(int mode, double value);  // sets up the command packet according to current settings
    int sendCommand(int mode);      // writes a command to the device
    int calcCRC(int value);     // calculates the checksum for the current command

    // Protocol decoding
    int readMessage();
    void unpackTelemetry(int &b1, int &b2, int &b3, int &buttons, int &type, int &value8, int &value12);

    // Mutex for controlling accessing private data
    QMutex pvars;

    // INBOUND TELEMETRY - all volatile since it is updated by the run() thread
    volatile double devicePower;            // current output power in Watts
    volatile double deviceHeartRate;        // current heartrate in BPM
    volatile double deviceCadence;          // current cadence in RPM
    volatile double deviceSpeed;            // current speed in KPH
    volatile double deviceRRC;              // calibrated Rolling Resistance
    volatile bool   deviceCalibrated;       // is it calibrated?
    volatile uint8_t spinScan[24];           // SS values only in SS_MODE
    volatile int    deviceButtons;          // Button status
    volatile bool   deviceHRConnected;      // HR jack is connected
    volatile bool   deviceCADConnected;     // Cadence jack is connected
    volatile int    deviceStatus;           // Device status running, paused, disconnected

    // OUTBOUND COMMANDS - all volatile since it is updated by the GUI thread
    volatile int mode;
    volatile double load;
    volatile double gradient;

    // i/o message holder
    uint8_t buf[7];

    // device port
    QString deviceFilename;
#ifdef WIN32
    HANDLE devicePort;              // file descriptor for reading from com3
    DCB deviceSettings;             // serial port settings baud rate et al
#else
    int devicePort;                 // unix!!
    struct termios deviceSettings;  // unix!!
#endif
    // raw device utils
    int rawWrite(uint8_t *bytes, int size); // unix!!
    int rawRead(uint8_t *bytes, int size); // unix!!
};

class CTsleeper : public QThread
{
    public:
    static void msleep(unsigned long msecs); // inherited from QThread
};

#endif // _GC_Computrainer_h

