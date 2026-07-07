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

#include <cmath>
#include <float.h>
#include "LTMOutliers.h"

#include <QDebug>


LTMOutliers::LTMOutliers(double *xdata, double *ydata, int count, int windowsize, bool absolute) : stdDeviation(0.0)
{
    double sum = 0;
    int points = 0;
    double allSum = 0.0;
    int pos=0;

    // initial samples from point 0 to windowsize
    for (; pos < windowsize && pos < count; ++pos) {

        // we could either use a deviation of zero
        // or base it on what we have so far...
        // I chose to use sofar since spikes
        // are common at the start of a ride
        xdev add;
        add.x = xdata[pos];
        add.y = ydata[pos];
        add.pos = pos;

        if (absolute) add.deviation = fabs(ydata[pos] - (sum/windowsize));
        else add.deviation = ydata[pos] - (sum/windowsize);

        rank.append(add);

        // when using -ve and +ve values stdDeviation is
        // based upon the absolute value of deviation
        // when not, we should only look at +ve values
        if ((!absolute && add.deviation > 0) || absolute) {
            allSum += add.deviation;
            points++;
        }
        sum += ydata[pos]; // initialise the moving average
    }

    // bulk of samples from windowsize to the end
    for (; pos<count; pos++) {

        // ranked list
        xdev add;
        add.x = xdata[pos];
        add.y = ydata[pos];
        add.pos = pos;
        if (absolute) add.deviation = fabs(ydata[pos] - (sum/windowsize));
        else add.deviation = ydata[pos] - (sum/windowsize);
        rank.append(add);

        // calculate the sum for moving average
        sum += ydata[pos] - ydata[pos-windowsize];

        // when using -ve and +ve values stdDeviation is
        // based upon the absolute value of deviation
        // when not, we should only look at +ve values
        if ((!absolute && add.deviation > 0) || absolute) {
            allSum += add.deviation;
            points++;
        }
    }

    // and to the list of deviations
    // calculate the average deviation across all points
    stdDeviation = allSum / (double)points;

    // create a ranked list
    std::sort(rank.begin(), rank.end());
}
