/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
 *               2015 Mark Liversedge (liversedge@gmail.com)
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
#include <cmath>
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

class aXPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aXPower)
    double xpower;
    double secs;

    public:

    aXPower() : xpower(0.0), secs(0.0)
    {
        setSymbol("a_skiba_xpower");
        setInternalName("axPower");
    }
    void initialize() {
        setName(tr("axPower"));
        setType(RideMetric::Average);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

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
            weighted += sampleWeight * point->apower;
            lastSecs = point->secs;
            total += pow(weighted, 4.0);
            count++;
        }
        xpower = count ? pow(total / count, 0.25) : 0.0;
        secs = count * secsDelta;

        setValue(xpower);
        setCount(secs);
    }
    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aXPower(*this); }
};

class aVariabilityIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aVariabilityIndex)
    double vi;
    double secs;

    public:

    aVariabilityIndex() : vi(0.0), secs(0.0)
    {
        setSymbol("a_skiba_variability_index");
        setInternalName("Skiba aVI");
    }
    void initialize() {
        setName(tr("Skiba aVI"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        assert(deps.contains("a_skiba_xpower"));
        assert(deps.contains("average_power"));
        aXPower *xp = dynamic_cast<aXPower*>(deps.value("a_skiba_xpower"));
        assert(xp);
        RideMetric *ap = dynamic_cast<RideMetric*>(deps.value("average_power"));
        assert(ap);
        vi = xp->value(true) / ap->value(true);
        secs = xp->count();

        setValue(vi);
    }
    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aVariabilityIndex(*this); }
};

class aRelativeIntensity : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aRelativeIntensity)
    double reli;
    double secs;

    public:

    aRelativeIntensity() : reli(0.0), secs(0.0)
    {
        setSymbol("a_skiba_relative_intensity");
        setInternalName("aPower Relative Intensity");
    }
    void initialize() {
        setName(tr("aPower Relative Intensity"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
    }
    void compute(const RideFile *r, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        if (zones && zoneRange >= 0) {
            assert(deps.contains("a_skiba_xpower"));
            aXPower *xp = dynamic_cast<aXPower*>(deps.value("a_skiba_xpower"));
            assert(xp);
            int cp = r->getTag("CP","0").toInt();
            reli = xp->value(true) / (cp ? cp : zones->getCP(zoneRange));
            secs = xp->count();
        }
        setValue(reli);
        setCount(secs);
    }

    // added djconnel: allow RI to be combined across rides
    bool canAggregate() { return true; }
    void aggregateWith(const RideMetric &other) {
        assert(symbol() == other.symbol());
	    const aRelativeIntensity &ap = dynamic_cast<const aRelativeIntensity&>(other);
	    reli = secs * pow(reli, bikeScoreN) + ap.count() * pow(ap.value(true), bikeScoreN);
	    secs += ap.count();
	    reli = pow(reli / secs, 1.0 / bikeScoreN);
        setValue(reli);
    }
    // end added djconnel

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aRelativeIntensity(*this); }
};

class aBikeScore : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aBikeScore)
    double score;

    public:

    aBikeScore() : score(0.0)
    {
        setSymbol("a_skiba_bike_score");
        setInternalName("aBikeScore");
    }
    void initialize() {
        setName("aBikeScore");  // Don't translate as many places have special coding for the "TM" sign
        setMetricUnits("");
        setImperialUnits("");
    }

    void compute(const RideFile *r, const Zones *zones, int zoneRange,
                 const HrZones *, int,
	    const QHash<QString,RideMetric*> &deps,
                 const Context *) {
	    if (!zones || zoneRange < 0)
	        return;

        assert(deps.contains("a_skiba_xpower"));
        assert(deps.contains("a_skiba_relative_intensity"));
        aXPower *xp = dynamic_cast<aXPower*>(deps.value("a_skiba_xpower"));
        RideMetric *ri = deps.value("a_skiba_relative_intensity");
        assert(ri);
        double normWork = xp->value(true) * xp->count();
        double rawBikeScore = normWork * ri->value(true);
        int cp = r->getTag("CP","0").toInt();
        double workInAnHourAtCP = (cp ? cp : zones->getCP(zoneRange)) * 3600;
        score = rawBikeScore / workInAnHourAtCP * 100.0;

        setValue(score);
    }

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aBikeScore(*this); }
};

class aResponseIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aResponseIndex)
    double ri;

    public:

    aResponseIndex() : ri(0.0)
    {
        setSymbol("a_skiba_response_index");
        setInternalName("aPower Response Index");
    }
    void initialize() {
        setName(tr("aPower Response Index"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        assert(deps.contains("a_skiba_xpower"));
        assert(deps.contains("average_hr"));
        aXPower *xp = dynamic_cast<aXPower*>(deps.value("a_skiba_xpower"));
        assert(xp);
        RideMetric *ah = dynamic_cast<RideMetric*>(deps.value("average_hr"));
        assert(ah);
        ri = xp->value(true) / ah->value(true);

        setValue(ri);
    }
    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aResponseIndex(*this); }
};

static bool addAllaSix() {
    RideMetricFactory::instance().addMetric(aXPower());
    QVector<QString> deps;
    deps.append("a_skiba_xpower");
    RideMetricFactory::instance().addMetric(aRelativeIntensity(), &deps);
    deps.append("a_skiba_relative_intensity");
    RideMetricFactory::instance().addMetric(aBikeScore(), &deps);
    deps.clear();
    deps.append("a_skiba_xpower");
    deps.append("average_power");
    RideMetricFactory::instance().addMetric(aVariabilityIndex(), &deps);
    deps.clear();
    deps.append("a_skiba_xpower");
    deps.append("average_hr");
    RideMetricFactory::instance().addMetric(aResponseIndex(), &deps);
    return true;
}

static bool allaSixAdded = addAllaSix();
