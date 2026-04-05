/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_TRAINING_PLAN_H
#define GC_TRAINING_PLAN_H

#include "BanisterSimulator.h"
#include "TrainingConstraintChecker.h"

#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QUuid>

class Context;

/**
 * One phase of a multi-week training plan (e.g., "Base", "Build", "Taper").
 *
 * Captures the macro-level prescription: weekly TSS target, mesocycle template,
 * intended workout type distribution, and duration.
 */
struct TrainingPlanPhase {
    QString name;
    int durationWeeks = 4;
    double weeklyTSS = 0.0;
    QString templateName;           // e.g., "progressive", "3-1", "flat"
    QString goal;                   // e.g., "build", "maintain", "taper", "recover"

    /** Workout type mix as fraction (0.0–1.0). Keys: recovery, endurance, sweetspot, etc. */
    QMap<QString, double> workoutMix;

    /** Optional notes for this phase. */
    QString notes;

    QJsonObject toJson() const;
    static TrainingPlanPhase fromJson(const QJsonObject &o, bool *ok = nullptr);
};

/**
 * Status of a training plan.
 */
enum class TrainingPlanStatus {
    Draft,      // created but not yet started
    Active,     // currently being followed
    Completed,  // finished
    Archived    // no longer relevant
};

QString trainingPlanStatusToString(TrainingPlanStatus s);
TrainingPlanStatus trainingPlanStatusFromString(const QString &s);

/**
 * A multi-week training plan that captures the macro planning context.
 *
 * Sits above individual WorkoutDrafts and planned activities. Preserves the
 * reasoning from simulation/Banister analysis so plans can be resumed,
 * adapted, and tracked across sessions.
 *
 * Serialized as JSON and stored in the athlete's config directory.
 */
struct TrainingPlan {
    static constexpr const char *SCHEMA_VERSION = "1";

    /** Unique plan identifier (UUID). */
    QString id;
    QString schemaVersion = QString::fromLatin1(SCHEMA_VERSION);

    /** Human-readable plan name. */
    QString name;
    QString description;

    /** Overall training goal. */
    QString goal;

    /** Ordered phases of the plan. */
    QList<TrainingPlanPhase> phases;

    /** Snapshot of Banister params at plan creation for audit trail. */
    BanisterParams banisterSnapshot;

    /** Constraint bounds used during plan creation. */
    TrainingConstraintBounds constraintBounds;

    /** Athlete snapshot values at plan creation for context. */
    double startingCTL = 0.0;
    double startingATL = 0.0;
    double startingTSB = 0.0;
    int athleteFTP = 0;

    /** Plan timeline. */
    QDate startDate;
    QDate endDate;
    TrainingPlanStatus status = TrainingPlanStatus::Draft;

    /** Audit trail. */
    QString generatorId;
    QString modelId;
    QString modelVersion;
    QDateTime createdAt;
    QDateTime updatedAt;

    /** Evaluation schedule: dates when the plan should be re-assessed. */
    QList<QDate> evaluationDates;

    /** Compute total duration in weeks across all phases. */
    int totalWeeks() const;

    /** Compute total planned TSS across the entire plan. */
    double totalPlannedTSS() const;

    /** Generate a new UUID-based plan id. */
    static QString generateId();

    QJsonObject toJson() const;
    static TrainingPlan fromJson(const QJsonObject &o, bool *ok = nullptr);

    // --- File I/O ---

    /** Directory path for training plans within athlete home. */
    static QString planDirectory(Context *context);

    /** Save this plan as JSON to the athlete's plan directory. Returns filepath on success. */
    bool save(Context *context, QString *savedPath = nullptr, QStringList *errors = nullptr) const;

    /** Load a plan from file. */
    static TrainingPlan load(const QString &filepath, bool *ok = nullptr);

    /** List all plans for an athlete. */
    static QList<TrainingPlan> listAll(Context *context);

    /** Delete a plan file by id. */
    static bool remove(Context *context, const QString &planId, QStringList *errors = nullptr);
};

#endif
