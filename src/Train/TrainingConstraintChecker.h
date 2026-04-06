/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_TRAINING_CONSTRAINT_CHECKER_H
#define GC_TRAINING_CONSTRAINT_CHECKER_H

#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QVector>

/**
 * A single constraint violation detected during plan evaluation.
 * See doc/ai/constraints.md for the full reference table.
 */
struct ConstraintViolation {
    enum Severity { Warning, Hard };

    QString constraintId;
    Severity severity = Hard;
    QString message;
    QDate date;
    double actual = 0.0;
    double limit = 0.0;

    QJsonObject toJson() const;
};

/**
 * Result of checking a daily stress sequence against physiological constraints.
 */
struct ConstraintCheckResult {
    bool passed = true;
    QList<ConstraintViolation> violations;

    void addViolation(const ConstraintViolation &v);
    QJsonObject toJson() const;
};

/**
 * Non-negotiable physiological constraint bounds.
 * Conservative defaults per doc/ai/constraints.md.
 * All values may be tightened but never relaxed.
 */
struct TrainingConstraintBounds {
    double maxCtlRampPerWeek = 5.0;
    double maxMonotony = 1.5;
    double minACWR = 0.8;
    double maxACWR = 1.3;
    double maxDailyTSS = 300.0;
    double tsbFloor = -30.0;
    int minRestDaysPerWeek = 1;

    static TrainingConstraintBounds recreational();
    static TrainingConstraintBounds elite();
};

/**
 * Validates daily training stress sequences against physiological safety constraints.
 *
 * Hard constraints must override any model output or optimizer recommendation.
 * No generated training plan may violate these bounds.
 *
 * See doc/ai/constraints.md for full rationale and citations.
 */
class TrainingConstraintChecker {

public:

    /**
     * Check a complete daily stress sequence against all constraints.
     * @param dailyStress  TSS values indexed from day 0
     * @param startDate    calendar date for day 0
     * @param priorCtl     CTL on the day before startDate (for ramp calculation)
     * @param priorAtl     ATL on the day before startDate (for ACWR)
     * @param bounds       constraint thresholds to apply
     * @param checkFromDay only flag violations from this day index onward;
     *                     earlier days are used for PMC seeding but not penalized
     */
    static ConstraintCheckResult checkAll(const QVector<double> &dailyStress,
                                          const QDate &startDate,
                                          double priorCtl, double priorAtl,
                                          const TrainingConstraintBounds &bounds = TrainingConstraintBounds::recreational(),
                                          int checkFromDay = 0);

    /** Training monotony = mean(daily load) / stddev(daily load) over a 7-day window. */
    static double monotony(const QVector<double> &dailyStress, int windowStart, int windowSize = 7);

    /** Acute:Chronic Workload Ratio = 7-day ATL / 28-day CTL. */
    static double acwr(double atl, double ctl);

    /** CTL ramp rate = change in CTL per week. */
    static double ctlRampRate(double ctlStart, double ctlEnd, int days);

    /** Compute exponential weighted moving average (EWMA) forward. */
    static void projectPMC(const QVector<double> &dailyStress,
                           double priorCtl, double priorAtl,
                           int ltsDays, int stsDays,
                           QVector<double> &outCtl, QVector<double> &outAtl,
                           QVector<double> &outTsb);
};

#endif
