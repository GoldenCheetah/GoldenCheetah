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
#include "ErgFileBase.h"

#include <stdint.h> // uint8_t
#include <QString>
#include <QApplication>

class RealtimeData
{
    Q_DECLARE_TR_FUNCTIONS(RealtimeData)

public:

    ErgFileFormat mode;

    // abstract to dataseries
    // the order matters. if we remove any entry, the corresponding layouts
    // will break since the data series is defined by its enum value. new data
    // series need to be appended.
    enum dataseries { None=0, Time, LapTime, Distance, Lap,
                      Watts, Speed, Cadence, HeartRate, Load,
                      XPower, BikeScore, RI, Joules, SkibaVI,
                      IsoPower, BikeStress, IF, VI, Wbal,
                      SmO2, tHb, HHb, O2Hb,
                      Rf, RMV, VO2, VCO2, RER, TidalVolume, FeO2,
                      AvgWatts, AvgSpeed, AvgCadence, AvgHeartRate,
                      AvgWattsLap, AvgSpeedLap, AvgCadenceLap, AvgHeartRateLap,
                      VirtualSpeed, AltWatts, LRBalance, LapTimeRemaining,
                      LeftTorqueEffectiveness, RightTorqueEffectiveness,
                      LeftPedalSmoothness, RightPedalSmoothness, Slope, 
                      LapDistance, LapDistanceRemaining, ErgTimeRemaining,
                      Latitude, Longitude, Altitude, RouteDistance,
                      DistanceRemaining,
                      RightPowerPhaseBegin, RightPowerPhaseEnd,
                      RightPowerPhasePeakBegin, RightPowerPhasePeakEnd,
                      Position, RightPCO, LeftPCO,
                      Temp,
                      CoreTemp, SkinTemp, HeatStrain, HeatLoad, VAM
                    };

    typedef enum dataseries DataSeries;

    double value(DataSeries) const;
    static QString seriesName(DataSeries);
    static QString seriesSymbol(DataSeries);
    static const QList<DataSeries> &listDataSeries();

    // style is coded to be compatible with FIT files
    // we use same IDs than ANT power meters messages.
    enum riderposition { seated = 0, transistionToSeated = 1, standing = 2, transitionToStanding=3 };
    typedef enum riderposition riderPosition;

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
    void setWheelRpm(double wheelRpm, bool fMarkTimeSample = false);
    void setCadence(double aCadence);
    void setLoad(double load);
    void setSlope(double slope);
    void setMsecs(long);
    void setLapMsecs(long);
    void setLapMsecsRemaining(long);
    void setErgMsecsRemaining(long);
    void setDistance(double);
    void setRouteDistance(double);
    void setDistanceRemaining(double);
    void setVAM(double);
    void setXPower(double);
    void setBikeScore(double);
    void setRI(double);
    void setSkibaVI(double);
    void setIsoPower(double);
    void setBikeStress(double);
    void setIF(double);
    void setVI(double);
    void setJoules(double);
    void setLap(long);
    void setLapDistance(double distance);
    void setLapDistanceRemaining(double distance);
    void setLRBalance(double);
    void setLTE(double);
    void setRTE(double);
    void setLPS(double);
    void setRPS(double);
    void setRppb(double);
    void setRppe(double);
    void setRpppb(double);
    void setRpppe(double);
    void setLppb(double);
    void setLppe(double);
    void setLpppb(double);
    void setLpppe(double);
    void setRightPCO(double);
    void setLeftPCO(double);
    void setPosition(RealtimeData::riderPosition);
    void setTorque(double);
    void setRTorque(double);
    void setLTorque(double);
    void setLatitude(double);
    void setLongitude(double);
    void setAltitude(double);

    void setAvgWatts(double);
    void setAvgSpeed(double);
    void setAvgCadence(double);
    void setAvgHeartRate(double);
    void setAvgWattsLap(double);
    void setAvgSpeedLap(double);
    void setAvgCadenceLap(double);
    void setAvgHeartRateLap(double);

    void setCoreTemp(double,double,double);
    const char *getName() const;

    // new muscle oxygen stuff
    void setHb(double smo2, double thb);
    double getSmO2() const;
    double gettHb() const;
    double getHHb() const;
    double getO2Hb() const;

    // VO2 related metrics
    void setVO2_VCO2(double vo2, double vco2);
    void setRf(double rf);
    void setRMV(double rmv);
    void setTv(double tv);
    void setFeO2(double feo2);
    double getVO2() const;
    double getVCO2() const;
    double getRf() const;
    double getRMV() const;
    double getRER() const;
    double getTv() const;
    double getFeO2() const;
    double getCoreTemp() const;
    double getSkinTemp() const;
    double getHeatStrain() const;

    double getWatts() const;
    double getAltWatts() const;
    double getAltDistance() const;
    double getHr() const;
    long getTime() const;
    double getSpeed() const;
    double getWbal() const;
    double getJoules() const;
    double getVirtualSpeed() const;
    double getWheelRpm() const;
    std::chrono::high_resolution_clock::time_point getWheelRpmSampleTime() const;
    double getCadence() const;
    double getLoad() const;
    double getSlope() const;
    long getMsecs() const;
    long getLapMsecs() const;
    double getDistance() const;
    double getRouteDistance() const;
    double getDistanceRemaining() const;
    double getVAM() const;
    double getXPower() const;
    double getBikeScore() const;
    double getRI() const;
    double getSkibaVI() const;
    double getIsoPower() const;
    double getBikeStress() const;
    double getIF() const;
    double getVI() const;
    long getLap() const;
    double getLapDistance() const;
    double getLapDistanceRemaining() const;
    double getLRBalance() const;
    double getLTE() const;
    double getRTE() const;
    double getLPS() const;
    double getRPS() const;
    double getRppb() const;
    double getRppe() const;
    double getRpppb() const;
    double getRpppe() const;
    double getLppb() const;
    double getLppe() const;
    double getLpppb() const;
    double getLpppe() const;
    double getRightPCO() const;
    double getLeftPCO() const;
    RealtimeData::riderPosition getPosition() const;
    double getTorque() const;
    double getLatitude() const;
    double getLongitude() const;
    double getAltitude() const;

    double getAvgWatts() const;
    double getAvgSpeed() const;
    double getAvgCadence() const;
    double getAvgHeartRate() const;
    double getAvgWattsLap() const;
    double getAvgSpeedLap() const;
    double getAvgCadenceLap() const;
    double getAvgHeartRateLap() const;

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

    void setTemp(double temp);
    double getTemp() const;

    uint8_t spinScan[24];

private:
    char name[64];

    // realtime telemetry
    double hr, watts, altWatts, altDistance, speed, wheelRpm, load, slope, lrbalance;
    double cadence;      // in rpm
    double smo2, thb;
    double lte, rte, lps, rps; // torque efficiency and pedal smoothness
    double rppb, rppe, rpppb, rpppe;
    double lppb, lppe, lpppb, lpppe;
    double rightPCO, leftPCO;
    double torque; // raw torque data for calibration display
    double RTorque, LTorque;
    double latitude, longitude, altitude;
    double vo2, vco2, rf, rmv, tv, feo2;
    RealtimeData::riderPosition position;
    double temp;
    double skinTemp, coreTemp;
    double heatStrain;

    std::chrono::high_resolution_clock::time_point wheelRpmSampleTime;

    // derived data
    double distance;
    double routeDistance;
    double distanceRemaining;
    double VAMValue;
    double lapDistance;
    double lapDistanceRemaining;
    double virtualSpeed;
    double wbal;
    double joules;
    double xPower, bikeScore, rI, skibaVI;
    double isoPower, bikeStress, iF, vI;
    double hhb, o2hb;
    double rer;
    long lap;
    long msecs;
    long lapMsecs;
    long lapMsecsRemaining;
    long ergMsecsRemaining;
    double avgWatts, avgSpeed, avgCadence, avgHeartRate;
    double avgWattsLap, avgSpeedLap, avgCadenceLap, avgHeartRateLap;

    bool trainerStatusAvailable;
    bool trainerReady;
    bool trainerRunning;
    bool trainerCalibRequired;
    bool trainerConfigRequired;
    bool trainerBrakeFault;
};

// Class implements queue for tracking altitude across time for
// past minute. This is used to derive a vertical meters per minute
// value that is retuned by each push.
// A route location change of more than 50 meters in a second is
// considered a 'skip' and clears the vam queue.
// New data is accepted only every second.

#include <queue>

class Vaminator {
    std::deque<std::tuple<double, double, double>> q;

public:
    Vaminator() {}
    double Push(double altitude, double sessionMS, double routeLocationKM) {

        bool fDoPush = true;
        if (!q.empty()) {
            // a seek is more than 50 meters in a second
            double routeDelta = routeLocationKM - std::get<2>(q.back());
            if (fabs(routeDelta) > (50. / 1000.)) {
                q.clear(); // make empty
            } else {
                // Pop elements more than 1 minute back
                double startTimeMS = std::get<1>(q.front());
                while ((sessionMS - startTimeMS) > (60. * 1000.)) {
                    q.pop_front();
                    if (q.empty())
                        break;
                    startTimeMS = std::get<1>(q.front());
                }

                fDoPush = q.empty() || sessionMS - 1000. >= std::get<1>(q.back());
            }
        }

        // Push element if its > 1 second ahead of newest
        if (fDoPush)
            q.push_back(std::make_tuple( altitude, sessionMS, routeLocationKM ));

        double vertDeltaM = std::get<0>(q.back()) - std::get<0>(q.front());
        double timeDeltaHour = (std::get<1>(q.back()) - std::get<1>(q.front())) / (1000. * 60. * 60);

        // Require 1 second of time data before posting non-zero value.
        return (timeDeltaHour > (1./3600.)) ? vertDeltaM / timeDeltaHour : 0.;
    }
};

class RealtimeDataSession : public RealtimeData
{
public:
    RealtimeDataSession(double CP, double WPRIME, double TAU);
    void newLap();
    void updateDerived();

private:
    double CP, WPRIME, TAU;

    Vaminator vaminator;

    double wbalr, wbal;

    double sumAvgWatts, sumAvgSpeed, sumAvgCadence, sumAvgHeartRate;
    int nAvgWatts, nAvgSpeed, nAvgCadence, nAvgHeartRate;
    double sumAvgWattsLap, sumAvgSpeedLap, sumAvgCadenceLap, sumAvgHeartRateLap;
    int nAvgWattsLap, nAvgSpeedLap, nAvgCadenceLap, nAvgHeartRateLap;

    // for keeping track of rolling averages (max 30s at 5hz)
    // used by IsoPower and XPower
    QVector<double> rolling;
    double sum, rollingSum;
    int count, index; // index into rolling (circular buffer)
    // used by XPower algorithm
    double rsum, ewma, xsum;
    int rcount;
};

#endif
