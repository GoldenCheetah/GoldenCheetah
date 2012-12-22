/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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
#include "ErgFile.h"

// DB Schema Version - YOU MUST UPDATE THIS IF THE TRAIN DB SCHEMA CHANGES

// Revision History
// Rev Date         Who                What Changed
// 01  21 Dec 2012  Mark Liversedge    Initial Build

static int TrainDBSchemaVersion = 1;
TrainDB *trainDB;

TrainDB::TrainDB(QDir home) : home(home)
{
    // we live above the rider directory
	initDatabase(home);
}

void TrainDB::closeConnection()
{
    dbconn.close();
}

TrainDB::~TrainDB()
{
    closeConnection();
}

void
TrainDB::initDatabase(QDir home)
{


    if(dbconn.isOpen()) return;

    // get a connection
    db = QSqlDatabase::addDatabase("QSQLITE", "1");
    db.setDatabaseName(home.absolutePath() + "/trainDB"); 
    dbconn = db.database("1");

    if (!dbconn.isOpen()) {
        QMessageBox::critical(0, qApp->translate("TrainDB","Cannot open database"),
                       qApp->translate("TrainDB","Unable to establish a database connection.\n"
                                       "This feature requires SQLite support. Please read "
                                       "the Qt SQL driver documentation for information how "
                                       "to build it.\n\n"
                                       "Click Cancel to exit."), QMessageBox::Cancel);
    } else {

        // create database - does nothing if its already there
        createDatabase();
    }
}

bool TrainDB::createVideoTable()
{
    QSqlQuery query(dbconn);
    bool rc;
    bool createTables = true;

    // does the table exist?
    rc = query.exec("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
    if (rc) {
        while (query.next()) {

            QString table = query.value(0).toString();
            if (table == "videos") {
                createTables = false;
                break;
            }
        }
    }
    // we need to create it!
    if (rc && createTables) {

        QString createVideoTable = "create table videos (filepath varchar primary key,"
                                    "filename varchar,"
                                    "timestamp integer,"
                                    "length integer);";

        rc = query.exec(createVideoTable);

        // insert the 'DVD' record for playing currently loaded DVD
        //XXX rc = query.exec("INSERT INTO videos (filepath, filename) values (\"\", \"DVD\");");

        // add row to version database
        query.exec("DELETE FROM version where table_name = \"videos\"");

        // insert into table
        query.prepare("INSERT INTO version (table_name, schema_version, creation_date) values (?,?,?);");
        query.addBindValue("videos");
	    query.addBindValue(TrainDBSchemaVersion);
	    query.addBindValue(QDateTime::currentDateTime().toTime_t());
        rc = query.exec();
    }
    return rc;
}

bool TrainDB::createWorkoutTable()
{
    QSqlQuery query(dbconn);
    bool rc;
    bool createTables = true;

    // does the table exist?
    rc = query.exec("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
    if (rc) {
        while (query.next()) {

            QString table = query.value(0).toString();
            if (table == "workouts") {
                createTables = false;
                break;
            }
        }
    }
    // we need to create it!
    if (rc && createTables) {

        QString createMetricTable = "create table workouts (filepath varchar primary key,"
                                    "filename,"
                                    "timestamp integer,"
                                    "description varchar,"
                                    "source varchar,"
                                    "ftp integer,"
                                    "length integer,"
                                    "coggan_tss integer,"
                                    "coggan_if integer,"
                                    "elevation integer,"
                                    "grade double );";

        rc = query.exec(createMetricTable);

        QString manualErg = QString("INSERT INTO workouts (filepath, filename) values (\"//1\", \"%1\");")
                         .arg(tr("Manual Erg Mode"));
        rc = query.exec(manualErg);

        QString manualCrs = QString("INSERT INTO workouts (filepath, filename) values (\"//2\", \"%1\");")
                         .arg(tr("Manual Slope Mode"));
        rc = query.exec(manualCrs);

        // add row to version database
        query.exec("DELETE FROM version where table_name = \"workouts\"");

        // insert into table
        query.prepare("INSERT INTO version (table_name, schema_version, creation_date) values (?,?,?);");
        query.addBindValue("workouts");
	    query.addBindValue(TrainDBSchemaVersion);
	    query.addBindValue(QDateTime::currentDateTime().toTime_t());
        rc = query.exec();
    }
    return rc;
}

bool TrainDB::dropVideoTable()
{
    QSqlQuery query("DROP TABLE videos", dbconn);
    bool rc = query.exec();
    return rc;
}

bool TrainDB::dropWorkoutTable()
{
    QSqlQuery query("DROP TABLE workouts", dbconn);
    bool rc = query.exec();
    return rc;
}

bool TrainDB::createDatabase()
{
    // check schema version and if missing recreate database
    checkDBVersion();

    // Workouts
	createWorkoutTable();
	createVideoTable();

    return true;
}

void TrainDB::checkDBVersion()
{
    // can we get a version number?
    QSqlQuery query("SELECT table_name, schema_version, creation_date from version;", dbconn);

    bool rc = query.exec();

    if (!rc) {
        // we couldn't read the version table properly
        // it must be out of date!!

        QSqlQuery dropM("DROP TABLE version", dbconn);
        dropM.exec();

        // recreate version table and add one entry
        QSqlQuery version("CREATE TABLE version ( table_name varchar primary key, schema_version integer, creation_date date );", dbconn);
        version.exec();

        // wipe away whatever (if anything is there)
        dropWorkoutTable();
        createWorkoutTable();

        dropVideoTable();
        createVideoTable();
        return;
    }

    // ok we checked out ok, so lets adjust db schema to reflect
    // tne current version / crc
    bool dropWorkout = false;
    bool dropVideo = false;
    while (query.next()) {

        QString table_name = query.value(0).toString();
        int currentversion = query.value(1).toInt();

        if (table_name == "workouts" && currentversion != TrainDBSchemaVersion) dropWorkout = true;
        if (table_name == "videos" && currentversion != TrainDBSchemaVersion) dropVideo = true;
    }
    query.finish();

    // "workouts" table, is it up-to-date?
    if (dropWorkout) dropWorkoutTable();
    if (dropVideo) dropVideoTable();
}

int TrainDB::getDBVersion()
{
    int schema_version = -1;

    // can we get a version number?
    QSqlQuery query("SELECT schema_version from version;", dbconn);

    bool rc = query.exec();

    if (rc) {
        while (query.next()) {
            if (query.value(0).toInt() > schema_version)
                schema_version = query.value(0).toInt();
        }
    }
    query.finish();
    return schema_version;
}

/*----------------------------------------------------------------------
 * CRUD routines
 *----------------------------------------------------------------------*/
bool TrainDB::importWorkout(QString pathname, ErgFile *ergFile)
{
	QSqlQuery query(dbconn);
    QDateTime timestamp = QDateTime::currentDateTime();

    // zap the current row - if there is one
    query.prepare("DELETE FROM workouts WHERE filepath = ?;");
    query.addBindValue(pathname);
    query.exec();

    // construct an insert statement
    QString insertStatement = "insert into workouts ( filepath, "
                                    "filename,"
                                    "timestamp,"
                                    "description,"
                                    "source,"
                                    "ftp,"
                                    "length,"
                                    "coggan_tss,"
                                    "coggan_if,"
                                    "elevation,"
                                    "grade ) values ( ?,?,?,?,?,?,?,?,?,?,? );";
	query.prepare(insertStatement);

    // filename, timestamp, ride date
	query.addBindValue(pathname);
	query.addBindValue(QFileInfo(pathname).fileName());
	query.addBindValue(timestamp);
    query.addBindValue(ergFile->Name);
	query.addBindValue(ergFile->Source);
	query.addBindValue(ergFile->Ftp);
	query.addBindValue((int)ergFile->Duration);
	query.addBindValue(ergFile->TSS);
	query.addBindValue(ergFile->IF);
	query.addBindValue(ergFile->ELE);
	query.addBindValue(ergFile->GRADE);

    // go do it!
	bool rc = query.exec();

	return rc;
}

bool TrainDB::importVideo(QString pathname)
{
	QSqlQuery query(dbconn);
    QDateTime timestamp = QDateTime::currentDateTime();

    // zap the current row - if there is one
    query.prepare("DELETE FROM videos WHERE filepath = ?;");
    query.addBindValue(pathname);
    query.exec();

    // construct an insert statement
    QString insertStatement = "insert into videos ( filepath,filename ) values ( ?,? );";
	query.prepare(insertStatement);

    // filename, timestamp, ride date
	query.addBindValue(pathname);
	query.addBindValue(QFileInfo(pathname).fileName());

    // go do it!
	bool rc = query.exec();

	return rc;
}

#if 0

// XXX WILL UPDATE THESE FOR WORKOUT/VIDEO SHORTLY
bool
TrainDB::deleteRide(QString name)
{
    QSqlQuery query(dbconn);

    query.prepare("DELETE FROM metrics WHERE filename = ?;");
    query.addBindValue(name);
    return query.exec();
}

QList<QDateTime> TrainDB::getAllDates()
{
    QSqlQuery query("SELECT ride_date from metrics ORDER BY ride_date;", dbconn);
    QList<QDateTime> dates;

    query.exec();
    while(query.next())
    {
        QDateTime date = query.value(0).toDateTime();
        dates << date;
    }
    return dates;
}

bool
TrainDB::getRide(QString filename, SummaryMetrics &summaryMetrics, QColor&color)
{
    // lookup a ride by filename returning true/false if found
    bool found = false;

    // construct the select statement
    QString selectStatement = "SELECT filename, identifier, ride_date, color";
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        selectStatement += QString(", X%1 ").arg(factory.metricName(i));
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!main->specialFields.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            selectStatement += QString(", Z%1 ").arg(main->specialFields.makeTechName(field.name));
        }
    }
    selectStatement += " FROM metrics where filename = :name;";

    // execute the select statement
    QSqlQuery query(selectStatement, dbconn);
    query.bindValue(":start", filename);
    query.exec();

    while(query.next())
    {
        found = true;

        // filename and date
        summaryMetrics.setFileName(query.value(0).toString());
        summaryMetrics.setId(query.value(1).toString());
        summaryMetrics.setRideDate(query.value(2).toDateTime());
        color = QColor(query.value(3).toString());

        // the values
        int i=0;
        for (; i<factory.metricCount(); i++)
            summaryMetrics.setForSymbol(factory.metricName(i), query.value(i+4).toDouble());

        foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
            if (!main->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                QString underscored = field.name;
                summaryMetrics.setForSymbol(underscored.replace("_"," "), query.value(i+4).toDouble());
                i++;
            } else if (!main->specialFields.isMetric(field.name) && field.type < 3) {
                QString underscored = field.name;
                // ignore texts for now XXX todo if want metadata from Summary Metrics
                summaryMetrics.setText(underscored.replace("_"," "), query.value(i+4).toString());
                i++;
            }
        }
    }
    return found;
}

QList<SummaryMetrics> TrainDB::getAllMetricsFor(QDateTime start, QDateTime end)
{
    QList<SummaryMetrics> metrics;

    // null date range fetches all, but not currently used by application code
    // since it relies too heavily on the results of the QDateTime constructor
    if (start == QDateTime()) start = QDateTime::currentDateTime().addYears(-10);
    if (end == QDateTime()) end = QDateTime::currentDateTime().addYears(+10);

    // construct the select statement
    QString selectStatement = "SELECT filename, identifier, ride_date";
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        selectStatement += QString(", X%1 ").arg(factory.metricName(i));
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!main->specialFields.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            selectStatement += QString(", Z%1 ").arg(main->specialFields.makeTechName(field.name));
        }
    }
    selectStatement += " FROM metrics where DATE(ride_date) >=DATE(:start) AND DATE(ride_date) <=DATE(:end) "
                       " ORDER BY ride_date;";

    // execute the select statement
    QSqlQuery query(selectStatement, dbconn);
    query.bindValue(":start", start.date());
    query.bindValue(":end", end.date());
    query.exec();
    while(query.next())
    {
        SummaryMetrics summaryMetrics;

        // filename and date
        summaryMetrics.setFileName(query.value(0).toString());
        summaryMetrics.setId(query.value(1).toString());
        summaryMetrics.setRideDate(query.value(2).toDateTime());
        // the values
        int i=0;
        for (; i<factory.metricCount(); i++)
            summaryMetrics.setForSymbol(factory.metricName(i), query.value(i+3).toDouble());
        foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
            if (!main->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                QString underscored = field.name;
                summaryMetrics.setForSymbol(underscored.replace("_"," "), query.value(i+3).toDouble());
                i++;
            } else if (!main->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {
                QString underscored = field.name;
                // ignore texts for now XXX todo if want metadata from Summary Metrics
                summaryMetrics.setText(underscored.replace("_"," "), query.value(i+3).toString());
                i++;
            }
        }
        metrics << summaryMetrics;
    }
    return metrics;
}

SummaryMetrics TrainDB::getRideMetrics(QString filename)
{
    SummaryMetrics summaryMetrics;

    // construct the select statement
    QString selectStatement = "SELECT filename, identifier";
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        selectStatement += QString(", X%1 ").arg(factory.metricName(i));
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!main->specialFields.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            selectStatement += QString(", Z%1 ").arg(main->specialFields.makeTechName(field.name));
        }
    }
    selectStatement += " FROM metrics where filename == :filename ;";

    // execute the select statement
    QSqlQuery query(selectStatement, dbconn);
    query.bindValue(":filename", filename);
    query.exec();
    while(query.next())
    {
        // filename and date
        summaryMetrics.setFileName(query.value(0).toString());
        summaryMetrics.setId(query.value(1).toString());
        // the values
        int i=0;
        for (; i<factory.metricCount(); i++)
            summaryMetrics.setForSymbol(factory.metricName(i), query.value(i+2).toDouble());
        foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
            if (!main->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                QString underscored = field.name;
                summaryMetrics.setForSymbol(underscored.replace(" ","_"), query.value(i+2).toDouble());
                i++;
            } else if (!main->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {
                // ignore texts for now XXX todo if want metadata from Summary Metrics
                QString underscored = field.name;
                summaryMetrics.setText(underscored.replace("_"," "), query.value(i+2).toString());
                i++;
            }
        }
    }
    return summaryMetrics;
}

/*----------------------------------------------------------------------
 * CRUD routines for Measures table
 *----------------------------------------------------------------------*/
bool TrainDB::importMeasure(SummaryMetrics *summaryMetrics)
{
	QSqlQuery query(dbconn);

    // construct an insert statement
    QString insertStatement = "insert into measures (timestamp, measure_date";

    // And all the metadata texts
    foreach(FieldDefinition field, mfieldDefinitions) {
        if (field.type < 3 || field.type == 7) {
            insertStatement += QString(", Z%1 ").arg(msp.makeTechName(field.name));
        }
    }
        // And all the metadata metrics
    foreach(FieldDefinition field, mfieldDefinitions) {
        if (field.type == 3 || field.type == 4) {
            insertStatement += QString(", Z%1 ").arg(msp.makeTechName(field.name));
        }
    }

    insertStatement += " ) values (?,?"; // timestamp, measure_date

    foreach(FieldDefinition field, mfieldDefinitions) {
        if (field.type < 5 || field.type == 7) {
            insertStatement += ",?";
        }
    }
    insertStatement += ")";

	query.prepare(insertStatement);

    // timestamp and date
    query.addBindValue(summaryMetrics->getDateTime().toTime_t());
    query.addBindValue(summaryMetrics->getDateTime().date());

    // And all the text measures
    foreach(FieldDefinition field, mfieldDefinitions) {
        if (field.type < 3 || field.type == 7) {
            query.addBindValue(summaryMetrics->getText(field.name, ""));
        }
    }
    // And all the numeric measures
    foreach(FieldDefinition field, mfieldDefinitions) {
        if (field.type == 3 || field.type == 4) {
            query.addBindValue(summaryMetrics->getText(field.name, "nan").toDouble());
        }
    }
    // go do it!
	bool rc = query.exec();

    //if(!rc) qDebug() << query.lastError();

	return rc;
}

QList<SummaryMetrics> TrainDB::getAllMeasuresFor(QDateTime start, QDateTime end)
{
    QList<FieldDefinition> fieldDefinitions;
    QList<KeywordDefinition> keywordDefinitions; //NOTE: not used in measures.xml
    QString colorfield;

    // check we have one and use built in if not there
    QString filename = main->home.absolutePath()+"/measures.xml";
    if (!QFile(filename).exists()) filename = ":/xml/measures.xml";
    RideMetadata::readXML(filename, keywordDefinitions, fieldDefinitions, colorfield);

    QList<SummaryMetrics> measures;

    // null date range fetches all, but not currently used by application code
    // since it relies too heavily on the results of the QDateTime constructor
    if (start == QDateTime()) start = QDateTime::currentDateTime().addYears(-10);
    if (end == QDateTime()) end = QDateTime::currentDateTime().addYears(+10);

    // construct the select statement
    QString selectStatement = "SELECT timestamp, measure_date";
    foreach(FieldDefinition field, fieldDefinitions) {
        if (!main->specialFields.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            selectStatement += QString(", Z%1 ").arg(main->specialFields.makeTechName(field.name));
        }
    }
    selectStatement += " FROM measures where DATE(measure_date) >=DATE(:start) AND DATE(measure_date) <=DATE(:end) "
                       " ORDER BY measure_date;";

    // execute the select statement
    QSqlQuery query(selectStatement, dbconn);
    query.bindValue(":start", start.date());
    query.bindValue(":end", end.date());
    query.exec();
    while(query.next())
    {
        SummaryMetrics add;

        // filename and date
        add.setDateTime(query.value(1).toDateTime());
        // the values
        int i=2;
        foreach(FieldDefinition field, fieldDefinitions) {
            if (field.type == 3 || field.type == 4) {
                add.setText(field.name, query.value(i).toString());
                i++;
            } else if (field.type < 3 || field.type == 7) {
                add.setText(field.name, query.value(i).toString());
                i++;
            }
        }
        measures << add;
    }
    return measures;
}
#endif
