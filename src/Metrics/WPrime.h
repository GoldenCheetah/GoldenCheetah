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
#include "ErgFile.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "RideMetric.h"
#include "SplineLookup.h"
#include <QVector>
#include <QThread>
#include <cmath>

struct Match {
    int start, stop, secs;       // all in whole seconds
    int cost;                   // W' depletion
    bool exhaust;
};

class WPrime {

    Q_DECLARE_TR_FUNCTIONS(WPrime)

    public:

        // construct and calculate series/metrics
        WPrime();

        // recalc from ride selected or workout selected in train mode
        void setRide(RideFile *ride);
        void setErg(ErgFile *erg);
        void setWatts(Context *context, QVector<int>&watts, int CP, int WPRIME);

        RideFile *ride() { return rideFile; }

        // W' 1second time series from 0
        QVector<double> &ydata() { check(); return values; }
        QVector<double> &xdata(bool bydist) { check(); return bydist ? xdvalues : xvalues; }

        // array of power >CP
        QVector<int> powerValues;
        QVector<int> smoothArray;

        QVector<double> &mydata() { check(); return mvalues; }
        QVector<double> &mxdata(bool bydist) { check(); return bydist ? mxdvalues : mxvalues; }

        double maxMatch();
        double minY, maxY;
        double TAU, PCP_, CP, WPRIME, EXP;

        // lowest we got as a percentage
        double maxE() { return WPRIME ? (((WPRIME - minY) / WPRIME) * 100.00f) : 0; }

        double PCP();

        QList<Match> matches;       // matches burned with associated cost

        int minForCP(int CP);

        // zoning with supplied values to avoid using a new Zones type config
        static QString summarize(int WPRIME, QVector<double> wtiz, QVector<double> wcptiz, QVector<double> wworktiz, QColor color);
        static int zoneCount() { return 4; }
        static QString zoneName(int i);
        static QString zoneDesc(int i);
        static int zoneLo(int i, int WPRIME);
        static int zoneHi(int i, int WPRIME);

    private:

        RideFile *rideFile;          // the ride file we worked on
        QVector<double> values;      // W' time series in 1s intervals
        QVector<double> xvalues;      // W' time series in 1s intervals
        QVector<double> xdvalues;      // W' distance

        QVector<double> mvalues;      // W' time series in 1s intervals
        QVector<double> mxvalues;      // W' time series in 1s intervals
        QVector<double> mxdvalues;      // W' distance

        SplineLookup smoothed;
        SplineLookup distance;
        int last;

        void check(); // check we don't need to recompute
        bool wasIntegral;
};

class WPrimeIntegrator : public QThread
{
    public:
        WPrimeIntegrator(QVector<int> &source, int begin, int end, double TAU);

        // integrate from start to stop from source into output
        // basically sums in the exponential decays, but we break it
        // into threads to parallelise the work
        void run();

        QVector<int> &source;
        int begin, end;
        double TAU;

        // resized to match source holds results
        QVector<double> output;
};
#endif
