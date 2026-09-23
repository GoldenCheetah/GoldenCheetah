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

#include "CalendarSync.h"
#include "CalDAV.h"
#include "RideItem.h"
#include "RideFile.h"
#include "Season.h"


//////////////////////////////////////////////////////////////////////////////
// CalendarSync::SyncObjects

int
CalendarSync::SyncObjects::count
() const
{
    return   seasons.count()
           + phases.count()
           + events.count()
           + plannedActivities.count()
           + actualActivities.count();
}


bool
CalendarSync::SyncObjects::isRelevantForType
(CalDAV::EntryType type) const
{
    bool ret = false;
    if (type == CalDAV::EntryType::Season) {
        ret = (root == CalDAV::EntryType::Season);
    } else if (type == CalDAV::EntryType::Phase) {
        ret = (   root == CalDAV::EntryType::Season
               || root == CalDAV::EntryType::Phase);
    } else if (type == CalDAV::EntryType::Event) {
        ret = (   root == CalDAV::EntryType::Season
               || root == CalDAV::EntryType::Event);
    } else if (   type == CalDAV::EntryType::PlannedActivity
               || type == CalDAV::EntryType::ActualActivity) {
        ret = (root != CalDAV::EntryType::Event);
    }
    return ret;
}


//////////////////////////////////////////////////////////////////////////////
// CalendarSync::SyncSelections

constexpr bool
CalendarSync::SyncSelections::isSync
(SyncMode syncMode) const
{
    return syncMode == SyncMode::Sync || syncMode == SyncMode::SyncAndCleanup;
}


bool
CalendarSync::SyncSelections::isRemove
(SyncMode syncMode, QSet<QString> localIds, QString remoteId) const
{
    return    (syncMode == SyncMode::SyncAndCleanup && ! localIds.contains(remoteId))
           || syncMode == SyncMode::Remove;
}


//////////////////////////////////////////////////////////////////////////////
// CalendarSync::SyncResults

bool
CalendarSync::SyncResults::ok() const {
    return    tech.fail == 0
           && seasons.fail == 0
           && phases.fail == 0
           && events.fail == 0
           && plannedActivities.fail == 0
           && actualActivities.fail == 0;
}


CalendarSync::SyncResult
CalendarSync::SyncResults::overall
() const
{
    SyncResult ret;
    ret.fail =   tech.fail
               + seasons.fail
               + phases.fail
               + events.fail
               + plannedActivities.fail
               + actualActivities.fail;
    ret.success =   tech.success
                  + seasons.success
                  + phases.success
                  + events.success
                  + plannedActivities.success
                  + actualActivities.success;
    ret.skip =   tech.skip
               + seasons.skip
               + phases.skip
               + events.skip
               + plannedActivities.skip
               + actualActivities.skip;
    return ret;
}


//////////////////////////////////////////////////////////////////////////////
// CalendarSync::CleanupResults

bool
CalendarSync::CleanupResults::ok
() const
{
    return    tech.fail == 0
           && cleanup.fail == 0;
}


//////////////////////////////////////////////////////////////////////////////
// CalendarSync

CalendarSync::CalendarSync
(Context *context)
: context(context)
{
}


CalendarSync::SyncObjects
CalendarSync::buildObjects
(Season const *season) const
{
    SyncObjects ret;
    if (season == nullptr) {
        return ret;
    }
    ret.root = CalDAV::EntryType::Season;
    ret.seasons << season;
    for (const Phase &phase : season->phases) {
        ret.phases << &phase;
    }
    for (const SeasonEvent &event : season->events) {
        ret.events << &event;
    }
    const QList<RideItem*> rides = context->athlete->rideCache->rides();
    for (RideItem *rideItem : rides) {
        if (   rideItem == nullptr
            || ! rideItem->dateTime.isValid()) {
            continue;
        }
        QDate rideDate = rideItem->dateTime.date();
        if (   rideDate < season->getStart()
            || rideDate > season->getEnd()) {
            continue;
        }
        if (   (context->isfiltered && ! context->filters.contains(rideItem->fileName))
            || (context->ishomefiltered && ! context->homeFilters.contains(rideItem->fileName))) {
            continue;
        }
        if (rideItem->planned) {
            ret.plannedActivities << rideItem;
        } else {
            ret.actualActivities << rideItem;
        }
    }
    return ret;
}


CalendarSync::SyncObjects
CalendarSync::buildObjects
(Phase const *phase) const
{
    SyncObjects ret;
    if (phase == nullptr) {
        return ret;
    }
    ret.root = CalDAV::EntryType::Phase;
    ret.phases << phase;
    const QList<RideItem*> rides = context->athlete->rideCache->rides();
    for (RideItem *rideItem : rides) {
        if (   rideItem == nullptr
            || ! rideItem->dateTime.isValid()) {
            continue;
        }
        QDate rideDate = rideItem->dateTime.date();
        if (   rideDate < phase->getStart()
            || rideDate > phase->getEnd()) {
            continue;
        }
        if (   (context->isfiltered && ! context->filters.contains(rideItem->fileName))
            || (context->ishomefiltered && ! context->homeFilters.contains(rideItem->fileName))) {
            continue;
        }
        if (rideItem->planned) {
            ret.plannedActivities << rideItem;
        } else {
            ret.actualActivities << rideItem;
        }
    }
    return ret;
}


CalendarSync::SyncObjects
CalendarSync::buildObjects
(SeasonEvent const *event) const
{
    SyncObjects ret;
    ret.root = CalDAV::EntryType::Event;
    ret.events << event;
    return ret;
}


CalendarSync::SyncObjects
CalendarSync::buildObjects
(RideItem *rideItem) const
{
    SyncObjects ret;
    if (rideItem->planned) {
        ret.root = CalDAV::EntryType::PlannedActivity;
        ret.plannedActivities << rideItem;
    } else {
        ret.root = CalDAV::EntryType::ActualActivity;
        ret.actualActivities << rideItem;
    }
    return ret;
}


CalendarSync::SyncResults
CalendarSync::syncObjects
(const SyncObjects &objects, const SyncSelections &syncSelections, QString cloudServiceName)
{
    CloudService *service = CloudServiceFactory::instance().newService(cloudServiceName, context);
    SyncResults ret = syncObjects(objects, syncSelections, service);
    if (service != nullptr) {
        delete service;
    }
    return ret;
}


CalendarSync::SyncResults
CalendarSync::syncObjects
(const SyncObjects &objects, const SyncSelections &syncSelections, CloudService *cloudService)
{
    SyncResults ret;
    ret.seasons.skip = objects.seasons.count();
    ret.phases.skip = objects.phases.count();
    ret.events.skip = objects.events.count();
    ret.plannedActivities.skip = objects.plannedActivities.count();
    ret.actualActivities.skip = objects.actualActivities.count();
    CalDAV *caldav = new CalDAV(context, cloudService);
    int current = 0;
    int count = objects.count();
    if (! caldav->isConfigured()) {
        ++ret.tech.fail;
        ret.tech.msg << tr("CloudService for CalDAV is not configured");
    } else {
        if (! cancelRequested) {
            if (syncSelections.isSync(syncSelections.seasons)) {
                emit syncObjectType(CalDAV::EntryType::Season);
                for (const Season * const &season : objects.seasons) {
                    if (cancelRequested) {
                        break;
                    }
                    --ret.seasons.skip;
                    syncSeason(season, caldav, ret.seasons.msg) ? ++ret.seasons.success : ++ret.seasons.fail;
                    emit syncProgress(++current, count);
                }
            } else {
                current += objects.seasons.count();
                emit syncProgress(current, count);
            }
        }
        if (! cancelRequested) {
            if (syncSelections.isSync(syncSelections.phases)) {
                emit syncObjectType(CalDAV::EntryType::Phase);
                for (const Phase * const &phase : objects.phases) {
                    if (cancelRequested) {
                        break;
                    }
                    --ret.phases.skip;
                    syncPhase(phase, caldav, ret.phases.msg) ? ++ret.phases.success : ++ret.phases.fail;
                    emit syncProgress(++current, count);
                }
            } else {
                current += objects.phases.count();
                emit syncProgress(current, count);
            }
        }
        if (! cancelRequested) {
            if (syncSelections.isSync(syncSelections.events)) {
                emit syncObjectType(CalDAV::EntryType::Event);
                for (const SeasonEvent * const &event : objects.events) {
                    if (cancelRequested) {
                        break;
                    }
                    --ret.events.skip;
                    syncEvent(event, caldav, ret.events.msg) ? ++ret.events.success : ++ret.events.fail;
                    emit syncProgress(++current, count);
                }
            } else {
                current += objects.events.count();
                emit syncProgress(current, count);
            }
        }
        if (! cancelRequested) {
            if (syncSelections.isSync(syncSelections.plannedActivities)) {
                emit syncObjectType(CalDAV::EntryType::PlannedActivity);
                for (RideItem *rideItem : objects.plannedActivities) {
                    if (cancelRequested) {
                        break;
                    }
                    --ret.plannedActivities.skip;
                    syncActivity(rideItem, caldav, ret.plannedActivities.msg) ? ++ret.plannedActivities.success : ++ret.plannedActivities.fail;
                    emit syncProgress(++current, count);
                }
            } else {
                current += objects.plannedActivities.count();
                emit syncProgress(current, count);
            }
        }
        if (! cancelRequested) {
            if (syncSelections.isSync(syncSelections.actualActivities)) {
                emit syncObjectType(CalDAV::EntryType::ActualActivity);
                for (RideItem *rideItem : objects.actualActivities) {
                    if (cancelRequested) {
                        break;
                    }
                    --ret.actualActivities.skip;
                    syncActivity(rideItem, caldav, ret.actualActivities.msg) ? ++ret.actualActivities.success : ++ret.actualActivities.fail;
                    emit syncProgress(++current, count);
                }
            } else {
                current += objects.actualActivities.count();
                emit syncProgress(current, count);
            }
        }
    }
    delete caldav;
    emit syncFinished(ret, cancelRequested && current != count);

    return ret;
}


bool
CalendarSync::cleanup
(const SyncObjects &objects, const SyncSelections &syncSelections, QString cloudServiceName)
{
    CloudService *service = CloudServiceFactory::instance().newService(cloudServiceName, context);
    bool ret = cleanup(objects, syncSelections, service);
    if (service != nullptr) {
        delete service;
    }
    return ret;
}


bool
CalendarSync::cleanup
(const SyncObjects &objects, const SyncSelections &syncSelections, CloudService *cloudService)
{
    bool ret = true;
    CleanupResults results;
    CalDAV *caldav = new CalDAV(context, cloudService);
    if (! caldav->isConfigured()) {
        ++results.tech.fail;
        results.tech.msg << tr("CloudService for CalDAV is not configured");
        ret = false;
    } else {
        QStringList remoteEntries;
        QString error;
        emit cleanupState(true);
        if ((ret = caldav->listBlocking(&remoteEntries, &error))) {
            QMap<CalDAV::EntryType, QSet<QString>> localIds;
            if (objects.isRelevantForType(CalDAV::EntryType::Season)) {
                for (const Season * const &season : objects.seasons) {
                    localIds[CalDAV::EntryType::Season].insert(season->id().toString());
                }
            }
            if (objects.isRelevantForType(CalDAV::EntryType::Phase)) {
                for (const Phase * const &phase : objects.phases) {
                    localIds[CalDAV::EntryType::Phase].insert(phase->id().toString());
                }
            }
            if (objects.isRelevantForType(CalDAV::EntryType::Event)) {
                for (const SeasonEvent * const &event : objects.events) {
                    localIds[CalDAV::EntryType::Event].insert(event->id);
                }
            }
            if (objects.isRelevantForType(CalDAV::EntryType::PlannedActivity)) {
                for (RideItem * const rideItem : objects.plannedActivities) {
                    localIds[CalDAV::EntryType::PlannedActivity].insert(rideItem->ride()->id());
                }
            }
            if (objects.isRelevantForType(CalDAV::EntryType::ActualActivity)) {
                for (RideItem * const rideItem : objects.actualActivities) {
                    localIds[CalDAV::EntryType::ActualActivity].insert(rideItem->ride()->id());
                }
            }
            QList<QString> deleteList;
            for (const QString &calId : remoteEntries) {
                CalDAV::EntryType entryType;
                QString idPart;
                QString originalId;
                if (! CalDAV::CalEntry::getIdParts(calId, &entryType, &idPart, &originalId)) {
                    // Ignore non-GoldenCheetah scoped entries
                    continue;
                }
                CalendarSync::SyncSelections::SyncMode syncMode;
                if (entryType == CalDAV::EntryType::Season && objects.isRelevantForType(CalDAV::EntryType::Season)) {
                    syncMode = syncSelections.seasons;
                } else if (entryType == CalDAV::EntryType::Phase && objects.isRelevantForType(CalDAV::EntryType::Phase)) {
                    syncMode = syncSelections.phases;
                } else if (entryType == CalDAV::EntryType::Event && objects.isRelevantForType(CalDAV::EntryType::Event)) {
                    syncMode = syncSelections.events;
                } else if (entryType == CalDAV::EntryType::PlannedActivity && objects.isRelevantForType(CalDAV::EntryType::PlannedActivity)) {
                    syncMode = syncSelections.plannedActivities;
                } else if (entryType == CalDAV::EntryType::ActualActivity && objects.isRelevantForType(CalDAV::EntryType::ActualActivity)) {
                    syncMode = syncSelections.actualActivities;
                } else {
                    continue;
                }
                if (syncSelections.isRemove(syncMode, localIds[entryType], idPart)) {
                    deleteList << originalId;
                }
            }
            emit cleanupState(false);
            int current = 0;
            results.cleanup.skip = deleteList.count();
            if (! deleteList.isEmpty()) {
                for (const QString &deleteId : deleteList) {
                    emit cleanupProgress(++current, deleteList.count());
                    if (caldav->removeBlocking(deleteId, &error)) {
                        ++results.cleanup.success;
                    } else {
                        ret = false;
                        ++results.cleanup.fail;
                        results.cleanup.msg << error;
                    }
                    --results.cleanup.skip;
                    if (cancelRequested) {
                        break;
                    }
                }
            } else {
                emit cleanupProgress(0, 0);
            }
        } else {
            ++results.tech.fail;
            results.tech.msg << error;
        }
    }
    delete caldav;

    emit cleanupFinished(results, cancelRequested);
    return ret;
}


bool
CalendarSync::isConfigured
(CloudService *cloudService) const
{
    CalDAV *caldav = new CalDAV(context, cloudService);
    bool ret = caldav->isConfigured();
    delete caldav;
    return ret;
}


void
CalendarSync::resetCancel
()
{
    cancelRequested = false;
}


void
CalendarSync::cancel
()
{
    cancelRequested = true;
}


bool
CalendarSync::syncSeason
(Season const *season, CalDAV *caldav, QStringList &errors) const
{
    bool ret = true;
    QString error;
    CalDAV::CalEntry calEntry;
    calEntry.title = tr("Season %1").arg(season->getName());
    calEntry.description = "";
    calEntry.start = QDateTime(season->getStart(), QTime());
    calEntry.end = QDateTime(season->getEnd(), QTime());
    calEntry.allDay = true;
    calEntry.id = season->id().toString();
    calEntry.type = CalDAV::EntryType::Season;

    if (! (ret = caldav->uploadBlocking(calEntry, &error))) {
        errors << QString("%1: %2").arg(calEntry.title, error);
    }
    return ret;
}


bool
CalendarSync::syncPhase
(Phase const *phase, CalDAV *caldav, QStringList &errors) const
{
    bool ret = true;
    QString error;
    CalDAV::CalEntry calEntry;
    calEntry.title = tr("Phase %1").arg(phase->getName());
    if (phase->getType() == Phase::prep) {
        calEntry.title = QString("%1 (%2)").arg(calEntry.title).arg(tr("Prep"));
    } else if (phase->getType() == Phase::base) {
        calEntry.title = QString("%1 (%2)").arg(calEntry.title).arg(tr("Base"));
    } else if (phase->getType() == Phase::build) {
        calEntry.title = QString("%1 (%2)").arg(calEntry.title).arg(tr("Build"));
    } else if (phase->getType() == Phase::camp) {
        calEntry.title = QString("%1 (%2)").arg(calEntry.title).arg(tr("Camp"));
    }
    calEntry.description = "";
    calEntry.start = QDateTime(phase->getStart(), QTime());
    calEntry.end = QDateTime(phase->getEnd(), QTime());
    calEntry.allDay = true;
    calEntry.id = phase->id().toString();
    calEntry.type = CalDAV::EntryType::Phase;

    if (! (ret = caldav->uploadBlocking(calEntry, &error))) {
        errors << QString("%1: %2").arg(calEntry.title, error);
    }
    return ret;
}


bool
CalendarSync::syncActivity
(RideItem *rideItem, CalDAV *caldav, QStringList &errors) const
{
    bool ret = false;
    if (rideItem && rideItem->ride()) {
        QString titleField = appsettings->cvalue(context->athlete->cyclist, GC_CALDAV_TITLE, "Workout Code").toString();
        QString descriptionField = appsettings->cvalue(context->athlete->cyclist, GC_CALDAV_DESCRIPTION, "Calendar Text").toString();
        CalDAV::CalEntry calEntry;
        calEntry.title = rideItem->ride()->getTag(titleField, "");
        if (calEntry.title.isEmpty()) {
            if (! rideItem->sport.isEmpty()) {
                calEntry.title = tr("%1 Activity").arg(rideItem->sport);
            } else {
                calEntry.title = tr("Unknown Activity");
            }
        }
        if (rideItem->ride()->id().isEmpty()) {
            if (! rideItem->isDirty()) {
                rideItem->ride()->setId(QUuid::createUuid().toString());
                context->mainWindow->saveSilent(context, rideItem);
            } else {
                errors << tr("%1: Can't add id to activity with unsaved changes. Save and retry").arg(calEntry.title);
                return false;
            }
        }
        QString error;
        if (rideItem->planned) {
            if (rideItem->hasLinkedActivity()) {
                calEntry.title = tr("Completed: %1").arg(calEntry.title);
            } else {
                calEntry.title = tr("Planned: %1").arg(calEntry.title);
            }
        } else {
            calEntry.title = tr("Actual: %1").arg(calEntry.title);
        }
        calEntry.description = rideItem->getText(descriptionField, "");
        calEntry.start = rideItem->dateTime;
        calEntry.end = calEntry.start.addSecs(rideItem->getForSymbol("workout_time", GlobalContext::context()->useMetricUnits));
        calEntry.allDay = false;
        calEntry.id = rideItem->ride()->id();
        calEntry.type = rideItem->planned ? CalDAV::EntryType::PlannedActivity : CalDAV::EntryType::ActualActivity;

        if (! (ret = caldav->uploadBlocking(calEntry, &error))) {
            errors << QString("%1: %2").arg(calEntry.title, error);
        }
    } else {
        errors << tr("invalid ride");
    }
    return ret;
}


bool
CalendarSync::syncEvent
(SeasonEvent const *event, CalDAV *caldav, QStringList &errors) const
{
    if (event == nullptr) {
        errors << tr("invalid event");
        return false;
    }
    bool ret = true;
    QString error;
    CalDAV::CalEntry calEntry;
    calEntry.title = ((! event->name.isEmpty()) ? event->name : tr("Unnamed Event")) + " ";
    if (event->priority == 0) {
        calEntry.title += tr("(Uncategorized)");
    } else if (event->priority == 1) {
        calEntry.title += tr("(Category A)");
    } else if (event->priority == 2) {
        calEntry.title += tr("(Category B)");
    } else if (event->priority == 3) {
        calEntry.title += tr("(Category C)");
    } else if (event->priority == 4) {
        calEntry.title += tr("(Category D)");
    } else {
        calEntry.title += tr("(Category E)");
    }
    calEntry.description = event->description;
    calEntry.start = QDateTime(event->date, QTime());
    calEntry.end = calEntry.start;
    calEntry.allDay = true;
    calEntry.id = event->id;
    calEntry.type = CalDAV::EntryType::Event;

    if (! (ret = caldav->uploadBlocking(calEntry, &error))) {
        errors << QString("%1: %2").arg(calEntry.title, error);
    }
    return ret;
}
