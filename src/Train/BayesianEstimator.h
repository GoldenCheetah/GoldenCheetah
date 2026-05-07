/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_BAYESIAN_ESTIMATOR_H
#define GC_BAYESIAN_ESTIMATOR_H

#include "BanisterSimulator.h"

#include <QJsonObject>
#include <QVector>

/**
 * Confidence interval for a Banister parameter.
 */
struct ParamDistribution {
    double mean = 0.0;
    double sd = 0.0;
    double ci95Low = 0.0;
    double ci95High = 0.0;

    QJsonObject toJson() const;
};

/**
 * Full uncertainty profile for Banister parameters.
 */
struct BanisterUncertainty {
    ParamDistribution k1;
    ParamDistribution k2;
    ParamDistribution p0;
    ParamDistribution t1;
    ParamDistribution t2;

    bool hasUncertainty = false;

    QJsonObject toJson() const;
};

/**
 * Ensemble projection with confidence bands.
 */
struct EnsembleProjection {
    QVector<double> median;
    QVector<double> ci95Low;
    QVector<double> ci95High;
    QVector<double> ci50Low;
    QVector<double> ci50High;

    int sampleCount = 0;

    QJsonObject toJson() const;
};

/**
 * Phase 3 stub: Bayesian parameter estimation for the Banister model.
 *
 * PURPOSE: Quantify uncertainty in the Banister model parameters to:
 * 1. Provide confidence intervals on performance predictions
 * 2. Distinguish between genuinely different training plans vs noise
 * 3. Gracefully degrade predictions when data is sparse
 *
 * CURRENT STATUS: Stub with bootstrap-based approximate Bayesian computation.
 * Future: full MCMC via Stan/JAGS integration (see Peng's IR-model on GitHub).
 *
 * COLD START STRATEGY:
 * - < 2 performance tests: use population priors from opendata
 * - 2-5 tests: wide credible intervals, conservative recommendations
 * - > 5 tests: model-specific parameters dominate
 *
 * REFERENCES:
 * - Peng (2023): github.com/KenKPeng/IR-model (R/JAGS implementation)
 * - Kolossa et al. (2017): Bayesian extension of fitness-fatigue model
 */
class BayesianEstimator {

public:

    /**
     * Estimate parameter uncertainty using bootstrap resampling.
     * This is a simplified approximate Bayesian computation (ABC).
     *
     * @param params   point-estimate parameters (from Levenberg-Marquardt)
     * @param dailyScores historical daily training load
     * @param perfDays    indices (into dailyScores) where performance tests occurred
     * @param perfValues  performance test values
     * @param nSamples    number of bootstrap samples (default 200)
     */
    static BanisterUncertainty estimateUncertainty(
        const BanisterParams &params,
        const QVector<double> &dailyScores,
        const QVector<int> &perfDays,
        const QVector<double> &perfValues,
        int nSamples = 200);

    /**
     * Project forward with parameter uncertainty.
     * Runs multiple forward simulations sampling from the parameter distribution.
     */
    static EnsembleProjection ensembleProject(
        const BanisterParams &params,
        const BanisterUncertainty &uncertainty,
        const QVector<double> &plannedTSS,
        int nSamples = 100);

    /**
     * Check if two plans produce meaningfully different outcomes.
     * Uses the ensemble projections to test if the difference is significant.
     *
     * @return probability that planA outperforms planB (0.0 to 1.0)
     */
    static double planDifferenceSignificance(
        const EnsembleProjection &projA,
        const EnsembleProjection &projB);

    /**
     * Determine appropriate confidence level based on data availability.
     * Returns a qualitative label: "high", "moderate", "low", "insufficient"
     */
    static QString dataConfidenceLevel(int performanceTests, int trainingDays);
};

#endif
