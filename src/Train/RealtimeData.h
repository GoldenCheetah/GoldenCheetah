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


#ifndef _GC_RealtimeData_h
#define _GC_RealtimeData_h 1
#include "GoldenCheetah.h"

#include <stdint.h> // uint8_t
#include <QString>
#include <QApplication>

class RealtimeData
{
    Q_DECLARE_TR_FUNCTIONS(RealtimeData)

public:

    int mode;

    // abstract to dataseries
    enum dataseries { None=0, Time, LapTime, Distance, Lap,
                      Watts, Speed, Cadence, HeartRate, Load,
                      XPower, BikeScore, RI, Joules, SkibaVI,
                      IsoPower, BikeStress, IF, VI, Wbal,
                      SmO2, tHb, HHb, O2Hb,
                      AvgWatts, AvgSpeed, AvgCadence, AvgHeartRate,
                      AvgWattsLap, AvgSpeedLap, AvgCadenceLap, AvgHeartRateLap,
                      VirtualSpeed, AltWatts, LRBalance, LapTimeRemaining,
                      LeftTorqueEffectiveness, RightTorqueEffectiveness,
                      LeftPedalSmoothness, RightPedalSmoothness, Slope, 
                      LapDistance, LapDistanceRemaining, ErgTimeRemaining,
                      Latitude, Longitude, Altitude };

    typedef enum dataseries DataSeries;

    double value(DataSeries) const;
    static QString seriesName(DataSeries);
    static const QList<DataSeries> &listDataSeries();

    RealtimeData();
    void reset(); // set all values to zero

    void setName(char *name);
    void setWatts(double watts);
    void setAltWatts(double watts);
    void setAltDistance(double distance);
    void setHr(double hr);
    void setTime(long time);
    void setSpeed(double speed);
    void setWbal(double speed);
    void setVirtualSpeed(double speed);
    void setWheelRpm(double wheelRpm);
    void setCadence(double aCadence);
    void setLoad(double load);
    void setSlope(double slope);
    void setMsecs(long);
    void setLapMsecs(long);
    void setLapMsecsRemaining(long);
    void setErgMsecsRemaining(long);
    void setDistance(double);
    void setBikeScore(long);
    void setJoules(long);
    void setXPower(long);
    void setLap(long);
    void setLapDistance(double distance);
    void setLapDistanceRemaining(double distance);
    void setLRBalance(double);
    void setLTE(double);
    void setRTE(double);
    void setLPS(double);
    void setRPS(double);
    void setTorque(double);
    void setLatitude(double);
    void setLongitude(double);
    void setAltitude(double);

    const char *getName() const;

    // new muscle oxygen stuff
    void setHb(double smo2, double thb);
    double getSmO2() const;
    double gettHb() const;
    double getHHb() const;
    double getO2Hb() const;

    double getWatts() const;
    double getAltWatts() const;
    double getAltDistance() const;
    double getHr() const;
    long getTime() const;
    double getSpeed() const;
    double getWbal() const;
    double getVirtualSpeed() const;
    double getWheelRpm() const;
    double getCadence() const;
    double getLoad() const;
    double getSlope() const;
    long getMsecs() const;
    long getLapMsecs() const;
    double getDistance() const;
    long getLap() const;
    double getLapDistance() const;
    double getLapDistanceRemaining() const;
    double getLRBalance() const;
    double getLTE() const;
    double getRTE() const;
    double getLPS() const;
    double getRPS() const;
    double getTorque() const;
    double getLatitude() const;
    double getLongitude() const;
    double getAltitude() const;

    void setTrainerStatusAvailable(bool status);
    bool getTrainerStatusAvailable() const;
    void setTrainerReady(bool status);
    void setTrainerRunning(bool status);
    void setTrainerCalibRequired(bool status);
    void setTrainerConfigRequired(bool status);
    void setTrainerBrakeFault(bool status);
    bool getTrainerReady() const;
    bool getTrainerRunning() const;
    bool getTrainerCalibRequired() const;
    bool getTrainerConfigRequired() const;
    bool getTrainerBrakeFault() const;

    uint8_t spinScan[24];

private:
    char name[64];

    // realtime telemetry
    double hr, watts, altWatts, altDistance, speed, wheelRpm, load, slope, lrbalance;
    double cadence;      // in rpm
    double smo2, thb;
    double lte, rte, lps, rps; // torque efficiency and pedal smoothness
    double torque; // raw torque data for calibration display
    double latitude, longitude, altitude;

    // derived data
    double distance;
    double lapDistance;
    double lapDistanceRemaining;
    double virtualSpeed;
    double wbal;
    double hhb, o2hb;
    long lap;
    long msecs;
    long lapMsecs;
    long lapMsecsRemaining;
    long ergMsecsRemaining;

    bool trainerStatusAvailable;
    bool trainerReady;
    bool trainerRunning;
    bool trainerCalibRequired;
    bool trainerConfigRequired;
    bool trainerBrakeFault;
};

#endif
