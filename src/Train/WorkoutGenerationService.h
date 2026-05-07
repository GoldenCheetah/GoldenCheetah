/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_WORKOUT_GENERATION_SERVICE_H
#define GC_WORKOUT_GENERATION_SERVICE_H

#include "WorkoutDraft.h"

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QStringList>

class Context;
class ErgFile;

struct WorkoutAthleteSnapshot {
    QString athleteName;
    QDate date;
    int ftp = 0;
    int cp = 0;
    double ctl = 0.0;
    double atl = 0.0;
    double tsb = 0.0;

    // Phase 1 enrichments for simulation-based workout selection
    double tsbTrend = 0.0;         // 7-day TSB delta (positive = freshening, negative = accumulating fatigue)
    double ctlRampRate = 0.0;      // CTL change per week over last 14 days
    int daysToEvent = -1;          // days until next SeasonEvent (-1 = no event)
    QString nextEventName;         // name of next event
    QVector<double> recentStress;  // last 7 days of daily TSS (index 0 = 7 days ago)

    // HRV readiness (populated from Measures system when available)
    double hrvRMSSD = 0.0;         // today's RMSSD reading (msec)
    double hrvBaseline = 0.0;      // rolling baseline RMSSD (7-day average)
    double hrvRatio = 0.0;         // today / baseline (1.0 = normal)
    bool hrvAvailable = false;     // true if HRV data exists for scoring

    bool canGenerate() const { return ftp > 0 || cp > 0; }
    bool hasSimulationData() const { return ctl > 0.0 || atl > 0.0; }
    QJsonObject toJson() const;
};

struct WorkoutHeuristicRequest {
    QString workoutType;
    int durationMin = 60;
    QDate date;
    QString displayName;
    QString description;
    QString generatorId;
    QString modelId;
    QString modelVersion;

    static WorkoutHeuristicRequest fromJson(const QJsonObject &o, bool *ok = nullptr, QString *error = nullptr);
};

/**
 * Builds GoldenCheetah-native workouts from WorkoutDraft using existing ErgFile
 * parsing and TrainDB import paths (save .erg -> importWorkout).
 */
class WorkoutGenerationService {

    public:

        /** ERG text suitable for ErgFile::fromContent (Computrainer format). */
        static QString buildErgText(const WorkoutDraft &draft, int ftp, int cp);

        /**
         * Parse ERG text into a new ErgFile; caller owns the pointer.
         * Sets name, description (including audit metadata), and source from the draft.
         */
        static ErgFile *createErgFile(Context *context, const WorkoutDraft &draft,
                                      const QDate &when, QStringList &errors);

        /** Snapshot of the currently loaded athlete data required by AI workout tooling. */
        static WorkoutAthleteSnapshot athleteSnapshot(Context *context, const QDate &when = QDate());

        /** Deterministic, local-only heuristic generator derived from the prototype workout dialog. */
        static WorkoutDraft generateHeuristicDraft(const WorkoutAthleteSnapshot &snapshot,
                                                   const WorkoutHeuristicRequest &request,
                                                   QStringList *warnings = nullptr);

        /** Save a draft to the workout directory and import it into TrainDB. */
        static bool saveDraft(Context *context, const WorkoutDraft &draft, const QDate &when,
                              QString *savedPath, QStringList &errors);

        /**
         * Create a planned activity JSON entry that references a saved workout.
         * Uses the existing planned-activity directory and ride cache refresh flow.
         */
        static bool createPlannedActivity(Context *context, const QString &workoutPath,
                                          const QDateTime &when, const QString &sport,
                                          const QString &title, const QString &description,
                                          QString *savedPath, QStringList &errors);

        /**
         * Delete a planned activity by its file path, going through
         * RideCache::removeRide() to keep the UI in sync.
         */
        static bool deletePlannedActivity(Context *context, const QString &filepath,
                                          QStringList &errors);

        /**
         * Update a planned activity's metadata and/or date/time.
         * Routes through RideCache::moveActivity() for date changes and
         * saveActivity() for metadata to keep the UI in sync.
         */
        static bool updatePlannedActivity(Context *context, const QString &filepath,
                                          const QDateTime &newWhen,
                                          const QString &sport, const QString &title,
                                          const QString &description,
                                          const QString &workoutPath,
                                          QString *updatedPath, QStringList &errors);

        /**
         * Delete a workout (.erg/.mrc/.zwo) file and remove it from TrainDB.
         */
        static bool deleteWorkout(Context *context, const QString &filepath,
                                  QStringList &errors);

        /** Small deterministic draft for tests and offline demos (no ML). */
        static WorkoutDraft exampleDeterministicDraft();
};

#endif
