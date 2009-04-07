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


DBAccess::DBAccess(QDir home)
{
	initDatabase(home);
}

void DBAccess::closeConnection()
{
    db.close();
}

QSqlDatabase DBAccess::initDatabase(QDir home)
{
	
    
    if(db.isOpen())
        return db;
    
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(home.absolutePath() + "/metricDB");
    if (!db.open()) {
        QMessageBox::critical(0, qApp->tr("Cannot open database"),
                              qApp->tr("Unable to establish a database connection.\n"
                                       "This example needs SQLite support. Please read "
                                       "the Qt SQL driver documentation for information how "
                                       "to build it.\n\n"
                                       "Click Cancel to exit."), QMessageBox::Cancel);
        return db;
    }
	
	return db;
}

bool DBAccess::createMetricsTable()
{
    QSqlQuery query;
    bool rc = query.exec("create table metrics (id integer primary key autoincrement, "
                         "filename varchar,"
                         "ride_date date,"
                         "ride_time double, "
                         "average_cad double,"
                         "workout_time double, "
                         "total_distance double,"
                         "x_power double,"
                         "average_speed double,"
                         "total_work double,"
                         "average_power double,"
                         "average_hr double,"
                         "relative_intensity double,"
                         "bike_score double)");
	
	if(!rc)
		qDebug() << query.lastError();
	
    return rc;
    
}

bool DBAccess::createSeasonsTable()
{
    QSqlQuery query;
    bool rc = query.exec("CREATE TABLE seasons(id integer primary key autoincrement,"
                         "start_date date,"
                         "end_date date,"
                         "name varchar)");

    if(!rc)
		qDebug() << query.lastError();

    return rc;
}


bool DBAccess::createDatabase()
{

    bool rc = false;

	rc = createMetricsTable();
    
    if(!rc)
        return rc;
    
    rc = createIndex();
    if(!rc)
        return rc;
    
    //Check to see if the table already exists..
	QStringList tableList = db.tables(QSql::Tables);
	if(!tableList.contains("seasons"))
        return createSeasonsTable();
	
    return true;
    
}

bool DBAccess::createIndex()
{
	QSqlQuery query;
	query.prepare("create INDEX IDX_FILENAME on metrics(filename)");
	bool rc = query.exec();			  
    if(!rc)
		qDebug() << query.lastError();
		
	return rc;
}

bool DBAccess::importRide(SummaryMetrics *summaryMetrics )
{
		
		QSqlQuery query;
	
		query.prepare("insert into metrics (filename, ride_date, ride_time, average_cad, workout_time, total_distance," 
					  "x_power, average_speed, total_work, average_power, average_hr,"
					  "relative_intensity, bike_score) values (?,?,?,?,?,?,?,?,?,?,?,?,?)");
	
	
	query.addBindValue(summaryMetrics->getFileName());
    query.addBindValue(summaryMetrics->getRideDate());
	query.addBindValue(summaryMetrics->getRideTime());
	query.addBindValue(summaryMetrics->getCadence());
	query.addBindValue(summaryMetrics->getWorkoutTime());
	query.addBindValue(summaryMetrics->getDistance());
	query.addBindValue(summaryMetrics->getXPower());
	query.addBindValue(summaryMetrics->getSpeed());
	query.addBindValue(summaryMetrics->getTotalWork());
	query.addBindValue(summaryMetrics->getWatts());
	query.addBindValue(summaryMetrics->getHeartRate());
	query.addBindValue(summaryMetrics->getRelativeIntensity());
    query.addBindValue(summaryMetrics->getBikeScore());
	
	 bool rc = query.exec();
	
	if(!rc)
	{
		qDebug() << query.lastError();
	}
	return rc;
}

QStringList DBAccess::getAllFileNames()
{
	QSqlQuery query("SELECT filename from metrics");
    QStringList fileList;
				  
	while(query.next())
	{
		QString filename = query.value(0).toString();
        fileList << filename;
	}
	
    return fileList;
}

QList<QDateTime> DBAccess::getAllDates()
{
    QSqlQuery query("SELECT ride_date from metrics");
    QList<QDateTime> dates;
    
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
    
    
    QSqlQuery query("SELECT filename, ride_date, ride_time, average_cad, workout_time, total_distance," 
                    "x_power, average_speed, total_work, average_power, average_hr,"
                    "relative_intensity, bike_scoreFROM metrics WHERE ride_date >=:start AND ride_date <=:end");
    query.bindValue(":start", start);
    query.bindValue(":end", end);
    
    while(query.next())
    {
 
        SummaryMetrics summaryMetrics;
        summaryMetrics.setFileName(query.value(0).toString());
        summaryMetrics.setRideDate(query.value(1).toDateTime());
        summaryMetrics.setRideTime(query.value(2).toDouble());
        summaryMetrics.setCadence(query.value(3).toDouble());
        summaryMetrics.setWorkoutTime(query.value(4).toDouble());
        summaryMetrics.setDistance(query.value(5).toDouble());
        summaryMetrics.setXPower(query.value(6).toDouble());
        summaryMetrics.setSpeed(query.value(7).toDouble());
        summaryMetrics.setTotalWork(query.value(8).toDouble());
        summaryMetrics.setWatts(query.value(9).toDouble());
        summaryMetrics.setHeartRate(query.value(10).toDouble());
        summaryMetrics.setRelativeIntensity(query.value(11).toDouble());
        summaryMetrics.setBikeScore(query.value(12).toDouble());        
            
        metrics << summaryMetrics;
    }
                    
    return metrics;
}

bool DBAccess::createSeason(Season season)
{
    QSqlQuery query;
	
    query.prepare("INSERT INTO season (start_date, end_date, name) values (?,?,?)");
	
	
	query.addBindValue(season.getStart());
    query.addBindValue(season.getEnd());
	query.addBindValue(season.getName());
	
    bool rc = query.exec();
	
	if(!rc)
		qDebug() << query.lastError();

	return rc;
    
}

QList<Season> DBAccess::getAllSeasons()
{
    QSqlQuery query("SELECT start_date, end_date, name from season");
    QList<Season> seasons;
    
    while(query.next())
    {
        Season season;
        season.setStart(query.value(0).toDateTime());
        season.setEnd(query.value(1).toDateTime());
        season.setName(query.value(2).toString());
        seasons << season;
        
    }
    return seasons;
    
}

bool DBAccess::dropMetricTable()
{
    
    
    QStringList tableList = db.tables(QSql::Tables);
	if(!tableList.contains("metrics"))
		return true;
    
    QSqlQuery query("DROP TABLE metrics");
    
    return query.exec();
}