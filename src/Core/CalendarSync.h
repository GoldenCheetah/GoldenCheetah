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

#ifndef _Gc_CalendarSync_h
#define _Gc_CalendarSync_h

#include <QObject>
#include <QList>
#include <QStringList>

#include "Context.h"
#include "CloudService.h"
#include "CalDAV.h"

class CalDAV;
class RideItem;
class Season;
class SeasonEvent;


class CloudCalendarLister {
public:
    struct CloudCalendarStatus {
        QString name;
        bool serviceActive;
        bool serviceConfigured;
    };

    virtual QList<CloudCalendarStatus> getCloudCalendarStatus() const = 0;
};


class CalendarSync : public QObject
{
    Q_OBJECT

public:
    struct SyncObjects {
        int count() const;
        bool isRelevantForType(CalDAV::EntryType type) const;

        CalDAV::EntryType root = CalDAV::EntryType::Season;
        QList<Season const *> seasons;
        QList<Phase const *> phases;
        QList<SeasonEvent const *> events;
        QList<RideItem*> plannedActivities;
        QList<RideItem*> actualActivities;
    };

    struct SyncSelections {
        enum class SyncMode : int {
            Sync = 0,
            SyncAndCleanup = 1,
            Skip = 2,
            Remove = 3
        };

        constexpr bool isSync(SyncMode syncMode) const;
        bool isRemove(SyncMode syncMode, QSet<QString> localIds, QString remoteId) const;

        SyncMode seasons = SyncMode::SyncAndCleanup;
        SyncMode phases = SyncMode::SyncAndCleanup;
        SyncMode events = SyncMode::SyncAndCleanup;
        SyncMode plannedActivities = SyncMode::SyncAndCleanup;
        SyncMode actualActivities = SyncMode::SyncAndCleanup;
    };

    struct SyncResult {
        int fail = 0;
        int success = 0;
        int skip = 0;
        QStringList msg;
    };

    struct SyncResults {
        bool ok() const;
        SyncResult overall() const;

        SyncResult tech;
        SyncResult seasons;
        SyncResult phases;
        SyncResult events;
        SyncResult plannedActivities;
        SyncResult actualActivities;
    };

    struct CleanupResults {
        bool ok() const;

        SyncResult tech;
        SyncResult cleanup;
    };

    explicit CalendarSync(Context *context);

    SyncObjects buildObjects(Season const *season) const;
    SyncObjects buildObjects(Phase const *phase) const;
    SyncObjects buildObjects(SeasonEvent const *event) const;
    SyncObjects buildObjects(RideItem *rideItem) const;

    SyncResults syncObjects(const SyncObjects &syncObjects, const SyncSelections &syncSelections, QString cloudServiceName);
    SyncResults syncObjects(const SyncObjects &syncObjects, const SyncSelections &syncSelections, CloudService *cloudService);

    bool cleanup(const SyncObjects &syncObjects, const SyncSelections &syncSelections, QString cloudServiceName);
    bool cleanup(const SyncObjects &syncObjects, const SyncSelections &syncSelections, CloudService *cloudService);

    void resetCancel();

    bool isConfigured(CloudService *cloudService) const;

public slots:
    void cancel();

signals:
    void syncObjectType(CalDAV::EntryType type);
    void syncProgress(int current, int count);
    void syncFinished(const SyncResults &results, bool canceled);
    void cleanupState(bool prepare);
    void cleanupProgress(int current, int count);
    void cleanupFinished(const CleanupResults &results, bool canceled);

private:
    bool syncSeason(Season const *season, CalDAV *caldav, QStringList &errors) const;
    bool syncPhase(Phase const *phase, CalDAV *caldav, QStringList &errors) const;
    bool syncEvent(SeasonEvent const *event, CalDAV *caldav, QStringList &errors) const;
    bool syncActivity(RideItem *rideItem, CalDAV *caldav, QStringList &errors) const;

    Context *context;
    bool cancelRequested = false;
};

#endif
