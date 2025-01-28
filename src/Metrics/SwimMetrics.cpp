/*
 * Copyright (c) 2016 Alejandro Martinez (amtrithlon@gmail.com)
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
#include "Units.h"
#include "cmath"
#include <assert.h>
#include <algorithm>
#include <QVector>
#include <QApplication>

// DistanceSwim is TotalDistance in swim units, relevant for swims in yards //
class DistanceSwim : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(DistanceSwim)
    double mts;

    public:

    DistanceSwim() : mts(0.0)
    {
        setSymbol("distance_swim");
        setInternalName("Distance Swim");
    }
    // Overrides to use Swim Pace units setting
    QString units(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::units(metricPace);
    }
    double value(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(metricPace);
    }
    double value(double v, bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(v, metricPace);
    }
    void initialize() {
        setName(tr("Distance Swim"));
        setType(RideMetric::Total);
        setMetricUnits(tr("m"));
        setImperialUnits(tr("yd"));
        setPrecision(0);
        setConversion(1.0/METERS_PER_YARD);
        setDescription(tr("Total Distance in meters or yards"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        RideMetric *distance = deps.value("total_distance");

        // convert to meters
        mts = distance->value(true) * 1000.0;
        setValue(mts);
        setCount(distance->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new DistanceSwim(*this); }
};

static bool addDistanceSwim()
{
    QVector<QString> deps;
    deps.append("total_distance");
    RideMetricFactory::instance().addMetric(DistanceSwim(), &deps);
    return true;
}
static bool distanceSwimAdded = addDistanceSwim();

//////////////////////////////////////////////////////////////////////////////
class PaceSwim : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PaceSwim)
    double pace;

    public:

    PaceSwim() : pace(0.0)
    {
        setSymbol("pace_swim");
        setInternalName("Pace Swim");
    }

    // Swim Pace ordering is reversed
    bool isLowerBetter() const { return true; }

    // Overrides to use Swim Pace units setting
    QString units(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::units(metricPace);
    }

    double value(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(metricPace);
    }

    double value(double v, bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(v, metricPace);
    }

    QString toString(bool metric) const {
        return time_to_string(value(metric)*60, true);
    }

    QString toString(double v) const {
        return time_to_string(v*60, true);
    }

    void initialize() {
        setName(tr("Pace Swim"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/100m"));
        setImperialUnits(tr("min/100yd"));
        setPrecision(1);
        setConversion(METERS_PER_YARD);
        setDescription(tr("Average Speed expressed in swim pace units: min/100m or min/100yd"));
   }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {
        // not a swim
        if (!item->isSwim) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        RideMetric *as = deps.value("average_speed");

        // divide by zero or stupidly low pace
        if (as->value(true) > 0.00f) pace = 6.0f / as->value(true);
        else pace = 0;

        setValue(pace);
        setCount(as->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PaceSwim(*this); }
};

static bool addPaceSwim()
{
    QVector<QString> deps;
    deps.append("average_speed");
    RideMetricFactory::instance().addMetric(PaceSwim(), &deps);
    return true;
}
static bool paceSwimAdded = addPaceSwim();

///////////////////////////////////////////////////////////////////////////////
class SwimPace : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(SwimPace)

    double total, count;

    public:

    SwimPace()
    {
        setSymbol("swim_pace");
        setInternalName("Swim Pace");
    }

    // Swim Pace ordering is reversed
    bool isLowerBetter() const { return true; }

    // Overrides to use Swim Pace units setting
    QString units(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::units(metricPace);
    }

    double value(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(metricPace);
    }

    double value(double v, bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(v, metricPace);
    }

    QString toString(bool metric) const {
        return time_to_string(value(metric)*60, true);
    }

    QString toString(double v) const {
        return time_to_string(v*60, true);
    }

    void initialize() {
        setName(tr("Swim Pace"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/100m"));
        setImperialUnits(tr("min/100yd"));
        setPrecision(1);
        setConversion(METERS_PER_YARD);
        setDescription(tr("Average Swim Pace, computed only when Cadence > 0 to avoid kick/drill lengths"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples or not a swim
        if (spec.isEmpty(item->ride()) || !item->isSwim) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->cad > 0) {
                total += point->kph;
                ++count;
            }
        }
        setValue((count > 0 && total > 0.0) ? 6.0 * count / total : 0.0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C") && ride->isSwim; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new SwimPace(*this); }
};

static bool swimPaceAdded =
    RideMetricFactory::instance().addMetric(SwimPace());

//////////////////////////////////////////////////////////////////////////////
class StrokeRate : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(StrokeRate)
    double stroke_rate;

    public:

    StrokeRate() : stroke_rate(0.0)
    {
        setSymbol("stroke_rate");
        setInternalName("Stroke Rate");
    }

    void initialize() {
        setName(tr("Stroke Rate"));
        setType(RideMetric::Average);
        setMetricUnits(tr("strokes/min"));
        setImperialUnits(tr("strokes/min"));
        setType(RideMetric::Average);
        setPrecision(0);
        setConversion(1);
        setDescription(tr("Stroke Rate in strokes/min, counting both arms for freestyle/backstroke, corrected by 3m push-off length for pool swims"));
   }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &deps) {

        // no ride or no samples or not a swim
        if (spec.isEmpty(item->ride()) || !item->isSwim) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // Pool Length from metadata, defaults to a large value for Open Water
        double pool_length = item->getText("Pool Length", "3000000").toDouble();
        // Push off length 3m
        double push_off = 3;

        // Average Cadence is cycles per minute
        RideMetric *cad = deps.value("average_cad");
        assert(cad);

        // Assume bilateral stroke s.t. crawl or backstroke
        stroke_rate = 2 * cad->value(true) / (1 - push_off / pool_length);

        setValue(stroke_rate);
        setCount(cad->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C") && ride->isSwim; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new StrokeRate(*this); }
};

static bool addStrokeRate()
{
    QVector<QString> deps;
    deps.append("average_cad");
    RideMetricFactory::instance().addMetric(StrokeRate(), &deps);
    return true;
}
static bool strokeRateAdded = addStrokeRate();

//////////////////////////////////////////////////////////////////////////////
class StrokesPerLength : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(StrokesPerLength)
    double spl;

    public:

    StrokesPerLength() : spl(0.0)
    {
        setSymbol("strokes_per_length");
        setInternalName("Strokes Per Length");
    }

    void initialize() {
        setName(tr("Strokes Per Length"));
        setType(RideMetric::Average);
        setMetricUnits(tr("strokes"));
        setImperialUnits(tr("strokes"));
        setType(RideMetric::Average);
        setPrecision(0);
        setConversion(1);
        setDescription(tr("Strokes per length, counting the arm using the watch, Pool Length defaults to 50m for open water swims"));
   }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &deps) {

        // no ride or no samples or not a swim
        if (spec.isEmpty(item->ride()) || !item->isSwim) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // Pool Length from metadata, defaults to 50m for Open Water
        double pool_length = item->getText("Pool Length", "50").toDouble();

        // Average Cadence is cycles per minute
        RideMetric *cad = deps.value("average_cad");
        assert(cad);

        // Swim Pace is average when Cadence > 0
        RideMetric *swim_pace = deps.value("swim_pace");
        assert(swim_pace);

        double strokes = cad->value(true) * (cad->count()/60.0);
        double meters_swim = swim_pace->value(true) > 0.0 ?  100 * (swim_pace->count()/60.0)/swim_pace->value(true) : 0.0;
        // Assume bilateral stroke s.t. crawl or backstroke
        spl = meters_swim > 0.0 ? strokes * pool_length / meters_swim : 0.0;

        setValue(spl);
        setCount(cad->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C") && ride->isSwim; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new StrokesPerLength(*this); }
};

static bool addStrokesPerLength()
{
    QVector<QString> deps;
    deps.append("average_cad");
    deps.append("swim_pace");
    RideMetricFactory::instance().addMetric(StrokesPerLength(), &deps);
    return true;
}
static bool strokesPerLengthAdded = addStrokesPerLength();

//////////////////////////////////////////////////////////////////////////////
class SWolf : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(SWolf)
    double swolf;

    public:

    SWolf() : swolf(0.0)
    {
        setSymbol("swolf");
        setInternalName("SWolf");
    }

    void initialize() {
        setName(tr("SWolf"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setType(RideMetric::Average);
        setPrecision(0);
        setConversion(1);
        setDescription(tr("Strokes per length, counting the arm using the watch plus time in seconds, Pool Length defaults to 50m for open water swims"));
   }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &deps) {

        // no ride or no samples or not a swim
        if (spec.isEmpty(item->ride()) || !item->isSwim) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // Pool Length from metadata, defaults to 50m for Open Water
        double pool_length = item->getText("Pool Length", "50").toDouble();

        // Average Cadence is cycles per minute
        RideMetric *cad = deps.value("average_cad");
        assert(cad);

        // Swim Pace is average when Cadence > 0
        RideMetric *swim_pace = deps.value("swim_pace");
        assert(swim_pace);

        double strokes = cad->value(true) * (cad->count()/60.0);
        double meters_swim = swim_pace->value(true) > 0.0 ?  100 * (swim_pace->count()/60.0)/swim_pace->value(true) : 0.0;
        // Assume bilateral stroke s.t. crawl or backstroke
        swolf = meters_swim > 0.0 ? (cad->count() + strokes) * pool_length / meters_swim : 0.0;

        setValue(swolf);
        setCount(cad->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C") && ride->isSwim; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new SWolf(*this); }
};

static bool addSWolf()
{
    QVector<QString> deps;
    deps.append("average_cad");
    deps.append("swim_pace");
    RideMetricFactory::instance().addMetric(SWolf(), &deps);
    return true;
}
static bool swolfAdded = addSWolf();

///////////////////////////////////////////////////////////////////////////////
class SwimPaceStroke : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(SwimPaceStroke)

    public:

    enum StrokeType { free = 1, back = 2, breast = 3, fly = 4 };

    void setStroke(StrokeType type) { strokeType = type; }

    SwimPaceStroke() : strokeType(free), total(0.0), count(0.0)
    {
        setType(RideMetric::Average);
        setPrecision(1);
        setConversion(METERS_PER_YARD);
    }

    // Swim Pace ordering is reversed
    bool isLowerBetter() const { return true; }

    // Overrides to use Swim Pace units setting
    QString units(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::units(metricPace);
    }

    double value(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(metricPace);
    }

    double value(double v, bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(v, metricPace);
    }

    QString toString(bool metric) const {
        return time_to_string(value(metric)*60, true);
    }

    QString toString(double v) const {
        return time_to_string(v*60, true);
    }

    void initialize() {
        setName(tr("Swim Pace"));
        setMetricUnits(tr("min/100m"));
        setImperialUnits(tr("min/100yd"));
        setDescription(tr("Average Swim Pace, computed only when Cadence > 0 to avoid kick/drill lengths"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        setValue(RideFile::NIL);
        setCount(0);

        // no ride or no samples or not a swim
        if (spec.isEmpty(item->ride()) || !item->isSwim)
            return;

        XDataSeries *series = item->ride()->xdata("SWIM");
        if (!series) // no SWIM specific data
            return;
        int typeIdx = -1;
        for (int a=0; a<series->valuename.count(); a++) {
            if (series->valuename.at(a) == "TYPE")
                typeIdx = a;
        }
        if (typeIdx == -1) // no Stroke Type
            return;

        int b=0, type=0;
        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            for (int j=b; j<series->datapoints.count(); j++) {
                if (series->datapoints.at(j)->secs > point->secs)
                    break;
                b=j;
                // Stroke Type
                type = series->datapoints.at(j)->number[typeIdx];
            }
            if (type == strokeType) {
                total += point->kph;
                ++count;
            }
        }
        setValue((count > 0 && total > 0.0) ? 6.0 * count / total : 0.0);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new SwimPaceStroke(*this); }

    private:

    StrokeType strokeType;
    double total, count;
};

class SwimPaceFree : public SwimPaceStroke {
    Q_DECLARE_TR_FUNCTIONS(SwimPaceFree)

    public:
        SwimPaceFree()
        {
            setStroke(free);
            setSymbol("swim_pace_free");
            setInternalName("Swim Pace Free");
        }
        void initialize ()
        {
            SwimPaceStroke::initialize();
            setName(tr("Swim Pace Free"));
            setDescription(tr("Average Swim Pace for freestyle lengths"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new SwimPaceFree(*this); }
};

class SwimPaceBack : public SwimPaceStroke {
    Q_DECLARE_TR_FUNCTIONS(SwimPaceBack)

    public:
        SwimPaceBack()
        {
            setStroke(back);
            setSymbol("swim_pace_back");
            setInternalName("Swim Pace Back");
        }
        void initialize ()
        {
            SwimPaceStroke::initialize();
            setName(tr("Swim Pace Back"));
            setDescription(tr("Average Swim Pace for backstroke lengths"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new SwimPaceBack(*this); }
};

class SwimPaceBreast : public SwimPaceStroke {
    Q_DECLARE_TR_FUNCTIONS(SwimPaceBreast)

    public:
        SwimPaceBreast()
        {
            setStroke(breast);
            setSymbol("swim_pace_breast");
            setInternalName("Swim Pace Breast");
        }
        void initialize ()
        {
            SwimPaceStroke::initialize();
            setName(tr("Swim Pace Breast"));
            setDescription(tr("Average Swim Pace for breaststroke lengths"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new SwimPaceBreast(*this); }
};

class SwimPaceFly : public SwimPaceStroke {
    Q_DECLARE_TR_FUNCTIONS(SwimPaceFly)

    public:
        SwimPaceFly()
        {
            setStroke(fly);
            setSymbol("swim_pace_fly");
            setInternalName("Swim Pace Fly");
        }
        void initialize ()
        {
            SwimPaceStroke::initialize();
            setName(tr("Swim Pace Fly"));
            setDescription(tr("Average Swim Pace for butterfly lengths"));
        }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
        RideMetric *clone() const { return new SwimPaceFly(*this); }
};

static bool addAllStrokePace() {
    RideMetricFactory::instance().addMetric(SwimPaceFree());
    RideMetricFactory::instance().addMetric(SwimPaceBack());
    RideMetricFactory::instance().addMetric(SwimPaceBreast());
    RideMetricFactory::instance().addMetric(SwimPaceFly());
    return true;
}

static bool allStrokePaceAdded = addAllStrokePace();

///////////////////////////////////////////////////////////////////////////////
class SwimStroke : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(SwimStroke)

    public:

    enum StrokeType { na = -1, rest = 0, free = 1, back = 2, breast = 3, fly = 4, drill = 5, mixed = 6 };

    SwimStroke() : strokeType(na)
    {
        setType(RideMetric::Peak); // TODO: We need a different type for this metric with aggregateWith override
        setPrecision(0);
        setSymbol("swim_stroke");
        setInternalName("Swim Stroke");
    }

    QString toString(bool) const {
        return toString(value(true));
    }

    QString toString(double type) const {
        switch (static_cast<StrokeType>((int)type)) {
            case rest   : return tr("rest");
            case free   : return tr("free");
            case back   : return tr("back");
            case breast : return tr("breast");
            case fly    : return tr("fly");
            case drill  : return tr("drill");
            case mixed  : return tr("mixed");
            default     : return tr("na");
        }
    }

    void initialize() {
        setName(tr("Swim Stroke"));
        setMetricUnits(tr("Stroke"));
        setImperialUnits(tr("Stroke"));
        setDescription(tr("Swim Stroke: 0 - rest, 1 - free, 2 - back, 3 - breast, 4 - fly, 5 - drill, 6 - mixed"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        setValue(na);

        // no ride or no samples or not a swim
        if (spec.isEmpty(item->ride()) || !item->isSwim)
            return;

        XDataSeries *series = item->ride()->xdata("SWIM");
        if (!series) // no SWIM specific data
            return;
        int typeIdx = -1;
        for (int a=0; a<series->valuename.count(); a++) {
            if (series->valuename.at(a) == "TYPE")
                typeIdx = a;
        }
        if (typeIdx == -1) // no Stroke Type
            return;

        int b=0;
        StrokeType type=na;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            for (int j=b; j<series->datapoints.count(); j++) {
                if (series->datapoints.at(j)->secs > point->secs)
                    break;
                b=j;
                // Stroke Type
                type = static_cast<StrokeType>((int)series->datapoints.at(j)->number[typeIdx]);
            }
            if (strokeType < 0) {
                strokeType = type; // first length indicates stroke
            } else if (strokeType != type && type != rest) {
                strokeType = mixed; // anything different other than rest indicates mixed
            }
        }
        setValue(strokeType);
    }

    void aggregateWith(const RideMetric &other) override {
        // anything different than the first one, other than rest, indicates mixed
        if (value(true) != other.value(true) && other.value(true) != rest) setValue(mixed);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new SwimStroke(*this); }

    private:

    StrokeType strokeType;
};

static bool addSwimStroke() {
    RideMetricFactory::instance().addMetric(SwimStroke());
    return true;
}

static bool swimStrokeAdded = addSwimStroke();
