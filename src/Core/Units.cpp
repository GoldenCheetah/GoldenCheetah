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

#include "Units.h"
#include <cmath>

QTime kphToPaceTime(double kph, bool metric, bool isSwim)
{
    // return a min/mile or min/kph string
    if (!metric && !isSwim) kph /= KM_PER_MILE;

    // stop silly stuff
    if (kph < 0.1) {
        return QTime(0, 0, 0);
    }

    if (kph > 99) {
        return QTime();
    }

    // Swim pace is min/100m or min/100y
    if (isSwim) kph = kph * 10.00f / (metric ? 1.00f : METERS_PER_YARD);

    // now how many minutes to do 1 ?
    double pace = 60.00f / kph;
    int minutes = pace; // rounds down
    int seconds = round((pace - (double)minutes) * 60.00f);

    // ugh. 8min miles becomes 7:60 ! bloody rounding !
    if (seconds >= 60) {
        seconds = 0;
        minutes++;
    }

    return QTime(0, minutes, seconds);
}

QString kphToPace(double kph, bool metric, bool isSwim)
{
    QTime time = kphToPaceTime(kph, metric, isSwim);
    if (! time.isValid()) {
        return "xx:xx";
    }
    return time.toString("mm:ss");
}

QString mphToPace(double mph, bool metric, bool isSwim)
{
    // convert to kph and then use kph function
    double kph = mph * KM_PER_MILE;

    return kphToPace(kph, metric, isSwim);
}
