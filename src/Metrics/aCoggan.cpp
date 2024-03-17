/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include "Zones.h"
#include <cmath>
#include <assert.h>
#include <QApplication>

class aIsoPower : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aIsoPower)
    double np;
    double secs;

    public:

    aIsoPower() : np(0.0), secs(0.0)
    {
        setSymbol("a_coggan_np");
        setInternalName("aIsoPower");
    }
    void initialize() {
        setName("aIsoPower");
        setType(RideMetric::Average);
        setMetricUnits("watts");
        setImperialUnits("watts");
        setPrecision(0);
        setDescription(tr("Altitude Adjusted Iso Power is an estimate of the power that you could have maintained for the same physiological 'cost' if your power output had been perfectly constant accounting for altitude."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || item->ride()->recIntSecs() == 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        int rollingwindowsize = 30 / item->ride()->recIntSecs();

        double total = 0;
        int count = 0;

        // no point doing a rolling average if the
        // sample rate is greater than the rolling average
        // window!!
        if (rollingwindowsize > 1) {

            QVector<double> rolling(rollingwindowsize);
            int index = 0;
            double sum = 0;

            RideFileIterator it(item->ride(), spec);

            // loop over the data and convert to a rolling
            // average for the given windowsize
            for (int i=it.firstIndex(); i >=0 && i<=it.lastIndex(); i++) {

                sum += item->ride()->dataPoints()[i]->apower;
                sum -= rolling[index];

                rolling[index] = item->ride()->dataPoints()[i]->apower;

                total += pow(sum/rollingwindowsize,4); // raise rolling average to 4th power
                count ++;

                // move index on/round
                index = (index >= rollingwindowsize-1) ? 0 : index+1;
            }
        }
        if (count) {
            np = pow(total / (count), 0.25);
            secs = count * item->ride()->recIntSecs();
        } else {
            np = secs = 0;
        }

        setValue(np);
        setCount(secs);
    }

    bool isRelevantForRide(const RideItem *item) const { 
        return item->present.contains("P") || (!item->isRun && !item->isSwim);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new aIsoPower(*this); }
};

class aVI : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aVI)
    double vi;
    double secs;

    public:

    aVI() : vi(0.0), secs(0.0)
    {
        setSymbol("a_coggam_variability_index");
        setInternalName("aVI");
    }
    void initialize() {
        setName("aVI");
        setType(RideMetric::Average);
        setPrecision(3);
        setDescription(tr("Altitude Adjusted Variability Index is the ratio between aIsoPower and Average aPower."));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("a_coggan_np"));
        assert(deps.contains("average_power"));
        aIsoPower *np = dynamic_cast<aIsoPower*>(deps.value("a_coggan_np"));
        assert(np);
        RideMetric *ap = dynamic_cast<RideMetric*>(deps.value("average_power"));
        assert(ap);
        vi = np->value(true) / ap->value(true);
        secs = np->count();

        setValue(vi);
        setCount(secs);
    }

    bool isRelevantForRide(const RideItem *item) const { 
        return item->present.contains("P") || (!item->isRun && !item->isSwim);
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new aVI(*this); }
};

class aIntensityFactor : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aIntensityFactor)
    double rif;
    double secs;

    public:

    aIntensityFactor() : rif(0.0), secs(0.0)
    {
        setSymbol("a_coggan_if");
        setInternalName("aIF");
    }
    void initialize() {
        setName("aIF");
        setType(RideMetric::Average);
        setPrecision(3);
        setDescription(tr("Altitude Adjusted Intensity Factor is the ratio between aIsoPower and the Critical Power (CP) configured in Power Zones."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // no ride or no samples
        if (item->zoneRange < 0 || item->context->athlete->zones(item->sport) == NULL) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        assert(deps.contains("a_coggan_np"));
        aIsoPower *np = dynamic_cast<aIsoPower*>(deps.value("a_coggan_np"));
        assert(np);
        int cp = item->getText("CP","0").toInt();
        rif = np->value(true) / (cp ? cp : item->context->athlete->zones(item->sport)->getCP(item->zoneRange));
        secs = np->count();

        setValue(rif);
        setCount(secs);
    }

    bool isRelevantForRide(const RideItem *item) const {
        return item->present.contains("P") || (!item->isRun && !item->isSwim);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new aIntensityFactor(*this); }
};

class aBikeStress : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aBikeStress)
    double score;

    public:

    aBikeStress() : score(0.0)
    {
        setSymbol("a_coggan_tss");
        setInternalName("aBikeStress");
    }
    void initialize() {
        setName("aBikeStress");
        setType(RideMetric::Total);
        setDescription(tr("Altitude Adjusted Training Stress Score takes into account both the intensity and the duration of the training session plus the altitude effect, it can be computed as 100 * hours * aIF^2"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // no ride or no samples
        if (item->zoneRange < 0 || item->context->athlete->zones(item->sport) == NULL) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        assert(deps.contains("a_coggan_np"));
        assert(deps.contains("a_coggan_if"));
        aIsoPower *np = dynamic_cast<aIsoPower*>(deps.value("a_coggan_np"));
        RideMetric *rif = deps.value("a_coggan_if");
        assert(rif);
        double normWork = np->value(true) * np->count();
        double rawTSS = normWork * rif->value(true);
        int cp = item->getText("CP","0").toInt();
        double workInAnHourAtCP = (cp ? cp : item->context->athlete->zones(item->sport)->getCP(item->zoneRange)) * 3600;
        score = rawTSS / workInAnHourAtCP * 100.0;

        setValue(score);
    }

    bool isRelevantForRide(const RideItem *item) const {
        return item->present.contains("P") || (!item->isRun && !item->isSwim);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new aBikeStress(*this); }
};

class aTSSPerHour : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aTSSPerHour)
    double points;
    double hours;

    public:

    aTSSPerHour() : points(0.0), hours(0.0)
    {
        setSymbol("a_coggan_tssperhour");
        setInternalName("aBikeStress per hour");
    }

    void initialize() {
        setName(tr("aBikeStress per hour"));
        setType(RideMetric::Average);
        setPrecision(0);
        setDescription(tr("Altitude Adjusted Training Stress Score divided by Duration in hours"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        // tss
        assert(deps.contains("a_coggan_tss"));
        aBikeStress *tss = dynamic_cast<aBikeStress*>(deps.value("a_coggan_tss"));
        assert(tss);

        // duration
        assert(deps.contains("workout_time"));
        RideMetric *duration = deps.value("workout_time");
        assert(duration);

        points = tss->value(true);
        hours = duration->value(true) / 3600;

        // set
        if (hours) setValue(points/hours);
        else setValue(0);
        setCount(hours);
    }

    bool isRelevantForRide(const RideItem *item) const {
        return item->present.contains("P") || (!item->isRun && !item->isSwim);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new aTSSPerHour(*this); }
};

class aEfficiencyFactor : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aEfficiencyFactor)
    double ef;

    public:

    aEfficiencyFactor() : ef(0.0)
    {
        setSymbol("a_friel_efficiency_factor");
        setInternalName("aPower Efficiency Factor");
    }
    void initialize() {
        setName(tr("aPower Efficiency Factor"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
        setDescription(tr("The ratio between aIsoPower and Average HR"));
    }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        assert(deps.contains("a_coggan_np"));
        assert(deps.contains("average_hr"));
        aIsoPower *np = dynamic_cast<aIsoPower*>(deps.value("a_coggan_np"));
        assert(np);
        RideMetric *ah = dynamic_cast<RideMetric*>(deps.value("average_hr"));
        assert(ah);
        ef = np->value(true) / ah->value(true);

        setValue(ef);
    }

    bool isRelevantForRide(const RideItem *item) const {
        return item->present.contains("P") || (!item->isRun && !item->isSwim);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new aEfficiencyFactor(*this); }
};

static bool addAllaCoggan() {
    RideMetricFactory::instance().addMetric(aIsoPower());
    QVector<QString> deps;
    deps.append("a_coggan_np");
    RideMetricFactory::instance().addMetric(aIntensityFactor(), &deps);
    deps.append("a_coggan_if");
    RideMetricFactory::instance().addMetric(aBikeStress(), &deps);
    deps.clear();
    deps.append("a_coggan_np");
    deps.append("average_power");
    RideMetricFactory::instance().addMetric(aVI(), &deps);
    deps.clear();
    deps.append("a_coggan_np");
    deps.append("average_hr");
    RideMetricFactory::instance().addMetric(aEfficiencyFactor(), &deps);
    deps.clear();
    deps.append("a_coggan_tss");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(aTSSPerHour(), &deps);
    return true;
}

static bool aCogganAdded = addAllaCoggan();

