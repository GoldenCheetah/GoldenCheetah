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

        DBAccess *db() { return dbaccess; }

    signals:
        void dataChanged(); // when metricDB table changed

    public slots:
        void update();
        void addRide(RideItem*);

    private:
        Context *context;
        DBAccess *dbaccess;
        bool first;
        bool refreshing;

	    typedef QHash<QString,RideMetric*> MetricMap;
	    bool importRide(QDir path, RideFile *ride, QString fileName, unsigned long, bool modify);
#ifdef GC_HAVE_INTERVALS
        bool importInterval(IntervalItem *interval, QString type, QString group, unsigned long fingerprint, bool modify);
        void refreshBestIntervals();
#endif

	    MetricMap metrics;
        ColorEngine *colorEngine;
};

#endif /* METRICAGGREGATOR_H_ */
