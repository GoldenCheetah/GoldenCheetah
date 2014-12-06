/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#include "RideMetric.h"
#include "TimeUtils.h"
#include "Zones.h"
#include "HrZones.h"

RideMetricFactory *RideMetricFactory::_instance;
QVector<QString> RideMetricFactory::noDeps;

QHash<QString,RideMetricPtr>
RideMetric::computeMetrics(const Context *context, const RideFile *ride, const Zones *zones, const HrZones *hrZones,
                           const QStringList &metrics)
{
    int zoneRange = zones->whichRange(ride->startTime().date());
    int hrZoneRange = hrZones->whichRange(ride->startTime().date());

    const RideMetricFactory &factory = RideMetricFactory::instance();
    QStringList todo = metrics;
    QHash<QString,RideMetric*> done;
    while (!todo.isEmpty()) {
        QString symbol = todo.takeFirst();
        if (!factory.haveMetric(symbol)) continue;
        const QVector<QString> &deps = factory.dependencies(symbol);
        bool ready = true;
        foreach (QString dep, deps) {
            if (!done.contains(dep)) {
                ready = false;
                if (!todo.contains(dep))
                    todo.append(dep);
            }
        }
        if (ready) {
            RideMetric *m = factory.newMetric(symbol);
            //if (!ride->dataPoints().isEmpty())
                m->compute(ride, zones, zoneRange, hrZones, hrZoneRange, done, context);
            if (ride->metricOverrides.contains(symbol))
                m->override(ride->metricOverrides.value(symbol));
            done.insert(symbol, m);
        }
        else {
            if (!todo.contains(symbol))
                todo.append(symbol);
        }
    }
    QHash<QString,RideMetricPtr> result;
    foreach (QString symbol, metrics) {
        result.insert(symbol, QSharedPointer<RideMetric>(done.value(symbol)));
        done.remove(symbol);
    }
    foreach (QString symbol, done.keys())
        delete done.value(symbol);
    return result;
}

QString 
RideMetric::toString(bool useMetricUnits)
{
    if (isTime()) return time_to_string(value(useMetricUnits));
    return QString("%1").arg(value(useMetricUnits), 0, 'f', precision());
}
