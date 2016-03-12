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
#include "RideItem.h"
#include "Zones.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include "Settings.h"
#include <cmath>
#include <assert.h>
#include <QApplication>

class AverageWPK : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AverageWPK)

    public:

    AverageWPK()
    {
        setSymbol("average_wpk");
        setInternalName("Watts Per Kilogram");
    }
    void initialize () {
        setName(tr("Watts Per Kilogram"));
        setType(RideMetric::Average);
        setMetricUnits(tr("w/kg"));
        setImperialUnits(tr("w/kg"));
        setPrecision(2);
        setDescription(tr("Average Power relative to Athlete Weight."));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // get thos dependencies
        double secs = deps.value("workout_time")->value(true);
        double weight = item->ride()->getWeight();
        double ap = deps.value("average_power")->value(true);

        // calculate watts per kilo
        setCount(secs);
        setValue((secs && ap && weight) ? ap/weight : 0);
    }
    RideMetric *clone() const { return new AverageWPK(*this); }
};

class PeakWPK : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK)
    double wpk;
    double secs;
    double weight;

    public:

    PeakWPK() : wpk(0.0), secs(0.0), weight(0.0)
    {
        setType(RideMetric::Peak);
        setMetricUnits(tr("w/kg"));
        setImperialUnits(tr("w/kg"));
        setPrecision(2);
    }
    void setSecs(double secs) { this->secs=secs; }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride()) || !item->ride()->areDataPresent()->watts) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        weight = item->ride()->getWeight();
        //weight = ride->getTag("Weight", appsettings->cvalue(GC_WEIGHT, "75.0").toString()).toDouble(); // default to 75kg
        QList<BestIntervalDialog::BestInterval> results;
        BestIntervalDialog::findBests(item->ride(), spec, secs, 1, results);
        if (results.count() > 0 && results.first().avg < 3000) wpk = results.first().avg / weight;
        else wpk = 0.0;
        setValue(wpk);
    }
    RideMetric *clone() const { return new PeakWPK(*this); }
};

class CPWPK : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(CPWPK)
    public:
        CPWPK()
        {
            setSecs(3600);
            setSymbol("60m_peak_wpk");
            setInternalName("60 min Peak WPK");
        }
        void initialize () {
            setName(tr("60 min Peak WPK"));
            setDescription(tr("60 min Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new CPWPK(*this); }
};

class PeakWPK1s : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK1s)
    public:
        PeakWPK1s()
        {
            setSecs(1);
            setSymbol("1s_peak_wpk");
            setInternalName("1 sec Peak WPK");
        }
        void initialize () {
            setName(tr("1 sec Peak WPK"));
            setDescription(tr("1 sec Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK1s(*this); }
};

class PeakWPK5s : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK5s)
    public:
        PeakWPK5s()
        {
            setSecs(5);
            setSymbol("5s_peak_wpk");
            setInternalName("5 sec Peak WPK");
        }
        void initialize () {
            setName(tr("5 sec Peak WPK"));
            setDescription(tr("5 sec Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK5s(*this); }
};

class PeakWPK10s : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK10s)
    public:
        PeakWPK10s()
        {
            setSecs(10);
            setSymbol("10s_peak_wpk");
            setInternalName("10 sec Peak WPK");
        }
        void initialize () {
            setName(tr("10 sec Peak WPK"));
            setDescription(tr("10 sec Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK10s(*this); }
};

class PeakWPK15s : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK15s)
    public:
        PeakWPK15s()
        {
            setSecs(15);
            setSymbol("15s_peak_wpk");
            setInternalName("15 sec Peak WPK");
        }
        void initialize () {
            setName(tr("15 sec Peak WPK"));
            setDescription(tr("15 sec Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK15s(*this); }
};

class PeakWPK20s : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK20s)
    public:
        PeakWPK20s()
        {
            setSecs(20);
            setSymbol("20s_peak_wpk");
            setInternalName("20 sec Peak WPK");
        }
        void initialize () {
            setName(tr("20 sec Peak WPK"));
            setDescription(tr("20 sec Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK20s(*this); }
};

class PeakWPK30s : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK30s)
    public:
        PeakWPK30s()
        {
            setSecs(30);
            setSymbol("30s_peak_wpk");
            setInternalName("30 sec Peak WPK");
        }
        void initialize () {
            setName(tr("30 sec Peak WPK"));
            setDescription(tr("30 sec Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK30s(*this); }
};

class PeakWPK1m : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK1m)
    public:
        PeakWPK1m()
        {
            setSecs(60);
            setSymbol("1m_peak_wpk");
            setInternalName("1 min Peak WPK");
        }
        void initialize () {
            setName(tr("1 min Peak WPK"));
            setDescription(tr("1 min Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK1m(*this); }
};

class PeakWPK5m : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK5m)
    public:
        PeakWPK5m()
        {
            setSecs(300);
            setSymbol("5m_peak_wpk");
            setInternalName("5 min Peak WPK");
        }
        void initialize () {
            setName(tr("5 min Peak WPK"));
            setDescription(tr("5 min Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK5m(*this); }
};

class PeakWPK10m : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK10m)
    public:
        PeakWPK10m()
        {
            setSecs(600);
            setSymbol("10m_peak_wpk");
            setInternalName("10 min Peak WPK");
        }
        void initialize () {
            setName(tr("10 min Peak WPK"));
            setDescription(tr("10 min Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK10m(*this); }
};

class PeakWPK20m : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK20m)
    public:
        PeakWPK20m()
        {
            setSecs(1200);
            setSymbol("20m_peak_wpk");
            setInternalName("20 min Peak WPK");
        }
        void initialize () {
            setName(tr("20 min Peak WPK"));
            setDescription(tr("20 min Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK20m(*this); }
};

class PeakWPK30m : public PeakWPK {
    Q_DECLARE_TR_FUNCTIONS(PeakWPK30m)
    public:
        PeakWPK30m()
        {
            setSecs(1800);
            setSymbol("30m_peak_wpk");
            setInternalName("30 min Peak WPK");
        }
        void initialize () {
            setName(tr("30 min Peak WPK"));
            setDescription(tr("30 min Peak Power relative to Athlete Weight."));
        }
        RideMetric *clone() const { return new PeakWPK30m(*this); }
};

class Vo2max : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(Vo2max)
    public:

    Vo2max()
    {
        setSymbol("vo2max");
        setInternalName("Estimated VO2MAX");
    }
    void initialize () {
        setName(tr("Estimated VO2MAX"));
        setType(RideMetric::Peak);
        setMetricUnits(tr("ml/min/kg"));
        setImperialUnits(tr("ml/min/kg"));
        setDescription(tr("Estimated VO2max from 5 min Peak Power relative to Athlete Weight using new ACSM formula: 10.8 * Watts / KG + 7 (3.5 per leg)."));
    }

    void compute(RideItem* /*item*/, Specification, const QHash<QString,RideMetric*> &deps) {

        PeakWPK5m *wpk5m = dynamic_cast<PeakWPK5m*>(deps.value("5m_peak_wpk"));

        // Using New ACSM formula also outlined here:
        // http://blue.utb.edu/mbailey/pdf/MetCalnew.pdf
        // 10.8 * Watts / KG + 7 (3.5 per leg)
        setValue(0);
        setCount(0);
        if (wpk5m->value(false) > 0) {
            setValue(10.8 * wpk5m->value(false) + 7);
            setCount(1);
        }
    }
    bool isRelevantForRide(const RideItem *ride) const {return ride->present.contains("P"); }
    RideMetric *clone() const { return new Vo2max(*this); }
};

class EtimatedAverageWPK_DrF : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(EtimatedAverageWPK_DrF)

    public:

    EtimatedAverageWPK_DrF()
    {
        setSymbol("estimated_average_wpk_drf");
        setInternalName("estimated Watts Per Kilogram (DrF.)");
    }
    void initialize () {
        setName(tr("estimated Watts Per Kilogram (DrF.)"));
        setType(RideMetric::Average);
        setMetricUnits(tr("w/kg"));
        setImperialUnits(tr("w/kg"));
        setPrecision(2);
        setDescription(tr("Estimated Average Power relative to Athlete Weight using Dr Ferrari formula based on VAM for gradient higher than or equal to 7%: VAM / (2 + (gradient/10)) / 100"));
    }

    void compute(RideItem* /*item*/, Specification, const QHash<QString,RideMetric*> &deps) {

        // get thos dependencies
        double vam = deps.value("vam")->value(true);
        double gradient = deps.value("gradient")->value(true);
        double secs = deps.value("workout_time")->value(true);


        // Calculated power output (W/kg) = [VAM (m/hour)] / [((2 + (% grade/10)) x 100)]
        setValue( gradient>=7 ? vam / 100.0 / (2 + gradient/10.0 ) : 0 );

        setCount(secs);
    }

    bool isRelevantForRide(const RideItem *ride) const {return ride->present.contains("P"); }

    RideMetric *clone() const { return new EtimatedAverageWPK_DrF(*this); }
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

    // Added for testing purpose
    // Work pretty well but
    // is it really a metric ?
    /*deps.clear();
    deps.append("vam");
    deps.append("gradient");
    deps.append("workout_time");
    RideMetricFactory::instance().addMetric(EtimatedAverageWPK_DrF(), &deps);*/

    return true;
}


static bool allWPKAdded = addAllWPK();
