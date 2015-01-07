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
    }

    void compute(const RideFile *ride, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        total = count = 0;

        foreach (const RideFilePoint *point, ride->dataPoints())  {
            if (point->cad && point->lrbalance > 0.0f) {
                total += point->lrbalance;
                ++count;
            }

        }
        setValue(count > 0 ? total / count : 0);
        setCount(count);
    }

    bool isRelevantForRide(const RideFile *ride) const { return ride->areDataPresent()->lrbalance; }


    RideMetric *clone() const { return new LeftRightBalance(*this); }
};

static bool leftRightBalanceAdded =
    RideMetricFactory::instance().addMetric(LeftRightBalance());
