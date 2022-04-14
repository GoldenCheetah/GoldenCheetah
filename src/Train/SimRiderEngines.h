/*
 * Copyright (c) 2020 Peter Kanatselis (pkanatselis@gmail.com)
 * Copyright (c) 2019 Eric Christoffersen (impolexg@outlook.com)
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

#include <QObject>
#include <QString>
#include "Context.h"
#include "Settings.h"
#include "Athlete.h"
#include "ErgFile.h"
#include "BicycleSim.h"
#include "Units.h"
#include "SimRiderStateData.h"
#include <QVector>

#ifndef _GC_SimRiderEngines_h
#define _GC_SimRiderEngines_h

struct validRouteInfo {
    double avgSpeed, totalSecs;
    QString fileFullPath, fileRouteTag;
};

class SimRiderEngines : public QObject {
    Q_OBJECT

public:
    SimRiderEngines(Context* context); // default ctor
    ~SimRiderEngines();
    void initEngines(Context* context);
    void runEngines(Context* context, double displayWorkoutDistance, double curWatts, long total_msecs);
    void updateSimRiderData();
    bool WorkoutFinished() const { return workoutFinished; };

    void setDistForAllSimRiders(double d) {
        for (int i = 0; i < riderNest.size(); i++) {
            riderNest[i].Distance() += d;
        }
    }
    // ErgFile wrapper to support stateful location queries.
    ErgFileQueryAdapter        ergFileQueryAdapter;

private slots:
    void stop();
    void start();
    void findValidRoutes(Context* context, ErgFile* f);

private:

    Context* context;
    double warmupCooldownPct = 0.05;
    double rideAttackEligable = 0.9;
    double prevRouteDistance = -1.;
    double simRiderRouteDistance, engineErrNum;
    std::vector<validRouteInfo> validRoutes;    // Allow up to 4 additional simRiders
    QString attackStatus, fileNameERG, fileNameGCRide, engineErrText;
    int coolDownStarts, warmUpEnds;
    int attackTotal, pacingInterval, maxSeparationLimit, savedMaxSeparationLimit;
    int nextAttackAt, attackPower, currentAttackAt, attackPowerIncrease;
    int attackDuration, engineType;
    QVector<double> attackLocationList;
    bool isAttacking, engineInitialized, isSimRiderEnabled, workoutFinished, isSimRiderWaiting;
    int attackCount;
    ErgFile* pErgFile;
    RideFile* previousGCRideFile;
    RideFile* prevGCRideFiles[5];
    SimRiderStateData srData;
    RiderNest riderNest;
    int nestSize;
    std::vector<double> riderDeltas;
    double attkDurationPct = 0.; // % of the ride that attacks will occur (Default 90%) = Duration - (warmup + cooldown)
    double vLat, vLon, vAlt, vDist, vWatts;

};

#endif