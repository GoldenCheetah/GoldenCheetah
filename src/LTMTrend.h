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

#ifndef _GC_LTMTrend_h
#define _GC_LTMTrend_h 1
#include "GoldenCheetah.h"

class LTMTrend
{
    public:
        // Constructor using arrays of x values and y values
        LTMTrend(double *, double *, int);
        double getYforX(double x) const { return (a + b * x); }
        double intercept() { return a; }
        double slope() { return b; }

        double minX, maxX, minY, maxY; // for the data set we have

    protected:
        long points;
        double sumX, sumY;
        double sumXsquared,
               sumYsquared;
        double sumXY;
        double a, b;   // a = intercept, b = slope

};

#endif
