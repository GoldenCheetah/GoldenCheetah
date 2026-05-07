/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "BayesianEstimator.h"

#include <QJsonArray>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>
#include <numeric>

// ---------------------------------------------------------------------------
// ParamDistribution
// ---------------------------------------------------------------------------

QJsonObject ParamDistribution::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("mean")] = mean;
    o[QStringLiteral("sd")] = sd;
    o[QStringLiteral("ci95Low")] = ci95Low;
    o[QStringLiteral("ci95High")] = ci95High;
    return o;
}

// ---------------------------------------------------------------------------
// BanisterUncertainty
// ---------------------------------------------------------------------------

QJsonObject BanisterUncertainty::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("hasUncertainty")] = hasUncertainty;
    o[QStringLiteral("k1")] = k1.toJson();
    o[QStringLiteral("k2")] = k2.toJson();
    o[QStringLiteral("p0")] = p0.toJson();
    o[QStringLiteral("t1")] = t1.toJson();
    o[QStringLiteral("t2")] = t2.toJson();
    return o;
}

// ---------------------------------------------------------------------------
// EnsembleProjection
// ---------------------------------------------------------------------------

QJsonObject EnsembleProjection::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("sampleCount")] = sampleCount;
    o[QStringLiteral("days")] = median.size();

    if (!median.isEmpty()) {
        o[QStringLiteral("medianEnd")] = median.last();
        o[QStringLiteral("ci95LowEnd")] = ci95Low.last();
        o[QStringLiteral("ci95HighEnd")] = ci95High.last();
    }

    return o;
}

// ---------------------------------------------------------------------------
// Helper: compute statistics over a vector
// ---------------------------------------------------------------------------

namespace {

struct VectorStats {
    double mean, sd, p2_5, p25, p50, p75, p97_5;
};

VectorStats computeStats(QVector<double> values)
{
    VectorStats s = {0, 0, 0, 0, 0, 0, 0};
    if (values.isEmpty()) return s;

    std::sort(values.begin(), values.end());
    int n = values.size();

    s.mean = std::accumulate(values.begin(), values.end(), 0.0) / n;
    double sumSq = 0;
    for (double v : values) sumSq += (v - s.mean) * (v - s.mean);
    s.sd = std::sqrt(sumSq / n);

    auto pctile = [&](double p) -> double {
        double idx = p * (n - 1);
        int lo = static_cast<int>(idx);
        int hi = std::min(lo + 1, n - 1);
        double frac = idx - lo;
        return values[lo] * (1.0 - frac) + values[hi] * frac;
    };

    s.p2_5 = pctile(0.025);
    s.p25 = pctile(0.25);
    s.p50 = pctile(0.50);
    s.p75 = pctile(0.75);
    s.p97_5 = pctile(0.975);

    return s;
}

ParamDistribution makeDistribution(const QVector<double> &samples)
{
    VectorStats s = computeStats(QVector<double>(samples));
    ParamDistribution d;
    d.mean = s.mean;
    d.sd = s.sd;
    d.ci95Low = s.p2_5;
    d.ci95High = s.p97_5;
    return d;
}

} // namespace

// ---------------------------------------------------------------------------
// Bootstrap uncertainty estimation
// ---------------------------------------------------------------------------

BanisterUncertainty
BayesianEstimator::estimateUncertainty(const BanisterParams &params,
                                       const QVector<double> &dailyScores,
                                       const QVector<int> &perfDays,
                                       const QVector<double> &perfValues,
                                       int nSamples)
{
    BanisterUncertainty unc;

    if (perfDays.size() < 2 || perfValues.size() < 2) {
        // Insufficient data: return wide priors
        unc.k1 = {params.k1, params.k1 * 0.5, params.k1 * 0.1, params.k1 * 2.0};
        unc.k2 = {params.k2, params.k2 * 0.5, params.k2 * 0.1, params.k2 * 2.0};
        unc.p0 = {params.p0, 10.0, params.p0 - 20.0, params.p0 + 20.0};
        unc.t1 = {params.t1, 15.0, 25.0, 80.0};
        unc.t2 = {params.t2, 5.0, 5.0, 20.0};
        unc.hasUncertainty = false;
        return unc;
    }

    // Bootstrap: resample performance test points with replacement
    QRandomGenerator *rng = QRandomGenerator::global();
    int nTests = perfDays.size();

    QVector<double> k1Samples, k2Samples, p0Samples;
    k1Samples.reserve(nSamples);
    k2Samples.reserve(nSamples);
    p0Samples.reserve(nSamples);

    for (int s = 0; s < nSamples; s++) {
        // Resample with replacement
        QVector<int> sampledDays(nTests);
        QVector<double> sampledValues(nTests);
        for (int i = 0; i < nTests; i++) {
            int idx = rng->bounded(nTests);
            sampledDays[i] = perfDays[idx];
            sampledValues[i] = perfValues[idx];
        }

        // Perturb parameters within reasonable bounds
        double sk1 = params.k1 * (1.0 + 0.2 * (rng->generateDouble() - 0.5));
        double sk2 = params.k2 * (1.0 + 0.2 * (rng->generateDouble() - 0.5));
        double sp0 = params.p0 + 5.0 * (rng->generateDouble() - 0.5);

        // Simulate and compute residuals
        double g = 0, h = 0;
        double totalResid = 0;
        int residCount = 0;
        double decayG = std::exp(-1.0 / params.t1);
        double decayH = std::exp(-1.0 / params.t2);

        for (int day = 0; day < dailyScores.size(); day++) {
            g = g * decayG + dailyScores[day];
            h = h * decayH + dailyScores[day];

            // Check if this is a sampled test day
            for (int t = 0; t < nTests; t++) {
                if (sampledDays[t] == day) {
                    double pred = sp0 + sk1 * g - sk2 * h;
                    totalResid += (pred - sampledValues[t]) * (pred - sampledValues[t]);
                    residCount++;
                }
            }
        }

        // Accept sample (simplified ABC — always accept, weight by residual later)
        k1Samples.append(sk1);
        k2Samples.append(sk2);
        p0Samples.append(sp0);
    }

    unc.k1 = makeDistribution(k1Samples);
    unc.k2 = makeDistribution(k2Samples);
    unc.p0 = makeDistribution(p0Samples);
    unc.t1 = {params.t1, 15.0, 25.0, 80.0};  // t1/t2 not fitted yet — use wide priors
    unc.t2 = {params.t2, 5.0, 5.0, 20.0};
    unc.hasUncertainty = true;

    return unc;
}

// ---------------------------------------------------------------------------
// Ensemble projection
// ---------------------------------------------------------------------------

EnsembleProjection
BayesianEstimator::ensembleProject(const BanisterParams &params,
                                   const BanisterUncertainty &uncertainty,
                                   const QVector<double> &plannedTSS,
                                   int nSamples)
{
    EnsembleProjection proj;
    int n = plannedTSS.size();
    if (n == 0) return proj;

    QRandomGenerator *rng = QRandomGenerator::global();

    // Storage for all sample trajectories
    QVector<QVector<double>> allTrajectories(nSamples);

    for (int s = 0; s < nSamples; s++) {
        // Sample parameters from distributions
        BanisterParams sampledParams = params;
        if (uncertainty.hasUncertainty) {
            sampledParams.k1 = std::max(0.0, uncertainty.k1.mean + uncertainty.k1.sd * (rng->generateDouble() * 2.0 - 1.0));
            sampledParams.k2 = std::max(0.0, uncertainty.k2.mean + uncertainty.k2.sd * (rng->generateDouble() * 2.0 - 1.0));
            sampledParams.p0 = uncertainty.p0.mean + uncertainty.p0.sd * (rng->generateDouble() * 2.0 - 1.0);
        }

        BanisterProjection bp = BanisterSimulator::projectForward(sampledParams, plannedTSS, QDate());
        allTrajectories[s] = bp.dailyPerf;
    }

    // Compute percentiles across samples for each day
    proj.median.resize(n);
    proj.ci95Low.resize(n);
    proj.ci95High.resize(n);
    proj.ci50Low.resize(n);
    proj.ci50High.resize(n);

    for (int d = 0; d < n; d++) {
        QVector<double> dayValues;
        dayValues.reserve(nSamples);
        for (int s = 0; s < nSamples; s++) {
            if (d < allTrajectories[s].size())
                dayValues.append(allTrajectories[s][d]);
        }
        VectorStats stats = computeStats(dayValues);
        proj.median[d] = stats.p50;
        proj.ci95Low[d] = stats.p2_5;
        proj.ci95High[d] = stats.p97_5;
        proj.ci50Low[d] = stats.p25;
        proj.ci50High[d] = stats.p75;
    }

    proj.sampleCount = nSamples;
    return proj;
}

// ---------------------------------------------------------------------------
// Plan difference significance
// ---------------------------------------------------------------------------

double
BayesianEstimator::planDifferenceSignificance(const EnsembleProjection &projA,
                                              const EnsembleProjection &projB)
{
    if (projA.median.isEmpty() || projB.median.isEmpty()) return 0.5;

    // Use the endpoint performance as the comparison metric
    int n = std::min(projA.median.size(), projB.median.size());
    int idx = n - 1;

    // Approximate: compare the distributions at the endpoint
    double medianA = projA.median[idx];
    double medianB = projB.median[idx];
    double sdA = (projA.ci95High[idx] - projA.ci95Low[idx]) / 3.92; // 95% CI => ~2*1.96*sd
    double sdB = (projB.ci95High[idx] - projB.ci95Low[idx]) / 3.92;

    // Combined standard deviation for the difference
    double sdDiff = std::sqrt(sdA * sdA + sdB * sdB);
    if (sdDiff < 1e-10) return medianA > medianB ? 1.0 : 0.0;

    // Z-score for the difference
    double z = (medianA - medianB) / sdDiff;

    // Approximate normal CDF using logistic approximation
    double p = 1.0 / (1.0 + std::exp(-1.7 * z));
    return std::max(0.0, std::min(1.0, p));
}

// ---------------------------------------------------------------------------
// Data confidence level
// ---------------------------------------------------------------------------

QString
BayesianEstimator::dataConfidenceLevel(int performanceTests, int trainingDays)
{
    if (performanceTests < 2 || trainingDays < 30)
        return QStringLiteral("insufficient");
    if (performanceTests < 5 || trainingDays < 90)
        return QStringLiteral("low");
    if (performanceTests < 10 || trainingDays < 180)
        return QStringLiteral("moderate");
    return QStringLiteral("high");
}
