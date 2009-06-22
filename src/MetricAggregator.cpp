/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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



#include "MetricAggregator.h"
#include "DBAccess.h"
#include "RideFile.h"
#include "Zones.h"
#include "Settings.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "TimeUtils.h"
#include <assert.h>
#include <math.h>
#include <QtXml/QtXml>

static char rideFileRegExp[] =
    "^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
    "_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.(raw|srm|csv|tcx)$";

MetricAggregator::MetricAggregator()
{

}

void MetricAggregator::aggregateRides(QDir home, Zones *zones)
{
	qDebug() << QDateTime::currentDateTime();
	DBAccess *dbaccess = new DBAccess(home);
    dbaccess->dropMetricTable();
    dbaccess->createDatabase();
	QRegExp rx(rideFileRegExp);
	QStringList errors;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(home));
    while (i.hasNext()) {
        QString name = i.next();
		QFile file(home.absolutePath() + "/" + name);
		RideFile *ride = RideFileFactory::instance().openRideFile(file, errors);
		importRide(home, zones, ride, name, dbaccess);
	}
    dbaccess->closeConnection();
    delete dbaccess;
	qDebug() << QDateTime::currentDateTime();
	
}

bool MetricAggregator::importRide(QDir path, Zones *zones, RideFile *ride, QString fileName, DBAccess *dbaccess)
{
		
	SummaryMetrics *summaryMetric = new SummaryMetrics();
	
	
	QFile file(path.absolutePath() + "/" + fileName);
	int zone_range = -1;
	
	QRegExp rx(rideFileRegExp);
    if (!rx.exactMatch(fileName)) {
        fprintf(stderr, "bad name: %s\n", fileName.toAscii().constData());
        assert(false);
		return false;
    }
	summaryMetric->setFileName(fileName);
    assert(rx.numCaptures() == 7);
    QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt()); 
    QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt()); 
    QDateTime dateTime(date, time);
	
    summaryMetric->setRideDate(dateTime);
	
	if (zones)
		zone_range = zones->whichRange(dateTime.date());
	
	QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
	QVariant unit = settings.value(GC_UNIT);
	
	const RideMetricFactory &factory = RideMetricFactory::instance();
	QSet<QString> todo;
	
	for (int i = 0; i < factory.metricCount(); ++i)
		todo.insert(factory.metricName(i));
	
	
	while (!todo.empty()) {
	    QMutableSetIterator<QString> i(todo);
	later:
	    while (i.hasNext()) {
		const QString &name = i.next();
		const QVector<QString> &deps = factory.dependencies(name);
		for (int j = 0; j < deps.size(); ++j)
		    if (!metrics.contains(deps[j]))
			goto later;
		RideMetric *metric = factory.newMetric(name);
		metric->compute(ride, zones, zone_range, metrics);
		metrics.insert(name, metric);
		i.remove();
		double value = metric->value(true);
		if(name ==  "workout_time")
		    summaryMetric->setWorkoutTime(value);
		else if(name == "average_cad")
		    summaryMetric->setCadence(value);
		else if(name == "total_distance")
		    summaryMetric->setDistance(value);
		else if(name == "skiba_xpower")
		    summaryMetric->setXPower(value);
		else if(name == "average_speed")
		    summaryMetric->setSpeed(value);
		else if(name == "total_work")
		    summaryMetric->setTotalWork(value);
		else if(name == "average_power")
		    summaryMetric->setWatts(value);
		else if(name == "time_riding")
		    summaryMetric->setRideTime(value);
		else if(name == "average_hr")
		    summaryMetric->setHeartRate(value);
		else if(name == "skiba_relative_intensity")
		    summaryMetric->setRelativeIntensity(value);
		else if(name == "skiba_bike_score")
		    summaryMetric->setBikeScore(value);
			
	    }
	}

	dbaccess->importRide(summaryMetric);
	delete summaryMetric;

	return true;
	
}

void MetricAggregator::scanForMissing(QDir home, Zones *zones)
{
    QStringList errors;
    DBAccess *dbaccess = new DBAccess(home);
    QStringList filenames = dbaccess->getAllFileNames();
    QRegExp rx(rideFileRegExp);
    QStringListIterator i(RideFileFactory::instance().listRideFiles(home));
    while (i.hasNext()) {
        QString name = i.next();
        if(!filenames.contains(name))
        {
           qDebug() << "Found missing file: " << name;
           QFile file(home.absolutePath() + "/" + name);
           RideFile *ride = RideFileFactory::instance().openRideFile(file, errors);
           importRide(home, zones, ride, name, dbaccess);
           
        }
           
	}
    dbaccess->closeConnection();
    delete dbaccess;
    
}

void MetricAggregator::resetMetricTable(QDir home)
{
    DBAccess dbAccess(home);
    dbAccess.dropMetricTable();
}

