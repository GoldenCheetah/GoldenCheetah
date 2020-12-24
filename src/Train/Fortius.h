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


#ifndef _GC_Fortius_h
#define _GC_Fortius_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDialog>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QFile>
#include <QtCore/qendian.h>
#include "RealtimeController.h"

#include "LibUsb.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

/* Device operation mode */
#define FT_IDLE        0x00
#define FT_ERGOMODE    0x01
#define FT_SSMODE      0x02
#define FT_CALIBRATE   0x04

/* Buttons_bitfield */
#define FT_PLUS        0x04
#define FT_MINUS       0x02
#define FT_CANCEL      0x08
#define FT_ENTER       0x01

/* Control status */
#define FT_RUNNING     0x01
#define FT_PAUSED      0x02

#define DEFAULT_LOAD         100.00
#define DEFAULT_GRADIENT     2.00
#define DEFAULT_WEIGHT       77
#define DEFAULT_CALIBRATION_ADJUST 0.00
#define DEFAULT_CALIBRATION_VALUE  1040
#define DEFAULT_SCALING      1.00

#define DEFAULT_WIND_SPEED         0.0
#define DEFAULT_ROLLING_RESISTANCE 0.004
#define DEFAULT_WIND_RESISTANCE    0.51

#define FT_USB_TIMEOUT      500

class Fortius : public QThread
{

public:
    Fortius(QObject *parent=0);                   // pass device
    ~Fortius();

    QObject *parent;

    // HIGH-LEVEL FUNCTIONS
    int start();                                // Calls QThread to start
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    int quit(int error);                        // called by thread before exiting

    bool find();                                // either unconfigured or configured device found
    bool discover(QString deviceFilename);        // confirm CT is attached to device

    // SET
    void setCalibrationValue(uint16_t value);   // set the calibration value for ERGOMODE and SSMODE
    void setLoad(double load_W);                // set the load to generate in ERGOMODE
    void setGradient(double gradient_percent);  // set the load to generate in SSMODE
    void setBrakeCalibrationFactor(double calibrationFactor);     // Impacts relationship between brake setpoint and load
    void setPowerScaleFactor(double calibrationFactor);         // Scales output power, so user can adjust to match hub or crank power meter
    void setMode(int mode);
    void setWeight(double weight_kg);           // set the total weight of rider + bike in kg's, for power calculation and virtual flywheel in SSMODE
    void setWindSpeed(double);                  // set the wind speed for power calculation in SSMODE
    void setRollingResistance(double);          // set the rolling resistance coefficient for power calculation in SSMODE
    void setWindResistance(double);             // set the wind resistance coefficient for power calculation in SSMODE
    
    int getMode() const;
    double getGradient() const;
    double getLoad() const;
    double getBrakeCalibrationFactor() const;
    double getPowerScaleFactor() const;
    double getWeight() const;
    
    // GET TELEMETRY AND STATUS
    // direct access to class variables is not allowed because we need to use wait conditions
    // to sync data read/writes between the run() thread and the main gui thread
    void getTelemetry(double &power, double& resistance, double &heartrate, double &cadence, double &speed, double &distance, int &buttons, int &steering, int &status);

private:
    void run();                                 // called by start to kick off the CT comtrol thread

    uint8_t ERGO_Command[12],
            SLOPE_Command[12],
            CALIBRATE_Command[12];

    // Utility and BG Thread functions
    int openPort();
    int closePort();

    // Protocol encoding
    int sendRunCommand(int16_t pedalSensor);
    int sendOpenCommand();
    int sendCloseCommand();
    int sendCalibrateCommand();

    // Protocol decoding
    int readMessage();
    //void unpackTelemetry(int &b1, int &b2, int &b3, int &buttons, int &type, int &value8, int &value12);

    // Mutex for controlling accessing private data
    mutable QMutex pvars;

    // INBOUND TELEMETRY - all volatile since it is updated by the run() thread
    double devicePower_W;          // current output power in Watts
    double deviceResistance_raw;   // current output force in ~1/137 N
    double deviceHeartRate_bpm;    // current heartrate in BPM
    double deviceCadence_rpm;      // current cadence in RPM
    double deviceSpeed_kph;        // current speed in KPH (derived from wheel speed)
    double deviceWheelSpeed_raw;   // current wheel speed from device
    double deviceDistance_m;       // odometer in meters
    int    deviceButtons_bitfield; // Button status
    int    deviceStatus;           // Device status running, paused, disconnected
    int    deviceSteering_deg;     // Steering angle
    
    // OUTBOUND COMMANDS - all volatile since it is updated by the GUI thread
    int    mode;
    double load_W;
    double gradient_percent;
    double brakeCalibrationFactor;
    double powerScaleFactor;
    double weight_kg;
    double windSpeed_ms;
    double rollingResistance;
    double windResistance;
    double brakeCalibrationValue;

    // i/o message holder
    uint8_t buf[64];

    // device port
    LibUsb *usb2;                   // used for USB2 support

    // raw device utils
    int rawWrite(uint8_t *bytes, int size); // unix!!
    int rawRead(uint8_t *bytes, int size); // unix!!
};

#endif // _GC_Fortius_h
