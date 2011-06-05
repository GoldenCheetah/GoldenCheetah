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
#include "Zones.h"
#include <QObject>
#include <math.h>
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
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Daniels Points");
    }
    void initialize() {
#endif
        setName(tr("Daniels Points"));
        setMetricUnits("");
        setImperialUnits("");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *ride, const Zones *zones,
                 int zoneRange, const HrZones *, int, const QHash<QString,RideMetric*> &) {
        if (!zones || zoneRange < 0) {
            setValue(0);
            return;
        }

        static const double EPSILON = 0.1;
        static const double NEGLIGIBLE = 0.1;

        double secsDelta = ride->recIntSecs();
        double sampsPerWindow = 25.0 / secsDelta;
        double attenuation = sampsPerWindow / (sampsPerWindow + secsDelta);
        double sampleWeight = secsDelta / (sampsPerWindow + secsDelta);

        double lastSecs = 0.0;
        double weighted = 0.0;

        score = 0.0;
        double cp = zones->getCP(zoneRange);

        foreach(const RideFilePoint *point, ride->dataPoints()) {
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
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Daniels EqP");
    }
    void initialize() {
#endif
        setName(tr("Daniels EqP"));
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setType(RideMetric::Average);
    }
    void compute(const RideFile *, const Zones *zones, int zoneRange, const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps)
    {
        if (!zones || zoneRange < 0) {
            setValue(0);
            return;
        }

        double cp = zones->getCP(zoneRange);
        assert(deps.contains("daniels_points"));
        assert(deps.contains("time_riding"));
        const RideMetric *danielsPoints = deps.value("daniels_points");
        const RideMetric *timeRiding = deps.value("time_riding");
        assert(danielsPoints);
        assert(timeRiding);
        double score = danielsPoints->value(true);
        double secs = timeRiding->value(true);
        watts = secs == 0.0 ? 0.0 : cp * pow(score / DanielsPoints::K / secs, 0.25);

        setValue(watts);
    }
    RideMetric *clone() const { return new DanielsEquivalentPower(*this); }
};

static bool added() {
    RideMetricFactory::instance().addMetric(DanielsPoints());
    QVector<QString> deps;
    deps.append("time_riding");
    deps.append("daniels_points");
    RideMetricFactory::instance().addMetric(DanielsEquivalentPower(), &deps);
    return true;
}

static bool added_ = added();
