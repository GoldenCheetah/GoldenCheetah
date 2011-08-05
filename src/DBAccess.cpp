/* 
 * Copyright (c) 2006 Justin Knotzke (jknotzke@shampoo.ca)
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

#include "DBAccess.h"
#include <QtSql>
#include <QtGui>
#include "RideFile.h"
#include "Zones.h"
#include "Settings.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "TimeUtils.h"
#include <assert.h>
#include <math.h>
#include <QtXml/QtXml>
#include <QFile>
#include <QFileInfo>
#include "SummaryMetrics.h"
#include "RideMetadata.h"
#include "SpecialFields.h"

#include <boost/scoped_array.hpp>
#include <boost/crc.hpp>

// DB Schema Version - YOU MUST UPDATE THIS IF THE SCHEMA VERSION CHANGES!!!
// Schema version will change if a) the default metadata.xml is updated
//                            or b) new metrics are added / old changed
static int DBSchemaVersion = 17;

DBAccess::DBAccess(MainWindow* main, QDir home) : main(main), home(home)
{
	initDatabase(home);
}

void DBAccess::closeConnection()
{
    dbconn.close();
}

DBAccess::~DBAccess()
{
    closeConnection();
}

void
DBAccess::initDatabase(QDir home)
{
	
    
    if(dbconn.isOpen()) return;

    QString cyclist = QFileInfo(home.path()).baseName();
    sessionid = QString("%1%2").arg(cyclist).arg(main->session++);

    if (main->session == 1) {
        // first
        main->db = QSqlDatabase::addDatabase("QSQLITE", sessionid);
        main->db.setDatabaseName(home.absolutePath() + "/metricDB");
        //dbconn = db.database(QString("GC"));
        dbconn = main->db.database(sessionid);
    } else {
        // clone the first one!
        dbconn = QSqlDatabase::cloneDatabase(main->db, sessionid);
        dbconn.open();
    }

    if (!dbconn.isOpen()) {
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
    SpecialFields sp;
    QSqlQuery query(dbconn);
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
                                    "timestamp integer,"
                                    "ride_date date,"
                                    "fingerprint integer";

        // Add columns for all the metric factory metrics
        const RideMetricFactory &factory = RideMetricFactory::instance();
        for (int i=0; i<factory.metricCount(); i++)
            createMetricTable += QString(", X%1 double").arg(factory.metricName(i));

        // And all the metadata metrics
        foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
            if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                createMetricTable += QString(", Z%1 double").arg(sp.makeTechName(field.name));
            }
        }
        createMetricTable += " )";

        rc = query.exec(createMetricTable);
        if (!rc) {
            qDebug()<<"create table failed!"  << query.lastError();
        }
    }
    return rc;
}

bool DBAccess::dropMetricTable()
{
    QSqlQuery query("DROP TABLE metrics", dbconn);

    return query.exec();
}

bool DBAccess::createDatabase()
{
    // check schema version and if missing recreate database
    checkDBVersion();

    // at present only one table!
	bool rc = createMetricsTable();
    if(!rc) return rc;

    // other tables here
    return true;
}

static int
computeFileCRC(QString filename)
{
    QFile file(filename);
    QFileInfo fileinfo(file);

    // open file
    if (!file.open(QFile::ReadOnly)) return 0;

    // allocate space
    boost::scoped_array<char> data(new char[file.size()]);

    // read entire file into memory
    QDataStream *rawstream(new QDataStream(&file));
    rawstream->readRawData(&data[0], file.size());
    file.close();

    // calculate the CRC
    boost::crc_optimal<16, 0x1021, 0xFFFF, 0, false, false> CRC;
    CRC.process_bytes(&data[0], file.size());

    return CRC.checksum();
}

void DBAccess::checkDBVersion()
{
    int currentversion = 0;
    int metadatacrc;    // crc for metadata.xml when last refreshed
    int metadatacrcnow; // current value for metadata.xml crc
    int creationdate;

    // get a CRC for metadata.xml
    QString metadataXML =  QString(home.absolutePath()) + "/metadata.xml";
    metadatacrcnow = computeFileCRC(metadataXML);

    // can we get a version number?
    QSqlQuery query("SELECT schema_version, creation_date, metadata_crc from version;", dbconn);

    bool rc = query.exec();
    while (rc && query.next()) {
        currentversion = query.value(0).toInt();
        creationdate = query.value(1).toInt();
        metadatacrc = query.value(2).toInt();
    }

    // if its not up-to-date
    if (!rc || currentversion != DBSchemaVersion || metadatacrc != metadatacrcnow) {

        // drop tables
        QSqlQuery dropV("DROP TABLE version", dbconn);
        dropV.exec();
        QSqlQuery dropM("DROP TABLE metrics", dbconn);
        dropM.exec();

        // recreate version table and add one entry
        QSqlQuery version("CREATE TABLE version ( schema_version integer primary key, creation_date date, metadata_crc integer );", dbconn);
        version.exec();

        // insert current version number
        QDateTime timestamp = QDateTime::currentDateTime();
        QSqlQuery insert("INSERT INTO version ( schema_version, creation_date, metadata_crc ) values (?,?,?)", dbconn);
	    insert.addBindValue(DBSchemaVersion);
	    insert.addBindValue(timestamp.toTime_t());
	    insert.addBindValue(metadatacrcnow);
        insert.exec();

        createMetricsTable();
    }
}

/*----------------------------------------------------------------------
 * CRUD routines for Metrics table
 *----------------------------------------------------------------------*/
bool DBAccess::importRide(SummaryMetrics *summaryMetrics, RideFile *ride, unsigned long fingerprint, bool modify)
{
    SpecialFields sp;
	QSqlQuery query(dbconn);
    QDateTime timestamp = QDateTime::currentDateTime();

    if (modify) {
        // zap the current row
	    query.prepare("DELETE FROM metrics WHERE filename = ?;");
	    query.addBindValue(summaryMetrics->getFileName());
        query.exec();
    }

    // construct an insert statement
    QString insertStatement = "insert into metrics ( filename, timestamp, ride_date, fingerprint ";
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        insertStatement += QString(", X%1 ").arg(factory.metricName(i));

    // And all the metadata metrics
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            insertStatement += QString(", Z%1 ").arg(sp.makeTechName(field.name));
        }
    }

    insertStatement += " ) values (?,?,?,?"; // filename, timestamp, ride_date
    for (int i=0; i<factory.metricCount(); i++)
        insertStatement += ",?";
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            insertStatement += ",?";
        }
    }
    insertStatement += ")";

	query.prepare(insertStatement);

    // filename, timestamp, ride date
	query.addBindValue(summaryMetrics->getFileName());
	query.addBindValue(timestamp.toTime_t());
    query.addBindValue(summaryMetrics->getRideDate());
    query.addBindValue((int)fingerprint);

    // values
    for (int i=0; i<factory.metricCount(); i++) {
	    query.addBindValue(summaryMetrics->getForSymbol(factory.metricName(i)));
    }

    // And all the metadata metrics
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {

        if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            query.addBindValue(ride->getTag(field.name, "0.0").toDouble());
        } else if (!sp.isMetric(field.name)) {
            if (field.name == "Recording Interval")  // XXX Special - need a better way...
                query.addBindValue(ride->recIntSecs());
        }
    }

    // go do it!
	bool rc = query.exec();

	if(!rc)
		qDebug() << query.lastError();

	return rc;
}

bool
DBAccess::deleteRide(QString name)
{
    QSqlQuery query(dbconn);

    query.prepare("DELETE FROM metrics WHERE filename = ?;");
    query.addBindValue(name);
    return query.exec();
}

QList<QDateTime> DBAccess::getAllDates()
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

QList<SummaryMetrics> DBAccess::getAllMetricsFor(QDateTime start, QDateTime end)
{
    SpecialFields sp;
    QList<SummaryMetrics> metrics;

    // null date range fetches all, but not currently used by application code
    // since it relies too heavily on the results of the QDateTime constructor
    if (start == QDateTime()) start = QDateTime::currentDateTime().addYears(-10);
    if (end == QDateTime()) end = QDateTime::currentDateTime().addYears(+10);

    // construct the select statement
    QString selectStatement = "SELECT filename, ride_date";
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        selectStatement += QString(", X%1 ").arg(factory.metricName(i));
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            selectStatement += QString(", Z%1 ").arg(sp.makeTechName(field.name));
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
        summaryMetrics.setRideDate(query.value(1).toDateTime());
        // the values
        int i=0;
        for (; i<factory.metricCount(); i++)
            summaryMetrics.setForSymbol(factory.metricName(i), query.value(i+2).toDouble());
        foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
            if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
                QString underscored = field.name;
                summaryMetrics.setForSymbol(underscored.replace(" ","_"), query.value(i+2).toDouble());
                i++;
            }
        }
        metrics << summaryMetrics;
    }
    return metrics;
}

