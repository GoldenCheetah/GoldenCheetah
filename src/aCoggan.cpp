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
#include "Zones.h"
#include <cmath>
#include <QApplication>

class aNP : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aNP)
    double np;
    double secs;

    public:

    aNP() : np(0.0), secs(0.0)
    {
        setSymbol("a_coggan_np");
        setInternalName("aNP");
    }
    void initialize() {
        setName("aNP");
        setType(RideMetric::Average);
        setMetricUnits("watts");
        setImperialUnits("watts");
        setPrecision(0);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        if(ride->recIntSecs() == 0) return;

        int rollingwindowsize = 30 / ride->recIntSecs();

        double total = 0;
        int count = 0;

        // no point doing a rolling average if the
        // sample rate is greater than the rolling average
        // window!!
        if (rollingwindowsize > 1) {

            QVector<double> rolling(rollingwindowsize);
            int index = 0;
            double sum = 0;

            // loop over the data and convert to a rolling
            // average for the given windowsize
            for (int i=0; i<ride->dataPoints().size(); i++) {

                sum += ride->dataPoints()[i]->apower;
                sum -= rolling[index];

                rolling[index] = ride->dataPoints()[i]->apower;

                total += pow(sum/rollingwindowsize,4); // raise rolling average to 4th power
                count ++;

                // move index on/round
                index = (index >= rollingwindowsize-1) ? 0 : index+1;
            }
        }
        if (count) {
            np = pow(total / (count), 0.25);
            secs = count * ride->recIntSecs();
        } else {
            np = secs = 0;
        }

        setValue(np);
        setCount(secs);
    }
    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aNP(*this); }
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
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
            assert(deps.contains("a_coggan_np"));
            assert(deps.contains("average_power"));
            aNP *np = dynamic_cast<aNP*>(deps.value("a_coggan_np"));
            assert(np);
            RideMetric *ap = dynamic_cast<RideMetric*>(deps.value("average_power"));
            assert(ap);
            vi = np->value(true) / ap->value(true);
            secs = np->count();

            setValue(vi);
            setCount(secs);
    }

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
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
    }
    void compute(const RideFile *r, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        if (zones && zoneRange >= 0) {
            assert(deps.contains("a_coggan_np"));
            aNP *np = dynamic_cast<aNP*>(deps.value("a_coggan_np"));
            assert(np);
            int cp = r->getTag("CP","0").toInt();
            rif = np->value(true) / (cp ? cp : zones->getCP(zoneRange));
            secs = np->count();

            setValue(rif);
            setCount(secs);
        }
    }

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aIntensityFactor(*this); }
};

class aTSS : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aTSS)
    double score;

    public:

    aTSS() : score(0.0)
    {
        setSymbol("a_coggan_tss");
        setInternalName("aTSS");
    }
    void initialize() {
        setName("aTSS");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *r, const Zones *zones, int zoneRange,
                 const HrZones *, int,
	    const QHash<QString,RideMetric*> &deps,
                 const Context *) {
	if (!zones || zoneRange < 0)
	    return;
        assert(deps.contains("a_coggan_np"));
        assert(deps.contains("a_coggan_if"));
        aNP *np = dynamic_cast<aNP*>(deps.value("a_coggan_np"));
        RideMetric *rif = deps.value("a_coggan_if");
        assert(rif);
        double normWork = np->value(true) * np->count();
        double rawTSS = normWork * rif->value(true);
        int cp = r->getTag("CP","0").toInt();
        double workInAnHourAtCP = (cp ? cp : zones->getCP(zoneRange)) * 3600;
        score = rawTSS / workInAnHourAtCP * 100.0;

        setValue(score);
    }

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aTSS(*this); }
};

class aTSSPerHour : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(aTSSPerHour)
    double points;
    double hours;

    public:

    aTSSPerHour() : points(0.0), hours(0.0)
    {
        setSymbol("a_coggan_tssperhour");
        setInternalName("aTSS per hour");
    }
    void initialize() {
        setName(tr("aTSS per hour"));
        setType(RideMetric::Average);
        setPrecision(0);
    }
    void compute(const RideFile *, const Zones *, int ,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

            // tss
            assert(deps.contains("a_coggan_tss"));
            aTSS *tss = dynamic_cast<aTSS*>(deps.value("a_coggan_tss"));
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

    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
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
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        assert(deps.contains("a_coggan_np"));
        assert(deps.contains("average_hr"));
        aNP *np = dynamic_cast<aNP*>(deps.value("a_coggan_np"));
        assert(np);
        RideMetric *ah = dynamic_cast<RideMetric*>(deps.value("average_hr"));
        assert(ah);
        ef = np->value(true) / ah->value(true);

        setValue(ef);
    }
    bool isRelevantForRide(const RideItem*ride) const { return (!ride->isRun && !ride->isSwim); }
    RideMetric *clone() const { return new aEfficiencyFactor(*this); }
};

static bool addAllaCoggan() {
    RideMetricFactory::instance().addMetric(aNP());
    QVector<QString> deps;
    deps.append("a_coggan_np");
    RideMetricFactory::instance().addMetric(aIntensityFactor(), &deps);
    deps.append("a_coggan_if");
    RideMetricFactory::instance().addMetric(aTSS(), &deps);
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

