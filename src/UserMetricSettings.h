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

#ifndef _GC_UserMetricSettings_h
#define _GC_UserMetricSettings_h 1
#include "GoldenCheetah.h"

#include "RideMetric.h"

// what version of user metric structure are we using?
// Version      Date           Who               What
// 1            10 Dec 2015    Mark Liversedge   Initial version created
#define USER_METRICS_VERSION_NUMBER 1

// This structure is used to pass config back and forth
// and is "compiled" into a UserMetric at runtime
class UserMetricSettings {

    public:

        QString symbol,
                name,
                description,
                unitsMetric,
                unitsImperial;

        double  conversion,
                conversionSum;

        QString program;
};
#endif
