/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "TrainingConstraintChecker.h"

#include <cmath>
#include <algorithm>
#include <numeric>

// ---------------------------------------------------------------------------
// ConstraintViolation
// ---------------------------------------------------------------------------

QJsonObject
ConstraintViolation::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("constraintId")] = constraintId;
    o[QStringLiteral("severity")] = severity == Hard ? QStringLiteral("hard") : QStringLiteral("warning");
    o[QStringLiteral("message")] = message;
    if (date.isValid())
        o[QStringLiteral("date")] = date.toString(Qt::ISODate);
    o[QStringLiteral("actual")] = actual;
    o[QStringLiteral("limit")] = limit;
    return o;
}

// ---------------------------------------------------------------------------
// ConstraintCheckResult
// ---------------------------------------------------------------------------

void
ConstraintCheckResult::addViolation(const ConstraintViolation &v)
{
    violations.append(v);
    if (v.severity == ConstraintViolation::Hard)
        passed = false;
}

QJsonObject
ConstraintCheckResult::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("passed")] = passed;
    QJsonArray arr;
    for (const ConstraintViolation &v : violations)
        arr.append(v.toJson());
    o[QStringLiteral("violations")] = arr;
    return o;
}

// ---------------------------------------------------------------------------
// TrainingConstraintBounds
// ---------------------------------------------------------------------------

TrainingConstraintBounds
TrainingConstraintBounds::recreational()
{
    TrainingConstraintBounds b;
    b.maxCtlRampPerWeek = 5.0;
    b.maxMonotony = 1.5;
    b.minACWR = 0.8;
    b.maxACWR = 1.3;
    b.maxDailyTSS = 300.0;
    b.tsbFloor = -30.0;
    b.minRestDaysPerWeek = 1;
    return b;
}

TrainingConstraintBounds
TrainingConstraintBounds::elite()
{
    TrainingConstraintBounds b;
    b.maxCtlRampPerWeek = 7.0;
    b.maxMonotony = 2.0;
    b.minACWR = 0.8;
    b.maxACWR = 1.5;
    b.maxDailyTSS = 450.0;
    b.tsbFloor = -35.0;
    b.minRestDaysPerWeek = 1;
    return b;
}

// ---------------------------------------------------------------------------
// PMC projection math (same EWMA as GoldenCheetah's PMCData)
// ---------------------------------------------------------------------------

void
TrainingConstraintChecker::projectPMC(const QVector<double> &dailyStress,
                                      double priorCtl, double priorAtl,
                                      int ltsDays, int stsDays,
                                      QVector<double> &outCtl, QVector<double> &outAtl,
                                      QVector<double> &outTsb)
{
    if (ltsDays <= 0) ltsDays = 42;
    if (stsDays <= 0) stsDays = 7;

    const double ltsDecay = 2.0 / (ltsDays + 1);
    const double stsDecay = 2.0 / (stsDays + 1);
    const int n = dailyStress.size();

    outCtl.resize(n);
    outAtl.resize(n);
    outTsb.resize(n);

    double ctl = priorCtl;
    double atl = priorAtl;

    for (int i = 0; i < n; i++) {
        double s = dailyStress[i];
        ctl = ctl + (s - ctl) * ltsDecay;
        atl = atl + (s - atl) * stsDecay;
        outCtl[i] = ctl;
        outAtl[i] = atl;
        outTsb[i] = ctl - atl;
    }
}

// ---------------------------------------------------------------------------
// Individual constraint calculations
// ---------------------------------------------------------------------------

double
TrainingConstraintChecker::monotony(const QVector<double> &dailyStress, int windowStart, int windowSize)
{
    if (windowSize <= 1) return 0.0;

    int end = std::min(windowStart + windowSize, static_cast<int>(dailyStress.size()));
    int count = end - windowStart;
    if (count <= 1) return 0.0;

    double sum = 0.0;
    for (int i = windowStart; i < end; i++)
        sum += dailyStress[i];
    double mean = sum / count;

    double sumSq = 0.0;
    for (int i = windowStart; i < end; i++)
        sumSq += (dailyStress[i] - mean) * (dailyStress[i] - mean);
    double sd = std::sqrt(sumSq / count);

    return sd > 0.0 ? mean / sd : 0.0;
}

double
TrainingConstraintChecker::acwr(double atl, double ctl)
{
    return ctl > 0.0 ? atl / ctl : 0.0;
}

double
TrainingConstraintChecker::ctlRampRate(double ctlStart, double ctlEnd, int days)
{
    if (days <= 0) return 0.0;
    return (ctlEnd - ctlStart) / (days / 7.0);
}

// ---------------------------------------------------------------------------
// Full constraint check
// ---------------------------------------------------------------------------

ConstraintCheckResult
TrainingConstraintChecker::checkAll(const QVector<double> &dailyStress,
                                    const QDate &startDate,
                                    double priorCtl, double priorAtl,
                                    const TrainingConstraintBounds &bounds,
                                    int checkFromDay)
{
    ConstraintCheckResult result;
    const int n = dailyStress.size();
    if (n == 0) return result;

    // Clamp to valid range
    const int from = std::max(0, std::min(checkFromDay, n));

    // Project PMC forward (over the full sequence for accurate state)
    QVector<double> projCtl, projAtl, projTsb;
    projectPMC(dailyStress, priorCtl, priorAtl, 42, 7, projCtl, projAtl, projTsb);

    // 1. Max daily TSS
    for (int i = from; i < n; i++) {
        if (dailyStress[i] > bounds.maxDailyTSS) {
            ConstraintViolation v;
            v.constraintId = QStringLiteral("maxDailyTSS");
            v.severity = ConstraintViolation::Hard;
            v.date = startDate.addDays(i);
            v.actual = dailyStress[i];
            v.limit = bounds.maxDailyTSS;
            v.message = QStringLiteral("Daily TSS %1 exceeds maximum %2")
                            .arg(dailyStress[i], 0, 'f', 0)
                            .arg(bounds.maxDailyTSS, 0, 'f', 0);
            result.addViolation(v);
        }
    }

    // 2. TSB floor
    for (int i = from; i < n; i++) {
        if (projTsb[i] < bounds.tsbFloor) {
            ConstraintViolation v;
            v.constraintId = QStringLiteral("tsbFloor");
            v.severity = ConstraintViolation::Hard;
            v.date = startDate.addDays(i);
            v.actual = projTsb[i];
            v.limit = bounds.tsbFloor;
            v.message = QStringLiteral("TSB %1 drops below floor %2")
                            .arg(projTsb[i], 0, 'f', 1)
                            .arg(bounds.tsbFloor, 0, 'f', 1);
            result.addViolation(v);
        }
    }

    // 3. ACWR (check each day)
    // ACWR is only meaningful when CTL is high enough for the ratio to be
    // reliable. With CTL < 30, even a moderate workout produces an extreme
    // ratio because the denominator is so small. Below that threshold,
    // all ACWR violations are warnings — absolute load checks (TSB floor,
    // daily TSS cap) remain the primary safety gates.
    static constexpr double kACWRHardCtlThreshold = 30.0;
    for (int i = from; i < n; i++) {
        double a = acwr(projAtl[i], projCtl[i]);
        if (projCtl[i] > 5.0 && (a < bounds.minACWR || a > bounds.maxACWR)) {
            ConstraintViolation v;
            v.constraintId = QStringLiteral("acwr");
            v.severity = (a > 1.5 && projCtl[i] >= kACWRHardCtlThreshold)
                             ? ConstraintViolation::Hard
                             : ConstraintViolation::Warning;
            v.date = startDate.addDays(i);
            v.actual = a;
            v.limit = (a > bounds.maxACWR) ? bounds.maxACWR : bounds.minACWR;
            v.message = QStringLiteral("ACWR %1 outside safe range [%2, %3]")
                            .arg(a, 0, 'f', 2)
                            .arg(bounds.minACWR, 0, 'f', 1)
                            .arg(bounds.maxACWR, 0, 'f', 1);
            result.addViolation(v);
        }
    }

    // 4. CTL ramp rate (check week-by-week)
    for (int week = 0; week * 7 < n; week++) {
        int dayStart = week * 7;
        if (dayStart + 6 < from) continue; // entire week is historical
        int dayEnd = std::min(dayStart + 7, n) - 1;
        double ctlAtStart = (dayStart == 0) ? priorCtl : projCtl[dayStart - 1];
        double ctlAtEnd = projCtl[dayEnd];
        double ramp = ctlAtEnd - ctlAtStart;
        if (ramp > bounds.maxCtlRampPerWeek) {
            ConstraintViolation v;
            v.constraintId = QStringLiteral("ctlRamp");
            v.severity = ConstraintViolation::Hard;
            v.date = startDate.addDays(dayStart);
            v.actual = ramp;
            v.limit = bounds.maxCtlRampPerWeek;
            v.message = QStringLiteral("CTL ramp %1 TSS/week exceeds maximum %2")
                            .arg(ramp, 0, 'f', 1)
                            .arg(bounds.maxCtlRampPerWeek, 0, 'f', 1);
            result.addViolation(v);
        }
    }

    // 5. Training monotony (7-day rolling windows)
    for (int i = 0; i + 7 <= n; i++) {
        if (i + 6 < from) continue; // window ends before check range
        double m = monotony(dailyStress, i, 7);
        if (m > bounds.maxMonotony) {
            ConstraintViolation v;
            v.constraintId = QStringLiteral("monotony");
            v.severity = ConstraintViolation::Warning;
            v.date = startDate.addDays(i);
            v.actual = m;
            v.limit = bounds.maxMonotony;
            v.message = QStringLiteral("Training monotony %1 exceeds limit %2")
                            .arg(m, 0, 'f', 2)
                            .arg(bounds.maxMonotony, 0, 'f', 2);
            result.addViolation(v);
        }
    }

    // 6. Minimum rest days per week (TSS < 20 counts as rest)
    static constexpr double kRestThreshold = 20.0;
    for (int week = 0; week * 7 < n; week++) {
        int dayStart = week * 7;
        if (dayStart + 6 < from) continue; // entire week is historical
        int dayEnd = std::min(dayStart + 7, n);
        int restDays = 0;
        for (int i = dayStart; i < dayEnd; i++) {
            if (dailyStress[i] < kRestThreshold)
                restDays++;
        }
        if ((dayEnd - dayStart) >= 7 && restDays < bounds.minRestDaysPerWeek) {
            ConstraintViolation v;
            v.constraintId = QStringLiteral("restDays");
            v.severity = ConstraintViolation::Hard;
            v.date = startDate.addDays(dayStart);
            v.actual = restDays;
            v.limit = bounds.minRestDaysPerWeek;
            v.message = QStringLiteral("Only %1 rest day(s) in week starting %2 (minimum %3)")
                            .arg(restDays)
                            .arg(startDate.addDays(dayStart).toString(Qt::ISODate))
                            .arg(bounds.minRestDaysPerWeek);
            result.addViolation(v);
        }
    }

    return result;
}
