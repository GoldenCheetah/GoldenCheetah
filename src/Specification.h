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
#include "TimeUtils.h"

class RideItem;

class FilterSet
{
    // used to collect filters and apply if needed
    QVector<QStringList> filters_;

    public:

        // create one with a set
        FilterSet(bool on, QStringList list) {
            if (on) filters_ << list;
        }

        // create an empty set
        FilterSet() {}

        // add a new filter
        void addFilter(bool on, QStringList list) {
            if (on) filters_ << list;
        }

        // clear the filter set
        void clear() {
            filters_.clear();
        }

        // does the name in question pass the filter set ?
        bool pass(QString name) {
            foreach(QStringList list, filters_)
                if (!list.contains(name))
                    return false;
            return true;
        }
};

class Specification
{
    public:
        Specification(DateRange dr, FilterSet fs);
        Specification();

        // does the rideitem pass the specification ?
        bool pass(RideItem*);

        // set criteria
        void setDateRange(DateRange dr);
        void setFilterSet(FilterSet fs);

    private:
        DateRange dr;
        FilterSet fs;
};
#endif
