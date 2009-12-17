/* 
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

const double  bikeScoreN   = 4.0;

// NOTE: This code follows the description of xPower, Relative Intensity, and
// BikeScore in "Analysis of Power Output and Training Stress in Cyclists: The
// Development of the BikeScore(TM) Algorithm", page 5, by Phil Skiba:
//
// http://www.physfarm.com/Analysis%20of%20Power%20Output%20and%20Training%20Stress%20in%20Cyclists-%20BikeScore.pdf
//
// The weighting factors for the exponentially weighted average are taken from
// a spreadsheet provided by Dr. Skiba.

class XPower : public RideMetric {
    double xpower;
    double secs;

    friend class RelativeIntensity;
    friend class BikeScore;

    public:

    XPower() : xpower(0.0), secs(0.0) {}
    QString name() const { return "skiba_xpower"; }
    QString units(bool) const { return "watts"; }
    double value(bool) const { return xpower; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {

        static const double EPSILON = 0.1;
        static const double NEGLIGIBLE = 0.1;

        double secsDelta = ride->recIntSecs();
        double sampsPerWindow = 25.0 / secsDelta;
        double attenuation = sampsPerWindow / (sampsPerWindow + secsDelta);
        double sampleWeight = secsDelta / (sampsPerWindow + secsDelta);
        
        double lastSecs = 0.0;
        double weighted = 0.0;

        double total = 0.0;
        int count = 0;

        foreach(const RideFilePoint *point, ride->dataPoints()) {
            while ((weighted > NEGLIGIBLE)
                   && (point->secs > lastSecs + secsDelta + EPSILON)) {
                weighted *= attenuation;
                lastSecs += secsDelta;
                total += pow(weighted, 4.0);
                count++;
            }
            weighted *= attenuation;
            weighted += sampleWeight * point->watts;
            lastSecs = point->secs;
            total += pow(weighted, 4.0);
            count++;
        }
        while (weighted > NEGLIGIBLE) {
            weighted *= attenuation;
            lastSecs += secsDelta;
            total += pow(weighted, 4.0);
            count++;
        }
        xpower = pow(total / count, 0.25);
        secs = count * secsDelta;
    }

    // added djconnel: allow RI to be combined across rides
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { 
        assert(name() == other->name());
	XPower *ap = dynamic_cast<XPower*>(other);
	xpower = pow(xpower, bikeScoreN) * secs + pow(ap->xpower, bikeScoreN) * ap->secs;
	secs += ap->secs;
	xpower = pow(xpower / secs, 1 / bikeScoreN);
    }
    // end added djconnel

    RideMetric *clone() const { return new XPower(*this); }
};

class RelativeIntensity : public RideMetric {
    double reli;
    double secs;

    public:

    RelativeIntensity() : reli(0.0), secs(0.0) {}
    QString name() const { return "skiba_relative_intensity"; }
    QString units(bool) const { return ""; }
    double value(bool) const { return reli; }
    void compute(const RideFile *, const Zones *zones, int zoneRange,
                 const QHash<QString,RideMetric*> &deps) {
        if (zones && zoneRange >= 0) {
            assert(deps.contains("skiba_xpower"));
            XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
            assert(xp);
            reli = xp->xpower / zones->getCP(zoneRange);
            secs = xp->secs;
        }
    }

    // added djconnel: allow RI to be combined across rides
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { 
        assert(name() == other->name());
	RelativeIntensity *ap = dynamic_cast<RelativeIntensity*>(other);
	reli = secs * pow(reli, bikeScoreN) + ap->secs * pow(ap->reli, bikeScoreN);
	secs += ap->secs;
	reli = pow(reli / secs, 1.0 / bikeScoreN);
    }
    // end added djconnel

    RideMetric *clone() const { return new RelativeIntensity(*this); }
};

class BikeScore : public RideMetric {
    double score;

    public:

    BikeScore() : score(0.0) {}
    QString name() const { return "skiba_bike_score"; }
    QString units(bool) const { return ""; }
    double value(bool) const { return score; }
    void compute(const RideFile *, const Zones *zones, int zoneRange,
	    const QHash<QString,RideMetric*> &deps) {
	if (!zones || zoneRange < 0)
	    return;
        assert(deps.contains("skiba_xpower"));
        assert(deps.contains("skiba_relative_intensity"));
        XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
        RideMetric *ri = deps.value("skiba_relative_intensity");
        assert(ri);
        double normWork = xp->xpower * xp->secs;
        double rawBikeScore = normWork * ri->value(true);
        double workInAnHourAtCP = zones->getCP(zoneRange) * 3600;
        score = rawBikeScore / workInAnHourAtCP * 100.0;
    }
    void override(const QMap<QString,QString> &map) {
        if (map.contains("value"))
            score = map.value("value").toDouble();
    }
    RideMetric *clone() const { return new BikeScore(*this); }
    bool canAggregate() const { return true; }
    void aggregateWith(RideMetric *other) { score += other->value(true); }
};

static bool addAllThree() {
    RideMetricFactory::instance().addMetric(XPower());
    QVector<QString> deps;
    deps.append("skiba_xpower");
    RideMetricFactory::instance().addMetric(RelativeIntensity(), &deps);
    deps.append("skiba_relative_intensity");
    RideMetricFactory::instance().addMetric(BikeScore(), &deps);
    return true;
}

static bool allThreeAdded = addAllThree();

