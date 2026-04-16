/*
 * Copyright (c) 2026 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "PlanBundle.h"

#include "Athlete.h"
#include "AthleteTab.h"
#include "TrainDB.h"
#include "ErgFile.h"

#include "../qzip/zipreader.h"
#include "../qzip/zipwriter.h"

static std::pair<int, int> writeActivities(Context *context, const PlanExportDescription &description, const QDir &activityWorkDir, const QDir &workoutWorkDir, PlanMetadata &metadata);
static bool packDir(const QDir &workDir, const QString &dirName, ZipWriter &zipWriter);
static QString hashFile(const QString &filePath);
static QString updateTags(RideFile *rideFile, const QDate &newDate, const QString &planName);
static bool scaleOverride(QMap<QString, QMap<QString, QString>> &overrides, const QString &key, int fromCP, int toCP);

static constexpr int baselinePower = 300;
static constexpr int md5HashLength = 32;


////////////////////////////////////////////////////////////////////////////////
// PlanMetadata


const QString PlanMetadata::ManifestFilename = "manifest.json";
const QString PlanMetadata::ReadmeFilename = "README.md";


void
PlanMetadata::reset
()
{
    bundleVersion = -1;
    name = QString();
    author = QString();
    sport = QString();
    copyright = QString();
    description = QString();
    exportedAt = QDateTime();
    durationDays = 0;
    frontGapDays = 0;
    backGapDays = 0;
}


bool
PlanMetadata::isValid
() const
{
    return    bundleVersion == 1
           && durationDays > 0
           && frontGapDays >= 0
           && backGapDays >= 0
           && exportedAt.isValid();
}


QList<QString>
PlanMetadata::save
(const QDir &dir) const
{
    QList<QString> filePaths;
    QFile manifestFile(dir.filePath(ManifestFilename));
    if (manifestFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(toJson());
        manifestFile.write(doc.toJson(QJsonDocument::Indented));
        manifestFile.close();
        filePaths << ManifestFilename;

        if (description.trimmed().length() > 0) {
            QFile readmeFile(dir.filePath(ReadmeFilename));
            if (readmeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&readmeFile);
                out << description;
                readmeFile.close();
                filePaths << ReadmeFilename;
            }
        }
    }
    return filePaths;
}


bool
PlanMetadata::load
(const QDir &dir)
{
    QFile manifestFile(dir.filePath(ManifestFilename));
    if (! manifestFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray manifestData = manifestFile.readAll();
    manifestFile.close();
    QJsonParseError manifestParseError;
    QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestData, &manifestParseError);
    if (manifestParseError.error != QJsonParseError::NoError) {
        return false;
    }
    fromJson(manifestDoc.object());
    if (! isValid()) {
        return false;
    }

    QFile readmeFile(dir.filePath(ReadmeFilename));
    if (readmeFile.open(QIODevice::ReadOnly)) {
        QByteArray readmeData = readmeFile.readAll();
        readmeFile.close();
        description = QString(readmeData);
    }

    return true;
}


QJsonObject
PlanMetadata::toJson
() const
{
    QJsonObject obj;
    obj["bundleVersion"] = bundleVersion;
    obj["name"] = name;
    obj["author"] = author;
    obj["sport"] = sport;
    obj["exportedAt"] = exportedAt.toString(Qt::ISODate);
    obj["durationDays"] = durationDays;
    obj["frontGapDays"] = frontGapDays;
    obj["backGapDays"] = backGapDays;
    obj["copyright"] = copyright;
    return obj;
}


bool
PlanMetadata::fromJson
(const QJsonObject &obj)
{
    bool ok = false;
    reset();
    if (! obj.contains("bundleVersion")) {
        return false;
    }
    int bv = obj["bundleVersion"].toInt();
    if (bv == 1) {
        if (   ! obj.contains("author")
            || ! obj.contains("name")) {
            return false;
        }
        bundleVersion = bv;
        name = obj["name"].toString().trimmed();
        author = obj["author"].toString().trimmed();
        sport = obj["sport"].toString().trimmed();
        exportedAt = QDateTime::fromString(obj["exportedAt"].toString().trimmed(), Qt::ISODate);
        durationDays = obj["durationDays"].toInt(0);
        frontGapDays = obj["frontGapDays"].toInt(0);
        backGapDays = obj["backGapDays"].toInt(0);
        copyright = obj["copyright"].toString().trimmed();
        ok = true;
    }
    return ok;
}


////////////////////////////////////////////////////////////////////////////////
// PlanResult

bool
PlanResult::ok
() const
{
    return errors.isEmpty();
}


void
PlanResult::addError
(const QString &error)
{
    errors << error;
}


void
PlanResult::addWarning
(const QString &warning)
{
    warnings << warning;
}


void
PlanResult::reset
()
{
    errors.clear();
    warnings.clear();
}


////////////////////////////////////////////////////////////////////////////////
// RideFileSelection

RideFileSelection::RideFileSelection
(RideFile *rideFile, bool selected, const QDateTime &dt)
: selected(selected), targetDateTime(dt), rideFile(rideFile)
{
}


RideFile*
RideFileSelection::getRideFile
() const
{
    return rideFile;
}


////////////////////////////////////////////////////////////////////////////////
// PlanBundleReader

PlanBundleReader::PlanBundleReader
(Context *context, const QDate &targetDate)
: context(context), targetRangeStart(targetDate)
{
}


PlanBundleReader::~PlanBundleReader
()
{
    reset();
}


PlanMetadata
PlanBundleReader::getMetadata
() const
{
    return metadata;
}


PlanResult
PlanBundleReader::loadBundle
(const QString &bundlePath)
{
    reset();
    lastValidationResult.reset();

    QFileInfo fileInfo(bundlePath);
    if (! fileInfo.isFile()) {
        lastValidationResult.addError(QObject::tr("Path is not a file"));
        return lastValidationResult;
    }
    if (! fileInfo.isReadable()) {
        lastValidationResult.addError(QObject::tr("File is not readable"));
        return lastValidationResult;
    }

    tempDir = new QTemporaryDir();
    if (! tempDir->isValid()) {
        lastValidationResult.addError(QObject::tr("Failed to create temporary directory"));
        return lastValidationResult;
    }
    QDir baseDir(tempDir->path());

    ZipReader zipReader(bundlePath);
    if (! zipReader.extractAll(baseDir.absolutePath())) {
        lastValidationResult.addError(QObject::tr("Failed to unpack bundle"));
        reset();
        return lastValidationResult;
    }
    if (! metadata.load(baseDir)) {
        lastValidationResult.addError(QObject::tr("Failed to load metadata"));
        reset();
        return lastValidationResult;
    }
    plannedDir.setPath(baseDir.filePath("planned"));
    workoutDir.setPath(baseDir.filePath("workouts"));
    if (! plannedDir.exists() || ! workoutDir.exists()) {
        lastValidationResult.addError(QObject::tr("Invalid bundle structure"));
        reset();
        return lastValidationResult;
    }

    QFileInfoList plannedList = plannedDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : plannedList) {
        QFile sourceFile(fileInfo.absoluteFilePath());
        QStringList errors;
        RideFile *rideFile = RideFileFactory::instance().openRideFile(context, sourceFile, errors);
        if (! rideFile) {
            lastValidationResult.addError(QObject::tr("Failed to load activity %1").arg(fileInfo.fileName()));
            reset();
            return lastValidationResult;
        }
        rideFiles << RideFileSelection { rideFile, true, rideFile->startTime() };
    }
    if (rideFiles.count() == 0) {
        lastValidationResult.addWarning(QObject::tr("No activities in bundle"));
    }
    std::sort(rideFiles.begin(), rideFiles.end(), [](const RideFileSelection &a, const RideFileSelection &b) {
        return a.getRideFile()->startTime() < b.getRideFile()->startTime();
    });
    calculateRange();

    validate();
    if (! lastValidationResult.ok()) {
        reset();
        return lastValidationResult;
    }

    for (RideFileSelection &rideFileSelection : rideFiles) {
        rideFileSelection.targetDateTime = rideFileSelection.targetDateTime.addDays(daysToAdd);
    }

    return lastValidationResult;
}


bool
PlanBundleReader::isValid
() const
{
    return ! isNull() && lastValidationResult.ok();
}


bool
PlanBundleReader::isNull
() const
{
    return tempDir == nullptr;
}


PlanResult
PlanBundleReader::importBundle
()
{
    lastImportResult.reset();
    if (isNull()) {
        lastImportResult.addError(QObject::tr("No bundle was loaded"));
        return lastImportResult;
    }
    if (! isValid()) {
        lastImportResult.addError(QObject::tr("Current bundle is invalid"));
        return lastImportResult;
    }

    findConflicts();
    if (existingLinked.count() > 0) {
        for (const RideFileSelection &entry : rideFiles) {
            if (entry.selected && existingLinked.contains(entry.getRideFile()->startTime())) {
                lastImportResult.addError(QObject::tr("Bundle can't be imported: Conflicts with linked planned activity on %1").arg(entry.getRideFile()->startTime().toString()));
                return lastImportResult;
            }
        }
    }
    context->athlete->rideCache->removeRides(toDelete, false);
    for (const RideFileSelection &entry : rideFiles) {
        if (entry.selected) {
            if (! processWorkout(entry.getRideFile())) {
                lastImportResult.addWarning(QObject::tr("Failed to process the attached workout"));
            }
            if (cleanAndCopyActivity(entry.getRideFile())) {
                context->athlete->rideCache->addRide(entry.getRideFile()->getTag("Filename", ""), true, false, false, true);
            } else {
                lastImportResult.addError(QObject::tr("Failed to import activity"));
                return lastImportResult;
            }
        }
    }
    context->athlete->rideCache->refresh();

    return lastImportResult;
}


QDate
PlanBundleReader::getTargetRangeStart
() const
{
    return targetRangeStart;
}


QDate
PlanBundleReader::getTargetRangeEnd
() const
{
    return targetRangeEnd;
}


int
PlanBundleReader::getDuration
() const
{
    return duration;
}


int
PlanBundleReader::getNumActivities
() const
{
    return rideFiles.count();
}


int
PlanBundleReader::getNumSelectedActivities
() const
{
    return std::count_if(rideFiles.begin(), rideFiles.end(), [](const RideFileSelection &entry) {
        return entry.selected;
    });
}


const QStringList&
PlanBundleReader::getActivitiesToRemove
() const
{
    return toDelete;
}


QList<RideItem*>
PlanBundleReader::getRideItemsToRemove
() const
{
    QList<RideItem*> rideItems;
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem == nullptr
            || ! rideItem->planned
            || rideItem->dateTime.date() < targetRangeStart
            || rideItem->dateTime.date() > targetRangeEnd) {
            continue;
        }
        if (! rideItem->hasLinkedActivity()) {
            rideItems << rideItem;
        }
    }
    return rideItems;
}


bool
PlanBundleReader::isIncludeGapDays
() const
{
    return includeGapDays;
}


void
PlanBundleReader::setIncludeGapDays
(bool includeGapDays)
{
    this->includeGapDays = includeGapDays;
    calculateRange();
}


const QSet<QDateTime>&
PlanBundleReader::getExistingLinked
() const
{
    return existingLinked;
}


PlanResult
PlanBundleReader::getLastValidationResult
() const
{
    return lastValidationResult;
}


PlanResult
PlanBundleReader::getLastImportResult
() const
{
    return lastImportResult;
}


void
PlanBundleReader::reset
()
{
    if (tempDir != nullptr) {
        delete tempDir;
        tempDir = nullptr;
    }
    metadata.reset();
    for (RideFileSelection &entry : rideFiles) {
        delete entry.getRideFile();
    }
    rideFiles.clear();
    targetRangeEnd = QDate();
    duration = 0;
    daysToAdd = 0;
    plannedDir = QDir();
    workoutDir = QDir();
    trainDBHashes.clear();
    toDelete.clear();
    existingLinked.clear();
}


void
PlanBundleReader::calculateRange
()
{
    if (rideFiles.count() == 0) {
        return;
    }

    QDate firstActivityDate = rideFiles.first().getRideFile()->startTime().date();
    QDate lastActivityDate = rideFiles.last().getRideFile()->startTime().date();

    if (includeGapDays) {
        int activitySpan = firstActivityDate.daysTo(lastActivityDate) + 1;
        duration = activitySpan + metadata.frontGapDays + metadata.backGapDays;
        daysToAdd = firstActivityDate.daysTo(targetRangeStart) + metadata.frontGapDays;
    } else {
        QDate firstSelected;
        QDate lastSelected;
        for (const RideFileSelection &ride : rideFiles) {
            if (! ride.selected) {
                continue;
            }
            QDate rideDate = ride.getRideFile()->startTime().date();
            if (! firstSelected.isValid() || rideDate < firstSelected) {
                firstSelected = rideDate;
            }
            if (! lastSelected.isValid() || rideDate > lastSelected) {
                lastSelected = rideDate;
            }
        }
        if (! firstSelected.isValid()) {
            duration = 0;
            targetRangeEnd = targetRangeStart;
            findConflicts();
            return;
        }
        duration = firstSelected.daysTo(lastSelected) + 1;
        daysToAdd = firstSelected.daysTo(targetRangeStart);
    }
    targetRangeEnd = targetRangeStart.addDays(duration - 1);

    findConflicts();
}



void
PlanBundleReader::findConflicts
()
{
    toDelete.clear();
    existingLinked.clear();
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem == nullptr
            || ! rideItem->planned
            || rideItem->dateTime.date() < targetRangeStart
            || rideItem->dateTime.date() > targetRangeEnd) {
            continue;
        }
        if (! rideItem->hasLinkedActivity()) {
            toDelete << rideItem->fileName;
        } else {
            existingLinked << rideItem->dateTime;
        }
    }
}


bool
PlanBundleReader::cleanAndCopyActivity
(RideFile *rideFile) const
{
    QString targetFileName = updateTags(rideFile, rideFile->startTime().date().addDays(daysToAdd), metadata.name);

    Zones const * const zones = context->athlete->zones(rideFile->getTag("Sport", ""));
    if (zones != nullptr) {
        int zonerange = zones->whichRange(rideFile->startTime().date());
        int cp = zones->getCP(zonerange);
        scaleOverride(rideFile->metricOverrides, "average_power", baselinePower, cp);
        scaleOverride(rideFile->metricOverrides, "coggan_np", baselinePower, cp);
        scaleOverride(rideFile->metricOverrides, "skiba_xpower", baselinePower, cp);
    }

    QString targetPath = context->athlete->home->planned().canonicalPath() + "/" + targetFileName;
    QFile targetFile(targetPath);
    if (! targetFile.exists()) {
        if (! RideFileFactory::instance().writeRideFile(context, rideFile, targetFile, "json")) {
            qWarning() << QString("Failed to write activity %1").arg(targetFileName);
            return false;
        }
    } else {
        qWarning() << QString("Activity %1 already exists; skipping import").arg(targetFileName);
        return false;
    }
    return true;
}


bool
PlanBundleReader::processWorkout
(RideFile *rideFile)
{
    QString workoutFilename = rideFile->getTag("WorkoutFilename", "");
    if (workoutFilename.isEmpty()) {
        return true;
    }
    if (workoutFilename.length() >= md5HashLength + 1 && workoutFilename[md5HashLength] == '-') {
        QString hash = workoutFilename.left(md5HashLength);
        QString origFilename = workoutFilename.mid(md5HashLength + 1);
        if (trainDBHashes.count() == 0) {
            trainDBHashes = trainDB->getWorkoutHashes();
        }
        if (trainDBHashes.contains(hash)) {
            rideFile->setTag("WorkoutFilename", trainDBHashes.value(hash));
        } else {
            QString gcWorkoutDir = appsettings->value(nullptr, GC_WORKOUTDIR).toString();
            if (gcWorkoutDir == "") {
                QDir root = context->athlete->home->root();
                root.cdUp();
                gcWorkoutDir = root.absolutePath();
            }
            QFile targetWorkoutFile(gcWorkoutDir + "/" + origFilename);
            if (targetWorkoutFile.exists()) {
                QFileInfo targetWorkoutFileInfo(targetWorkoutFile);
                QString targetWorkoutExtension = "." + targetWorkoutFileInfo.suffix();
                targetWorkoutFile.setFileName(gcWorkoutDir + "/" + hash + targetWorkoutExtension);
                if (targetWorkoutFile.exists()) {
                    targetWorkoutFile.setFileName(gcWorkoutDir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + targetWorkoutExtension);
                }
            }
            QFile workout(workoutDir.filePath(workoutFilename));
            if (workout.copy(targetWorkoutFile.fileName())) {
                trainDB->startLUW();
                ErgFile ergFile(targetWorkoutFile.fileName(), ErgFileFormat::unknown, context);
                trainDB->importWorkout(targetWorkoutFile.fileName(), ergFile);
                trainDB->endLUW();
                rideFile->setTag("WorkoutFilename", targetWorkoutFile.fileName());
                trainDBHashes.insert(hash, targetWorkoutFile.fileName());
            } else {
                return false;
            }
        }
    } else {
        qWarning("Linked workout has invalid name schema; removing link");
        rideFile->removeTag("WorkoutFilename");
        return false;
    }
    return true;
}


void
PlanBundleReader::validate
()
{
    // Not checked (done while loading):
    // 1. Is the metadata valid?
    // 2. Is at least one activity available?
    // 3. Are all activities valid?
    // 4. Are all folders available?
    // 5. Are non-activity files in the planned-folder?

    if (tempDir == nullptr) {
        lastValidationResult.addError(QObject::tr("No temporary directory set"));
        return;
    }
    QDir baseDir(tempDir->path());

    // Checked:
    // 1. Are additional folders / files present?
    // 4. Are all workouts readable?
    QStringList baseDirNames;
    for (const QFileInfo &fi : baseDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        baseDirNames.append(fi.fileName());
    }
    const QStringList mandatoryEntries = { "planned", "workouts", PlanMetadata::ManifestFilename };
    const QStringList permittedEntries = mandatoryEntries + QStringList { PlanMetadata::ReadmeFilename };

    for (const QString &mandatory : mandatoryEntries) {
        if (! baseDirNames.contains(mandatory)) {
            lastValidationResult.addError(QObject::tr("Bundle is missing required entry: %1").arg(mandatory));
            return;
        }
    }
    for (const QString &entry : baseDirNames) {
        if (! permittedEntries.contains(entry)) {
            lastValidationResult.addError(QObject::tr("Bundle contains unexpected entry: %1").arg(entry));
            return;
        }
    }

    QFileInfoList plannedDirList = plannedDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    if (plannedDirList.count() != rideFiles.count()) {
        lastValidationResult.addError(QObject::tr("Mismatch between the manifest and available activities"));
        return;
    }

    QFileInfoList workoutDirList = workoutDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo &fileInfo : workoutDirList) {
        if (fileInfo.exists() && fileInfo.isFile()) {
            ErgFile ergFile(workoutDir.filePath(fileInfo.fileName()), ErgFileFormat::unknown, context);
            if (! ergFile.isValid()) {
                lastValidationResult.addError(QObject::tr("Invalid workout file %1 in bundle").arg(fileInfo.fileName()));
                return;
            }
            // 2. Do all workout-filenames match the format?
            // 5. Do all workouts match their hash?
            QString hash = hashFile(workoutDir.filePath(fileInfo.fileName()));
            if (   hash.length() != md5HashLength
                || fileInfo.fileName().length() <= hash.length() + 2
                || ! fileInfo.fileName().startsWith(hash + "-")) {
                lastValidationResult.addError(QObject::tr("Bad hash or filename for workout file (%1) in bundle").arg(fileInfo.fileName()));
                return;
            }
        } else {
            lastValidationResult.addError(QObject::tr("Non-file %1 given in workout folder").arg(fileInfo.fileName()));
            return;
        }
    }

    // 3. Are all workouts linked from an activity?
    // 6. Are workouts linked from an activity missing?
    QSet<QString> workouts;
    for (const RideFileSelection &entry : rideFiles) {
        QString workoutFilename = entry.getRideFile()->getTag("WorkoutFilename", "");
        if (workoutFilename.isEmpty()) {
            continue;
        }
        if (! QFile::exists(workoutDir.filePath(workoutFilename))) {
            lastValidationResult.addError(QObject::tr("Missing workout %1 linked from activity %2").arg(workoutFilename).arg(PlanBundle::getRideName(entry.getRideFile())));
            return;
        }
        workouts.insert(workoutFilename);
    }
    if (workouts.count() != workoutDirList.count()) {
        lastValidationResult.addError(QObject::tr("Bundle contains unused workouts"));
        return;
    }

    int expectedDuration = 0;
    if (rideFiles.count() > 0) {
        QDate firstActivityDate = rideFiles.first().getRideFile()->startTime().date();
        QDate lastActivityDate = rideFiles.last().getRideFile()->startTime().date();
        int activitySpan = firstActivityDate.daysTo(lastActivityDate) + 1;
        expectedDuration = activitySpan + metadata.frontGapDays + metadata.backGapDays;
    }
    if (expectedDuration != metadata.durationDays) {
        lastValidationResult.addError(QObject::tr("Duration mismatch between manifest and activities"));
        return;
    }

    return;
}


////////////////////////////////////////////////////////////////////////////////
// Namespace PlanBundle


bool
PlanBundle::exportBundle
(Context *context, const PlanExportDescription &description)
{
    QTemporaryDir tempDir;
    tempDir.setAutoRemove(true);

    QDir baseDir(tempDir.path());
    baseDir.mkdir("planned");
    baseDir.mkdir("workouts");
    QDir plannedDir(baseDir.filePath("planned"));
    QDir workoutDir(baseDir.filePath("workouts"));

    PlanMetadata metadata;
    metadata.bundleVersion = 1;
    metadata.name = description.name;
    metadata.author = description.author;
    metadata.sport = description.sport;
    metadata.description = description.description;
    metadata.exportedAt = QDateTime::currentDateTime();
    metadata.copyright = description.copyright;

    std::pair<int, int> copied = writeActivities(context, description, plannedDir, workoutDir, metadata);
    if (copied.first > 0) {
        ZipWriter zipWriter(description.planFile);
        if (zipWriter.status() != ZipWriter::NoError) {
            return false;
        }
        zipWriter.setCompressionPolicy(ZipWriter::AutoCompress);

        if (! packDir(plannedDir, "planned", zipWriter)) {
            zipWriter.close();
            return false;
        }
        if (copied.second > 0) {
            if (! packDir(workoutDir, "workouts", zipWriter)) {
                zipWriter.close();
                return false;
            }
        }

        QList<QString> savedFiles = metadata.save(baseDir);
        for (const QString &savedFile : savedFiles) {
            QFile file(baseDir.filePath(savedFile));
            zipWriter.setCreationPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                             QFileDevice::ReadGroup |
                                             QFileDevice::ReadOther);
            zipWriter.addFile(savedFile, &file);
        }

        zipWriter.close();
        if (zipWriter.status() != ZipWriter::NoError) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}


QString
PlanBundle::getRideName
(RideItem const * const rideItem)
{
    QString rideName = rideItem->getText("Route", "").trimmed();
    if (rideName.isEmpty()) {
        rideName = rideItem->getText("Workout Code", "").trimmed();
        if (rideName.isEmpty()) {
            rideName = QObject::tr("<unnamed>");
        }
    }
    return rideName;
}


QString
PlanBundle::getRideName
(RideFile const * const rideFile)
{
    QString rideName = rideFile->getTag("Route", "").trimmed();
    if (rideName.isEmpty()) {
        rideName = rideFile->getTag("Workout Code", "").trimmed();
        if (rideName.isEmpty()) {
            rideName = QObject::tr("<unnamed>");
        }
    }
    return rideName;
}


QString
PlanBundle::getRideSport
(RideItem const * const rideItem)
{
    QString sport = rideItem->sport;
    if (! rideItem->getText("SubSport", "").isEmpty()) {
        sport += " / " + rideItem->getText("SubSport", "");
    }
    return sport;
}


QString
PlanBundle::getRideSport
(RideFile const * const rideFile)
{
    QString sport = rideFile->sport();
    if (! rideFile->getTag("SubSport", "").isEmpty()) {
        sport += " / " + rideFile->getTag("SubSport", "");
    }
    return sport;
}


QDate
PlanBundle::getRideDate
(RideItem const * const rideItem, bool preferOriginal)
{
    QDate date = rideItem->dateTime.date();
    if (preferOriginal) {
        QString originalDateString = rideItem->getText("Original Date", "");
        if (! originalDateString.isEmpty()) {
            QDate originalDate = QDate::fromString(originalDateString, "yyyy/MM/dd");
            if (originalDate.isValid()) {
                date = originalDate;
            }
        }
    }
    return date;
}


QDate
PlanBundle::getRideDate
(RideFile const * const rideFile, bool preferOriginal)
{
    QDate date = rideFile->startTime().date();
    if (preferOriginal) {
        QString originalDateString = rideFile->getTag("Original Date", "");
        if (! originalDateString.isEmpty()) {
            QDate originalDate = QDate::fromString(originalDateString, "yyyy/MM/dd");
            if (originalDate.isValid()) {
                date = originalDate;
            }
        }
    }
    return date;
}


std::pair<int, int>
writeActivities
(Context *context, const PlanExportDescription &description, const QDir &activityWorkDir, const QDir &workoutWorkDir, PlanMetadata &metadata)
{
    std::pair<int, int> ret = { 0, 0 };

    QDate firstDate;
    QDate lastDate;

    for (const QString &activityFile : description.activityFiles) {
        QString sourceFilename = context->athlete->home->planned().canonicalPath() + "/" + activityFile;
        QFile sourceFile(sourceFilename);
        QStringList errors;
        RideFile *rideFile = RideFileFactory::instance().openRideFile(context, sourceFile, errors);
        if (! rideFile) {
            continue;
        }
        QDate rideDate = PlanBundle::getRideDate(rideFile, description.preferOriginal);
        if (rideDate >= description.rangeStart && rideDate <= description.rangeEnd) {
            if (! firstDate.isValid() || ! lastDate.isValid()) {
                firstDate = rideDate;
                lastDate = rideDate;
            } else {
                if (firstDate > rideDate) {
                    firstDate = rideDate;
                }
                if (lastDate < rideDate) {
                    lastDate = rideDate;
                }
            }
        } else {
            delete rideFile;
            continue;
        }
        QString targetFileName = updateTags(rideFile, rideDate, description.name);

        QString workoutFilename = rideFile->getTag("WorkoutFilename", "");
        if (! workoutFilename.isEmpty()) {
            QFile workoutFile(workoutFilename);
            QFileInfo workoutFileInfo(workoutFile);
            QString hash = hashFile(workoutFilename);
            if (hash.length() == md5HashLength) {
                QString newName = QString("%1-%2").arg(hash).arg(workoutFileInfo.fileName());
                if (workoutFile.copy(workoutWorkDir.filePath(newName))) {
                    rideFile->setTag("WorkoutFilename", newName);
                    ++ret.second;
                } else {
                    rideFile->removeTag("WorkoutFilename");
                }
            } else {
                rideFile->removeTag("WorkoutFilename");
            }
        }
        Zones const * const zones = context->athlete->zones(rideFile->getTag("Sport", ""));
        if (zones != nullptr) {
            int zonerange = zones->whichRange(rideFile->startTime().date());
            int cp = zones->getCP(zonerange);
            scaleOverride(rideFile->metricOverrides, "average_power", cp, baselinePower);
            scaleOverride(rideFile->metricOverrides, "coggan_np", cp, baselinePower);
            scaleOverride(rideFile->metricOverrides, "skiba_xpower", cp, baselinePower);
        }

        QString targetPath = activityWorkDir.filePath(targetFileName);
        if (! QFile::exists(targetPath)) {
            QFile targetFile(targetPath);
            if (RideFileFactory::instance().writeRideFile(context, rideFile, targetFile, "json")) {
                ++ret.first;
            } else {
                qWarning() << QString("Failed to write rideFile %1").arg(targetPath);
            }
        } else {
            qWarning() << QString("RideFile %1 already exists").arg(targetPath);
        }
        delete rideFile;
    }
    if (firstDate.isValid() && lastDate.isValid()) {
        metadata.frontGapDays = description.rangeStart.daysTo(firstDate);
        metadata.backGapDays = lastDate.daysTo(description.rangeEnd);
    } else {
        metadata.frontGapDays = 0;
        metadata.backGapDays = 0;
    }
    metadata.durationDays = description.rangeStart.daysTo(description.rangeEnd) + 1;
    return ret;
}


bool
packDir
(const QDir &workDir, const QString &dirName, ZipWriter &zipWriter)
{
    QStringList fileNames = workDir.entryList(QDir::Files);
    bool ret = fileNames.count() > 0;
    if (ret) {
        zipWriter.setCreationPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                         QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                                         QFileDevice::ReadOther | QFileDevice::ExeOther);
        zipWriter.addDirectory(dirName + "/");
        for (const QString &fileName : fileNames) {
            QString fullPath = workDir.filePath(fileName);
            QFile file(fullPath);
            zipWriter.setCreationPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                             QFileDevice::ReadGroup |
                                             QFileDevice::ReadOther);
            zipWriter.addFile(dirName + "/" + fileName, &file);
        }
        if (zipWriter.status() != ZipWriter::NoError) {
            ret = false;
        }
    }
    return ret;
}


QString
hashFile
(const QString &filePath)
{
    QFile file(filePath);
    if (! file.open(QIODevice::ReadOnly)) {
        return "";
    }
    QCryptographicHash hash(QCryptographicHash::Md5);
    while (! file.atEnd()) {
        hash.addData(file.read(8192));
    }
    return hash.result().toHex();
}


QString
updateTags
(RideFile *rideFile, const QDate &newDate, const QString &planName)
{
    QDateTime newDateTime(rideFile->startTime());
    newDateTime.setDate(newDate);
    rideFile->setStartTime(newDateTime);
    rideFile->removeTag("Athlete");
    rideFile->removeTag("Month");
    rideFile->removeTag("Weekday");
    rideFile->removeTag("Year");
    rideFile->removeTag("Original Date");
    rideFile->removeTag("Linked Filename");
    rideFile->setTag("Plan", planName);
    rideFile->setTag("Change History", "");
    QString targetFileName = rideFile->startTime().toString("yyyy_MM_dd_HH_mm_ss") + ".json";
    rideFile->setTag("Filename", targetFileName);
    return targetFileName;
}


bool
scaleOverride
(QMap<QString, QMap<QString, QString>> &overrides, const QString &key, int fromCP, int toCP)
{
    if (fromCP <= 0 || toCP <= 0) {
        return false;
    }
    if (! overrides.contains(key)) {
        return false;
    }
    QMap<QString, QString> valueMap = overrides.value(key);
    if (! valueMap.contains("value")) {
        return false;
    }
    bool ok;
    int value = valueMap.value("value").toInt(&ok);
    if (ok) {
        value = static_cast<int>(std::round(static_cast<double>(value) * toCP / fromCP));
        valueMap["value"] = QString::number(value);
        overrides[key] = valueMap;
    }
    return ok;
}
