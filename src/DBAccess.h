/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#ifndef _GC_DBAccess_h
#define _GC_DBAccess_h 1

#include "GoldenCheetah.h"


#include <QDir>
#include <QHash>
#include <QtSql>
#include <QMessageBox>

#include "SummaryMetrics.h"
#include "Context.h"
#include "Season.h"
#include "RideFile.h"
#include "SpecialFields.h"
#include "RideMetadata.h"

extern int DBSchemaVersion;

class RideFile;
class Zones;
class RideMetric;
class DBAccess
{

	public:

        // get connection name
        QSqlDatabase connection() { return db->database(sessionid); }

        // check the db structure is up to date
        void checkDBVersion();

        // get schema version
        int getDBVersion();
        QList<FieldDefinition> &getMetadataFields() { return mfieldDefinitions; }

        // create and drop connections
	    DBAccess(Context *context);
        ~DBAccess();

        // Create/Delete Metrics
	    bool importRide(SummaryMetrics *summaryMetrics, RideFile *ride, QColor color, unsigned long, bool);
        bool deleteRide(QString);

        // Create/Delete Measures
        bool importMeasure(SummaryMetrics *summaryMetrics);

        // Query Records
        QList<SummaryMetrics> getAllMetricsFor(QDateTime start, QDateTime end);
        QList<SummaryMetrics> getAllMetricsFor(DateRange dr) {
            return getAllMetricsFor(QDateTime(dr.from,QTime(0,0,0)), QDateTime(dr.to, QTime(23,59,59)));
        }
        QList<QString> getDistinctValues(FieldDefinition field);

        bool getRide(QString filename, SummaryMetrics &metrics, QColor&color);
        QList<SummaryMetrics> getAllMeasuresFor(QDateTime start, QDateTime end);
        QList<SummaryMetrics> getAllMeasuresFor(DateRange dr) { 
            return getAllMeasuresFor(QDateTime(dr.from,QTime(0,0,0)), QDateTime(dr.to, QTime(23,59,59)));
        }

        SummaryMetrics getRideMetrics(QString filename); // for a filename

	    QList<QDateTime> getAllDates();
        QList<Season> getAllSeasons();

	private:

        Context *context;
        QSqlDatabase *db;
        QString sessionid;

        SpecialFields msp;
        QList<FieldDefinition> mfieldDefinitions;
        QList<KeywordDefinition> mkeywordDefinitions; //NOTE: not used in measures.xml
        QString mcolorfield;

	    typedef QHash<QString,RideMetric*> MetricMap;

	    bool createDatabase();
        bool createMetricsTable();
        bool dropMetricTable();
        bool createMeasuresTable();
        bool dropMeasuresTable();
	    void initDatabase(QDir home);
};
#endif
