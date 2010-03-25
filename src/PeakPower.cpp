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
#include <math.h>

#define tr(s) QObject::tr(s)

class PeakPower : public RideMetric {
    double watts;
    double secs;

    public:

    PeakPower() : watts(0.0), secs(0.0)
    {
        setType(RideMetric::Peak);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
    }
    void setSecs(double secs) { this->secs=secs; }
    void compute(const RideFile *ride, const Zones *, int,
                 const QHash<QString,RideMetric*> &) {
        QList<BestIntervalDialog::BestInterval> results;
        BestIntervalDialog::findBests(ride, secs, 1, results);
        if (results.count() > 0 && results.first().avg < 3000) watts = results.first().avg;
        else watts = 0.0;
        setValue(watts);
    }
    RideMetric *clone() const { return new PeakPower(*this); }
};

class CriticalPower : public PeakPower {
    public:
        CriticalPower()
        {
            setSecs(3600);
            setSymbol("60m_critical_power");
            setName(tr("60 min Peak Power"));
        }
        RideMetric *clone() const { return new CriticalPower(*this); }
};

class PeakPower1s : public PeakPower {
    public:
        PeakPower1s()
        {
            setSecs(1);
            setSymbol("1s_critical_power");
            setName(tr("1 sec Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower1s(*this); }
};

class PeakPower5s : public PeakPower {
    public:
        PeakPower5s()
        {
            setSecs(5);
            setSymbol("5s_critical_power");
            setName(tr("5 sec Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower5s(*this); }
};

class PeakPower10s : public PeakPower {
    public:
        PeakPower10s()
        {
            setSecs(10);
            setSymbol("10s_critical_power");
            setName(tr("10 sec Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower10s(*this); }
};

class PeakPower15s : public PeakPower {
    public:
        PeakPower15s()
        {
            setSecs(15);
            setSymbol("15s_critical_power");
            setName(tr("15 sec Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower15s(*this); }
};

class PeakPower20s : public PeakPower {
    public:
        PeakPower20s()
        {
            setSecs(20);
            setSymbol("20s_critical_power");
            setName(tr("20 sec Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower20s(*this); }
};

class PeakPower30s : public PeakPower {
    public:
        PeakPower30s()
        {
            setSecs(30);
            setSymbol("30s_critical_power");
            setName(tr("30 sec Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower30s(*this); }
};

class PeakPower1m : public PeakPower {
    public:
        PeakPower1m()
        {
            setSecs(60);
            setSymbol("1m_critical_power");
            setName(tr("1 min Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower1m(*this); }
};

class PeakPower5m : public PeakPower {
    public:
        PeakPower5m()
        {
            setSecs(300);
            setSymbol("5m_critical_power");
            setName(tr("5 min Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower5m(*this); }
};

class PeakPower10m : public PeakPower {
    public:
        PeakPower10m()
        {
            setSecs(600);
            setSymbol("10m_critical_power");
            setName(tr("10 min Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower10m(*this); }
};

class PeakPower20m : public PeakPower {
    public:
        PeakPower20m()
        {
            setSecs(1200);
            setSymbol("20m_critical_power");
            setName(tr("20 min Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower20m(*this); }
};

class PeakPower30m : public PeakPower {
    public:
        PeakPower30m()
        {
            setSecs(1800);
            setSymbol("30m_critical_power");
            setName(tr("30 min Peak Power"));
        }
        RideMetric *clone() const { return new PeakPower30m(*this); }
};

static bool addAllPeaks() {
    RideMetricFactory::instance().addMetric(PeakPower1s());
    RideMetricFactory::instance().addMetric(PeakPower5s());
    RideMetricFactory::instance().addMetric(PeakPower10s());
    RideMetricFactory::instance().addMetric(PeakPower15s());
    RideMetricFactory::instance().addMetric(PeakPower20s());
    RideMetricFactory::instance().addMetric(PeakPower30s());
    RideMetricFactory::instance().addMetric(PeakPower1m());
    RideMetricFactory::instance().addMetric(PeakPower5m());
    RideMetricFactory::instance().addMetric(PeakPower10m());
    RideMetricFactory::instance().addMetric(PeakPower20m());
    RideMetricFactory::instance().addMetric(PeakPower30m());
    RideMetricFactory::instance().addMetric(CriticalPower());
    return true;
}

static bool allPeaksAdded = addAllPeaks();
