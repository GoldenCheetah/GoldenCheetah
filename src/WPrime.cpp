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

// Many thanks for the gracious support from Dr Philip Skiba for this
// component. Not only did Dr Phil happily agree for us to re-use his
// research, but also provided information and guidance to assist in the
// implementation.
//
// This code implements the W replenishment / utilisation algorithm
// as defined in "Modeling the Expenditure and Reconstitution of Work Capacity 
// above Critical Power." Med Sci Sports Exerc 2012;:1.
//
// The actual code is derived from an MS Office Excel spreadsheet shared
// privately to assist in the development of the code.
// 
// There is definitely room form a performance improvement from anyone
// with a greater math expertise than this developer. I suspect that is
// most!
//

#include "WPrime.h"

const double WprimeMultConst = 1.0;
const int WprimeDecayPeriod = 1200; // 1200 seconds or 20 minutes
const double E = 2.71828183;

const int WprimeMatchSmoothing = 25; // 25 sec smoothing looking for matches
const int WprimeMatchMinJoules = 100; 

WPrime::WPrime()
{
    // XXX will need to reset metrics when they are added
    minY = maxY = 0;
}

void
WPrime::setRide(RideFile *input)
{
    QTime time; // for profiling performance of the code
    time.start();

    // remember the ride for next time
    rideFile = input;

    // reset from previous
    values.resize(0); // the memory is kept for next time so this is efficient
    xvalues.resize(0);

    CP = WPRIME = TAU=0;
    minY = maxY = WPRIME;
        
    // no data or no power data then forget it.
    if (!input || input->dataPoints().count() == 0 || input->areDataPresent()->watts == false) {
        return;
    }

    // STEP 1: CONVERT POWER DATA TO A 1 SECOND TIME SERIES
    // create a raw time series in the format QwtSpline wants
    QVector<QPointF> points;

    int last=0;

    if (input->recIntSecs() >= 1) {
        RideFilePoint *lp=NULL;
        foreach(RideFilePoint *p, input->dataPoints()) {

            // fill gaps in recording with zeroes
            if (lp)
                for(int t=lp->secs+input->recIntSecs();
                    t < p->secs;
                    t += input->recIntSecs())
                    points << QPointF(t, 0);

            // lets not go backwards -- or two sampls at the same time
            if ((lp && p->secs > lp->secs) || !lp)
                points << QPointF(p->secs, p->watts);

            // update state
            last = p->secs;
            lp = p;
        }
    } else {

        foreach(RideFilePoint *p, input->dataPoints()) {
            points << QPointF(p->secs, p->watts);
            last = p->secs;
        }
    }

    // Create a spline
    QwtSpline smoothed;
    smoothed.setSplineType(QwtSpline::Natural);
    smoothed.setPoints(QPolygonF(points));

    // Get CP
    CP = 250; // default
    if (input->context->athlete->zones()) {
        int zoneRange = input->context->athlete->zones()->whichRange(input->startTime().date());
        CP = zoneRange >= 0 ? input->context->athlete->zones()->getCP(zoneRange) : 0;
        WPRIME = zoneRange >= 0 ? input->context->athlete->zones()->getWprime(zoneRange) : 0;

        // did we override CP in metadata / metrics ?
        int oCP = input->getTag("CP","0").toInt();
        if (oCP) CP=oCP;
    }

    // since we will be running up and down the data series multiple times
    // as we iterate and run a SUMPRODUCT it is best to extract the data
    // into a vector of ints for the watts above CP
    double totalBelowCP=0;
    double countBelowCP=0;
    QVector<int> inputArray(last+1);
    for (int i=0; i<last; i++) {

        int value = smoothed.value(i);
        inputArray[i] = value > CP ? value-CP : 0;

        if (value < CP) {
            totalBelowCP += value;
            countBelowCP++;
        }
    }

    TAU = 546.00f * pow(E,-0.01*(CP - (totalBelowCP/countBelowCP))) + 316.00f;
    TAU = int(TAU); // round it down

    //qDebug()<<"data preparation took"<<time.elapsed();

    // STEP 2: ITERATE OVER DATA TO CREATE W' DATA SERIES

    // initialise with Wbal equal to W' and therefore 0 expenditure
    double Wbal = WPRIME;
    double Wexp = 0;
    int u = 0;

    // lets run forward from 0s to end of ride
    minY = WPRIME;
    maxY = WPRIME;
    values.resize(last+1);
    xvalues.resize(last+1);

    for (int t=0; t<=last; t++) {

        // because we work with watts per second
        // joules = watts * 1 i.e. joules = watts
        double watts = smoothed.value(t);
        if (watts > CP) {

            Wbal -= (watts-CP); // expending
            Wexp = WPRIME-Wbal;
            u = t; 

        } else {

            // calculate bal
            Wbal = WPRIME - (Wexp * pow(E, -(double(t-u)/TAU)));
        }

        // update arrays
        xvalues[t] = double(t)/60.00f;
        values[t] = Wbal;

        // min / max
        if (Wbal < minY) minY = Wbal;
        if (Wbal > maxY) maxY = Wbal;
        
    }

    // STEP 3: FIND MATCHES

    // SMOOTH DATA SERIES 

    // get raw data adjusted to 1s intervals (as before)
    QVector<int> smoothArray(last+1);
    QVector<int> rawArray(last+1);
    for (int i=0; i<last; i++) {
        smoothArray[i] = smoothed.value(i);
        rawArray[i] = smoothed.value(i);
    }
    
    // initialise rolling average
    double rtot = 0;
    for (int i=WprimeMatchSmoothing; i>0 && last-i >=0; i--) {
        rtot += smoothArray[last-i];
    }

    // now run backwards setting the rolling average
    for (int i=last; i>=WprimeMatchSmoothing; i--) {
        int here = smoothArray[i];
        smoothArray[i] = rtot / WprimeMatchSmoothing;
        rtot -= here;
        rtot += smoothArray[i-WprimeMatchSmoothing];
    }

    // FIND MATCHES -- INTERVALS WHERE POWER > CP 
    //                 AND W' DEPLETED BY > WprimeMatchMinJoules
    bool inmatch=false;
    matches.clear();
    mvalues.clear();
    mxvalues.clear();
    for(int i=0; i<last; i++) {

        Match match;

        if (!inmatch && (smoothArray[i] >= CP || rawArray[i] >= CP)) {
            inmatch=true;
            match.start=i;
        }

        if (inmatch && (smoothArray[i] < CP && rawArray[i] < CP)) {

            // lets work backwards as we're at the end
            // we only care about raw data to avoid smoothing
            // artefacts
            int end=i-1;
            while (end > match.start && rawArray[end] < CP) {
                end--;
            }

            if (end > match.start) {

                match.stop = end;
                match.secs = (match.stop-match.start) +1; // don't fencepost!
                match.cost = values[match.start] - values[match.stop];

                if (match.cost >= WprimeMatchMinJoules) {
                    matches << match;
                }
            }
            inmatch=false;
        }
    }

    // SET MATCH SERIES FOR ALLPLOT CHART
    foreach (struct Match match, matches) {

        // we only count 1kj asa match
        if (match.cost >= 2000) { //XXX need to agree how to define a match -- or even if we want to...
            mvalues << values[match.start];
            mxvalues << xvalues[match.start];
            mvalues << values[match.stop];
            mxvalues << xvalues[match.stop];
        }
    }
}

double
WPrime::maxMatch()
{
    double max=0;
    foreach(struct Match match, matches) 
        if (match.cost > max) max = match.cost;

    return max;
}

//
// Associated Metrics
//

class MinWPrime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MinWPrime);

    public:

    MinWPrime()
    {
        setSymbol("skiba_wprime_low");
        setInternalName("Minimum W'bal");
    }
    void initialize() {
        setName(tr("Minimum W' bal"));
        setType(RideMetric::Low);
        setMetricUnits(tr("Kj"));
        setImperialUnits(tr("Kj"));
        setPrecision(1);
    }
    void compute(const RideFile *r, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        WPrime w;
        w.setRide((RideFile*)r);
        setValue(w.minY/1000.00f);
    }

    bool canAggregate() { return false; }
    RideMetric *clone() const { return new MinWPrime(*this); }
};

class MaxMatch : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxMatch);

    public:

    MaxMatch()
    {
        setSymbol("skiba_wprime_maxmatch");
        setInternalName("Maximum W'bal Match");
    }
    void initialize() {
        setName(tr("Maximum W'bal Match"));
        setType(RideMetric::Low);
        setMetricUnits(tr("Kj"));
        setImperialUnits(tr("Kj"));
        setPrecision(1);
    }
    void compute(const RideFile *r, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        WPrime w;
        w.setRide((RideFile*)r);
        setValue(w.maxMatch()/1000.00f);
    }

    bool canAggregate() { return false; }
    RideMetric *clone() const { return new MaxMatch(*this); }
};

// add to catalogue
static bool addMetrics() {
    RideMetricFactory::instance().addMetric(MinWPrime());
    RideMetricFactory::instance().addMetric(MaxMatch());
    return true;
}

static bool added = addMetrics();
