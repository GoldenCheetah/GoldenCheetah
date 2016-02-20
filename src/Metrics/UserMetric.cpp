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
#include "DataFilter.h"

UserMetric::UserMetric(Context *context, UserMetricSettings settings)
    : RideMetric(), settings(settings)
{
    // compile the program - built in a context that can close.
    program = new DataFilter(NULL, context, settings.program);
    root = program->root();
    rt = &program->rt;

    // lookup functions we need
    finit = rt->functions.contains("init") ? rt->functions.value("init") : NULL;
    frelevant = rt->functions.contains("relevant") ? rt->functions.value("relevant") : NULL;
    fsample = rt->functions.contains("sample") ? rt->functions.value("sample") : NULL;
    fvalue = rt->functions.contains("value") ? rt->functions.value("value") : NULL;
    fcount = rt->functions.contains("count") ? rt->functions.value("count") : NULL;

    // we're not a clone, we're the original
    clone_ = false;
}

UserMetric::UserMetric(const UserMetric *from) : RideMetric()
{
    this->settings = from->settings;
    this->program = from->program;

    this->root = from->program->root();
    this->finit = from->finit;
    this->frelevant = from->frelevant;
    this->fsample = from->fsample;
    this->fvalue = from->fvalue;
    this->fcount = from->fcount;

    this->index_ = from->index_;

    rt = new DataFilterRuntime;

    // and copy it in an atomic operation
    *rt = *from->rt;

    // we are being cloned
    clone_ = true;
}

UserMetric::~UserMetric()
{
    if (!clone_ && program) delete program;
    if (clone_) delete rt;
}

RideMetric *
UserMetric::clone() const
{
    return new UserMetric(this);
}

void
UserMetric::initialize()
{
    return; // nothing doing
}

QString
UserMetric::symbol() const
{
    return settings.symbol;
}

// A short string suitable for showing to the user in the ride
// summaries, configuration dialogs, etc.  It should be translated
// using tr().
QString
UserMetric::name() const
{
    return settings.name;
}

QString
UserMetric::internalName() const
{
    return settings.name;
}

QString
UserMetric::description() const
{
    return settings.description;
}

RideMetric::MetricType
UserMetric::type() const
{
    return static_cast<RideMetric::MetricType>(settings.type);
}

// units
QString
UserMetric::units(bool metric) const
{
    return metric ? settings.unitsMetric : settings.unitsImperial;
}

// Factor to multiple value to convert from metric to imperial
double
UserMetric::conversion() const
{
    return settings.conversion;
}

// And sum for example Fahrenheit from CentigradE
double
UserMetric::conversionSum() const
{
    return settings.conversionSum;
}

// How many digits after the decimal we should show when displaying the
// value of a UserRideMetric.
int
UserMetric::precision() const
{
    return settings.precision;
}

// Get the value and apply conversion if needed
double
UserMetric::value(bool metric) const
{
    if (metric) return value();
    else return (value() * conversion()) + conversionSum();
}

// The internal value of this ride metric, useful to cache and then setValue.
double
UserMetric::value() const
{
    return value_;
}

// for averages the count of items included in the average
double
UserMetric::count() const
{
    return count_;
}

// when aggregating averages, should we include zeroes ? no by default
bool
UserMetric::aggregateZero() const
{
    return settings.aggzero;
}

// is this metric relevant
bool
UserMetric::isRelevantForRide(const RideItem *item) const
{
    if (item->context && root) {
        if (frelevant) {
            Result res = root->eval(rt, frelevant, 0, const_cast<RideItem*>(item), NULL);
            return res.number;
        } else
            return true;
    }
    return false;
}

// Compute the ride metric from a file.
void
UserMetric::compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &pc)
{
    QTime timer;
    timer.start();

    //qDebug()<<"CODE";
    if (!root) {
        setValue(RideFile::NIL);
        setCount(0);
        return;
    }

    // if there are no precomputed metrics then just use the values for the rideitem
    // this is a specific use case when testing a user metric in preferences
    const QHash<QString, RideMetric*> *c = NULL;
    if (pc.count()) c=&pc;

    //qDebug()<<"INIT";
    // always init first
    if (finit) root->eval(rt, finit, 0, const_cast<RideItem*>(item), NULL, c);

    //qDebug()<<"CHECK";
    // can it provide a value and is it relevant ?
    if (!fvalue || !isRelevantForRide(item)) {
        setValue(RideFile::NIL);
        setCount(0);
        return;
    }

    //qDebug()<<"SAMPLE";
    // process samples, if there are any and a function exists
    if (!spec.isEmpty(item->ride()) && fsample) {
        RideFileIterator it(item->ride(), spec);

        while(it.hasNext()) {
            struct RideFilePoint *point = it.next();
            root->eval(rt, fsample, 0, const_cast<RideItem*>(item), point, c);
        }
    }

    //qDebug()<<"VALUE";
    // value ?
    if (fvalue) {
        Result v = root->eval(rt, fvalue, 0, const_cast<RideItem*>(item), NULL, c);
        setValue(v.number);
    }

    //qDebug()<<"COUNT";
    // count?
    if (fcount) {
        Result n = root->eval(rt, fcount, 0, const_cast<RideItem*>(item), NULL, c);
        setCount(n.number);
    }

    //qDebug()<<symbol()<<index_<<value_<<"ELAPSED="<<timer.elapsed()<<"ms";
}


bool
UserMetric::isTime() const
{
    return settings.istime;
}
