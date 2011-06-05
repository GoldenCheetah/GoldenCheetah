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
#include <QApplication>

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
    Q_DECLARE_TR_FUNCTIONS(XPower)

    double xpower;
    double secs;

    public:

    XPower() : xpower(0.0), secs(0.0)
    {
        setSymbol("skiba_xpower");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("xPower");
    }
    void initialize() {
#endif
        setName(tr("xPower"));
        setType(RideMetric::Average);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
    }

    void compute(const RideFile *ride, const Zones *, int, const HrZones *, int,
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
        xpower = pow(total / count, 0.25);
        secs = count * secsDelta;

        setValue(xpower);
        setCount(secs);
    }
    RideMetric *clone() const { return new XPower(*this); }
};

class VariabilityIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(VariabilityIndex)

    double vi;
    double secs;

    public:

    VariabilityIndex() : vi(0.0), secs(0.0)
    {
        setSymbol("skiba_variability_index");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Skiba VI");
    }
    void initialize() {
#endif
        setName(tr("Skiba VI"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
    }

    void compute(const RideFile *, const Zones *, int, const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps) {
        assert(deps.contains("skiba_xpower"));
        assert(deps.contains("average_power"));
        XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
        assert(xp);
        RideMetric *ap = dynamic_cast<RideMetric*>(deps.value("average_power"));
        assert(ap);
        vi = xp->value(true) / ap->value(true);
        secs = xp->count();

        setValue(vi);
    }
    RideMetric *clone() const { return new VariabilityIndex(*this); }
};

class RelativeIntensity : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(RelativeIntensity)

    double reli;
    double secs;

    public:

    RelativeIntensity() : reli(0.0), secs(0.0)
    {
        setSymbol("skiba_relative_intensity");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("Relative Intensity");
    }
    void initialize() {
#endif
        setName(tr("Relative Intensity"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
    }
    void compute(const RideFile *, const Zones *zones, int zoneRange, const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps) {
        if (zones && zoneRange >= 0) {
            assert(deps.contains("skiba_xpower"));
            XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
            assert(xp);
            reli = xp->value(true) / zones->getCP(zoneRange);
            secs = xp->count();
        }
        setValue(reli);
        setCount(secs);
    }

    // added djconnel: allow RI to be combined across rides
    bool canAggregate() const { return true; }
    void aggregateWith(const RideMetric &other) { 
        assert(symbol() == other.symbol());
	    const RelativeIntensity &ap = dynamic_cast<const RelativeIntensity&>(other);
	    reli = secs * pow(reli, bikeScoreN) + ap.count() * pow(ap.value(true), bikeScoreN);
	    secs += ap.count();
	    reli = pow(reli / secs, 1.0 / bikeScoreN);
        setValue(reli);
    }
    // end added djconnel

    RideMetric *clone() const { return new RelativeIntensity(*this); }
};

class BikeScore : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(BikeScore)

    double score;

    public:

    BikeScore() : score(0.0)
    {
        setSymbol("skiba_bike_score");
#ifdef ENABLE_METRICS_TRANSLATION
        setInternalName("BikeScore&#8482;");
    }
    void initialize() {
#endif
        setName(tr("BikeScore&#8482;"));
        setMetricUnits("");
        setImperialUnits("");
    }
    void compute(const RideFile *, const Zones *zones, int zoneRange,const HrZones *, int,
	    const QHash<QString,RideMetric*> &deps) {
	    if (!zones || zoneRange < 0)
	        return;

        assert(deps.contains("skiba_xpower"));
        assert(deps.contains("skiba_relative_intensity"));
        XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
        RideMetric *ri = deps.value("skiba_relative_intensity");
        assert(ri);
        double normWork = xp->value(true) * xp->count();
        double rawBikeScore = normWork * ri->value(true);
        double workInAnHourAtCP = zones->getCP(zoneRange) * 3600;
        score = rawBikeScore / workInAnHourAtCP * 100.0;

        setValue(score);
    }

    RideMetric *clone() const { return new BikeScore(*this); }
};

static bool addAllFour() {
    RideMetricFactory::instance().addMetric(XPower());
    QVector<QString> deps;
    deps.append("skiba_xpower");
    RideMetricFactory::instance().addMetric(RelativeIntensity(), &deps);
    deps.append("skiba_relative_intensity");
    RideMetricFactory::instance().addMetric(BikeScore(), &deps);
    deps.clear();
    deps.append("skiba_xpower");
    deps.append("average_power");
    RideMetricFactory::instance().addMetric(VariabilityIndex(), &deps);
    return true;
}

static bool allFourAdded = addAllFour();

