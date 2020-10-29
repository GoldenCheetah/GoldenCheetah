/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
 *               2010 Mark Liversedge (liversedge@gmail.com)
 *               2014 Alejandro Martinez (amtriathlon@gmail.com)
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
#include "PaceZones.h"
#include "Units.h"
#include "Settings.h"
#include "RideItem.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include <cmath>
#include <assert.h>
#include <algorithm>
#include <QApplication>

// NOTE: This code follows the description of SwimScore in 
// "Calculating Power Output and Training Stress in Swimmers:
// The Development of the SwimScoreTM Algorithm", by Dr. Phil Skiba:
// http://www.physfarm.com/swimscore.pdf

// Swimming Power from Speed
static inline double swimming_power( double weight, double speed ) {
    const double K = 0.35 * weight + 2; // Drag Factor (Eq. 6)
    const double ep = 0.6; // Toussaint�s propelling efficiency

    return (K / ep) * pow(speed, 3); // Eq. 5
}

// Swimming Speed from Power
static inline double swimming_speed( double weight, double power ) {
    const double K = 0.35 * weight + 2; // Drag Factor (Eq. 6)
    const double ep = 0.6; // Toussaint�s propelling efficiency

    return pow((ep / K) * power, 1/3.0); // Eq. 5
}

// XPowerSwim, used for SwimScore and xPaceSwim calculation
class XPowerSwim : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(XPowerSwim)
    double xpower;
    double secs;

    public:

    XPowerSwim() : xpower(0.0), secs(0.0)
    {
        setSymbol("swimscore_xpower");
        setInternalName("xPower Swim");
    }
    void initialize() {
        setName(tr("xPower Swim"));
        setType(RideMetric::Average);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setDescription(tr("Swimming power normalized for variations in speed as defined by Dr. Skiba in the SwimScore algorithm"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) ||
            // xPowerSwim only makes sense for running and it needs recIntSecs > 0
            !item->isSwim || item->ride()->recIntSecs() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double weight = item->getWeight();

        static const double EPSILON = 0.1;
        static const double NEGLIGIBLE = 0.1;

        double secsDelta = item->ride()->recIntSecs();
        double sampsPerWindow = 25.0 / secsDelta;
        double attenuation = sampsPerWindow / (sampsPerWindow + secsDelta);
        double sampleWeight = secsDelta / (sampsPerWindow + secsDelta);

        double lastSecs = 0.0;
        double weighted = 0.0;

        double total = 0.0;
        int count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            while ((weighted > NEGLIGIBLE)
                   && (point->secs > lastSecs + secsDelta + EPSILON)) {
                weighted *= attenuation;
                lastSecs += secsDelta;
                total += pow(weighted, 3.0);
                count++;
            }
            weighted *= attenuation;
            weighted += sampleWeight * swimming_power(weight, point->kph/3.6);
            lastSecs = point->secs;
            total += pow(weighted, 3.0);
            count++;
        }
        xpower = count ? pow(total / count, 1/3.0) : 0.0;
        secs = count * secsDelta;

        setValue(xpower);
        setCount(secs);
    }
    bool isRelevantForRide(const RideItem*ride) const { return ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new XPowerSwim(*this); }
};

// xPaceSwim: constant Pace which requires the same xPowerSwim
class XPaceSwim : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(XPaceSwim)
    double xPaceSwim;

    public:

    XPaceSwim() : xPaceSwim(0.0)
    {
        setSymbol("swimscore_xpace");
        setInternalName("xPace Swim");
    }
    // Swim Pace ordering is reversed
    bool isLowerBetter() const { return true; }
    // Overrides to use Swim Pace units setting
    QString units(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::units(metricPace);
    }
    double value(bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::value(metricPace);
    }
    double value(double v, bool) const {
        bool metricPace = appsettings->value(NULL, GC_SWIMPACE, true).toBool();
        return RideMetric::value(v, metricPace);
    }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60);
    }
    QString toString(double v) const {
        return time_to_string(v*60);
    }
    void initialize() {
        setName(tr("xPace Swim"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/100m"));
        setImperialUnits(tr("min/100yd"));
        setPrecision(1);
        setConversion(METERS_PER_YARD);
        setDescription(tr("Iso Swim Pace in min/100m or min/100yd, defined as the constant pace which requires the same xPowerSwim"));
    }


    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // xPowerSwim only makes sense for running and it needs recIntSecs > 0
        if (!item->isSwim || item->ride()->recIntSecs() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double weight = item->getWeight();

        assert(deps.contains("swimscore_xpower"));
        XPowerSwim *xPowerSwim = dynamic_cast<XPowerSwim*>(deps.value("swimscore_xpower"));
        assert(xPowerSwim);
        double watts = xPowerSwim->value(true);

        double speed = swimming_speed(weight, watts);
        
        xPaceSwim = speed ? (100.0/60.0) / speed : 0.0;

        setValue(xPaceSwim);
        setCount(xPowerSwim->count());
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new XPaceSwim(*this); }
};

// Swimming Threshold Power based on CV, used for SwimScore calculation
class STP : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(STP)

    public:

    STP()
    {
        setSymbol("swimscore_tp");
        setInternalName("STP");
    }
    void initialize() {
        setName(tr("STP"));
        setType(RideMetric::Average);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setPrecision(0);
        setDescription(tr("Swimming Threshold Power based on Swimming Critical Velocity, used for SwimScore calculation"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {

        // xPowerSwim only makes sense for running and it needs recIntSecs > 0
        if (!item->isSwim || item->ride()->recIntSecs() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double weight = item->getWeight();
        
        const PaceZones *zones = item->context->athlete->paceZones(true);
        int zoneRange = item->paceZoneRange;

        // did user override for this ride?
        double cv = item->getText("CV","0").toDouble();

        // not overriden so use the set value
        // if it has been set at all
        if (!cv && zones && zoneRange >= 0) 
            cv = zones->getCV(zoneRange);
        
        // Swimming power at cv
        double watts = swimming_power(weight, cv/3.6);

        setValue(watts);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new STP(*this); }
};

// Swimming Relative Intensity
class SRI : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(SRI)
    double reli;
    double secs;

    public:

    SRI() : reli(0.0), secs(0.0)
    {
        setSymbol("swimscore_ri");
        setInternalName("SRI");
    }
    void initialize() {
        setName(tr("SRI"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(2);
        setDescription(tr("Swimming Relative Intensity, used for SwimScore calculation, defined as xPowerSwim/STP"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // xPowerSwim only makes sense for running and it needs recIntSecs > 0
        if (!item->isSwim || item->ride()->recIntSecs() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        assert(deps.contains("swimscore_xpower"));
        XPowerSwim *xPowerSwim = dynamic_cast<XPowerSwim*>(deps.value("swimscore_xpower"));
        assert(xPowerSwim);
        assert(deps.contains("swimscore_tp"));
        STP *stp = dynamic_cast<STP*>(deps.value("swimscore_tp"));
        assert(stp);
        reli = stp->value(true) ? xPowerSwim->value(true) / stp->value(true) : 0;
        secs = xPowerSwim->count();

        setValue(reli);
        setCount(secs);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new SRI(*this); }
};

// SwimScore Metric for swimming
class SwimScore : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(SwimScore)
    double score;

    public:

    SwimScore() : score(0.0)
    {
        setSymbol("swimscore");
        setInternalName("SwimScore");
    }
    void initialize() {
        setName("SwimScore");
        setType(RideMetric::Total);
        setDescription(tr("SwimScore swimming stress metric as defined by Dr. Skiba"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // xPowerSwim only makes sense for running and it needs recIntSecs > 0
        if (!item->isSwim || item->ride()->recIntSecs() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }
        assert(deps.contains("swimscore_xpower"));
        assert(deps.contains("swimscore_ri"));
        assert(deps.contains("swimscore_tp"));
        XPowerSwim *xPowerSwim = dynamic_cast<XPowerSwim*>(deps.value("swimscore_xpower"));
        assert(xPowerSwim);
        RideMetric *sri = deps.value("swimscore_ri");
        assert(sri);
        RideMetric *stp = deps.value("swimscore_tp");
        assert(stp);
        double normWork = xPowerSwim->value(true) * xPowerSwim->count();
        double rawGOVSS = normWork * sri->value(true);
        // No samples in manual workouts, use power at average speed and duration
        if (rawGOVSS == 0.0) {
            // unconst naughty boy, get athlete's data
            double weight = item->getWeight();
            assert(deps.contains("average_speed"));
            assert(deps.contains("workout_time"));
            double watts = swimming_power(weight, deps.value("average_speed")->value(true) / 3.6);
            double secs = deps.value("workout_time")->value(true);
            double sri = stp->value(true) ? watts / stp->value(true) : 0.0;
            rawGOVSS = watts * secs * sri;
        }
        double workInAnHourAtSTP = stp->value(true) * 3600;
        score = workInAnHourAtSTP ? rawGOVSS / workInAnHourAtSTP * 100.0 : 0;

        setValue(score);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isSwim; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new SwimScore(*this); }
};

static bool addAllSwimScore() {
    RideMetricFactory::instance().addMetric(XPowerSwim());
    RideMetricFactory::instance().addMetric(STP());
    QVector<QString> deps;
    deps.append("swimscore_xpower");
    RideMetricFactory::instance().addMetric(XPaceSwim(), &deps);
    deps.append("swimscore_tp");
    RideMetricFactory::instance().addMetric(SRI(), &deps);
    deps.append("swimscore_ri");
    deps.append("average_speed");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(SwimScore(), &deps);
    return true;
}

static bool SwimScoreAdded = addAllSwimScore();

// TriScore Metric for triathlon
class TriScore : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TriScore)
    double score;

    public:

    TriScore() : score(0.0)
    {
        setSymbol("triscore");
        setInternalName("TriScore");
    }
    void initialize() {
        setName("TriScore");
        setType(RideMetric::Total);
        setDescription(tr("TriScore combined stress metric based on Dr. Skiba stress metrics, defined as BikeScore for cycling, GOVSS for running and SwimScore for swimming. On zero fallback to TRIMP Zonal Points for HR based score."));
    }
    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        if (item->isSwim) {
            assert(deps.contains("swimscore"));
            score = deps.value("swimscore")->value(true);
        } else if (item->isRun) {
            assert(deps.contains("govss"));
            score = deps.value("govss")->value(true);
        } else {
            assert(deps.contains("skiba_bike_score"));
            score = deps.value("skiba_bike_score")->value(true);
        }

        // On zero fallback to TRIMP Zonal Points for HR based score
        if (score == 0.0) {
            assert(deps.contains("trimp_zonal_points"));
            score = deps.value("trimp_zonal_points")->value(true);
        }

        setValue(score);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new TriScore(*this); }
};

static bool addTriScore() {
    QVector<QString> deps;
    deps.append("swimscore");
    deps.append("govss");
    deps.append("skiba_bike_score");
    deps.append("trimp_zonal_points");
    RideMetricFactory::instance().addMetric(TriScore(), &deps);
    return true;
}

static bool TriScoreAdded = addTriScore();
