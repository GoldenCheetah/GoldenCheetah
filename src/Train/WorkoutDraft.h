/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_WORKOUT_DRAFT_H
#define GC_WORKOUT_DRAFT_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

/** How each step interprets powerValue. */
enum class WorkoutDraftPowerMode {
    PercentFtp,
    AbsoluteWatts
};

/** One steady block (flat power). Ramps / multi-step intervals can be added later. */
struct WorkoutDraftStep {
    int durationSec = 0;
    WorkoutDraftPowerMode powerMode = WorkoutDraftPowerMode::PercentFtp;
    /** Percent of FTP (0–inf) or watts, depending on powerMode. */
    double powerValue = 0.0;
    QString label;

    QJsonObject toJson() const;
    static WorkoutDraftStep fromJson(const QJsonObject &o, bool *ok = nullptr);
};

/**
 * Portable workout description for generation, APIs, and MCP bridges.
 * Serialize with toJson / fromJson for HTTP or tool boundaries.
 */
struct WorkoutDraft {
    static constexpr const char *SCHEMA_VERSION = "1";

    QString schemaVersion = QString::fromLatin1(SCHEMA_VERSION);
    QString displayName;
    QString description;
    /** Stable id for the generator (e.g. heuristic service name), stored as workout SOURCE on import. */
    QString generatorId;
    /** Optional audit: model or provider identifier. */
    QString modelId;
    QString modelVersion;
    QList<WorkoutDraftStep> steps;

    QJsonObject toJson() const;
    static WorkoutDraft fromJson(const QJsonObject &o, bool *ok = nullptr);
};

#endif
