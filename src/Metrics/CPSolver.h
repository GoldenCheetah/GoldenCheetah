/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_CPSolver_h
#define _GC_CPSolver_h 1
#include "GoldenCheetah.h"
#include "Settings.h"

#include "RideMetric.h"
#include "RideItem.h"
#include "RideFile.h"
#include "WPrime.h"

#include <QList>
#include <QVector>
#include <QObject>

class Context;

// W'bal parameters passed around as a set
class WBParms {
public:
    WBParms() : CP(0), W(0), TAU(0) {}
    WBParms(double CP, double W, double TAU) : CP(CP), W(W), TAU(TAU) {}
    double CP, W, TAU; // the parameters
    double wpbal; // the result (used to pass back)
};

class CPSolverConstraints {
    public:
    CPSolverConstraints() : cpf(100), cpto(500), wf(5000), wto(50000), tf(300), tto(700) { check(); }
    CPSolverConstraints(int cpf, int cpto, int wf, int wto, int tf, int tto) :
    cpf(cpf), cpto(cpto), wf(wf), wto(wto), tf(tf), tto(tto) { check(); }
    int cpf, cpto, wf, wto, tf, tto;
    int ccpf, ccpto, cwf, cwto; // configured bounds for selected rides

    void setConfig(int ccpf, int ccpto, int cwf, int cwto) {
        this->ccpf = ccpf;
        this->ccpto = ccpto;
        this->cwf = cwf;
        this->cwto = cwto;
    }

    // swap if malformed
    void check() {
        if (cpf > cpto) { int t=cpto; cpto=cpf; cpf=t; }
        if (wf > wto) { int t=wto; wto=wf; wf=t; }
        if (tf > tto) { int t=tto; tto=tf; tf=t; }
    }
};

class CPSolver : public QObject {

    Q_OBJECT

    public:

        // as simulated annealing algorithm to solve W', CP and tau
        // from a collection of exhaustion points within a ride
        CPSolver(Context *);

        // set the data to solve
        void setData(CPSolverConstraints constraints, QList<RideItem*>);

        // compute the cost, using the settings passed
        double cost(WBParms parms);

        // compute ending W'bal for the exhaustion series
        double compute(QVector<int> &ride, WBParms parms);

        WBParms neighbour(WBParms, int k, int kmax);
        double probability(double,double,double);
        double temperature(double);

        // get a 1s power array from the data
        QVector<int> power1s(RideFile *f, double secs);

    signals:
        void newBest(int,WBParms,double);
        void current(int,WBParms,double);

    public slots:

        // as underlying ride data changes the
        // contents are invalidated and refreshed
        void reset();
        void start();
        void pause();
        void stop();

    private:

        // who we for ?
        Context *context;
        CPSolverConstraints constraints;
        bool integral;

        // an array of power data leading up to each exhaust point
        QList<QVector<int> > data;
        QList<RideItem*> rides;

        // annealling parms
        WBParms s0, sbest;

        // to signal we need to stop
        bool halt;
};

#endif
