/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "LLMWorkoutBridge.h"

#include <QJsonArray>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// LLMWorkoutContext
// ---------------------------------------------------------------------------

QJsonObject LLMWorkoutContext::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("snapshot")] = snapshot.toJson();
    o[QStringLiteral("recommendation")] = recommendation.toJson();
    o[QStringLiteral("goal")] = trainingGoalToString(goal);
    o[QStringLiteral("workoutType")] = workoutType;
    o[QStringLiteral("targetTSS")] = targetTSS;
    o[QStringLiteral("targetDurationMin")] = targetDurationMin;
    if (!athleteNotes.isEmpty())
        o[QStringLiteral("athleteNotes")] = athleteNotes;
    if (daysToEvent >= 0)
        o[QStringLiteral("daysToEvent")] = daysToEvent;
    o[QStringLiteral("tsbTrend")] = tsbTrend;
    return o;
}

// ---------------------------------------------------------------------------
// LLMWorkoutResponse
// ---------------------------------------------------------------------------

QJsonObject LLMWorkoutResponse::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("valid")] = valid;
    o[QStringLiteral("actualTSS")] = actualTSS;
    o[QStringLiteral("tssDeviation")] = tssDeviation;
    if (!validationErrors.isEmpty()) {
        QJsonArray arr;
        for (const QString &e : validationErrors) arr.append(e);
        o[QStringLiteral("validationErrors")] = arr;
    }
    o[QStringLiteral("draft")] = draft.toJson();
    return o;
}

// ---------------------------------------------------------------------------
// Build context from simulation results
// ---------------------------------------------------------------------------

LLMWorkoutContext
LLMWorkoutBridge::buildContext(const WorkoutAthleteSnapshot &snapshot,
                               const SimulationRanking &ranking,
                               int durationMin,
                               const QString &athleteNotes)
{
    LLMWorkoutContext ctx;
    ctx.snapshot = snapshot;
    ctx.goal = ranking.goal;
    ctx.targetDurationMin = durationMin;
    ctx.athleteNotes = athleteNotes;
    ctx.daysToEvent = snapshot.daysToEvent;
    ctx.tsbTrend = snapshot.tsbTrend;

    // Pick the top-ranked feasible candidate
    for (const SimulationResult &r : ranking.candidates) {
        if (r.feasible) {
            ctx.recommendation = r;
            ctx.workoutType = r.workoutType;
            ctx.targetTSS = r.estimatedTSS;
            break;
        }
    }

    return ctx;
}

// ---------------------------------------------------------------------------
// Prompt generation
// ---------------------------------------------------------------------------

QJsonObject
LLMWorkoutBridge::generatePrompt(const LLMWorkoutContext &ctx)
{
    QJsonObject prompt;

    // System instruction
    prompt[QStringLiteral("role")] = QStringLiteral("system");
    prompt[QStringLiteral("instruction")] = QStringLiteral(
        "You are a cycling coach designing a structured indoor workout. "
        "The training simulation engine has already determined the optimal "
        "workout type and TSS target. Your job is to design the specific "
        "interval structure: warmup, main set, cooldown. "
        "Return a JSON object matching the WorkoutDraft schema."
    );

    // Athlete context
    QJsonObject athlete;
    athlete[QStringLiteral("ftp")] = ctx.snapshot.ftp;
    athlete[QStringLiteral("ctl")] = ctx.snapshot.ctl;
    athlete[QStringLiteral("atl")] = ctx.snapshot.atl;
    athlete[QStringLiteral("tsb")] = ctx.snapshot.tsb;
    athlete[QStringLiteral("tsbTrend")] = ctx.tsbTrend;
    if (ctx.daysToEvent >= 0)
        athlete[QStringLiteral("daysToEvent")] = ctx.daysToEvent;
    prompt[QStringLiteral("athlete")] = athlete;

    // Prescription
    QJsonObject prescription;
    prescription[QStringLiteral("workoutType")] = ctx.workoutType;
    prescription[QStringLiteral("targetTSS")] = ctx.targetTSS;
    prescription[QStringLiteral("durationMin")] = ctx.targetDurationMin;
    prescription[QStringLiteral("goal")] = trainingGoalToString(ctx.goal);
    prompt[QStringLiteral("prescription")] = prescription;

    // Constraints
    QJsonObject constraints;
    constraints[QStringLiteral("maxPowerPct")] = 200;  // no step above 200% FTP
    constraints[QStringLiteral("tssTolerancePct")] = 20; // ±20% of target
    constraints[QStringLiteral("requireWarmup")] = true;
    constraints[QStringLiteral("requireCooldown")] = true;
    prompt[QStringLiteral("constraints")] = constraints;

    // Output schema
    QJsonObject schema;
    schema[QStringLiteral("type")] = QStringLiteral("WorkoutDraft");
    schema[QStringLiteral("stepsFormat")] = QStringLiteral(
        "Array of {durationSec, powerMode:'PercentFtp', powerValue, label}"
    );
    prompt[QStringLiteral("outputSchema")] = schema;

    if (!ctx.athleteNotes.isEmpty())
        prompt[QStringLiteral("athleteNotes")] = ctx.athleteNotes;

    return prompt;
}

// ---------------------------------------------------------------------------
// TSS estimation for a draft
// ---------------------------------------------------------------------------

double
LLMWorkoutBridge::estimateDraftTSS(const WorkoutDraft &draft, int ftp)
{
    if (ftp <= 0) return 0.0;

    double totalTSS = 0.0;
    for (const WorkoutDraftStep &step : draft.steps) {
        double power;
        if (step.powerMode == WorkoutDraftPowerMode::PercentFtp) {
            power = ftp * step.powerValue / 100.0;
        } else {
            power = step.powerValue;
        }
        double ifactor = power / ftp;
        double hours = step.durationSec / 3600.0;
        totalTSS += hours * ifactor * ifactor * 100.0;
    }
    return totalTSS;
}

// ---------------------------------------------------------------------------
// Draft validation
// ---------------------------------------------------------------------------

LLMWorkoutResponse
LLMWorkoutBridge::validateDraft(const WorkoutDraft &draft,
                                const LLMWorkoutContext &ctx)
{
    LLMWorkoutResponse resp;
    resp.draft = draft;
    resp.valid = true;

    // Check non-empty
    if (draft.steps.isEmpty()) {
        resp.validationErrors << QStringLiteral("Draft has no steps");
        resp.valid = false;
        return resp;
    }

    // Estimate TSS
    resp.actualTSS = estimateDraftTSS(draft, ctx.snapshot.ftp);
    if (ctx.targetTSS > 0) {
        resp.tssDeviation = std::abs(resp.actualTSS - ctx.targetTSS) / ctx.targetTSS;
        if (resp.tssDeviation > 0.20) {
            resp.validationErrors << QStringLiteral("TSS deviation %1% exceeds ±20% tolerance (target=%2, actual=%3)")
                                        .arg(resp.tssDeviation * 100.0, 0, 'f', 1)
                                        .arg(ctx.targetTSS, 0, 'f', 0)
                                        .arg(resp.actualTSS, 0, 'f', 0);
            resp.valid = false;
        }
    }

    // Check power bounds
    for (int i = 0; i < draft.steps.size(); i++) {
        const WorkoutDraftStep &step = draft.steps[i];
        double pctFtp;
        if (step.powerMode == WorkoutDraftPowerMode::PercentFtp) {
            pctFtp = step.powerValue;
        } else if (ctx.snapshot.ftp > 0) {
            pctFtp = step.powerValue / ctx.snapshot.ftp * 100.0;
        } else {
            continue;
        }

        if (pctFtp > 200.0) {
            resp.validationErrors << QStringLiteral("Step %1 power %2% FTP exceeds 200% safety limit")
                                        .arg(i + 1).arg(pctFtp, 0, 'f', 0);
            resp.valid = false;
        }
        if (pctFtp < 0.0) {
            resp.validationErrors << QStringLiteral("Step %1 has negative power").arg(i + 1);
            resp.valid = false;
        }
    }

    // Check total duration
    int totalSec = 0;
    for (const WorkoutDraftStep &step : draft.steps) totalSec += step.durationSec;
    int targetSec = ctx.targetDurationMin * 60;
    if (targetSec > 0 && std::abs(totalSec - targetSec) > targetSec * 0.15) {
        resp.validationErrors << QStringLiteral("Duration %1 min differs from target %2 min by >15%")
                                    .arg(totalSec / 60).arg(ctx.targetDurationMin);
        // Duration mismatch is a warning, not a hard failure
    }

    return resp;
}

// ---------------------------------------------------------------------------
// Safety clamping
// ---------------------------------------------------------------------------

WorkoutDraft
LLMWorkoutBridge::clampToSafety(const WorkoutDraft &draft,
                                const TrainingConstraintBounds &bounds,
                                int ftp)
{
    Q_UNUSED(bounds);
    WorkoutDraft clamped = draft;
    static constexpr double kMaxPctFtp = 200.0;

    for (WorkoutDraftStep &step : clamped.steps) {
        if (step.powerMode == WorkoutDraftPowerMode::PercentFtp) {
            step.powerValue = std::min(step.powerValue, kMaxPctFtp);
            step.powerValue = std::max(step.powerValue, 0.0);
        } else if (ftp > 0) {
            double maxWatts = ftp * kMaxPctFtp / 100.0;
            step.powerValue = std::min(step.powerValue, maxWatts);
            step.powerValue = std::max(step.powerValue, 0.0);
        }
    }

    return clamped;
}
