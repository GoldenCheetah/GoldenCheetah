/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_BANISTER_SIMULATOR_H
#define GC_BANISTER_SIMULATOR_H

#include "Banister.h"

#include <QDate>
#include <QJsonObject>
#include <QString>
#include <QVector>

class Athlete;

/**
 * Fitted Banister parameters extracted from a training window.
 * These are the athlete-specific values needed for forward simulation.
 */
struct BanisterParams {
    double k1 = 0.2;    // PTE coefficient
    double k2 = 0.2;    // NTE coefficient
    double p0 = 0.0;    // baseline performance
    double t1 = 50.0;   // PTE decay constant (days)
    double t2 = 11.0;   // NTE decay constant (days)

    // Current accumulated state (carried forward from historical data)
    double g = 0.0;     // current PTE accumulator (g)
    double h = 0.0;     // current NTE accumulator (h)

    bool isFitted = false;

    static BanisterParams defaultPriors();
    QJsonObject toJson() const;
};

/**
 * Forward-projected performance trajectory.
 */
struct BanisterProjection {
    QDate startDate;
    QVector<double> dailyPerf;    // predicted performance per day
    QVector<double> dailyPTE;     // PTE component per day
    QVector<double> dailyNTE;     // NTE component per day
    double peakPerf = 0.0;
    int peakDay = -1;             // day index of peak performance

    QJsonObject toJson() const;
};

/**
 * Comparison of two simulated plans.
 */
struct PlanComparison {
    QString planAName;
    QString planBName;
    BanisterProjection projA;
    BanisterProjection projB;
    double perfDelta = 0.0;       // projA.peak - projB.peak

    QJsonObject toJson() const;
};

/**
 * Banister-based forward training simulator (Phase 2).
 *
 * Uses the fitted Banister impulse-response model parameters (k1, k2, τ1, τ2)
 * to simulate predicted performance trajectories for candidate training plans.
 *
 * CRITICAL SAFETY RULE: Never optimize the linear Banister model directly.
 * It produces "train maximally every day then stop" (bang-bang solution).
 * Ceddia et al. (2025) showed it predicts 6,020-unit improvement vs 33 observed.
 * All plans must be constrained through TrainingConstraintChecker.
 *
 * The model equation:
 *   Performance(t) = p0 + k1·g(t) - k2·h(t)
 *   g(t) = g(t-1)·exp(-1/τ1) + score(t)    (slow-decaying fitness)
 *   h(t) = h(t-1)·exp(-1/τ2) + score(t)    (fast-decaying fatigue)
 */
class BanisterSimulator {

public:

    /**
     * Extract fitted Banister parameters for an athlete.
     * Uses the most recent fitting window from the athlete's Banister model.
     * Falls back to default priors if no fitted model exists.
     */
    static BanisterParams extractParams(Athlete *athlete,
                                        const QString &loadMetric = QStringLiteral("coggan_tss"),
                                        const QDate &asOf = QDate());

    /**
     * Project performance forward given a daily training load sequence.
     * @param params   fitted model parameters (with current g/h state)
     * @param dailyTSS planned daily load from day 0 onward
     * @param startDate calendar date for day 0
     */
    static BanisterProjection projectForward(const BanisterParams &params,
                                             const QVector<double> &dailyTSS,
                                             const QDate &startDate);

    /**
     * Compare two training plans by their predicted performance trajectories.
     */
    static PlanComparison comparePlans(const BanisterParams &params,
                                       const QString &nameA,
                                       const QVector<double> &planA,
                                       const QString &nameB,
                                       const QVector<double> &planB,
                                       const QDate &startDate);

    /**
     * Generate a simple heuristic plan template (daily TSS sequence).
     * These are the candidates compared by the plan ranker.
     */
    static QVector<double> buildPlanTemplate(const QString &templateName,
                                             double targetWeeklyTSS,
                                             int days);

    /** Available plan template names. */
    static QStringList planTemplateNames();
};

#endif
