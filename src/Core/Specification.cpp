/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "Specification.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"

Specification::Specification(DateRange dr, FilterSet fs) : dr(dr), fs(fs), it(NULL), recintsecs(0), ri(NULL) {}
Specification::Specification(IntervalItem *it, double recintsecs) : it(it), recintsecs(recintsecs), ri(NULL) {}
Specification::Specification() : it(NULL), recintsecs(0), ri(NULL) {}

// does the date pass the specification ?
bool
Specification::pass(QDate date) const
{
    return (dr.pass(date));
}

// does the rideitem pass the specification ?
bool 
Specification::pass(RideItem*item) const
{
    return (dr.pass(item->dateTime.date()) && fs.pass(item->fileName));
}

bool
Specification::pass(RideFilePoint *p) const
{
    if (it == NULL) return true;
    else if ((p->secs+recintsecs) >= it->start && p->secs <= it->stop) return true;
    return false;
}

// set criteria
void 
Specification::setDateRange(DateRange dr)
{
    this->dr = dr;
}

void 
Specification::setFilterSet(FilterSet fs)
{
    this->fs = fs;
}

void
Specification::addMatches(QStringList other)
{
    fs.addFilter(true, other);
}

void
Specification::setIntervalItem(IntervalItem *it, double recintsecs)
{
    this->it = it;
    this->recintsecs = recintsecs;
}

double 
Specification::secsStart() const
{
    if (it) return it->start;
    else return -1;
}

double 
Specification::secsEnd() const
{
    if (it) return it->stop;
    else return -1;
}

bool
Specification::isEmpty(RideFile *ride) const
{
    // its null !
    if (!ride) return true;

    // no data points
    if (ride->dataPoints().count() <= 0) return true;

    // interval points yield no points ?
    if (it) {
        RideFileIterator i(ride, *this);

        // yikes
        if (i.firstIndex() < 0 || i.lastIndex() < 0) return true;

        // reversed (1s interval has same start and stop)
        if ((i.lastIndex() - i.firstIndex()) < 0) return true;
    }

    return false;
}

void
Specification::setRideItem(RideItem *ri)
{
    this->ri = ri;
}

void
Specification::print()
{
    if (it) qDebug()<<it->name<<it->start<<it->stop;
    else qDebug()<<"item";
}
