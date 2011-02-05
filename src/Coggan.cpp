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

#define tr(s) QObject::tr(s)

class NP : public RideMetric {
    double np;
    double secs;

    public:

    NP() : np(0.0), secs(0.0)
    {
        setSymbol("coggan_np");
        setName("NP");
        setType(RideMetric::Average);
        setMetricUnits("watts");
        setImperialUnits("watts");
        setPrecision(0);
    }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *) {
        QVector<double> last30secs;

        double secsDelta = ride->recIntSecs();
        if(secsDelta == 0)
            return;
        double sampsPerWindow = ceil(30.0 / secsDelta);
        last30secs.resize(sampsPerWindow + 1); // add 1 just in case

        double total = 0.0;
        int count = 0;

        foreach(const RideFilePoint *point, ride->dataPoints()) {
            if (count < sampsPerWindow) {
                last30secs[count] = point->watts;
                count++;
            } else {

                double rolling = 0.0;
                for (int i=0; i<sampsPerWindow; i++) {
                    rolling += last30secs[i];
                }
                rolling /= sampsPerWindow;
                total += pow(rolling, 4.0);

                last30secs.remove(0);
                last30secs.resize(sampsPerWindow+1); // add 1 to the end
                last30secs[sampsPerWindow-1] = point->watts;
                count++;
            }
        }
        if (count > sampsPerWindow)
            np = pow(total / (count-sampsPerWindow), 0.25);
        secs = count * secsDelta;

        setValue(np);
        setCount(secs);
    }
    RideMetric *clone() const { return new NP(*this); }
};

class VI : public RideMetric {
    double vi;
    double secs;

    public:

    VI() : vi(0.0), secs(0.0)
    {
        setSymbol("coggam_variability_index");
        setName("VI");
        setType(RideMetric::Average);
        setPrecision(3);
    }
    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {
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
    double rif;
    double secs;

    public:

    IntensityFactor() : rif(0.0), secs(0.0)
    {
        setSymbol("coggan_if");
        setName("IF");
        setType(RideMetric::Average);
        setPrecision(3);
    }
    void compute(const RideFile *, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {
        if (zones && zoneRange >= 0) {
            assert(deps.contains("coggan_np"));
            NP *np = dynamic_cast<NP*>(deps.value("coggan_np"));
            assert(np);
            rif = np->value(true) / zones->getCP(zoneRange);
            secs = np->count();

            setValue(rif);
            setCount(secs);
        }
    }

    RideMetric *clone() const { return new IntensityFactor(*this); }
};

class TSS : public RideMetric {
    double score;

    public:

    TSS() : score(0.0)
    {
        setSymbol("coggan_tss");
        setName("TSS");
        setType(RideMetric::Total);
    }
    void compute(const RideFile *, const Zones *zones, int zoneRange,
                 const HrZones *, int,
	    const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {
	if (!zones || zoneRange < 0)
	    return;
        assert(deps.contains("coggan_np"));
        assert(deps.contains("coggan_if"));
        NP *np = dynamic_cast<NP*>(deps.value("coggan_np"));
        RideMetric *rif = deps.value("coggan_if");
        assert(rif);
        double normWork = np->value(true) * np->count();
        double rawTSS = normWork * rif->value(true);
        double workInAnHourAtCP = zones->getCP(zoneRange) * 3600;
        score = rawTSS / workInAnHourAtCP * 100.0;

        setValue(score);
    }

    RideMetric *clone() const { return new TSS(*this); }
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
    return true;
}

static bool CogganAdded = addAllCoggan();

