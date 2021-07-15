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

#ifndef _GC_Specification_h
#define _GC_Specification_h

#include <QString>
#include <QStringList>
#include <QSet>
#include "TimeUtils.h"

//
// A 'specification' can be passed around to use as a filter.
// It can be used om collections of ride items, or on ride samples.
//
// The same object it used, can be set with filters for either
// but the pass() function should be passed either a ride item
// or a ride point.
//
class RideItem;
class RideFile;
class IntervalItem;
struct RideFilePoint;

class FilterSet
{

    // used to collect filters and apply if needed
    QVector<QSet<QString>> filters_;

    public:

        // create one with a set
        FilterSet(bool on, QStringList list) {
            if (on) filters_ << QSet<QString>(list.begin(), list.end());
        }

        // create an empty set
        FilterSet() {}

        // add a new filter
        void addFilter(bool on, QStringList list) {
            if (on) filters_ << QSet<QString>(list.begin(), list.end());
        }

        // clear the filter set
        void clear() {
            filters_.clear();
        }

        // does the name in question pass the filter set ?
        bool pass(QString name) {
            foreach(QSet<QString> set, filters_)
                if (!set.contains(name))
                    return false;
            return true;
        }

        int count() { return filters_.count(); }
};

class RideFileIterator;
class Specification
{
    friend class ::RideFileIterator;
    public:
        Specification(DateRange dr, FilterSet fs);
        Specification(IntervalItem *it, double recintsecs);
        Specification();

        // does the date pass the specification ?
        bool pass(QDate);

        // does the rideitem pass the specification ?
        bool pass(RideItem*);

        // does the ridepoint pass the specification ?
        bool pass(RideFilePoint *p);

        // would it yield no data points for this ride ?
        bool isEmpty(RideFile *);

        // non-null if exists
        IntervalItem *interval() { return it; }

        // set criteria
        void setDateRange(DateRange dr);
        void setFilterSet(FilterSet fs);
        void setIntervalItem(IntervalItem *it, double recintsecs);
        void setRideItem(RideItem *ri);

        void addMatches(QStringList matches);

        DateRange dateRange() { return dr; }
        FilterSet filterSet() { return fs; }
        bool isFiltered() { return (fs.count() > 0); }

        // just start/stop and item for now
        // when working with samples
        void print();

        // when working with intervals secs start and end
        // if no interval is set then they return -1 to indicate
        // that the entire ride is in scope
        double secsStart() const;
        double secsEnd() const;

    private:
        DateRange dr;
        FilterSet fs;
        IntervalItem *it;
        double recintsecs;
        RideItem *ri;
};
#endif
