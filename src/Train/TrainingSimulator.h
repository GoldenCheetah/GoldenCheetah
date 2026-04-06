/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_TRAINING_SIMULATOR_H
#define GC_TRAINING_SIMULATOR_H

#include "TrainingConstraintChecker.h"
#include "WorkoutGenerationService.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

/**
 * Training goal affects how candidate workouts are ranked.
 */
enum class TrainingGoal {
    Build,      // maximize CTL increase (fitness building phase)
    Maintain,   // keep CTL stable with minimal fatigue
    Taper,      // reduce fatigue while preserving fitness (pre-race)
    Recover     // maximize TSB recovery
};

QString trainingGoalToString(TrainingGoal g);
TrainingGoal trainingGoalFromString(const QString &s);

/**
 * Forward projection result for a single candidate workout.
 */
struct SimulationResult {
    QString workoutType;
    double estimatedTSS = 0.0;

    // 7-day projected values (after the workout day)
    QVector<double> projCtl;
    QVector<double> projAtl;
    QVector<double> projTsb;

    // Scoring
    double score = 0.0;
    bool feasible = true;
    ConstraintCheckResult constraints;

    // Pre-existing hard violations that also occur with a rest day (TSS=0).
    // These are conditions the athlete can't fix today.
    int preExistingHardViolations = 0;

    QJsonObject toJson() const;
};

/**
 * Ranked set of candidate workout simulations.
 */
struct SimulationRanking {
    TrainingGoal goal = TrainingGoal::Build;
    QList<SimulationResult> candidates;

    QJsonObject toJson() const;
};

/**
 * Forward simulation engine for training plan evaluation.
 *
 * Phase 1 (PMC-based): Given an athlete snapshot, estimates TSS for each
 * of the 7 standard workout types, projects CTL/ATL/TSB forward 7 days,
 * checks all constraints, and scores/ranks candidates by training goal.
 *
 * This does NOT require Banister performance tests — it works with
 * basic CTL/ATL/TSB data available for any athlete.
 *
 * SAFETY: All results are filtered through TrainingConstraintChecker.
 * No candidate that violates hard constraints will be ranked first.
 */
class TrainingSimulator {

public:

    /**
     * Estimate the TSS a workout type would produce for the given snapshot.
     * Uses typical IF * duration relationships per workout type.
     */
    static double estimateTSS(const WorkoutAthleteSnapshot &snapshot,
                              const QString &workoutType,
                              int durationMin = 60);

    /**
     * Simulate a single workout candidate forward in time.
     * Projects CTL/ATL/TSB for projectionDays after the workout day.
     */
    static SimulationResult simulateCandidate(const WorkoutAthleteSnapshot &snapshot,
                                              const QString &workoutType,
                                              int durationMin = 60,
                                              int projectionDays = 7,
                                              const TrainingConstraintBounds &bounds = TrainingConstraintBounds::recreational());

    /**
     * Simulate all standard workout types and rank by training goal.
     */
    static SimulationRanking rankCandidates(const WorkoutAthleteSnapshot &snapshot,
                                            TrainingGoal goal,
                                            int durationMin = 60,
                                            int projectionDays = 7,
                                            const TrainingConstraintBounds &bounds = TrainingConstraintBounds::recreational());

    /**
     * Score a simulation result against a training goal.
     */
    static double scoreForGoal(const SimulationResult &result,
                               const WorkoutAthleteSnapshot &snapshot,
                               TrainingGoal goal);

    /** Canonical list of workout types the simulator evaluates. */
    static QStringList workoutTypes();
};

#endif
