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
#include "RideItem.h"
#include "IntervalItem.h"
#include "Specification.h"
#include "UserMetricSettings.h"
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
// 77  18  Jun 2014 Mark Liversedge    Add BikeStress per hour metric
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
// 95  08  Dec 2014 Mark Liversedge    Deprecated Measures table
// 96  06  Jan 2015 Damien Grauser     Garmin Cycling Dynamics
// 97  07  Jan 2015 Mark Liversedge    Added isSwim first class variable
// 98  10  Jan 2015 Ale Martinez       Added Triscore and SwimScore metrics
// 99  14  Jan 2015 Damien Grauser     Added TotalCalories
// 100 05  Feb 2015 Ale Martinez       Use duration not time moving when its 0 (rpe metrics)
// 101 05  Feb 2015 Mark Liversedge    aPower versions of Coggan metrics aIsoPower et al
// 102 05  Feb 2015 Mark Liversedge    aPower versions of Skiba metrics aBikeScore et al
// 103 23  Feb 2015 Mark Liversedge    Added total heartbeats
// 104 24  Apr 2015 Mark Liversedge    Added Workbeat stress (Work * Heartbeats) / 10000
// 105 09  May 2015 Ale Martinez       Added PeakPace and PeakPaceSwim metrics
// 106 09  May 2015 Mark Liversedge    Added MMP Percentage - %age of power for duration vs CP model
// 107 29  May 2015 Mark Liversedge    Added AP as percent of maximum
// 108 29  May 2015 Mark Liversedge    Added W' Power - average power contribution from W'
// 109 29  May 2015 Mark Liversedge    Added Sustained Time In Zone metrics
// 110 12  Jun 2015 Mark Liversedge    Added climb rating
// 111 14  Jun 2015 Mark Liversedge    Added W'bal time in zone metrics
// 112 18  Jun 2015 Mark Liversedge    Added Core Temp average and max
// 113 27  Jun 2015 Mark Liversedge    Added Average/Min/Max tHb
// 114 17  Jul 2015 Ale Martinez       Added Distance Swim
// 115 18  Jul 2015 Mark Liversedge    Added Withings Fat, Fat Percent, Lean Body Weight
// 116 21  Aug 2015 Ale Martinez       TRIMP Zonal Points fallback when Average HR has been entered manually
// 117 29  Aug 2015 Mark Liversedge    Min non-zero HR
// 118 16  Sep 2015 Damien Grauser     Use FTP for BikeStress and IF
// 119 20  Oct 2015 Ale Martinez       Added VDOT and TPace for Running
// 120 3   Nov 2015 Mark Liversedge    Added Above CP time in W'bal zones
// 121 3   Nov 2015 Mark Liversedge    Added Work in W'bal zones
// 122 7   Nov 2015 Mark Liversedge    Added HR Zones 9 and 10
// 123 19  Nov 2015 Mark Liversedge    Force recompute of BikeStress/IF after logic fix
// 124 03  Dec 2015 Mark Liversedge    Min Temp
// 125 08  Dec 2015 Ale Martinez       Support metrics in Calendar Text
// 126 08  Mar 2016 Mark Liversedge    Added count of To Exhaustions
// 127 25  Mar 2016 Mark Liversedge    Best R metric for Exhaustion Points
// 128 15  May 2016 Mark Liversedge    Add ActivityCRC so R scripts can use when caching
// 129 10  Jul 2016 Damien Grauser     Average Running Cadence
// 130 12  Jul 2016 Ale Martinez       Added Best Times for common distances
// 131 20  Jul 2016 Damien Grauser     Average Running Vertical Oscillation and Ground Contact Time
// 132 21  Jul 2016 Ale Martinez       Added SwimMetrics (Stroke Rate et al)
// 133 22  Jul 2016 Damien Grauser     Added Efficiency Index
// 134 22  Jul 2016 Damien Grauser     Add Stride length
// 135 10  Aug 2016 Ale Martinez       Added Average Swim Pace for the 4 Strokes
// 136 17  Oct 2016 Ale Martinez       Changed Best Times units to minutes
// 137 16  Feb 2017 Leif Warland       Added HrvMetrics
// 138 01  Mar 2017 Mark Liversedge	   Added elapsed_time metric for intervals
// 139 88  Mar 2017 Leif Warland       Added SDANN and SDNNIDX to HRV metrics
// 140 08  Apr 2017 Ale Martinez       Added Peak Pace Hr metrics
// 141 14  Apr 2017 Joern Rischmueller Added 'Athlete Bones', 'Athlete Muscles' Body Measures metric
// 142 25  Jul 2017 Ale Martinez       Added HRV metrics at rest (Measures)
// 143 07  Feb 2018 Walter Buerki      Daniels Points using GAP when no power
// 144 02  Apr 2018 Mark Liversedge    Force refresh for compatibility metrics
// 145 06  Apr 2018 Ale Martinez       Python Scripts in UserMetric honor RideItem and Specification
// 146 21  Apr 2018 Ale Martinez       TriScore Fallback to TRIMP Zonal Points
// 147 06  May 2018 Ale Martinez       Added PeakHr metrics and HrZone
// 148 27  Jul 2018 Ale Martinez       Changed Hrv Measures to retrun 0 when no record for the date
// 149 04  Jan 2019 Mark Liversedge    PowerIndex metric to score performance tests/intervals/rides vs typical athlete PD curve
// 150 28  Mar 2019 Ale Martinez       Additional Running Dynamics metrics
int DBSchemaVersion = 150;

RideMetricFactory *RideMetricFactory::_instance;
QVector<QString> RideMetricFactory::noDeps;
QList<QString> RideMetricFactory::compatibilitymetrics;

// user defined metrics are loaded by the ridecache on startup
// and then reloaded by ridecache if they change
QList<UserMetricSettings> _userMetrics;
quint16 UserMetricSchemaVersion = 0;

quint16
RideMetric::userMetricFingerprint(QList<UserMetricSettings> these)
{
    // run through loaded metrics and compute a fingerprint CRC
    QByteArray fingers;
    foreach(UserMetricSettings x, these)
        fingers += x.fingerprint.toLocal8Bit();

    return qChecksum(fingers.constData(), fingers.size());
}

QHash<QString,RideMetricPtr>
RideMetric::computeMetrics(RideItem *item, Specification spec, const QStringList &metrics)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();

    // generate worklist from metrics we know
    // bear in mind this can change as users add
    // and remove user metrics
    // builtin User metrics are computed after builtins
    // since they don't have explicit dependencies set, yet.
    QStringList builtin;
    QStringList user;
    foreach(QString metric, metrics)
        if (factory.haveMetric(metric)) {
            if (factory.rideMetric(metric)->isUser())
                user << metric;
            else
                builtin << metric;
        }

    // this is what we've completed as we go
    QHash<QString,RideMetric*> done;

    // resize the metric array in the interval if needed
    if (spec.interval() && spec.interval()->metrics().size() < factory.metricCount()) 
        spec.interval()->metrics().resize(factory.metricCount());

    // resize the metric array in the interval if needed
    if (!spec.interval() && item->metrics().size() < factory.metricCount())
        item->metrics().resize(factory.metricCount());

    // working through the todo list...
    while (!builtin.isEmpty() || !user.isEmpty()) {

        // next one to do, builtins first then user defined
        QString symbol = builtin.isEmpty() ? user.takeFirst() :
                                             builtin.takeFirst();

        // doesn't exist !
        if (!factory.haveMetric(symbol)) continue;

        // does this one have any dependencies?
        const QVector<QString> &deps = factory.dependencies(symbol);

        bool ready = true;

        // if the dependencies aren't done yet add to the end of the list
        foreach (QString dep, deps) {
            if (!done.contains(dep)) {
                ready = false;
                if (!builtin.contains(dep))
                    builtin.append(dep);
            }
        }

        // if all our depencies are computed we can do this one
        if (ready) {

            // we clone so we can remain thread safe
            // do not be tempted to change this (!)
            RideMetric *m = factory.newMetric(symbol);
            m->setValue(0.0);
            m->setCount(0);
            m->compute(item, spec, done);

            // override the computed value if set by user, but not for intervals
            if (!spec.interval() && item->ride() && item->ride()->metricOverrides.contains(symbol))
                m->override(item->ride()->metricOverrides.value(symbol));

            // all computed add to the return list
            done.insert(symbol, m);

            // put into value array too. user metrics will interrogate
            // this for symbol values, rather than the metric pointer
            // this is crucial, even though RideItem and IntervalItem both
            // update their values directly. But only need to bother if the
            // user has defined any local metrics.
            if (user.count()) {
                if (spec.interval()) spec.interval()->metrics()[m->index()] = m->value();
                else item->metrics()[m->index()] = m->value();
            }

        } else {

            // we need to wait for our dependencies so add
            // to the back of the list 
            if (!builtin.contains(symbol))
                builtin.append(symbol);
        }
    }

    // lets prepate the results using a shared pointer
    // which is deleted when reference count 0 and goes out of scope
    QHash<QString,RideMetricPtr> result;
    foreach (QString symbol, metrics) {
        if (factory.haveMetric(symbol)) {
            result.insert(symbol, QSharedPointer<RideMetric>(done.value(symbol)));
            done.remove(symbol);
        }
    }

    // delete the cloned metrics, no memory leak here :)
    foreach (QString symbol, done.keys())
        delete done.value(symbol);

    // and we're done
    return result;
}

double 
RideMetric::getForSymbol(QString symbol, const QHash<QString,RideMetric*> *p)
{
    if (p == NULL ) return RideFile::NIL;

    RideMetric *m=p->value(symbol, NULL);

    if (m == NULL) return RideFile::NIL;

    // ok, lets get the value
    return m->value();
}

QString 
RideMetric::toString(bool useMetricUnits) const
{
    if (isTime()) return time_to_string(value(useMetricUnits));
    return QString("%1").arg(value(useMetricUnits), 0, 'f', this->precision());
}

QString
RideMetric::toString(bool useMetricUnits, double v) const
{
    if (isTime()) return time_to_string(value(useMetricUnits));
    return QString("%1").arg(value(v, useMetricUnits), 0, 'f', this->precision());
}
