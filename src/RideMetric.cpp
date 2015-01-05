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

// DB Schema Version - YOU MUST UPDATE THIS IF THE SCHEMA VERSION CHANGES!!!
// Schema version will change if a) the default metadata.xml is updated
//                            or b) new metrics are added / old changed
//                            or c) the metricDB tables structures change

// Revision History
// Rev Date         Who                What Changed
//-----------------------------------------------------------------------
//
// ******* Prior to version 29 no revision history was maintained *******
//
// 29  5th Sep 2011 Mark Liversedge    Added color to the ride fields
// 30  8th Sep 2011 Mark Liversedge    Metadata 'data' field for data present string
// 31  22  Nov 2011 Mark Liversedge    Added Average WPK metric
// 32  9th Dec 2011 Damien Grauser     Temperature data flag (metadata field 'Data')
// 33  17  Dec 2011 Damien Grauser     Added ResponseIndex and EfficiencyFactor
// 34  15  Jan 2012 Mark Liversedge    Added Average and Max Temperature and Metric->conversionSum()
// 35  13  Feb 2012 Mark Liversedge    Max/Avg Cadence adjusted conversion
// 36  18  Feb 2012 Mark Liversedge    Added Pace (min/mile) and 250m, 500m Pace metrics
// 37  06  Apr 2012 Rainer Clasen      Added non-zero average Power (watts)
// 38  8th Jul 2012 Mark Liversedge    Computes metrics for manual files now
// 39  18  Aug 2012 Mark Liversedge    New metric LRBalance
// 40  20  Oct 2012 Mark Liversedge    Lucene search/filter and checkbox metadata field
// 41  27  Oct 2012 Mark Liversedge    Lucene switched to StandardAnalyzer and search all texts by default
// 42  03  Dec 2012 Mark Liversedge    W/KG ridefilecache changes - force a rebuild.
// 43  24  Jan 2012 Mark Liversedge    TRIMP update
// 44  19  Apr 2013 Mark Liversedge    Aerobic Decoupling precision reduced to 1pt
// 45  09  May 2013 Mark Liversedge    Added 2,3,8 and 90m peak power for fatigue profiling
// 46  13  May 2013 Mark Liversedge    Handle absence of speed in metric calculations
// 47  17  May 2013 Mark Liversedge    Reimplementation of w/kg and ride->getWeight()
// 48  22  May 2013 Mark Liversedge    Removing local measures.xml, till v3.1
// 49  29  Oct 2013 Mark Liversedge    Added percentage time in zone
// 50  29  Oct 2013 Mark Liversedge    Added percentage time in heartrate zone
// 51  05  Nov 2013 Mark Liversedge    Added average aPower
// 52  05  Nov 2013 Mark Liversedge    Added EOA - Effect of Altitude
// 53  18  Dec 2013 Mark Liversedge    Added Fatigue Index (for power)
// 54  07  Jan 2014 Mark Liversedge    Revised Estimated VO2MAX metric formula
// 55  20  Jan 2014 Mark Liversedge    Added back Minimum W'bal metric and MaxMatch
// 56  20  Jan 2014 Mark Liversedge    Added W' TAU to be able to track it
// 57  20  Jan 2014 Mark Liversedge    Added W' Expenditure for total energy spent above CP
// 58  23  Jan 2014 Mark Liversedge    W' work rename and calculate without reference to WPrime class (speed)
// 59  24  Jan 2014 Mark Liversedge    Added Maximum W' exp which is same as W'bal bur expressed as used not left
// 60  05  Feb 2014 Mark Liversedge    Added Critical Power as a metric -- retrieves from settings for now
// 61  15  Feb 2014 Mark Liversedge    Fixed W' Work (for recintsecs not 1s!).
// 62  06  Mar 2014 Mark Liversedge    Fixed Fatigue Index to find peak then watch for decay, primarily useful in sprint intervals
// 63  06  Mar 2014 Mark Liversedge    Added Pacing Index AP as %age of Max Power
// 64  17  Mar 2014 Mark Liversedge    Added W' and CP work to PMC metrics
// 65  17  Mar 2014 Mark Liversedge    Added Aerobic TISS prototype
// 66  18  Mar 2014 Mark Liversedge    Updated aPower calculation
// 67  22  Mar 2014 Mark Liversedge    Added Anaerobic TISS prototype
// 68  22  Mar 2014 Mark Liversedge    Added dTISS prototype
// 69  23  Mar 2014 Mark Liversedge    Updated Gompertz constansts for An-TISS sigmoid
// 70  27  Mar 2014 Mark Liversedge    Add file CRC to refresh only if contents change (not just timestamps)
// 71  14  Apr 2014 Mark Liversedge    Added average lef/right vector metrics (Pedal Smoothness / Torque Effectiveness)
// 72  24  Apr 2014 Mark Liversedge    Andy Froncioni's faster algorithm for W' bal
// 73  11  May 2014 Mark Liversedge    Default color of 1,1,1 now uses CPLOTMARKER for ride color, change version to force rebuild
// 74  20  May 2014 Mark Liversedge    Added Athlete Weight
// 75  25  May 2014 Mark Liversedge    W' work calculation changed to only include energy above CP
// 76  14  Jun 2014 Mark Liversedge    Add new 'present' field that uses Data tag data
// 77  18  Jun 2014 Mark Liversedge    Add TSS per hour metric
// 78  19  Jun 2014 Mark Liversedge    Do not include zeroes in average L/R pedal smoothness/torque effectiveness
// 79  20  Jun 2014 Mark Liversedge    Change the way average temperature is handled
// 80  13  Jul 2014 Mark Liversedge    W' work + Below CP work = Work
// 81  16  Aug 2014 Joern Rischmueller Added 'Elevation Loss'
// 82  23  Aug 2014 Mark Liversedge    Added W'bal Matches
// 83  05  Sep 2014 Joern Rischmueller Added 'Time Carrying' and 'Elevation Gain Carrying'
// 84  08  Sep 2014 Mark Liversedge    Added HrPw Ratio
// 85  09  Sep 2014 Mark Liversedge    Added HrNp Ratio
// 86  26  Sep 2014 Mark Liversedge    Added isRun first class var
// 87  11  Oct 2014 Mark Liversedge    W'bal inegrator fixed up by Dave Waterworth
// 88  14  Oct 2014 Mark Liversedge    Pace Zone Metrics
// 89  07  Nov 2014 Ale Martinez       GOVSS
// 90  08  Nov 2014 Mark Liversedge    Update data flags for Moxy and Garmin Running Dynamics
// 91  16  Nov 2014 Damien Grauser     Do not include values if data not present in TimeInZone and HRTimeInZone
// 92  21  Nov 2014 Mark Liversedge    Added Watts:RPE ratio
// 93  26  Nov 2014 Mark Liversedge    Added Min, Max, Avg SmO2
// 94  02  Dec 2014 Ale Martinez       Added xPace
// 95  08  Dec 2014 Ale Martinez       Deprecated Measures table

int DBSchemaVersion = 95;

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
RideMetric::toString(bool useMetricUnits) const
{
    if (isTime()) return time_to_string(value(useMetricUnits));
    return QString("%1").arg(value(useMetricUnits), 0, 'f', precision(useMetricUnits));
}
