/*
 * Copyright (c) 2019 Mark Liversedge (liversedge@gmail.com)
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

#include "RideMetric.h"
#include "Context.h"
#include "Estimator.h"
#include <QObject>
#include <QDate>
#include <QVector>

#ifndef GC_Banister_h
#define GC_Banister_h 1

// typical athlete as defined by the opendata analysis
// using floats because they are typically used in calculations
// that require float precision (the actual values are integers)
extern const double typical_CP, typical_WPrime, typical_Pmax;

// calculate power index, used to model in cp plot too
extern double powerIndex(double averagepower, double duration);

// gap with no training that constitutes break in seasons
extern const int typical_SeasonBreak;

class banisterData{
public:
    banisterData() : score(0), g(0), h(0), nte(0), pte(0), perf(0), test(0) {}
    double score,       // TRIMP, BikeScore etc for day
           g,           // accumulated load with t1 decay for pte
           h,           // accumulated load with t2 decay for nte
           nte,         // Negative Training Effect
           pte,         // Positive Training Effect
           perf,        // Performance (nte+pte)
           test;        // test value
};

class Banister;
class banisterFit {
public:
    banisterFit(Banister *parent) : p0(0),k1(0),k2(0),t1(0),t2(0),tests(0),testoffset(-1),parent(parent) {}

    double f(double t, const double *p);
    void combine(banisterFit other);
    void compute(long startIndex, long stopIndex);

    long startIndex, stopIndex;
    QDate startDate, stopDate;

    double p0,k1,k2,t1,t2;

    // for indexing into banister::performanceDay and banister::performanceScore
    int tests;
    int testoffset;

    Banister *parent;
};

class Banister : public QObject {

    Q_OBJECT

public:

    Banister(Context *context, QString symbol, double t1, double t2, double k1=0, double k2=0);
    double value(QDate date, int type);

    // utility
    QDate getPeakCP(QDate from, QDate to, int &CP);
    double RMSE(QDate from, QDate to, int &n); // only look at it for a date range

    // model parameters - initial 'priors' to use
    QString symbol;         // load metric
    double k1, k2;          // nte/pte coefficients
    double t1, t2;          // nte/pte decay constants

    // metric
    RideMetric *metric;

    QDate start, stop;
    long long days;

    // curves
    double meanscore; // whats the average non-zero ride score (used for scaling)
    long rides;       // how many rides are there with a non-zero score

    // time series data
    QVector<banisterData> data;

    // fitting windows with parameter estimates
    // window effectively a season
    QList<banisterFit> windows;

    // performances as double
    int performances;
    QVector<double> performanceDay, performanceScore;

public slots:

    void init();        // reset previous fits
    void invalidate();  // mark as stale
    void refresh();     // collect data from rides etc
    void setDecay(double t1, double t2); // adjust the t1/t2 parameters
    void fit();         // perform fits along windows

private:
    Context *context;
    bool isstale;

};
#endif
