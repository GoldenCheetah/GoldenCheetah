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

    bool canGenerate() const { return ftp > 0 || cp > 0; }
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

        /** Small deterministic draft for tests and offline demos (no ML). */
        static WorkoutDraft exampleDeterministicDraft();
};

#endif
