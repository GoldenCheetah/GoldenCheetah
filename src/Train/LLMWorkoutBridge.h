/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_LLM_WORKOUT_BRIDGE_H
#define GC_LLM_WORKOUT_BRIDGE_H

#include "TrainingConstraintChecker.h"
#include "TrainingSimulator.h"
#include "WorkoutDraft.h"

#include <QJsonObject>
#include <QString>

/**
 * Context package sent to an LLM for workout session design.
 *
 * The math models determine WHAT load to prescribe (workout type + TSS target).
 * The LLM determines HOW to structure the session (intervals, rest, progression).
 *
 * This struct bundles everything the LLM needs to make micro-level decisions:
 * interval durations, rest periods, progression patterns, warmup/cooldown design.
 */
struct LLMWorkoutContext {
    // From the simulation engine (Phase 1+2)
    WorkoutAthleteSnapshot snapshot;
    SimulationResult recommendation;      // the winning candidate from ranking
    TrainingGoal goal;

    // Workout envelope constraints
    QString workoutType;                  // e.g. "sweetspot", "vo2max"
    double targetTSS = 0.0;              // TSS the session should produce
    int targetDurationMin = 60;

    // Athlete context for session design
    QString athleteNotes;                 // freeform athlete input ("legs feel heavy")
    int daysToEvent = -1;
    double tsbTrend = 0.0;

    // Safety bounds the LLM must respect
    TrainingConstraintBounds bounds;

    QJsonObject toJson() const;
};

/**
 * LLM response validated and converted to a WorkoutDraft.
 */
struct LLMWorkoutResponse {
    WorkoutDraft draft;
    bool valid = false;
    double actualTSS = 0.0;             // TSS of the generated workout
    double tssDeviation = 0.0;          // |actualTSS - targetTSS| / targetTSS
    QStringList validationErrors;

    QJsonObject toJson() const;
};

/**
 * Phase 5 design: LLM Tactical Executor Bridge.
 *
 * This is the integration layer between mathematical training models
 * and LLM-based session designers. It:
 *
 * 1. Packages model outputs into structured prompts for LLMs
 * 2. Validates LLM responses against safety constraints
 * 3. Ensures generated workouts match the prescribed TSS envelope
 *
 * ARCHITECTURE PRINCIPLE: "LLM as Coach, Models as Engine"
 * - Math models decide load distribution (macro planning)
 * - LLM designs session structure (micro planning)
 * - Hard constraints override both
 *
 * The LLM is an ADVISOR, not an AUTONOMOUS AGENT. All outputs are validated.
 *
 * CURRENT STATUS: Interface and validation layer. Actual LLM communication
 * is handled by external callers (MCP tools, REST API clients) — this class
 * provides the structured context and validation, not the LLM call itself.
 */
class LLMWorkoutBridge {

public:

    /**
     * Build the context package for an LLM workout design request.
     * Takes the simulation ranking output and prepares everything
     * the LLM needs for session design.
     */
    static LLMWorkoutContext buildContext(const WorkoutAthleteSnapshot &snapshot,
                                         const SimulationRanking &ranking,
                                         int durationMin = 60,
                                         const QString &athleteNotes = QString());

    /**
     * Generate a structured prompt for the LLM.
     * Returns a JSON object that can be sent to any LLM API.
     *
     * The prompt includes:
     * - Athlete metrics and current state
     * - Recommended workout type and TSS target
     * - Training goal context
     * - Safety constraints
     * - Expected output format (WorkoutDraft JSON schema)
     */
    static QJsonObject generatePrompt(const LLMWorkoutContext &ctx);

    /**
     * Validate an LLM-generated WorkoutDraft against the context.
     * Checks:
     * - TSS is within ±20% of target
     * - No step exceeds safe power limits
     * - Duration matches request
     * - All steps have valid power values
     */
    static LLMWorkoutResponse validateDraft(const WorkoutDraft &draft,
                                            const LLMWorkoutContext &ctx);

    /**
     * Estimate the TSS of a WorkoutDraft.
     * Uses step durations and power targets relative to FTP.
     */
    static double estimateDraftTSS(const WorkoutDraft &draft, int ftp);

    /**
     * Clamp an LLM-generated draft to safety limits.
     * Reduces any power values that exceed safe bounds.
     * Returns a modified copy.
     */
    static WorkoutDraft clampToSafety(const WorkoutDraft &draft,
                                      const TrainingConstraintBounds &bounds,
                                      int ftp);
};

#endif
