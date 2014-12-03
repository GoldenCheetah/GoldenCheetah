
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
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
    }
    bool isTime() const { return true; }
    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *zone, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *)
    {
        seconds = 0;
        // get zone ranges
        if (zone && zoneRange >= 0 && ride->areDataPresent()->watts) {
            // iterate and compute
            foreach(const RideFilePoint *point, ride->dataPoints()) {
                if (zone->whichZone(zoneRange, point->watts) == level)
                    seconds += ride->recIntSecs();
            }
        }
        setValue(seconds);
    }

    bool canAggregate() { return false; }
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
            setInternalName("L1 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L1 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L2 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L2 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L3 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L3 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L4 Time in Zone");
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        void initialize ()
        {
            setName(tr("L4 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L5 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L5 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L6 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L6 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L7 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L7 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L8 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L8 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L9 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L9 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
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
            setInternalName("L10 Time in Zone");
        }
        void initialize ()
        {
            setName(tr("L10 Time in Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new ZoneTime10(*this); }
};

// Now for Time In Zone as a Percentage of Ride Time
class ZonePTime1 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime1)

    public:

        ZonePTime1()
        {
            setSymbol("percent_in_zone_L1");
            setInternalName("L1 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L1 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L1"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L1")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime1(*this); }
};

class ZonePTime2 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime2)

    public:

        ZonePTime2()
        {
            setSymbol("percent_in_zone_L2");
            setInternalName("L2 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L2 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L2"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L2")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime2(*this); }
};

class ZonePTime3 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime3)

    public:

        ZonePTime3()
        {
            setSymbol("percent_in_zone_L3");
            setInternalName("L3 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L3 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L3"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L3")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime3(*this); }
};

class ZonePTime4 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime4)

    public:

        ZonePTime4()
        {
            setSymbol("percent_in_zone_L4");
            setInternalName("L4 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L4 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L4"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L4")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime4(*this); }
};

class ZonePTime5 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime5)

    public:

        ZonePTime5()
        {
            setSymbol("percent_in_zone_L5");
            setInternalName("L5 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L5 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L5"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L5")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime5(*this); }
};

class ZonePTime6 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime6)

    public:

        ZonePTime6()
        {
            setSymbol("percent_in_zone_L6");
            setInternalName("L6 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L6 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L6"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L6")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime6(*this); }
};

class ZonePTime7 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime7)

    public:

        ZonePTime7()
        {
            setSymbol("percent_in_zone_L7");
            setInternalName("L7 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L7 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L7"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L7")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime7(*this); }
};

class ZonePTime8 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime8)

    public:

        ZonePTime8()
        {
            setSymbol("percent_in_zone_L8");
            setInternalName("L8 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L8 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L8"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L8")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime8(*this); }
};

class ZonePTime9 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime9)

    public:

        ZonePTime9()
        {
            setSymbol("percent_in_zone_L9");
            setInternalName("L9 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L9 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L9"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L9")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime9(*this); }
};

class ZonePTime10 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(ZonePTime10)

    public:

        ZonePTime10()
        {
            setSymbol("percent_in_zone_L10");
            setInternalName("L10 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("L10 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_L10"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_L10")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new ZonePTime10(*this); }
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
    QVector<QString> deps;
    deps.append("time_in_zone_L1");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime1(), &deps);
    deps.clear();
    deps.append("time_in_zone_L2");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime2(), &deps);
    deps.clear();
    deps.append("time_in_zone_L3");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime3(), &deps);
    deps.clear();
    deps.append("time_in_zone_L4");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime4(), &deps);
    deps.clear();
    deps.append("time_in_zone_L5");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime5(), &deps);
    deps.clear();
    deps.append("time_in_zone_L6");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime6(), &deps);
    deps.clear();
    deps.append("time_in_zone_L7");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime7(), &deps);
    deps.clear();
    deps.append("time_in_zone_L8");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime8(), &deps);
    deps.clear();
    deps.append("time_in_zone_L9");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime9(), &deps);
    deps.clear();
    deps.append("time_in_zone_L10");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(ZonePTime10(), &deps);
    return true;
}

static bool allZonesAdded = addAllZones();
