/*
 * Copyright (c) 2022 Peter Kanatselis (pkanatselis@gmail.com)
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

#include <QString>

#ifndef _GC_SimRiderStateData_h
#define _GC_SimRiderStateData_h

struct SimRiderRealTimeData {
    double srLat, srLon, srAlt, srWatts, srDist, srSpeed; 
    double srRaceStartDist, srRaceDuration;  // Future enhancment to ride part of the route
    bool srIsEnabled, srIsWorkoutFinished, srIsEngineInitialized;  
    QString srAttackStatus, srErrorMsg, fileNameERG, fileNameGCRide, simRiderDisplayStatus;
    int srAttackCount, srErrorCode;
};

class SimRiderStateData {

private:

    SimRiderRealTimeData srRTD[5];    // Allow up to 4 additional simRiders
    bool isSimRiderFeatureEnabled, isSimRiderEngineInitialized, isSimRiderAttacking;
    int simRiderEngineType;
    int simRidersCount, simRiderAttackCount, simRiderNextAtack;
    QString SimRiderRouteName;

public:
    
    SimRiderStateData() {};

    //Setters
    double& srDistanceFor(int idx) { return this->srRTD[idx].srDist; };
    double& srLatitudeFor(int idx) { return this->srRTD[idx].srLat; };
    double& srLongitudeFor(int idx) { return this->srRTD[idx].srLon; };
    double& srAltitudeFor(int idx) { return this->srRTD[idx].srAlt; };
    double& srWattsFor(int idx) { return this->srRTD[idx].srWatts; };
    double& srSpeedFor(int idx) { return this->srRTD[idx].srSpeed; };
    double& srRaceStartDistFor(int idx) { return this->srRTD[idx].srRaceStartDist; };
    double& srRaceDurationFor(int idx) { return this->srRTD[idx].srRaceDuration; };

    bool& srIsEnabledFor(int idx) { return this->srRTD[idx].srIsEnabled; };
    bool& srIsWorkoutFinishedFor(int idx) { return this->srRTD[idx].srIsWorkoutFinished; };
    bool& srIsEngineInitializedFor(int idx) { return this->srRTD[idx].srIsEngineInitialized; };
    bool& srIsAttacking() { return isSimRiderAttacking; };

    QString& srAttackStatusFor(int idx) { return this->srRTD[idx].srAttackStatus; };
    QString& srErrorMsgFor(int idx) { return this->srRTD[idx].srErrorMsg; };
    QString& srfileNameERGFor(int idx) { return this->srRTD[idx].fileNameERG; };
    QString& srfileNameGCRideFor(int idx) { return this->srRTD[idx].fileNameGCRide; };
    QString& srDisplayStatusFor(int idx) { return this->srRTD[idx].simRiderDisplayStatus; };

    int& srAttackCountFor(int idx) { return this->srRTD[idx].srAttackCount; };
    int& srErrorCodeFor(int idx) { return this->srRTD[idx].srErrorCode; };

    bool& srFeatureEnabled() { return isSimRiderFeatureEnabled; };
    bool& srEngineinitialized() { return isSimRiderEngineInitialized; };
    int& srEngineType() { return simRiderEngineType; };
    int& srCount() { return simRidersCount; };
    int& srAttackCount() { return simRiderAttackCount; };
    int& srNextAttack() { return simRiderNextAtack; };
    QString& srRouteName() { return SimRiderRouteName; };

    //Getters
    double srDistanceFor(int idx) const { return this->srRTD[idx].srDist; };
    double srLatitudeFor(int idx) const { return this->srRTD[idx].srLat; };
    double srLongitudeFor(int idx) const { return this->srRTD[idx].srLon; };
    double srAltitudeFor(int idx) const { return this->srRTD[idx].srAlt; };
    double srWattsFor(int idx) const { return this->srRTD[idx].srWatts; };
    double srSpeedFor(int idx) const { return this->srRTD[idx].srSpeed; };
    double srRaceStartDistFor(int idx) const { return this->srRTD[idx].srRaceStartDist; };
    double srRaceDurationFor(int idx) const { return this->srRTD[idx].srRaceDuration; };

    bool srIsEnabledFor(int idx) const { return this->srRTD[idx].srIsEnabled; };
    bool srIsWorkoutFinishedFor(int idx) const { return this->srRTD[idx].srIsWorkoutFinished; };
    bool srIsEngineInitializedFor(int idx) const { return this->srRTD[idx].srIsEngineInitialized; };
    bool srIsAttacking() const { return isSimRiderAttacking; };

    QString srAttackStatusFor(int idx) const { return this->srRTD[idx].srAttackStatus; };
    QString srErrorMsg(int idx) const { return this->srRTD[idx].srErrorMsg; };
    QString srfileNameERGFor(int idx) const { return this->srRTD[idx].fileNameERG; };
    QString srfileNameGCRideFor(int idx) const { return this->srRTD[idx].fileNameGCRide; }
    QString srDisplayStatusFor(int idx) const { return this->srRTD[idx].simRiderDisplayStatus; };

    int srAttackCountFor(int idx) const { return this->srRTD[idx].srAttackCount; };
    int srErrorCodeFor(int idx) const { return this->srRTD[idx].srErrorCode; };

    bool srFeatureEnabled() const { return isSimRiderFeatureEnabled; };
    bool srEngineinitialized() const { return isSimRiderEngineInitialized; };
    int srEngineType() const { return simRiderEngineType; };
    int srCount() const { return simRidersCount; };
    int srAttackCount() const { return simRiderAttackCount; };
    int srNextAttack() const { return simRiderNextAtack; };
    QString srRouteName() const { return SimRiderRouteName; };
};

#endif
