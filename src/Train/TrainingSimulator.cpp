/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "TrainingSimulator.h"

#include <QJsonArray>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// TrainingGoal string conversion
// ---------------------------------------------------------------------------

QString trainingGoalToString(TrainingGoal g)
{
    switch (g) {
    case TrainingGoal::Build:    return QStringLiteral("build");
    case TrainingGoal::Maintain: return QStringLiteral("maintain");
    case TrainingGoal::Taper:    return QStringLiteral("taper");
    case TrainingGoal::Recover:  return QStringLiteral("recover");
    }
    return QStringLiteral("build");
}

TrainingGoal trainingGoalFromString(const QString &s)
{
    QString lower = s.toLower().trimmed();
    if (lower == QLatin1String("maintain")) return TrainingGoal::Maintain;
    if (lower == QLatin1String("taper"))    return TrainingGoal::Taper;
    if (lower == QLatin1String("recover"))  return TrainingGoal::Recover;
    return TrainingGoal::Build;
}

// ---------------------------------------------------------------------------
// SimulationResult / SimulationRanking JSON
// ---------------------------------------------------------------------------

QJsonObject SimulationResult::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("workoutType")] = workoutType;
    o[QStringLiteral("estimatedTSS")] = estimatedTSS;
    o[QStringLiteral("score")] = score;
    o[QStringLiteral("feasible")] = feasible;

    if (!projCtl.isEmpty()) {
        QJsonObject proj;
        auto last = [](const QVector<double> &v) { return v.isEmpty() ? 0.0 : v.last(); };
        proj[QStringLiteral("ctlEnd")] = last(projCtl);
        proj[QStringLiteral("atlEnd")] = last(projAtl);
        proj[QStringLiteral("tsbEnd")] = last(projTsb);
        proj[QStringLiteral("tsbMin")] = *std::min_element(projTsb.begin(), projTsb.end());
        proj[QStringLiteral("tsbMax")] = *std::max_element(projTsb.begin(), projTsb.end());
        o[QStringLiteral("projection")] = proj;
    }

    if (!constraints.violations.isEmpty()) {
        o[QStringLiteral("constraints")] = constraints.toJson();
    }

    return o;
}

QJsonObject SimulationRanking::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("goal")] = trainingGoalToString(goal);
    QJsonArray arr;
    for (const SimulationResult &r : candidates)
        arr.append(r.toJson());
    o[QStringLiteral("candidates")] = arr;
    return o;
}

// ---------------------------------------------------------------------------
// TSS estimation per workout type
// ---------------------------------------------------------------------------

namespace {

// Typical Intensity Factor for each workout type.
// IF = NP / FTP; TSS = (duration_sec * NP * IF) / (FTP * 3600)
// Simplified: TSS = (durationHours) * IF^2 * 100
struct WorkoutProfile {
    const char *type;
    double typicalIF;
};

static const WorkoutProfile kProfiles[] = {
    { "recovery",   0.55 },
    { "endurance",  0.70 },
    { "tempo",      0.80 },
    { "sweetspot",  0.90 },
    { "threshold",  1.00 },
    { "vo2max",     1.05 },  // High IF but shorter effective duration; we account for rest
    { "anaerobic",  0.95 },  // Mixed — bursts are >1.5 IF but lots of rest
};
static constexpr int kProfileCount = sizeof(kProfiles) / sizeof(kProfiles[0]);

double typicalIF(const QString &type)
{
    for (int i = 0; i < kProfileCount; i++) {
        if (type == QLatin1String(kProfiles[i].type))
            return kProfiles[i].typicalIF;
    }
    return 0.70; // default to endurance
}

} // namespace

QStringList TrainingSimulator::workoutTypes()
{
    QStringList list;
    for (int i = 0; i < kProfileCount; i++)
        list << QString::fromLatin1(kProfiles[i].type);
    return list;
}

double
TrainingSimulator::estimateTSS(const WorkoutAthleteSnapshot &snapshot,
                               const QString &workoutType,
                               int durationMin)
{
    Q_UNUSED(snapshot);
    double ifactor = typicalIF(workoutType);
    double hours = durationMin / 60.0;
    // TSS = durationHours * IF^2 * 100
    return hours * ifactor * ifactor * 100.0;
}

// ---------------------------------------------------------------------------
// Forward simulation
// ---------------------------------------------------------------------------

SimulationResult
TrainingSimulator::simulateCandidate(const WorkoutAthleteSnapshot &snapshot,
                                     const QString &workoutType,
                                     int durationMin,
                                     int projectionDays,
                                     const TrainingConstraintBounds &bounds)
{
    SimulationResult result;
    result.workoutType = workoutType;
    result.estimatedTSS = estimateTSS(snapshot, workoutType, durationMin);

    // Build daily stress sequence: [today's workout, then 0 for remaining days]
    int totalDays = 1 + projectionDays;
    QVector<double> dailyStress(totalDays, 0.0);
    dailyStress[0] = result.estimatedTSS;

    // Include recent stress context for constraint checks
    QVector<double> fullContext;
    if (!snapshot.recentStress.isEmpty()) {
        fullContext = snapshot.recentStress;
    }
    fullContext.append(dailyStress);

    QDate contextStart = snapshot.date.addDays(-snapshot.recentStress.size());
    double contextCtl = snapshot.ctl;
    double contextAtl = snapshot.atl;
    // Walk back CTL/ATL to the context start using inverse EWMA approximation
    if (!snapshot.recentStress.isEmpty()) {
        // Recompute from 7 days earlier — use rough prior estimates
        double ltsDecay = 2.0 / (42 + 1);
        double stsDecay = 2.0 / (7 + 1);
        int n = snapshot.recentStress.size();
        // Estimate prior state by un-doing the EWMA for n days (rough approx)
        contextCtl = snapshot.ctl;
        contextAtl = snapshot.atl;
        for (int i = n - 1; i >= 0; i--) {
            double s = snapshot.recentStress[i];
            contextCtl = (contextCtl - s * ltsDecay) / (1.0 - ltsDecay);
            contextAtl = (contextAtl - s * stsDecay) / (1.0 - stsDecay);
        }
        // Clamp to non-negative
        contextCtl = std::max(0.0, contextCtl);
        contextAtl = std::max(0.0, contextAtl);
    }

    // Check constraints on the full context
    result.constraints = TrainingConstraintChecker::checkAll(
        fullContext, contextStart, contextCtl, contextAtl, bounds);
    result.feasible = result.constraints.passed;

    // Project from today forward (for scoring)
    TrainingConstraintChecker::projectPMC(dailyStress, snapshot.ctl, snapshot.atl,
                                          42, 7,
                                          result.projCtl, result.projAtl, result.projTsb);

    return result;
}

// ---------------------------------------------------------------------------
// Scoring
// ---------------------------------------------------------------------------

double
TrainingSimulator::scoreForGoal(const SimulationResult &result,
                                const WorkoutAthleteSnapshot &snapshot,
                                TrainingGoal goal)
{
    if (result.projCtl.isEmpty()) return 0.0;

    double ctlEnd = result.projCtl.last();
    double tsbEnd = result.projTsb.last();
    double ctlDelta = ctlEnd - snapshot.ctl;

    switch (goal) {
    case TrainingGoal::Build:
        // Maximize CTL gain while staying feasible
        return ctlDelta;

    case TrainingGoal::Maintain:
        // Minimize absolute CTL change — closest to zero is best
        return -std::abs(ctlDelta);

    case TrainingGoal::Taper:
        // Maximize TSB (freshness) at end of projection while not losing too much CTL
        return tsbEnd - 0.5 * std::abs(ctlDelta);

    case TrainingGoal::Recover:
        // Maximize TSB recovery
        return tsbEnd;
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
// Rank all candidates
// ---------------------------------------------------------------------------

SimulationRanking
TrainingSimulator::rankCandidates(const WorkoutAthleteSnapshot &snapshot,
                                  TrainingGoal goal,
                                  int durationMin,
                                  int projectionDays,
                                  const TrainingConstraintBounds &bounds)
{
    SimulationRanking ranking;
    ranking.goal = goal;

    QStringList types = workoutTypes();
    for (const QString &type : types) {
        SimulationResult r = simulateCandidate(snapshot, type, durationMin, projectionDays, bounds);
        r.score = scoreForGoal(r, snapshot, goal);
        ranking.candidates.append(r);
    }

    // Sort: feasible first, then by descending score
    std::sort(ranking.candidates.begin(), ranking.candidates.end(),
              [](const SimulationResult &a, const SimulationResult &b) {
                  if (a.feasible != b.feasible) return a.feasible > b.feasible;
                  return a.score > b.score;
              });

    return ranking;
}
