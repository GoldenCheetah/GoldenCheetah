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
#include "SummaryMetrics.h"

// DB Schema Version - YOU MUST UPDATE THIS IF THE SCHEMA VERSION CHANGES!!!
static int DBSchemaVersion = 10;

// each DB connection gets a unique session id based upon this number:
int DBAccess::session=0;

DBAccess::DBAccess(QDir home)
{
	initDatabase(home);
}

void DBAccess::closeConnection()
{
    dbconn.close();
}

void
DBAccess::initDatabase(QDir home)
{
	
    
    if(dbconn.isOpen()) return;

    sessionid = QString("session%1").arg(session++);

    db = QSqlDatabase::addDatabase("QSQLITE", sessionid);
    db.setDatabaseName(home.absolutePath() + "/metricDB");

    dbconn = db.database(sessionid);

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
                                    "ride_date date";

        // Add columns for all the metrics
        const RideMetricFactory &factory = RideMetricFactory::instance();
        for (int i=0; i<factory.metricCount(); i++)
            createMetricTable += QString(", X%1 double").arg(factory.metricName(i));
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

void DBAccess::checkDBVersion()
{
    int currentversion = 0;

    // can we get a version number?
    QSqlQuery query("SELECT schema_version from version;", dbconn);

    bool rc = query.exec();
    while(rc && query.next()) currentversion = query.value(0).toInt();

    // if its not up-to-date
    if (!rc || currentversion != DBSchemaVersion) {
        // drop tables
        QSqlQuery dropV("DROP TABLE version", dbconn);
        dropV.exec();
        QSqlQuery dropM("DROP TABLE metrics", dbconn);
        dropM.exec();

        // recreate version table and add one entry
        QSqlQuery version("CREATE TABLE version ( schema_version integer primary key);", dbconn);
        version.exec();

        // insert current version number
        QSqlQuery insert("INSERT INTO version ( schema_version ) values (?)", dbconn);
	    insert.addBindValue(DBSchemaVersion);
        insert.exec();
    }
}

/*----------------------------------------------------------------------
 * CRUD routines for Metrics table
 *----------------------------------------------------------------------*/
bool DBAccess::importRide(SummaryMetrics *summaryMetrics, bool modify)
{
	QSqlQuery query(dbconn);
    QDateTime timestamp = QDateTime::currentDateTime();

    if (modify) {
        // zap the current row
	    query.prepare("DELETE FROM metrics WHERE filename = ?;");
	    query.addBindValue(summaryMetrics->getFileName());
        query.exec();
    }

    // construct an insert statement
    QString insertStatement = "insert into metrics ( filename, timestamp, ride_date";
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++)
        insertStatement += QString(", X%1 ").arg(factory.metricName(i));
    insertStatement += " ) values (?,?,?"; // filename, timestamp, ride_date
    for (int i=0; i<factory.metricCount(); i++)
        insertStatement += ",?";
    insertStatement += ")";

	query.prepare(insertStatement);

    // filename, timestamp, ride date
	query.addBindValue(summaryMetrics->getFileName());
	query.addBindValue(timestamp.toTime_t());
    query.addBindValue(summaryMetrics->getRideDate());

    // values
    for (int i=0; i<factory.metricCount(); i++) {
	    query.addBindValue(summaryMetrics->getForSymbol(factory.metricName(i)));
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
        for (int i=0; i<factory.metricCount(); i++)
            summaryMetrics.setForSymbol(factory.metricName(i), query.value(i+2).toDouble());

        metrics << summaryMetrics;
    }
    return metrics;
}

