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


#include "CPSolver.h"
#include <ctime>

CPSolver::CPSolver(Context *context)
   : context(context)
{
    integral = (appsettings->value(NULL, GC_WBALFORM, "int").toString() == "int");
}

// set the data to solve
void
CPSolver::setData(QList<RideItem*> rides)
{
    // remember the rides
    this->rides=rides;

    // set the vectors!
    foreach(RideItem *item, rides) {

        // we don't do null well
        if (!item || !item->ride()) continue;

        // each reference gets a separate data series
        foreach(RideFilePoint *rp, item->ride()->referencePoints()) {

            // skip other reference types
            if (rp->secs <= 0) continue;

            // ok, now we have a point we need to get the power data
            // from the start to the point of exhaustion into a
            // 1 second sample array
            data << power1s(item->ride(), rp->secs);
        }
    }
}

// get a 1s array to the point secs
QVector<int>
CPSolver::power1s(RideFile *f, double secs)
{
    // its already in 1s samples so just pull it in
    QVector<int> returning;

    foreach(RideFilePoint *p, f->resample(1, 0)->dataPoints()) {
        if (p->secs < secs) returning << p->watts;
        else break;
    }

    return returning;
}

// compute the cost, using the settings passed
double
CPSolver::cost(WBParms parms)
{
    // returning sum(W'bal ^ 2)

    // loop through each ride for now, but avoid foreach()
    // since it will make a copy of the contents which has
    // a significant performance impact
    double sumwb2=0;
    for(int i=0; i<data.count();i++)  sumwb2 += pow(compute(data[i], parms)/1000.0f,2);

    //qDebug()<<"cost="<<QString("%1").arg(sumwb2, 0, 'g', 7);

    // what we got
    return sumwb2;
}

double
CPSolver::compute(QVector<int> &ride, WBParms parms)
{
    // compute w'bal for the ride using the paramters
    // XXX for now we use the integral model
    double I=0.00f;
    int t=0;
    double wpbal=parms.W;
    foreach(int watts, ride) {

        if (integral) {

            // INTEGRAL
            I += exp(((double)(t) / parms.TAU)) * (watts > parms.CP ? watts-parms.CP : 0);
            wpbal = parms.W - (exp(-((double)(t) / parms.TAU)) * I);

        } else {

            // DIFFERENTIAL
            wpbal  += watts < parms.CP ? ((parms.CP-watts)*(wpbal-parms.W)/wpbal) : (parms.CP-watts);
        }

        t++;
    }

    //qDebug()<<"w'bal at exhaustion:"<<wpbal<<"t="<<t;
    return wpbal;
}

// get us a neighbour
WBParms
CPSolver::neighbour(WBParms p)
{
    WBParms returning;

    returning.CP = p.CP + (rand()%3 + (-1));
    returning.W = p.W + (rand()%201 + (-100));
    returning.TAU = p.TAU + (rand()%31 + (-15));

    // set some bounds to physiologically plausible values
    // i.e. limit the search space !
    if (returning.CP < 100) returning.CP=100;
    if (returning.CP > 450) returning.CP=450;
    if (returning.W > 50000) returning.W = 50000;
    if (returning.W < 5000) returning.W = 5000;
    if (returning.TAU < 400) returning.TAU = 600;
    if (returning.TAU > 600) returning.TAU = 400;

    //qDebug()<<"X:"<<p.CP<<p.W<<p.TAU<<"neighbour:"<<returning.CP<<returning.W<<returning.TAU;
    return returning;
}

void
CPSolver::reset()
{
    rides.clear();
    data.clear();
}

void
CPSolver::start()
{
    // set starting conditions from first ride
    if (data.count() == 0 || rides.count() == 0) return;

    // get starting conditions from first ride
    s0.CP =  rides[0]->ride()->wprimeData()->CP;
    s0.W =  rides[0]->ride()->wprimeData()->WPRIME;
    s0.TAU =  300; // this is where the magic happens

    QTime p;
    p.start();

    // initial conditions
    srand((unsigned int) time (NULL)); // seed ONCE!
    double E = cost(s0);
    double Ebest = E;
    WBParms s = s0;
    WBParms sbest = s;

    // 10,000 iterations at most
    int k=0;
    int N=0;
    int kmax = 100000;

    // give up when we're on it or run out of loops
    while (/*E > 0.1f &&*/ k < kmax) {

        WBParms snew = neighbour(s);
        double Enew = cost(snew);

        // progress update
        emit current(k, snew,Enew);

        // move here, randomly pick one thats worse
        if (Enew < E || (rand()%100 == 1))  s = snew;

        // is it better than our very best?
        if (Enew < Ebest) {
            Ebest = Enew;
            sbest = s;
            emit newBest(k, sbest,Ebest);
            //qDebug()<<k<<"new best"<<Ebest <<s.CP<<s.W<<s.TAU;
        }

        // time to switch to new space?
        if (++N%10000 == 0) {
            s = sbest;
            s.TAU = 300 + (rand()%300);
            E = cost(s);
            //qDebug()<<k<<"reset to"<<E <<s.CP<<s.W<<s.TAU;
        }

        // don't run forever
        k++;
    }
    emit newBest(0, sbest,Ebest);
}

void
CPSolver::pause()
{
}

void
CPSolver::stop()
{
}
