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
                      NP, TSS, IF, VI, Wbal,
                      AvgWatts, AvgSpeed, AvgCadence, AvgHeartRate,
                      AvgWattsLap, AvgSpeedLap, AvgCadenceLap, AvgHeartRateLap,
                      VirtualSpeed, AltWatts, LRBalance, LapTimeRemaining };

    typedef enum dataseries DataSeries;
    double value(DataSeries) const;
    static QString seriesName(DataSeries);
    static const QList<DataSeries> &listDataSeries();

    RealtimeData();
    void reset(); // set all values to zero

    void setName(char *name);
    void setWatts(double watts);
    void setAltWatts(double watts);
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
    void setDistance(double);
    void setBikeScore(long);
    void setJoules(long);
    void setXPower(long);
    void setLap(long);

    const char *getName() const;
    double getWatts() const;
    double getAltWatts() const;
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

    uint8_t spinScan[24];

private:
    char name[64];

    // realtime telemetry
    double hr, watts, altWatts, speed, wheelRpm, load, slope;
    double cadence;      // in rpm

    // derived data
    double distance;
    double virtualSpeed;
    double wbal;
    long lap;
    long msecs;
    long lapMsecs;
    long lapMsecsRemaining;
};

#endif
