/*
 * Copyright (c) 2019 Mark Liversedge (liversedge@gmail.com)
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

#include "Banister.h"

#include "RideMetric.h"
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "LTMOutliers.h"
#include "Units.h"
#include "Zones.h"
#include "cmath"
#include <assert.h>
#include <algorithm>
#include <QVector>
#include <QApplication>


const double typical_CP = 261,
             typical_WPrime = 15500,
             typical_Pmax = 1100;

//
// Convert power-duration of test, interval, ride to a percentage comparison
// to the mean athlete from opendata (100% would be a mean effort, whilst a
// score <100 is below average and a score >100 is above average
//
// For TTE tests in the 2-20 minute range we would expect the scores to
// be largely similar; so useful for evaluating a set of maximal efforts
// and also useful to normalise TTEs for use in the Banister model as a
// performance measurement.
//
class PowerIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PowerIndex)
    public:

    PowerIndex()
    {
        setSymbol("power_index");
        setInternalName("Power Index");
        setPrecision(1);
        setType(Average); // not even sure aggregation makes sense
    }
    void initialize() {
        setName(tr("Power Index"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setDescription(tr("Power Index"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // calculate for this interval/ride
        double duration=0, averagepower=0;
        if (item->ride()->areDataPresent()->watts) {

            // loop through and count
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                // use duration as count for now, and accumulate power
                duration++;
                averagepower += point->watts;
            }
            if (duration >0) {
                averagepower /= duration; // convert to AP
                duration *= item->ride()->recIntSecs(); // convert to seconds
            }
        }

        // failed to get duration and average power
        if (duration <= 0 || averagepower <= 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // so now lets work out what the 3p model says the
        // typical athlete would do for the same duration
        //
        // P(t) = W' / (t - (W'/(CP-Pmax))) + CP
        //
        double typicalpower = (typical_WPrime / (duration - (typical_WPrime/(typical_CP-typical_Pmax)))) + typical_CP;

        // make sure we got a sensible value
        if (typicalpower < 0 || typicalpower > 2500) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // we could convert to linear work time model before
        // indexing, but they cancel out so no value in doing so
        setValue(100.0 * averagepower/typicalpower);
        setCount(1);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PowerIndex(*this); }
};

static bool countAdded = RideMetricFactory::instance().addMetric(PowerIndex());
