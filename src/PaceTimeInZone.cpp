
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

#include "Context.h"
#include "Athlete.h"
#include "BestIntervalDialog.h"
#include "PaceZones.h"

#include <math.h>
#include <QApplication>

class PaceZoneTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime)
    int level;
    double seconds;

    QList<int> lo;
    QList<int> hi;

    public:

    PaceZoneTime() : level(0), seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
    }
    bool isTime() const { return true; }
    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *, int ,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *context)
    {
        seconds = 0;

        // pace only makes sense for running
        if (ride->isRun()) {

            const PaceZones *zone = context->athlete->paceZones();
            int zoneRange = context->athlete->paceZones()->whichRange(ride->startTime().date());

            // get zone ranges
            if (zone && zoneRange >= 0) {
                // iterate and compute
                foreach(const RideFilePoint *point, ride->dataPoints()) {
                    if (zone->whichZone(zoneRange, point->kph) == level)
                        seconds += ride->recIntSecs();
                }
            }
        }

        setValue(seconds);
    }

    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    RideMetric *clone() const { return new PaceZoneTime(*this); }
};

class PaceZoneTime1 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime1)

    public:
        PaceZoneTime1()
        {
            setLevel(1);
            setSymbol("time_in_zone_P1");
            setInternalName("P1 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P1 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime1(*this); }
};

class PaceZoneTime2 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime2)

    public:
        PaceZoneTime2()
        {
            setLevel(2);
            setSymbol("time_in_zone_P2");
            setInternalName("P2 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P2 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime2(*this); }
};

class PaceZoneTime3 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime3)

    public:
        PaceZoneTime3()
        {
            setLevel(3);
            setSymbol("time_in_zone_P3");
            setInternalName("P3 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P3 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime3(*this); }
};

class PaceZoneTime4 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime4)

    public:
        PaceZoneTime4()
        {
            setLevel(4);
            setSymbol("time_in_zone_P4");
            setInternalName("P4 Time in Pace Zone");
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        void initialize ()
        {
            setName(tr("P4 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime4(*this); }
};

class PaceZoneTime5 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime5)

    public:
        PaceZoneTime5()
        {
            setLevel(5);
            setSymbol("time_in_zone_P5");
            setInternalName("P5 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P5 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime5(*this); }
};

class PaceZoneTime6 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime6)

    public:
        PaceZoneTime6()
        {
            setLevel(6);
            setSymbol("time_in_zone_P6");
            setInternalName("P6 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P6 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime6(*this); }
};

class PaceZoneTime7 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime7)

    public:
        PaceZoneTime7()
        {
            setLevel(7);
            setSymbol("time_in_zone_P7");
            setInternalName("P7 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P7 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime7(*this); }
};

class PaceZoneTime8 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime8)

    public:
        PaceZoneTime8()
        {
            setLevel(8);
            setSymbol("time_in_zone_P8");
            setInternalName("P8 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P8 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime8(*this); }
};

class PaceZoneTime9 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime9)

    public:
        PaceZoneTime9()
        {
            setLevel(9);
            setSymbol("time_in_zone_P9");
            setInternalName("P9 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P9 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime9(*this); }
};

class PaceZoneTime10 : public PaceZoneTime {
    Q_DECLARE_TR_FUNCTIONS(PaceZoneTime10)

    public:
        PaceZoneTime10()
        {
            setLevel(10);
            setSymbol("time_in_zone_P10");
            setInternalName("P10 Time in Pace Zone");
        }
        void initialize ()
        {
            setName(tr("P10 Time in Pace Zone"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new PaceZoneTime10(*this); }
};

// Now for Time In Zone as a Percentage of Ride Time
class PaceZonePTime1 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime1)

    public:

        PaceZonePTime1()
        {
            setSymbol("percent_in_zone_P1");
            setInternalName("P1 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P1 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P1"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P1")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime1(*this); }
};

class PaceZonePTime2 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime2)

    public:

        PaceZonePTime2()
        {
            setSymbol("percent_in_zone_P2");
            setInternalName("P2 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P2 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P2"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P2")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime2(*this); }
};

class PaceZonePTime3 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime3)

    public:

        PaceZonePTime3()
        {
            setSymbol("percent_in_zone_P3");
            setInternalName("P3 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P3 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P3"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P3")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime3(*this); }
};

class PaceZonePTime4 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime4)

    public:

        PaceZonePTime4()
        {
            setSymbol("percent_in_zone_P4");
            setInternalName("P4 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P4 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P4"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P4")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime4(*this); }
};

class PaceZonePTime5 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime5)

    public:

        PaceZonePTime5()
        {
            setSymbol("percent_in_zone_P5");
            setInternalName("P5 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P5 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P5"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P5")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime5(*this); }
};

class PaceZonePTime6 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime6)

    public:

        PaceZonePTime6()
        {
            setSymbol("percent_in_zone_P6");
            setInternalName("P6 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P6 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P6"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P6")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime6(*this); }
};

class PaceZonePTime7 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime7)

    public:

        PaceZonePTime7()
        {
            setSymbol("percent_in_zone_P7");
            setInternalName("P7 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P7 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P7"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P7")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime7(*this); }
};

class PaceZonePTime8 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime8)

    public:

        PaceZonePTime8()
        {
            setSymbol("percent_in_zone_P8");
            setInternalName("P8 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P8 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P8"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P8")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime8(*this); }
};

class PaceZonePTime9 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime9)

    public:

        PaceZonePTime9()
        {
            setSymbol("percent_in_zone_P9");
            setInternalName("P9 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P9 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P9"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P9")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime9(*this); }
};

class PaceZonePTime10 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(PaceZonePTime10)

    public:

        PaceZonePTime10()
        {
            setSymbol("percent_in_zone_P10");
            setInternalName("P10 Percent in Pace Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("P10 Percent in Pace Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_P10"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_P10")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new PaceZonePTime10(*this); }
};

static bool addAllZones() {
    RideMetricFactory::instance().addMetric(PaceZoneTime1());
    RideMetricFactory::instance().addMetric(PaceZoneTime2());
    RideMetricFactory::instance().addMetric(PaceZoneTime3());
    RideMetricFactory::instance().addMetric(PaceZoneTime4());
    RideMetricFactory::instance().addMetric(PaceZoneTime5());
    RideMetricFactory::instance().addMetric(PaceZoneTime6());
    RideMetricFactory::instance().addMetric(PaceZoneTime7());
    RideMetricFactory::instance().addMetric(PaceZoneTime8());
    RideMetricFactory::instance().addMetric(PaceZoneTime9());
    RideMetricFactory::instance().addMetric(PaceZoneTime10());
    QVector<QString> deps;
    deps.append("time_in_zone_P1");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime1(), &deps);
    deps.clear();
    deps.append("time_in_zone_P2");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime2(), &deps);
    deps.clear();
    deps.append("time_in_zone_P3");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime3(), &deps);
    deps.clear();
    deps.append("time_in_zone_P4");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime4(), &deps);
    deps.clear();
    deps.append("time_in_zone_P5");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime5(), &deps);
    deps.clear();
    deps.append("time_in_zone_P6");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime6(), &deps);
    deps.clear();
    deps.append("time_in_zone_P7");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime7(), &deps);
    deps.clear();
    deps.append("time_in_zone_P8");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime8(), &deps);
    deps.clear();
    deps.append("time_in_zone_P9");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime9(), &deps);
    deps.clear();
    deps.append("time_in_zone_P10");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PaceZonePTime10(), &deps);
    return true;
}

static bool allZonesAdded = addAllZones();
