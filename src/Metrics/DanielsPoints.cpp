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
        if (spec.isEmpty(item->ride()) ||
            item->context->athlete->zones(item->isRun) == NULL || item->zoneRange < 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        static const double EPSILON = 0.1;
        static const double NEGLIGIBLE = 0.1;

        double secsDelta = item->ride()->recIntSecs();
        double sampsPerWindow = 25.0 / secsDelta;
        double attenuation = sampsPerWindow / (sampsPerWindow + secsDelta);
        double sampleWeight = secsDelta / (sampsPerWindow + secsDelta);

        double lastSecs = 0.0;
        double weighted = 0.0;

        score = 0.0;
        double cp = item->context->athlete->zones(item->isRun)->getCP(item->zoneRange);

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
        while (weighted > NEGLIGIBLE) {
            weighted *= attenuation;
            lastSecs += secsDelta;
            inc(secsDelta, weighted, cp);
        }
        setValue(score);
    }
    RideMetric *clone() const { return new DanielsPoints(*this); }
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
        if (item->context->athlete->zones(item->isRun) == NULL || item->zoneRange < 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double cp = item->context->athlete->zones(item->isRun)->getCP(item->zoneRange);

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
