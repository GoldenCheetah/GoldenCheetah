
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

#define tr(s) QObject::tr(s)

RealtimeData::RealtimeData()
{
    name[0] = '\0';
    watts = hr = speed = wheelRpm = cadence  = load = 0;
    msecs = lapMsecs = bikeScore = joules = 0;
}

void RealtimeData::setName(char *name)
{
    strcpy(this->name, name);
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
void RealtimeData::setWheelRpm(double wheelRpm)
{
    this->wheelRpm = wheelRpm;
}
void RealtimeData::setCadence(double aCadence)
{
    cadence = (int)aCadence;
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
void RealtimeData::setDistance(double x)
{
    this->distance = x;
}
void RealtimeData::setJoules(long x)
{
    this->joules = x;
}
void RealtimeData::setBikeScore(long x)
{
    this->bikeScore = x;
}
void RealtimeData::setXPower(long x)
{
    this->xPower = x;
}

char *
RealtimeData::getName()
{
    return name;
}
double RealtimeData::getWatts()
{
    return watts;
}
double RealtimeData::getHr()
{
    return hr;
}
double RealtimeData::getSpeed()
{
    return speed;
}
double RealtimeData::getWheelRpm()
{
    return wheelRpm;
}
double RealtimeData::getCadence()
{
    return cadence;
}
double RealtimeData::getLoad()
{
    return load;
}
long RealtimeData::getMsecs()
{
    return msecs;
}
long RealtimeData::getLapMsecs()
{
    return lapMsecs;
}
double RealtimeData::getDistance()
{
    return distance;
}
long RealtimeData::getJoules()
{
    return joules;
}
long RealtimeData::getBikeScore()
{
    return bikeScore;
}
long RealtimeData::getXPower()
{
    return xPower;
}

double RealtimeData::value(DataSeries series) const
{
    switch (series) {

    case Time: return msecs;
        break;

    case LapTime: return lapMsecs;
        break;

    case Distance: return distance;
        break;

    case Joules: return joules;
        break;

    case BikeScore: return bikeScore;
        break;

    case XPower: return xPower;
        break;

    case Watts: return watts;
        break;

    case Speed: return speed;
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
        seriesList << LapTime;
        seriesList << Distance;
        seriesList << Watts;
        seriesList << Speed;
        seriesList << Cadence;
        seriesList << HeartRate;
        seriesList << Load;
        seriesList << BikeScore;
        seriesList << XPower;
        seriesList << Joules;
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

    case LapTime: return tr("Lap Time");
        break;

    case BikeScore: return tr("BikeScore");
        break;

    case Joules: return tr("Joules");
        break;

    case XPower: return tr("XPower");
        break;

    case Distance: return tr("Distance");
        break;

    case Watts: return tr("Power");
        break;

    case Speed: return tr("Speed");
        break;

    case Cadence: return tr("Cadence");
        break;

    case HeartRate: return tr("Heart Rate");
        break;

    case Load: return tr("Target Power");
        break;
    }
}
