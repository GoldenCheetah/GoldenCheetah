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


#include <QMap>
#include "RideFile.h"
#include <QDir>
#include "Zones.h"
#include "RideMetric.h"
#include "DBAccess.h"


class MetricAggregator
{
	public:
		MetricAggregator();
		void aggregateRides(QDir home, Zones *zones);
	    typedef QHash<QString,RideMetric*> MetricMap;
	bool importRide(QDir path, Zones *zones, RideFile *ride, QString fileName, DBAccess *dbaccess);
	MetricMap metrics;
    void MetricAggregator::scanForMissing(QDir home, Zones *zones);
    void resetMetricTable(QDir home);



};



#endif /* METRICAGGREGATOR_H_ */
