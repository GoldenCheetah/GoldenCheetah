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

#include "SimRiderEngines.h"
#include <QSound>
#include <QDebug>
#include <iostream>
#include <vector>

SimRiderEngines::SimRiderEngines(Context* context) : riderNest(context) { 
    engineInitialized = false;
    QString sSimRiderEnabled = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ISENABLED, "").toString();
    if (sSimRiderEnabled == "1") {
        isSimRiderEnabled = true;
        engineErrText = "SimRider enabled, not initialized";
    }
    else {
        isSimRiderEnabled = false;
        engineErrText = "SimRider disabled, not initialized";
    }

    attackStatus = "NOT STARTED";
    workoutFinished = true;
    isAttacking = false;
    nextAttackAt = -1;
    attackCount = -9999;
    vLat = -999.;
    vLon = -999.;
    vAlt = -999.;
    vDist = -999.;
    vWatts = -999.;
    engineType = 0;
    isSimRiderWaiting = false;
    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(SimRiderStateUpdate(SimRiderStateData)), this, SLOT(SimRiderStateUpdate(SimRiderStateData)));
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));

    //Test_RunAll();
}

SimRiderEngines::~SimRiderEngines() {

}

void SimRiderEngines::stop() {
    engineInitialized = false;
    for (int i = 0; i < riderNest.size(); i++) {
        srData.srDistanceFor(i) = 0.;
        srData.srWattsFor(i) = 0.;
        srData.srLatitudeFor(i) = 0.;
        srData.srLongitudeFor(i) = 0.;
        srData.srAltitudeFor(i) = 0.;
        srData.srSpeedFor(i) = 0.;
    }
}

void SimRiderEngines::start() {
    // Force SimRider to read engine settings 
    engineInitialized = false;
    ergFileQueryAdapter.resetQueryState();
    riderNest.resize(0);
    for (int i = 0; i < riderNest.size(); i++) {
        riderNest[i].clear();
    }
    riderNest.resize(0);
}

void SimRiderEngines::findValidRoutes(Context* ctx, ErgFile* f) {
    try {
        // f is the workout the user selected. It must be valid
        if (f && f->filename != "") {

            QStringList errors_; // validFiles;
            RideFile* selectedRideFile;
            QString selectedFileRouteTag;
            QFile rideFile(f->filename);
            selectedRideFile = RideFileFactory::instance().openRideFile(ctx, rideFile, errors_);
            validRoutes.resize(0);
            int routeCtr = 0;
            if (errors_.size() == 0) selectedFileRouteTag = selectedRideFile->getTag("Route", "");
            else qDebug() << __FUNCTION__ << " ERROR reading file tag information.";
            if (!selectedFileRouteTag.isEmpty()) srData.srRouteName() = selectedFileRouteTag;

            QString path = ctx->athlete->home->activities().canonicalPath() + "/";
            QDirIterator it(path, QStringList() << "*.json", QDir::Files, QDirIterator::NoIteratorFlags);
            while (it.hasNext()) {
                // Build a list of files with the correct tag name to be used as simRider for the previous ride mode
                QStringList errors_;
                RideFile* tempRideFile;
                QString currFile, myTag;
                currFile = it.next();
                QFile rideFile(currFile);
                tempRideFile = RideFileFactory::instance().openRideFile(ctx, rideFile, errors_);
                QVariant avgSpeed, totalSecs;
                if ((tempRideFile != NULL) && errors_.size() == 0) {
                    myTag = tempRideFile->getTag("Route", "");
                    if ((selectedFileRouteTag == myTag) && (!myTag.isEmpty()) && (!selectedFileRouteTag.isEmpty())) {
                        qDebug() << currFile << " route tag: " << myTag << " avg speed: " << tempRideFile->getAvgPoint(RideFile::kph) << " Totaltime: " << tempRideFile->getMaxPoint(RideFile::secs);
                        validRouteInfo v;
                        v.avgSpeed = tempRideFile->getAvgPoint(RideFile::kph).toDouble();
                        v.totalSecs = tempRideFile->getMaxPoint(RideFile::secs).toDouble();
                        v.fileFullPath = currFile;
                        v.fileRouteTag = myTag;
                        validRoutes.push_back(v);
                        routeCtr++;
                    }
                }
                else {
                    qDebug() << __FUNCTION__ << " ERROR reading file tag information.";
                }
            }
            // Find up to the best 4 routes to include SimRiders
            if (routeCtr > 0) {
                // Sort vector by time. We want to keep the smaller numbers
                std::sort(validRoutes.begin(), validRoutes.end(), [](const validRouteInfo& a, const validRouteInfo& b) { return a.avgSpeed < b.avgSpeed; });
                // Drop routes at possition 4 and higher 
                while (validRoutes.size() > 4) validRoutes.pop_back();
            }

        }
    }
    catch (const char* message) {
        qDebug() << __FUNCTION__ << "EROOR ===> " << message;
    }
}


void SimRiderEngines::initEngines(Context* context) {
    QString simRiderTotalAttacks;
    QString sSimRiderEnabled = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ISENABLED, "").toString();
    engineType = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "").toInt();
    engineInitialized = false;
    srData.srEngineType() = -1;

    if (sSimRiderEnabled == "1" && engineType >= 0 && engineType < 2) {
        isSimRiderEnabled = true;
        srData.srFeatureEnabled() = isSimRiderEnabled;
        srData.srEngineType() = engineType;
    }
    else {
        isSimRiderEnabled = false;
        srData.srFeatureEnabled() = false;
    }

    if (context->currentErgFile() && !engineInitialized && isSimRiderEnabled) {
        // Initialize the selected SimRider engine
        switch (engineType) {
        case 0: // Intervals AI engine. Need to code FSM

            break;
        case 1:
            // Previous ride
            findValidRoutes(context, context->currentErgFile());

            if (validRoutes.size() > 0) {
                // Set intitial riders. riderNest[0] will always hold the current user.
                riderNest.resize(validRoutes.size() + 1);         // Allow for 4 rides plus the current user at possition 0  
                srData.srCount() = (int)riderNest.size();
            }
            else {
                riderNest.resize(1); // No routes found for selected workout
                srData.srCount() = 1;
            }
            srData.srfileNameGCRideFor(0) = "";
            srData.srDistanceFor(0) = 0.;
            srData.srIsEnabledFor(0) = true;
            srData.srIsWorkoutFinishedFor(0) = false;
            srData.srIsEngineInitializedFor(0) = true;
            srData.srErrorMsgFor(0) = "Engine initialized, previous route loaded.";
            srData.srErrorCodeFor(0) = 0;
            for (int i = 1; i < riderNest.size(); i++) {
                srData.srfileNameGCRideFor(i) = validRoutes[i - 1].fileFullPath;
                if (!srData.srfileNameGCRideFor(i).isEmpty()) {
                    QStringList errors_;
                    QFile rideFile(srData.srfileNameGCRideFor(i));
                    prevGCRideFiles[i] = RideFileFactory::instance().openRideFile(context, rideFile, errors_);
                    if ((prevGCRideFiles[i] != NULL) && (prevGCRideFiles[i]->areDataPresent()->watts)) {
                        srData.srIsEngineInitializedFor(i) = true;
                        srData.srIsEnabledFor(i) = true;
                        srData.srErrorMsgFor(i) = "Engine initialized, previous route loaded.";
                        srData.srErrorCodeFor(i) = 0;
                        srData.srIsWorkoutFinishedFor(i) = false;
                        srData.srDistanceFor(i) = 0.;
                        engineInitialized = true;
                    }
                    else {
                        srData.srIsEngineInitializedFor(i) = false;
                        srData.srErrorMsgFor(i) = "Error initializing Sim Rider engine.";
                        srData.srErrorCodeFor(i) = -1;
                        srData.srIsEnabledFor(i) = false;
                        srData.srIsWorkoutFinishedFor(i) = true;

                    }
                }
            }
            engineInitialized = true;
            break;
        default: // Invalid selection 
            qDebug() << __FUNCTION__ << "Invalid Virtual Partner engine number.";
            engineInitialized = false;
            break;
        }
        isAttacking = false;
        workoutFinished = false;
        attackStatus = "Pacing";
    }
    else { // Engine not initialized, SimRider not enabled, no workout file selected
        qDebug() << __FUNCTION__ << "Virtual Partner Engine cannot initialize. Check chart settings";
        engineErrText = "Engine not initialized, SimRider not enabled, no workout file selected.";
        engineErrNum = -99;
    }
    context->notifySimRiderStateUpdate(srData);
}

void SimRiderEngines::runEngines(Context* context, double displayWorkoutDistance, double curWatts, long total_msecs) {

    // Run the selected SimRider engine
    if (isSimRiderEnabled && engineInitialized) {
        if ((riderNest[0].Distance() * 1000) > context->workout->Duration) {
            workoutFinished = true;
            qDebug() << __FUNCTION__ << "User finished the workout.";
        }
        else {
            workoutFinished = false;
        }
        double curDist = displayWorkoutDistance * 1000;
        switch (engineType) {
        case 0: // Intervals AI engine. Need to code FSM

            break;
        case 1:
            // Previous ride
            riderNest[0].Watts() = curWatts;  // Current user
            for (int i = 1; i < riderNest.size(); i++) {
                if ((riderNest[i].Distance() * 1000) >= context->workout->Duration && !srData.srIsWorkoutFinishedFor(i)) {
                    srData.srIsWorkoutFinishedFor(i) = true;
                    riderNest[i].clear();
                }
                int currentSecond = (int)total_msecs / 1000; //Used to query the previous route for power at that time
                // Check if simRider is finished
                if (!srData.srIsWorkoutFinishedFor(i)) {
                    riderNest[i].Watts() = prevGCRideFiles[i]->dataPoints()[currentSecond]->watts;
                }
                else {
                    riderNest[i].Watts() = 0; // SimRider is done, set watts to 0
                }
            }
            riderNest.update(context->currentErgFile());
            for (int i = 0; i < riderNest.size(); i++) {
                srData.srDistanceFor(i) = riderNest[i].Distance();
                srData.srWattsFor(i) = riderNest[i].Watts();
                srData.srLatitudeFor(i) = riderNest[i].Latitude();
                srData.srLongitudeFor(i) = riderNest[i].Longitude();
                srData.srAltitudeFor(i) = riderNest[i].Altitude();
                srData.srSpeedFor(i) = riderNest[i].Speed();
            }
            break;
        default: // Invalid selection 
            qDebug() << __FUNCTION__ << "Invalid Virtual Partner engine number.";
            break;
        }  // Switch Case ENDS

    }
    else { // Engine not inititalized and/or SimRider not enabled
        qDebug() << __FUNCTION__ << "Virtual Partner Engine not inititalized and/or SimRider not enabled";
    }
    updateSimRiderData();
    context->notifySimRiderStateUpdate(srData);
}

void SimRiderEngines::updateSimRiderData() {
    srData.srNextAttack() = nextAttackAt;
    srData.srAttackCount() = attackCount;
    srData.srIsAttacking() = isAttacking;
    srData.srEngineinitialized() = engineInitialized;
    srData.srFeatureEnabled() = isSimRiderEnabled;
    if (attackCount >= 7) qDebug() << " ===> " << __FUNCTION__ << "attackCount = " << attackCount;
}

