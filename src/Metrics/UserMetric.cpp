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
    program->refcount = 1;
    root = program->root();
    rt = &program->rt;

    // lookup functions we need
    finit = rt->functions.contains("init") ? rt->functions.value("init") : NULL;
    frelevant = rt->functions.contains("relevant") ? rt->functions.value("relevant") : NULL;
    fsample = rt->functions.contains("sample") ? rt->functions.value("sample") : NULL;
    fbefore = rt->functions.contains("before") ? rt->functions.value("before") : NULL;
    fafter = rt->functions.contains("after") ? rt->functions.value("after") : NULL;
    fvalue = rt->functions.contains("value") ? rt->functions.value("value") : NULL;
    fcount = rt->functions.contains("count") ? rt->functions.value("count") : NULL;

    // we're not a clone, we're the original
    clone_ = false;
}

UserMetric::UserMetric(const UserMetric *from) : RideMetric()
{
    RideMetricFactory::instance().mutex.lock();
    this->settings = from->settings;
    this->program = from->program;
    this->program->refcount++;

    this->root = from->program->root();
    this->finit = from->finit;
    this->frelevant = from->frelevant;
    this->fsample = from->fsample;
    this->fbefore = from->fbefore;
    this->fafter = from->fafter;
    this->fvalue = from->fvalue;
    this->fcount = from->fcount;

    this->index_ = from->index_;

    rt = new DataFilterRuntime;

    // and copy it in an atomic operation
    *rt = *from->rt;

    // we are being cloned
    clone_ = true;
    RideMetricFactory::instance().mutex.unlock();
}

UserMetric::~UserMetric()
{
    // program is shared, only delete when last is destroyed
    RideMetricFactory::instance().mutex.lock();
    if (program) {
        program->refcount--;
        if (!program->refcount) delete program;
    }
    if (clone_) delete rt;
    RideMetricFactory::instance().mutex.unlock();
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

// Apply conversion if needed to the supplied value
double
UserMetric::value(double v, bool metric) const
{
    if (metric) return v;
    else return (v * conversion()) + conversionSum();
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
            Result res = root->eval(rt, frelevant, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL);
            return res.number();
        } else
            return true;
    }
    return false;
}

// Compute the ride metric from a file.
void
UserMetric::compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &pc)
{
    QElapsedTimer timer;
    timer.start();

    //qDebug()<<"CODE";
    if (!root) {
        setValue(RideFile::NIL);
        setCount(0);
        return;
    }

    // clear rt indexes
    rt->indexes.clear();

    // if there are no precomputed metrics then just use the values for the rideitem
    // this is a specific use case when testing a user metric in preferences
    const QHash<QString, RideMetric*> *c = NULL;
    if (pc.count()) c=&pc;

    //qDebug()<<"INIT";
    // always init first
    if (finit) root->eval(rt, finit, Result(0), 0, const_cast<RideItem*>(item), NULL, c, spec);

    //qDebug()<<"CHECK";
    // can it provide a value and is it relevant ?
    if (!fvalue || !isRelevantForRide(item)) {
        setValue(RideFile::NIL);
        setCount(0);
        return;
    }

    //qDebug()<<"BEFORE";
    if (!spec.isEmpty(item->ride()) && fbefore) {
        RideFileIterator it(item->ride(), spec, RideFileIterator::Before);

        while(it.hasNext()) {
            struct RideFilePoint *point = it.next();
            root->eval(rt, fbefore, Result(0), 0, const_cast<RideItem*>(item), point, c, spec);
        }
    }

    //qDebug()<<"SAMPLE";
    // process samples, if there are any and a function exists
    if (!spec.isEmpty(item->ride()) && fsample) {
        RideFileIterator it(item->ride(), spec);

        while(it.hasNext()) {
            struct RideFilePoint *point = it.next();
            root->eval(rt, fsample, Result(0), 0, const_cast<RideItem*>(item), point, c, spec);
        }
    }

    //qDebug()<<"AFTER";
    if (!spec.isEmpty(item->ride()) && fafter) {
        RideFileIterator it(item->ride(), spec, RideFileIterator::After);

        while(it.hasNext()) {
            struct RideFilePoint *point = it.next();
            root->eval(rt, fafter, Result(0), 0, const_cast<RideItem*>(item), point, c, spec);
        }
    }


    //qDebug()<<"VALUE";
    // value ?
    if (fvalue) {
        Result v = root->eval(rt, fvalue, Result(0), 0, const_cast<RideItem*>(item), NULL, c, spec);
        setValue(v.number());
    }

    //qDebug()<<"COUNT";
    // count?
    if (fcount) {
        Result n = root->eval(rt, fcount, Result(0), 0, const_cast<RideItem*>(item), NULL, c, spec);
        setCount(n.number());
    }

    //qDebug()<<symbol()<<index_<<value_<<"ELAPSED="<<timer.elapsed()<<"ms";
}


bool
UserMetric::isTime() const
{
    return settings.istime;
}

void
UserMetric::addCompatibility(QList<UserMetricSettings>&metrics)
{
    // update the list of compatibility metrics
    RideMetricFactory &f = RideMetricFactory::instance();

    // reset, get added below ONLY if user didn't define them
    f.compatibilitymetrics.clear();

    // add metrics for backwards compatibility if they are not
    // already defined by the user
    bool hasc1=false, hasc2=false, hasc3=false;
    foreach(UserMetricSettings x, metrics) {
        if (x.name == "TSS") hasc1=true;
        if (x.name == "IF") hasc2=true;
        if (x.name == "NP") hasc3=true;
    }

    if (!hasc1) {
        UserMetricSettings c1;

        // add backwards compatibility metrics
        c1.symbol = "compatibility_1";
        c1.name = "TSS";
        c1.description = "BikeStress";
        c1.type = 0;
        c1.unitsMetric = "";
        c1.unitsImperial = "";
        c1.conversion = 1;
        c1.conversionSum = 0;
        c1.program = "{ value { BikeStress; } count { Duration; } }";
        c1.precision = 0;
        c1.istime = false;
        c1.aggzero = true;
        c1.fingerprint = c1.symbol + DataFilter::fingerprint(c1.program);
        metrics.insert(0, c1);
        f.compatibilitymetrics << "TSS";
    }

    if (!hasc2) {
        UserMetricSettings c2;

        // add backwards compatibility metrics
        c2.symbol = "compatibility_2";
        c2.name = "IF";
        c2.type = 1;
        c2.description = "BikeIntensity";
        c2.unitsMetric = "";
        c2.unitsImperial = "";
        c2.conversion = 1;
        c2.conversionSum = 0;
        c2.program = "{ value { BikeIntensity; } count { Duration; } }";
        c2.precision = 3;
        c2.istime = false;
        c2.aggzero = true;
        c2.fingerprint = c2.symbol + DataFilter::fingerprint(c2.program);
        metrics.insert(0, c2);
        f.compatibilitymetrics << "IF";
    }

    if (!hasc3) {
        UserMetricSettings c3;

        // add backwards compatibility metrics
        c3.symbol = "compatibility_3";
        c3.name = "NP";
        c3.type = 1;
        c3.description = "IsoPower";
        c3.unitsMetric = "";
        c3.unitsImperial = "";
        c3.conversion = 1;
        c3.conversionSum = 0;
        c3.program = "{ value { IsoPower; } count { Duration; } }";
        c3.precision = 0;
        c3.istime = false;
        c3.aggzero = true;
        c3.fingerprint = c3.symbol + DataFilter::fingerprint(c3.program);
        metrics.insert(0, c3);
        f.compatibilitymetrics << "NP";
    }
}
