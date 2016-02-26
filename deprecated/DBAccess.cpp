/*
 * Copyright (c) 2006 Justin Knotzke (jknotzke@shampoo.ca)
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "MainWindow.h"
#include "Athlete.h"
#include "DBAccess.h"
#include <QtSql>
#include <QtGui>
#include "RideFile.h"
#include "Zones.h"
#include "Settings.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideMetric.h"
#include "TimeUtils.h"
#include <math.h>
#include <QtXml/QtXml>
#include <QFile>
#include <QFileInfo>
#include "SummaryMetrics.h"
#include "RideMetadata.h"
#include "SpecialFields.h"

// DB Schema Version - YOU MUST UPDATE THIS IF THE SCHEMA VERSION CHANGES!!!
// Schema version will change if a) the default metadata.xml is updated
//                            or b) new metrics are added / old changed
//                            or c) the metricDB tables structures change

// Revision History
// Rev Date         Who                What Changed
//-----------------------------------------------------------------------
//
// ******* Prior to version 29 no revision history was maintained *******
//
// 29  5th Sep 2011 Mark Liversedge    Added color to the ride fields
// 30  8th Sep 2011 Mark Liversedge    Metadata 'data' field for data present string
// 31  22  Nov 2011 Mark Liversedge    Added Average WPK metric
// 32  9th Dec 2011 Damien Grauser     Temperature data flag (metadata field 'Data')
// 33  17  Dec 2011 Damien Grauser     Added ResponseIndex and EfficiencyFactor
// 34  15  Jan 2012 Mark Liversedge    Added Average and Max Temperature and Metric->conversionSum()
// 35  13  Feb 2012 Mark Liversedge    Max/Avg Cadence adjusted conversion
// 36  18  Feb 2012 Mark Liversedge    Added Pace (min/mile) and 250m, 500m Pace metrics
// 37  06  Apr 2012 Rainer Clasen      Added non-zero average Power (watts)
// 38  8th Jul 2012 Mark Liversedge    Computes metrics for manual files now
// 39  18  Aug 2012 Mark Liversedge    New metric LRBalance
// 40  20  Oct 2012 Mark Liversedge    Lucene search/filter and checkbox metadata field
// 41  27  Oct 2012 Mark Liversedge    Lucene switched to StandardAnalyzer and search all texts by default
// 42  03  Dec 2012 Mark Liversedge    W/KG ridefilecache changes - force a rebuild.
// 43  24  Jan 2012 Mark Liversedge    TRIMP update
// 44  19  Apr 2013 Mark Liversedge    Aerobic Decoupling precision reduced to 1pt
// 45  09  May 2013 Mark Liversedge    Added 2,3,8 and 90m peak power for fatigue profiling
// 46  13  May 2013 Mark Liversedge    Handle absence of speed in metric calculations
// 47  17  May 2013 Mark Liversedge    Reimplementation of w/kg and ride->getWeight()
// 48  22  May 2013 Mark Liversedge    Removing local measures.xml, till v3.1
// 49  29  Oct 2013 Mark Liversedge    Added percentage time in zone
// 50  29  Oct 2013 Mark Liversedge    Added percentage time in heartrate zone
// 51  05  Nov 2013 Mark Liversedge    Added average aPower
// 52  05  Nov 2013 Mark Liversedge    Added EOA - Effect of Altitude
// 53  18  Dec 2013 Mark Liversedge    Added Fatigue Index (for power)
// 54  07  Jan 2014 Mark Liversedge    Revised Estimated VO2MAX metric formula
// 55  20  Jan 2014 Mark Liversedge    Added back Minimum W'bal metric and MaxMatch
// 56  20  Jan 2014 Mark Liversedge    Added W' TAU to be able to track it
// 57  20  Jan 2014 Mark Liversedge    Added W' Expenditure for total energy spent above CP
// 58  23  Jan 2014 Mark Liversedge    W' work rename and calculate without reference to WPrime class (speed)
// 59  24  Jan 2014 Mark Liversedge    Added Maximum W' exp which is same as W'bal bur expressed as used not left
// 60  05  Feb 2014 Mark Liversedge    Added Critical Power as a metric -- retrieves from settings for now
// 61  15  Feb 2014 Mark Liversedge    Fixed W' Work (for recintsecs not 1s!).
// 62  06  Mar 2014 Mark Liversedge    Fixed Fatigue Index to find peak then watch for decay, primarily useful in sprint intervals
// 63  06  Mar 2014 Mark Liversedge    Added Pacing Index AP as %age of Max Power
// 64  17  Mar 2014 Mark Liversedge    Added W' and CP work to PMC metrics
// 65  17  Mar 2014 Mark Liversedge    Added Aerobic TISS prototype
// 66  18  Mar 2014 Mark Liversedge    Updated aPower calculation
// 67  22  Mar 2014 Mark Liversedge    Added Anaerobic TISS prototype
// 68  22  Mar 2014 Mark Liversedge    Added dTISS prototype
// 69  23  Mar 2014 Mark Liversedge    Updated Gompertz constansts for An-TISS sigmoid
// 70  27  Mar 2014 Mark Liversedge    Add file CRC to refresh only if contents change (not just timestamps)
// 71  14  Apr 2014 Mark Liversedge    Added average lef/right vector metrics (Pedal Smoothness / Torque Effectiveness)
// 72  24  Apr 2014 Mark Liversedge    Andy Froncioni's faster algorithm for W' bal
// 73  11  May 2014 Mark Liversedge    Default color of 1,1,1 now uses CPLOTMARKER for ride color, change version to force rebuild
// 74  20  May 2014 Mark Liversedge    Added Athlete Weight
// 75  25  May 2014 Mark Liversedge    W' work calculation changed to only include energy above CP
// 76  14  Jun 2014 Mark Liversedge    Add new 'present' field that uses Data tag data
// 77  18  Jun 2014 Mark Liversedge    Add TSS per hour metric
// 78  19  Jun 2014 Mark Liversedge    Do not include zeroes in average L/R pedal smoothness/torque effectiveness
// 79  20  Jun 2014 Mark Liversedge    Change the way average temperature is handled
// 80  13  Jul 2014 Mark Liversedge    W' work + Below CP work = Work
// 81  16  Aug 2014 Joern Rischmueller Added 'Elevation Loss'
// 82  23  Aug 2014 Mark Liversedge    Added W'bal Matches
// 83  05  Sep 2014 Joern Rischmueller Added 'Time Carrying' and 'Elevation Gain Carrying'
// 84  08  Sep 2014 Mark Liversedge    Added HrPw Ratio
// 85  09  Sep 2014 Mark Liversedge    Added HrNp Ratio
// 86  26  Sep 2014 Mark Liversedge    Added isRun first class var
// 87  11  Oct 2014 Mark Liversedge    W'bal inegrator fixed up by Dave Waterworth
// 88  14  Oct 2014 Mark Liversedge    Pace Zone Metrics
// 89  07  Nov 2014 Ale Martinez       GOVSS
// 90  08  Nov 2014 Mark Liversedge    Update data flags for Moxy and Garmin Running Dynamics
// 91  16  Nov 2014 Damien Grauser     Do not include values if data not present in TimeInZone and HRTimeInZone
// 92  21  Nov 2014 Mark Liversedge    Added Watts:RPE ratio
// 93  26  Nov 2014 Mark Liversedge    Added Min, Max, Avg SmO2
// 94  02  Dec 2014 Ale Martinez       Added xPace
// 95  08  Dec 2014 Ale Martinez       Deprecated Measures table

int DBSchemaVersion = 95;

DBAccess::DBAccess(Context* context) : context(context), db(NULL)
{
    initDatabase(context->athlete->home->cache());
}

DBAccess::~DBAccess()
{
    if (db) {
        db->close();
        delete db;
        QSqlDatabase::removeDatabase(sessionid);
    }
}

void
DBAccess::initDatabase(QDir home)
{

    sessionid = QString("%1").arg(context->athlete->cyclist);

    if(db && db->database(sessionid).isOpen()) return;

    // use different name for v3 metricDB to avoid constant rebuilding
    // when switching between v2 stable and v3 development builds
    db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", sessionid));
    db->setDatabaseName(home.canonicalPath() + "/metricDBv3");

    if (!db->database(sessionid).isOpen()) {

        QMessageBox::critical(0, qApp->translate("DBAccess","Cannot open database"),
                       qApp->translate("DBAccess","Unable to establish a database connection.\n"
                                       "This feature requires SQLite support. Please read "
                                       "the Qt SQL driver documentation for information how "
                                       "to build it.\n\n"
                                       "Click Cancel to exit."), QMessageBox::Cancel);
    } else {

        // create database - does nothing if its already there
        createDatabase();
    }
}


bool DBAccess::createMetricsTable()
{
    QSqlQuery query(db->database(sessionid));
    bool rc;
    bool createTables = true;

    // does the table exist?
    rc = query.exec("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
    if (rc) {
        while (query.next()) {

            QString table = query.value(0).toString();
            if (table == "metrics") {
                createTables = false;
                break;
            }
        }
    }

    // we need to create it!
    if (rc && createTables) {
        QString createMetricTable = "create table metrics (filename varchar primary key,"
                                    "identifier varchar,"
                                    "timestamp integer,"
                                    "crc integer,"
                                    "ride_date date,"
                                    "isRun integer,"
                                    "present varchar,"
                                    "color varchar,"
                                    "fingerprint integer";

        // Add columns for all the metric factory metrics
        const RideMetricFactory &factory = RideMetricFactory::instance();
        for (int i=0; i<factory.metricCount(); i++)
            createMetricTable += QString(", X%1 double").arg(factory.metricName(i));

        // And all the metadata texts
        foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            if (!context->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {
                createMetricTable += QString(", Z%1 varchar").arg(context->specialFields.makeTechName(field.name));
            }
        }

        // And all the metadata metrics
        foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            if (!context->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                createMetricTable += QString(", Z%1 double").arg(context->specialFields.makeTechName(field.name));
            }
        }
        createMetricTable += " )";

        rc = query.exec(createMetricTable);
        //if (!rc) qDebug()<<"create table failed!"  << query.lastError();

        // add row to version database
        QString metadataXML =  QString(context->athlete->home->config().absolutePath()) + "/metadata.xml";
        int metadatacrcnow = RideFile::computeFileCRC(metadataXML);
        QDateTime timestamp = QDateTime::currentDateTime();

        // wipe current version row
        query.exec("DELETE FROM version where table_name = \"metrics\"");

        query.prepare("INSERT INTO version (table_name, schema_version, creation_date, metadata_crc ) values (?,?,?,?)");
        query.addBindValue("metrics");
	    query.addBindValue(DBSchemaVersion);
	    query.addBindValue(timestamp.toTime_t());
	    query.addBindValue(metadatacrcnow);
        rc = query.exec();
    }
    return rc;
}

bool DBAccess::dropMetricTable()
{
    QSqlQuery query("DROP TABLE metrics", db->database(sessionid));
    bool rc = query.exec();
    return rc;
}

bool DBAccess::createIntervalMetricsTable()
{
    QSqlQuery query(db->database(sessionid));
    bool rc;
    bool createTables = true;

    // does the table exist?
    rc = query.exec("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
    if (rc) {
        while (query.next()) {

            QString table = query.value(0).toString();
            if (table == "interval_metrics") {
                createTables = false;
                break;
            }
        }
    }

    // we need to create it!
    if (rc && createTables) {
        QString createIntervalMetricTable = "create table interval_metrics (identifier varchar,"
                                    "filename varchar,"
                                    "timestamp integer,"
                                    "crc integer,"
                                    "ride_date date,"
                                    "type varchar,"
                                    "groupName varchar,"
                                    "name varchar,"
                                    "start integer,"
                                    "stop integer,"
                                    "color varchar,"
                                    "fingerprint integer";

        // Add columns for all the metric factory metrics
        const RideMetricFactory &factory = RideMetricFactory::instance();
        for (int i=0; i<factory.metricCount(); i++)
            createIntervalMetricTable += QString(", X%1 double").arg(factory.metricName(i));

        // And all the metadata texts from ride of the intervals
        foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            if (!context->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {
                createIntervalMetricTable += QString(", Z%1 varchar").arg(context->specialFields.makeTechName(field.name));
            }
        }

        // And all the metadata metrics from ride of the intervals
        foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            if (!context->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                createIntervalMetricTable += QString(", Z%1 double").arg(context->specialFields.makeTechName(field.name));
            }
        }
        createIntervalMetricTable += " )";

        rc = query.exec(createIntervalMetricTable);
        if (!rc) qDebug()<<"create table interval_metrics failed!"  << query.lastError();

        // add row to version database
        QString metadataXML =  QString(context->athlete->home->config().absolutePath()) + "/metadata.xml";
        int metadatacrcnow = RideFile::computeFileCRC(metadataXML);
        QDateTime timestamp = QDateTime::currentDateTime();

        // wipe current version row
        query.exec("DELETE FROM version where table_name = \"interval_metrics\"");

        query.prepare("INSERT INTO version (table_name, schema_version, creation_date, metadata_crc ) values (?,?,?,?)");
        query.addBindValue("interval_metrics");
        query.addBindValue(DBSchemaVersion);
        query.addBindValue(timestamp.toTime_t());
        query.addBindValue(metadatacrcnow);
        rc = query.exec();
    }
    return rc;
}

bool DBAccess::dropIntervalMetricTable()
{
    QSqlQuery query("DROP TABLE interval_metrics", db->database(sessionid));
    bool rc = query.exec();
    return rc;
}

bool DBAccess::createDatabase()
{
    // check schema version and if missing recreate database
    checkDBVersion();

    // Ride metrics
	createMetricsTable();

    return true;
}

void DBAccess::checkDBVersion()
{

    // get a CRC for metadata.xml
    QString metadataXML =  QString(context->athlete->home->config().absolutePath()) + "/metadata.xml";
    int metadatacrcnow = RideFile::computeFileCRC(metadataXML);

    // can we get a version number?
    QSqlQuery query("SELECT table_name, schema_version, creation_date, metadata_crc from version;", db->database(sessionid));

    bool rc = query.exec();

    if (!rc) {
        // we couldn't read the version table properly
        // it must be out of date!!

        QSqlQuery dropM("DROP TABLE version", db->database(sessionid));
        dropM.exec();

        // recreate version table and add one entry
        QSqlQuery version("CREATE TABLE version ( table_name varchar primary key, schema_version integer, creation_date date, metadata_crc integer );", db->database(sessionid));
        version.exec();

        // wipe away whatever (if anything is there)
        dropMetricTable();
        dropIntervalMetricTable();

        // create afresh
        createMetricsTable();
        createIntervalMetricsTable();
        return;
    }

    // ok we checked out ok, so lets adjust db schema to reflect
    // tne current version / crc
    bool dropMetric = true;
    bool dropIntervalMetric = true;

    while (query.next()) {

        QString table_name = query.value(0).toString();
        int currentversion = query.value(1).toInt();
        //int creationdate = query.value(2).toInt(); // not relevant anymore, we use version/crc
        int crc = query.value(3).toInt();

        if (table_name == "metrics") {
            if (currentversion == DBSchemaVersion && crc == metadatacrcnow) {
                dropMetric = false;
            }
        }

        if (table_name == "interval_metrics") {
            if (currentversion == DBSchemaVersion && crc == metadatacrcnow) {
                dropIntervalMetric = false;
            }
        }

    }
    query.finish();


    // "metrics" table, is it up-to-date?
    if (dropMetric) {
        dropMetricTable();
        createMetricsTable();
    }

    // "interval_metrics" table, is it up-to-date?
    if (dropIntervalMetric) {
        dropIntervalMetricTable();
        createIntervalMetricsTable();
    }
}

int DBAccess::getDBVersion()
{
    int schema_version = -1;
    // can we get a version number?
    QSqlQuery query("SELECT schema_version from version;", db->database(sessionid));

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
 * CRUD routines for Metrics table
 *----------------------------------------------------------------------*/
bool DBAccess::importRide(SummaryMetrics *summaryMetrics, RideFile *ride, QColor color, unsigned long fingerprint, bool modify)
{
	QSqlQuery query(db->database(sessionid));
    QDateTime timestamp = QDateTime::currentDateTime();

    if (modify) {
        // zap the current row
	    query.prepare("DELETE FROM metrics WHERE filename = ?;");
	    query.addBindValue(summaryMetrics->getFileName());
        query.exec();
    }

    // construct an insert statement
    QString insertStatement = "insert into metrics ( filename, identifier, crc, timestamp, ride_date, isRun, present, color, fingerprint ";
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        insertStatement += QString(", X%1 ").arg(factory.metricName(i));

    // And all the metadata texts
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!context->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {
            insertStatement += QString(", Z%1 ").arg(context->specialFields.makeTechName(field.name));
        }
    }
        // And all the metadata metrics
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!context->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            insertStatement += QString(", Z%1 ").arg(context->specialFields.makeTechName(field.name));
        }
    }

    insertStatement += " ) values (?,?,?,?,?,?,?,?,?"; // filename, identifier, crc, timestamp, ride_date, isRun, present, color, fingerprint
    for (int i=0; i<factory.metricCount(); i++)
        insertStatement += ",?";
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!context->specialFields.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            insertStatement += ",?";
        }
    }
    insertStatement += ")";

	query.prepare(insertStatement);

    // filename, crc, timestamp, ride date
    QString fullPath =  QString(context->athlete->home->activities().absolutePath()) + "/" + summaryMetrics->getFileName();
	query.addBindValue(summaryMetrics->getFileName());
	query.addBindValue(summaryMetrics->getId());
	query.addBindValue((int)RideFile::computeFileCRC(fullPath));
	query.addBindValue(timestamp.toTime_t());
    query.addBindValue(summaryMetrics->getRideDate());
    query.addBindValue(summaryMetrics->isRun());
    query.addBindValue(ride->getTag("Data", ""));
    query.addBindValue(color.name());
    query.addBindValue((int)fingerprint);

    // values
    for (int i=0; i<factory.metricCount(); i++) {
	    query.addBindValue(summaryMetrics->getForSymbol(factory.metricName(i)));
    }

    // And all the metadata texts
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {

        if (!context->specialFields.isMetric(field.name) && (field.type < 3 || field.type ==7)) {
            query.addBindValue(ride->getTag(field.name, ""));
        }
    }
    // And all the metadata metrics
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {

        if (!context->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            query.addBindValue(ride->getTag(field.name, "0.0").toDouble());
        } else if (!context->specialFields.isMetric(field.name)) {
            if (field.name == "Recording Interval") 
                query.addBindValue(ride->recIntSecs());
        }
    }

    // go do it!
	bool rc = query.exec();

	//if(!rc) qDebug() << query.lastError();

	return rc;
}

bool
DBAccess::deleteRide(QString name)
{
    QSqlQuery query(db->database(sessionid));

    query.prepare("DELETE FROM metrics WHERE filename = ?;");
    query.addBindValue(name);
    return query.exec();
}

QList<SummaryMetrics> DBAccess::getAllMetricsFor(QDateTime start, QDateTime end)
{
    QList<SummaryMetrics> metrics;

    // null date range fetches all, but not currently used by application code
    // since it relies too heavily on the results of the QDateTime constructor
    if (start == QDateTime()) start = QDateTime::currentDateTime().addYears(-10);
    if (end == QDateTime()) end = QDateTime::currentDateTime().addYears(+10);

    // construct the select statement
    QString selectStatement = "SELECT filename, identifier, ride_date, isRun";
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        selectStatement += QString(", X%1 ").arg(factory.metricName(i));
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!context->specialFields.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            selectStatement += QString(", Z%1 ").arg(context->specialFields.makeTechName(field.name));
        }
    }
    selectStatement += " FROM metrics where DATE(ride_date) >=DATE(:start) AND DATE(ride_date) <=DATE(:end) "
                       " ORDER BY ride_date;";

    // execute the select statement
    QSqlQuery query(db->database(sessionid));
    query.prepare(selectStatement);
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
        summaryMetrics.setIsRun(query.value(3).toInt());
        // the values
        int i=0;
        for (; i<factory.metricCount(); i++)
            summaryMetrics.setForSymbol(factory.metricName(i), query.value(i+4).toDouble());
        foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            if (!context->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                QString underscored = field.name;
                summaryMetrics.setForSymbol(underscored.replace("_"," "), query.value(i+4).toDouble());
                i++;
            } else if (!context->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {
                QString underscored = field.name;
                summaryMetrics.setText(underscored.replace("_"," "), query.value(i+4).toString());
                i++;
            }
        }
        metrics << summaryMetrics;
    }
    return metrics;
}

/*----------------------------------------------------------------------
 * CRUD routines for Interval Metrics table
 *----------------------------------------------------------------------*/
bool DBAccess::importInterval(SummaryMetrics *summaryMetrics, IntervalItem *interval, QString type, QString groupName, QColor color, unsigned long fingerprint, bool modify)
{
    QSqlQuery query(db->database(sessionid));
    QDateTime timestamp = QDateTime::currentDateTime();

    if (modify) {
        // zap the current row
        query.prepare("DELETE FROM interval_metrics WHERE filename = :filename AND type = :type AND groupName = :groupName AND start = :start;");

        query.bindValue(":filename", summaryMetrics->getFileName());
        query.bindValue(":type", type);
        query.bindValue(":groupName", groupName);
        query.bindValue(":start", interval->start);
        query.exec();
    }

    // construct an insert statement
    QString insertStatement = "insert into interval_metrics ( identifier, filename, crc, timestamp, ride_date, type, groupName, name, start, stop, color, fingerprint ";

    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        insertStatement += QString(", X%1 ").arg(factory.metricName(i));

    // And all the metadata texts
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!context->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {
            insertStatement += QString(", Z%1 ").arg(context->specialFields.makeTechName(field.name));
        }
    }
        // And all the metadata metrics
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!context->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            insertStatement += QString(", Z%1 ").arg(context->specialFields.makeTechName(field.name));
        }
    }


    insertStatement += " ) values (?,?,?,?,?,?,?,?,?,?,?,?"; // identifier, filename, crc, timestamp, ride_date, type, groupName, name, start, stop, color, fingerprint

    for (int i=0; i<factory.metricCount(); i++)
        insertStatement += ",?";
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!context->specialFields.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            insertStatement += ",?";
        }
    }
    insertStatement += ");";

    query.prepare(insertStatement);

    // filename, crc, timestamp, ride date
    QString fullPath =  QString(context->athlete->home->activities().absolutePath()) + "/" + summaryMetrics->getFileName();
    query.addBindValue(summaryMetrics->getFileName()+"-"+interval->displaySequence);
    query.addBindValue(summaryMetrics->getFileName());
    query.addBindValue((int)RideFile::computeFileCRC(fullPath));
    query.addBindValue(timestamp.toTime_t());
    query.addBindValue(summaryMetrics->getRideDate());
    query.addBindValue(type); // type
    query.addBindValue(groupName); // groupName,
    query.addBindValue(interval->text(0)); // name,
    query.addBindValue(interval->start); // start,
    query.addBindValue(interval->stop); // stop,
    query.addBindValue(color.name());
    query.addBindValue((int)fingerprint);

    // values
    for (int i=0; i<factory.metricCount(); i++) {
        query.addBindValue(summaryMetrics->getForSymbol(factory.metricName(i)));
    }

    // And all the metadata texts
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {

        if (!context->specialFields.isMetric(field.name) && (field.type < 3 || field.type ==7)) {
            query.addBindValue(interval->ride->getTag(field.name, ""));
        }
    }
    // And all the metadata metrics
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {

        if (!context->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            query.addBindValue(interval->ride->getTag(field.name, "0.0").toDouble());
        } else if (!context->specialFields.isMetric(field.name)) {
            if (field.name == "Recording Interval")
                query.addBindValue(interval->ride->recIntSecs());
        }
    }

    // go do it!
    bool rc = query.exec();

    if(!rc) qDebug() << query.lastError();

    return rc;
}

bool
DBAccess::deleteIntervalsForRide(QString filename)
{
    QSqlQuery query(db->database(sessionid));

    query.prepare("DELETE FROM interval_metrics WHERE filename = ?;");
    query.addBindValue(filename);
    return query.exec();
}

bool
DBAccess::deleteIntervalsForTypeAndGroupName(QString type, QString groupName)
{
    QSqlQuery query(db->database(sessionid));

    query.prepare("DELETE FROM interval_metrics WHERE type = :type AND groupName = :groupName;");
    query.addBindValue(type); // type
    query.addBindValue(groupName); // groupName,
    return query.exec();
}

bool
DBAccess::getInterval(QString filename, QString type, QString groupName, int start, SummaryMetrics &summaryMetrics, QColor&color)
{
    // lookup a ride by filename returning true/false if found
    bool found = false;

    // construct the select statement
    QString selectStatement = "SELECT identifier, filename, crc, type, groupName, color"; //identifier, filename, crc, timestamp, ride_date, type, groupName, name, start, stop

    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        selectStatement += QString(", X%1 ").arg(factory.metricName(i));
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!context->specialFields.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            selectStatement += QString(", Z%1 ").arg(context->specialFields.makeTechName(field.name));
        }
    }
    selectStatement += " FROM interval_metrics where filename = :filename AND type = :type AND groupName = :groupName AND start = :start;";

    // execute the select statement
    QSqlQuery query(selectStatement, db->database(sessionid));
    query.bindValue(":filename", filename);
    query.bindValue(":type", type);
    query.bindValue(":groupName", groupName);
    query.bindValue(":start", start);
    query.exec();

    while(query.next())
    {
        found = true;

        // filename and date
        summaryMetrics.setFileName(query.value(0).toString());
        summaryMetrics.setId(query.value(1).toString());
        summaryMetrics.setRideDate(query.value(2).toDateTime());
        summaryMetrics.setIsRun(query.value(3).toInt());
        color = QColor(query.value(4).toString());

        // the values
        int i=0;
        for (; i<factory.metricCount(); i++)
            summaryMetrics.setForSymbol(factory.metricName(i), query.value(i+5).toDouble());

        foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            if (!context->specialFields.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                QString underscored = field.name;
                summaryMetrics.setForSymbol(underscored.replace("_"," "), query.value(i+4).toDouble());
                i++;
            } else if (!context->specialFields.isMetric(field.name) && field.type < 3) {
                QString underscored = field.name;
                summaryMetrics.setText(underscored.replace("_"," "), query.value(i+4).toString());
                i++;
            }
        }
    }
    query.finish();
    return found;
}

