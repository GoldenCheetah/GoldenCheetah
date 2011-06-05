
/*
 * Copyright (c) 2010 Damien Grauser (Damien.Grauser@pev-geneve.ch), Mark Liversedge (liversedge@gmail.com)
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
#include "HrZones.h"
#include <math.h>
#include <QApplication>

class HrZoneTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime)

    int level;
    double seconds;

    QList<int> lo;
    QList<int> hi;

public:

    HrZoneTime() : level(0), seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits("seconds");
        setImperialUnits("seconds");
        setPrecision(0);
        setConversion(1.0);
    }
    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *, int, const HrZones *hrZone, int hrZoneRange,
                 const QHash<QString,RideMetric*> &)
    {
        seconds = 0;
        // get zone ranges
        if (hrZone && hrZoneRange >= 0) {
            // iterate and compute
            foreach(const RideFilePoint *point, ride->dataPoints()) {
                if (hrZone->whichZone(hrZoneRange, point->hr) == level)
                    seconds += ride->recIntSecs();
            }
        }
        setValue(seconds);
    }

    bool canAggregate() const { return false; }
    void aggregateWith(const RideMetric &) {}
    RideMetric *clone() const { return new HrZoneTime(*this); }
};

class HrZoneTime1 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime1)

public:
    HrZoneTime1()
    {
        setLevel(1);
        setSymbol("time_in_zone_H1");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("H1 Time in Zone");
        }
        void initialize () {
#endif
        setName(tr("H1 Time in Zone"));
    }
    RideMetric *clone() const { return new HrZoneTime1(*this); }
};

class HrZoneTime2 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime2)

public:
    HrZoneTime2()
    {
        setLevel(2);
        setSymbol("time_in_zone_H2");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("H2 Time in Zone");
        }
        void initialize () {
#endif
        setName(tr("H2 Time in Zone"));
    }
    RideMetric *clone() const { return new HrZoneTime2(*this); }
};

class HrZoneTime3 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime3)

public:
    HrZoneTime3()
    {
        setLevel(3);
        setSymbol("time_in_zone_H3");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("H3 Time in Zone");
        }
        void initialize () {
#endif
        setName(tr("H3 Time in Zone"));
    }
    RideMetric *clone() const { return new HrZoneTime3(*this); }
};

class HrZoneTime4 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime4)

public:
    HrZoneTime4()
    {
        setLevel(4);
        setSymbol("time_in_zone_H4");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("H4 Time in Zone");
        }
        void initialize () {
#endif
        setName(tr("H4 Time in Zone"));
    }
    RideMetric *clone() const { return new HrZoneTime4(*this); }
};

class HrZoneTime5 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime5)

public:
    HrZoneTime5()
    {
        setLevel(5);
        setSymbol("time_in_zone_H5");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("H5 Time in Zone");
        }
        void initialize () {
#endif
        setName(tr("H5 Time in Zone"));
    }
    RideMetric *clone() const { return new HrZoneTime5(*this); }
};

class HrZoneTime6 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime6)

public:
    HrZoneTime6()
    {
        setLevel(6);
        setSymbol("time_in_zone_H6");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("H6 Time in Zone");
        }
        void initialize () {
#endif
        setName(tr("H6 Time in Zone"));
    }
    RideMetric *clone() const { return new HrZoneTime6(*this); }
};

class HrZoneTime7 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime7)

public:
    HrZoneTime7()
    {
        setLevel(7);
        setSymbol("time_in_zone_H7");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("H7 Time in Zone");
        }
        void initialize () {
#endif
        setName(tr("H7 Time in Zone"));
    }
    RideMetric *clone() const { return new HrZoneTime7(*this); }
};

class HrZoneTime8 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime8)

public:
    HrZoneTime8()
    {
        setLevel(8);
        setSymbol("time_in_zone_H8");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("H8 Time in Zone");
        }
        void initialize () {
#endif
        setName(tr("H8 Time in Zone"));
    }
    RideMetric *clone() const { return new HrZoneTime8(*this); }
};

static bool addAllHrZones() {
    RideMetricFactory::instance().addMetric(HrZoneTime1());
    RideMetricFactory::instance().addMetric(HrZoneTime2());
    RideMetricFactory::instance().addMetric(HrZoneTime3());
    RideMetricFactory::instance().addMetric(HrZoneTime4());
    RideMetricFactory::instance().addMetric(HrZoneTime5());
    RideMetricFactory::instance().addMetric(HrZoneTime6());
    RideMetricFactory::instance().addMetric(HrZoneTime7());
    RideMetricFactory::instance().addMetric(HrZoneTime8());
    return true;
}

static bool allHrZonesAdded = addAllHrZones();
