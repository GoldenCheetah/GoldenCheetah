/*
 * Copyright (c) 2016 Damien Grauser (Damien.Grauser@gmail.com)
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
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "RideItem.h"
#include "LTMOutliers.h"
#include "Units.h"
#include "Zones.h"
#include "cmath"
#include <assert.h>
#include <algorithm>
#include <QVector>
#include <QApplication>



struct AvgRunCadence : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(AvgRunCadence)

    double total, count;

    public:

    AvgRunCadence()
    {
        setSymbol("average_run_cad");
        setInternalName("Average Running Cadence");
    }

    void initialize() {
        setName(tr("Average Running Cadence"));
        setMetricUnits(tr("spm"));
        setImperialUnits(tr("spm"));
        setType(RideMetric::Average);
        setDescription(tr("Average Running Cadence, computed when Cadence > 0"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        total = count = 0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();

            if (point->cad > 0) {
                total += point->cad;
                ++count;
            } else if (point->rcad > 0) {
                total += point->rcad;
                ++count;
            }
        }
        setValue(count > 0 ? total / count : count);
        setCount(count);
    }

    bool isRelevantForRide(const RideItem *ride) const { return (ride->present.contains("C") && ride->isRun); }

    RideMetric *clone() const { return new AvgRunCadence(*this); }
};

static bool avgRunCadenceAdded =
    RideMetricFactory::instance().addMetric(AvgRunCadence());

//////////////////////////////////////////////////////////////////////////////

class MaxRunCadence : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxRunCadence)
    public:

    MaxRunCadence()
    {
        setSymbol("max_run_cadence");
        setInternalName("Max Running Cadence");
    }

    void initialize() {
        setName(tr("Max Running Cadence"));
        setMetricUnits(tr("spm"));
        setImperialUnits(tr("spm"));
        setType(RideMetric::Peak);
        setDescription(tr("Maximum Running Cadence"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        double max = 0.0;

        RideFileIterator it(item->ride(), spec);
        while (it.hasNext()) {
            struct RideFilePoint *point = it.next();
            if (point->cad > max) max = point->cad;
            if (point->rcad > max) max = point->rcad;
        }

        setValue(max);
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("C") && ride->isRun; }

    RideMetric *clone() const { return new MaxRunCadence(*this); }
};

static bool maxRunCadenceAdded =
    RideMetricFactory::instance().addMetric(MaxRunCadence());

//////////////////////////////////////////////////////////////////////////////

