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

#ifndef _GC_DBAccess_h
#define _GC_DBAccess_h 1



#import <QDir>
#import <QHash>
#import <QTSql>
#import "SummaryMetrics.h"
#import "Season.h"

class RideFile;
class Zones;
class RideMetric;
class DBAccess
{
	
	public:
	DBAccess(QDir home);
	typedef QHash<QString,RideMetric*> MetricMap;
	void importAllRides(QDir path, Zones *zones);
	bool importRide(SummaryMetrics *summaryMetrics);
	QSqlDatabase initDatabase(QDir home);
	bool createDatabase();
	QStringList getAllFileNames();
    void closeConnection();
	QList<QDateTime> getAllDates();
    QList<SummaryMetrics> getAllMetricsFor(QDateTime start, QDateTime end);
    bool createSeasonsTable();
    bool createMetricsTable();
    bool createSeason(Season season);
    QList<Season> getAllSeasons();
    //bool deleteSeason(Season season);


	
	private:
	QSqlDatabase db;
	bool createIndex();

};
#endif