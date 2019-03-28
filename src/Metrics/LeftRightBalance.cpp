/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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
#include "RideItem.h"
#include "Context.h"
#include "Athlete.h"
#include "Specification.h"
#include <cmath>
#include <QApplication>

class LeftRightBalance : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(LeftRightBalance)
    double count, total;

    public:

    LeftRightBalance()
    {
        setSymbol("left_right_balance");
        setInternalName("Left/Right Balance");
    }

    void initialize() {
        setName(tr("Left/Right Balance"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setType(RideMetric::Average);
        setPrecision(1);
        setDescription(tr("Left/Right Balance shows the proportion of power coming from each pedal for rides and the proportion of Ground Contact Time from each leg for runs."));
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
            if ((point->cad || point->rcad) && point->lrbalance > 0.0f) {
                total += point->lrbalance;
                ++count;
            }

        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    QString toString(bool useMetricUnits) const
    {
        double v1 = value(useMetricUnits);
        double v2 = 100-v1;
        return QString("%1-%2").arg(v1, 0, 'f', this->precision()).arg(v2, 0, 'f', this->precision());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("V"); }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new LeftRightBalance(*this); }
};

static bool leftRightBalanceAdded =
    RideMetricFactory::instance().addMetric(LeftRightBalance());
