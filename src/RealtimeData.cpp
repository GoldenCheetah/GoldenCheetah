
/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "RealtimeData.h"

#include <QtDebug>

RealtimeData::RealtimeData()
{
    name[0] = '\0';
	hr, watts, altWatts, speed, wheelRpm, load, slope = 0.0;
	cadence = distance = virtualSpeed = 0.0;
	lap = msecs = lapMsecs = lapMsecsRemaining = 0;

    memset(spinScan, 0, 24);
}

void RealtimeData::setName(char *name)
{
    strcpy(this->name, name);
}
void RealtimeData::setAltWatts(double watts)
{
    this->altWatts = (int)watts;
}
void RealtimeData::setWatts(double watts)
{
    this->watts = (int)watts;
}
void RealtimeData::setHr(double hr)
{
    this->hr = (int)hr;
}
void RealtimeData::setSpeed(double speed)
{
    this->speed = speed;
}
void RealtimeData::setVirtualSpeed(double speed)
{
    this->virtualSpeed = speed;
}
void RealtimeData::setWheelRpm(double wheelRpm)
{
    this->wheelRpm = wheelRpm;
}
void RealtimeData::setCadence(double aCadence)
{
    cadence = (int)aCadence;
}
void RealtimeData::setSlope(double slope)
{
    this->slope = slope;
}
void RealtimeData::setLoad(double load)
{
    this->load = load;
}
void RealtimeData::setMsecs(long x)
{
    this->msecs = x;
}
void RealtimeData::setLapMsecs(long x)
{
    this->lapMsecs = x;
}
void RealtimeData::setLapMsecsRemaining(long x)
{
    this->lapMsecsRemaining = x;
}

void RealtimeData::setDistance(double x)
{
    this->distance = x;
}
const char *
RealtimeData::getName() const
{
    return name;
}
double RealtimeData::getAltWatts() const
{
    return altWatts;
}
double RealtimeData::getWatts() const
{
    return watts;
}
double RealtimeData::getHr() const
{
    return hr;
}
double RealtimeData::getSpeed() const
{
    return speed;
}
double RealtimeData::getVirtualSpeed() const
{
    return virtualSpeed;
}
double RealtimeData::getWheelRpm() const
{
    return wheelRpm;
}
double RealtimeData::getCadence() const
{
    return cadence;
}
double RealtimeData::getSlope() const
{
    return slope;
}
double RealtimeData::getLoad() const
{
    return load;
}
long RealtimeData::getMsecs() const
{
    return msecs;
}
long RealtimeData::getLapMsecs() const
{
    return lapMsecs;
}
double RealtimeData::getDistance() const
{
    return distance;
}

double RealtimeData::value(DataSeries series) const
{
    switch (series) {

    case Time: return msecs;
        break;

    case Lap: return lap;
        break;

    case LapTime: return lapMsecs;
        break;

    case LapTimeRemaining: return lapMsecsRemaining;
        break;

    case Distance: return distance;
        break;

    case AltWatts: return altWatts;
        break;

    case Watts: return watts;
        break;

    case Speed: return speed;
        break;

    case VirtualSpeed: return virtualSpeed;
        break;

    case Cadence: return cadence;
        break;

    case HeartRate: return hr;
        break;

    case Load: return load;
        break;

    case None: 
    default:
        return 0;
        break;

    }
}

// provide a list of data series
const QList<RealtimeData::DataSeries> &RealtimeData::listDataSeries()
{
    static QList<DataSeries> seriesList;

    if (seriesList.count() == 0) {

        // better populate it first!
        seriesList << None;
        seriesList << Time;
        seriesList << Lap;
        seriesList << LapTime;
        seriesList << Distance;
        seriesList << Watts;
        seriesList << Speed;
        seriesList << Cadence;
        seriesList << HeartRate;
        seriesList << Load;
        seriesList << BikeScore;
        seriesList << SkibaVI;
        seriesList << TSS;
        seriesList << XPower;
        seriesList << NP;
        seriesList << RI;
        seriesList << IF;
        seriesList << VI;
        seriesList << Joules;
        seriesList << AvgWatts;
        seriesList << AvgSpeed;
        seriesList << AvgCadence;
        seriesList << AvgHeartRate;
        seriesList << AvgWattsLap;
        seriesList << AvgSpeedLap;
        seriesList << AvgCadenceLap;
        seriesList << AvgHeartRateLap;
        seriesList << VirtualSpeed;
        seriesList << AltWatts;
        seriesList << LRBalance;
        seriesList << LapTimeRemaining;
    }
    return seriesList;
}

QString RealtimeData::seriesName(DataSeries series)
{
    switch (series) {

    default:
    case None: return tr("None");
        break;

    case Time: return tr("Time");
        break;

    case Lap: return tr("Lap");
        break;

    case LapTime: return tr("Lap Time");
        break;

    case LapTimeRemaining: return tr("Lap Time Remaining");
        break;

    case TSS: return tr("TSS");
        break;

    case BikeScore: return tr("BikeScore");
        break;

    case Joules: return tr("kJoules");
        break;

    case XPower: return tr("XPower");
        break;

    case NP: return tr("Normalized Power");
        break;

    case IF: return tr("Intensity Factor");
        break;

    case RI: return tr("Relative Intensity");
        break;

    case SkibaVI: return tr("Skiba Variability Index");
        break;

    case VI: return tr("Variability Index");
        break;

    case Distance: return tr("Distance");
        break;

    case AltWatts: return tr("Alternate Power");
        break;

    case Watts: return tr("Power");
        break;

    case Speed: return tr("Speed");
        break;

    case VirtualSpeed: return tr("Virtual Speed");
        break;

    case Cadence: return tr("Cadence");
        break;

    case HeartRate: return tr("Heart Rate");
        break;

    case Load: return tr("Target Power");
        break;

    case AvgWatts: return tr("Average Power");
        break;

    case AvgSpeed: return tr("Average Speed");
        break;

    case AvgHeartRate: return tr("Average Heartrate");
        break;

    case AvgCadence: return tr("Average Cadence");
        break;

    case AvgWattsLap: return tr("Lap Power");
        break;

    case AvgSpeedLap: return tr("Lap Speed");
        break;

    case AvgHeartRateLap: return tr("Lap Heartrate");
        break;

    case AvgCadenceLap: return tr("Lap Cadence");
        break;

    case LRBalance: return tr("Left/Right Balance");
        break;
    }
}

void RealtimeData::setLap(long lap)
{
    this->lap = lap;
}

long RealtimeData::getLap() const
{
    return lap;
}
