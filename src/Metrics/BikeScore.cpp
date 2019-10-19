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
#include "RideItem.h"
#include "Specification.h"
#include "RideFile.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include <cmath>
#include <assert.h>
#include <QApplication>

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
        setInternalName("xPower");
    }

    void initialize() {
        setName(tr("xPower"));
        setType(RideMetric::Average);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setDescription(tr("xPower is an estimate of the power that you could have maintained for the same physiological 'cost' if your power output had been perfectly constant, similar to IsoPower."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || item->ride()->recIntSecs()==0) {
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

        double total = 0.0;
        int count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

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
        xpower = count ? pow(total / count, 0.25) : 0.0;
        secs = count * secsDelta;

        setValue(xpower);
        setCount(secs);
    }
    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setInternalName("Skiba VI");
    }

    void initialize() {
        setName(tr("Skiba VI"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
        setDescription(tr("Skiba Variability Index is the ratio between xPower and Average Power."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

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
    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
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
        setInternalName("Relative Intensity");
    }

    void initialize() {
        setName(tr("Relative Intensity"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
        setDescription(tr("Relative Intensity is the ratio between xPower and the Critical Power (CP) configured in Power Zones, similar to IF."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        if (item->context->athlete->zones(item->isRun) && item->zoneRange >= 0) {
            assert(deps.contains("skiba_xpower"));
            XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
            assert(xp);
            int cp = item->getText("CP","0").toInt();
            reli = xp->value(true) / (cp ? cp : item->context->athlete->zones(item->isRun)->getCP(item->zoneRange));
            secs = xp->count();
        }
        setValue(reli);
        setCount(secs);
    }

    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new RelativeIntensity(*this); }
};

class CriticalPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(CriticalPower)

    public:

    CriticalPower()
    {
        setSymbol("cp_setting");
        setInternalName("CP setting");
    }

    void initialize() {
        setName(tr("Critical Power"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(0);
        setDescription(tr("Critical Power (CP) configured in Power Zones."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {

        // did user override for this ride?
        int cp = item->getText("CP","0").toInt();

        // not overriden so use the set value
        // if it has been set at all
        if (!cp && item->context->athlete->zones(item->isRun) && item->zoneRange >= 0) 
            cp = item->context->athlete->zones(item->isRun)->getCP(item->zoneRange);
        
        setValue(cp);
    }

    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new CriticalPower(*this); }
};

class aTISS : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aTISS)

    public:

    aTISS()
    {
        setSymbol("atiss_score");
        setInternalName("Aerobic TISS");
    }

    void initialize() {
        setName(tr("Aerobic TISS"));
        setMetricUnits("");
        setImperialUnits("");
        setDescription(tr("Aerobic Training Impact Scoring System. It's a metric to quantify the training strain or response on the aerobic system"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

	    if (!item->context->athlete->zones(item->isRun) || item->zoneRange < 0) return;

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // aTISS - Aerobic Training Impact Scoring System
        static const double a = 0.663788683661645f;
        static const double b = -7.5095428451195;
        static const double c = -0.86118031563782;
        double aTISS = 0.0f;

        int cp = item->getText("CP","0").toInt();
        if (!cp) cp = item->context->athlete->zones(item->isRun)->getCP(item->zoneRange);

        if (cp && item->ride()->areDataPresent()->watts) {

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

                // a * exp (b * exp (c * fraction of cp) ) 
                aTISS += item->ride()->recIntSecs() * (a * exp(b * exp(c * (double(point->watts) / double(cp)))));
            }
        }
        setValue(aTISS);
    }

    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new aTISS(*this); }
};

class anTISS : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aTISS)

    public:

    anTISS()
    {
        setSymbol("antiss_score");
        setInternalName("Anaerobic TISS");
    }

    void initialize() {
        setName(tr("Anaerobic TISS"));
        setMetricUnits("");
        setImperialUnits("");
        setDescription(tr("Anaerobic Training Impact Scoring System. It's a metric to quantify the training strain or response on the anaerobic system"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

	    if (!item->context->athlete->zones(item->isRun) || item->zoneRange < 0) return;

        // anTISS - Aerobic Training Impact Scoring System
        static const double a = 0.238923886004611f;
        //static const double b = -12.2066385296127f;
        static const double b = -61.849f;
        static const double c = -1.73549567522521f;
        double anTISS = 0.0f;

        int cp = item->getText("CP","0").toInt();
        if (!cp) cp = item->context->athlete->zones(item->isRun)->getCP(item->zoneRange);

        if (cp && item->ride()->areDataPresent()->watts) {
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();

                // a * exp (b * exp (c * fraction of cp) ) 
                anTISS += item->ride()->recIntSecs() * (a * exp(b * exp(c * (double(point->watts) / double(cp)))));
            }
        }
        setValue(anTISS);
    }

    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new anTISS(*this); }
};

class dTISS : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(dTISS)

    public:

    dTISS()
    {
        setSymbol("tiss_delta");
        setInternalName("TISS Aerobicity");
        setType(RideMetric::Average);
    }

    void initialize() {
        setName(tr("TISS Aerobicity"));
        setMetricUnits("Percent");
        setImperialUnits("Percent");
        setDescription(tr("TISS Aerobicity is a percentage of Aerobic TISS of the total TISS"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

	    if (!item->context->athlete->zones(item->isRun) || item->zoneRange < 0) return;

        assert(deps.contains("atiss_score"));
        assert(deps.contains("antiss_score"));
        double atscore = dynamic_cast<aTISS*>(deps.value("atiss_score"))->value(true);
        double antscore = dynamic_cast<anTISS*>(deps.value("antiss_score"))->value(true);

        // we don't like nan results
        if (atscore == 0.00 && antscore == 0.00) setValue(0.0);
        else setValue((atscore/(atscore+antscore)) * 100.00f);
    }

    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new dTISS(*this); }
};

class BikeScore : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(BikeScore)
    double score;

    public:

    BikeScore() : score(0.0)
    {
        setSymbol("skiba_bike_score");
        setInternalName("BikeScore&#8482;");
    }

    void initialize() {
        setName("BikeScore&#8482;");  // Don't translate as many places have special coding for the "TM" sign
        setMetricUnits("");
        setImperialUnits("");
        setDescription(tr("Skiba's stress score taking into account both the intensity and the duration of the training session, similar to BikeStress it can be computed as 100 * hours * (Relative Intensity)^2"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // run, swim or no zones
        if (item->isSwim || item->isRun ||
           !item->context->athlete->zones(item->isRun) || item->zoneRange < 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        assert(deps.contains("skiba_xpower"));
        assert(deps.contains("skiba_relative_intensity"));
        XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
        RideMetric *ri = deps.value("skiba_relative_intensity");
        assert(ri);
        double normWork = xp->value(true) * xp->count();
        double rawBikeScore = normWork * ri->value(true);
        int cp = item->getText("CP","0").toInt();
        double workInAnHourAtCP = (cp ? cp : item->context->athlete->zones(item->isRun)->getCP(item->zoneRange)) * 3600;
        score = rawBikeScore / workInAnHourAtCP * 100.0;

        setValue(score);
    }

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new BikeScore(*this); }
};

class ResponseIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(ResponseIndex)
    double ri;

    public:

    ResponseIndex() : ri(0.0)
    {
        setSymbol("skiba_response_index");
        setInternalName("Response Index");
    }

    void initialize() {
        setName(tr("Response Index"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
        setDescription(tr("The ratio between xPower and Average HR, similar to Efficiency Factor"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("skiba_xpower"));
        assert(deps.contains("average_hr"));
        XPower *xp = dynamic_cast<XPower*>(deps.value("skiba_xpower"));
        assert(xp);
        RideMetric *ah = dynamic_cast<RideMetric*>(deps.value("average_hr"));
        assert(ah);
        ri = xp->value(true) / ah->value(true);

        setValue(ri);
    }
    bool isRelevantForRide(const RideItem*ride) const { return ride->present.contains("P") || (!ride->isRun && !ride->isSwim); }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new ResponseIndex(*this); }
};

static bool addAllSix() {
    RideMetricFactory::instance().addMetric(aTISS());
    RideMetricFactory::instance().addMetric(anTISS());
    RideMetricFactory::instance().addMetric(CriticalPower());
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
    deps.clear();
    deps.append("atiss_score");
    deps.append("antiss_score");
    RideMetricFactory::instance().addMetric(dTISS(), &deps);
    deps.clear();
    deps.append("skiba_xpower");
    deps.append("average_hr");
    RideMetricFactory::instance().addMetric(ResponseIndex(), &deps);
    return true;
}

static bool allSixAdded = addAllSix();

