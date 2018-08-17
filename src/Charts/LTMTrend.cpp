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
#include "LTMTrend.h"

#include <QDebug>

LTMTrend::LTMTrend(const double *xdata, const double *ydata, int count) :
          minX(0.0), maxX(0.0), minY(0.0), maxY(0.0),
          points(0.0), sumX(0.0), sumY(0.0), sumXsquared(0.0),
          sumYsquared(0.0), sumXY(0.0), a(0.0), b(0.0)
{
    if (count <= 2) return;

    for (int i = 0; i < count; i++) {

        // ignore zero points
        if (ydata[i] == 0.00) continue;

        points++;
        sumX += xdata[i];
        sumY += ydata[i];
        sumXsquared += xdata[i] * xdata[i];
        sumYsquared += ydata[i] * ydata[i];
        sumXY += xdata[i] * ydata[i];
    }

    if (fabs( double(points) * sumXsquared - sumX * sumX) > DBL_EPSILON) {
        b = ( double(points) * sumXY - sumY * sumX) /
            ( double(points) * sumXsquared - sumX * sumX);
        a = (sumY - b * sumX) / double(points);
        /*UNUSED double r = ( double(points) * sumXY - sumY * sumX) /
            sqrt(( double(points) * sumXsquared - sumX * sumX) * ( double(points) * sumYsquared - sumY * sumY)); */

        //UNUSED double sx = b * ( sumXY - sumX * sumY / double(points) );
        //UNUSED double sy2 = sumYsquared - sumY * sumY / double(points);
        //UNUSED double sy = sy2 - sx;

        //UNUSED double coefD = sx / sy2;
        //UNUSED double coefC = sqrt(coefD);
        //UNUSED double stdError = sqrt(sy / double(points - 2));

        //qDebug() << coefD << coefC << stdError << r;
    }
}

