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

#ifndef _RideDB_h
#define _RideDB_h
#include "GoldenCheetah.h"

#include "RideCache.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include <stdio.h>
#include <QDebug>
#include <QString>
#include "JsonRideFile.h" // for DATETIME_FORMAT

// change history
// version  date       who                     what
// 1.0      Dec 2014   Mark Liversedge         initial version
// 1.1      12 Dec 14  Mark Liversedge         added color, isRun and present
// 1.2      03 May 15  Mark Liversedge         added intervals, samples bool and metric <> 0
// 1.3      27 Jun 15  Mark Liversedge         rationalised all the discovery intervals
// 1.4       5 Sep 15  Mark Liversedge         overrides_ in RideItem
// 1.5      15 Dec 15  Mark Liversedge         zone ranges added as part of metric compute() refactor
// 1.6      16 Jul 16  Mark Liversedge         xdata added
// 1.7      20 Feb 17  Mark Liversedge         Metric count (if nonzero) added
// 1.8      22 Feb 17  Mark Liversedge         Metric stdmean() stdvariance() added
// 1.9      29 Jul 18  Mark Liversedge         Mark intervals as performance tests
// 2.0      04 Apr 20  Ale Martinez            sport replaces isRun and isSwim

#define RIDEDB_VERSION "2.0"

class APIWebService;
class HttpResponse;
class HttpRequest;

// using context (we are reentrant)
struct RideDBContext {

    // athlete folder name
    QString folder;

    // either reading a cache or providing an api ...
    RideCache *cache;
    Context *context;

    // api parms
    APIWebService *api;
    HttpRequest *request;
    HttpResponse *response;

    // the scanner
    void *scanner;

    // Set during parser processing, using same
    // naming conventions as yacc/lex -p
    RideItem item;
    IntervalItem interval;

    // term state data is held in these variables
    QString JsonString;
    QStringList JsonStringList;
    QString key, value, count;
    QStringList errors;

    // is cache/rideDB.json an older version ?
    bool old;
    int loading;

    // tracks the last progress update sent
    double lastProgressUpdate;
};

#endif

