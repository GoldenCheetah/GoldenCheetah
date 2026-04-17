/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "WorkoutDraft.h"

static const char *KEY_SCHEMA = "schemaVersion";
static const char *KEY_DISPLAY = "displayName";
static const char *KEY_DESC = "description";
static const char *KEY_GEN = "generatorId";
static const char *KEY_MODEL = "modelId";
static const char *KEY_MODEL_VER = "modelVersion";
static const char *KEY_STEPS = "steps";

static const char *KEY_DUR = "durationSec";
static const char *KEY_MODE = "powerMode";
static const char *KEY_PWR = "powerValue";
static const char *KEY_LABEL = "label";

static QString
powerModeToString(WorkoutDraftPowerMode m)
{
    switch (m) {
    case WorkoutDraftPowerMode::AbsoluteWatts:
        return QStringLiteral("absoluteWatts");
    case WorkoutDraftPowerMode::PercentFtp:
    default:
        return QStringLiteral("percentFtp");
    }
}

static WorkoutDraftPowerMode
powerModeFromString(const QString &s, bool *ok)
{
    if (s == QStringLiteral("absoluteWatts")) {
        if (ok) {
            *ok = true;
        }
        return WorkoutDraftPowerMode::AbsoluteWatts;
    }
    if (s == QStringLiteral("percentFtp")) {
        if (ok) {
            *ok = true;
        }
        return WorkoutDraftPowerMode::PercentFtp;
    }
    if (ok) {
        *ok = false;
    }
    return WorkoutDraftPowerMode::PercentFtp;
}

QJsonObject
WorkoutDraftStep::toJson() const
{
    QJsonObject o;
    o[KEY_DUR] = durationSec;
    o[KEY_MODE] = powerModeToString(powerMode);
    o[KEY_PWR] = powerValue;
    o[KEY_LABEL] = label;
    return o;
}

WorkoutDraftStep
WorkoutDraftStep::fromJson(const QJsonObject &o, bool *ok)
{
    WorkoutDraftStep s;
    bool innerOk = true;
    if (!o.contains(KEY_DUR)) {
        innerOk = false;
    } else {
        s.durationSec = o[KEY_DUR].toInt();
        if (s.durationSec <= 0) {
            innerOk = false;
        }
    }
    if (!o.contains(KEY_MODE) || !o[KEY_MODE].isString()) {
        innerOk = false;
    } else {
        bool mOk = false;
        s.powerMode = powerModeFromString(o[KEY_MODE].toString(), &mOk);
        innerOk = innerOk && mOk;
    }
    if (!o.contains(KEY_PWR)) {
        innerOk = false;
    } else {
        s.powerValue = o[KEY_PWR].toDouble();
    }
    if (o.contains(KEY_LABEL) && o[KEY_LABEL].isString()) {
        s.label = o[KEY_LABEL].toString();
    }
    if (ok) {
        *ok = innerOk;
    }
    return s;
}

QJsonObject
WorkoutDraft::toJson() const
{
    QJsonObject o;
    o[KEY_SCHEMA] = schemaVersion;
    o[KEY_DISPLAY] = displayName;
    o[KEY_DESC] = description;
    o[KEY_GEN] = generatorId;
    o[KEY_MODEL] = modelId;
    o[KEY_MODEL_VER] = modelVersion;
    QJsonArray a;
    for (const WorkoutDraftStep &st : steps) {
        a.append(st.toJson());
    }
    o[KEY_STEPS] = a;
    return o;
}

WorkoutDraft
WorkoutDraft::fromJson(const QJsonObject &o, bool *ok)
{
    WorkoutDraft d;
    bool innerOk = true;

    if (!o.contains(KEY_SCHEMA) || !o[KEY_SCHEMA].isString()) {
        innerOk = false;
    } else {
        d.schemaVersion = o[KEY_SCHEMA].toString();
        if (d.schemaVersion != QString::fromLatin1(SCHEMA_VERSION)) {
            innerOk = false;
        }
    }
    if (o.contains(KEY_DISPLAY) && o[KEY_DISPLAY].isString()) {
        d.displayName = o[KEY_DISPLAY].toString();
    }
    if (o.contains(KEY_DESC) && o[KEY_DESC].isString()) {
        d.description = o[KEY_DESC].toString();
    }
    if (o.contains(KEY_GEN) && o[KEY_GEN].isString()) {
        d.generatorId = o[KEY_GEN].toString();
    }
    if (o.contains(KEY_MODEL) && o[KEY_MODEL].isString()) {
        d.modelId = o[KEY_MODEL].toString();
    }
    if (o.contains(KEY_MODEL_VER) && o[KEY_MODEL_VER].isString()) {
        d.modelVersion = o[KEY_MODEL_VER].toString();
    }
    if (!o.contains(KEY_STEPS) || !o[KEY_STEPS].isArray()) {
        innerOk = false;
    } else {
        QJsonArray a = o[KEY_STEPS].toArray();
        for (const QJsonValue &v : a) {
            if (!v.isObject()) {
                innerOk = false;
                break;
            }
            bool stepOk = false;
            WorkoutDraftStep st = WorkoutDraftStep::fromJson(v.toObject(), &stepOk);
            if (!stepOk) {
                innerOk = false;
                break;
            }
            d.steps.append(st);
        }
    }
    if (ok) {
        *ok = innerOk;
    }
    return d;
}
