/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2018 Ale Martinez (amtriathlon@gmail.com)
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
#include "HrZones.h"
#include <cmath>

class HrZone : public RideMetric {

    Q_DECLARE_TR_FUNCTIONS(HrZone)

    public:

    HrZone()
    {
        setType(RideMetric::Average);
        setSymbol("hr_zone");
        setInternalName("Hr Zone");
    }
    void initialize ()
    {
        setName(tr("Hr Zone"));
        setMetricUnits(tr(""));
        setPrecision(1); // e.g. 99.9%
        setImperialUnits(tr(""));
        setDescription(tr("Hr Zone fractional number determined from Average Hr."));
    }

    //QString toString(bool useMetricUnits) const {
        //if (value() == 0) return QString("N/A");
        //else return ;
    //}

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // no zones
        const HrZones* zones = item->context->athlete->hrZones(item->sport);
        if (!zones || !item->ride()->areDataPresent()->hr) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double ahr = deps.value("average_hr")->value(true);
        double percent=0;

        // if range is -1 we need to fall back to a default value
        int zone = item->hrZoneRange >= 0 ? zones->whichZone(item->hrZoneRange, ahr) + 1 : 0;

        // ok, how far up  the zone was this?
        if (item->hrZoneRange >= 0 && zone) {

            // get zone info
            QString name, description;
            int low, high;
            double trimp;
            zones->zoneInfo(item->hrZoneRange, zone-1, name, description, low, high, trimp);

            // use MaxHr as upper bound, this is used
            // for the limit of upper zone ALWAYS
            const int pmax = zones->getMaxHr(item->hrZoneRange);
            high = std::min(high, pmax);

            // how far in?
            percent = double(ahr-low) / double(high-low);

            // avoid rounding up !
            if (percent >0.9f && percent <1.00f) percent = 0.9f;
        }

        // we want 4.1 as zone, for 10% into zone 4
        setValue(double(zone) + percent);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new HrZone(*this); }
};


class PeakHr : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PeakHr)
    double hr;
    double secs;

    public:

    PeakHr() : hr(0.0), secs(0.0)
    {
        setType(RideMetric::Peak);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->hr) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        QList<AddIntervalDialog::AddedInterval> results;
        AddIntervalDialog::findPeaks(item->context, true, item->ride(), spec, RideFile::hr, RideFile::original, secs, 1, results, "", "");
        if (results.count() > 0 && results.first().avg < 300) hr = results.first().avg;
        else hr = 0.0;

        setValue(hr);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("H"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr(*this); }
};

class PeakHr1m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr1m)
    public:
    PeakHr1m()
    {
        setSecs(60);
        setSymbol("1m_peak_hr");
        setInternalName("1 min Peak Hr");
    }
    void initialize () {
        setName(tr("1 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr1m(*this); }
};

class PeakHr2m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr2m)
    public:
    PeakHr2m()
    {
        setSecs(120);
        setSymbol("2m_peak_hr");
        setInternalName("2 min Peak Hr");
    }
    void initialize () {
        setName(tr("2 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr2m(*this); }
};

class PeakHr3m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr3m)
    public:
    PeakHr3m()
    {
        setSecs(180);
        setSymbol("3m_peak_hr");
        setInternalName("3 min Peak Hr");
    }
    void initialize () {
        setName(tr("3 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr3m(*this); }
};

class PeakHr5m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr5m)
    public:
    PeakHr5m()
    {
        setSecs(300);
        setSymbol("5m_peak_hr");
        setInternalName("5 min Peak Hr");
    }
    void initialize () {
        setName(tr("5 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr5m(*this); }
};

class PeakHr8m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr8m)
    public:
    PeakHr8m()
    {
        setSecs(8*60);
        setSymbol("8m_peak_hr");
        setInternalName("8 min Peak Hr");
    }
    void initialize () {
        setName(tr("8 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr8m(*this); }
};

class PeakHr10m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr10m)
    public:
    PeakHr10m()
    {
        setSecs(600);
        setSymbol("10m_peak_hr");
        setInternalName("10 min Peak Hr");
    }
    void initialize () {
        setName(tr("10 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr10m(*this); }
};

class PeakHr20m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr20m)
    public:
    PeakHr20m()
    {
        setSecs(1200);
        setSymbol("20m_peak_hr");
        setInternalName("20 min Peak Hr");
    }
    void initialize () {
        setName(tr("20 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr20m(*this); }
};

class PeakHr30m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr30m)
    public:
    PeakHr30m()
    {
        setSecs(1800);
        setSymbol("30m_peak_hr");
        setInternalName("30 min Peak Hr");
    }
    void initialize () {
        setName(tr("30 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr30m(*this); }
};

class PeakHr60m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr60m)
    public:
    PeakHr60m()
    {
        setSecs(3600);
        setSymbol("60m_peak_hr");
        setInternalName("60 min Peak Hr");
    }
    void initialize () {
        setName(tr("60 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr60m(*this); }
};

class PeakHr90m : public PeakHr {
    Q_DECLARE_TR_FUNCTIONS(PeakHr90m)
    public:
    PeakHr90m()
    {
        setSecs(90*60);
        setSymbol("90m_peak_hr");
        setInternalName("90 min Peak Hr");
    }
    void initialize () {
        setName(tr("90 min Peak Hr"));
        setMetricUnits(tr("bpm"));
        setImperialUnits(tr("bpm"));
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakHr90m(*this); }
};


static bool addAllPeaks() {

    QVector<QString> deps;
    deps.clear();
    deps.append("average_hr");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(HrZone(), &deps);

    RideMetricFactory::instance().addMetric(PeakHr1m());
    RideMetricFactory::instance().addMetric(PeakHr2m());
    RideMetricFactory::instance().addMetric(PeakHr3m());
    RideMetricFactory::instance().addMetric(PeakHr5m());
    RideMetricFactory::instance().addMetric(PeakHr8m());
    RideMetricFactory::instance().addMetric(PeakHr10m());
    RideMetricFactory::instance().addMetric(PeakHr20m());
    RideMetricFactory::instance().addMetric(PeakHr30m());
    RideMetricFactory::instance().addMetric(PeakHr60m());
    RideMetricFactory::instance().addMetric(PeakHr90m());
    return true;
}

static bool allPeaksAdded = addAllPeaks();
