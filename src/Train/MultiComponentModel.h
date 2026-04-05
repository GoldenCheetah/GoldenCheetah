/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_MULTICOMPONENT_MODEL_H
#define GC_MULTICOMPONENT_MODEL_H

#include <QJsonObject>
#include <QString>
#include <QVector>

/**
 * Energy system classification for decomposing training impulse.
 *
 * The standard Banister model treats all training as a single scalar (TSS).
 * This loses critical information — a 100 TSS sweet spot ride and a 100 TSS
 * VO2max ride stress different physiological systems.
 *
 * The 3D-IR model (Mujika et al., Pfeifer) decomposes training impulse
 * into three energy system components:
 *   - Aerobic (Zone 1-2, < VT1)
 *   - Glycolytic (Zone 3-4, VT1 to VT2)
 *   - Alactic/Neuromuscular (Zone 5+, > VT2)
 *
 * Each component has its own fitness/fatigue response with potentially
 * different decay constants and coefficients.
 */
enum class EnergySystem {
    Aerobic,      // Below VT1 / Zone 1-2 / < 75% FTP
    Glycolytic,   // VT1 to VT2 / Zone 3-4 / 75-105% FTP
    Neuromuscular // Above VT2 / Zone 5+ / > 105% FTP
};

/**
 * Decomposed training impulse for a single session.
 */
struct DecomposedImpulse {
    double aerobic = 0.0;       // time-in-zone weighted impulse, aerobic
    double glycolytic = 0.0;    // time-in-zone weighted impulse, glycolytic
    double neuromuscular = 0.0; // time-in-zone weighted impulse, neuromuscular
    double total = 0.0;         // sum (should equal original TSS approximately)

    QJsonObject toJson() const;
};

/**
 * Per-component Banister parameters.
 */
struct ComponentParams {
    EnergySystem system;
    double k1 = 0.2;   // fitness gain coefficient
    double k2 = 0.2;   // fatigue gain coefficient
    double t1 = 50.0;  // fitness decay constant
    double t2 = 11.0;  // fatigue decay constant
    double g = 0.0;    // current fitness accumulator
    double h = 0.0;    // current fatigue accumulator

    QJsonObject toJson() const;
};

/**
 * Multi-component forward projection.
 */
struct MultiComponentProjection {
    QVector<double> aerobicPerf;
    QVector<double> glycolyticPerf;
    QVector<double> neuromuscularPerf;
    QVector<double> compositePerf;  // weighted combination
    double peakComposite = 0.0;
    int peakDay = -1;

    QJsonObject toJson() const;
};

/**
 * Phase 4 stub: Multi-component (3D) Impulse-Response Model.
 *
 * Extends the standard Banister model by decomposing training load into
 * energy system components, each with its own fitness/fatigue dynamics.
 *
 * CURRENT STATUS: Interface design only. Implementation requires:
 * 1. Power zone time-in-zone data per ride (available via RideFile intervals)
 * 2. Zone boundary calibration (VT1/VT2 or %FTP thresholds)
 * 3. Separate fitting per energy system (needs component-specific performance tests)
 *
 * KEY DESIGN DECISIONS:
 * - Zone boundaries from athlete's power zones (Zones class)
 * - Aerobic: < 75% FTP, Glycolytic: 75-105% FTP, Neuromuscular: > 105% FTP
 * - Composite performance = weighted sum of component performances
 * - Weights depend on event type (road race vs TT vs criterium)
 *
 * REFERENCES:
 * - Mujika et al. (1996): multi-component IR for swimmers
 * - Pfeifer (2008): 3D-IR model for cycling
 * - Morton (1997): original multi-component extension proposal
 */
class MultiComponentModel {

public:

    /**
     * Decompose a ride's TSS into energy system components.
     * Uses power zone time-in-zone distribution.
     *
     * @param timeInZone  seconds spent in each power zone (zone 1..7+)
     * @param totalTSS    total TSS for the ride
     * @param ftp         functional threshold power
     */
    static DecomposedImpulse decompose(const QVector<double> &timeInZone,
                                       double totalTSS,
                                       int ftp);

    /**
     * Decompose by intensity factor (when zone data unavailable).
     * Uses a heuristic based on workout IF to estimate energy system split.
     */
    static DecomposedImpulse decomposeByIF(double tss, double intensityFactor);

    /**
     * Project multi-component model forward.
     * Each energy system is projected independently, then combined.
     *
     * @param params      per-component model parameters (3 elements)
     * @param dailyImpulse decomposed daily training impulse
     * @param weights     composite weighting (aerobic, glycolytic, neuromuscular)
     */
    static MultiComponentProjection projectForward(
        const QVector<ComponentParams> &params,
        const QVector<DecomposedImpulse> &dailyImpulse,
        const QVector<double> &weights = {0.5, 0.35, 0.15});

    /** Default component parameters based on literature. */
    static QVector<ComponentParams> defaultParams();
};

#endif
