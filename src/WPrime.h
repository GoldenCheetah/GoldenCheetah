/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _gc_wprime_include
#define _gc_wprime_include

#include "RideFile.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "RideMetric.h"
#include <QVector>
#include <qwt_spline.h> // smoothing
#include <math.h>

struct Match {
    int start, stop, secs;       // all in whole seconds
    int cost;                   // W' depletion
};

class WPrime {


    public: 

        // construct and calculate series/metrics
        WPrime();

        // recalc from ride selected
        void setRide(RideFile *ride);
        RideFile *ride() { return rideFile; }

        // W' 1second time series from 0
        QVector<double> &ydata() { return values; }
        QVector<double> &xdata() { return xvalues; }

        QVector<double> &mydata() { return mvalues; }
        QVector<double> &mxdata() { return mxvalues; }

        double maxMatch();
        double minY, maxY;
        double TAU, CP, WPRIME;

        QList<Match> matches;       // matches burned with associated cost

    private:

        RideFile *rideFile;          // the ride file we worked on
        QVector<double> values;      // W' time series in 1s intervals
        QVector<double> xvalues;      // W' time series in 1s intervals
    
        QVector<double> mvalues;      // W' time series in 1s intervals
        QVector<double> mxvalues;      // W' time series in 1s intervals
};

#endif
