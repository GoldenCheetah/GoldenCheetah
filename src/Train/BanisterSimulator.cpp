/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "BanisterSimulator.h"

#include <QJsonArray>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// BanisterParams
// ---------------------------------------------------------------------------

BanisterParams BanisterParams::defaultPriors()
{
    BanisterParams p;
    p.k1 = 0.2;
    p.k2 = 0.2;
    p.p0 = 0.0;
    p.t1 = 50.0;
    p.t2 = 11.0;
    p.g = 0.0;
    p.h = 0.0;
    p.isFitted = false;
    return p;
}

QJsonObject BanisterParams::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("k1")] = k1;
    o[QStringLiteral("k2")] = k2;
    o[QStringLiteral("p0")] = p0;
    o[QStringLiteral("t1")] = t1;
    o[QStringLiteral("t2")] = t2;
    o[QStringLiteral("g")] = g;
    o[QStringLiteral("h")] = h;
    o[QStringLiteral("isFitted")] = isFitted;
    return o;
}

// ---------------------------------------------------------------------------
// BanisterProjection
// ---------------------------------------------------------------------------

QJsonObject BanisterProjection::toJson() const
{
    QJsonObject o;
    if (startDate.isValid())
        o[QStringLiteral("startDate")] = startDate.toString(Qt::ISODate);
    o[QStringLiteral("peakPerf")] = peakPerf;
    o[QStringLiteral("peakDay")] = peakDay;
    o[QStringLiteral("days")] = dailyPerf.size();

    // Only include endpoint and peak to keep JSON compact
    if (!dailyPerf.isEmpty()) {
        o[QStringLiteral("perfStart")] = dailyPerf.first();
        o[QStringLiteral("perfEnd")] = dailyPerf.last();
    }

    return o;
}

// ---------------------------------------------------------------------------
// PlanComparison
// ---------------------------------------------------------------------------

QJsonObject PlanComparison::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("planA")] = planAName;
    o[QStringLiteral("planB")] = planBName;
    o[QStringLiteral("projA")] = projA.toJson();
    o[QStringLiteral("projB")] = projB.toJson();
    o[QStringLiteral("perfDelta")] = perfDelta;
    return o;
}

// ---------------------------------------------------------------------------
// Forward projection (core math — mirrors banisterFit::compute)
// ---------------------------------------------------------------------------

BanisterProjection
BanisterSimulator::projectForward(const BanisterParams &params,
                                  const QVector<double> &dailyTSS,
                                  const QDate &startDate)
{
    BanisterProjection proj;
    proj.startDate = startDate;

    const int n = dailyTSS.size();
    if (n == 0) return proj;

    proj.dailyPerf.resize(n);
    proj.dailyPTE.resize(n);
    proj.dailyNTE.resize(n);

    double g = params.g;
    double h = params.h;
    double peakPerf = -1e99;
    int peakDay = 0;

    const double decayG = std::exp(-1.0 / params.t1);
    const double decayH = std::exp(-1.0 / params.t2);

    for (int i = 0; i < n; i++) {
        g = g * decayG + dailyTSS[i];
        h = h * decayH + dailyTSS[i];

        double pte = params.k1 * g;
        double nte = params.k2 * h;
        double perf = params.p0 + pte - nte;

        proj.dailyPTE[i] = pte;
        proj.dailyNTE[i] = nte;
        proj.dailyPerf[i] = perf;

        if (perf > peakPerf) {
            peakPerf = perf;
            peakDay = i;
        }
    }

    proj.peakPerf = peakPerf;
    proj.peakDay = peakDay;

    return proj;
}

// ---------------------------------------------------------------------------
// Plan comparison
// ---------------------------------------------------------------------------

PlanComparison
BanisterSimulator::comparePlans(const BanisterParams &params,
                                const QString &nameA,
                                const QVector<double> &planA,
                                const QString &nameB,
                                const QVector<double> &planB,
                                const QDate &startDate)
{
    PlanComparison c;
    c.planAName = nameA;
    c.planBName = nameB;
    c.projA = projectForward(params, planA, startDate);
    c.projB = projectForward(params, planB, startDate);
    c.perfDelta = c.projA.peakPerf - c.projB.peakPerf;
    return c;
}

// ---------------------------------------------------------------------------
// Plan templates
// ---------------------------------------------------------------------------

QStringList BanisterSimulator::planTemplateNames()
{
    return QStringList()
        << QStringLiteral("3-1")
        << QStringLiteral("2-1")
        << QStringLiteral("progressive")
        << QStringLiteral("polarized")
        << QStringLiteral("flat");
}

QVector<double>
BanisterSimulator::buildPlanTemplate(const QString &templateName,
                                     double targetWeeklyTSS,
                                     int days)
{
    QVector<double> plan(days, 0.0);
    if (days <= 0 || targetWeeklyTSS <= 0) return plan;

    double dailyAvg = targetWeeklyTSS / 7.0;

    if (templateName == QLatin1String("3-1")) {
        // 3 weeks build, 1 week recovery — classic periodization
        for (int d = 0; d < days; d++) {
            int weekInBlock = (d / 7) % 4;
            double weekFactor;
            switch (weekInBlock) {
            case 0: weekFactor = 0.90; break;
            case 1: weekFactor = 1.00; break;
            case 2: weekFactor = 1.10; break;
            case 3: weekFactor = 0.50; break; // recovery week
            default: weekFactor = 1.0;
            }
            int dow = d % 7;
            double dayFactor = 1.0;
            if (dow == 0) dayFactor = 1.3;       // Monday — key session
            else if (dow == 2) dayFactor = 1.4;   // Wednesday — intensity
            else if (dow == 4) dayFactor = 1.2;   // Friday — tempo
            else if (dow == 5) dayFactor = 1.5;   // Saturday — long ride
            else if (dow == 6) dayFactor = 0.0;   // Sunday — rest
            else dayFactor = 0.6;                  // easy days

            plan[d] = dailyAvg * weekFactor * dayFactor;
        }

    } else if (templateName == QLatin1String("2-1")) {
        // 2 weeks build, 1 week recovery
        for (int d = 0; d < days; d++) {
            int weekInBlock = (d / 7) % 3;
            double weekFactor;
            switch (weekInBlock) {
            case 0: weekFactor = 0.95; break;
            case 1: weekFactor = 1.10; break;
            case 2: weekFactor = 0.50; break;
            default: weekFactor = 1.0;
            }
            int dow = d % 7;
            double dayFactor = 1.0;
            if (dow == 0) dayFactor = 1.3;
            else if (dow == 2) dayFactor = 1.4;
            else if (dow == 4) dayFactor = 1.2;
            else if (dow == 5) dayFactor = 1.5;
            else if (dow == 6) dayFactor = 0.0;
            else dayFactor = 0.6;

            plan[d] = dailyAvg * weekFactor * dayFactor;
        }

    } else if (templateName == QLatin1String("progressive")) {
        // Gradual weekly increase with no dedicated recovery weeks
        for (int d = 0; d < days; d++) {
            int week = d / 7;
            int totalWeeks = std::max(1, days / 7);
            double weekFactor = 0.8 + 0.4 * (double(week) / totalWeeks);
            int dow = d % 7;
            double dayFactor = 1.0;
            if (dow == 5) dayFactor = 1.5;
            else if (dow == 6) dayFactor = 0.0;
            else if (dow == 2) dayFactor = 1.3;
            else dayFactor = 0.8;

            plan[d] = dailyAvg * weekFactor * dayFactor;
        }

    } else if (templateName == QLatin1String("polarized")) {
        // 80/20 — mostly low intensity with 2 hard days per week
        for (int d = 0; d < days; d++) {
            int dow = d % 7;
            if (dow == 6) plan[d] = 0.0;              // rest
            else if (dow == 2) plan[d] = dailyAvg * 2.0; // hard
            else if (dow == 4) plan[d] = dailyAvg * 1.8; // hard
            else plan[d] = dailyAvg * 0.5;               // easy
        }

    } else { // "flat" or unknown
        // Steady daily load with one rest day
        for (int d = 0; d < days; d++) {
            if (d % 7 == 6) plan[d] = 0.0;
            else plan[d] = targetWeeklyTSS / 6.0;
        }
    }

    return plan;
}
