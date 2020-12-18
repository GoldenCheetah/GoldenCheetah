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

#include <cmath>

#include <QString>
#include <QThread>
#include <QMutex>

#include "LibUsb.h"

template <size_t N>
class NSampleSmoothing
{
private:
    int    nSamples = 0;
    double samples[N];
    int    index = 0;
    double total = 0;
    bool   full = false;

public:
    NSampleSmoothing()
    {
        reset();
    }

    void reset()
    {
        for (int i = 0; i < N; ++i)
            samples[i] = 0.;
        nSamples = 0;
        index = 0;
        total = 0;
        full = false;
    }

    void update(double newVal)
    {
        ++nSamples;

        total += newVal - samples[index];
        samples[index] = newVal;
        if (++index == N) { index = 0; full = true; }
    }

    bool is_full() const
    {
        return full;
    }

    double mean() const
    {
        // average if we have enough values, otherwise return latest
        return total / N;//(nSamples > N) ? (total / N) : samples[(index + (N-1))%N];
    }

    double stddev() const
    {
        const double avg = mean();
        const double sum_squares = std::accumulate(std::begin(samples), std::end(samples), 0.0, [avg](double acc, double sample) {return acc + (sample - avg) * (sample - avg); });
        return sqrt(sum_squares / static_cast<double>(N));
    }
};

class Fortius : public QThread
{
private:
    enum FortiusControlStatus    { FT_RUNNING = 0x01, FT_PAUSED = 0x02 };
    enum FortiusCommandModeValue { FT_MODE_IDLE = 0x00, FT_MODE_ACTIVE = 0x02, FT_MODE_CALIBRATE = 0x03 };

public:
    static inline double kph_to_ms      (double kph) { return kph / 3.6; }
    static inline double ms_to_kph      (double ms)  { return ms  * 3.6; }

    static inline double rawForce_to_N  (double raw) { return raw / 137.; }
    static inline double N_to_rawForce  (double N)   { return N   * 137.; }

    static inline double rawSpeed_to_ms (double raw) { return raw / 1043.1; } // 289.75*3.6
    static inline double ms_to_rawSpeed (double ms)  { return ms  * 1043.1; } // 289.75*3.6

    enum FortiusMode    { FT_IDLE, FT_ERGOMODE, FT_SSMODE, FT_CALIBRATE };
    enum FortiusButtons { FT_ENTER = 0x01, FT_MINUS = 0x02, FT_PLUS = 0x04, FT_CANCEL = 0x08 };

    Fortius(QObject *parent=0);                  // pass device
    ~Fortius();

    QObject *parent;

    // HIGH-LEVEL FUNCTIONS
    int start();                                 // Calls QThread to start
    int restart();                               // restart after paused
    int pause();                                 // pauses data collection, inbound telemetry is discarded
    int stop();                                  // stops data collection thread
    int quit(int error);                         // called by thread before exiting

    bool find();                                 // either unconfigured or configured device found
    bool discover(QString deviceFilename);       // confirm CT is attached to device

    // SET
    void setLoad(double load);                   // set the load to generate in ERGOMODE
    void setGradientWithSimState(double gradient,
        double targetForce_N, double speed_kph); // set the load to generate in SSMODE
    void setBrakeCalibrationForce(double value); // set the calibration force (N) for ERGOMODE and SSMODE
    void setBrakeCalibrationFactor(double);      // Impacts relationship between brake setpoint and load
    void setPowerScaleFactor(double);            // Scales output power, so user can adjust to match hub or crank power meter
    void setMode(int mode);                      // set the mode
    void setWeight(double weight_kg);            // set the total weight of rider + bike in kg's

    // GET
    int    getMode() const;
    double getGradient() const;
    double getLoad() const;
    double getBrakeCalibrationForce() const;
    double getBrakeCalibrationFactor() const;
    double getPowerScaleFactor() const;
    double getWeight() const;

    // GET TELEMETRY AND STATUS
    // direct access to class variables is not allowed because we need to use wait conditions
    // to sync data read/writes between the run() thread and the main gui thread
    struct DeviceTelemetry
    {
        double Force_N;       // current output force in Newtons
        double Power_W;       // current output power in Watts
        double HeartRate;     // current heartrate in BPM
        double Cadence;       // current cadence in RPM
        double Speed_ms;      // current speed in Meters per second (derived from wheel speed)
        double Distance;      // odometer in meters
        int    Buttons;       // Button status
        int    Steering;      // Steering angle
    };
    void getTelemetry(DeviceTelemetry&);

    // calibration data for use by controller
    uint8_t  calibrationState;
    time_t   calibrationStarted;

    NSampleSmoothing<100> calibration_values;

private:
    void run();                                 // called by start to kick off the CT comtrol thread

    // Utility and BG Thread functions
    int openPort();
    int closePort();

    // Protocol encoding
    int sendRunCommand(double deviceSpeed_ms, int16_t pedalSensor);

    int sendCommand_OPEN();
    int sendCommand_IDLE();
    int sendCommand_RESISTANCE(double force_N, uint8_t pedecho, uint8_t weight_kg);
    int sendCommand_CALIBRATE(double speed_ms);
    int sendCommand_GENERIC(uint8_t mode, double rawForce, uint8_t pedecho, uint8_t weight_kg, uint16_t calibration);


    // Protocol decoding
    int readMessage();
    //void unpackTelemetry(int &b1, int &b2, int &b3, int &buttons, int &type, int &value8, int &value12);

    // Mutex for controlling accessing private data
    mutable QMutex pvars;

    // Device status running, paused, disconnected
    int deviceStatus; // must acquire pvars for read/write

    // INBOUND TELEMETRY - read & write requires lock since written by run() thread
    DeviceTelemetry _device; // must acquire pvars for read/write

    // OUTBOUND COMMANDS read & write requires lock since written by gui() thread
    struct ControlParameters {
        int    mode;
        double targetPower_W;      // set-point power demanded for ERGO mode
        double targetForce_N;      // load demanded by simulator
        double simSpeed_ms;        // simulator's speed, a speed to match if possible
        double gradient;           // may be used in reworked/alternative slope algorithm
        double powerScaleFactor;
        double weight_kg;
        double brakeCalibrationFactor;
        double brakeCalibrationForce_N;
    } _control; // must acquire pvars for read/write


    // i/o message holder
    uint8_t buf[64];

    // device port
    LibUsb *usb2;                   // used for USB2 support
    static const int FT_USB_TIMEOUT = 500;

    // raw device utils
    int rawWrite(const uint8_t *bytes, int size); // unix!!
    int rawRead(uint8_t *bytes, int size);  // unix!!
};

#endif // _GC_Fortius_h
