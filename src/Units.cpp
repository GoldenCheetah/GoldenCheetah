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
#include <math.h>

QString kphToPace(double kph, bool metric)
{
    // return a min/mile or min/kph string
    if (!metric) kph /= KM_PER_MILE;

    // stop silly stuff
    if (kph < 0.1) {
        return "00:00";
    }

    if (kph > 99) {
        return "xx:xx";
    }

    // now how many minutes to do 1 ?
    double pace = 60.00f / kph;
    int minutes = pace; // rounds down
    int seconds = round((pace - (double)minutes) * 60.00f);

    // ugh. 8min miles becomes 7:60 ! bloody rounding !
    if (seconds >= 60) {
        seconds = 0;
        minutes++;
    }

    return QString("%1:%2")
                  .arg(minutes, 2, 10, QLatin1Char('0'))
                  .arg(seconds, 2, 10, QLatin1Char('0'));
}
