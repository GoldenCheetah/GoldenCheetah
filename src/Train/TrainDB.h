/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_TrainDB_h
#define _GC_TrainDB_h 1

#include <QDir>
#include <QtSql>
#include <QAbstractTableModel>

#include <TagStore.h>

#include "VideoSyncFileBase.h"
#include "ErgFileBase.h"


namespace TdbWorkoutModelIdx {
    enum WorkoutModelIdx {
        displayname = 0,
        sortdummy,
        fulltext,
        description,
        category,
        subcategory,
        categoryIndex,
        filepath,
        type,
        created,
        duration,
        bikestress,
        intensity,
        isoPower,
        vi,
        xp,
        ri,
        bs,
        svi,
        minPower,
        maxPower,
        avgPower,
        dominantZone,
        numZones,
        z1Percent,
        z1Seconds,
        z2Percent,
        z2Seconds,
        z3Percent,
        z3Seconds,
        z4Percent,
        z4Seconds,
        z5Percent,
        z5Seconds,
        z6Percent,
        z6Seconds,
        z7Percent,
        z7Seconds,
        z8Percent,
        z8Seconds,
        z9Percent,
        z9Seconds,
        z10Percent,
        z10Seconds,
        distance,
        elevation,
        avgGrade,
        rating,
        lastRun
    };
}

namespace TdbVideoModelIdx {
    enum VideoModelIdx {
        filepath = 0,
        created,
        displayname
    };
}

namespace TdbVideosyncModelIdx {
    enum VideosyncModelIdx {
        filepath = 0,
        created,
        source,
        displayname
    };
}


enum class ImportMode {
    replace,    // remove old entry (if exists) then insert; connected user-info is lost
    update,     // update existing entry; keeps all connected user-info; fails if no old entry is available
    insertOrUpdate,     // update existing entry if exists, insert otherwise; keeps all connected user-info
    insert      // insert new entry; fails if item is already available
};

struct WorkoutUserInfo {
    int rating = 0;
    qlonglong lastRun = 0;
};

enum class ZoneContentType {
    percent,
    seconds
};


extern int workoutModelZoneIndex(int zone, ZoneContentType zt);


class TrainDB : public QObject, public TagStore
{
    Q_OBJECT

    public:
        TrainDB(QDir home);
        ~TrainDB();

        // check the db structure is up to date
        void checkDBVersion() const;
        std::pair<bool, int> needsUpgrade() const;

        // get schema version
        int getDBVersion() const;
        int getCount() const;

        void startLUW();
        void endLUW();

        QAbstractTableModel *getWorkoutModel() const;
        QAbstractTableModel *getVideoModel() const;
        QAbstractTableModel *getVideoSyncModel() const;


        bool importWorkout(QString filepath, const ErgFileBase &ergFileBase, ImportMode importMode = ImportMode::insert);
        bool hasWorkout(QString filepath) const;
        bool deleteWorkout(QString filepath) const;
        bool rateWorkout(QString filepath, int rating);
        bool lastWorkout(QString filepath);
        WorkoutUserInfo getWorkoutUserInfo(QString filepath) const;

        bool importVideo(QString filepath, ImportMode importMode = ImportMode::insert) const;
        bool hasVideo(QString filepath) const;
        bool deleteVideo(QString filepath) const;

        bool importVideoSync(QString filepath, const VideoSyncFileBase &videoSyncFileBase, ImportMode importMode = ImportMode::insert) const;
        bool hasVideoSync(QString filepath) const;
        bool deleteVideoSync(QString filepath) const;

        bool hasItem(QString filepath, QString table) const;

        virtual QStringList getWorkouts() const;

        // Implementation of TagStore
        virtual void deferTagSignals(bool deferred) override;
        virtual bool isDeferredTagSignals() const override;
        virtual void catchupTagSignals() override;
        virtual int addTag(const QString &label) override;
        virtual bool updateTag(int id, const QString &label) override;
        virtual bool deleteTag(int id) override;
        virtual bool deleteTag(const QString &label) override;
        virtual int getTagId(const QString &label) const override;
        virtual QString getTagLabel(int id) const override;
        virtual bool hasTag(int id) const override;
        virtual bool hasTag(const QString &label) const override;

        virtual QList<Tag> getTags() const override;
        virtual QList<int> getTagIds() const override;
        virtual QStringList getTagLabels() const override;
        virtual QStringList getTagLabels(const QList<int> ids) const override;

        virtual int countTagUsage(int id) const override;

        // Helpers for Taggable
        bool workoutHasTag(const QString &filepath, int id) const;
        void workoutAddTag(const QString &filepath, int id);
        void workoutRemoveTag(const QString &filepath, int id);
        void workoutClearTags(const QString &filepath);
        QList<int> workoutGetTagIds(const QString &filepath) const;

        // for 3.3
        // TODO Upgrade-Functions come last - after everything else is running
        bool upgradeDefaultEntriesWorkout();

        bool upgradeSchemaTo3() const;

        // drop and recreate tables
        void rebuildDB() const;
        // As rebuildDB() but keeps tables that hold user data: ratings, tags, etc
        void rebuildDBButUserDataTables() const;

        // Helpers for DB-upgrade
        QStringList getMigrateableWorkoutPaths() const;
        QStringList getMigrateableVideoPaths() const;
        QStringList getMigrateableVideoSyncPaths() const;
        void dropLegacyTables() const;

    signals:
        void dataChanged();
        void tagsChanged(int idAdded, int idDeleted, int idUpdated);
        void deferredTagsChanged(QList<int> idsAdded, QList<int> idsDeleted, QList<int> idsUpdated);

    private:
        QDir home;
        QSqlDatabase *db;
        const QString sessionid;
        bool tagSignalsDeferred = false;
        QList<int> deferredTagsAdded;
        QList<int> deferredTagsDeleted;
        QList<int> deferredTagsUpdated;
        int startupVersion = -1;

        // get connection name
        QSqlDatabase connection() const;

        void initDatabase();

        bool importWorkoutNoTags(QString filepath, const ErgFileBase &ergFileBase, ImportMode importMode = ImportMode::insert) const;

        /**
         * create database - does nothing if its already there
         */
        bool createDatabase() const;
        void closeConnection() const;

        bool createDefaultEntriesWorkout() const;
        bool createDefaultEntriesVideo() const;
        bool createDefaultEntriesVideoSync() const;
        bool createDefaultEntriesTagStore() const;
        bool createDefaultEntriesWorkoutTags() const;

        bool createAllDataTables() const;
        bool dropAllDataTables() const;
        bool dropTablesButUserDataTables() const;

        bool dropTable(QString tableName, bool hasVersionEntry = true) const;

        /**
         * @return: -1: Failed to create the table; 0: Table already present and unchanged; 1: Table created
         */
        int createTable(QString tableName, QString createBody, bool hasVersionEntry = true, bool force = false) const;

        bool updateTableVersion(QString tableName) const;

        /**
         * Check if a table exists and has an entry in the table version.
         *
         * @return -2: query error; -1: table does not exist; 0: table exists but has no entry in table version; > 0: table is available and has this version
         */
        int getTableVersion(QString tableName) const;

        bool bindIfValue(QSqlQuery &query, const QString &placeholder, const QString &value) const;
        bool bindIfValue(QSqlQuery &query, const QString &placeholder, double value) const;
        bool bindIfValue(QSqlQuery &query, const QString &placeholder, qlonglong value, qlonglong nullValue = 0) const;
        bool bindIfValue(QSqlQuery &query, const QString &placeholder, int value, int nullValue = 0) const;
        bool bindIfPredecessor(QSqlQuery &query, const QString &placeholder, double value, bool predecessor) const;

        void bindWorkout(QSqlQuery &query, const QString &filepath, const ErgFileBase &ergFileBase) const;

        // Helpers for DB-upgrade
        bool renameTable(const QString &from, const QString &to) const;
        bool copyData(const QString &from, const QString &to, const QString &fields) const;
};

extern TrainDB *trainDB;
#endif
