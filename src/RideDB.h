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

#define RIDEDB_VERSION "1.3"

#endif

