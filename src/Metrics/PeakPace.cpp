/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 *               2015 Alejandro Martinez (amtriathlon@gmail.com)
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
#include "AddIntervalDialog.h"
#include "RideItem.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include "Settings.h"
#include "Units.h"
#include <cmath>
#include <QApplication>

class PeakPace : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PeakPace)
    double pace;
    double secs;

    public:

    PeakPace() : pace(0.0), secs(0.0)
    {
        setType(RideMetric::Low);
    }
    // Pace ordering is reversed
    bool isLowerBetter() const { return true; }
    // Overrides to use Pace units setting
    QString units(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::units(metricRunPace);
    }
    double value(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(metricRunPace);
    }
    double value(double v, bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(v, metricRunPace);
    }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60, true);
    }
    QString toString(double v) const {
        return time_to_string(v*60, true);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->isRun) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        QList<AddIntervalDialog::AddedInterval> results;
        AddIntervalDialog::findPeaks(item->context, true, item->ride(), spec, RideFile::kph, RideFile::original, secs, 1, results, "", "");
        if (results.count() > 0 && results.first().avg > 0 && results.first().avg < 36) pace = 60.0 / results.first().avg;
        else pace = 0.0;

        setValue(pace);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakPace(*this); }
};

class PeakPace10s : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace10s)
    public:
        PeakPace10s()
        {
            setSecs(10);
            setSymbol("10s_critical_pace");
            setInternalName("10 sec Peak Pace");
        }
        void initialize () {
            setName(tr("10 sec Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace10s(*this); }
};

class PeakPace15s : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace15s)
    public:
        PeakPace15s()
        {
            setSecs(15);
            setSymbol("15s_critical_pace");
            setInternalName("15 sec Peak Pace");
        }
        void initialize () {
            setName(tr("15 sec Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace15s(*this); }
};

class PeakPace20s : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace20s)
    public:
        PeakPace20s()
        {
            setSecs(20);
            setSymbol("20s_critical_pace");
            setInternalName("20 sec Peak Pace");
        }
        void initialize () {
            setName(tr("20 sec Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace20s(*this); }
};

class PeakPace30s : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace30s)
    public:
        PeakPace30s()
        {
            setSecs(30);
            setSymbol("30s_critical_pace");
            setInternalName("30 sec Peak Pace");
        }
        void initialize () {
            setName(tr("30 sec Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace30s(*this); }
};

class PeakPace1m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace1m)
    public:
        PeakPace1m()
        {
            setSecs(60);
            setSymbol("1m_critical_pace");
            setInternalName("1 min Peak Pace");
        }
        void initialize () {
            setName(tr("1 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace1m(*this); }
};

class PeakPace2m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace2m)
    public:
        PeakPace2m()
        {
            setSecs(120);
            setSymbol("2m_critical_pace");
            setInternalName("2 min Peak Pace");
        }
        void initialize () {
            setName(tr("2 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace2m(*this); }
};

class PeakPace3m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace3m)
    public:
        PeakPace3m()
        {
            setSecs(180);
            setSymbol("3m_critical_pace");
            setInternalName("3 min Peak Pace");
        }
        void initialize () {
            setName(tr("3 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace3m(*this); }
};

class PeakPace5m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace5m)
    public:
        PeakPace5m()
        {
            setSecs(300);
            setSymbol("5m_critical_pace");
            setInternalName("5 min Peak Pace");
        }
        void initialize () {
            setName(tr("5 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace5m(*this); }
};

class PeakPace8m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace8m)
    public:
        PeakPace8m()
        {
            setSecs(8*60);
            setSymbol("8m_critical_pace");
            setInternalName("8 min Peak Pace");
        }
        void initialize () {
            setName(tr("8 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace8m(*this); }
};

class PeakPace10m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace10m)
    public:
        PeakPace10m()
        {
            setSecs(600);
            setSymbol("10m_critical_pace");
            setInternalName("10 min Peak Pace");
        }
        void initialize () {
            setName(tr("10 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace10m(*this); }
};

class PeakPace20m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace20m)
    public:
        PeakPace20m()
        {
            setSecs(1200);
            setSymbol("20m_critical_pace");
            setInternalName("20 min Peak Pace");
        }
        void initialize () {
            setName(tr("20 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace20m(*this); }
};

class PeakPace30m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace30m)
    public:
        PeakPace30m()
        {
            setSecs(1800);
            setSymbol("30m_critical_pace");
            setInternalName("30 min Peak Pace");
        }
        void initialize () {
            setName(tr("30 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace30m(*this); }
};

class PeakPace60m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace60m)
    public:
        PeakPace60m()
        {
            setSecs(3600);
            setSymbol("60m_critical_pace");
            setInternalName("60 min Peak Pace");
        }
        void initialize () {
            setName(tr("60 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace60m(*this); }
};

class PeakPace90m : public PeakPace {
    Q_DECLARE_TR_FUNCTIONS(PeakPace90m)
    public:
        PeakPace90m()
        {
            setSecs(90*60);
            setSymbol("90m_critical_pace");
            setInternalName("90 min Peak Pace");
        }
        void initialize () {
            setName(tr("90 min Peak Pace"));
            setMetricUnits(tr("min/km"));
            setImperialUnits(tr("min/mile"));
            setPrecision(1);
            setConversion(KM_PER_MILE);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPace90m(*this); }
};

class PeakPaceSwim : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim)
    double pace;
    double secs;

    public:

    PeakPaceSwim() : pace(0.0), secs(0.0)
    {
        setType(RideMetric::Low);
    }
    // Swim Pace ordering is reversed
    bool isLowerBetter() const { return true; }
    // Overrides to use Swim Pace units setting
    QString units(bool) const {
        bool metricSwimPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::units(metricSwimPace);
    }
    double value(bool) const {
        bool metricSwimPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(metricSwimPace);
    }
    double value(double v, bool) const {
        bool metricSwimPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(v, metricSwimPace);
    }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60, true);
    }
    QString toString(double v) const {
        return time_to_string(v*60, true);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->isSwim) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        QList<AddIntervalDialog::AddedInterval> results;
        AddIntervalDialog::findPeaks(item->context, true, item->ride(), spec, RideFile::kph, RideFile::original, secs, 1, results, "", "");
        if (results.count() > 0 && results.first().avg > 0 && results.first().avg < 9) pace = 6.0 / results.first().avg;
        else pace = 0.0;
        setValue(pace);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakPaceSwim(*this); }
};

class PeakPaceSwim10s : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim10s)
    public:
        PeakPaceSwim10s()
        {
            setSecs(10);
            setSymbol("10s_critical_pace_swim");
            setInternalName("10 sec Peak Pace Swim");
        }
        void initialize () {
            setName(tr("10 sec Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim10s(*this); }
};

class PeakPaceSwim15s : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim15s)
    public:
        PeakPaceSwim15s()
        {
            setSecs(15);
            setSymbol("15s_critical_pace_swim");
            setInternalName("15 sec Peak Pace Swim");
        }
        void initialize () {
            setName(tr("15 sec Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim15s(*this); }
};

class PeakPaceSwim20s : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim20s)
    public:
        PeakPaceSwim20s()
        {
            setSecs(20);
            setSymbol("20s_critical_pace_swim");
            setInternalName("20 sec Peak Pace Swim");
        }
        void initialize () {
            setName(tr("20 sec Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim20s(*this); }
};

class PeakPaceSwim30s : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim30s)
    public:
        PeakPaceSwim30s()
        {
            setSecs(30);
            setSymbol("30s_critical_pace_swim");
            setInternalName("30 sec Peak Pace Swim");
        }
        void initialize () {
            setName(tr("30 sec Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim30s(*this); }
};

class PeakPaceSwim1m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim1m)
    public:
        PeakPaceSwim1m()
        {
            setSecs(60);
            setSymbol("1m_critical_pace_swim");
            setInternalName("1 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("1 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim1m(*this); }
};

class PeakPaceSwim2m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim2m)
    public:
        PeakPaceSwim2m()
        {
            setSecs(120);
            setSymbol("2m_critical_pace_swim");
            setInternalName("2 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("2 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim2m(*this); }
};

class PeakPaceSwim3m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim3m)
    public:
        PeakPaceSwim3m()
        {
            setSecs(180);
            setSymbol("3m_critical_pace_swim");
            setInternalName("3 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("3 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim3m(*this); }
};

class PeakPaceSwim5m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim5m)
    public:
        PeakPaceSwim5m()
        {
            setSecs(300);
            setSymbol("5m_critical_pace_swim");
            setInternalName("5 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("5 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim5m(*this); }
};

class PeakPaceSwim8m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim8m)
    public:
        PeakPaceSwim8m()
        {
            setSecs(8*60);
            setSymbol("8m_critical_pace_swim");
            setInternalName("8 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("8 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim8m(*this); }
};

class PeakPaceSwim10m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim10m)
    public:
        PeakPaceSwim10m()
        {
            setSecs(600);
            setSymbol("10m_critical_pace_swim");
            setInternalName("10 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("10 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim10m(*this); }
};

class PeakPaceSwim20m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim20m)
    public:
        PeakPaceSwim20m()
        {
            setSecs(1200);
            setSymbol("20m_critical_pace_swim");
            setInternalName("20 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("20 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim20m(*this); }
};

class PeakPaceSwim30m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim30m)
    public:
        PeakPaceSwim30m()
        {
            setSecs(1800);
            setSymbol("30m_critical_pace_swim");
            setInternalName("30 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("30 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim30m(*this); }
};

class PeakPaceSwim60m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim60m)
    public:
        PeakPaceSwim60m()
        {
            setSecs(3600);
            setSymbol("60m_critical_pace_swim");
            setInternalName("60 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("60 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim60m(*this); }
};

class PeakPaceSwim90m : public PeakPaceSwim {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceSwim90m)
    public:
        PeakPaceSwim90m()
        {
            setSecs(90*60);
            setSymbol("90m_critical_pace_swim");
            setInternalName("90 min Peak Pace Swim");
        }
        void initialize () {
            setName(tr("90 min Peak Pace Swim"));
            setMetricUnits(tr("min/100m"));
            setImperialUnits(tr("min/100yd"));
            setPrecision(1);
            setConversion(METERS_PER_YARD);
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceSwim90m(*this); }
};

class BestTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(BestTime)
    double meters;
    double secs;

    public:

    BestTime() : meters(0.0)
    {
        setType(RideMetric::Low);
    }
    // BestTime ordering is reversed
    bool isLowerBetter() const { return true; }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60, true);
    }
    QString toString(double v) const {
        return time_to_string(v*60, true);
    }
    void setMeters(double meters) { this->meters=meters; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        QList<AddIntervalDialog::AddedInterval> results;
        AddIntervalDialog::findPeaks(item->context, false, item->ride(), spec, RideFile::kph, RideFile::original, meters, 1, results, "", "");
        if (results.count() > 0) secs = results.first().stop - results.first().start;
        else secs = 0.0;

        setValue(secs / 60.0);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("S"); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new BestTime(*this); }
};

class BestTime50m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime50m)
    public:
        BestTime50m()
        {
            setMeters(50);
            setSymbol("best_50m");
            setInternalName("Best 50m");
        }
        void initialize () {
            setName(tr("Best 50m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime50m(*this); }
};

class BestTime100m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime100m)
    public:
        BestTime100m()
        {
            setMeters(100);
            setSymbol("best_100m");
            setInternalName("Best 100m");
        }
        void initialize () {
            setName(tr("Best 100m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime100m(*this); }
};

class BestTime200m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime200m)
    public:
        BestTime200m()
        {
            setMeters(200);
            setSymbol("best_200m");
            setInternalName("Best 200m");
        }
        void initialize () {
            setName(tr("Best 200m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime200m(*this); }
};

class BestTime400m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime400m)
    public:
        BestTime400m()
        {
            setMeters(400);
            setSymbol("best_400m");
            setInternalName("Best 400m");
        }
        void initialize () {
            setName(tr("Best 400m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime400m(*this); }
};

class BestTime500m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime500m)
    public:
        BestTime500m()
        {
            setMeters(500);
            setSymbol("best_500m");
            setInternalName("Best 500m");
        }
        void initialize () {
            setName(tr("Best 500m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime500m(*this); }
};

class BestTime800m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime800m)
    public:
        BestTime800m()
        {
            setMeters(800);
            setSymbol("best_800m");
            setInternalName("Best 800m");
        }
        void initialize () {
            setName(tr("Best 800m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime800m(*this); }
};

class BestTime1000m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime1000m)
    public:
        BestTime1000m()
        {
            setMeters(1000);
            setSymbol("best_1000m");
            setInternalName("Best 1000m");
        }
        void initialize () {
            setName(tr("Best 1000m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime1000m(*this); }
};

class BestTime1500m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime1500m)
    public:
        BestTime1500m()
        {
            setMeters(1500);
            setSymbol("best_1500m");
            setInternalName("Best 1500m");
        }
        void initialize () {
            setName(tr("Best 1500m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime1500m(*this); }
};

class BestTime2000m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime2000m)
    public:
        BestTime2000m()
        {
            setMeters(2000);
            setSymbol("best_2000m");
            setInternalName("Best 2000m");
        }
        void initialize () {
            setName(tr("Best 2000m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime2000m(*this); }
};

class BestTime3000m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime3000m)
    public:
        BestTime3000m()
        {
            setMeters(3000);
            setSymbol("best_3000m");
            setInternalName("Best 3000m");
        }
        void initialize () {
            setName(tr("Best 3000m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime3000m(*this); }
};

class BestTime4000m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime4000m)
    public:
        BestTime4000m()
        {
            setMeters(4000);
            setSymbol("best_4000m");
            setInternalName("Best 4000m");
        }
        void initialize () {
            setName(tr("Best 4000m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime4000m(*this); }
};

class BestTime5000m : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime5000m)
    public:
        BestTime5000m()
        {
            setMeters(5000);
            setSymbol("best_5000m");
            setInternalName("Best 5000m");
        }
        void initialize () {
            setName(tr("Best 5000m"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime5000m(*this); }
};

class BestTime10km : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime10km)
    public:
        BestTime10km()
        {
            setMeters(10000);
            setSymbol("best_10km");
            setInternalName("Best 10km");
        }
        void initialize () {
            setName(tr("Best 10km"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime10km(*this); }
};

class BestTime15km : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime15km)
    public:
        BestTime15km()
        {
            setMeters(15000);
            setSymbol("best_15km");
            setInternalName("Best 15km");
        }
        void initialize () {
            setName(tr("Best 15km"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime15km(*this); }
};

class BestTime20km : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime20km)
    public:
        BestTime20km()
        {
            setMeters(20000);
            setSymbol("best_20km");
            setInternalName("Best 20km");
        }
        void initialize () {
            setName(tr("Best 20km"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime20km(*this); }
};

class BestTimeHalfMarathon : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTimeHalfMarathon)
    public:
        BestTimeHalfMarathon()
        {
            setMeters(21097.5);
            setSymbol("best_half_marathon");
            setInternalName("Best Half Marathon");
        }
        void initialize () {
            setName(tr("Best Half Marathon"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTimeHalfMarathon(*this); }
};

class BestTime30km : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime30km)
    public:
        BestTime30km()
        {
            setMeters(30000);
            setSymbol("best_30km");
            setInternalName("Best 30km");
        }
        void initialize () {
            setName(tr("Best 30km"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime30km(*this); }
};

class BestTime40km : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTime40km)
    public:
        BestTime40km()
        {
            setMeters(40000);
            setSymbol("best_40km");
            setInternalName("Best 40km");
        }
        void initialize () {
            setName(tr("Best 40km"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTime40km(*this); }
};

class BestTimeMarathon : public BestTime {
    Q_DECLARE_TR_FUNCTIONS(BestTimeMarathon)
    public:
        BestTimeMarathon()
        {
            setMeters(42195);
            setSymbol("best_Marathon");
            setInternalName("Best Marathon");
        }
        void initialize () {
            setName(tr("Best Marathon"));
            setMetricUnits(tr("minutes"));
            setImperialUnits(tr("minutes"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new BestTimeMarathon(*this); }
};

class PeakPaceHr : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceHr)

    double hr;
    double secs;

    public:

    PeakPaceHr() : hr(0.0), secs(0.0)
    {
        setType(RideMetric::Peak);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples or not a run nor a swim
        if (spec.isEmpty(item->ride()) || !(item->isRun || item->isSwim)) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // find peak pace interval
        QList<AddIntervalDialog::AddedInterval> results;
        AddIntervalDialog::findPeaks(item->context, true, item->ride(), spec, RideFile::kph, RideFile::original, secs, 1, results, "", "");

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
    bool isRelevantForRide(const RideItem *ride) const { return ride->isRun || ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PeakPaceHr(*this); }
};

class PeakPaceHr1m : public PeakPaceHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceHr1m)

    public:
        PeakPaceHr1m()
        {
            setSecs(60);
            setSymbol("1m_critical_pace_hr");
            setInternalName("1 min Peak Pace HR");
        }
        void initialize () {
            setName(tr("1 min Peak Pace HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 1 min Peak Pace interval"));
        }
        MetricClass classification() const { return Undefined; }
        MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceHr1m(*this); }
};

class PeakPaceHr5m : public PeakPaceHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceHr5m)

    public:
        PeakPaceHr5m()
        {
            setSecs(300);
            setSymbol("5m_critical_pace_hr");
            setInternalName("5 min Peak Pace HR");
        }
        void initialize () {
            setName(tr("5 min Peak Pace HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 5 min Peak Pace interval"));
        }
        MetricClass classification() const { return Undefined; }
        MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceHr5m(*this); }
};

class PeakPaceHr10m : public PeakPaceHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceHr10m)

    public:
        PeakPaceHr10m()
        {
            setSecs(600);
            setSymbol("10m_critical_pace_hr");
            setInternalName("10 min Peak Pace HR");
        }
        void initialize () {
            setName(tr("10 min Peak Pace HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 10 min Peak Pace interval"));
        }
        MetricClass classification() const { return Undefined; }
        MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceHr10m(*this); }
};

class PeakPaceHr20m : public PeakPaceHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceHr20m)

    public:
        PeakPaceHr20m()
        {
            setSecs(1200);
            setSymbol("20m_critical_pace_hr");
            setInternalName("20 min Peak Pace HR");
        }
        void initialize () {
            setName(tr("20 min Peak Pace HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 20 min Peak Pace interval"));
        }
        MetricClass classification() const { return Undefined; }
        MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceHr20m(*this); }
};

class PeakPaceHr30m : public PeakPaceHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceHr30m)

    public:
        PeakPaceHr30m()
        {
            setSecs(1800);
            setSymbol("30m_critical_pace_hr");
            setInternalName("30 min Peak Pace HR");
        }
        void initialize () {
            setName(tr("30 min Peak Pace HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 30 min Peak Pace interval"));
        }
        MetricClass classification() const { return Undefined; }
        MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceHr30m(*this); }
};

class PeakPaceHr60m : public PeakPaceHr {
    Q_DECLARE_TR_FUNCTIONS(PeakPaceHr30m)

    public:
        PeakPaceHr60m()
        {
            setSecs(3600);
            setSymbol("60m_critical_pace_hr");
            setInternalName("60 min Peak Pace HR");
        }
        void initialize () {
            setName(tr("60 min Peak Pace HR"));
            setMetricUnits(tr("bpm"));
            setImperialUnits(tr("bpm"));
            setDescription(tr("Average Heart Rate for 60 min Peak Pace interval"));
        }
        MetricClass classification() const { return Undefined; }
        MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new PeakPaceHr60m(*this); }
};


static bool addAllPacePeaks() {
    RideMetricFactory::instance().addMetric(PeakPace10s());
    RideMetricFactory::instance().addMetric(PeakPace15s());
    RideMetricFactory::instance().addMetric(PeakPace20s());
    RideMetricFactory::instance().addMetric(PeakPace30s());
    RideMetricFactory::instance().addMetric(PeakPace1m());
    RideMetricFactory::instance().addMetric(PeakPace2m());
    RideMetricFactory::instance().addMetric(PeakPace3m());
    RideMetricFactory::instance().addMetric(PeakPace5m());
    RideMetricFactory::instance().addMetric(PeakPace8m());
    RideMetricFactory::instance().addMetric(PeakPace10m());
    RideMetricFactory::instance().addMetric(PeakPace20m());
    RideMetricFactory::instance().addMetric(PeakPace30m());
    RideMetricFactory::instance().addMetric(PeakPace60m());
    RideMetricFactory::instance().addMetric(PeakPace90m());

    RideMetricFactory::instance().addMetric(PeakPaceSwim10s());
    RideMetricFactory::instance().addMetric(PeakPaceSwim15s());
    RideMetricFactory::instance().addMetric(PeakPaceSwim20s());
    RideMetricFactory::instance().addMetric(PeakPaceSwim30s());
    RideMetricFactory::instance().addMetric(PeakPaceSwim1m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim2m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim3m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim5m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim8m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim10m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim20m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim30m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim60m());
    RideMetricFactory::instance().addMetric(PeakPaceSwim90m());

    RideMetricFactory::instance().addMetric(BestTime50m());
    RideMetricFactory::instance().addMetric(BestTime100m());
    RideMetricFactory::instance().addMetric(BestTime200m());
    RideMetricFactory::instance().addMetric(BestTime400m());
    RideMetricFactory::instance().addMetric(BestTime500m());
    RideMetricFactory::instance().addMetric(BestTime800m());
    RideMetricFactory::instance().addMetric(BestTime1000m());
    RideMetricFactory::instance().addMetric(BestTime1500m());
    RideMetricFactory::instance().addMetric(BestTime2000m());
    RideMetricFactory::instance().addMetric(BestTime3000m());
    RideMetricFactory::instance().addMetric(BestTime4000m());
    RideMetricFactory::instance().addMetric(BestTime5000m());
    RideMetricFactory::instance().addMetric(BestTime10km());
    RideMetricFactory::instance().addMetric(BestTime15km());
    RideMetricFactory::instance().addMetric(BestTime20km());
    RideMetricFactory::instance().addMetric(BestTimeHalfMarathon());
    RideMetricFactory::instance().addMetric(BestTime30km());
    RideMetricFactory::instance().addMetric(BestTime40km());
    RideMetricFactory::instance().addMetric(BestTimeMarathon());

    RideMetricFactory::instance().addMetric(PeakPaceHr1m());
    RideMetricFactory::instance().addMetric(PeakPaceHr5m());
    RideMetricFactory::instance().addMetric(PeakPaceHr10m());
    RideMetricFactory::instance().addMetric(PeakPaceHr20m());
    RideMetricFactory::instance().addMetric(PeakPaceHr30m());
    RideMetricFactory::instance().addMetric(PeakPaceHr60m());

    return true;
}

static bool allPacePeaksAdded = addAllPacePeaks();
