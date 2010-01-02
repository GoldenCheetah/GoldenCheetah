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
#include <math.h>

// The idea: Fit a curve to the points system in Table 2.2 of "Daniel's Running
// Formula", Second Edition, assume that power at VO2Max is 1.2 * FTP, further
// assume that pace is proportional to power (which is not too far off in
// running), and scale so that one hour at FTP is worth 33 points (which is
// the arbitrary value that Daniels chose).  Then you get a nice fit where
//
//               points per second = 33/3600 * (watts / FTP) ^ 4.


class DanielsPoints : public RideMetric {

    static const double K;
    double score, cp;
    void count(double secs, double watts) {
        score += K * secs * pow(watts / cp, 4);
    }

    public:

    DanielsPoints() : score(0.0), cp(0.0) {}
    QString symbol() const { return "daniels_points"; }
    QString name() const { return QObject::tr("Daniels Points"); }
    QString units(bool) const { return ""; }
    int precision() const { return 0; }
    double value(bool) const { return score; }
    void compute(const RideFile *ride, const Zones *zones,
                 int zoneRange, const QHash<QString,RideMetric*> &) {
        if (!zones || zoneRange < 0)
            return;

        if (ride->deviceType() == QString("Manual CSV")) {
            // Manual entry: use BS from dataPoints with a scaling factor
            // that works about right for long, steady rides.
            double scaling_factor = 0.55;
            if (ride->metricOverrides.contains("skiba_bike_score")) {
                const QMap<QString,QString> bsm =
                    ride->metricOverrides.value("skiba_bike_score");
                if (bsm.contains("value")) {
                    double bs = bsm.value("value").toDouble();
                    score = bs * scaling_factor;
                }
            }
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
        cp = zones->getCP(zoneRange);

        foreach(const RideFilePoint *point, ride->dataPoints()) {
            while ((weighted > NEGLIGIBLE)
                   && (point->secs > lastSecs + secsDelta + EPSILON)) {
                weighted *= attenuation;
                lastSecs += secsDelta;
                count(secsDelta, weighted);
            }
            weighted *= attenuation;
            weighted += sampleWeight * point->watts;
            lastSecs = point->secs;
            count(secsDelta, weighted);
        }
        while (weighted > NEGLIGIBLE) {
            weighted *= attenuation;
            lastSecs += secsDelta;
            count(secsDelta, weighted);
        }
    }
    void override(const QMap<QString,QString> &map) {
        if (map.contains("value"))
            score = map.value("value").toDouble();
    }
    void aggregateWith(const RideMetric &other) { score += other.value(true); }
    RideMetric *clone() const { return new DanielsPoints(*this); }
};

// Choose K such that 1 hour at FTP yields a score of 100.
const double DanielsPoints::K = 100.0 / 3600.0;

static bool added() {
    RideMetricFactory::instance().addMetric(DanielsPoints());
    return true;
}

static bool added_ = added();

