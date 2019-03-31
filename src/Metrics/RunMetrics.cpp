/*
 * Copyright (c) 2016 Damien Grauser (Damien.Grauser@gmail.com)
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
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "RideItem.h"
#include "RideFile.h"
#include "LTMOutliers.h"
#include "Units.h"
#include "Zones.h"
#include "cmath"
#include <assert.h>
#include <algorithm>
#include <QVector>
#include <QApplication>

struct AvgRunCadence : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgRunCadence)

    double total, count;

    public:

    AvgRunCadence()
    {
        setSymbol("average_run_cad");
        setInternalName("Average Running Cadence");
    }

    void initialize() {
        setName(tr("Average Running Cadence"));
        setMetricUnits(tr("spm"));
        setImperialUnits(tr("spm"));
        setType(RideMetric::Average);
        setDescription(tr("Average Running Cadence, computed when Cadence > 0"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->cad > 0) {
                total += point->cad;
                ++count;
            } else if (point->rcad > 0) {
                total += point->rcad;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("C") && ride->isRun); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AvgRunCadence(*this); }
};

static bool avgRunCadenceAdded =
    RideMetricFactory::instance().addMetric(AvgRunCadence());

//////////////////////////////////////////////////////////////////////////////

class MaxRunCadence : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxRunCadence)
    public:

    MaxRunCadence()
    {
        setSymbol("max_run_cadence");
        setInternalName("Max Running Cadence");
    }

    void initialize() {
        setName(tr("Max Running Cadence"));
        setMetricUnits(tr("spm"));
        setImperialUnits(tr("spm"));
        setType(RideMetric::Peak);
        setDescription(tr("Maximum Running Cadence"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double max = 0.0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->cad > max) max = point->cad;
            if (point->rcad > max) max = point->rcad;
        }

        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C") && ride->isRun; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new MaxRunCadence(*this); }
};

static bool maxRunCadenceAdded =
    RideMetricFactory::instance().addMetric(MaxRunCadence());

//////////////////////////////////////////////////////////////////////////////

struct AvgRunGroundContactTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgRunGroundContactTime)

    double total, count;

    public:

    AvgRunGroundContactTime()
    {
        setSymbol("average_run_ground_contact");
        setInternalName("Average Ground Contact Time");
    }

    void initialize() {
        setName(tr("Average Ground Contact Time"));
        setMetricUnits(tr("ms"));
        setImperialUnits(tr("ms"));
        setType(RideMetric::Average);
        setDescription(tr("Average Ground Contact Time"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->rcontact > 0) {
                total += point->rcontact;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("R") && ride->isRun); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AvgRunGroundContactTime(*this); }
};

static bool avgRunGroundContactTimeAdded =
    RideMetricFactory::instance().addMetric(AvgRunGroundContactTime());

//////////////////////////////////////////////////////////////////////////////

struct AvgRunVerticalOscillation  : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgRunVerticalOscillation)

    double total, count;

    public:

    AvgRunVerticalOscillation()
    {
        setSymbol("average_run_vert_oscillation");
        setInternalName("Average Vertical Oscillation");
    }

    void initialize() {
        setName(tr("Average Vertical Oscillation"));
        setMetricUnits(tr("cm"));
        setImperialUnits(tr("in"));
        setPrecision(1);
        setConversion(INCH_PER_CM);
        setType(RideMetric::Average);
        setDescription(tr("Average Vertical Oscillation"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->rvert > 0) {
                total += point->rvert;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("R") && ride->isRun); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AvgRunVerticalOscillation(*this); }
};

static bool avgRunVerticalOscillationAdded =
    RideMetricFactory::instance().addMetric(AvgRunVerticalOscillation());

//////////////////////////////////////////////////////////////////////////////

class Pace : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(Pace)
    double pace;

    public:

    Pace() : pace(0.0)
    {
        setSymbol("pace");
        setInternalName("Pace");
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
        return time_to_string(value(metric)*60, true);
    }

    void initialize() {
        setName(tr("Pace"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/km"));
        setImperialUnits(tr("min/mile"));
        setPrecision(1);
        setConversion(KM_PER_MILE);
        setDescription(tr("Average Speed expressed in pace units: min/km or min/mile"));
   }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        RideMetric *as = deps.value("average_speed");

        // divide by zero or stupidly low pace
        if (as->value(true) > 0.00f) pace = 60.0f / as->value(true);
        else pace = 0;

        setValue(pace);
        setCount(as->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->isRun; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new Pace(*this); }
};

static bool addPace()
{
    QVector<QString> deps;
    deps.append("average_speed");
    RideMetricFactory::instance().addMetric(Pace(), &deps);
    return true;
}
static bool paceAdded = addPace();

//////////////////////////////////////////////////////////////////////////////

class EfficiencyIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(EfficiencyIndex)

    public:

    EfficiencyIndex()
    {
        setSymbol("efficiency_index");
        setInternalName("Efficiency Index");
    }

    void initialize() {
        setName(tr("Efficiency Index"));
        setType(RideMetric::Average);
        setMetricUnits("");
        setPrecision(2);
        setDescription(tr("Efficiency Index : average speed by average power"));
   }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        double avg_power = deps.value("average_power")->value(true);
        double avg_speed = deps.value("average_speed")->value(true);

        double workout_time = deps.value("workout_time")->value(true);
        double time_moving = deps.value("time_riding")->value(true);

        double ei=0;
        if (avg_power > 0.00f && workout_time > 0.00f)
            ei = 1000 * avg_speed * time_moving / 60.0 / avg_power / workout_time;

        setValue(ei);
        setCount(1);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->isRun && ride->present.contains("P") && ride->present.contains("S"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new EfficiencyIndex(*this); }
};

static bool addEfficiencyIndex()
{
    QVector<QString> deps;
    deps.append("average_power");
    deps.append("average_speed");
    deps.append("workout_time");
    deps.append("time_riding");
    RideMetricFactory::instance().addMetric(EfficiencyIndex(), &deps);
    return true;
}
static bool efficiencyIndexAdded = addEfficiencyIndex();

//////////////////////////////////////////////////////////////////////////////

struct AvgStrideLength  : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgStrideLength)

    double total, count;

    public:

    AvgStrideLength()
    {
        setSymbol("average_stride_length");
        setInternalName("Average Stride Length");
    }

    void initialize() {
        setName(tr("Average Stride Length"));
        setMetricUnits(tr("m"));
        setImperialUnits(tr("ft"));
        setPrecision(2);
        setConversion(FEET_PER_METER);
        setType(RideMetric::Average);
        setDescription(tr("Average Stride Length"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->clength > 0) {
                total += point->clength;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("S") && ride->present.contains("C") && ride->isRun); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AvgStrideLength(*this); }
};

static bool avgStrideLengthAdded =
    RideMetricFactory::instance().addMetric(AvgStrideLength());

//////////////////////////////////////////////////////////////////////////////

struct AvgRunVerticalRatio  : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgRunVerticalRatio)

    double total, count;

    public:

    AvgRunVerticalRatio()
    {
        setSymbol("average_run_vert_ratio");
        setInternalName("Average Vertical Ratio");
    }

    void initialize() {
        setName(tr("Average Vertical Ratio"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setPrecision(1);
        setType(RideMetric::Average);
        setDescription(tr("Average Vertical Ratio (%): Vertical Oscillation / Step Length"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        int idx = 0;
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            double vert_ratio = item->ride()->xdataValue(point, idx, "EXTRA", "VERTICALRATIO", RideFile::SPARSE);
            if (point->rcad > 0 && vert_ratio > 0) {
                total += vert_ratio;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("R") && ride->isRun); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AvgRunVerticalRatio(*this); }
};

static bool avgRunVerticalRatio =
    RideMetricFactory::instance().addMetric(AvgRunVerticalRatio());

//////////////////////////////////////////////////////////////////////////////

struct AvgRunStanceTimePercent  : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgRunStanceTimePercent)

    double total, count;

    public:

    AvgRunStanceTimePercent()
    {
        setSymbol("average_run_stance_time_percent");
        setInternalName("Average Stance Time Percent");
    }

    void initialize() {
        setName(tr("Average Stance Time Percent"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setPrecision(1);
        setType(RideMetric::Average);
        setDescription(tr("Average Stance Time Percent (%): Ground Contact Time / Step Time"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        int idx = 0;
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            double stance_time_pct = item->ride()->xdataValue(point, idx, "EXTRA", "STANCETIMEPERCENT", RideFile::SPARSE);
            if (point->rcad > 0 && stance_time_pct > 0) {
                total += stance_time_pct;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("R") && ride->isRun); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AvgRunStanceTimePercent(*this); }
};

static bool avgRunStanceTimePercent =
    RideMetricFactory::instance().addMetric(AvgRunStanceTimePercent());

//////////////////////////////////////////////////////////////////////////////

struct AvgStepLength  : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgStepLength)

    double total, count;

    public:

    AvgStepLength()
    {
        setSymbol("average_step_length");
        setInternalName("Average Step Length");
    }

    void initialize() {
        setName(tr("Average Step Length"));
        setMetricUnits(tr("mm"));
        setImperialUnits(tr("mm"));
        setPrecision(0);
        setType(RideMetric::Average);
        setDescription(tr("Average Step Length: average single step (L to R / R to L) length in mm"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        int idx = 0;
        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            double step_length = item->ride()->xdataValue(point, idx, "EXTRA", "STEPLENGTH", RideFile::SPARSE);
            if (point->rcad > 0 && step_length > 0) {
                total += step_length;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("S") && ride->present.contains("C") && ride->isRun); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new AvgStepLength(*this); }
};

static bool avgStepLengthAdded =
    RideMetricFactory::instance().addMetric(AvgStepLength());

//////////////////////////////////////////////////////////////////////////////
