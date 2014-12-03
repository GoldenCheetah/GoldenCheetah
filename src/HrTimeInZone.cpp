
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
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
    }
    bool isTime() const { return true; }
    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *, int, const HrZones *hrZone, int hrZoneRange,
                 const QHash<QString,RideMetric*> &, const Context *)
    {
        seconds = 0;
        // get zone ranges
        if (hrZone && hrZoneRange >= 0 && ride->areDataPresent()->hr) {
            // iterate and compute
            foreach(const RideFilePoint *point, ride->dataPoints()) {
                if (hrZone->whichZone(hrZoneRange, point->hr) == level)
                    seconds += ride->recIntSecs();
            }
        }
        setValue(seconds);
    }

    bool canAggregate() { return false; }
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
        setInternalName("H1 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H1 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
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
        setInternalName("H2 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H2 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
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
        setInternalName("H3 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H3 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
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
        setInternalName("H4 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H4 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
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
        setInternalName("H5 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H5 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
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
        setInternalName("H6 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H6 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
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
        setInternalName("H7 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H7 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
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
        setInternalName("H8 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H8 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
    }
    RideMetric *clone() const { return new HrZoneTime8(*this); }
};

// Now for Time In Zone as a Percentage of Ride Time
class HrZonePTime1 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime1)

    public:

        HrZonePTime1()
        {
            setSymbol("percent_in_zone_H1");
            setInternalName("H1 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H1 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_H1"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_H1")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new HrZonePTime1(*this); }
};

class HrZonePTime2 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime2)

    public:

        HrZonePTime2()
        {
            setSymbol("percent_in_zone_H2");
            setInternalName("H2 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H2 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_H2"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_H2")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new HrZonePTime2(*this); }
};

class HrZonePTime3 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime3)

    public:

        HrZonePTime3()
        {
            setSymbol("percent_in_zone_H3");
            setInternalName("H3 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H3 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_H3"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_H3")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new HrZonePTime3(*this); }
};

class HrZonePTime4 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime4)

    public:

        HrZonePTime4()
        {
            setSymbol("percent_in_zone_H4");
            setInternalName("H4 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H4 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_H4"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_H4")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new HrZonePTime4(*this); }
};

class HrZonePTime5 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime5)

    public:

        HrZonePTime5()
        {
            setSymbol("percent_in_zone_H5");
            setInternalName("H5 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H5 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_H5"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_H5")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new HrZonePTime5(*this); }
};

class HrZonePTime6 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime6)

    public:

        HrZonePTime6()
        {
            setSymbol("percent_in_zone_H6");
            setInternalName("H6 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H6 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_H6"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_H6")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new HrZonePTime6(*this); }
};

class HrZonePTime7 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime7)

    public:

        HrZonePTime7()
        {
            setSymbol("percent_in_zone_H7");
            setInternalName("H7 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H7 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_H7"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_H7")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new HrZonePTime7(*this); }
};

class HrZonePTime8 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime8)

    public:

        HrZonePTime8()
        {
            setSymbol("percent_in_zone_H8");
            setInternalName("H8 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H8 Percent in Zone"));
        }

        void compute(const RideFile *, const Zones *, int,
                    const HrZones *, int,
                    const QHash<QString,RideMetric*> &deps,
                    const Context *)
        {
            assert(deps.contains("time_in_zone_H8"));
            assert(deps.contains("workout_time"));

            // compute
            double time = deps.value("workout_time")->value(true);
            double inzone = deps.value("time_in_zone_H8")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
        RideMetric *clone() const { return new HrZonePTime8(*this); }
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
    QVector<QString> deps;
    deps.append("time_in_zone_H1");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZonePTime1(), &deps);
    deps.clear();
    deps.append("time_in_zone_H2");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZonePTime2(), &deps);
    deps.clear();
    deps.append("time_in_zone_H3");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZonePTime3(), &deps);
    deps.clear();
    deps.append("time_in_zone_H4");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZonePTime4(), &deps);
    deps.clear();
    deps.append("time_in_zone_H5");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZonePTime5(), &deps);
    deps.clear();
    deps.append("time_in_zone_H6");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZonePTime6(), &deps);
    deps.clear();
    deps.append("time_in_zone_H7");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZonePTime7(), &deps);
    deps.clear();
    deps.append("time_in_zone_H8");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZonePTime8(), &deps);
    return true;
}

static bool allHrZonesAdded = addAllHrZones();
