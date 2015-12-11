/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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
#include "UserMetricSettings.h"

UserMetric::UserMetric(UserMetricSettings settings)
{
}

UserMetric::~UserMetric()
{
}

void
UserMetric::initialize()
{
}

QString
UserMetric::symbol() const
{
    return "";
}

// A short string suitable for showing to the user in the ride
// summaries, configuration dialogs, etc.  It should be translated
// using tr().
QString
UserMetric::name() const
{
    return "";
}

QString
UserMetric::internalName() const
{
    return "";
}

RideMetric::MetricType
UserMetric::type() const
{
}

// units
QString
UserMetric::units(bool metric) const
{
    return "";
}

// Factor to multiple value to convert from metric to imperial
double
UserMetric::conversion() const
{
    return 1;
}

// And sum for example Fahrenheit from CentigradE
double
UserMetric::conversionSum() const
{
    return 0;
}

// How many digits after the decimal we should show when displaying the
// value of a UserRideMetric.
int
UserMetric::precision() const
{
    return 0;
}

// Get the value and apply conversion if needed
double
UserMetric::value(bool metric) const
{
    return 0;
}

// The internal value of this ride metric, useful to cache and then setValue.
double
UserMetric::value() const
{
    return 0;
}

// for averages the count of items included in the average
double
UserMetric::count() const
{
    return 0;
}

// when aggregating averages, should we include zeroes ? no by default
bool
UserMetric::aggregateZero() const
{
    return true;
}

// is this metric relevant
bool
UserMetric::isRelevantForRide(const RideItem *) const
{
    return true;
}

// Compute the ride metric from a file.
void
UserMetric::compute(const RideFile *ride,
                         const Zones *zones, int zoneRange,
                         const HrZones *hrzones, int hrzoneRange,
                         const QHash<QString,RideMetric*> &deps,
                         const Context *context)
{
}


bool
UserMetric::isTime() const
{
    return false;
}
