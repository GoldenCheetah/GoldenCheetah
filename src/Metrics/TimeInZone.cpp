
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
#include "RideItem.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include "Zones.h"
#include <cmath>
#include <assert.h>
#include <QApplication>

class ZoneTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ZoneTime)
    int level;
    double seconds;

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

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) ||
            item->context->athlete->zones(item->sport) == NULL || item->zoneRange < 0 ||
            !item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double totalSecs = 0.0;
        seconds = 0;

        // iterate and compute
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            totalSecs += item->ride()->recIntSecs();
            if (item->context->athlete->zones(item->sport)->whichZone(item->zoneRange, point->watts) == level)
                seconds += item->ride()->recIntSecs();
        }
        setValue(seconds);
        setCount(totalSecs);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 1."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 2."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 3."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 4."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 5."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 6."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 7."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 8."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 9."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Time in Power Zone 10."));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 1."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L1"));

            // compute
            double time = deps.value("time_in_zone_L1")->count();
            double inzone = deps.value("time_in_zone_L1")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 2."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L2"));

            // compute
            double time = deps.value("time_in_zone_L2")->count();
            double inzone = deps.value("time_in_zone_L2")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 3."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L3"));

            // compute
            double time = deps.value("time_in_zone_L3")->count();
            double inzone = deps.value("time_in_zone_L3")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 4."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L4"));

            // compute
            double time = deps.value("time_in_zone_L4")->count();
            double inzone = deps.value("time_in_zone_L4")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 5."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L5"));

            // compute
            double time = deps.value("time_in_zone_L5")->count();
            double inzone = deps.value("time_in_zone_L5")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 6."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L6"));

            // compute
            double time = deps.value("time_in_zone_L6")->count();
            double inzone = deps.value("time_in_zone_L6")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 7."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L7"));

            // compute
            double time = deps.value("time_in_zone_L7")->count();
            double inzone = deps.value("time_in_zone_L7")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 8."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L8"));

            // compute
            double time = deps.value("time_in_zone_L8")->count();
            double inzone = deps.value("time_in_zone_L8")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 9."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L9"));

            // compute
            double time = deps.value("time_in_zone_L9")->count();
            double inzone = deps.value("time_in_zone_L9")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Power Zone 10."));
        }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_L10"));

            // compute
            double time = deps.value("time_in_zone_L10")->count();
            double inzone = deps.value("time_in_zone_L10")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new ZonePTime10(*this); }
};

// Time in Zones I, II and III
class ZoneTimeI : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ZoneTimeI)
    double seconds;

    public:

    ZoneTimeI() : seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
        setSymbol("time_in_zone_LI");
        setInternalName("LI Time in Zone");
    }

    void initialize ()
    {
        setName(tr("LI Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time in Power Zone I - Below AeT"));
    }

    bool isTime() const { return true; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) ||
            item->context->athlete->zones(item->sport) == NULL || item->zoneRange < 0 ||
            !item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        int AeT = item->context->athlete->zones(item->sport)->getAeT(item->zoneRange);
        double totalSecs = 0.0;
        seconds = 0;

        // iterate and compute
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            totalSecs += item->ride()->recIntSecs();
            if (point->watts < AeT)
                seconds += item->ride()->recIntSecs();
        }
        setValue(seconds);
        setCount(totalSecs);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ZoneTimeI(*this); }
};

class ZoneTimeII : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ZoneTimeII)
    double seconds;

    public:

    ZoneTimeII() : seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
        setSymbol("time_in_zone_LII");
        setInternalName("LII Time in Zone");
    }

    void initialize ()
    {
        setName(tr("LII Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time in Power Zone II - Between AeT and CP"));
    }

    bool isTime() const { return true; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) ||
            item->context->athlete->zones(item->sport) == NULL || item->zoneRange < 0 ||
            !item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        int AeT = item->context->athlete->zones(item->sport)->getAeT(item->zoneRange);
        int CP = item->context->athlete->zones(item->sport)->getCP(item->zoneRange);
        double totalSecs = 0.0;
        seconds = 0;

        // iterate and compute
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            totalSecs += item->ride()->recIntSecs();
            if (point->watts >= AeT && point->watts < CP)
                seconds += item->ride()->recIntSecs();
        }
        setValue(seconds);
        setCount(totalSecs);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ZoneTimeII(*this); }
};

class ZoneTimeIII : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ZoneTimeIII)
    double seconds;

    public:

    ZoneTimeIII() : seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
        setSymbol("time_in_zone_LIII");
        setInternalName("LIII Time in Zone");
    }

    void initialize ()
    {
        setName(tr("LIII Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time in Power Zone III - Above CP"));
    }

    bool isTime() const { return true; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) ||
            item->context->athlete->zones(item->sport) == NULL || item->zoneRange < 0 ||
            !item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        int CP = item->context->athlete->zones(item->sport)->getCP(item->zoneRange);
        double totalSecs = 0.0;
        seconds = 0;

        // iterate and compute
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            totalSecs += item->ride()->recIntSecs();
            if (point->watts >= CP)
                seconds += item->ride()->recIntSecs();
        }
        setValue(seconds);
        setCount(totalSecs);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ZoneTimeIII(*this); }
};

// Now for Time In Zones as a Percentage of Ride Time
class ZonePTimeI : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ZonePTimeI)

    public:

    ZonePTimeI()
    {
        setSymbol("percent_in_zone_LI");
        setInternalName("LI Percent in Zone");
        setType(RideMetric::Average);
        setMetricUnits("%");
        setImperialUnits("%");
        setPrecision(0);
        setConversion(1.0);
    }

    void initialize ()
    {
        setName(tr("LI Percent in Zone"));
        setDescription(tr("Percent of Time in Power Zone I - Below AeT"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps)
    {
        assert(deps.contains("time_in_zone_LI"));

        // compute
        double time = deps.value("time_in_zone_LI")->count();
        double inzone = deps.value("time_in_zone_LI")->value(true);

        if (time && inzone) setValue((inzone / time) * 100.00);
        else setValue(0);
        setCount(time);
    }

    bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ZonePTimeI(*this); }
};

class ZonePTimeII : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ZonePTimeII)

    public:

    ZonePTimeII()
    {
        setSymbol("percent_in_zone_LII");
        setInternalName("LII Percent in Zone");
        setType(RideMetric::Average);
        setMetricUnits("%");
        setImperialUnits("%");
        setPrecision(0);
        setConversion(1.0);
    }

    void initialize ()
    {
        setName(tr("LII Percent in Zone"));
        setDescription(tr("Percent of Time in Power Zone II - Between AeT and CP"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps)
    {
        assert(deps.contains("time_in_zone_LII"));

        // compute
        double time = deps.value("time_in_zone_LII")->count();
        double inzone = deps.value("time_in_zone_LII")->value(true);

        if (time && inzone) setValue((inzone / time) * 100.00);
        else setValue(0);
        setCount(time);
    }

    bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ZonePTimeII(*this); }
};

class ZonePTimeIII : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ZonePTimeIII)

    public:

    ZonePTimeIII()
    {
        setSymbol("percent_in_zone_LIII");
        setInternalName("LIII Percent in Zone");
        setType(RideMetric::Average);
        setMetricUnits("%");
        setImperialUnits("%");
        setPrecision(0);
        setConversion(1.0);
    }

    void initialize ()
    {
        setName(tr("LIII Percent in Zone"));
        setDescription(tr("Percent of Time in Power Zone III - Above CP"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps)
    {
        assert(deps.contains("time_in_zone_LIII"));

        // compute
        double time = deps.value("time_in_zone_LIII")->count();
        double inzone = deps.value("time_in_zone_LIII")->value(true);

        if (time && inzone) setValue((inzone / time) * 100.00);
        else setValue(0);
        setCount(time);
    }

    bool aggregateZero() const { return true; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ZonePTimeIII(*this); }
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
    RideMetricFactory::instance().addMetric(ZonePTime1(), &deps);
    deps.clear();
    deps.append("time_in_zone_L2");
    RideMetricFactory::instance().addMetric(ZonePTime2(), &deps);
    deps.clear();
    deps.append("time_in_zone_L3");
    RideMetricFactory::instance().addMetric(ZonePTime3(), &deps);
    deps.clear();
    deps.append("time_in_zone_L4");
    RideMetricFactory::instance().addMetric(ZonePTime4(), &deps);
    deps.clear();
    deps.append("time_in_zone_L5");
    RideMetricFactory::instance().addMetric(ZonePTime5(), &deps);
    deps.clear();
    deps.append("time_in_zone_L6");
    RideMetricFactory::instance().addMetric(ZonePTime6(), &deps);
    deps.clear();
    deps.append("time_in_zone_L7");
    RideMetricFactory::instance().addMetric(ZonePTime7(), &deps);
    deps.clear();
    deps.append("time_in_zone_L8");
    RideMetricFactory::instance().addMetric(ZonePTime8(), &deps);
    deps.clear();
    deps.append("time_in_zone_L9");
    RideMetricFactory::instance().addMetric(ZonePTime9(), &deps);
    deps.clear();
    deps.append("time_in_zone_L10");
    RideMetricFactory::instance().addMetric(ZonePTime10(), &deps);

    RideMetricFactory::instance().addMetric(ZoneTimeI());
    RideMetricFactory::instance().addMetric(ZoneTimeII());
    RideMetricFactory::instance().addMetric(ZoneTimeIII());

    deps.clear();
    deps.append("time_in_zone_LI");
    RideMetricFactory::instance().addMetric(ZonePTimeI(), &deps);
    deps.clear();
    deps.append("time_in_zone_LII");
    RideMetricFactory::instance().addMetric(ZonePTimeII(), &deps);
    deps.clear();
    deps.append("time_in_zone_LIII");
    RideMetricFactory::instance().addMetric(ZonePTimeIII(), &deps);

    return true;
}

static bool allZonesAdded = addAllZones();
