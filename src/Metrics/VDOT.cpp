/*
 * Copyright (c) 2015 Alejandro Martinez (amtriathlon@gmail.com)
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
#include "Units.h"
#include "Settings.h"
#include "RideFileCache.h"
#include "VDOTCalculator.h"
#include "RideItem.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include <cmath>
#include <assert.h>
#include <QApplication>

// Daniels VDOT
class VDOT : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(VDOT)
    double vdot;

    public:

    VDOT() : vdot(0.0)
    {
        setSymbol("VDOT");
        setInternalName("VDOT");
    }
    void initialize() {
        setName(tr("VDOT"));
        setMetricUnits(tr("ml/min/kg"));
        setImperialUnits(tr("ml/min/kg"));
        setType(RideMetric::Peak);
        setPrecision(1);
        setDescription(tr("Daniels' VDOT computed from best average pace for durations from 4 min 4 hr"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {

        // not a run
        if (!item->isRun) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // build a cache -- VDOT does not work with intervals !
        RideFileCache rfc(item->ride());

        // search for max VDOT from 4 min to 4 hr
        vdot = 0.0;
        int iMax = std::min((int)rfc.meanMaxArray(RideFile::kph).size(), 14400);
        for (int i = 240; i < iMax; i++) {
            double vel = rfc.meanMaxArray(RideFile::kph)[i]*1000.0/60.0;
            vdot = std::max(vdot, VDOTCalculator::vdot(i / 60.0, vel));
        }
        setValue(vdot);
        setCount(1);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new VDOT(*this); }
};

// Daniels T-Pace
class TPace : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(TPace)
    double tPace;

    public:

    TPace() : tPace(0.0)
    {
        setSymbol("TPace");
        setInternalName("TPace");
    }
    // TPace ordering is reversed
    bool isLowerBetter() const { return true; }
    // Overrides to use Pace units setting
    QString units(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::units(metricRunPace);
    }
    double value(bool) const {
        bool metricRunPace = appsettings->value(NULL, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
        return RideMetric::value(metricRunPace);
    }
    QString toString(bool metric) const {
        return time_to_string(value(metric)*60);
    }
    void initialize() {
        setName(tr("TPace"));
        setType(RideMetric::Low);
        setMetricUnits(tr("min/km"));
        setImperialUnits(tr("min/mile"));
        setPrecision(1);
        setConversion(KM_PER_MILE);
        setDescription(tr("Daniels' TPace, computed as 90%vVDOT from VDOT metric, in min/km or min/mile"));
    }


    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &deps) {

        // not a run
        if (!item->isRun) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }
        assert(deps.contains("VDOT"));
        VDOT *vdot = dynamic_cast<VDOT*>(deps.value("VDOT"));
        assert(vdot);
        // TPace corresponds to 90%vVDOT
        if (vdot->value() > 0) {
            tPace = 1000.0/VDOTCalculator::vVdot(vdot->value())/0.9;
        } else {
            tPace = 0.0;
        }
        setValue(tPace);
        setCount(1);
    }
    bool isRelevantForRide(const RideItem *ride) const { return ride->isRun; }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new TPace(*this); }
};

static bool added() {
    RideMetricFactory::instance().addMetric(VDOT());
    QVector<QString> deps;
    deps.append("VDOT");
    RideMetricFactory::instance().addMetric(TPace(), &deps);
    return true;
}

static bool added_ = added();
