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
CPSolver::setData(CPSolverConstraints constraints, QList<RideItem*> rides)
{
    // remember the rides
    this->constraints=constraints;
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
    for(int i=0; i<data.count();i++)  sumwb2 += pow(compute(data[i], parms),2);

    //qDebug()<<"cost="<<QString("%1").arg(sumwb2, 0, 'g', 7);

    // what we got
    return sumwb2 /1000.0f;
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
CPSolver::neighbour(WBParms p, int k, int kmax)
{
    WBParms returning;

    // a crucial aspect of the simulated annealling algorithm is that
    // we search a wide space for solutions as we start searching, but
    // as time passes we look in a much smaller range; i.e. we distribute
    // across the search space up front, but hone in as we get nearer the end

    // wild ass guesses at the beginning down to very closest neighbours
    // start at 150% and drop down to 1%
    double factor = (double(kmax - k) / (double(kmax)));

    // range from where we are now from hi to lo
    // value range (e.g. 400 is range of CP between 100-500)
    int CPrange = 3 + ((constraints.cpto - constraints.cpf) * factor);
    int Wrange = 101 + ((constraints.wto - constraints.wf) * factor);
    int TAUrange = 3 + ((constraints.tto - constraints.tf) * factor);
    int it=0;

    do {
        returning.CP = p.CP + (rand()%CPrange - (CPrange/2));
        returning.W = p.W + (rand()%Wrange - (Wrange/2));
        returning.TAU = p.TAU + (rand()%TAUrange - (TAUrange/2));

    } while (it++ < 3 && (returning.CP < constraints.cpf || returning.CP > constraints.cpto ||
                          returning.W > constraints.cpto || returning.W < constraints.cpf ||
                          returning.TAU < constraints.tf || returning.TAU > constraints.tto));

    // if we failed to randomise just check bounds
    if (returning.CP > constraints.cpto) returning.CP = constraints.cpto;
    if (returning.CP < constraints.cpf) returning.CP = constraints.cpf;
    if (returning.W > constraints.wto) returning.W = constraints.wto;
    if (returning.W < constraints.wf) returning.W = constraints.wf;
    if (returning.TAU > constraints.tto) returning.TAU = constraints.tto;
    if (returning.TAU < constraints.tf) returning.TAU = constraints.tf;

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

    // to flag when to stop
    halt = false;

    // set starting conditions at maximals
    s0.CP =   constraints.cpto;
    s0.W =    constraints.wto;
    s0.TAU =  constraints.tto;

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
    int kmax = 100000;

    // give up when we're on it or run out of loops
    while (halt == false && k < kmax) {

        WBParms snew = neighbour(s, k, kmax);
        double Enew = cost(snew);

        // progress update
        emit current(k, snew,Enew);

        // probability - always 1 if better, but randomly accept higher
        double random = double(rand()%101)/100.00f;
        double temp = temperature(double(k)/double(kmax));
        double prob = probability(E,Enew,temp);

        if (prob > random) {
            s = snew;
            E = Enew;
        }

        // is it better than our very best?
        if (E < Ebest) {
            Ebest = E;
            sbest = s;
            emit newBest(k, sbest, Ebest);
            //qDebug()<<k<<"new best"<<Ebest <<s.CP<<s.W<<s.TAU;
        }

        // don't run forever
        k++;

    }
    emit newBest(0, sbest,Ebest);
}

double
CPSolver::temperature(double alpha)
{
    return (1.0-(0.02*alpha));
}

double
CPSolver::probability(double sold, double snew, double temperature)
{
    if(snew < sold ) return 1.0;
    return(exp((sold - snew)/temperature));
}

void
CPSolver::pause()
{
}

void
CPSolver::stop()
{
    halt = true;
}
