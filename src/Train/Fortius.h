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
#include <limits>

/* Device operation mode */
#define FT_IDLE        0x00
#define FT_ERGOMODE    0x01
#define FT_SSMODE      0x02
#define FT_CALIBRATE   0x04

/* Buttons */
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
#define DEFAULT_CALIBRATION_FORCE_N (rawForce_to_N(1040))
#define DEFAULT_CALIBRATION_FACTOR  0.00
#define DEFAULT_SCALING      1.00
#define DEFAULT_WINDSPEED    0.00
#define DEFAULT_Crr          0.004
#define DEFAULT_CdARho       0.51

#define FT_USB_TIMEOUT      500

class Fortius : public QThread
{
    Q_DECLARE_TR_FUNCTIONS(Fortius)

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
    void setLoad(double load);                  // set the load to generate in ERGOMODE
    void setGradient(double gradient);          // set the load to generate in SSMODE
    void setBrakeCalibrationForce(double value); // set the calibration force (N) for ERGOMODE and SSMODE
    void setBrakeCalibrationFactor(double calibrationFactor);     // Impacts relationship between brake setpoint and load
    void setPowerScaleFactor(double calibrationFactor);         // Scales output power, so user can adjust to match hub or crank power meter
    void setMode(int mode);
    void setWeight(double weight);                 // set the total weight of rider + bike in kg's
    void setWindSpeed(double windSpeed);                 // set the wind speed in m/s
    void setRollingResistance(double rollingResistance); // set the rolling resistance (Crr)
    void setWindResistance(double windResistance);       // set the wind resistance (CdARho)
    
    int getMode();
    double getGradient();
    double getLoad();
    double getBrakeCalibrationForce() const;
    double getBrakeCalibrationFactor();
    double getPowerScaleFactor();
    double getWeight();
    
    // GET TELEMETRY AND STATUS
    // direct access to class variables is not allowed because we need to use wait conditions
    // to sync data read/writes between the run() thread and the main gui thread
    void getTelemetry(double &power, double &force, double &heartrate, double &cadence, double &speed, double &distance, int &buttons, int &steering, int &status);

private:
    void run();                                 // called by start to kick off the CT comtrol thread

    uint8_t ERGO_Command[12],
            SLOPE_Command[12];
    
    // Utility and BG Thread functions
    int openDevice();
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
    volatile double devicePower;            // current output power in Watts
    volatile double deviceForce_N;          // current output force in Newtons
    volatile double deviceHeartRate;        // current heartrate in BPM
    volatile double deviceCadence;          // current cadence in RPM
    volatile double deviceSpeed;            // current speed in KPH
    volatile double deviceDistance;         // odometer in meters
    volatile int    deviceButtons;          // Button status
    volatile int    deviceStatus;           // Device status running, paused, disconnected
    volatile int    deviceSteering;            // Steering angle
    
    // OUTBOUND COMMANDS - all volatile since it is updated by the GUI thread
    volatile int mode;
    volatile double load;
    volatile double gradient;
    volatile double brakeCalibrationForce_N;
    volatile double brakeCalibrationFactor;
    volatile double powerScaleFactor;
    volatile double weight;
    volatile double windSpeed;
    volatile double rollingResistance;
    volatile double windResistance;

    // Model version
    unsigned int motorBrakeFirmwareVersion;
    unsigned int motorBrakeRawSerialNumber;
    unsigned int motorBrakeVersion;
    unsigned int motorBrakeType;
    unsigned int motorBrakeYear;
    unsigned int motorBrakeSerialNumber;
    bool motorBrake;
    
    // i/o message holder
    uint8_t buf[64];

    // device port
    LibUsb *usb2;                   // used for USB2 support

    // raw device utils
    int rawWrite(uint8_t *bytes, int size); // unix!!
    int rawRead(uint8_t *bytes, int size); // unix!!


public:
    // Unit conversion routines
    static inline double kph_to_ms      (double kph) { return kph / 3.6; }
    static inline double ms_to_kph      (double ms)  { return ms  * 3.6; }

    // Source: https://github.com/totalreverse/ttyT1941/wiki
    //         - "A force of 1 N ~= 137 'load units'"
    static inline double rawForce_to_N  (double raw) { return raw / 137.; }
    static inline double N_to_rawForce  (double N)   { return N   * 137.; }

private:
    // Source: https://github.com/totalreverse/ttyT1941/wiki
    //         - "speed = 'kph * 289.75'"
    static inline double rawSpeed_to_ms (double raw) { return raw / 1043.1; } // 289.75*3.6
    static inline double ms_to_rawSpeed (double raw) { return raw * 1043.1; } // 289.75*3.6

    // Convert double value to type T, clipping to range of type T, if necessary
    template <typename T>
    static inline T clip_double_to_type(double d)
    {
        static const double MAX = std::numeric_limits<T>::max();
        static const double MIN = std::numeric_limits<T>::min();
        return std::max(MIN, std::min(MAX, d));
    }


    // Parameterised calculation of resistive forces in steady-state
    double NewtonsForV(double speed_ms) const;

    // Routine to limit trainer resistance value at low wheel speeds
    // Source: https://github.com/WouterJD/FortiusANT/blob/master/pythoncode/usbTrainer.py
    static int16_t rawForce_FortiusANT_AvoidCycleOfDeath(int16_t rawForce, double speedKph);
};

#endif // _GC_Fortius_h
