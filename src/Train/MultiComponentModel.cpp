/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "MultiComponentModel.h"

#include <QJsonArray>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// DecomposedImpulse
// ---------------------------------------------------------------------------

QJsonObject DecomposedImpulse::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("aerobic")] = aerobic;
    o[QStringLiteral("glycolytic")] = glycolytic;
    o[QStringLiteral("neuromuscular")] = neuromuscular;
    o[QStringLiteral("total")] = total;
    return o;
}

// ---------------------------------------------------------------------------
// ComponentParams
// ---------------------------------------------------------------------------

QJsonObject ComponentParams::toJson() const
{
    QJsonObject o;
    QString sysName;
    switch (system) {
    case EnergySystem::Aerobic: sysName = QStringLiteral("aerobic"); break;
    case EnergySystem::Glycolytic: sysName = QStringLiteral("glycolytic"); break;
    case EnergySystem::Neuromuscular: sysName = QStringLiteral("neuromuscular"); break;
    }
    o[QStringLiteral("system")] = sysName;
    o[QStringLiteral("k1")] = k1;
    o[QStringLiteral("k2")] = k2;
    o[QStringLiteral("t1")] = t1;
    o[QStringLiteral("t2")] = t2;
    return o;
}

// ---------------------------------------------------------------------------
// MultiComponentProjection
// ---------------------------------------------------------------------------

QJsonObject MultiComponentProjection::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("peakComposite")] = peakComposite;
    o[QStringLiteral("peakDay")] = peakDay;
    o[QStringLiteral("days")] = compositePerf.size();
    if (!compositePerf.isEmpty()) {
        o[QStringLiteral("compositeStart")] = compositePerf.first();
        o[QStringLiteral("compositeEnd")] = compositePerf.last();
    }
    return o;
}

// ---------------------------------------------------------------------------
// Decomposition
// ---------------------------------------------------------------------------

DecomposedImpulse
MultiComponentModel::decompose(const QVector<double> &timeInZone,
                               double totalTSS,
                               int ftp)
{
    Q_UNUSED(ftp);
    DecomposedImpulse imp;
    imp.total = totalTSS;

    if (timeInZone.isEmpty() || totalTSS <= 0) {
        imp.aerobic = totalTSS;
        return imp;
    }

    // Map GC zones to energy systems:
    //   Zones 1-2 (Recovery, Endurance) => Aerobic
    //   Zones 3-4 (Tempo, Threshold) => Glycolytic
    //   Zones 5+ (VO2max, Anaerobic, Neuromuscular) => Neuromuscular
    double totalTime = 0;
    for (double t : timeInZone) totalTime += t;
    if (totalTime <= 0) {
        imp.aerobic = totalTSS;
        return imp;
    }

    double aerobicTime = 0, glycolyticTime = 0, neuroTime = 0;
    for (int z = 0; z < timeInZone.size(); z++) {
        if (z < 2) aerobicTime += timeInZone[z];
        else if (z < 4) glycolyticTime += timeInZone[z];
        else neuroTime += timeInZone[z];
    }

    // Weight by squared intensity factor for each zone
    // (higher zones produce disproportionate training stress per unit time)
    double aerobicWeight = aerobicTime / totalTime;
    double glycoWeight = (glycolyticTime / totalTime) * 1.5;   // higher weight per unit time
    double neuroWeight = (neuroTime / totalTime) * 2.5;        // much higher weight
    double sumWeight = aerobicWeight + glycoWeight + neuroWeight;
    if (sumWeight > 0) {
        imp.aerobic = totalTSS * aerobicWeight / sumWeight;
        imp.glycolytic = totalTSS * glycoWeight / sumWeight;
        imp.neuromuscular = totalTSS * neuroWeight / sumWeight;
    } else {
        imp.aerobic = totalTSS;
    }

    return imp;
}

DecomposedImpulse
MultiComponentModel::decomposeByIF(double tss, double intensityFactor)
{
    DecomposedImpulse imp;
    imp.total = tss;

    if (tss <= 0) return imp;

    // Heuristic split based on IF
    if (intensityFactor < 0.75) {
        imp.aerobic = tss * 0.90;
        imp.glycolytic = tss * 0.08;
        imp.neuromuscular = tss * 0.02;
    } else if (intensityFactor < 0.90) {
        imp.aerobic = tss * 0.50;
        imp.glycolytic = tss * 0.40;
        imp.neuromuscular = tss * 0.10;
    } else if (intensityFactor < 1.05) {
        imp.aerobic = tss * 0.25;
        imp.glycolytic = tss * 0.55;
        imp.neuromuscular = tss * 0.20;
    } else {
        imp.aerobic = tss * 0.10;
        imp.glycolytic = tss * 0.35;
        imp.neuromuscular = tss * 0.55;
    }

    return imp;
}

// ---------------------------------------------------------------------------
// Default parameters
// ---------------------------------------------------------------------------

QVector<ComponentParams>
MultiComponentModel::defaultParams()
{
    QVector<ComponentParams> p(3);

    // Aerobic: slow to develop, slow to decay
    p[0].system = EnergySystem::Aerobic;
    p[0].k1 = 0.15;
    p[0].k2 = 0.10;
    p[0].t1 = 60.0;
    p[0].t2 = 15.0;

    // Glycolytic: moderate development and decay
    p[1].system = EnergySystem::Glycolytic;
    p[1].k1 = 0.20;
    p[1].k2 = 0.25;
    p[1].t1 = 45.0;
    p[1].t2 = 10.0;

    // Neuromuscular: fast to develop, fast to decay
    p[2].system = EnergySystem::Neuromuscular;
    p[2].k1 = 0.30;
    p[2].k2 = 0.35;
    p[2].t1 = 30.0;
    p[2].t2 = 7.0;

    return p;
}

// ---------------------------------------------------------------------------
// Forward projection
// ---------------------------------------------------------------------------

MultiComponentProjection
MultiComponentModel::projectForward(const QVector<ComponentParams> &params,
                                    const QVector<DecomposedImpulse> &dailyImpulse,
                                    const QVector<double> &weights)
{
    MultiComponentProjection proj;
    int n = dailyImpulse.size();
    if (n == 0 || params.size() != 3) return proj;

    proj.aerobicPerf.resize(n);
    proj.glycolyticPerf.resize(n);
    proj.neuromuscularPerf.resize(n);
    proj.compositePerf.resize(n);

    double g[3], h[3], decayG[3], decayH[3];
    for (int c = 0; c < 3; c++) {
        g[c] = params[c].g;
        h[c] = params[c].h;
        decayG[c] = std::exp(-1.0 / params[c].t1);
        decayH[c] = std::exp(-1.0 / params[c].t2);
    }

    // Default weights if not provided
    double w[3] = {0.5, 0.35, 0.15};
    for (int c = 0; c < std::min(3, static_cast<int>(weights.size())); c++)
        w[c] = weights[c];

    double peakComposite = -1e99;
    int peakDay = 0;

    for (int i = 0; i < n; i++) {
        double impulse[3] = {
            dailyImpulse[i].aerobic,
            dailyImpulse[i].glycolytic,
            dailyImpulse[i].neuromuscular
        };

        double compPerf[3];
        for (int c = 0; c < 3; c++) {
            g[c] = g[c] * decayG[c] + impulse[c];
            h[c] = h[c] * decayH[c] + impulse[c];
            compPerf[c] = params[c].k1 * g[c] - params[c].k2 * h[c];
        }

        proj.aerobicPerf[i] = compPerf[0];
        proj.glycolyticPerf[i] = compPerf[1];
        proj.neuromuscularPerf[i] = compPerf[2];
        proj.compositePerf[i] = w[0] * compPerf[0] + w[1] * compPerf[1] + w[2] * compPerf[2];

        if (proj.compositePerf[i] > peakComposite) {
            peakComposite = proj.compositePerf[i];
            peakDay = i;
        }
    }

    proj.peakComposite = peakComposite;
    proj.peakDay = peakDay;

    return proj;
}
