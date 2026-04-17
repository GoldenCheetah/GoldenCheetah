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
#include "Library.h"
#include "LibraryParser.h"
#include "Measures.h"
#include "PMCData.h"
#include "RideMetadata.h"
#include "Season.h"
#include "Seasons.h"
#include "Settings.h"
#include "TrainDB.h"
#include "Zones.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <QThread>

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
canonicalOrAbsolutePath(const QString &path)
{
    QFileInfo info(path);
    QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}

QString
normalizedWorkoutReference(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    return QDir::cleanPath(canonicalOrAbsolutePath(trimmed));
}

bool
pathIsWithinDirectory(const QString &path, const QString &directory)
{
    QString normalizedPath = normalizedWorkoutReference(path);
    QString normalizedDir = normalizedWorkoutReference(directory);
    if (normalizedPath.isEmpty() || normalizedDir.isEmpty()) {
        return false;
    }
    if (!normalizedDir.endsWith(QLatin1Char('/'))) {
        normalizedDir += QLatin1Char('/');
    }
    return normalizedPath == normalizedDir.chopped(1) || normalizedPath.startsWith(normalizedDir);
}

void
ensureWorkoutReference(Context *context, const QString &filepath)
{
    if (!context || !context->athlete) {
        return;
    }

    QDir gcRoot = context->athlete->home->root();
    if (!gcRoot.cdUp()) {
        return;
    }

    Library::initialise(gcRoot);

    Library *library = Library::findLibrary(QStringLiteral("Media Library"));
    if (!library) {
        library = new Library;
        library->name = QStringLiteral("Media Library");
        libraries.append(library);
    }

    const QString normalizedPath = normalizedWorkoutReference(filepath);
    if (normalizedPath.isEmpty()) {
        return;
    }

    for (const QString &path : std::as_const(library->paths)) {
        if (pathIsWithinDirectory(normalizedPath, path)) {
            return;
        }
    }

    for (const QString &ref : std::as_const(library->refs)) {
        if (canonicalOrAbsolutePath(ref) == normalizedPath) {
            return;
        }
    }

    library->refs.append(normalizedPath);
    LibraryParser::serialize(context->athlete->home->root());
}

void
ensureWorkoutReferenceOnMainThread(Context *context, const QString &filepath)
{
    QCoreApplication *application = QCoreApplication::instance();
    if (!application) {
        return;
    }

    if (QThread::currentThread() == application->thread()) {
        ensureWorkoutReference(context, filepath);
        return;
    }

    QMetaObject::invokeMethod(application, [context, filepath]() {
        ensureWorkoutReference(context, filepath);
    }, Qt::BlockingQueuedConnection);
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

QString
plannedActivityBasename(const QDateTime &dateTime)
{
    return dateTime.toString(QStringLiteral("yyyy_MM_dd_HH_mm_ss"));
}

QString
normalizedSport(QString sport)
{
    sport = RideFile::sportTag(sport.trimmed());
    return sport.isEmpty() ? QStringLiteral("Bike") : sport;
}

void
setMetricOverride(RideFile &rideFile, const QString &metricName, int value)
{
    if (value <= 0) {
        return;
    }
    QMap<QString, QString> values;
    values.insert(QStringLiteral("value"), QString::number(value));
    rideFile.metricOverrides.insert(metricName, values);
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

    // Phase 1 enrichments
    o[QStringLiteral("tsbTrend")] = tsbTrend;
    o[QStringLiteral("ctlRampRate")] = ctlRampRate;
    o[QStringLiteral("hasSimulationData")] = hasSimulationData();
    if (daysToEvent >= 0) {
        o[QStringLiteral("daysToEvent")] = daysToEvent;
        o[QStringLiteral("nextEventName")] = nextEventName;
    }
    if (!recentStress.isEmpty()) {
        QJsonArray arr;
        for (double s : recentStress) arr.append(s);
        o[QStringLiteral("recentStress")] = arr;
    }

    // HRV readiness
    if (hrvAvailable) {
        QJsonObject hrv;
        hrv[QStringLiteral("rmssd")] = hrvRMSSD;
        hrv[QStringLiteral("baseline")] = hrvBaseline;
        hrv[QStringLiteral("ratio")] = hrvRatio;
        o[QStringLiteral("hrv")] = hrv;
    }
    o[QStringLiteral("hrvAvailable")] = hrvAvailable;

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

        // TSB trend: 7-day delta (positive = freshening up)
        double tsbNow = snapshot.tsb;
        double tsb7ago = pmc->sb(snapshot.date.addDays(-7));
        snapshot.tsbTrend = tsbNow - tsb7ago;

        // CTL ramp rate over last 14 days (change per week)
        double ctl14ago = pmc->lts(snapshot.date.addDays(-14));
        snapshot.ctlRampRate = (snapshot.ctl - ctl14ago) / 2.0;

        // Recent 7 days of daily stress for monotony/constraint checks
        snapshot.recentStress.resize(7);
        for (int i = 0; i < 7; i++) {
            QDate d = snapshot.date.addDays(-(7 - i));
            snapshot.recentStress[i] = pmc->stress(d);
        }
    }

    // Days to next event from Seasons
    Seasons *seasons = context->athlete->seasons;
    if (seasons) {
        int closestDays = -1;
        QString closestName;
        for (const Season &s : seasons->seasons) {
            for (const SeasonEvent &e : s.events) {
                int days = static_cast<int>(snapshot.date.daysTo(e.date));
                if (days >= 0 && (closestDays < 0 || days < closestDays)) {
                    closestDays = days;
                    closestName = e.name;
                }
            }
        }
        snapshot.daysToEvent = closestDays;
        snapshot.nextEventName = closestName;
    }

    // HRV readiness from Measures system
    Measures *measures = context->athlete->measures;
    if (measures) {
        MeasuresGroup *hrvGroup = measures->getGroup(Measures::Hrv);
        if (hrvGroup) {
            // Field 0 = RMSSD in the hardcoded HRV group
            double todayRMSSD = hrvGroup->getFieldValue(snapshot.date, 0);
            if (todayRMSSD > 0.0) {
                snapshot.hrvRMSSD = todayRMSSD;

                // Compute 7-day rolling baseline (average of prior 7 days with data)
                double sum = 0.0;
                int count = 0;
                for (int i = 1; i <= 7; i++) {
                    double val = hrvGroup->getFieldValue(snapshot.date.addDays(-i), 0);
                    if (val > 0.0) {
                        sum += val;
                        count++;
                    }
                }
                if (count >= 3) {
                    // Need at least 3 days of baseline data to be meaningful
                    snapshot.hrvBaseline = sum / count;
                    snapshot.hrvRatio = todayRMSSD / snapshot.hrvBaseline;
                    snapshot.hrvAvailable = true;
                }
            }
        }
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

    // trainDB's SQLite connection lives on the main thread; when called from
    // an HTTP-handler thread we must dispatch the import there.
    bool imported = false;
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        trainDB->startLUW();
        imported = trainDB->importWorkout(filepath, *ergFile);
        trainDB->endLUW();
    } else {
        ErgFile *ef = ergFile;              // captured by the lambda
        QString fp  = filepath;
        QMetaObject::invokeMethod(trainDB, [&imported, fp, ef]() {
            trainDB->startLUW();
            imported = trainDB->importWorkout(fp, *ef);
            trainDB->endLUW();
        }, Qt::BlockingQueuedConnection);
    }
    delete ergFile;

    if (!imported) {
        errors << QStringLiteral("Workout saved but failed to import into library");
        return false;
    }

    ensureWorkoutReferenceOnMainThread(context, filepath);

    if (savedPath) {
        *savedPath = filepath;
    }
    return true;
}

bool
WorkoutGenerationService::createPlannedActivity(Context *context, const QString &workoutPath,
                                                const QDateTime &when, const QString &sport,
                                                const QString &title, const QString &description,
                                                QString *savedPath, QStringList &errors)
{
    if (!context || !context->athlete || !context->athlete->rideCache) {
        errors << QStringLiteral("Invalid context");
        return false;
    }

    QString cleanedWorkoutPath = workoutPath.trimmed();
    if (cleanedWorkoutPath.isEmpty()) {
        errors << QStringLiteral("workoutPath is required");
        return false;
    }

    QFileInfo workoutInfo(cleanedWorkoutPath);
    if (!workoutInfo.exists() || !workoutInfo.isFile()) {
        errors << QStringLiteral("Workout file does not exist");
        return false;
    }

    QDateTime effectiveWhen = when.isValid()
        ? when
        : QDateTime(QDate::currentDate(), QTime(6, 0, 0));
    if (!effectiveWhen.isValid()) {
        errors << QStringLiteral("Invalid planned activity date/time");
        return false;
    }

    QString plannedDirPath = context->athlete->home->planned().canonicalPath();
    if (plannedDirPath.isEmpty()) {
        errors << QStringLiteral("Planned activity directory is not available");
        return false;
    }

    QString plannedFileName = plannedActivityBasename(effectiveWhen) + QStringLiteral(".json");
    QString plannedPath = plannedDirPath + QLatin1Char('/') + plannedFileName;
    if (QFile::exists(plannedPath)) {
        errors << QStringLiteral("There is already a planned activity at the requested time");
        return false;
    }

    ErgFile ergFile(cleanedWorkoutPath, ErgFileFormat::unknown, context, effectiveWhen.date());
    if (!ergFile.isValid()) {
        errors << QStringLiteral("Saved workout could not be parsed");
        return false;
    }

    RideFile rideFile;
    rideFile.setStartTime(effectiveWhen);
    rideFile.setRecIntSecs(0.00);
    rideFile.setDeviceType(QStringLiteral("Manual"));
    rideFile.setFileFormat(QStringLiteral("GoldenCheetah Json"));
    rideFile.setTag(QStringLiteral("Original Date"), effectiveWhen.date().toString(QStringLiteral("yyyy/MM/dd")));
    rideFile.setTag(QStringLiteral("Year"), effectiveWhen.toString(QStringLiteral("yyyy")));
    rideFile.setTag(QStringLiteral("Month"), effectiveWhen.toString(QStringLiteral("MMMM")));
    rideFile.setTag(QStringLiteral("Weekday"), effectiveWhen.toString(QStringLiteral("ddd")));
    rideFile.setTag(QStringLiteral("Sport"), normalizedSport(sport));
    rideFile.setTag(QStringLiteral("WorkoutFilename"), normalizedWorkoutReference(cleanedWorkoutPath));

    QString resolvedTitle = title.trimmed();
    if (resolvedTitle.isEmpty()) {
        resolvedTitle = ergFile.name().trimmed();
    }
    if (resolvedTitle.isEmpty()) {
        resolvedTitle = workoutInfo.completeBaseName();
    }
    if (!resolvedTitle.isEmpty()) {
        rideFile.setTag(QStringLiteral("Route"), resolvedTitle);
        rideFile.setTag(QStringLiteral("Workout Code"), resolvedTitle);
    }

    QString resolvedDescription = description.trimmed();
    if (resolvedDescription.isEmpty()) {
        resolvedDescription = ergFile.description().trimmed();
    }
    if (!resolvedDescription.isEmpty()) {
        rideFile.setTag(QStringLiteral("Notes"), resolvedDescription);
    }

    int durationSec = static_cast<int>(std::lround(ergFile.duration() / 1000.0));
    setMetricOverride(rideFile, QStringLiteral("workout_time"), durationSec);
    setMetricOverride(rideFile, QStringLiteral("time_riding"), durationSec);
    setMetricOverride(rideFile, QStringLiteral("average_power"), static_cast<int>(std::lround(ergFile.AP())));
    setMetricOverride(rideFile, QStringLiteral("coggan_np"), static_cast<int>(std::lround(ergFile.IsoPower())));
    setMetricOverride(rideFile, QStringLiteral("coggan_tss"), static_cast<int>(std::lround(ergFile.bikeStress())));
    setMetricOverride(rideFile, QStringLiteral("skiba_bike_score"), static_cast<int>(std::lround(ergFile.BS())));
    setMetricOverride(rideFile, QStringLiteral("skiba_xpower"), static_cast<int>(std::lround(ergFile.XP())));
    setMetricOverride(rideFile, QStringLiteral("elevation_gain"), static_cast<int>(std::lround(ergFile.ele())));

    GlobalContext::context()->rideMetadata->setLinkedDefaults(&rideFile);

    QFile out(plannedPath);
    if (!RideFileFactory::instance().writeRideFile(context, &rideFile, out, QStringLiteral("json"))) {
        errors << QStringLiteral("Failed to write planned activity");
        return false;
    }

    // addRide / rideCache operations touch main-thread Qt objects; dispatch
    // there when called from an HTTP-handler thread.
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        context->athlete->addRide(plannedFileName, true, false, false, true);
        RideItem *plannedItem = context->athlete->rideCache->getRide(plannedFileName, true);
        if (plannedItem) {
            context->athlete->rideCache->updateFromWorkout(plannedItem, true);
        }
    } else {
        Context *ctx = context;
        QString pfn  = plannedFileName;
        QMetaObject::invokeMethod(QCoreApplication::instance(), [ctx, pfn]() {
            ctx->athlete->addRide(pfn, true, false, false, true);
            RideItem *plannedItem = ctx->athlete->rideCache->getRide(pfn, true);
            if (plannedItem) {
                ctx->athlete->rideCache->updateFromWorkout(plannedItem, true);
            }
        }, Qt::BlockingQueuedConnection);
    }

    if (savedPath) {
        *savedPath = plannedPath;
    }
    return true;
}

bool
WorkoutGenerationService::deletePlannedActivity(Context *context, const QString &filepath,
                                                QStringList &errors)
{
    if (!context || !context->athlete || !context->athlete->rideCache) {
        errors << QStringLiteral("Invalid context");
        return false;
    }

    QFileInfo fi(filepath);
    if (!fi.exists()) {
        errors << QStringLiteral("Planned activity file not found: %1").arg(filepath);
        return false;
    }

    QString filename = fi.fileName();

    // Verify this is actually a planned activity in the ride cache
    RideItem *item = context->athlete->rideCache->getRide(filename, true);
    if (!item) {
        errors << QStringLiteral("Planned activity not found in ride cache: %1").arg(filename);
        return false;
    }

    // removeRide touches Qt UI objects (model updates, notifications);
    // dispatch to the main thread when called from an HTTP-handler thread.
    bool ok = false;
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        ok = context->athlete->rideCache->removeRide(filename);
    } else {
        Context *ctx = context;
        QString fn = filename;
        QMetaObject::invokeMethod(QCoreApplication::instance(), [ctx, fn, &ok]() {
            ok = ctx->athlete->rideCache->removeRide(fn);
        }, Qt::BlockingQueuedConnection);
    }

    if (!ok) {
        errors << QStringLiteral("Failed to remove planned activity from ride cache");
        return false;
    }

    return true;
}

bool
WorkoutGenerationService::updatePlannedActivity(Context *context, const QString &filepath,
                                                const QDateTime &newWhen,
                                                const QString &sport, const QString &title,
                                                const QString &description,
                                                const QString &workoutPath,
                                                QString *updatedPath, QStringList &errors)
{
    if (!context || !context->athlete || !context->athlete->rideCache) {
        errors << QStringLiteral("Invalid context");
        return false;
    }

    QFileInfo fi(filepath);
    if (!fi.exists()) {
        errors << QStringLiteral("Planned activity file not found: %1").arg(filepath);
        return false;
    }

    QString filename = fi.fileName();
    RideItem *item = context->athlete->rideCache->getRide(filename, true);
    if (!item) {
        errors << QStringLiteral("Planned activity not found in ride cache: %1").arg(filename);
        return false;
    }

    // If date/time is changing, use moveActivity to rename the file properly
    bool dateChanged = newWhen.isValid() && newWhen != item->dateTime;
    const QString normalizedWorkoutPath = normalizedWorkoutReference(workoutPath);

    // Apply metadata changes via ride tags
    auto applyMetadata = [&](RideItem *ri) {
        RideFile *ride = ri->ride(true);
        if (!ride) return false;

        bool changed = false;
        if (!sport.trimmed().isEmpty()) {
            ride->setTag(QStringLiteral("Sport"), normalizedSport(sport));
            changed = true;
        }
        if (!title.trimmed().isEmpty()) {
            ride->setTag(QStringLiteral("Route"), title.trimmed());
            ride->setTag(QStringLiteral("Workout Code"), title.trimmed());
            changed = true;
        }
        if (!description.trimmed().isEmpty()) {
            ride->setTag(QStringLiteral("Notes"), description.trimmed());
            changed = true;
        }
        if (!normalizedWorkoutPath.isEmpty()) {
            ride->setTag(QStringLiteral("WorkoutFilename"), normalizedWorkoutPath);
            changed = true;
        }
        if (changed) ri->setDirty(true);
        return true;
    };

    // All RideCache operations must run on the main thread.
    bool ok = false;
    QString resultPath;

    auto doUpdate = [&]() {
        if (!applyMetadata(item)) {
            errors << QStringLiteral("Failed to open activity for metadata update");
            return;
        }

        if (dateChanged) {
            RideCache::OperationResult moveResult =
                context->athlete->rideCache->moveActivity(item, newWhen);
            if (!moveResult.success) {
                errors << moveResult.error;
                return;
            }
        }

        if (item->isDirty()) {
            QString saveError;
            context->athlete->rideCache->saveActivity(item, saveError);
            if (!saveError.isEmpty()) {
                errors << saveError;
                return;
            }
        }

        if (!normalizedWorkoutPath.isEmpty()) {
            ensureWorkoutReference(context, normalizedWorkoutPath);
        }

        QString dir = context->athlete->home->planned().canonicalPath();
        resultPath = dir + QLatin1Char('/') + item->fileName;
        ok = true;
    };

    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        doUpdate();
    } else {
        QMetaObject::invokeMethod(QCoreApplication::instance(), doUpdate,
                                  Qt::BlockingQueuedConnection);
    }

    if (ok && updatedPath) {
        *updatedPath = resultPath;
    }
    return ok;
}

bool
WorkoutGenerationService::deleteWorkout(Context *context, const QString &filepath,
                                        QStringList &errors)
{
    Q_UNUSED(context);

    QFileInfo fi(filepath);
    if (!fi.exists()) {
        errors << QStringLiteral("Workout file not found: %1").arg(filepath);
        return false;
    }

    QString suffix = fi.suffix().toLower();
    if (suffix != QLatin1String("erg") && suffix != QLatin1String("mrc") && suffix != QLatin1String("zwo")) {
        errors << QStringLiteral("Expected a workout file (.erg, .mrc, or .zwo)");
        return false;
    }

    QString absPath = fi.absoluteFilePath();

    // Remove from TrainDB (SQLite connection lives on main thread)
    auto doDelete = [&]() {
        trainDB->startLUW();
        trainDB->deleteWorkout(absPath);
        trainDB->endLUW();
    };

    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        doDelete();
    } else {
        QMetaObject::invokeMethod(QCoreApplication::instance(), doDelete,
                                  Qt::BlockingQueuedConnection);
    }

    // Delete the file from disk
    if (!QFile::remove(absPath)) {
        errors << QStringLiteral("Failed to delete workout file: %1").arg(absPath);
        return false;
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
