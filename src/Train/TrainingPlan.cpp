/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "TrainingPlan.h"

#include "Context.h"
#include "Athlete.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QUuid>

// ---------------------------------------------------------------------------
// TrainingPlanPhase
// ---------------------------------------------------------------------------

QJsonObject TrainingPlanPhase::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("name")] = name;
    o[QStringLiteral("durationWeeks")] = durationWeeks;
    o[QStringLiteral("weeklyTSS")] = weeklyTSS;
    o[QStringLiteral("templateName")] = templateName;
    o[QStringLiteral("goal")] = goal;
    o[QStringLiteral("notes")] = notes;

    QJsonObject mix;
    for (auto it = workoutMix.constBegin(); it != workoutMix.constEnd(); ++it)
        mix[it.key()] = it.value();
    o[QStringLiteral("workoutMix")] = mix;

    return o;
}

TrainingPlanPhase TrainingPlanPhase::fromJson(const QJsonObject &o, bool *ok)
{
    TrainingPlanPhase p;
    p.name = o.value(QStringLiteral("name")).toString();
    p.durationWeeks = o.value(QStringLiteral("durationWeeks")).toInt(4);
    p.weeklyTSS = o.value(QStringLiteral("weeklyTSS")).toDouble(0.0);
    p.templateName = o.value(QStringLiteral("templateName")).toString();
    p.goal = o.value(QStringLiteral("goal")).toString();
    p.notes = o.value(QStringLiteral("notes")).toString();

    QJsonObject mix = o.value(QStringLiteral("workoutMix")).toObject();
    for (auto it = mix.constBegin(); it != mix.constEnd(); ++it)
        p.workoutMix[it.key()] = it.value().toDouble();

    if (ok) *ok = !p.name.isEmpty();
    return p;
}

// ---------------------------------------------------------------------------
// TrainingPlanStatus
// ---------------------------------------------------------------------------

QString trainingPlanStatusToString(TrainingPlanStatus s)
{
    switch (s) {
    case TrainingPlanStatus::Draft:     return QStringLiteral("draft");
    case TrainingPlanStatus::Active:    return QStringLiteral("active");
    case TrainingPlanStatus::Completed: return QStringLiteral("completed");
    case TrainingPlanStatus::Archived:  return QStringLiteral("archived");
    }
    return QStringLiteral("draft");
}

TrainingPlanStatus trainingPlanStatusFromString(const QString &s)
{
    QString lower = s.toLower().trimmed();
    if (lower == QLatin1String("active"))    return TrainingPlanStatus::Active;
    if (lower == QLatin1String("completed")) return TrainingPlanStatus::Completed;
    if (lower == QLatin1String("archived"))  return TrainingPlanStatus::Archived;
    return TrainingPlanStatus::Draft;
}

// ---------------------------------------------------------------------------
// TrainingPlan
// ---------------------------------------------------------------------------

int TrainingPlan::totalWeeks() const
{
    int weeks = 0;
    for (const auto &phase : phases)
        weeks += phase.durationWeeks;
    return weeks;
}

double TrainingPlan::totalPlannedTSS() const
{
    double total = 0.0;
    for (const auto &phase : phases)
        total += phase.weeklyTSS * phase.durationWeeks;
    return total;
}

QString TrainingPlan::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QJsonObject TrainingPlan::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("schemaVersion")] = schemaVersion;
    o[QStringLiteral("id")] = id;
    o[QStringLiteral("name")] = name;
    o[QStringLiteral("description")] = description;
    o[QStringLiteral("goal")] = goal;
    o[QStringLiteral("status")] = trainingPlanStatusToString(status);

    // Phases
    QJsonArray phasesArr;
    for (const auto &phase : phases)
        phasesArr.append(phase.toJson());
    o[QStringLiteral("phases")] = phasesArr;

    // Banister snapshot
    o[QStringLiteral("banisterSnapshot")] = banisterSnapshot.toJson();

    // Constraint bounds
    QJsonObject boundsObj;
    boundsObj[QStringLiteral("maxCtlRampPerWeek")] = constraintBounds.maxCtlRampPerWeek;
    boundsObj[QStringLiteral("maxMonotony")] = constraintBounds.maxMonotony;
    boundsObj[QStringLiteral("minACWR")] = constraintBounds.minACWR;
    boundsObj[QStringLiteral("maxACWR")] = constraintBounds.maxACWR;
    boundsObj[QStringLiteral("maxDailyTSS")] = constraintBounds.maxDailyTSS;
    boundsObj[QStringLiteral("tsbFloor")] = constraintBounds.tsbFloor;
    boundsObj[QStringLiteral("minRestDaysPerWeek")] = constraintBounds.minRestDaysPerWeek;
    o[QStringLiteral("constraintBounds")] = boundsObj;

    // Athlete starting state
    o[QStringLiteral("startingCTL")] = startingCTL;
    o[QStringLiteral("startingATL")] = startingATL;
    o[QStringLiteral("startingTSB")] = startingTSB;
    o[QStringLiteral("athleteFTP")] = athleteFTP;

    // Timeline
    if (startDate.isValid())
        o[QStringLiteral("startDate")] = startDate.toString(Qt::ISODate);
    if (endDate.isValid())
        o[QStringLiteral("endDate")] = endDate.toString(Qt::ISODate);

    // Audit
    o[QStringLiteral("generatorId")] = generatorId;
    o[QStringLiteral("modelId")] = modelId;
    o[QStringLiteral("modelVersion")] = modelVersion;
    if (createdAt.isValid())
        o[QStringLiteral("createdAt")] = createdAt.toString(Qt::ISODate);
    if (updatedAt.isValid())
        o[QStringLiteral("updatedAt")] = updatedAt.toString(Qt::ISODate);

    // Evaluation dates
    QJsonArray evalArr;
    for (const auto &d : evaluationDates)
        evalArr.append(d.toString(Qt::ISODate));
    o[QStringLiteral("evaluationDates")] = evalArr;

    // Computed fields
    o[QStringLiteral("totalWeeks")] = totalWeeks();
    o[QStringLiteral("totalPlannedTSS")] = totalPlannedTSS();

    return o;
}

TrainingPlan TrainingPlan::fromJson(const QJsonObject &o, bool *ok)
{
    TrainingPlan plan;
    plan.schemaVersion = o.value(QStringLiteral("schemaVersion")).toString(
        QString::fromLatin1(SCHEMA_VERSION));
    plan.id = o.value(QStringLiteral("id")).toString();
    plan.name = o.value(QStringLiteral("name")).toString();
    plan.description = o.value(QStringLiteral("description")).toString();
    plan.goal = o.value(QStringLiteral("goal")).toString();
    plan.status = trainingPlanStatusFromString(
        o.value(QStringLiteral("status")).toString());

    // Phases
    QJsonArray phasesArr = o.value(QStringLiteral("phases")).toArray();
    for (const auto &v : phasesArr) {
        bool phaseOk = false;
        TrainingPlanPhase phase = TrainingPlanPhase::fromJson(v.toObject(), &phaseOk);
        if (phaseOk)
            plan.phases.append(phase);
    }

    // Banister snapshot (reuse BanisterParams::toJson format)
    QJsonObject banObj = o.value(QStringLiteral("banisterSnapshot")).toObject();
    if (!banObj.isEmpty()) {
        plan.banisterSnapshot.k1 = banObj.value(QStringLiteral("k1")).toDouble(0.2);
        plan.banisterSnapshot.k2 = banObj.value(QStringLiteral("k2")).toDouble(0.2);
        plan.banisterSnapshot.p0 = banObj.value(QStringLiteral("p0")).toDouble(0.0);
        plan.banisterSnapshot.t1 = banObj.value(QStringLiteral("t1")).toDouble(50.0);
        plan.banisterSnapshot.t2 = banObj.value(QStringLiteral("t2")).toDouble(11.0);
        plan.banisterSnapshot.g = banObj.value(QStringLiteral("g")).toDouble(0.0);
        plan.banisterSnapshot.h = banObj.value(QStringLiteral("h")).toDouble(0.0);
        plan.banisterSnapshot.isFitted = banObj.value(QStringLiteral("isFitted")).toBool(false);
    }

    // Constraint bounds
    QJsonObject boundsObj = o.value(QStringLiteral("constraintBounds")).toObject();
    if (!boundsObj.isEmpty()) {
        plan.constraintBounds.maxCtlRampPerWeek = boundsObj.value(QStringLiteral("maxCtlRampPerWeek")).toDouble(5.0);
        plan.constraintBounds.maxMonotony = boundsObj.value(QStringLiteral("maxMonotony")).toDouble(1.5);
        plan.constraintBounds.minACWR = boundsObj.value(QStringLiteral("minACWR")).toDouble(0.8);
        plan.constraintBounds.maxACWR = boundsObj.value(QStringLiteral("maxACWR")).toDouble(1.3);
        plan.constraintBounds.maxDailyTSS = boundsObj.value(QStringLiteral("maxDailyTSS")).toDouble(300.0);
        plan.constraintBounds.tsbFloor = boundsObj.value(QStringLiteral("tsbFloor")).toDouble(-30.0);
        plan.constraintBounds.minRestDaysPerWeek = boundsObj.value(QStringLiteral("minRestDaysPerWeek")).toInt(1);
    }

    // Athlete starting state
    plan.startingCTL = o.value(QStringLiteral("startingCTL")).toDouble(0.0);
    plan.startingATL = o.value(QStringLiteral("startingATL")).toDouble(0.0);
    plan.startingTSB = o.value(QStringLiteral("startingTSB")).toDouble(0.0);
    plan.athleteFTP = o.value(QStringLiteral("athleteFTP")).toInt(0);

    // Timeline
    plan.startDate = QDate::fromString(
        o.value(QStringLiteral("startDate")).toString(), Qt::ISODate);
    plan.endDate = QDate::fromString(
        o.value(QStringLiteral("endDate")).toString(), Qt::ISODate);

    // Audit
    plan.generatorId = o.value(QStringLiteral("generatorId")).toString();
    plan.modelId = o.value(QStringLiteral("modelId")).toString();
    plan.modelVersion = o.value(QStringLiteral("modelVersion")).toString();
    plan.createdAt = QDateTime::fromString(
        o.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    plan.updatedAt = QDateTime::fromString(
        o.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);

    // Evaluation dates
    QJsonArray evalArr = o.value(QStringLiteral("evaluationDates")).toArray();
    for (const auto &v : evalArr) {
        QDate d = QDate::fromString(v.toString(), Qt::ISODate);
        if (d.isValid())
            plan.evaluationDates.append(d);
    }

    if (ok) *ok = !plan.id.isEmpty() && !plan.name.isEmpty();
    return plan;
}

// ---------------------------------------------------------------------------
// File I/O
// ---------------------------------------------------------------------------

QString TrainingPlan::planDirectory(Context *context)
{
    if (!context || !context->athlete || !context->athlete->home)
        return QString();
    return context->athlete->home->config().absolutePath()
           + QStringLiteral("/plans");
}

bool TrainingPlan::save(Context *context, QString *savedPath, QStringList *errors) const
{
    QString dirPath = planDirectory(context);
    if (dirPath.isEmpty()) {
        if (errors) *errors << QStringLiteral("Invalid context or athlete home");
        return false;
    }

    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (errors) *errors << QStringLiteral("Could not create plans directory");
        return false;
    }

    QString effectiveId = id.isEmpty() ? generateId() : id;
    QString filename = QStringLiteral("plan_%1.json").arg(effectiveId);
    QString filepath = dir.absoluteFilePath(filename);

    QJsonDocument doc(toJson());
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errors) *errors << QStringLiteral("Could not open plan file for writing: %1").arg(filepath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (savedPath) *savedPath = filepath;
    return true;
}

TrainingPlan TrainingPlan::load(const QString &filepath, bool *ok)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (ok) *ok = false;
        return TrainingPlan();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (ok) *ok = false;
        return TrainingPlan();
    }

    return fromJson(doc.object(), ok);
}

QList<TrainingPlan> TrainingPlan::listAll(Context *context)
{
    QList<TrainingPlan> result;
    QString dirPath = planDirectory(context);
    if (dirPath.isEmpty()) return result;

    QDir dir(dirPath);
    if (!dir.exists()) return result;

    QStringList filters;
    filters << QStringLiteral("plan_*.json");
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const auto &fi : files) {
        bool ok = false;
        TrainingPlan plan = load(fi.absoluteFilePath(), &ok);
        if (ok)
            result.append(plan);
    }

    return result;
}

bool TrainingPlan::remove(Context *context, const QString &planId, QStringList *errors)
{
    QString dirPath = planDirectory(context);
    if (dirPath.isEmpty()) {
        if (errors) *errors << QStringLiteral("Invalid context or athlete home");
        return false;
    }

    QDir dir(dirPath);
    QString filename = QStringLiteral("plan_%1.json").arg(planId);
    QString filepath = dir.absoluteFilePath(filename);

    if (!QFile::exists(filepath)) {
        if (errors) *errors << QStringLiteral("Plan not found: %1").arg(planId);
        return false;
    }

    if (!QFile::remove(filepath)) {
        if (errors) *errors << QStringLiteral("Could not delete plan file: %1").arg(filepath);
        return false;
    }

    return true;
}
