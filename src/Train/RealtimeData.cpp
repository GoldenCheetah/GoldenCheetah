
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
    hr= watts= altWatts= speed= wheelRpm= load= slope= torque= 0.0;
    cadence = distance = altDistance = virtualSpeed = wbal = 0.0;
    lap = msecs = lapMsecs = lapMsecsRemaining = ergMsecsRemaining = 0;
    thb = smo2 = o2hb = hhb = 0.0;
    lrbalance = rte = lte = lps = rps = 0.0;
    latitude = longitude = altitude = 0.0;
    trainerStatusAvailable = false;
    trainerReady = true;
    trainerRunning = true;
    trainerCalibRequired = false;
    trainerConfigRequired = false;
    trainerBrakeFault = false;
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
    // negs not allowed (usually from virtual power)
    if (watts < 0) watts = 0;

    this->watts = (int)watts;
}

void RealtimeData::setAltDistance(double x)
{
    this->altDistance = x;
}

void RealtimeData::setHr(double hr)
{
    this->hr = (int)hr;
}
void RealtimeData::setSpeed(double speed)
{
    this->speed = speed;
}
void RealtimeData::setWbal(double wbal)
{
    this->wbal = wbal;
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

void RealtimeData::setErgMsecsRemaining(long x)
{
    this->ergMsecsRemaining = x;
}

void RealtimeData::setDistance(double x)
{
    this->distance = x;
}

void RealtimeData::setLapDistance(double x)
{
    this->lapDistance = x;
}

void RealtimeData::setLapDistanceRemaining(double x)
{
    this->lapDistanceRemaining = x;
}

void RealtimeData::setLRBalance(double x)
{
    this->lrbalance = x;
}

void RealtimeData::setLTE(double x)
{
    this->lte = x;
}

void RealtimeData::setRTE(double x)
{
    this->rte = x;
}

void RealtimeData::setLPS(double x)
{
    this->lps = x;
}

void RealtimeData::setRPS(double x)
{
    this->rps = x;
}

const char *
RealtimeData::getName() const
{
    return name;
}

double RealtimeData::getAltDistance() const
{
    return altDistance;
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
double RealtimeData::getWbal() const
{
    return wbal;
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
double RealtimeData::getLapDistance() const
{
    return lapDistance;
}
double RealtimeData::getLapDistanceRemaining() const
{
    return lapDistanceRemaining;
}
double RealtimeData::getLRBalance() const
{
    return lrbalance;
}
double RealtimeData::getLTE() const
{
    return lte;
}
double RealtimeData::getRTE() const
{
    return rte;
}
double RealtimeData::getLPS() const
{
    return lps;
}
double RealtimeData::getRPS() const
{
    return rps;
}

double RealtimeData::getTorque() const
{
    return torque;
}
void RealtimeData::setTrainerStatusAvailable(bool status)
{
    this->trainerStatusAvailable = status;
}

bool RealtimeData::getTrainerStatusAvailable() const
{
    return trainerStatusAvailable;
}

void RealtimeData::setTrainerReady(bool status)
{
    this->trainerReady = status;
}

void RealtimeData::setTrainerRunning(bool status)
{
    this->trainerRunning = status;
}

void RealtimeData::setTrainerCalibRequired(bool status)
{
    this->trainerCalibRequired = status;
}

void RealtimeData::setTrainerConfigRequired(bool status)
{
    this->trainerConfigRequired = status;
}

void RealtimeData::setTrainerBrakeFault(bool status)
{
    this->trainerBrakeFault = status;
}

bool RealtimeData::getTrainerReady() const
{
    return trainerReady;
}

bool RealtimeData::getTrainerRunning() const
{
    return trainerRunning;
}

bool RealtimeData::getTrainerCalibRequired() const
{
    return trainerCalibRequired;
}

bool RealtimeData::getTrainerConfigRequired() const
{
    return trainerConfigRequired;
}

bool RealtimeData::getTrainerBrakeFault() const
{
    return trainerBrakeFault;
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

    case ErgTimeRemaining: return ergMsecsRemaining;
        break;

    case Distance: return distance;
        break;

    case LapDistance: return lapDistance;
        break;

    case LapDistanceRemaining: return lapDistanceRemaining;
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

    case tHb: return thb;
        break;

    case SmO2: return smo2;
        break;

    case HHb: return hhb;
        break;

    case O2Hb: return o2hb;
        break;

    case LRBalance: return lrbalance;
        break;

    case LeftTorqueEffectiveness: return lte;
        break;

    case RightTorqueEffectiveness: return rte;
        break;

    case LeftPedalSmoothness: return lps;
        break;

    case RightPedalSmoothness: return rps;
        break;

    case Slope: return slope;
        break;

    case Latitude: return latitude;
        break;

    case Longitude: return longitude;
        break;

    case Altitude: return altitude;
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
        seriesList << BikeStress;
        seriesList << XPower;
        seriesList << IsoPower;
        seriesList << RI;
        seriesList << IF;
        seriesList << VI;
        seriesList << Joules;
        seriesList << Wbal;
        seriesList << SmO2;
        seriesList << tHb;
        seriesList << HHb;
        seriesList << O2Hb;
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
        seriesList << LeftTorqueEffectiveness;
        seriesList << RightTorqueEffectiveness;
        seriesList << LeftPedalSmoothness;
        seriesList << RightPedalSmoothness;
        seriesList << Slope;
        seriesList << LapDistance;
        seriesList << LapDistanceRemaining;
        seriesList << ErgTimeRemaining;
        seriesList << Latitude;
        seriesList << Longitude;
        seriesList << Altitude;
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

    case ErgTimeRemaining: return tr("Section Time Remaining");
        break;

    case BikeStress: return tr("BikeStress");
        break;

    case BikeScore: return "BikeScore (TM)";
        break;

    case Joules: return tr("kJoules");
        break;

    case Wbal: return tr("W' bal");
        break;

    case XPower: return tr("XPower");
        break;

    case IsoPower: return tr("Iso Power");
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

    case tHb: return tr("Total Hb Mass");
        break;

    case SmO2: return tr("Hb O2 Saturation");
        break;

    case HHb: return tr("Deoxy Hb");
        break;

    case O2Hb: return tr("Oxy Hb");
        break;

    case LeftTorqueEffectiveness: return tr("Left Torque Effectiveness");
        break;

    case RightTorqueEffectiveness: return tr("Right Torque Effectiveness");
        break;

    case LeftPedalSmoothness: return tr("Left Pedal Smoothness");
        break;

    case RightPedalSmoothness: return tr("Right Pedal Smoothness");
        break;

    case Slope: return tr("Slope");
        break;

    case LapDistance: return tr("Lap Distance");
        break;

    case LapDistanceRemaining: return tr("Lap Distance Remaining");
        break;

    case Latitude: return tr("Latitude");
        break;

    case Longitude: return tr("Longitude");
        break;

    case Altitude: return tr("Altitude");
        break;
    }
}

void RealtimeData::setHb(double smo2, double thb)
{
    this->smo2 = smo2;
    this->thb = thb;
    if (smo2 > 0 && thb > 0) {
        o2hb = (thb * smo2) / 100.00f;
        hhb = thb - o2hb;
    } else {
        o2hb = hhb = 0;
    }
}
double RealtimeData::getSmO2() const { return smo2; }
double RealtimeData::gettHb() const { return thb; }
double RealtimeData::getHHb() const { return hhb; }
double RealtimeData::getO2Hb() const { return o2hb; }

void RealtimeData::setTorque(double torque)
{
    this->torque = torque;
}

void RealtimeData::setLap(long lap)
{
    this->lap = lap;
}

long RealtimeData::getLap() const
{
    return lap;
}

double RealtimeData::getLatitude() const { return latitude; }
double RealtimeData::getLongitude() const { return longitude; }
double RealtimeData::getAltitude() const { return altitude; }

void RealtimeData::setLatitude(double d) { latitude = d; }
void RealtimeData::setLongitude(double d) { longitude = d; }
void RealtimeData::setAltitude(double d) { altitude = d; }
