
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
#include <QApplication>

class ZoneTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime)

    int level;
    double seconds;

    QList<int> lo;
    QList<int> hi;

    public:

    ZoneTime() : level(0), seconds(0.0)
    {
        setType(RideMetric::Total);
        setPrecision(0);
        setConversion(1.0);
        setMetricUnits("seconds");
        setImperialUnits("seconds");
    }

    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *zone, int zoneRange, const HrZones *, int,
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
    Q_DECLARE_TR_FUNCTIONS(ZoneTime1)

    public:
        ZoneTime1()
        {
            setLevel(1);
            setSymbol("time_in_zone_L1");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L1 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L1 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime1(*this); }
};

class ZoneTime2 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime2)

    public:
        ZoneTime2()
        {
            setLevel(2);
            setSymbol("time_in_zone_L2");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L2 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L2 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime2(*this); }
};

class ZoneTime3 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime3)

    public:
        ZoneTime3()
        {
            setLevel(3);
            setSymbol("time_in_zone_L3");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L3 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L3 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime3(*this); }
};

class ZoneTime4 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime4)

    public:
        ZoneTime4()
        {
            setLevel(4);
            setSymbol("time_in_zone_L4");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L4 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L4 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime4(*this); }
};

class ZoneTime5 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime5)

    public:
        ZoneTime5()
        {
            setLevel(5);
            setSymbol("time_in_zone_L5");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L5 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L5 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime5(*this); }
};

class ZoneTime6 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime6)

    public:
        ZoneTime6()
        {
            setLevel(6);
            setSymbol("time_in_zone_L6");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L6 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L6 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime6(*this); }
};

class ZoneTime7 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime7)

    public:
        ZoneTime7()
        {
            setLevel(7);
            setSymbol("time_in_zone_L7");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L7 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L7 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime7(*this); }
};

class ZoneTime8 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime8)

    public:
        ZoneTime8()
        {
            setLevel(8);
            setSymbol("time_in_zone_L8");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L8 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L8 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime8(*this); }
};

class ZoneTime9 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime9)

    public:
        ZoneTime9()
        {
            setLevel(9);
            setSymbol("time_in_zone_L9");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L9 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L9 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime9(*this); }
};

class ZoneTime10 : public ZoneTime {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime10)

    public:
        ZoneTime10()
        {
            setLevel(10);
            setSymbol("time_in_zone_L10");
#ifdef ENABLE_METRICS_TRANSLATION
            setInternalName("L10 Time in Zone");
        }
        void initialize () {
#endif
            setName(tr("L10 Time in Zone"));
        }
        RideMetric *clone() const { return new ZoneTime10(*this); }
};



static bool addAllZones() {
    RideMetricFactory::instance().addMetric(ZoneTime1());
    RideMetricFactory::instance().addMetric(ZoneTime2());
    RideMetricFactory::instance().addMetric(ZoneTime3());
    RideMetricFactory::instance().addMetric(ZoneTime4());
    RideMetricFactory::instance().addMetric(ZoneTime5());
    RideMetricFactory::instance().addMetric(ZoneTime6());
    RideMetricFactory::instance().addMetric(ZoneTime7());
    RideMetricFactory::instance().addMetric(ZoneTime8());
    RideMetricFactory::instance().addMetric(ZoneTime9());
    RideMetricFactory::instance().addMetric(ZoneTime10());
    return true;
}

static bool allZonesAdded = addAllZones();
