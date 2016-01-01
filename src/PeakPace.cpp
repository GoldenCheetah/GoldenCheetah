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
#include "BestIntervalDialog.h"
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
        bool metricRunPace = appsettings->value(NULL, GC_PACE, true).toBool();
        return RideMetric::units(metricRunPace);
    }
    double value(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_PACE, true).toBool();
        return RideMetric::value(metricRunPace);
    }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->isRun) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        QList<BestIntervalDialog::BestInterval> results;
        BestIntervalDialog::findBestsKPH(item->ride(), spec, secs, 1, results);
        if (results.count() > 0 && results.first().avg > 0 && results.first().avg < 36) pace = 60.0 / results.first().avg;
        else pace = 0.0;

        setValue(pace);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isRun; }
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
        bool metricSwimPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::units(metricSwimPace);
    }
    double value(bool) const {
        bool metricSwimPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::value(metricSwimPace);
    }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->isSwim) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        QList<BestIntervalDialog::BestInterval> results;
        BestIntervalDialog::findBestsKPH(item->ride(), spec, secs, 1, results);
        if (results.count() > 0 && results.first().avg > 0 && results.first().avg < 9) pace = 6.0 / results.first().avg;
        else pace = 0.0;
        setValue(pace);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }
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
        RideMetric *clone() const { return new PeakPaceSwim90m(*this); }
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
    return true;
}

static bool allPacePeaksAdded = addAllPacePeaks();
