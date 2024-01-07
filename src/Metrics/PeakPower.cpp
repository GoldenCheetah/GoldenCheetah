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
#include "AddIntervalDialog.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include "Zones.h"
#include <cmath>
#include <QApplication>

class PeakPercent : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(PeakPercent)
    double maxp;
    double minp;

    public:

    PeakPercent() : maxp(0.0), minp(10000)
    {
        setType(RideMetric::Average);
        setSymbol("peak_percent");
        setInternalName("MMP Percentage");
    }
    void initialize ()
    {
        setName(tr("MMP Percentage"));
        setMetricUnits(tr("%"));
        setPrecision(1); // e.g. 99.9%
        setImperialUnits(tr("%"));
        setDescription(tr("Average Power as Percent of Mean Maximal Power for Duration."));
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P"); }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // no ride or no samples
        if (!item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        int ap = deps.value("average_power")->value(true);
        int duration = deps.value("workout_time")->value(true);

        if (duration>120) {

            // get W' and CP parameters for 2 parameter model
            double CP = 250;
            double WPRIME = 22000;

            const Zones* zones = item->context->athlete->zones(item->sport);
            if (zones) {

                // if range is -1 we need to fall back to a default value
                CP = item->zoneRange >= 0 ? zones->getCP(item->zoneRange) : 250;
                WPRIME = item->zoneRange >= 0 ? zones->getWprime(item->zoneRange) : 22000;

                // did we override CP in metadata ?
                int oCP = item->getText("CP","0").toInt();
                if (oCP) CP=oCP;
            }

            // work out waht actual TTE is for this value
            int joules = ap * duration;
            double tc = (joules - WPRIME) / CP;
            setValue(100.0f * tc / double(duration));

        } else {
            setValue(0); // not for < 2m
        }
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakPercent(*this); }
};

class PowerZone : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(PowerZone)

    public:

    PowerZone()
    {
        setType(RideMetric::Average);
        setSymbol("power_zone");
        setInternalName("Power Zone");
    }
    void initialize ()
    {
        setName(tr("Power Zone"));
        setMetricUnits(tr(""));
        setPrecision(1); // e.g. 99.9%
        setImperialUnits(tr(""));
        setDescription(tr("Power Zone fractional number determined from Average Power."));
    }

    //QString toString(bool useMetricUnits) const {
        //if (value() == 0) return QString("N/A");
        //else return ;
    //}

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P"); }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // no zones
        const Zones* zones = item->context->athlete->zones(item->sport);
        if (!zones || !item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double ap = deps.value("average_power")->value(true);
        double percent=0;

        // if range is -1 we need to fall back to a default value
        int zone = item->zoneRange >= 0 ? zones->whichZone(item->zoneRange, ap) + 1 : 0;

        // ok, how far up  the zone was this?
        if (item->zoneRange >= 0 && zone) {

            // get zone info
            QString name, description;
            int low, high;
            zones->zoneInfo(item->zoneRange, zone-1, name, description, low, high);

            // use Pmax as upper bound, this is used
            // for the limit of upper zone ALWAYS
            const int pmax = zones->getPmax(item->zoneRange);
            high = std::min(high, pmax);
            
            // how far in?
            percent = double(ap-low) / double(high-low);

            // avoid rounding up !
            if (percent >0.9f && percent <1.00f) percent = 0.9f;
        }

        // we want 4.1 as zone, for 10% into zone 4
        setValue(double(zone) + percent);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PowerZone(*this); }
};

class FatigueIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(FatigueIndex)
    double maxp;
    double minp;

    public:

    FatigueIndex() : maxp(0.0), minp(10000)
    {
        setType(RideMetric::Average);
        setSymbol("power_fatigue_index");
        setInternalName("Fatigue Index");
    }
    void initialize ()
    {
        setName(tr("Fatigue Index"));
        setMetricUnits(tr("%"));
        setPrecision(1); // e.g. 99.9%
        setImperialUnits(tr("%"));
        setDescription(tr("Fatigue Index is power decay from Max Power to Min Power as a percent of Max Power."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // find peak and work from that
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->watts > maxp && point->watts != 0) minp = maxp = point->watts;
        }

        // now again and find peak
        bool hitpeak = false;
        it.toFront();
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (hitpeak == false && point->watts >= maxp) hitpeak = true;
            if (hitpeak == true && point->watts < minp && point->watts != 0) minp = point->watts;
        }

        if (minp > maxp) setValue(0.00); // minp wasn't changed, all zeroes?
        else setValue(100 * ((maxp-minp)/maxp)); // as a percentage
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new FatigueIndex(*this); }
};

class PacingIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PacingIndex)

    double maxp;
    double count, total;

    public:

    PacingIndex() : maxp(0.0), count(0), total(0)
    {
        setType(RideMetric::Average);
        setSymbol("power_pacing_index");
        setInternalName("Pacing Index");
    }
    void initialize ()
    {
        setName(tr("Pacing Index"));
        setMetricUnits(tr("%"));
        setPrecision(1); // e.g. 99.9%
        setImperialUnits(tr("%"));
        setDescription(tr("Pacing Index is Average Power as a percent of Maximal Power"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // find peak and work from that
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->watts > maxp && point->watts != 0) maxp = point->watts;
            total += point->watts;
            count++;
        }

        if (!count || !total) setValue(0.00); // minp wasn't changed, all zeroes?
        else setValue(((total/count) / maxp) * 100.00f);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PacingIndex(*this); }
};

class PeakPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PeakPower)
    double watts;
    double secs;

    public:

    PeakPower() : watts(0.0), secs(0.0)
    {
        setType(RideMetric::Peak);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        QList<AddIntervalDialog::AddedInterval> results;
        AddIntervalDialog::findPeaks(item->context, true, item->ride(), spec, RideFile::watts, RideFile::original, secs, 1, results, "", "");
        if (results.count() > 0 && results.first().avg < 3000) watts = results.first().avg;
        else watts = 0.0;

        setValue(watts);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakPower(*this); }
};

class PeakPower60m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower60m)
    public:
        PeakPower60m()
        {
            setSecs(3600);
            setSymbol("60m_critical_power");
            setInternalName("60 min Peak Power");
        }
        void initialize () {
            setName(tr("60 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower60m(*this); }
};

class PeakPower1s : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower1s)
    public:
        PeakPower1s()
        {
            setSecs(1);
            setSymbol("1s_critical_power");
            setInternalName("1 sec Peak Power");
        }
        void initialize () {
            setName(tr("1 sec Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower1s(*this); }
};

class PeakPower5s : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower5s)
    public:
        PeakPower5s()
        {
            setSecs(5);
            setSymbol("5s_critical_power");
            setInternalName("5 sec Peak Power");
        }
        void initialize () {
            setName(tr("5 sec Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower5s(*this); }
};

class PeakPower10s : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower10s)
    public:
        PeakPower10s()
        {
            setSecs(10);
            setSymbol("10s_critical_power");
            setInternalName("10 sec Peak Power");
        }
        void initialize () {
            setName(tr("10 sec Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower10s(*this); }
};

class PeakPower15s : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower15s)
    public:
        PeakPower15s()
        {
            setSecs(15);
            setSymbol("15s_critical_power");
            setInternalName("15 sec Peak Power");
        }
        void initialize () {
            setName(tr("15 sec Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower15s(*this); }
};

class PeakPower20s : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower20s)
    public:
        PeakPower20s()
        {
            setSecs(20);
            setSymbol("20s_critical_power");
            setInternalName("20 sec Peak Power");
        }
        void initialize () {
            setName(tr("20 sec Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower20s(*this); }
};

class PeakPower30s : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower30s)
    public:
        PeakPower30s()
        {
            setSecs(30);
            setSymbol("30s_critical_power");
            setInternalName("30 sec Peak Power");
        }
        void initialize () {
            setName(tr("30 sec Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower30s(*this); }
};

class PeakPower1m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower1m)
    public:
        PeakPower1m()
        {
            setSecs(60);
            setSymbol("1m_critical_power");
            setInternalName("1 min Peak Power");
        }
        void initialize () {
            setName(tr("1 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower1m(*this); }
};

class PeakPower2m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower2m)
    public:
        PeakPower2m()
        {
            setSecs(120);
            setSymbol("2m_critical_power");
            setInternalName("2 min Peak Power");
        }
        void initialize () {
            setName(tr("2 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower2m(*this); }
};

class PeakPower3m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower3m)
    public:
        PeakPower3m()
        {
            setSecs(180);
            setSymbol("3m_critical_power");
            setInternalName("3 min Peak Power");
        }
        void initialize () {
            setName(tr("3 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower3m(*this); }
};

class PeakPower5m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower5m)
    public:
        PeakPower5m()
        {
            setSecs(300);
            setSymbol("5m_critical_power");
            setInternalName("5 min Peak Power");
        }
        void initialize () {
            setName(tr("5 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower5m(*this); }
};

class PeakPower8m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower8m)
    public:
        PeakPower8m()
        {
            setSecs(8*60);
            setSymbol("8m_critical_power");
            setInternalName("8 min Peak Power");
        }
        void initialize () {
            setName(tr("8 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower8m(*this); }
};

class PeakPower10m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower10m)
    public:
        PeakPower10m()
        {
            setSecs(600);
            setSymbol("10m_critical_power");
            setInternalName("10 min Peak Power");
        }
        void initialize () {
            setName(tr("10 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower10m(*this); }
};

class PeakPower20m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower20m)
    public:
        PeakPower20m()
        {
            setSecs(1200);
            setSymbol("20m_critical_power");
            setInternalName("20 min Peak Power");
        }
        void initialize () {
            setName(tr("20 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower20m(*this); }
};

class PeakPower30m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower30m)
    public:
        PeakPower30m()
        {
            setSecs(1800);
            setSymbol("30m_critical_power");
            setInternalName("30 min Peak Power");
        }
        void initialize () {
            setName(tr("30 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower30m(*this); }
};

class PeakPower90m : public PeakPower {
    Q_DECLARE_TR_FUNCTIONS(PeakPower90m)
    public:
        PeakPower90m()
        {
            setSecs(90*60);
            setSymbol("90m_critical_power");
            setInternalName("90 min Peak Power");
        }
        void initialize () {
            setName(tr("90 min Peak Power"));
            setMetricUnits(tr("watts"));
            setImperialUnits(tr("watts"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPower90m(*this); }
};

class PeakPowerHr : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PeakPowerHr)

    double hr;
    double secs;

    public:

    PeakPowerHr() : hr(0.0), secs(0.0)
    {
        setType(RideMetric::Peak);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // find peak power interval
        QList<AddIntervalDialog::AddedInterval> results;
        AddIntervalDialog::findPeaks(item->context, true, item->ride(), spec, RideFile::watts, RideFile::original, secs, 1, results, "", "");

        // work out average hr during that interval
        if (results.count() > 0) {

            // start and stop is in seconds within the ride
            double start = results.first().start;
            double stop = results.first().stop;
            int points = 0;

            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                if (point->secs >= start && point->secs < stop) {
                    points++;
                    hr = (point->hr + (points-1)*hr) / (points);
                }
            }
        }

        setValue(hr);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakPowerHr(*this); }
};

class PeakPowerHr1m : public PeakPowerHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPowerHr1m)

    public:
        PeakPowerHr1m()
        {
            setSecs(60);
            setSymbol("1m_critical_power_hr");
            setInternalName("1 min Peak Power HR");
        }
        void initialize () {
            setName(tr("1 min Peak Power HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 1 min Peak Power interval"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPowerHr1m(*this); }
};

class PeakPowerHr5m : public PeakPowerHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPowerHr5m)

    public:
        PeakPowerHr5m()
        {
            setSecs(300);
            setSymbol("5m_critical_power_hr");
            setInternalName("5 min Peak Power HR");
        }
        void initialize () {
            setName(tr("5 min Peak Power HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 5 min Peak Power interval"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPowerHr5m(*this); }
};

class PeakPowerHr10m : public PeakPowerHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPowerHr10m)

    public:
        PeakPowerHr10m()
        {
            setSecs(600);
            setSymbol("10m_critical_power_hr");
            setInternalName("10 min Peak Power HR");
        }
        void initialize () {
            setName(tr("10 min Peak Power HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 10 min Peak Power interval"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPowerHr10m(*this); }
};

class PeakPowerHr20m : public PeakPowerHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPowerHr20m)

    public:
        PeakPowerHr20m()
        {
            setSecs(1200);
            setSymbol("20m_critical_power_hr");
            setInternalName("20 min Peak Power HR");
        }
        void initialize () {
            setName(tr("20 min Peak Power HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 20 min Peak Power interval"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPowerHr20m(*this); }
};

class PeakPowerHr30m : public PeakPowerHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPowerHr30m)

    public:
        PeakPowerHr30m()
        {
            setSecs(1800);
            setSymbol("30m_critical_power_hr");
            setInternalName("30 min Peak Power HR");
        }
        void initialize () {
            setName(tr("30 min Peak Power HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 30 min Peak Power interval"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPowerHr30m(*this); }
};


class PeakPowerHr60m : public PeakPowerHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPowerHr60m)

    public:
        PeakPowerHr60m()
        {
            setSecs(3600);
            setSymbol("60m_critical_power_hr");
            setInternalName("60 min Peak Power HR");
        }
        void initialize () {
            setName(tr("60 min Peak Power HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 60 min Peak Power interval"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPowerHr60m(*this); }
};

static bool addAllPeaks() {

    QVector<QString> deps;
    deps.clear();
    deps.append("average_power");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(PeakPercent(), &deps);
    RideMetricFactory::instance().addMetric(PowerZone(), &deps);
    RideMetricFactory::instance().addMetric(FatigueIndex());
    RideMetricFactory::instance().addMetric(PacingIndex());  

    RideMetricFactory::instance().addMetric(PeakPower1s());
    RideMetricFactory::instance().addMetric(PeakPower5s());
    RideMetricFactory::instance().addMetric(PeakPower10s());
    RideMetricFactory::instance().addMetric(PeakPower15s());
    RideMetricFactory::instance().addMetric(PeakPower20s());
    RideMetricFactory::instance().addMetric(PeakPower30s());
    RideMetricFactory::instance().addMetric(PeakPower1m());
    RideMetricFactory::instance().addMetric(PeakPower2m());
    RideMetricFactory::instance().addMetric(PeakPower3m());
    RideMetricFactory::instance().addMetric(PeakPower5m());
    RideMetricFactory::instance().addMetric(PeakPower8m());
    RideMetricFactory::instance().addMetric(PeakPower10m());
    RideMetricFactory::instance().addMetric(PeakPower20m());
    RideMetricFactory::instance().addMetric(PeakPower30m());
    RideMetricFactory::instance().addMetric(PeakPower60m());
    RideMetricFactory::instance().addMetric(PeakPower90m());
    RideMetricFactory::instance().addMetric(PeakPowerHr1m());
    RideMetricFactory::instance().addMetric(PeakPowerHr5m());
    RideMetricFactory::instance().addMetric(PeakPowerHr10m());
    RideMetricFactory::instance().addMetric(PeakPowerHr20m());
    RideMetricFactory::instance().addMetric(PeakPowerHr30m());
    RideMetricFactory::instance().addMetric(PeakPowerHr60m());
    return true;
}

static bool allPeaksAdded = addAllPeaks();
