/*
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "WorkoutGenerationService.h"

#include "Athlete.h"
#include "Context.h"
#include "ErgFile.h"
#include "PMCData.h"
#include "Settings.h"
#include "TrainDB.h"
#include "Zones.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <climits>

extern TrainDB *trainDB;

namespace {

static constexpr int kMinDurationMin = 20;
static constexpr int kMaxDurationMin = 240;
static constexpr int kWarmupSec = 10 * 60;
static constexpr int kCooldownSec = 5 * 60;
static constexpr const char *kDefaultGeneratorId = "goldencheetah-heuristic-v1";

QDate
normalizeDate(const QDate &date)
{
    return date.isValid() ? date : QDate::currentDate();
}

QDate
parseDateString(const QString &raw)
{
    if (raw.isEmpty()) {
        return QDate();
    }
    QDate parsed = QDate::fromString(raw, Qt::ISODate);
    if (!parsed.isValid()) {
        parsed = QDate::fromString(raw, QStringLiteral("yyyy/MM/dd"));
    }
    if (!parsed.isValid()) {
        parsed = QDate::fromString(raw, QStringLiteral("yyyy-MM-dd"));
    }
    return parsed;
}

QString
normalizedWorkoutType(QString value)
{
    value = value.trimmed().toLower();
    value.remove(QRegularExpression(QStringLiteral("[\\s_-]+")));
    if (value == QStringLiteral("recovery")) {
        return QStringLiteral("recovery");
    }
    if (value == QStringLiteral("endurance")) {
        return QStringLiteral("endurance");
    }
    if (value == QStringLiteral("sweetspot")) {
        return QStringLiteral("sweetspot");
    }
    if (value == QStringLiteral("threshold")) {
        return QStringLiteral("threshold");
    }
    if (value == QStringLiteral("vo2") || value == QStringLiteral("vo2max")) {
        return QStringLiteral("vo2max");
    }
    if (value == QStringLiteral("anaerobic") || value == QStringLiteral("sprint")) {
        return QStringLiteral("anaerobic");
    }
    if (value == QStringLiteral("mixed")) {
        return QStringLiteral("mixed");
    }
    return QString();
}

QString
workoutTypeTitle(const QString &value)
{
    if (value == QStringLiteral("recovery")) {
        return QStringLiteral("Recovery");
    }
    if (value == QStringLiteral("endurance")) {
        return QStringLiteral("Endurance");
    }
    if (value == QStringLiteral("sweetspot")) {
        return QStringLiteral("Sweet Spot");
    }
    if (value == QStringLiteral("threshold")) {
        return QStringLiteral("Threshold");
    }
    if (value == QStringLiteral("vo2max")) {
        return QStringLiteral("VO2max");
    }
    if (value == QStringLiteral("anaerobic")) {
        return QStringLiteral("Anaerobic");
    }
    if (value == QStringLiteral("mixed")) {
        return QStringLiteral("Mixed");
    }
    return value;
}

void
appendPercentStep(WorkoutDraft &draft, int durationSec, double pct, const QString &label)
{
    if (durationSec <= 0) {
        return;
    }
    WorkoutDraftStep step;
    step.durationSec = durationSec;
    step.powerMode = WorkoutDraftPowerMode::PercentFtp;
    step.powerValue = pct;
    step.label = label;
    draft.steps.append(step);
}

int
stepsDurationSec(const WorkoutDraft &draft)
{
    int totalSec = 0;
    for (const WorkoutDraftStep &step : draft.steps) {
        if (step.durationSec > 0) {
            totalSec += step.durationSec;
        }
    }
    return totalSec;
}

int
scaledIntervalCount(int baseCount, double cycleMinutes, double availableMinutes, double ctl)
{
    double ctlFactor = ctl > 0.0 ? std::max(0.6, std::min(1.5, ctl / 70.0)) : 0.8;
    int scaled = qRound(baseCount * ctlFactor);
    scaled = std::max(2, scaled);
    int maxFit = static_cast<int>(availableMinutes / cycleMinutes);
    return std::min(scaled, std::max(1, maxFit));
}

void
fillRemaining(WorkoutDraft &draft, int targetCoreSec, double pct, const QString &label)
{
    int remaining = targetCoreSec - stepsDurationSec(draft);
    if (remaining > 0) {
        appendPercentStep(draft, remaining, pct, label);
    }
}

QString
sanitizeFilenameBase(QString value)
{
    value = value.trimmed();
    value.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]+")), QStringLiteral(" "));
    value = value.simplified();
    value.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral("_"));
    value.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")));
    if (value.isEmpty()) {
        value = QStringLiteral("ai_workout");
    }
    return value.left(80);
}

int
resolveWatts(const WorkoutDraftStep &step, int ftp, int cp)
{
    switch (step.powerMode) {
    case WorkoutDraftPowerMode::AbsoluteWatts:
        return static_cast<int>(std::lround(step.powerValue));
    case WorkoutDraftPowerMode::PercentFtp:
    default: {
        double pct = step.powerValue / 100.0;
        return static_cast<int>(std::lround(pct * static_cast<double>(ftp > 0 ? ftp : cp)));
    }
    }
}

}

QJsonObject
WorkoutAthleteSnapshot::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("athleteName")] = athleteName;
    o[QStringLiteral("date")] = date.toString(Qt::ISODate);
    o[QStringLiteral("ftp")] = ftp;
    o[QStringLiteral("cp")] = cp;
    o[QStringLiteral("ctl")] = ctl;
    o[QStringLiteral("atl")] = atl;
    o[QStringLiteral("tsb")] = tsb;
    o[QStringLiteral("canGenerate")] = canGenerate();
    return o;
}

WorkoutHeuristicRequest
WorkoutHeuristicRequest::fromJson(const QJsonObject &o, bool *ok, QString *error)
{
    WorkoutHeuristicRequest request;
    bool innerOk = true;
    QString innerError;

    QString rawType = o.value(QStringLiteral("workoutType")).toString();
    request.workoutType = normalizedWorkoutType(rawType);
    if (request.workoutType.isEmpty()) {
        innerOk = false;
        innerError = QStringLiteral("Unsupported or missing workoutType");
    }

    if (o.contains(QStringLiteral("durationMin"))) {
        request.durationMin = o.value(QStringLiteral("durationMin")).toInt();
    }
    if (request.durationMin < kMinDurationMin || request.durationMin > kMaxDurationMin) {
        innerOk = false;
        if (innerError.isEmpty()) {
            innerError = QStringLiteral("durationMin must be between 20 and 240");
        }
    }

    request.date = parseDateString(o.value(QStringLiteral("date")).toString());
    request.displayName = o.value(QStringLiteral("displayName")).toString().trimmed();
    request.description = o.value(QStringLiteral("description")).toString().trimmed();
    request.generatorId = o.value(QStringLiteral("generatorId")).toString().trimmed();
    request.modelId = o.value(QStringLiteral("modelId")).toString().trimmed();
    request.modelVersion = o.value(QStringLiteral("modelVersion")).toString().trimmed();

    if (request.generatorId.isEmpty()) {
        request.generatorId = QString::fromLatin1(kDefaultGeneratorId);
    }

    if (ok) {
        *ok = innerOk;
    }
    if (error) {
        *error = innerError;
    }
    return request;
}

QString
WorkoutGenerationService::buildErgText(const WorkoutDraft &draft, int ftp, int cp)
{
    QStringList lines;
    lines << QStringLiteral("[COURSE HEADER]");
    if (ftp > 0) {
        lines << QStringLiteral("FTP=%1").arg(ftp);
    }
    if (!draft.generatorId.isEmpty()) {
        lines << QStringLiteral("SOURCE=%1").arg(draft.generatorId);
    }
    lines << QStringLiteral("VERSION=2");
    lines << QStringLiteral("UNITS=METRIC");

    QString desc = draft.description;
    QString meta = QStringLiteral("[gc-meta schema=%1").arg(draft.schemaVersion);
    if (!draft.generatorId.isEmpty()) {
        meta += QStringLiteral(" generator=%1").arg(draft.generatorId);
    }
    if (!draft.modelId.isEmpty()) {
        meta += QStringLiteral(" model=%1").arg(draft.modelId);
    }
    if (!draft.modelVersion.isEmpty()) {
        meta += QStringLiteral(" modelVersion=%1").arg(draft.modelVersion);
    }
    meta += QStringLiteral("]");
    if (!desc.isEmpty()) {
        desc += QStringLiteral("\n");
    }
    desc += meta;
    lines << QStringLiteral("DESCRIPTION=%1").arg(desc);
    lines << QStringLiteral("MINUTES WATTS");
    lines << QStringLiteral("[END COURSE HEADER]");
    lines << QStringLiteral("[COURSE DATA]");

    double tMs = 0.0;
    int prevW = INT_MIN;

    for (const WorkoutDraftStep &step : draft.steps) {
        if (step.durationSec <= 0) {
            continue;
        }
        int w = resolveWatts(step, ftp, cp);
        if (prevW == INT_MIN) {
            lines << QStringLiteral("%1 %2").arg(0.0, 0, 'f', 4).arg(w);
        } else if (w != prevW) {
            double tMin = tMs / 60000.0;
            lines << QStringLiteral("%1 %2").arg(tMin, 0, 'f', 4).arg(prevW);
            lines << QStringLiteral("%1 %2").arg(tMin, 0, 'f', 4).arg(w);
        }
        tMs += static_cast<double>(step.durationSec) * 1000.0;
        double tEndMin = tMs / 60000.0;
        lines << QStringLiteral("%1 %2").arg(tEndMin, 0, 'f', 4).arg(w);
        prevW = w;
    }

    lines << QStringLiteral("[END COURSE DATA]");
    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

ErgFile *
WorkoutGenerationService::createErgFile(Context *context, const WorkoutDraft &draft,
                                        const QDate &when, QStringList &errors)
{
    if (!context || !context->athlete) {
        errors << QStringLiteral("Invalid context");
        return nullptr;
    }
    if (draft.steps.isEmpty()) {
        errors << QStringLiteral("Workout draft has no steps");
        return nullptr;
    }

    QDate effectiveDate = normalizeDate(when);
    int totalSec = 0;
    for (const WorkoutDraftStep &step : draft.steps) {
        if (step.durationSec > 0) {
            totalSec += step.durationSec;
        }
    }
    if (totalSec <= 0) {
        errors << QStringLiteral("Workout draft has no intervals with positive duration");
        return nullptr;
    }

    int ftp = 0;
    int cp = 0;
    const Zones *zones = context->athlete->zones(QStringLiteral("Bike"));
    if (zones) {
        int zoneRange = zones->whichRange(effectiveDate);
        if (zoneRange >= 0) {
            ftp = zones->getFTP(zoneRange);
            cp = zones->getCP(zoneRange);
            if (ftp <= 0) {
                ftp = cp;
            }
            if (cp <= 0) {
                cp = ftp;
            }
        }
    }

    bool needsRelativePower = false;
    for (const WorkoutDraftStep &step : draft.steps) {
        if (step.powerMode == WorkoutDraftPowerMode::PercentFtp) {
            needsRelativePower = true;
            break;
        }
    }
    if (needsRelativePower && ftp <= 0 && cp <= 0) {
        errors << QStringLiteral("Bike FTP/CP is not configured for the requested date");
        return nullptr;
    }

    QString erg = buildErgText(draft, ftp, cp);
    ErgFile *ergFile = ErgFile::fromContent(erg, context, effectiveDate);
    if (!ergFile || !ergFile->isValid()) {
        errors << QStringLiteral("Failed to parse generated workout");
        delete ergFile;
        return nullptr;
    }

    if (!draft.displayName.isEmpty()) {
        ergFile->name(draft.displayName);
    }
    if (!draft.generatorId.isEmpty()) {
        ergFile->source(draft.generatorId);
    }

    ergFile->Laps.clear();
    ergFile->Texts.clear();
    double elapsedMs = 0.0;
    int lapNum = 1;
    for (const WorkoutDraftStep &step : draft.steps) {
        QString label = step.label.trimmed();
        if (!label.isEmpty()) {
            ergFile->Laps.append(ErgFileLap(elapsedMs, lapNum++, label));
            ergFile->Texts.append(ErgFileText(elapsedMs, std::min(step.durationSec, 30), label));
        }
        elapsedMs += static_cast<double>(step.durationSec) * 1000.0;
    }
    ergFile->finalize();
    return ergFile;
}

WorkoutAthleteSnapshot
WorkoutGenerationService::athleteSnapshot(Context *context, const QDate &when)
{
    WorkoutAthleteSnapshot snapshot;
    if (!context || !context->athlete) {
        return snapshot;
    }

    snapshot.athleteName = context->athlete->cyclist;
    snapshot.date = normalizeDate(when);

    const Zones *zones = context->athlete->zones(QStringLiteral("Bike"));
    if (zones) {
        int zoneRange = zones->whichRange(snapshot.date);
        if (zoneRange >= 0) {
            snapshot.ftp = zones->getFTP(zoneRange);
            snapshot.cp = zones->getCP(zoneRange);
            if (snapshot.ftp <= 0) {
                snapshot.ftp = snapshot.cp;
            }
        }
    }

    PMCData *pmc = context->athlete->getPMCFor(QStringLiteral("coggan_tss"));
    if (pmc) {
        snapshot.ctl = pmc->lts(snapshot.date);
        snapshot.atl = pmc->sts(snapshot.date);
        snapshot.tsb = pmc->sb(snapshot.date);
    }

    return snapshot;
}

WorkoutDraft
WorkoutGenerationService::generateHeuristicDraft(const WorkoutAthleteSnapshot &snapshot,
                                                 const WorkoutHeuristicRequest &request,
                                                 QStringList *warnings)
{
    WorkoutDraft draft;
    draft.schemaVersion = QString::fromLatin1(WorkoutDraft::SCHEMA_VERSION);
    draft.generatorId = request.generatorId.isEmpty() ? QString::fromLatin1(kDefaultGeneratorId) : request.generatorId;
    draft.modelId = request.modelId;
    draft.modelVersion = request.modelVersion;

    QString type = normalizedWorkoutType(request.workoutType);
    QString title = workoutTypeTitle(type);
    draft.displayName = request.displayName.isEmpty()
        ? QStringLiteral("AI %1 %2min").arg(title).arg(request.durationMin)
        : request.displayName;
    draft.description = request.description.isEmpty()
        ? QStringLiteral("Heuristic %1 workout, %2 minutes, FTP=%3, CTL=%4, ATL=%5, TSB=%6.")
              .arg(title)
              .arg(request.durationMin)
              .arg(snapshot.ftp)
              .arg(snapshot.ctl, 0, 'f', 1)
              .arg(snapshot.atl, 0, 'f', 1)
              .arg(snapshot.tsb, 0, 'f', 1)
        : request.description;

    if (warnings && snapshot.ctl <= 0.0) {
        warnings->append(QStringLiteral("PMC data unavailable or zero; using conservative interval counts."));
    }

    int availableSec = std::max(5 * 60, request.durationMin * 60 - kWarmupSec - kCooldownSec);

    appendPercentStep(draft, 5 * 60, 50.0, QStringLiteral("Warmup"));
    appendPercentStep(draft, 5 * 60, 60.0, QStringLiteral("Warmup"));

    if (type == QStringLiteral("recovery")) {
        const double pattern[] = {45.0, 50.0, 55.0, 50.0};
        int remaining = availableSec;
        int idx = 0;
        while (remaining > 0) {
            int block = std::min(2 * 60, remaining);
            appendPercentStep(draft, block, pattern[idx % 4], QStringLiteral("Recovery"));
            remaining -= block;
            ++idx;
        }
    } else if (type == QStringLiteral("endurance")) {
        const double pattern[] = {65.0, 70.0, 75.0, 70.0};
        int remaining = availableSec;
        int idx = 0;
        while (remaining > 0) {
            int block = std::min(3 * 60, remaining);
            appendPercentStep(draft, block, pattern[idx % 4], QStringLiteral("Endurance"));
            remaining -= block;
            ++idx;
        }
    } else if (type == QStringLiteral("sweetspot")) {
        int intervals = scaledIntervalCount(4, 12.0, availableSec / 60.0, snapshot.ctl);
        for (int i = 0; i < intervals; ++i) {
            appendPercentStep(draft, 8 * 60, 90.0, QStringLiteral("Sweet Spot %1").arg(i + 1));
            appendPercentStep(draft, 4 * 60, 55.0, QStringLiteral("Recovery"));
        }
        fillRemaining(draft, kWarmupSec + availableSec, 65.0, QStringLiteral("Endurance"));
    } else if (type == QStringLiteral("threshold")) {
        int intervals = scaledIntervalCount(4, 10.0, availableSec / 60.0, snapshot.ctl);
        for (int i = 0; i < intervals; ++i) {
            appendPercentStep(draft, 5 * 60, 100.0, QStringLiteral("Threshold %1").arg(i + 1));
            appendPercentStep(draft, 5 * 60, 55.0, QStringLiteral("Recovery"));
        }
        fillRemaining(draft, kWarmupSec + availableSec, 60.0, QStringLiteral("Endurance"));
    } else if (type == QStringLiteral("vo2max")) {
        int intervals = scaledIntervalCount(5, 6.0, availableSec / 60.0, snapshot.ctl);
        for (int i = 0; i < intervals; ++i) {
            appendPercentStep(draft, 3 * 60, 115.0, QStringLiteral("VO2max %1").arg(i + 1));
            appendPercentStep(draft, 3 * 60, 50.0, QStringLiteral("Recovery"));
        }
        fillRemaining(draft, kWarmupSec + availableSec, 55.0, QStringLiteral("Endurance"));
    } else if (type == QStringLiteral("anaerobic")) {
        int intervals = scaledIntervalCount(8, 5.0, availableSec / 60.0, snapshot.ctl);
        for (int i = 0; i < intervals; ++i) {
            appendPercentStep(draft, 30, 150.0, QStringLiteral("Sprint %1").arg(i + 1));
            appendPercentStep(draft, 4 * 60 + 30, 40.0, QStringLiteral("Recovery"));
        }
        fillRemaining(draft, kWarmupSec + availableSec, 45.0, QStringLiteral("Recovery"));
    } else {
        int firstBlockSec = std::min(10 * 60, availableSec / 4);
        appendPercentStep(draft, firstBlockSec, 70.0, QStringLiteral("Endurance"));

        int mixedStartSec = stepsDurationSec(draft);
        int remainingSec = kWarmupSec + availableSec - mixedStartSec;
        int thresholdBudgetSec = static_cast<int>(remainingSec * 0.6);
        int thresholdIntervals = scaledIntervalCount(3, 7.0, thresholdBudgetSec / 60.0, snapshot.ctl);
        for (int i = 0; i < thresholdIntervals; ++i) {
            appendPercentStep(draft, 4 * 60, 100.0, QStringLiteral("Threshold %1").arg(i + 1));
            appendPercentStep(draft, 3 * 60, 55.0, QStringLiteral("Recovery"));
        }

        remainingSec = kWarmupSec + availableSec - stepsDurationSec(draft);
        int vo2Intervals = scaledIntervalCount(3, 4.0, remainingSec / 60.0, snapshot.ctl);
        for (int i = 0; i < vo2Intervals; ++i) {
            appendPercentStep(draft, 2 * 60, 115.0, QStringLiteral("VO2max %1").arg(i + 1));
            appendPercentStep(draft, 2 * 60, 50.0, QStringLiteral("Recovery"));
        }
        fillRemaining(draft, kWarmupSec + availableSec, 60.0, QStringLiteral("Endurance"));
    }

    appendPercentStep(draft, 3 * 60, 55.0, QStringLiteral("Cooldown"));
    appendPercentStep(draft, 2 * 60, 40.0, QStringLiteral("Cooldown"));

    return draft;
}

bool
WorkoutGenerationService::saveDraft(Context *context, const WorkoutDraft &draft, const QDate &when,
                                    QString *savedPath, QStringList &errors)
{
    if (!context || !context->athlete) {
        errors << QStringLiteral("Invalid context");
        return false;
    }

    QString workoutDir = appsettings->value(nullptr, GC_WORKOUTDIR, QString()).toString();
    if (workoutDir.isEmpty()) {
        errors << QStringLiteral("No workout directory configured");
        return false;
    }

    QDir dir(workoutDir);
    if (!dir.exists()) {
        errors << QStringLiteral("Workout directory does not exist");
        return false;
    }

    QDate effectiveDate = normalizeDate(when);
    QStringList buildErrors;
    ErgFile *ergFile = createErgFile(context, draft, effectiveDate, buildErrors);
    if (!ergFile) {
        errors << buildErrors;
        return false;
    }

    QString stem = sanitizeFilenameBase(draft.displayName);
    QString baseName = QStringLiteral("%1_%2").arg(stem).arg(effectiveDate.toString(QStringLiteral("yyyyMMdd")));
    QString filepath = dir.absoluteFilePath(baseName + QStringLiteral(".erg"));
    int suffix = 1;
    while (QFile::exists(filepath)) {
        filepath = dir.absoluteFilePath(QStringLiteral("%1_%2.erg").arg(baseName).arg(suffix++));
    }

    ergFile->filename(filepath);
    bool ok = ergFile->save(buildErrors);
    if (!ok) {
        errors << buildErrors;
        delete ergFile;
        return false;
    }

    trainDB->startLUW();
    bool imported = trainDB->importWorkout(filepath, *ergFile);
    trainDB->endLUW();
    delete ergFile;

    if (!imported) {
        errors << QStringLiteral("Workout saved but failed to import into library");
        return false;
    }

    if (savedPath) {
        *savedPath = filepath;
    }
    return true;
}

WorkoutDraft
WorkoutGenerationService::exampleDeterministicDraft()
{
    WorkoutDraft d;
    d.schemaVersion = QString::fromLatin1(WorkoutDraft::SCHEMA_VERSION);
    d.displayName = QStringLiteral("Example endurance blocks");
    d.description = QStringLiteral("Deterministic warmup / tempo / cooldown (heuristic demo).");
    d.generatorId = QStringLiteral("golden-cheetah-example");
    d.modelId.clear();
    d.modelVersion.clear();

    WorkoutDraftStep w;
    w.durationSec = 10 * 60;
    w.powerMode = WorkoutDraftPowerMode::PercentFtp;
    w.powerValue = 55.0;
    w.label = QStringLiteral("Warmup");
    d.steps.append(w);

    WorkoutDraftStep m;
    m.durationSec = 15 * 60;
    m.powerMode = WorkoutDraftPowerMode::PercentFtp;
    m.powerValue = 85.0;
    m.label = QStringLiteral("Tempo");
    d.steps.append(m);

    WorkoutDraftStep c;
    c.durationSec = 10 * 60;
    c.powerMode = WorkoutDraftPowerMode::PercentFtp;
    c.powerValue = 50.0;
    c.label = QStringLiteral("Cooldown");
    d.steps.append(c);

    return d;
}
