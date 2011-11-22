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
#include "BestIntervalDialog.h"
#include "Zones.h"
#include "Settings.h"
#include "MetricAggregator.h"
#include <math.h>

#define tr(s) QObject::tr(s)

// first use RideFile::startTime, then Measure then fallback to Global Setting
static double
getWeight(const MainWindow *main, const RideFile *ride)
{
    // ride
    double weight;
    if ((weight = ride->getTag("Weight", "0.0").toDouble()) > 0) {
        return weight;
    }
#if 0
    // withings?
    QList<SummaryMetrics> measures = main->metricDB->getAllMeasuresFor(QDateTime::fromString("Jan 1 00:00:00 1900"), ride->startTime());
    if (measures.count()) {
        return measures.last().getText("Weight", "0.0").toDouble();
    }
#endif

    // global options
    return appsettings->cvalue(main->cyclist, GC_WEIGHT, "75.0").toString().toDouble(); // default to 75kg
}

class AverageWPK : public RideMetric {

    public:

    AverageWPK()
    {
        setSymbol("average_wpk");
        setName(tr("Watts Per Kilogram"));
        setType(RideMetric::Average);
        setMetricUnits(tr("wpk"));
        setImperialUnits(tr("wpk"));
        setPrecision(1);
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *main) {

        // get thos dependencies
        double secs = deps.value("workout_time")->value(true);
        double weight = getWeight(main, ride);
        double ap = deps.value("average_power")->value(true);

        // calclate watts per kilo
        setCount(secs);
        setValue((secs && ap && weight) ? ap/weight : 0);
    }
    RideMetric *clone() const { return new AverageWPK(*this); }
};

class PeakWPK : public RideMetric {
    double wpk;
    double secs;
    double weight;

    public:

    PeakWPK() : wpk(0.0), secs(0.0), weight(0.0)
    {
        setType(RideMetric::Peak);
        setMetricUnits(tr("wpk"));
        setImperialUnits(tr("wpk"));
        setPrecision(1);
    }
    void setSecs(double secs) { this->secs=secs; }
    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const MainWindow *main) {
        weight = getWeight(main, ride);
        //weight = ride->getTag("Weight", appsettings->cvalue(GC_WEIGHT, "75.0").toString()).toDouble(); // default to 75kg
        QList<BestIntervalDialog::BestInterval> results;
        BestIntervalDialog::findBests(ride, secs, 1, results);
        if (results.count() > 0 && results.first().avg < 3000) wpk = results.first().avg / weight;
        else wpk = 0.0;
        setValue(wpk);
    }
    RideMetric *clone() const { return new PeakWPK(*this); }
};

class CPWPK : public PeakWPK {
    public:
        CPWPK()
        {
            setSecs(3600);
            setSymbol("60m_peak_wpk");
            setName(tr("60 min Peak WPK"));
        }
        RideMetric *clone() const { return new CPWPK(*this); }
};

class PeakWPK1s : public PeakWPK {
    public:
        PeakWPK1s()
        {
            setSecs(1);
            setSymbol("1s_peak_wpk");
            setName(tr("1 sec Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK1s(*this); }
};

class PeakWPK5s : public PeakWPK {
    public:
        PeakWPK5s()
        {
            setSecs(5);
            setSymbol("5s_peak_wpk");
            setName(tr("5 sec Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK5s(*this); }
};

class PeakWPK10s : public PeakWPK {
    public:
        PeakWPK10s()
        {
            setSecs(10);
            setSymbol("10s_peak_wpk");
            setName(tr("10 sec Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK10s(*this); }
};

class PeakWPK15s : public PeakWPK {
    public:
        PeakWPK15s()
        {
            setSecs(15);
            setSymbol("15s_peak_wpk");
            setName(tr("15 sec Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK15s(*this); }
};

class PeakWPK20s : public PeakWPK {
    public:
        PeakWPK20s()
        {
            setSecs(20);
            setSymbol("20s_peak_wpk");
            setName(tr("20 sec Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK20s(*this); }
};

class PeakWPK30s : public PeakWPK {
    public:
        PeakWPK30s()
        {
            setSecs(30);
            setSymbol("30s_peak_wpk");
            setName(tr("30 sec Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK30s(*this); }
};

class PeakWPK1m : public PeakWPK {
    public:
        PeakWPK1m()
        {
            setSecs(60);
            setSymbol("1m_peak_wpk");
            setName(tr("1 min Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK1m(*this); }
};

class PeakWPK5m : public PeakWPK {
    public:
        PeakWPK5m()
        {
            setSecs(300);
            setSymbol("5m_peak_wpk");
            setName(tr("5 min Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK5m(*this); }
};

class PeakWPK10m : public PeakWPK {
    public:
        PeakWPK10m()
        {
            setSecs(600);
            setSymbol("10m_peak_wpk");
            setName(tr("10 min Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK10m(*this); }
};

class PeakWPK20m : public PeakWPK {
    public:
        PeakWPK20m()
        {
            setSecs(1200);
            setSymbol("20m_peak_wpk");
            setName(tr("20 min Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK20m(*this); }
};

class PeakWPK30m : public PeakWPK {
    public:
        PeakWPK30m()
        {
            setSecs(1800);
            setSymbol("30m_peak_wpk");
            setName(tr("30 min Peak WPK"));
        }
        RideMetric *clone() const { return new PeakWPK30m(*this); }
};

class Vo2max : public RideMetric {
    public:

    Vo2max()
    {
        setSymbol("vo2max");
        setName(tr("Estimated VO2MAX"));
        setType(RideMetric::Average);
        setMetricUnits(tr("ml/min/kg"));
        setImperialUnits(tr("ml/min/kg"));
    }

    void compute(const RideFile *, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &deps,
                 const MainWindow *) {

        PeakWPK5m *wpk5m = dynamic_cast<PeakWPK5m*>(deps.value("5m_peak_wpk"));
        setValue(wpk5m->value(false) * 12 + 3.5);
        setCount(1);
    }
    RideMetric *clone() const { return new Vo2max(*this); }
};

static bool addAllWPK() {
    RideMetricFactory::instance().addMetric(PeakWPK1s());
    RideMetricFactory::instance().addMetric(PeakWPK5s());
    RideMetricFactory::instance().addMetric(PeakWPK10s());
    RideMetricFactory::instance().addMetric(PeakWPK15s());
    RideMetricFactory::instance().addMetric(PeakWPK20s());
    RideMetricFactory::instance().addMetric(PeakWPK30s());
    RideMetricFactory::instance().addMetric(PeakWPK1m());
    RideMetricFactory::instance().addMetric(PeakWPK5m());
    RideMetricFactory::instance().addMetric(PeakWPK10m());
    RideMetricFactory::instance().addMetric(PeakWPK20m());
    RideMetricFactory::instance().addMetric(PeakWPK30m());
    RideMetricFactory::instance().addMetric(CPWPK());
    QVector<QString> deps;
    deps.append("5m_peak_wpk");
    RideMetricFactory::instance().addMetric(Vo2max(), &deps);
    deps.clear();
    deps.append("average_power");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(AverageWPK(), &deps);
    return true;
}


static bool allWPKAdded = addAllWPK();
