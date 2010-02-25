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



#include <QDir>
#include <QHash>
#include <QtSql>
#include "SummaryMetrics.h"
#include "Season.h"

class RideFile;
class Zones;
class RideMetric;
class DBAccess
{
	
	public:

    // get connection name
    QSqlDatabase connection() { return dbconn; }

    // create and drop connections
	DBAccess(QDir home);
    void closeConnection();

    // Create/Delete Records
	bool importRide(SummaryMetrics *summaryMetrics, bool);
    bool deleteRide(QString);

    // Query Records
	QList<QDateTime> getAllDates();
    QList<SummaryMetrics> getAllMetricsFor(QDateTime start, QDateTime end);
    QList<Season> getAllSeasons();

	private:
	QSqlDatabase db, dbconn;
	typedef QHash<QString,RideMetric*> MetricMap;

	bool createDatabase();
    bool createMetricsTable();
    bool dropMetricTable();
	bool createIndex();
    void checkDBVersion();
	void initDatabase(QDir home);

    QString sessionid;
    static int session;

};
#endif
