/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _Gc_WithingsReading_h
#define _Gc_WithingsReading_h
#include <QString>
#include <QStringList>
#include <QDateTime>

// The Withings API returns measures and targets, and for each
// it will provide a fair amount of information. We capture them
// in the structure below. These map to the JSON fields that are
// returned by the "getmeas" action.
//
// The Wihings WIFI scale and website API documentation can be
// found here http://www.withings.com/en/api/bodyscale
//

#define WITHINGS_WEIGHT     0
#define WITHINGS_FATKG      1
#define WITHINGS_FATPERCENT 2
#define WITHINGS_LEANKG     3
#define WITHINGS_HEIGHT     4

class WithingsReading {

public:
    WithingsReading() : category(0), groupId(0), attribution(0), when(QDateTime()), comment(""),
                        weightkg(0), fatkg(0), leankg(0), fatpercent(0), sizemeter(0) {}

    int category;           // 1 = target, 2 = measurement
    int groupId;            // serialized for synchronizing
    int attribution;        // who is this attributed to ... 0 means this user

    QDateTime when;         // when was this reading taken
    QString comment;        // user commentary regarding this measurement

    double  weightkg,       // weight in Kilograms
            fatkg,          // fat in Kilograms
            leankg,         // lean mass in Kilograms
            fatpercent,     // body fat as a percentage of weight
            sizemeter;      // height ?

    // used by qSort()
    bool operator< (const WithingsReading &right) const {
        return (when < right.when);
    }
};

#endif
