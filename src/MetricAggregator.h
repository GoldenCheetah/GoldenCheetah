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


#ifndef METRICAGGREGATOR_H_
#define METRICAGGREGATOR_H_
#include "GoldenCheetah.h"

#include <QMap>
#include "RideFile.h"
#include <QDir>
#include <QProgressDialog>
#include "Zones.h"
#include "HrZones.h"
#include "RideMetric.h"
#include "SummaryMetrics.h"
#include "Context.h"
#include "DBAccess.h"
#include "Colors.h"
#include "PDModel.h"

class MetricAggregator : public QObject
{
    Q_OBJECT
    G_OBJECT


	public:
        MetricAggregator(Context *);
		~MetricAggregator();


        void refreshMetrics();
        void refreshMetrics(QDateTime forceAfterThisDate);
        void refreshCPModelMetrics(bool bg=false); // metrics derived from model

        void getFirstLast(QDate &, QDate &);
        DBAccess *db() { return dbaccess; }
        SummaryMetrics getAllMetricsFor(QString filename); // for a single ride
        QList<SummaryMetrics> getAllMetricsFor(QDateTime start, QDateTime end);
        QList<SummaryMetrics> getAllMetricsFor(DateRange);
        QList<SummaryMetrics> getAllMeasuresFor(QDateTime start, QDateTime end);
        QList<SummaryMetrics> getAllMeasuresFor(DateRange);
        SummaryMetrics getRideMetrics(QString filename);
        void writeAsCSV(QString filename); // export all...
        QStringList allActivityFilenames();

    signals:
        void dataChanged(); // when metricDB table changed
        void modelProgress(int, int); // let others know when we're refreshing the model estimates

    public slots:
        void update();
        void addRide(RideItem*);
        void importMeasure(SummaryMetrics *sm);

    private:
        Context *context;
        DBAccess *dbaccess;
        bool first;
        bool refreshing;

	    typedef QHash<QString,RideMetric*> MetricMap;
	    bool importRide(QDir path, RideFile *ride, QString fileName, unsigned long, bool modify);
        bool importInterval(IntervalItem *interval, QString type, QString group, unsigned long fingerprint, bool modify);
#ifdef GC_HAVE_INTERVALS
        void refreshBestIntervals();
#endif

	    MetricMap metrics;
        ColorEngine *colorEngine;
};

#endif /* METRICAGGREGATOR_H_ */
