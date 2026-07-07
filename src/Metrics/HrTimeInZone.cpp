
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
#include "RideItem.h"
#include "HrZones.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include <cmath>
#include <assert.h>
#include <QApplication>

class HrZoneTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime)
    int level;
    double seconds;

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

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double totalSecs = 0.0;
        seconds = 0;

        // get zone ranges
        if (item->context->athlete->hrZones(item->sport) && item->hrZoneRange >= 0 && item->ride()->areDataPresent()->hr) {
            // iterate and compute
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                totalSecs += item->ride()->recIntSecs();
                if (item->context->athlete->hrZones(item->sport)->whichZone(item->hrZoneRange, point->hr) == level)
                    seconds += item->ride()->recIntSecs();
            }
        }
        setValue(seconds);
        setCount(totalSecs);
    }

    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Time in Heart Rate Zone 1."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Time in Heart Rate Zone 2."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Time in Heart Rate Zone 3."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Time in Heart Rate Zone 4."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Time in Heart Rate Zone 5."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Time in Heart Rate Zone 6."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Time in Heart Rate Zone 7."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setDescription(tr("Time in Heart Rate Zone 8."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZoneTime8(*this); }
};

class HrZoneTime9 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime9)

public:
    HrZoneTime9()
    {
        setLevel(9);
        setSymbol("time_in_zone_H9");
        setInternalName("H9 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H9 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time in Heart Rate Zone 9."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZoneTime9(*this); }
};
class HrZoneTime10 : public HrZoneTime {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTime10)

public:
    HrZoneTime10()
    {
        setLevel(10);
        setSymbol("time_in_zone_H10");
        setInternalName("H10 Time in Zone");
    }
    void initialize ()
    {
        setName(tr("H10 Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time in Heart Rate Zone 10."));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZoneTime10(*this); }
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
            setDescription(tr("Percent of Time in Heart Rate Zone 1."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H1"));

            // compute
            double time = deps.value("time_in_zone_H1")->count();
            double inzone = deps.value("time_in_zone_H1")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Heart Rate Zone 2."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H2"));

            // compute
            double time = deps.value("time_in_zone_H2")->count();
            double inzone = deps.value("time_in_zone_H2")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Heart Rate Zone 3."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H3"));

            // compute
            double time = deps.value("time_in_zone_H3")->count();
            double inzone = deps.value("time_in_zone_H3")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Heart Rate Zone 4."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H4"));

            // compute
            double time = deps.value("time_in_zone_H4")->count();
            double inzone = deps.value("time_in_zone_H4")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Heart Rate Zone 5."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H5"));

            // compute
            double time = deps.value("time_in_zone_H5")->count();
            double inzone = deps.value("time_in_zone_H5")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Heart Rate Zone 6."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H6"));

            // compute
            double time = deps.value("time_in_zone_H6")->count();
            double inzone = deps.value("time_in_zone_H6")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Heart Rate Zone 7."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H7"));

            // compute
            double time = deps.value("time_in_zone_H7")->count();
            double inzone = deps.value("time_in_zone_H7")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
            setDescription(tr("Percent of Time in Heart Rate Zone 8."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H8"));

            // compute
            double time = deps.value("time_in_zone_H8")->count();
            double inzone = deps.value("time_in_zone_H8")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new HrZonePTime8(*this); }
};
class HrZonePTime9 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime9)

    public:

        HrZonePTime9()
        {
            setSymbol("percent_in_zone_H9");
            setInternalName("H9 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H9 Percent in Zone"));
            setDescription(tr("Percent of Time in Heart Rate Zone 9."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H9"));

            // compute
            double time = deps.value("time_in_zone_H9")->count();
            double inzone = deps.value("time_in_zone_H9")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new HrZonePTime9(*this); }
};
class HrZonePTime10 : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTime10)

    public:

        HrZonePTime10()
        {
            setSymbol("percent_in_zone_H10");
            setInternalName("H10 Percent in Zone");
            setType(RideMetric::Average);
            setMetricUnits("%");
            setImperialUnits("%");
            setPrecision(0);
            setConversion(1.0);
        }

        void initialize ()
        {
            setName(tr("H10 Percent in Zone"));
            setDescription(tr("Percent of Time in Heart Rate Zone 10."));
        }

        void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

            assert(deps.contains("time_in_zone_H10"));

            // compute
            double time = deps.value("time_in_zone_H10")->count();
            double inzone = deps.value("time_in_zone_H10")->value(true);

            if (time && inzone) setValue((inzone / time) * 100.00);
            else setValue(0);
            setCount(time);
        }

        bool aggregateZero() const { return true; }
        bool canAggregate() { return false; }
        void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new HrZonePTime10(*this); }
};

// Time in Zones I, II and III
class HrZoneTimeI : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTimeI)
    double seconds;

public:

    HrZoneTimeI() : seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
        setSymbol("time_in_zone_HI");
        setInternalName("HI Time in Zone");
    }

    void initialize ()
    {
        setName(tr("HI Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time in Heart Rate Zone I - Below AeT"));
    }

    bool isTime() const { return true; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double totalSecs = 0.0;
        seconds = 0;

        // get zone ranges
        if (item->context->athlete->hrZones(item->sport) && item->hrZoneRange >= 0 && item->ride()->areDataPresent()->hr) {

            int AeT = item->context->athlete->hrZones(item->sport)->getAeT(item->hrZoneRange);

            // iterate and compute
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                totalSecs += item->ride()->recIntSecs();
                if (point->hr < AeT)
                    seconds += item->ride()->recIntSecs();
            }
        }
        setValue(seconds);
        setCount(totalSecs);
    }

    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZoneTimeI(*this); }
};

class HrZoneTimeII : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTimeII)
    double seconds;

public:

    HrZoneTimeII() : seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
        setSymbol("time_in_zone_HII");
        setInternalName("HII Time in Zone");
    }

    void initialize ()
    {
        setName(tr("HII Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time in Heart Rate Zone II - Between AeT and LT"));
    }

    bool isTime() const { return true; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double totalSecs = 0.0;
        seconds = 0;

        // get zone ranges
        if (item->context->athlete->hrZones(item->sport) && item->hrZoneRange >= 0 && item->ride()->areDataPresent()->hr) {

            int AeT = item->context->athlete->hrZones(item->sport)->getAeT(item->hrZoneRange);
            int LT = item->context->athlete->hrZones(item->sport)->getLT(item->hrZoneRange);

            // iterate and compute
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                totalSecs += item->ride()->recIntSecs();
                if (point->hr >= AeT && point->hr < LT)
                    seconds += item->ride()->recIntSecs();
            }
        }
        setValue(seconds);
        setCount(totalSecs);
    }

    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZoneTimeII(*this); }
};

class HrZoneTimeIII : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(HrZoneTimeIII)
    double seconds;

public:

    HrZoneTimeIII() : seconds(0.0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
        setSymbol("time_in_zone_HIII");
        setInternalName("HIII Time in Zone");
    }

    void initialize ()
    {
        setName(tr("HIII Time in Zone"));
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setDescription(tr("Time in Heart Rate Zone III - Above LT"));
    }

    bool isTime() const { return true; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double totalSecs = 0.0;
        seconds = 0;

        // get zone ranges
        if (item->context->athlete->hrZones(item->sport) && item->hrZoneRange >= 0 && item->ride()->areDataPresent()->hr) {

            int LT = item->context->athlete->hrZones(item->sport)->getLT(item->hrZoneRange);

            // iterate and compute
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                totalSecs += item->ride()->recIntSecs();
                if (point->hr >= LT)
                    seconds += item->ride()->recIntSecs();
            }
        }
        setValue(seconds);
        setCount(totalSecs);
    }

    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZoneTimeIII(*this); }
};

// Now for Time In Zone as a Percentage of Ride Time
class HrZonePTimeI : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTimeI)

    public:

    HrZonePTimeI()
    {
        setSymbol("percent_in_zone_HI");
        setInternalName("HI Percent in Zone");
        setType(RideMetric::Average);
        setMetricUnits("%");
        setImperialUnits("%");
        setPrecision(0);
        setConversion(1.0);
    }

    void initialize ()
    {
        setName(tr("HI Percent in Zone"));
        setDescription(tr("Percent of Time in Heart Rate Zone I - Below AeT"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("time_in_zone_HI"));

        // compute
        double time = deps.value("time_in_zone_HI")->count();
        double inzone = deps.value("time_in_zone_HI")->value(true);

        if (time && inzone) setValue((inzone / time) * 100.00);
        else setValue(0);
        setCount(time);
    }

    bool aggregateZero() const { return true; }
    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZonePTimeI(*this); }
};

class HrZonePTimeII : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTimeII)

    public:

    HrZonePTimeII()
    {
        setSymbol("percent_in_zone_HII");
        setInternalName("HII Percent in Zone");
        setType(RideMetric::Average);
        setMetricUnits("%");
        setImperialUnits("%");
        setPrecision(0);
        setConversion(1.0);
    }

    void initialize ()
    {
        setName(tr("HII Percent in Zone"));
        setDescription(tr("Percent of Time in Heart Rate Zone II - Between AeT and LT"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("time_in_zone_HII"));

        // compute
        double time = deps.value("time_in_zone_HII")->count();
        double inzone = deps.value("time_in_zone_HII")->value(true);

        if (time && inzone) setValue((inzone / time) * 100.00);
        else setValue(0);
        setCount(time);
    }

    bool aggregateZero() const { return true; }
    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZonePTimeII(*this); }
};

class HrZonePTimeIII : public RideMetric {

        Q_DECLARE_TR_FUNCTIONS(HrZonePTimeIII)

    public:

    HrZonePTimeIII()
    {
        setSymbol("percent_in_zone_HIII");
        setInternalName("HIII Percent in Zone");
        setType(RideMetric::Average);
        setMetricUnits("%");
        setImperialUnits("%");
        setPrecision(0);
        setConversion(1.0);
    }

    void initialize ()
    {
        setName(tr("HIII Percent in Zone"));
        setDescription(tr("Percent of Time in Heart Rate Zone III - Above LT"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("time_in_zone_HIII"));

        // compute
        double time = deps.value("time_in_zone_HIII")->count();
        double inzone = deps.value("time_in_zone_HIII")->value(true);

        if (time && inzone) setValue((inzone / time) * 100.00);
        else setValue(0);
        setCount(time);
    }

    bool aggregateZero() const { return true; }
    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZonePTimeIII(*this); }
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
    RideMetricFactory::instance().addMetric(HrZoneTime9());
    RideMetricFactory::instance().addMetric(HrZoneTime10());

    QVector<QString> deps;
    deps.append("time_in_zone_H1");
    RideMetricFactory::instance().addMetric(HrZonePTime1(), &deps);
    deps.clear();
    deps.append("time_in_zone_H2");
    RideMetricFactory::instance().addMetric(HrZonePTime2(), &deps);
    deps.clear();
    deps.append("time_in_zone_H3");
    RideMetricFactory::instance().addMetric(HrZonePTime3(), &deps);
    deps.clear();
    deps.append("time_in_zone_H4");
    RideMetricFactory::instance().addMetric(HrZonePTime4(), &deps);
    deps.clear();
    deps.append("time_in_zone_H5");
    RideMetricFactory::instance().addMetric(HrZonePTime5(), &deps);
    deps.clear();
    deps.append("time_in_zone_H6");
    RideMetricFactory::instance().addMetric(HrZonePTime6(), &deps);
    deps.clear();
    deps.append("time_in_zone_H7");
    RideMetricFactory::instance().addMetric(HrZonePTime7(), &deps);
    deps.clear();
    deps.append("time_in_zone_H8");
    RideMetricFactory::instance().addMetric(HrZonePTime8(), &deps);
    deps.clear();
    deps.append("time_in_zone_H9");
    RideMetricFactory::instance().addMetric(HrZonePTime9(), &deps);
    deps.clear();
    deps.append("time_in_zone_H10");
    RideMetricFactory::instance().addMetric(HrZonePTime10(), &deps);

    RideMetricFactory::instance().addMetric(HrZoneTimeI());
    RideMetricFactory::instance().addMetric(HrZoneTimeII());
    RideMetricFactory::instance().addMetric(HrZoneTimeIII());

    deps.clear();
    deps.append("time_in_zone_HI");
    RideMetricFactory::instance().addMetric(HrZonePTimeI(), &deps);
    deps.clear();
    deps.append("time_in_zone_HII");
    RideMetricFactory::instance().addMetric(HrZonePTimeII(), &deps);
    deps.clear();
    deps.append("time_in_zone_HIII");
    RideMetricFactory::instance().addMetric(HrZonePTimeIII(), &deps);

    return true;
}

static bool allHrZonesAdded = addAllHrZones();
