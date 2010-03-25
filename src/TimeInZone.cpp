
/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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
#include "BestIntervalDialog.h"
#include "Zones.h"
#include <math.h>

#define tr(s) QObject::tr(s)

class ZoneTime : public RideMetric {
    int level;
    double seconds;

    QList<int> lo;
    QList<int> hi;

    public:

    ZoneTime() : level(0), seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits("seconds");
        setImperialUnits("seconds");
        setPrecision(0);
        setConversion(1.0);
    }
    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *zone, int zoneRange,
                 const QHash<QString,RideMetric*> &)
    {
        seconds = 0;
        // get zone ranges
        if (zone && zoneRange >= 0) {
            // iterate and compute
            foreach(const RideFilePoint *point, ride->dataPoints()) {
                if (zone->whichZone(zoneRange, point->watts) == level)
                    seconds += ride->recIntSecs();
            }
        }
        setValue(seconds);
    }

    bool canAggregate() const { return false; }
    void aggregateWith(const RideMetric &) {}
    RideMetric *clone() const { return new ZoneTime(*this); }
};

class ZoneTime1 : public ZoneTime {

    public:
        ZoneTime1()
        {
            setLevel(1);
            setSymbol("time_in_zone_L1");
            setName(tr("L1 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime1(*this); }
};

class ZoneTime2 : public ZoneTime {

    public:
        ZoneTime2()
        {
            setLevel(2);
            setSymbol("time_in_zone_L2");
            setName(tr("L2 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime2(*this); }
};

class ZoneTime3 : public ZoneTime {

    public:
        ZoneTime3()
        {
            setLevel(3);
            setSymbol("time_in_zone_L3");
            setName(tr("L3 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime3(*this); }
};

class ZoneTime4 : public ZoneTime {

    public:
        ZoneTime4()
        {
            setLevel(4);
            setSymbol("time_in_zone_L4");
            setName(tr("L4 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime4(*this); }
};

class ZoneTime5 : public ZoneTime {

    public:
        ZoneTime5()
        {
            setLevel(5);
            setSymbol("time_in_zone_L5");
            setName(tr("L5 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime5(*this); }
};

class ZoneTime6 : public ZoneTime {

    public:
        ZoneTime6()
        {
            setLevel(6);
            setSymbol("time_in_zone_L6");
            setName(tr("L6 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime6(*this); }
};

class ZoneTime7 : public ZoneTime {

    public:
        ZoneTime7()
        {
            setLevel(7);
            setSymbol("time_in_zone_L7");
            setName(tr("L7 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime7(*this); }
};



static bool addAllZones() {
    RideMetricFactory::instance().addMetric(ZoneTime1());
    RideMetricFactory::instance().addMetric(ZoneTime2());
    RideMetricFactory::instance().addMetric(ZoneTime3());
    RideMetricFactory::instance().addMetric(ZoneTime4());
    RideMetricFactory::instance().addMetric(ZoneTime5());
    RideMetricFactory::instance().addMetric(ZoneTime6());
    RideMetricFactory::instance().addMetric(ZoneTime7());
    return true;
}

static bool allZonesAdded = addAllZones();
