
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
#include "RideFile.h"
#include "Settings.h"
#include "Context.h"
#include "Athlete.h"

#include <QtDebug>

#ifdef Q_CC_MSVC
// 'strcpy': This function or variable may be unsafe.
#pragma warning(disable:4996)
#endif

RealtimeData::RealtimeData()
{
    name[0] = '\0';
    hr= watts= altWatts= speed= wheelRpm= load= slope= torque= 0.0;
    cadence = distance = altDistance = virtualSpeed = wbal = 0.0;
    lap = msecs = lapMsecs = lapMsecsRemaining = ergMsecsRemaining = 0;
    thb = smo2 = o2hb = hhb = 0.0;
    lrbalance = RideFile::NA;
    position = RealtimeData::seated;
    rte = lte = lps = rps = 0.0;
    rppb = rppe = rpppb = rpppe = 0.0;
    lppb = lppe = lpppb = lpppe = 0.0;
    coreTemp = skinTemp = 0.0;
    heatStrain = heatLoad = 0.0;
    latitude = longitude = altitude = 0.0;
    rf = rmv = vo2 = vco2 = tv = feo2 = 0.0;
    routeDistance = distanceRemaining = VAMValue = 0.0;
    trainerStatusAvailable = false;
    trainerReady = true;
    trainerRunning = true;
    trainerCalibRequired = false;
    trainerConfigRequired = false;
    trainerBrakeFault = false;
    memset(spinScan, 0, 24);
    temp = 0.0;
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
void RealtimeData::setJoules(double joules)
{
    this->joules = joules;
}
void RealtimeData::setWbal(double wbal)
{
    this->wbal = wbal;
}
void RealtimeData::setBikeScore(double bikeScore)
{
    this->bikeScore = bikeScore;
}
void RealtimeData::setBikeStress(double bikeStress)
{
    this->bikeStress = bikeStress;
}
void RealtimeData::setXPower(double xPower)
{
    this->xPower = xPower;
}
void RealtimeData::setIsoPower(double isoPower)
{
    this->isoPower = isoPower;
}
void RealtimeData::setRI(double rI)
{
    this->rI = rI;
}
void RealtimeData::setIF(double iF)
{
    this->iF = iF;
}
void RealtimeData::setSkibaVI(double skibaVI)
{
    this->skibaVI = skibaVI;
}
void RealtimeData::setVI(double vI)
{
    this->vI = vI;
}
void RealtimeData::setVirtualSpeed(double speed)
{
    this->virtualSpeed = speed;
}
void RealtimeData::setWheelRpm(double wheelRpm, bool fMarkWheelRpmTime)
{
    this->wheelRpm = wheelRpm;

    if (fMarkWheelRpmTime)
        this->wheelRpmSampleTime = std::chrono::high_resolution_clock::now();
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

void RealtimeData::setRouteDistance(double x)
{
    this->routeDistance = x;
}

void RealtimeData::setDistanceRemaining(double x)
{
    this->distanceRemaining = x;
}

void RealtimeData::setVAM(double x)
{
    this->VAMValue = x;
}

void RealtimeData::setLapDistance(double x)
{
    this->lapDistance = x;
}

void RealtimeData::setLapDistanceRemaining(double x)
{
    this->lapDistanceRemaining = x;
}

void RealtimeData::setAvgWatts(double x)
{
    this->avgWatts = x;
}

void RealtimeData::setAvgSpeed(double x)
{
    this->avgSpeed = x;
}

void RealtimeData::setAvgCadence(double x)
{
    this->avgCadence = x;
}

void RealtimeData::setAvgHeartRate(double x)
{
    this->avgHeartRate = x;
}

void RealtimeData::setAvgWattsLap(double x)
{
    this->avgWattsLap = x;
}

void RealtimeData::setAvgSpeedLap(double x)
{
    this->avgSpeedLap = x;
}

void RealtimeData::setAvgCadenceLap(double x)
{
    this->avgCadenceLap = x;
}

void RealtimeData::setAvgHeartRateLap(double x)
{
    this->avgHeartRateLap = x;
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

//Skin temp passed but not used elsewhere
void RealtimeData::setCoreTemp(double core, double skin, double heatStrain) {
    this->coreTemp = core;
    this->skinTemp = skin;
    this->heatStrain = heatStrain;
}
void RealtimeData::setHeatLoad(double heatLoad) {
    this->heatLoad = heatLoad;
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
double RealtimeData::getJoules() const
{
    return joules;
}
double RealtimeData::getWbal() const
{
    return wbal;
}
double RealtimeData::getBikeScore() const
{
    return bikeScore;
}
double RealtimeData::getBikeStress() const
{
    return bikeStress;
}
double RealtimeData::getXPower() const
{
    return xPower;
}
double RealtimeData::getIsoPower() const
{
    return isoPower;
}
double RealtimeData::getRI() const
{
    return rI;
}
double RealtimeData::getIF() const
{
    return iF;
}
double RealtimeData::getSkibaVI() const
{
    return skibaVI;
}
double RealtimeData::getVI() const
{
    return vI;
}
double RealtimeData::getVirtualSpeed() const
{
    return virtualSpeed;
}
double RealtimeData::getWheelRpm() const
{
    return wheelRpm;
}
std::chrono::high_resolution_clock::time_point RealtimeData::getWheelRpmSampleTime() const
{
    return this->wheelRpmSampleTime;
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
double RealtimeData::getRouteDistance() const
{
    return routeDistance;
}
double RealtimeData::getDistanceRemaining() const
{
    return distanceRemaining;
}
double RealtimeData::getVAM() const
{
    return VAMValue;
}
double RealtimeData::getLapDistance() const
{
    return lapDistance;
}
double RealtimeData::getLapDistanceRemaining() const
{
    return lapDistanceRemaining;
}
double RealtimeData::getAvgWatts() const
{
    return avgWatts;
}
double RealtimeData::getAvgSpeed() const
{
    return avgSpeed;
}
double RealtimeData::getAvgCadence() const
{
    return avgCadence;
}
double RealtimeData::getAvgHeartRate() const
{
    return avgHeartRate;
}
double RealtimeData::getAvgWattsLap() const
{
    return avgWattsLap;
}
double RealtimeData::getAvgSpeedLap() const
{
    return avgSpeedLap;
}
double RealtimeData::getAvgCadenceLap() const
{
    return avgCadenceLap;
}
double RealtimeData::getAvgHeartRateLap() const
{
    return avgHeartRateLap;
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

double RealtimeData::getRppb() const
{
    return rppb;
}

double RealtimeData::getRppe() const
{
    return rppe;
}

double RealtimeData::getRpppb() const
{
    return rpppb;
}

double RealtimeData::getRpppe() const
{
    return rpppe;
}

double RealtimeData::getLppb() const
{
    return lppb;
}

double RealtimeData::getLppe() const
{
    return lppe;
}

double RealtimeData::getLpppb() const
{
    return lpppb;
}

double RealtimeData::getLpppe() const
{
    return lpppe;
}

double RealtimeData::getRightPCO() const
{
    return rightPCO;
}

double RealtimeData::getLeftPCO() const
{
    return leftPCO;
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

    case RouteDistance: return routeDistance;
        break;

    case DistanceRemaining: return distanceRemaining;
        break;

    case VAM: return VAMValue;
        break;

    case LapDistance: return lapDistance;
        break;

    case LapDistanceRemaining: return lapDistanceRemaining;
        break;

    case AltWatts: return altWatts;
        break;

    case Watts: return watts;
        break;

    case Joules: return joules;
        break;

    case Wbal: return wbal;
        break;

    case BikeScore: return bikeScore;
        break;

    case BikeStress: return bikeStress;
        break;

    case IsoPower: return isoPower;
        break;

    case XPower: return xPower;
        break;

    case RI: return rI;
        break;

    case IF: return iF;
        break;

    case SkibaVI: return skibaVI;
        break;

    case VI: return vI;
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

    case AvgWatts: return avgWatts;
        break;

    case AvgSpeed: return avgSpeed;
        break;

    case AvgCadence: return avgCadence;
        break;

    case AvgHeartRate: return avgHeartRate;
        break;

    case AvgWattsLap: return avgWattsLap;
        break;

    case AvgSpeedLap: return avgSpeedLap;
        break;

    case AvgCadenceLap: return avgCadenceLap;
        break;

    case AvgHeartRateLap: return avgHeartRateLap;
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

   case RightPowerPhaseBegin: return rppb;
        break;

    case RightPowerPhaseEnd: return rppe;
        break;

    case RightPowerPhasePeakBegin: return rpppb;
        break;

    case RightPowerPhasePeakEnd: return rpppe;
        break;

    case RightPCO: return rightPCO;
        break;

    case LeftPCO: return leftPCO;
        break;

    case Slope: return slope;
        break;

    case Latitude: return latitude;
        break;

    case Longitude: return longitude;
        break;

    case Altitude: return altitude;
        break;

    case Rf: return rf;
        break;

    case RMV: return rmv;
        break;

    case VO2: return vo2;
        break;

    case VCO2: return vco2;
        break;

    case RER: return rer;
        break;

    case TidalVolume: return tv;
        break;

    case FeO2: return feo2;
        break;

    case Temp: return temp;
        break;

    case CoreTemp: return coreTemp;
        break;
    case SkinTemp: return skinTemp;
        break;
    case HeatStrain: return heatStrain;
        break;
    case HeatLoad: return heatLoad;
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
        seriesList << Rf;
        seriesList << RMV;
        seriesList << VO2;
        seriesList << VCO2;
        seriesList << RER;
        seriesList << TidalVolume;
        seriesList << FeO2;
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
        seriesList << RouteDistance;
        seriesList << DistanceRemaining;
        seriesList << Temp;
        seriesList << CoreTemp;
        seriesList << SkinTemp;
        seriesList << HeatStrain;
        seriesList << HeatLoad;
        seriesList << VAM;
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

    case RouteDistance: return tr("Route Distance");
        break;

    case DistanceRemaining: return tr("Distance Remaining");
        break;

    case VAM: return tr("VAM");
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

    case RightPowerPhaseBegin: return tr("Right Power Phase Start");
        break;

    case RightPowerPhaseEnd: return tr("Right Power Phase End");
        break;

    case RightPowerPhasePeakBegin: return tr("Right Power Phase Peak Start");
        break;

    case RightPowerPhasePeakEnd: return tr("Right Power Phase Peak End");
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

    case Rf: return tr("Respiratory Frequency");
        break;

    case RMV: return tr("Ventilation");
        break;

    case VO2: return tr("VO2");
        break;

    case VCO2: return tr("VCO2");
        break;

    case RER: return tr("Respiratory Exchange Ratio");
        break;

    case TidalVolume: return tr("Tidal Volume");
        break;

    case FeO2: return tr("Fraction O2 Expired");
        break;

    case Temp: return tr("Temperature");
        break;

    case CoreTemp: return tr("Core Temp");
        break;
    case SkinTemp: return tr("Skin Temp");
        break;
    case HeatStrain: return tr("Heat Strain");
        break;
    case HeatLoad: return tr("Estimated Heat Load");
        break;

    case RightPCO: return tr("Right PCO");
        break;
    case LeftPCO: return tr("Left PCO");
        break;
    }
}

QString RealtimeData::seriesSymbol(DataSeries series)
{
    switch (series) {

    default:
    case None: return QString("None");
        break;

    case Time: return QString("Time");
        break;

    case Lap: return QString("Lap");
        break;

    case LapTime: return QString("Lap Time");
        break;

    case LapTimeRemaining: return QString("Lap Time Remaining");
        break;

    case ErgTimeRemaining: return QString("Section Time Remaining");
        break;

    case Joules: return QString("kJoules");
        break;

    case Wbal: return QString("W' bal");
        break;

    case BikeStress: return QString("BikeStress");
        break;

    case BikeScore: return QString("BikeScore");
        break;

    case XPower: return QString("XPower");
        break;

    case IsoPower: return QString("Iso Power");
        break;

    case IF: return QString("Intensity Factor");
        break;

    case RI: return QString("Relative Intensity");
        break;

    case SkibaVI: return QString("Skiba Variability Index");
        break;

    case VI: return QString("Variability Index");
        break;

    case Distance: return QString("Distance");
        break;

    case RouteDistance: return QString("Route Distance");
        break;

    case DistanceRemaining: return QString("Distance Remaining");
        break;

    case VAM: return QString("VAM");
        break;

    case AltWatts: return QString("Alternate Power");
        break;

    case Watts: return QString("Power");
        break;

    case Speed: return QString("Speed");
        break;

    case VirtualSpeed: return QString("Virtual Speed");
        break;

    case Cadence: return QString("Cadence");
        break;

    case HeartRate: return QString("Heart Rate");
        break;

    case Load: return QString("Target Power");
        break;

    case AvgWatts: return QString("Average Power");
        break;

    case AvgSpeed: return QString("Average Speed");
        break;

    case AvgHeartRate: return QString("Average Heartrate");
        break;

    case AvgCadence: return QString("Average Cadence");
        break;

    case AvgWattsLap: return QString("Lap Power");
        break;

    case AvgSpeedLap: return QString("Lap Speed");
        break;

    case AvgHeartRateLap: return QString("Lap Heartrate");
        break;

    case AvgCadenceLap: return QString("Lap Cadence");
        break;

    case LRBalance: return QString("Left/Right Balance");
        break;

    case tHb: return QString("Total Hb Mass");
        break;

    case SmO2: return QString("Hb O2 Saturation");
        break;

    case HHb: return QString("Deoxy Hb");
        break;

    case O2Hb: return QString("Oxy Hb");
        break;

    case LeftTorqueEffectiveness: return QString("Left Torque Effectiveness");
        break;

    case RightTorqueEffectiveness: return QString("Right Torque Effectiveness");
        break;

    case LeftPedalSmoothness: return QString("Left Pedal Smoothness");
        break;

    case RightPedalSmoothness: return QString("Right Pedal Smoothness");
        break;

    case RightPowerPhaseBegin: return QString("Right Power Phase Start");
        break;

    case RightPowerPhaseEnd: return QString("Right Power Phase End");
        break;

    case RightPowerPhasePeakBegin: return QString("Right Power Phase Peak Start");
        break;

    case RightPowerPhasePeakEnd: return QString("Right Power Phase Peak End");
        break;

    case Slope: return QString("Slope");
        break;

    case LapDistance: return QString("Lap Distance");
        break;

    case LapDistanceRemaining: return QString("Lap Distance Remaining");
        break;

    case Latitude: return QString("Latitude");
        break;

    case Longitude: return QString("Longitude");
        break;

    case Altitude: return QString("Altitude");
        break;

    case Rf: return QString("Respiratory Frequency");
        break;

    case RMV: return QString("Ventilation");
        break;

    case VO2: return QString("VO2");
        break;

    case VCO2: return QString("VCO2");
        break;

    case RER: return QString("Respiratory Exchange Ratio");
        break;

    case TidalVolume: return QString("Tidal Volume");
        break;

    case FeO2: return QString("Fraction O2 Expired");
        break;

    case Temp: return QString("Temperature");
        break;

    case CoreTemp: return QString("Core Temp");
        break;
    case SkinTemp: return QString("Skin Temp");
        break;
    case HeatStrain: return QString("Heat Strain");
        break;
    case HeatLoad: return QString("Estimated Heat Load");
        break;

    case RightPCO: return QString("Right PCO");
        break;
    case LeftPCO: return QString("Left PCO");
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

void RealtimeData::setRTorque(double torque)
{
    this->RTorque = torque;
}

void RealtimeData::setLTorque(double torque)
{
    this->LTorque = torque;
}

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
RealtimeData::riderPosition  RealtimeData::getPosition() const { return position; }

void RealtimeData::setLatitude(double d) { latitude = d; }
void RealtimeData::setLongitude(double d) { longitude = d; }
void RealtimeData::setAltitude(double d) { altitude = d; }

void RealtimeData::setRppb(double rppb) { this->rppb = rppb; }
void RealtimeData::setRppe(double rppe) { this->rppe = rppe; }
void RealtimeData::setRpppb(double rpppb) { this->rpppb = rpppb; }
void RealtimeData::setRpppe(double rpppe) { this->rpppe = rpppe; }
void RealtimeData::setLppb(double lppb) { this->lppb = lppb; }
void RealtimeData::setLppe(double lppe) { this->lppe = lppe; }
void RealtimeData::setLpppb(double lpppb) { this->lpppb = lpppb; }
void RealtimeData::setLpppe(double lpppe) { this->lpppe = lpppe; }
void RealtimeData::setRightPCO(double rightPCO) { this->rightPCO = rightPCO; }
void RealtimeData::setLeftPCO(double leftPCO) { this->leftPCO = leftPCO; }
void RealtimeData::setPosition(RealtimeData::riderPosition position) { this->position = position; }

void RealtimeData::setRf(double rf) { this->rf = rf; }
void RealtimeData::setRMV(double rmv) { this->rmv = rmv; }
void RealtimeData::setTv(double tv) { this->tv = tv; }
void RealtimeData::setFeO2(double feo2) {this->feo2 = feo2; }

void RealtimeData::setVO2_VCO2(double vo2, double vco2)
{
    this->vo2 = vo2;
    this->vco2 = vco2;

    if (vo2 > 0 && vco2 > 0) {
        rer = vco2/vo2;
    }
}

double RealtimeData::getRf() const { return rf; }
double RealtimeData::getRMV() const { return rmv; }
double RealtimeData::getVO2() const { return vo2; }
double RealtimeData::getVCO2() const { return vco2; }
double RealtimeData::getRER() const { return rer; }
double RealtimeData::getTv() const { return tv; }
double RealtimeData::getFeO2() const { return feo2; }

void RealtimeData::setTemp(double temp) { this->temp = temp; }
double RealtimeData::getTemp() const { return temp; }

double RealtimeData::getCoreTemp() const { return coreTemp; }
double RealtimeData::getSkinTemp() const { return skinTemp; }
double RealtimeData::getHeatStrain() const { return heatStrain; }
double RealtimeData::getHeatLoad() const { return heatLoad; }

//
// RealtimeDataSession
//
RealtimeDataSession::RealtimeDataSession(Context* context, double CP, double WPRIME, double TAU) :
                                         context(context), CP(CP), WPRIME(WPRIME), TAU(TAU)
{
    // Initialize derived series data for the session

    // Joules
    setJoules(0.0);

    // Averages
    sumAvgWatts = sumAvgSpeed = sumAvgCadence = sumAvgHeartRate = 0.0;
    nAvgWatts = nAvgSpeed = nAvgCadence = nAvgHeartRate = 0;
    sumAvgWattsLap = sumAvgSpeedLap = sumAvgCadenceLap = sumAvgHeartRateLap = 0.0;
    nAvgWattsLap = nAvgSpeedLap = nAvgCadenceLap = nAvgHeartRateLap = 0;

    // W'bal
    wbalr = 0;
    wbal = WPRIME;

    // Coggan Metrics
    rolling.resize(150); // enough for 30 seconds at 5hz
    rolling.fill(0.00);
    sum = rollingSum = 0.0;
    count = index = 0;

    // Skiba Metrics
    rsum = ewma = xsum = 0.0;
    rcount = 0;

    // Heat Load Estimation - we want to preserve the heat load across sessions,
    // resetting if local midnight passes while not running
    setHeatLoad(appsettings->cvalue(context->athlete->cyclist, GC_REALTIMEDATA_HEATLOAD, "").toDouble());
    heatLoadMSec = appsettings->cvalue(context->athlete->cyclist, GC_REALTIMEDATA_HEATLOAD_MSEC, "").toULongLong();
    heatLoadLocalDate = QDateTime::fromString(appsettings->cvalue(context->athlete->cyclist, GC_REALTIMEDATA_HEATLOAD_LOCALDATE, "").toString(), Qt::ISODate);
    if(heatLoadLocalDate.date().dayOfYear() != QDateTime::currentDateTime().date().dayOfYear()) {
        qDebug()<<"resetting heat load at " << QDateTime::currentDateTime() << "heatLoadLocalDate" << heatLoadLocalDate;
        setHeatLoad(0.0);
        heatLoadMSec = 0;
        heatLoadLocalDate = QDateTime::currentDateTime();
    }
}

RealtimeDataSession::~RealtimeDataSession()
{
    // Heat Load Estimation - we want to preserve the heat load across sessions,
    // resetting if local midnight passes while not running
    appsettings->setCValue(context->athlete->cyclist, GC_REALTIMEDATA_HEATLOAD, getHeatLoad());
    appsettings->cvalue(context->athlete->cyclist, GC_REALTIMEDATA_HEATLOAD_MSEC, heatLoadMSec);
    appsettings->cvalue(context->athlete->cyclist, GC_REALTIMEDATA_HEATLOAD_LOCALDATE, heatLoadLocalDate.toString(Qt::ISODate));
}

void RealtimeDataSession::newLap()
{
    // Initialize derived series data for the lap
    sumAvgWattsLap = sumAvgSpeedLap = sumAvgCadenceLap = sumAvgHeartRateLap = 0.0;
    nAvgWattsLap = nAvgSpeedLap = nAvgCadenceLap = nAvgHeartRateLap = 0;
}

#define HEATLOAD_OFFSET 0.9596
#define HEATLOAD_MULT 35.92
#define HEATLOAD_EXP 1.324

void RealtimeDataSession::updateDerived()
{
    // Update derived series data for session and lap (each 200msec)

    //
    // Joules
    //
    setJoules(getJoules() + getWatts()/5000.0);

    //
    // Averages
    //
    sumAvgWatts += getWatts();
    setAvgWatts(sumAvgWatts/nAvgWatts++);
    sumAvgSpeed += getSpeed();
    setAvgSpeed(sumAvgSpeed/nAvgSpeed++);
    sumAvgCadence += getCadence();
    setAvgCadence(sumAvgCadence/nAvgCadence++);
    sumAvgHeartRate += getHr();
    setAvgHeartRate(sumAvgHeartRate/nAvgHeartRate++);
    sumAvgWattsLap += getWatts();
    setAvgWattsLap(sumAvgWattsLap/nAvgWattsLap++);
    sumAvgSpeedLap += getSpeed();
    setAvgSpeedLap(sumAvgSpeedLap/nAvgSpeedLap++);
    sumAvgCadenceLap += getCadence();
    setAvgCadenceLap(sumAvgCadenceLap/nAvgCadenceLap++);
    sumAvgHeartRateLap += getHr();
    setAvgHeartRateLap(sumAvgHeartRateLap/nAvgHeartRateLap++);

    //
    // W'bal on the fly using Dave Waterworth's reformulation
    //

    // any watts expended in last 200msec?
    double joules = double(getWatts() - CP) / 5.00f;
    if (joules < 0) joules = 0;

    // running total of replenishment
    wbalr += joules * exp((getMsecs()/1000.00f) / TAU);
    wbal = WPRIME - (wbalr * exp((-getMsecs()/1000.00f) / TAU));

    setWbal(wbal);

    //
    // VAM
    //
    setVAM(vaminator.Push(getAltitude(), getMsecs(), getRouteDistance()));

    //
    // Coggan Metrics
    //

    // Update sum of watts for last 30 seconds
    sum += value(RealtimeData::Watts);
    sum -= rolling[index];
    rolling[index] = value(RealtimeData::Watts);

    // raise average to the 4th power
    rollingSum += pow(sum/150,4); // raise rolling average to 4th power
    count ++;

    // move index on/round
    index = (index >= 149) ? 0 : index+1;

    // calculate IsoPower
    double np = pow(rollingSum / (count), 0.25);
    setIsoPower(np);

    // carry on and calculate IF
    double rif = CP ? np / CP : 0.0;
    setIF(rif);

    // now TSS
    double normWork = np * (value(RealtimeData::Time) / 1000); // msecs
    double rawTSS = normWork * rif;
    double workInAnHourAtCP = CP * 3600;
    double tss = rawTSS / workInAnHourAtCP * 100.0;
    setBikeStress(tss);

    // VI is all that is left!
    double ap = value(RealtimeData::AvgWatts);
    double vi = ap ? np / ap : 0.0;
    setVI(vi);

    //
    // Skiba Metrics
    //

    static const double exp = 2.0f / ((25.0f / 0.2f) + 1.0f);
    static const double rem = 1.0f - exp;

    rcount++;

    if (rcount < 125) {

        // get up to speed
        rsum += value(RealtimeData::Watts);
        ewma = rsum / rcount;

    } else {

        // we're up to speed
        ewma = (value(RealtimeData::Watts) * exp) + (ewma * rem);
    }

    xsum += pow(ewma, 4.0f);
    double xpower = pow(xsum / rcount, 0.25f);
    setXPower(xpower);

    // carry on and calculate RI
    double ri = CP ? xpower / CP : 0.0;
    setRI(ri);

    // now BikeScore
    double normWorkBS = xpower * (value(RealtimeData::Time) / 1000); // msecs
    double rawBS = normWorkBS * ri;
    double bikescore = rawBS / workInAnHourAtCP * 100.0;
    setBikeScore(bikescore);

    // VI is all that is left!
    double svi = ap ? xpower / ap : 0.0;
    setSkibaVI(svi);

    // Heat Load Estimate
    double heatStrain = getHeatStrain();
    QDateTime currentTime = QDateTime::currentDateTimeUtc();
    qint64 msecEpoc = currentTime.toMSecsSinceEpoch();

    if(heatLoadMSec != 0 && heatStrain > HEATLOAD_OFFSET) //don't try to calculate a negative effective heat strain
    {
        //(HSI-AOC_Off)^AOC_EXP*TIME/AOC_Mult

        qint64 deltaMSec = msecEpoc - heatLoadMSec;
        double deltamin = deltaMSec / (1000.0 * 60.0); //msec to minutes

        double newLoad = pow(heatStrain-HEATLOAD_OFFSET, HEATLOAD_EXP) * deltamin / HEATLOAD_MULT;

        if(newLoad > 0)
            setHeatLoad(getHeatLoad() + newLoad);

        //qDebug()<<"newLoad is "<< newLoad << "a" << a << "b" << b << "heatStrain" << heatStrain << "c" << c << "deltamin" << deltamin << "heatLoad" << getHeatLoad();

        if(getHeatLoad() >= 10.0)
            setHeatLoad(10.0);
    }
    heatLoadMSec = msecEpoc;
}
