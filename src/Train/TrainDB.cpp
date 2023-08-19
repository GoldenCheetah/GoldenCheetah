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

#include "TrainDB.h"

#include <QMessageBox>
#include <QSet>
#include <QSqlQueryModel>


// DB Schema Version - YOU MUST UPDATE THIS IF THE TRAIN DB SCHEMA CHANGES

// Revision History
// Rev Date         Who                What Changed
// 01  21 Dec 2012  Mark Liversedge    Initial Build
// 02  21 Feb 2023  Joachim Kohlhammer Schema-Update

#define TABLE_VERSION "version"
#define FIELDS_VERSION \
    "table_name TEXT NOT NULL UNIQUE," \
    "schema_version INTEGER NOT NULL," \
    "creation_date INTEGER NOT NULL," \
    "PRIMARY KEY(table_name)"
#define TABLE_WORKOUT "workout"
// if: Intensity Factor
// vi: Variability Index
// xp: XPower
// ri: RelativeIntensity
// bs: BikeScore
// svi: SkibaVariabilityIndex
#define FIELDS_WORKOUT \
    "filepath TEXT NOT NULL UNIQUE," \
    "type TEXT NOT NULL," \
    "creation_date INTEGER NOT NULL DEFAULT (STRFTIME('%s', 'now'))," \
    "source TEXT," \
    "source_id TEXT," \
    "displayname TEXT NOT NULL," \
    "description TEXT," \
    "erg_subtype TEXT," \
    "erg_duration INTEGER," \
    "erg_bikestress REAL," \
    "erg_if REAL," \
    "erg_iso_power REAL," \
    "erg_vi REAL," \
    "erg_xp REAL," \
    "erg_ri REAL," \
    "erg_bs REAL," \
    "erg_svi REAL," \
    "erg_min_power INTEGER," \
    "erg_max_power INTEGER," \
    "erg_avg_power INTEGER," \
    "erg_dominant_zone INTEGER," \
    "erg_num_zones INTEGER," \
    "erg_duration_z1 REAL," \
    "erg_duration_z2 REAL," \
    "erg_duration_z3 REAL," \
    "erg_duration_z4 REAL," \
    "erg_duration_z5 REAL," \
    "erg_duration_z6 REAL," \
    "erg_duration_z7 REAL," \
    "erg_duration_z8 REAL," \
    "erg_duration_z9 REAL," \
    "erg_duration_z10 REAL," \
    "slp_distance REAL," \
    "slp_elevation INTEGER," \
    "slp_avg_grade REAL," \
    "rating INTEGER," \
    "last_run INTEGER," \
    "PRIMARY KEY(filepath)"
#define TABLE_VIDEO "video"
#define FIELDS_VIDEO \
    "filepath TEXT NOT NULL UNIQUE," \
    "creation_date INTEGER NOT NULL DEFAULT (STRFTIME('%s', 'now'))," \
    "displayname TEXT NOT NULL," \
    "PRIMARY KEY(filepath)"
#define TABLE_VIDEOSYNC "videosync"
#define FIELDS_VIDEOSYNC \
    "filepath TEXT NOT NULL UNIQUE," \
    "creation_date INTEGER NOT NULL DEFAULT (STRFTIME('%s', 'now'))," \
    "source TEXT," \
    "displayname TEXT NOT NULL," \
    "PRIMARY KEY(filepath)"

static int TrainDBSchemaVersion = 2;
TrainDB *trainDB;


extern int
workoutModelZoneIndex
(int zone, ZoneContentType zt)
{
    if (zt == ZoneContentType::percent) {
        return TdbWorkoutModelIdx::z1Percent + (zone - 1) * 2;
    } else {
        return TdbWorkoutModelIdx::z1Seconds + (zone - 1) * 2;
    }
}


TrainDB::TrainDB
(QDir home): home(home), sessionid("train")
{
    initDatabase();
}


TrainDB::~TrainDB
()
{
    if (db) {
        db->close();
        delete db;
        QSqlDatabase::removeDatabase(sessionid);
    }
}


QSqlDatabase
TrainDB::connection
() const
{
    return db->database(sessionid);
}


void
TrainDB::closeConnection
() const
{
    connection().close();
}


void
TrainDB::initDatabase
()
{
    if (connection().isOpen()) return;

    // get a connection
    db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", sessionid));
    db->setDatabaseName(home.canonicalPath() + "/trainDB");

    if (connection().isOpen()) {
        createDatabase();
    } else {
        QMessageBox::critical(0,
                              qApp->translate("TrainDB","Cannot open database"),
                              qApp->translate("TrainDB","Unable to establish a database connection.\n"
                                              "This feature requires SQLite support. Please read "
                                              "the Qt SQL driver documentation for information how "
                                              "to build it.\n\n"
                                              "Click Cancel to exit."),
                              QMessageBox::Cancel);
    }
}


// rebuild effectively drops and recreates all tables
// but not the version table, since its about deleting
// user data (e.g. when rescanning their hard disk)
void
TrainDB::rebuildDB
() const
{
    dropAllDataTables();
    createAllDataTables();
}


///////////////////////// Helpers for DB-upgrade

QStringList
TrainDB::getMigrateableWorkoutPaths
() const
{
    int dbver = getDBVersion();
    int oldver = dbver - 1;

    QSqlQuery query(connection());
    QStringList ret;

    switch (oldver) {
    case 1:
        query.prepare("SELECT filepath FROM workouts WHERE filepath NOT LIKE '//%'");
        break;
    case 2:
        query.prepare("SELECT filepath FROM workout WHERE source != 'gcdefault' AND type != 'code'");
        break;
    default:
        qInfo() << "TrainDB::getMigrateableWorkoutPaths: Nothing to migrate from DB Version" << dbver;
        return ret;
    }
    if (query.exec()) {
        while (query.next()) {
            ret << query.value(0).toString();
        }
    }
    return ret;
}


QStringList
TrainDB::getMigrateableVideoPaths
() const
{
    int dbver = getDBVersion();
    int oldver = dbver - 1;

    QSqlQuery query(connection());
    QStringList ret;

    switch (oldver) {
    case 1:
        query.prepare("SELECT filepath FROM videos");
        break;
    case 2:
        query.prepare("SELECT filepath FROM video");
        break;
    default:
        qInfo() << "TrainDB::getMigrateableVideoPaths: Nothing to migrate from DB Version" << dbver;
        return ret;
    }
    if (query.exec()) {
        while (query.next()) {
            ret << query.value(0).toString();
        }
    }
    return ret;
}


QStringList
TrainDB::getMigrateableVideoSyncPaths
() const
{
    int dbver = getDBVersion();
    int oldver = dbver - 1;

    QSqlQuery query(connection());
    QStringList ret;

    switch (oldver) {
    case 1:
        query.prepare("SELECT filepath FROM videosyncs WHERE filepath NOT LIKE '//%'");
        break;
    case 2:
        query.prepare("SELECT filepath FROM videosync WHERE source != 'gcdefault'");
        break;
    default:
        qInfo() << "TrainDB::getMigrateableVideoSyncPaths: Nothing to migrate from DB Version" << dbver;
        return ret;
    }
    if (query.exec()) {
        while (query.next()) {
            ret << query.value(0).toString();
        }
    }
    return ret;
}


void
TrainDB::dropLegacyTables
() const
{
    dropTable("workouts", true);
    dropTable("videos", true);
    dropTable("videosyncs", true);
}


/////////////////////////


bool
TrainDB::createDatabase
() const
{
    // check schema version and drop out-of-date tables
    checkDBVersion();
    createAllDataTables();

    return true;
}


void
TrainDB::checkDBVersion
() const
{
    QSet<QString> dataTables;
    dataTables << TABLE_WORKOUT
               << TABLE_VIDEO
               << TABLE_VIDEOSYNC;

    // can we get a version number?
    QSqlQuery query("SELECT table_name, schema_version, creation_date FROM version", connection());
    if (! query.exec()) {
        // we couldn't read the version table properly
        // it must be out of date!!
        dropTable(TABLE_VERSION, false);
        createTable(TABLE_VERSION, FIELDS_VERSION, false);

        // wipe away whatever (if anything is there)
        QSetIterator<QString> i(dataTables);
        while (i.hasNext()) {
            dropTable(i.next());
        }
        return;
    }

    // ok we checked out ok, so lets adjust db schema to reflect
    // the current version / crc
    while (query.next()) {
        QString tableName = query.value(0).toString();
        int currentVersion = query.value(1).toInt();

        if (currentVersion == TrainDBSchemaVersion && dataTables.contains(tableName)) {
            dataTables.remove(tableName);
        }
    }
    QSetIterator<QString> i(dataTables);
    while (i.hasNext()) {
        dropTable(i.next());
    }
}


bool
TrainDB::needsUpgrade
() const
{
    QSqlQuery query("SELECT MIN(schema_version) from version", connection());
    if (query.exec() && query.next()) {
        return query.value(0).toInt() != 2;
    }
    return false;
}


int
TrainDB::getCount
() const
{
    // how many workouts are there?
    QSqlQuery query("SELECT count(*) FROM workout", connection());

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}


void
TrainDB::startLUW
()
{
    connection().transaction();
}


void
TrainDB::endLUW
()
{
    connection().commit();
    emit dataChanged();
}


QAbstractTableModel*
TrainDB::getWorkoutModel
() const
{
    QSqlQuery query(connection());

    query.prepare("SELECT w.displayname, w.type || '_' || upper(w.displayname) as _sortdummy, "
                  "       w.displayname || ' ' || IFNULL(w.description, '') || ' ' || w.type || ' ' || IFNULL(w.erg_subtype, '') || ' ' || IFNULL(w.source, '') as _fulltext, "
                  "       w.description, w.filepath, w.type, w.creation_date, "
                  "       w.erg_duration, "
                  "       w.erg_bikestress, w.erg_if, w.erg_iso_power, w.erg_vi, w.erg_xp, w.erg_ri, w.erg_bs, w.erg_svi, "
                  "       w.erg_min_power, w.erg_max_power, w.erg_avg_power, "
                  "       w.erg_dominant_zone, w.erg_num_zones, "
                  "       w.erg_duration_z1, w.erg_duration_z1 * w.erg_duration / 100000 as _erg_duration_z1_secs, "
                  "       w.erg_duration_z2, w.erg_duration_z2 * w.erg_duration / 100000 as _erg_duration_z2_secs, "
                  "       w.erg_duration_z3, w.erg_duration_z3 * w.erg_duration / 100000 as _erg_duration_z3_secs, "
                  "       w.erg_duration_z4, w.erg_duration_z4 * w.erg_duration / 100000 as _erg_duration_z4_secs, "
                  "       w.erg_duration_z5, w.erg_duration_z5 * w.erg_duration / 100000 as _erg_duration_z5_secs, "
                  "       w.erg_duration_z6, w.erg_duration_z6 * w.erg_duration / 100000 as _erg_duration_z6_secs, "
                  "       w.erg_duration_z7, w.erg_duration_z7 * w.erg_duration / 100000 as _erg_duration_z7_secs, "
                  "       w.erg_duration_z8, w.erg_duration_z8 * w.erg_duration / 100000 as _erg_duration_z8_secs, "
                  "       w.erg_duration_z9, w.erg_duration_z9 * w.erg_duration / 100000 as _erg_duration_z9_secs, "
                  "       w.erg_duration_z10, w.erg_duration_z10 * w.erg_duration / 100000 as _erg_duration_z10_secs, "
                  "       w.slp_distance, w.slp_elevation, w.slp_avg_grade, "
                  "       IFNULL(w.rating, 0) as _rating, w.last_run "
                  "  FROM workout w ");

    query.exec();
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(query);
    while (model->canFetchMore(QModelIndex())) {
        model->fetchMore(QModelIndex());
    }
    qDebug() << "TrainDB::getWorkoutModel: " << model->rowCount() << "x" << model->columnCount() << "/" << query.lastError();
    return model;
}


QAbstractTableModel*
TrainDB::getVideoModel
() const
{
    QSqlTableModel *model = new QSqlTableModel(nullptr, connection());
    model->setTable(TABLE_VIDEO);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();
    while (model->canFetchMore(QModelIndex())) {
        model->fetchMore(QModelIndex());
    }
    qDebug() << "TrainDB::getVideoModel:" << model->rowCount() << "x" << model->columnCount();
    return model;
}


QAbstractTableModel*
TrainDB::getVideoSyncModel
() const
{
    QSqlTableModel *model = new QSqlTableModel(nullptr, connection());
    model->setTable(TABLE_VIDEOSYNC);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();
    while (model->canFetchMore(QModelIndex())) {
        model->fetchMore(QModelIndex());
    }
    qDebug() << "TrainDB::getVideoSyncModel:" << model->rowCount() << "x" << model->columnCount();
    return model;
}


int
TrainDB::getDBVersion
() const
{
    int schema_version = -1;

    // can we get a version number?
    QSqlQuery query("SELECT max(schema_version) FROM version", connection());
    if (query.exec() && query.first()) {
        schema_version = query.value(0).toInt();
    }
    return schema_version;
}


/*----------------------------------------------------------------------
 * CRUD routines
 *----------------------------------------------------------------------*/


bool
TrainDB::hasWorkout
(QString filepath) const
{
    return hasItem(filepath, "workout");
}


bool
TrainDB::deleteWorkout
(QString filepath) const
{
    QSqlQuery query(connection());

    query.prepare("DELETE FROM workout WHERE filepath = :filepath");
    query.bindValue(":filepath", filepath);
    return query.exec();
}


bool
TrainDB::rateWorkout
(QString filepath, int rating)
{
    QSqlQuery query(connection());

    query.prepare("UPDATE workout "
                  "   SET rating = :rating "
                  " WHERE filepath = :filepath");
    query.bindValue(":filepath", filepath);
    query.bindValue(":rating", rating);
    bool ret = query.exec();
    if (ret) {
        emit dataChanged();
    }
    return ret;
}


bool
TrainDB::lastWorkout
(QString filepath)
{
    QSqlQuery query(connection());

    query.prepare("UPDATE workout "
                  "   SET last_run = :last_run "
                  " WHERE filepath = :filepath");
    query.bindValue(":filepath", filepath);
    query.bindValue(":last_run", QDateTime::currentDateTime().toSecsSinceEpoch());
    bool ret = query.exec();
    if (ret) {
        emit dataChanged();
    }
    return ret;
}


WorkoutUserInfo
TrainDB::getWorkoutUserInfo
(QString filepath) const
{
    WorkoutUserInfo ret;
    QSqlQuery query(connection());
    query.prepare("SELECT rating, last_run "
                  "  FROM workout "
                  " WHERE filepath = :filepath");
    query.bindValue(":filepath", filepath);
    if (query.exec() && query.next()) {
        ret.rating = query.value(0).toInt();
        ret.lastRun = query.value(1).toLongLong();
    }
    return ret;
}


bool
TrainDB::importWorkout
(QString filepath, const ErgFileBase &ergFileBase, ImportMode importMode) const
{
    if (importMode == ImportMode::replace) {
        deleteWorkout(filepath);
    }

    QSqlQuery query(connection());

    bool ok = false;
    if (   importMode == ImportMode::replace
        || importMode == ImportMode::insert
        || importMode == ImportMode::insertOrUpdate) {
        query.prepare("INSERT "
                      "  INTO workout "
                      "       (filepath, type, source, source_id, displayname, description, "
                      "        erg_subtype, erg_duration, erg_bikestress, erg_if, "
                      "        erg_iso_power, erg_vi, erg_xp, erg_ri, erg_bs, erg_svi, "
                      "        erg_min_power, erg_max_power, erg_avg_power, "
                      "        erg_dominant_zone, erg_num_zones, "
                      "        erg_duration_z1, erg_duration_z2, erg_duration_z3, erg_duration_z4, erg_duration_z5, "
                      "        erg_duration_z6, erg_duration_z7, erg_duration_z8, erg_duration_z9, erg_duration_z10, "
                      "        slp_distance, slp_elevation, slp_avg_grade) "
                      "VALUES (:filepath, :type, :source, :source_id, :displayname, :description, "
                      "        :erg_subtype, :erg_duration,:erg_bikestress, :erg_if, "
                      "        :erg_iso_power, :erg_vi, :erg_xp, :erg_ri, :erg_bs, :erg_svi, "
                      "        :erg_min_power, :erg_max_power, :erg_avg_power, "
                      "        :erg_dominant_zone, :erg_num_zones, "
                      "        :erg_duration_z1, :erg_duration_z2, :erg_duration_z3, :erg_duration_z4, :erg_duration_z5, "
                      "        :erg_duration_z6, :erg_duration_z7, :erg_duration_z8, :erg_duration_z9, :erg_duration_z10, "
                      "        :slp_distance, :slp_elevation, :slp_avg_grade)");
        bindWorkout(query, filepath, ergFileBase);
        ok = query.exec();
    }
    if (   hasWorkout(filepath)
        && (   (! ok && importMode == ImportMode::insertOrUpdate)
            || importMode == ImportMode::update)) {
        query.prepare("UPDATE workout "
                      "   SET type = :type, source = :source, source_id = :source_id, displayname = :displayname, description = :description, "
                      "       erg_subtype = :erg_subtype, erg_duration = :erg_duration, erg_bikestress = :erg_bikestress, erg_if = :erg_if, "
                      "       erg_iso_power = :erg_iso_power, erg_vi = :erg_vi, erg_xp = :erg_xp, erg_ri = :erg_ri, erg_bs = :erg_bs, erg_svi = :erg_svi, "
                      "       erg_min_power = :erg_min_power, erg_max_power = :erg_max_power, erg_avg_power = erg_avg_power, "
                      "       erg_dominant_zone = :erg_dominant_zone, erg_num_zones = :erg_num_zones, "
                      "       erg_duration_z1 = :erg_duration_z1, erg_duration_z2 = :erg_duration_z2, "
                      "       erg_duration_z3 = :erg_duration_z3, erg_duration_z4 = :erg_duration_z4, "
                      "       erg_duration_z5 = :erg_duration_z5, erg_duration_z6 = :erg_duration_z6, "
                      "       erg_duration_z7 = :erg_duration_z7, erg_duration_z8 = :erg_duration_z8, "
                      "       erg_duration_z9 = :erg_duration_z9, erg_duration_z10 = :erg_duration_z10, "
                      "       slp_distance = :slp_distance, slp_elevation = :slp_elevation, slp_avg_grade = :slp_avg_grade "
                      " WHERE filepath = :filepath");
        bindWorkout(query, filepath, ergFileBase);
        ok = query.exec();
    }

    return ok;
}


bool
TrainDB::hasVideoSync
(QString filepath) const
{
    return hasItem(filepath, "videosync");
}


bool
TrainDB::deleteVideoSync
(QString filepath) const
{
    QSqlQuery query(connection());

    query.prepare("DELETE FROM videosync WHERE filepath = :filepath");
    query.bindValue(":filepath", filepath);
    return query.exec();
}


bool
TrainDB::importVideoSync
(QString filepath, const VideoSyncFileBase &videoSyncFileBase, ImportMode importMode) const
{
    if (importMode == ImportMode::replace) {
        deleteVideo(filepath);
    }

    QSqlQuery query(connection());

    bool ok = false;
    if (   importMode == ImportMode::replace
        || importMode == ImportMode::insert
        || importMode == ImportMode::insertOrUpdate) {
        query.prepare("INSERT "
                      "  INTO videosync "
                      "       (filepath, source, displayname)"
                      "VALUES (:filepath, :source, :displayname)");
        query.bindValue(":filepath", filepath);
        if (! bindIfValue(query, ":displayname", videoSyncFileBase.name())) {
            bindIfValue(query, ":displayname", QFileInfo(filepath).baseName());
        }
        bindIfValue(query, ":source", videoSyncFileBase.source());
        ok = query.exec();
    }
    if (   hasVideo(filepath)
        && (   (! ok && importMode == ImportMode::insertOrUpdate)
            || importMode == ImportMode::update)) {
        query.prepare("UPDATE videosync "
                      "   SET source = :source, displayname = :displayname "
                      " WHERE filepath = :filepath");
        query.bindValue(":filepath", filepath);
        if (! bindIfValue(query, ":displayname", videoSyncFileBase.name())) {
            bindIfValue(query, ":displayname", QFileInfo(filepath).baseName());
        }
        bindIfValue(query, ":source", videoSyncFileBase.source());
        ok = query.exec();
    }

    return ok;
}


bool
TrainDB::hasVideo
(QString filepath) const
{
    return hasItem(filepath, "video");
}


bool
TrainDB::deleteVideo
(QString filepath) const
{
    QSqlQuery query(connection());

    query.prepare("DELETE FROM video WHERE filepath = :filepath");
    query.bindValue(":filepath", filepath);
    return query.exec();
}


bool
TrainDB::importVideo
(QString filepath, ImportMode importMode) const
{
    if (importMode == ImportMode::replace) {
        deleteVideo(filepath);
    }

    QSqlQuery query(connection());

    bool ok = false;
    if (   importMode == ImportMode::replace
        || importMode == ImportMode::insert
        || importMode == ImportMode::insertOrUpdate) {
        query.prepare("INSERT "
                      "  INTO video "
                      "       (filepath, displayname)"
                      "VALUES (:filepath, :displayname)");
        query.bindValue(":filepath", filepath);
        query.bindValue(":displayname", QFileInfo(filepath).baseName());
        ok = query.exec();
    }
    if (   hasVideo(filepath)
        && (   (! ok && importMode == ImportMode::insertOrUpdate)
            || importMode == ImportMode::update)) {
        query.prepare("UPDATE video "
                      "   SET displayname = :displayname "
                      " WHERE filepath = :filepath");
        query.bindValue(":filepath", filepath);
        query.bindValue(":displayname", QFileInfo(filepath).baseName());
        ok = query.exec();
    }

    return ok;
}


bool
TrainDB::createDefaultEntriesWorkout
() const
{
    bool rc = true;

    ErgFileBase efb;
    efb.name(tr("Manual Erg Mode"));
    efb.format(ErgFileFormat::code);
    efb.source("gcdefault");
    rc &= importWorkout("//1", efb);

    efb.name(tr("Manual Slope Mode"));
    efb.format(ErgFileFormat::code);
    efb.source("gcdefault");
    rc &= importWorkout("//2", efb);

    return rc;
}


bool
TrainDB::hasItem
(QString filepath, QString table) const
{
    int hasItem = false;

    QSqlQuery query(connection());
    query.prepare(QString("SELECT count(*) FROM %1 WHERE filepath = :filepath").arg(table));
    query.bindValue(":filepath", filepath);
    if (query.exec() && query.first()) {
        hasItem = query.value(0).toInt() > 0;
    }
    return hasItem;
}


bool
TrainDB::upgradeDefaultEntriesWorkout
()
{
    // set texts starting with " " in upgrade - since due to same translation errors the " " was lost e.g. in German
    QSqlQuery query(connection());

    // adding a space at the front of string to make manual mode always
    // appear first in a sorted list is a bit of a hack, but works ok
    QString manualErg = QString("UPDATE workouts SET filename = \"%1\" WHERE filepath = \"//1\";")
            .arg(" " + tr("Manual Erg Mode")); // keep the SPACE separate so that translation cannot remove it
    bool rc = query.exec(manualErg);

    QString manualCrs = QString("UPDATE workouts SET filename = \"%1\" WHERE filepath = \"//2\";")
            .arg(" " + tr("Manual Slope Mode")); // keep the SPACE separate so that translation cannot remove it
    rc &= query.exec(manualCrs);

    return rc;
}


bool
TrainDB::createDefaultEntriesVideo
() const
{
    // insert the 'DVD' record for playing currently loaded DVD
    // need to resolve DVD playback in v3.1, there is an open feature request for this.
    // rc = query.exec("INSERT INTO videos (filepath, filename) values (\"\", \"DVD\");");
    return true;
}


bool
TrainDB::createDefaultEntriesVideoSync
() const
{
    bool rc = true;

    // adding a space at the front of string to make "None" always
    // appear first in a sorted list is a bit of a hack, but works ok
    VideoSyncFileBase vsfb;
    vsfb.name(" " + tr("None")); // keep the SPACE separate so that translation cannot remove it
    vsfb.source("gcdefault");
    rc &= importVideoSync("//1", vsfb);

    return rc;

}


bool
TrainDB::createAllDataTables
() const
{
    bool ok = true;
    int ret;
    ok &= (ret = createTable(TABLE_WORKOUT, FIELDS_WORKOUT)) != -1;
    if (ret > 0) {
        ok &= createDefaultEntriesWorkout();
    }
    ok &= (ret = createTable(TABLE_VIDEO, FIELDS_VIDEO)) != -1;
    if (ret > 0) {
        ok &= createDefaultEntriesVideo();
    }
    ok &= (ret = createTable(TABLE_VIDEOSYNC, FIELDS_VIDEOSYNC)) != -1;
    if (ret > 0) {
        ok &= createDefaultEntriesVideoSync();
    }
    return ok;
}


bool
TrainDB::dropAllDataTables
() const
{
    bool ok = dropTable(TABLE_WORKOUT);
    ok &= dropTable(TABLE_VIDEO);
    ok &= dropTable(TABLE_VIDEOSYNC);
    return ok;
}


bool
TrainDB::dropTable
(QString tableName, bool hasVersionEntry) const
{
    bool ok = true;
    if (hasVersionEntry) {
        QSqlQuery queryDelete(connection());
        queryDelete.prepare("DELETE FROM version where table_name = :tablename");
        queryDelete.bindValue(":tablename", tableName);
        ok = queryDelete.exec();
    }
    if (ok) {
        QSqlQuery queryDrop(QString("DROP TABLE IF EXISTS %1").arg(tableName), connection());
        ok = queryDrop.exec();
    }
    return ok;
}


// 1. Check if the table exists (query sqlite_master) - if found: No action required: exit
// 2. Create table
// 3. Create default entries
// 4. Update version of table
int
TrainDB::createTable
(QString tableName, QString createBody, bool hasVersionEntry, bool force) const
{
    bool ret = 0;
    bool ok = true;
    int version = 0;

    if (! force && hasVersionEntry) {
        if ((version = getTableVersion(tableName)) == -2) {
            return -1;
        }
    }
    if (force) {
        dropTable(tableName);
    }
    if (force || ! hasVersionEntry || (hasVersionEntry && version < TrainDBSchemaVersion)) {
        QSqlQuery query(connection());
        if (! (ok = query.exec(QString("CREATE TABLE %1 (%2)").arg(tableName, createBody)))) {
            qDebug() << query.lastError() << "/" << query.lastQuery();
        }
        if (ok && hasVersionEntry) {
            ok = updateTableVersion(tableName);
        }
        qDebug() << "TrainDB::createTable:" << tableName << "/ Done: " << ok;
        ret = 1;
    } else {
        qDebug() << "TrainDB::createTable:" << tableName << "/ Nothing to do";
        ret = 0;
    }
    return ret;
}


bool
TrainDB::updateTableVersion
(QString tableName) const
{
    QSqlQuery query(connection());
    query.prepare("INSERT OR REPLACE INTO version (table_name, schema_version, creation_date) values (:tablename, :schemaversion, :creationdate)");
    query.bindValue(":tablename", tableName);
    query.bindValue(":schemaversion", TrainDBSchemaVersion);
    query.bindValue(":creationdate", QDateTime::currentDateTime().toSecsSinceEpoch());
    bool ok = query.exec();
    if (! ok) {
        qDebug() << "bool TrainDB::updateTableVersion(QString tableName) const" << query.lastError() << "/" << query.lastQuery();
    }
    return query.exec();
}


int
TrainDB::getTableVersion
(QString tableName) const
{
    QSqlQuery masterQuery(connection());
    masterQuery.prepare("SELECT name FROM sqlite_master WHERE type='table' and name = :tablename");
    masterQuery.bindValue(":tablename", tableName);
    if (masterQuery.exec()) {
        if (! masterQuery.next()) {
            return -1;
        }
    } else {
        return -2;
    }

    int version = -1;
    QSqlQuery versionQuery(connection());
    versionQuery.prepare("SELECT schema_version FROM version WHERE table_name = :tablename");
    versionQuery.bindValue(":tablename", tableName);
    if (versionQuery.exec()) {
        if (versionQuery.first()) {
            version = versionQuery.value(0).toInt();
        }
    } else {
        version = -2;
    }
    return version;
}


bool
TrainDB::bindIfValue
(QSqlQuery &query, const QString &placeholder, const QString &value) const
{
    if (value.isEmpty()) {
        return false;
    }
    query.bindValue(placeholder, value);
    return true;
}


bool
TrainDB::bindIfValue
(QSqlQuery &query, const QString &placeholder, double value) const
{
    if (qFuzzyIsNull(value)) {
        return false;
    }
    query.bindValue(placeholder, value);
    return true;
}


bool
TrainDB::bindIfValue
(QSqlQuery &query, const QString &placeholder, qlonglong value) const
{
    if (value == 0) {
        return false;
    }
    query.bindValue(placeholder, value);
    return true;
}


bool
TrainDB::bindIfValue
(QSqlQuery &query, const QString &placeholder, int value) const
{
    if (value == 0) {
        return false;
    }
    query.bindValue(placeholder, value);
    return true;
}


bool
TrainDB::bindIfPredecessor
(QSqlQuery &query, const QString &placeholder, double value, bool predecessor) const
{
    if (! predecessor) {
        return false;
    }
    query.bindValue(placeholder, value);
    return true;
}


void
TrainDB::bindWorkout
(QSqlQuery &query, const QString &filepath, const ErgFileBase &ergFileBase) const
{
    query.bindValue(":filepath", filepath);
    query.bindValue(":type", ergFileBase.typeString());
    bindIfValue(query, ":source", ergFileBase.source());
    bindIfValue(query, ":source_id", ergFileBase.ergDBId());
    if (! bindIfValue(query, ":displayname", ergFileBase.name().trimmed())) {
        query.bindValue(":displayname", QFileInfo(filepath).baseName());
    }
    bindIfValue(query, ":description", ergFileBase.description());
    if (ergFileBase.hasWatts()) {
        bindIfValue(query, ":erg_subtype", ergFileBase.ergSubTypeString());
        bindIfValue(query, ":erg_duration", qlonglong(ergFileBase.duration()));
        bindIfValue(query, ":erg_bikestress", ergFileBase.bikeStress());
        bindIfValue(query, ":erg_if", ergFileBase.IF());
        bindIfValue(query, ":erg_iso_power", ergFileBase.IsoPower());
        bindIfValue(query, ":erg_vi", ergFileBase.VI());
        bindIfValue(query, ":erg_xp", ergFileBase.XP());
        bindIfValue(query, ":erg_ri", ergFileBase.RI());
        bindIfValue(query, ":erg_bs", ergFileBase.BS());
        bindIfValue(query, ":erg_svi", ergFileBase.SVI());
        query.bindValue(":erg_min_power", ergFileBase.minWatts());
        query.bindValue(":erg_max_power", ergFileBase.maxWatts());
        query.bindValue(":erg_avg_power", ergFileBase.AP());
        query.bindValue(":erg_dominant_zone", ergFileBase.dominantZoneInt());
        query.bindValue(":erg_num_zones", ergFileBase.numZones());
        bindIfPredecessor(query, ":erg_duration_z1", ergFileBase.powerZonePC(ErgFilePowerZone::z1), ergFileBase.numZones() >= 1);
        bindIfPredecessor(query, ":erg_duration_z2", ergFileBase.powerZonePC(ErgFilePowerZone::z2), ergFileBase.numZones() >= 2);
        bindIfPredecessor(query, ":erg_duration_z3", ergFileBase.powerZonePC(ErgFilePowerZone::z3), ergFileBase.numZones() >= 3);
        bindIfPredecessor(query, ":erg_duration_z4", ergFileBase.powerZonePC(ErgFilePowerZone::z4), ergFileBase.numZones() >= 4);
        bindIfPredecessor(query, ":erg_duration_z5", ergFileBase.powerZonePC(ErgFilePowerZone::z5), ergFileBase.numZones() >= 5);
        bindIfPredecessor(query, ":erg_duration_z6", ergFileBase.powerZonePC(ErgFilePowerZone::z6), ergFileBase.numZones() >= 6);
        bindIfPredecessor(query, ":erg_duration_z7", ergFileBase.powerZonePC(ErgFilePowerZone::z7), ergFileBase.numZones() >= 7);
        bindIfPredecessor(query, ":erg_duration_z8", ergFileBase.powerZonePC(ErgFilePowerZone::z8), ergFileBase.numZones() >= 8);
        bindIfPredecessor(query, ":erg_duration_z9", ergFileBase.powerZonePC(ErgFilePowerZone::z9), ergFileBase.numZones() >= 9);
        bindIfPredecessor(query, ":erg_duration_z10", ergFileBase.powerZonePC(ErgFilePowerZone::z10), ergFileBase.numZones() >= 10);
    } else if (ergFileBase.type() == ErgFileType::slp) {
        query.bindValue(":slp_distance", qlonglong(ergFileBase.duration()));
        query.bindValue(":slp_elevation", ergFileBase.ele());
        query.bindValue(":slp_avg_grade", ergFileBase.grade());
    }
}
