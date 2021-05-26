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

    // what we got - normalise to number of fits
    return (sumwb2/data.count()) /1000.0f;
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
            wpbal  += watts < parms.CP ? ((double(parms.TAU)/100.0f) * (parms.W - wpbal)/parms.W * (parms.CP - watts) ) : (parms.CP-watts);
        }

        t++;
    }

    // we solve for W'bal=500 as it is not possible to completely
    // exhaust W', 500 is the point at which most athletes will
    // fail to continue, on average.
    // See: http://www.ncbi.nlm.nih.gov/pubmed/24509723
    return wpbal - 500;
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

    // scale rand() to our range (32767 is typical for RAND_MAX)
    double f = double(Wrange) / double(RAND_MAX);

    do {
        returning.CP = p.CP + (rand()%CPrange - (CPrange/2));
        returning.W = p.W + (int(double(rand())*f)%Wrange - (Wrange/2));
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

        // progress update k=0 means stop so we offset by one
        emit current(k+1, snew,Enew);

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

            // k of zero means stop so we offset by one
            emit newBest(k+1, sbest, Ebest);
            //qDebug()<<k<<"new best"<<Ebest <<s.CP<<s.W<<s.TAU;
        }

        // don't run forever
        k++;

    }

    // k of zero means stop
    emit newBest(0, sbest,Ebest);
    //qDebug()<<"TOOK"<<p.elapsed();
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

// Metric of best 'R' for first exhaustion point in a ride
// if there are no exhaustion points we set to NA

class BestR : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(BestR);

    public:

    BestR()
    {
        setSymbol("cpsolver_best_r");
        setInternalName("Exhaustion Best R");
    }
    void initialize() {
        setName(tr("Exhaustion Best R"));
        setType(RideMetric::Average);
        setMetricUnits(tr("R"));
        setImperialUnits(tr("R"));
        setPrecision(2);
        setDescription(tr("Best value for R in differential model for exhaustion point."));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {
        // does it even have an exhaustion point?
        double returning = RideFile::NA;

        // don't do for intervals or null rides or rides with not exhaustion points!
        if (spec.interval() == NULL && item && item->ride() && item->ride()->referencePoints().count()) {

            // find first and compute for it
            foreach(RideFilePoint *p, item->ride()->referencePoints()) {

                if (p->secs) {

                    double CP = 250;
                    double W = 20000;

                    // set from the ride
                    if (item->context->athlete->zones(item->sport)) {
                        int zoneRange = item->context->athlete->zones(item->sport)->whichRange(item->dateTime.date());
                        CP = zoneRange >= 0 ? item->context->athlete->zones(item->sport)->getCP(zoneRange) : 0;
                        W = zoneRange >= 0 ? item->context->athlete->zones(item->sport)->getWprime(zoneRange) : 0;

                        // did we override CP in metadata / metrics ?
                        int oCP = item->getText("CP","0").toInt();
                        if (oCP) CP=oCP;
                        int oW = item->getText("W'","0").toInt();
                        if (oW) W=oW;
                    }

                    double bestR=0;
                    double bestW=0;
                    bool first=true;

                    // set to 1s recording
                    RideFile *f = item->ride()->resample(1,0);

                    for(double r=0.2; r<0.9; r += 0.001) {

                        double wpbal=W;

                        // loop through and count
                        RideFileIterator it(item->ride(), spec);
                        while (it.hasNext()) {
                            struct RideFilePoint *point = it.next();

                            // iterate for this point
                            wpbal  += point->watts < CP ? (r * (W - wpbal)/W * (CP - point->watts)) : (CP-point->watts);
                            if (point->secs > p->secs) break;
                        }
                        if (first || fabs(wpbal-double(500.0f))<bestW) {
                            first = false;
                            bestR = r;
                            bestW = fabs(wpbal-500.0f);
                        }
                    }

                    // set it and we're done
                    delete f;

                    // if not really in the ball park ignore
                    if (bestW < 2000) setValue(bestR);
                    else setValue(RideFile::NIL);

                    return;
                }
            }
        }
        setValue(returning);
    }

    bool isRelevantForRide(const RideItem *ride) const {
        return ride->present.contains("P");
    }
    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new BestR(*this); }
};

static bool addMetrics() {
    RideMetricFactory::instance().addMetric(BestR());
    return true;
}

static bool added = addMetrics();
