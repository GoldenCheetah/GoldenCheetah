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

#ifndef _GC_LTMOutliers_h
#define _GC_LTMOutliers_h 1
#include "GoldenCheetah.h"

#include <QVector>
#include <QMap>

class LTMOutliers
{
    // used to produce a sorted list
    struct xdev {
        double x,y;
        int pos;
        double deviation;

        bool operator< (const xdev &right) const {
            return (deviation > right.deviation);  // sort ascending! (.gt not .lt)
        }
    };

    public:
        // Constructor using arrays of x values and y values
        LTMOutliers(double *x, double *y, int count, int windowsize, bool absolute=true);

        // ranked values
        int getIndexForRank(int i) { return rank[i].pos; }
        double getXForRank(int i) { return rank[i].x; }
        double getYForRank(int i) { return rank[i].y; }
        double getDeviationForRank(int i) { return rank[i].deviation; }

        // std deviation
        double getStdDeviation() { return stdDeviation; }

    protected:
        double stdDeviation;
        QVector<xdev> rank;             // ranked list of x sorted by deviation
};

#endif
