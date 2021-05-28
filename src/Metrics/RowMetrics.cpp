/*
 * Copyright (c) 2021 Alejandro Martinez (amtriathlon@gmail.com)
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
#include "RideFile.h"
#include "LTMOutliers.h"
#include "Units.h"
#include "Zones.h"
#include "cmath"
#include <assert.h>
#include <QApplication>

//////////////////////////////////////////////////////////////////////////////

class PaceRow : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PaceRow)
    double pace;

    public:

    PaceRow() : pace(0.0)
    {
        setSymbol("pace_row");
        setInternalName("Pace Row");
    }

    // PaceRow ordering is reversed
    bool isLowerBetter() const { return true; }

    QString toString(bool) const {
        return time_to_string(value(true)*60, true);
    }

    QString toString(double v) const {
        return time_to_string(v*60, true);
    }

    void initialize() {
        setName(tr("Pace Row"));
        setType(RideMetric::Average);
        setMetricUnits(tr("min/500m"));
        setImperialUnits(tr("min/500m"));
        setPrecision(1);
        setConversion(1);
        setDescription(tr("Average Speed expressed in min/500m"));
   }

    void compute(RideItem *, Specification, const QHash<QString,RideMetric*> &deps) {

        RideMetric *as = deps.value("average_speed");

        // divide by zero or stupidly low pace
        if (as->value(true) > 0.00f) pace = 30.0f / as->value(true);
        else pace = 0;

        setValue(pace);
        setCount(as->count());
    }

    bool isRelevantForRide(const RideItem *ride) const { return ride->sport == "Row"; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PaceRow(*this); }
};

static bool addPaceRow()
{
    QVector<QString> deps;
    deps.append("average_speed");
    RideMetricFactory::instance().addMetric(PaceRow(), &deps);
    return true;
}
static bool paceAdded = addPaceRow();

//////////////////////////////////////////////////////////////////////////////
