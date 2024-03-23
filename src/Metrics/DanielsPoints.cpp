/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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
#include "Zones.h"
#include "PaceZones.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include <cmath>
#include <assert.h>
#include <QApplication>

// The idea: Fit a curve to the points system in Table 2.2 of "Daniel's Running
// Formula", Second Edition, assume that power at VO2Max is 1.2 * FTP, further
// assume that pace is proportional to power (which is not too far off in
// running), and scale so that one hour at FTP is worth 33 points (which is
// the arbitrary value that Daniels chose).  Then you get a nice fit where
//
//               points per second = 33/3600 * (watts / FTP) ^ 4.



static inline double running_grade_adjusted(double speed, double slope=0.0) {
    // TODO: maybe move this function to separate file if using somewhere else
    double factor;

    if (slope == 0.0) {
        factor = 1;
    } else if (slope > 40) {
        factor = 3.739;
    } else if (slope < -40) {
        factor = 1.937;
    } else {
        // polynom function according points from strava GAP, works between -40% and 40%
        factor = -0.00000000340028981678083 * pow(slope, 5)
                 -0.000000471345135734814 * pow(slope, 4)
                 +0.000000777801576537029 * pow(slope, 3)
                 +0.001906977 * pow(slope, 2)
                 +0.0299822939 * slope
                 +0.9935756154;
    }
    return speed * factor;

    /* Running speed adjusted, according slope (grade adjusted pace model)
     * points from strava curve https://medium.com/strava-engineering/an-improved-gap-model-8b07ae8886c3
     * similar to research from Minetti 2002, but a little lower factor uphill and much lower downhill
     * slope is gradient in % (100% -> 45°)
        { -40, 1.937},
        { -34, 1.665},
        { -32, 1.590},
        { -30, 1.490},
        { -28, 1.400},
        { -26, 1.315},
        { -24, 1.240},
        { -22, 1.160},
        { -20, 1.085},
        { -18, 1.020},
        { -16, 0.965},
        { -14, 0.920},
        { -12, 0.885},
        { -10, 0.875},
        {  -9, 0.875},
        {  -8, 0.875},
        {  -7, 0.875},
        {  -6, 0.885},
        {  -5, 0.900},
        {  -4, 0.910},
        {  -3, 0.935},
        {  -2, 0.950},
        {  -1, 0.980},
        {   0, 1.000},
        {   1, 1.020},
        {   2, 1.050},
        {   3, 1.100},
        {   4, 1.140},
        {   5, 1.190},
        {   6, 1.240},
        {   7, 1.290},
        {   8, 1.350},
        {   9, 1.410},
        {  10, 1.480},
        {  12, 1.600},
        {  14, 1.770},
        {  16, 1.930},
        {  18, 2.110},
        {  20, 2.280},
        {  22, 2.470},
        {  24, 2.645},
        {  26, 2.820},
        {  28, 2.985},
        {  30, 3.160},
        {  32, 3.320},
        {  34, 3.470},
        {  40, 3.739}
    */
}


class DanielsPoints : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(DanielsPoints)

    double score;
    void inc(double secs, double watts, double cp) {
        score += K * secs * pow(watts / cp, 4);
    }

public:

    static const double K;

    DanielsPoints() : score(0.0)
    {
        setSymbol("daniels_points");
        setInternalName("Daniels Points");
    }

    void initialize() {
        setName(tr("Daniels Points"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
        setDescription(tr("Daniels Points adapted for cycling using power instead of pace and assuming VO2max-power=1.2*CP, normalized to assign 100 points to 1 hour at CP."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        if (item->ride()->areDataPresent()->watts) {
            computeOnPower(item, spec);
        } else if (item->isRun || item->isSwim) {
            // calculate danielpoints according CV for Run and Swim
            computeOnSpeed(item, spec);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("P") || ride->isRun || ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new DanielsPoints(*this); }

private:

    void computeOnPower(RideItem *item, Specification spec) {
        static const double EPSILON = 0.1;
        static const double NEGLIGIBLE = 0.1;
        int loops = 0;

        double secsDelta = item->ride()->recIntSecs();
        double sampsPerWindow = 25.0 / secsDelta;
        double attenuation = sampsPerWindow / (sampsPerWindow + secsDelta);
        double sampleWeight = secsDelta / (sampsPerWindow + secsDelta);

        double lastSecs = 0.0;
        double weighted = 0.0;

	if (item->context->athlete->zones(item->sport) == NULL || item->zoneRange < 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
	}
        double cp = item->context->athlete->zones(item->sport)->getCP(item->zoneRange);

        score = 0.0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            while ((weighted > NEGLIGIBLE)
                   && (point->secs > lastSecs + secsDelta + EPSILON)) {
                weighted *= attenuation;
                lastSecs += secsDelta;
                inc(secsDelta, weighted, cp);
            }
            weighted *= attenuation;
            weighted += sampleWeight * point->watts;
            lastSecs = point->secs;
            inc(secsDelta, weighted, cp);
        }

        // we must limit this loop, it could end up looping forever (and did before we constrained it)
        while (++loops < 10000 && !std::isnan(weighted) && !std::isinf(weighted) && weighted > NEGLIGIBLE) {
            weighted *= attenuation;
            lastSecs += secsDelta;
            inc(secsDelta, weighted, cp);
        }
        setValue(score);
    }

    void computeOnSpeed(RideItem *item, Specification spec) {
        static const double AVERAGE_WINDOW_IN_SECS = 10;

        double secsDelta = 0;
        double sampsPerWindow = 0;
        double lastWindowSecs = 0.0;
        double previousSecs = 0.0;
        double weighted = 0.0;
        double recIntSecs = item->ride()->recIntSecs();

	if (item->context->athlete->paceZones(item->isSwim) == NULL || item->paceZoneRange < 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
	}
        double cs = item->context->athlete->paceZones(item->isSwim)->getCV(item->paceZoneRange);

        score = 0.0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            // add speed for average calculation, for running adjust speed according slope (up/downwards)
            weighted += (item->isSwim ? point->kph : running_grade_adjusted(point->kph, point->slope));
            sampsPerWindow++;

            if (point->secs > lastWindowSecs + AVERAGE_WINDOW_IN_SECS || // take average value of this window
                point->secs > previousSecs + recIntSecs ||               // invalid recording gap, stop averaging
                it.hasNext() == false) {                                 // last point of ride

                weighted = sampsPerWindow > 0 ? (weighted / sampsPerWindow) : 0;
                secsDelta = previousSecs - lastWindowSecs;
                inc(secsDelta, weighted, cs);

                lastWindowSecs = point->secs;
                weighted = 0;
                sampsPerWindow = 0;
            }
            previousSecs = point->secs;
        }
        setValue(score);
    }

};


// Choose K such that 1 hour at FTP yields a score of 100.
const double DanielsPoints::K = 100.0 / 3600.0;

class DanielsEquivalentPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(DanielsEquivalentPower)
    double watts;

    public:

    DanielsEquivalentPower() : watts(0.0)
    {
        setSymbol("daniels_equivalent_power");
        setInternalName("Daniels EqP");
    }

    void initialize() {
        setName(tr("Daniels EqP"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Average);
        setDescription(tr("Daniels EqP is the constant power which would produce equivalent Daniels Points."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // no zones
        if (item->context->athlete->zones(item->sport) == NULL || item->zoneRange < 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double cp = item->context->athlete->zones(item->sport)->getCP(item->zoneRange);

        assert(deps.contains("daniels_points"));
        assert(deps.contains("time_riding"));
        assert(deps.contains("workout_time"));
        const RideMetric *danielsPoints = deps.value("daniels_points");
        const RideMetric *timeRiding = deps.value("time_riding");
        const RideMetric *workoutTime = deps.value("workout_time");
        assert(danielsPoints);
        assert(timeRiding);
        assert(workoutTime);
        double score = danielsPoints->value(true);
        double secs = timeRiding->value(true) ? timeRiding->value(true) :
                                                workoutTime->value(true);
        watts = secs == 0.0 ? 0.0 : cp * pow(score / DanielsPoints::K / secs, 0.25);

        setValue(watts);
    }
    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("P")); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new DanielsEquivalentPower(*this); }
};

static bool added() {
    RideMetricFactory::instance().addMetric(DanielsPoints());
    QVector<QString> deps;
    deps.append("time_riding");
    deps.append("workout_time");
    deps.append("daniels_points");
    RideMetricFactory::instance().addMetric(DanielsEquivalentPower(), &deps);
    return true;
}

static bool added_ = added();
