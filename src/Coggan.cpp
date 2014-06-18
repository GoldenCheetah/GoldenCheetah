/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

class NP : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(NP)
    double np;
    double secs;

    public:

    NP() : np(0.0), secs(0.0)
    {
        setSymbol("coggan_np");
        setInternalName("NP");
    }
    void initialize() {
        setName("NP");
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

                sum += ride->dataPoints()[i]->watts;
                sum -= rolling[index];

                rolling[index] = ride->dataPoints()[i]->watts;

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
    RideMetric *clone() const { return new NP(*this); }
};

class VI : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(VI)
    double vi;
    double secs;

    public:

    VI() : vi(0.0), secs(0.0)
    {
        setSymbol("coggam_variability_index");
        setInternalName("VI");
    }
    void initialize() {
        setName("VI");
        setType(RideMetric::Average);
        setPrecision(3);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
            assert(deps.contains("coggan_np"));
            assert(deps.contains("average_power"));
            NP *np = dynamic_cast<NP*>(deps.value("coggan_np"));
            assert(np);
            RideMetric *ap = dynamic_cast<RideMetric*>(deps.value("average_power"));
            assert(ap);
            vi = np->value(true) / ap->value(true);
            secs = np->count();

            setValue(vi);
            setCount(secs);
    }

    RideMetric *clone() const { return new VI(*this); }
};

class IntensityFactor : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(IntensityFactor)
    double rif;
    double secs;

    public:

    IntensityFactor() : rif(0.0), secs(0.0)
    {
        setSymbol("coggan_if");
        setInternalName("IF");
    }
    void initialize() {
        setName("IF");
        setType(RideMetric::Average);
        setPrecision(3);
    }
    void compute(const RideFile *r, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        if (zones && zoneRange >= 0) {
            assert(deps.contains("coggan_np"));
            NP *np = dynamic_cast<NP*>(deps.value("coggan_np"));
            assert(np);
            int cp = r->getTag("CP","0").toInt();
            rif = np->value(true) / (cp ? cp : zones->getCP(zoneRange));
            secs = np->count();

            setValue(rif);
            setCount(secs);
        }
    }

    RideMetric *clone() const { return new IntensityFactor(*this); }
};

class TSS : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TSS)
    double score;

    public:

    TSS() : score(0.0)
    {
        setSymbol("coggan_tss");
        setInternalName("TSS");
    }
    void initialize() {
        setName("TSS");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *r, const Zones *zones, int zoneRange,
                 const HrZones *, int,
	    const QHash<QString,RideMetric*> &deps,
                 const Context *) {
	if (!zones || zoneRange < 0)
	    return;
        assert(deps.contains("coggan_np"));
        assert(deps.contains("coggan_if"));
        NP *np = dynamic_cast<NP*>(deps.value("coggan_np"));
        RideMetric *rif = deps.value("coggan_if");
        assert(rif);
        double normWork = np->value(true) * np->count();
        double rawTSS = normWork * rif->value(true);
        int cp = r->getTag("CP","0").toInt();
        double workInAnHourAtCP = (cp ? cp : zones->getCP(zoneRange)) * 3600;
        score = rawTSS / workInAnHourAtCP * 100.0;

        setValue(score);
    }

    RideMetric *clone() const { return new TSS(*this); }
};

class TSSPerHour : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TSSPerHour)
    double points;
    double hours;

    public:

    TSSPerHour() : points(0.0), hours(0.0)
    {
        setSymbol("coggan_tssperhour");
        setInternalName("TSS per hour");
    }
    void initialize() {
        setName("TSS per hour");
        setType(RideMetric::Average);
        setPrecision(0);
    }
    void compute(const RideFile *r, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {

            // tss
            assert(deps.contains("coggan_tss"));
            TSS *tss = dynamic_cast<TSS*>(deps.value("coggan_tss"));
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

    RideMetric *clone() const { return new TSSPerHour(*this); }
};

class EfficiencyFactor : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(EfficiencyFactor)
    double ef;

    public:

    EfficiencyFactor() : ef(0.0)
    {
        setSymbol("friel_efficiency_factor");
        setInternalName("Efficiency Factor");
    }
    void initialize() {
        setName(tr("Efficiency Factor"));
        setType(RideMetric::Average);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(3);
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const Context *) {
        assert(deps.contains("coggan_np"));
        assert(deps.contains("average_hr"));
        NP *np = dynamic_cast<NP*>(deps.value("coggan_np"));
        assert(np);
        RideMetric *ah = dynamic_cast<RideMetric*>(deps.value("average_hr"));
        assert(ah);
        ef = np->value(true) / ah->value(true);

        setValue(ef);
    }
    RideMetric *clone() const { return new EfficiencyFactor(*this); }
};

static bool addAllCoggan() {
    RideMetricFactory::instance().addMetric(NP());
    QVector<QString> deps;
    deps.append("coggan_np");
    RideMetricFactory::instance().addMetric(IntensityFactor(), &deps);
    deps.append("coggan_if");
    RideMetricFactory::instance().addMetric(TSS(), &deps);
    deps.clear();
    deps.append("coggan_np");
    deps.append("average_power");
    RideMetricFactory::instance().addMetric(VI(), &deps);
    deps.clear();
    deps.append("coggan_np");
    deps.append("average_hr");
    RideMetricFactory::instance().addMetric(EfficiencyFactor(), &deps);
    deps.clear();
    deps.append("coggan_tss");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(TSSPerHour(), &deps);
    return true;
}

static bool CogganAdded = addAllCoggan();

