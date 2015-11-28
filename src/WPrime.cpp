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
// To optimise the original implementation that computed the integral at
// each point t as a function of the preceding power above CP at time u through t
// we did 2 things;
//
// 1. compute the exp decay for each poweer above CP and integrate the decay
//    into the future -- but crucially, no need to bother if power above CP is 0.
//    This typically reduces the cpu cycles by a factor of 4
//
// 2. Because the decay is calculated forward at time u we can do these in parallel;
//    i.e. run multiple threads for t=0 through t=time/nthreads. This reduced the
//    elapsed time by a factor of about 2/3rds on a dual core processor.
//
// We could extend the threading to have more than 2 threads on PCs with more cores
// but this would conflict with the ridefilecache computations anyway.
//
// There may be room for improvement by adopting a different integration strategy
// in the future, but now, a typical 4 hour hilly ride can be computed in 250ms on
// and Athlon dual core CPU where previously it took 4000ms.


#include "WPrime.h"
#include "RideItem.h"
#include "Units.h" // for MILES_PER_KM
#include "Settings.h" // for GC_WBALFORM

const double WprimeMultConst = 1.0;
const int WPrimeDecayPeriod = 1800; // 1 hour, tried infinite but costly and limited value
                                    //         on long rides anyway
const double E = 2.71828183;

const int WprimeMatchSmoothing = 25; // 25 sec smoothing looking for matches
const int WprimeMatchMinJoules = 100; 

// used by summarise
static struct WPRIMEZONES {

    int lo, hi;
    QString name;
    const char *desc;

} wbal_zones[] = {

    { 0, 25, "W1", QT_TRANSLATE_NOOP("wbalzone", "Recovered") },
    { 25, 50, "W2", QT_TRANSLATE_NOOP("wbalzone", "Moderate Fatigue") },
    { 50, 75, "W3", QT_TRANSLATE_NOOP("wbalzone", "Heavy Fatigue") },
    { 75, 100, "W4", QT_TRANSLATE_NOOP("wbalzone", "Severe Fatigue") }

};

QString WPrime::zoneName(int i) { return wbal_zones[i].name; }
QString WPrime::zoneDesc(int i) { return qApp->translate("wbalzone", wbal_zones[i].desc); }

WPrime::WPrime()
{
    // XXX will need to reset metrics when they are added
    minY = maxY = 0;
    wasIntegral = (appsettings->value(NULL, GC_WBALFORM, "int").toString() == "int");
}

void
WPrime::check()
{
    bool integral = (appsettings->value(NULL, GC_WBALFORM, "int").toString() == "int");
    if (integral == wasIntegral) return;
    else if (rideFile) {
        wasIntegral = integral;
        setRide(rideFile); // reset coz calc mode changed
    }
}

void
WPrime::setRide(RideFile *input)
{
    bool integral = (appsettings->value(NULL, GC_WBALFORM, "int").toString() == "int");

    QTime time; // for profiling performance of the code
    time.start();

    // remember the ride for next time
    rideFile = input;

    // reset from previous
    values.resize(0); // the memory is kept for next time so this is efficient
    xvalues.resize(0);
    xdvalues.resize(0);

    EXP = PCP_ = CP = WPRIME = TAU=0;

    // no data or no power data then forget it.
    if (!input || input->dataPoints().count() == 0 || input->areDataPresent()->watts == false) {
        return;
    }

    // STEP 1: CONVERT POWER DATA TO A 1 SECOND TIME SERIES
    // create a raw time series in the format QwtSpline wants
    QVector<QPointF> points;
    QVector<QPointF> pointsd;
    double convert = input->context->athlete->useMetricUnits ? 1.00f : MILES_PER_KM;

    last=0;
    double offset = 0; // always start from zero seconds (e.g. intervals start at and offset in ride)
    bool first = true;

    RideFilePoint *lp=NULL;
    foreach(RideFilePoint *p, input->dataPoints()) {

        // yuck! nasty data
        if (p->secs > (25*60*60)) return;

        if (first) {
            offset = p->secs;
            first = false;
        }

        // fill gaps in recording with zeroes
        if (lp)
            for(double t=lp->secs+input->recIntSecs();
                (t + input->recIntSecs()) < p->secs;
                t += input->recIntSecs()) {
                points << QPointF(t-offset, 0);
                pointsd << QPointF(t-offset, p->km * convert); // not zero !!!! this is a map from secs -> km not a series
            }

        // lets not go backwards -- or two samples at the same time
        if ((lp && p->secs > lp->secs) || !lp) {
            points << QPointF(p->secs - offset, p->watts);
            pointsd << QPointF(p->secs - offset, p->km * convert);
        }

        // update state
        last = p->secs - offset;
        lp = p;
    }

    // Create a spline
    distance.setSplineType(QwtSpline::Periodic);
    distance.setPoints(QPolygonF(pointsd));
    smoothed.setSplineType(QwtSpline::Periodic);
    smoothed.setPoints(QPolygonF(points));

    // Get CP
    CP = 250; // default
    WPRIME = 20000;
    if (input->context->athlete->zones()) {
        int zoneRange = input->context->athlete->zones()->whichRange(input->startTime().date());
        CP = zoneRange >= 0 ? input->context->athlete->zones()->getCP(zoneRange) : 0;
        WPRIME = zoneRange >= 0 ? input->context->athlete->zones()->getWprime(zoneRange) : 0;

        // did we override CP in metadata / metrics ?
        int oCP = input->getTag("CP","0").toInt();
        if (oCP) CP=oCP;
        int oW = input->getTag("W'","0").toInt();
        if (oW) WPRIME=oW;
    }
    minY = maxY = WPRIME;

    // input array contains the actual W' expenditure
    // and will also contain non-zero values
    double totalBelowCP=0;
    double countBelowCP=0;
    powerValues.resize(last+1);
    EXP = 0;
    for (int i=0; i<last; i++) {

        int value = smoothed.value(i);
        if (value < 0) value = 0; // don't go negative now

        powerValues[i] = value > CP ? value-CP : 0;

        if (value < CP) {
            totalBelowCP += value;
            countBelowCP++;
        } else EXP += value; // total expenditure above CP
    }

    if (countBelowCP > 0)
        TAU = 546.00f * exp(-0.01*(CP - (totalBelowCP/countBelowCP))) + 316.00f;
    else
        TAU = 546.00f * exp(-0.01*(CP)) + 316.00f;

    TAU = int(TAU); // round it down

    // STEP 2: ITERATE OVER DATA TO CREATE W' DATA SERIES

    if (integral) {

        // integral formula Skiba et al

        minY = WPRIME;
        maxY = WPRIME;
        values.resize(last+1);
        xvalues.resize(last+1);
        xdvalues.resize(last+1);

        QVector<double> myvalues(last+1);

        WPrimeIntegrator a(powerValues, 0, last, TAU);

        a.start();
        a.wait();

        // sum values
        for (int t=0; t<=last; t++) {
            values[t] = a.output[t];
            xvalues[t] = t / 60.00f;
        }

        // now subtract WPRIME and work out minimum etc
        for(int t=0; t <= last; t++) {
            double value = WPRIME - values[t];
            values[t] = value;
            xdvalues[t] = distance.value(t);

            if (value > maxY) maxY = value;
            if (value < minY) minY = value;
        }

    } else {

        // differential equation Froncioni / Clarke

        // lets run forward from 0s to end of ride
        minY = WPRIME;
        maxY = WPRIME;
        values.resize(last+1);
        xvalues.resize(last+1);
        xdvalues.resize(last+1);

        double W = WPRIME;
        for (int t=0; t<=last; t++) {

            if(smoothed.value(t) < CP) {
                W  = W + (CP-smoothed.value(t))*(WPRIME-W)/WPRIME;
            } else {
                W  = W + (CP-smoothed.value(t));
            }

            if (W > maxY) maxY = W;
            if (W < minY) minY = W;

            values[t] = W;
            xvalues[t] = double(t) / 60.00f;
            xdvalues[t] = distance.value(t);
        }
    }

    if (minY < -30000) minY = 0; // the data is definitely out of bounds!
                                 // so lets not exacerbate the problem - truncate

    //qDebug()<<"compute W'bal curve took"<<time.elapsed();

    // STEP 3: FIND MATCHES

    // SMOOTH DATA SERIES 

    // get raw data adjusted to 1s intervals (as before)
    smoothArray.resize(last+1);
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
    mxdvalues.clear();
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
                match.exhaust = values[match.stop] <= 500 ? true : false; // its to exhaustion!
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
            mxdvalues << xdvalues[match.start];
            mvalues << values[match.stop];
            mxvalues << xvalues[match.stop];
            mxdvalues << xdvalues[match.stop];
        }
    }
}

void
WPrime::setErg(ErgFile *input)
{
    bool integral = (appsettings->value(NULL, GC_WBALFORM, "int").toString() == "int");

    QTime time; // for profiling performance of the code
    time.start();

    // reset from previous
    values.resize(0); // the memory is kept for next time so this is efficient
    xvalues.resize(0);

    // Get CP
    CP = 250; // defaults
    WPRIME = 20000;

    if (input->context->athlete->zones()) {
        int zoneRange = input->context->athlete->zones()->whichRange(QDate::currentDate());
        CP = zoneRange >= 0 ? input->context->athlete->zones()->getCP(zoneRange) : 250;
        WPRIME = zoneRange >= 0 ? input->context->athlete->zones()->getWprime(zoneRange) : 20000;
    }

    // no data or no power data then forget it.
    bool bydist = (input->format == CRS) ? true : false;
    if (!input->isValid() || bydist) {
        return; // needs to be a valid erg file...
    }

    minY = maxY = WPRIME;

    if (integral) {

        last = input->Duration / 1000; 
        values.resize(last);
        xvalues.resize(last);

        // input array contains the actual W' expenditure
        // and will also contain non-zero values
        double totalBelowCP=0;
        double countBelowCP=0;
        QVector<int> powerValues(last+1);
        EXP = 0;
        for (int i=0; i<last; i++) {

            // get watts at point in time
            int lap;
            int value = input->wattsAt(i*1000, lap);

            powerValues[i] = value > CP ? value-CP : 0;

            if (value < CP) {
                totalBelowCP += value;
                countBelowCP++;
            } else EXP += value; // total expenditure above CP
        }

        TAU = appsettings->cvalue(input->context->athlete->cyclist, GC_WBALTAU, 300).toInt();

        // lets run forward from 0s to end of ride
        values.resize(last+1);
        xvalues.resize(last+1);

        QVector<double> myvalues(last+1);

        WPrimeIntegrator a(powerValues, 0, last, TAU);

        a.start();
        a.wait();

        // sum values
        for (int t=0; t<=last; t++) {
            values[t] = a.output[t];
            xvalues[t] = t * 1000.00f;
        }

        // now subtract WPRIME and work out minimum etc
        for(int t=0; t <= last; t++) {
            double value = WPRIME - values[t];
            values[t] = value;

            if (value > maxY) maxY = value;
            if (value < minY) minY = value;
        }

    } else {

        // how many points ?
        last = input->Duration / 1000; 
        values.resize(last);
        xvalues.resize(last);

        // input array contains the actual W' expenditure
        // and will also contain non-zero values
        double W = WPRIME;
        int lap; // passed by reference
        for (int i=0; i<last; i++) {

            // get watts at point in time
            int value = input->wattsAt(i*1000, lap);

            if(value < CP) {
                W  = W + (CP-value)*(WPRIME-W)/WPRIME;
            } else {
                W  = W + (CP-value);
            }

            if (W > maxY) maxY = W;
            if (W < minY) minY = W;

            values[i] = W;
            xvalues[i] = i*1000;
        }
    }

    if (minY < -30000) minY = 0; // the data is definitely out of bounds!
                                 // so lets not exacerbate the problem - truncate
}

double
WPrime::PCP()
{
    if (PCP_) return PCP_;

    // check WPRIME is correct otherwise we will run forever!
    // if its way off don't even try!
    if (minY < -10000 || WPRIME < 10000) return PCP_ = 0; // Wprime not set properly

    int cp = CP;
    do {
    
        if (minForCP(cp) > 0) return PCP_=cp;
        else cp += 3; // +/- 3w is ok, especially since +/- 2kJ is typical accuracy for W' anyway

    } while (cp <= 500);

    return PCP_=cp;
}

int 
WPrime::minForCP(int cp)
{
    // STEP 2: ITERATE OVER DATA TO CREATE W' DATA SERIES

    // lets run forward from 0s to end of ride
    int min = WPRIME;
    double W = WPRIME;
    for (int t=0; t<=last; t++) {

        if(smoothed.value(t) < cp) {
            W  = W + (cp-smoothed.value(t))*(WPRIME-W)/WPRIME;
        } else {
            W  = W + (cp-smoothed.value(t));
        }

        if (W < min) min = W;
    }
    return min;
}

double
WPrime::maxMatch()
{
    double max=0;
    foreach(struct Match match, matches) 
        if (match.cost > max) max = match.cost;

    return max;
}


// decay and integrate -- split into threads for best performance
WPrimeIntegrator::WPrimeIntegrator(QVector<int> &source, int begin, int end, double TAU) :
    source(source), begin(begin), end(end), TAU(TAU)
{
    output.resize(source.size());
}

void
WPrimeIntegrator::run()
{
    // run from start to stop adding decay to end
    double I = 0.00f;
    for (int t=0; t<=end; t++) {

        I += exp(((double)(t) / TAU)) * source[t];
        output[t] = exp(-((double)(t) / TAU)) * I;
    }
}

//
// HTML zone summary
//
QString
WPrime::summarize(int WPRIME, QVector<double> wtiz, QVector<double> wcptiz, QVector<double>wworktiz, QColor color)
{
    // if wtiz is not 4 big return empty
    if (wtiz.count() != 4) return "";

    // otherwise lets go
    QString summary;

    // W' used for summary
    summary += "<table align=\"center\" width=\"70%\" border=\"0\">";
    summary += "<tr><td align=\"center\">";
    summary += QString(tr("W' (Joules): %1")).arg(WPRIME);
    summary += "</td></tr></table>";

    // Heading
    summary += "<table align=\"center\" width=\"70%\" ";
    summary += "border=\"0\">";
    summary += "<tr>";
    summary += tr("<td align=\"center\">Zone</td>");
    summary += tr("<td align=\"center\">Description</td>");
    summary += tr("<td align=\"center\">High (J)</td>");
    summary += tr("<td align=\"center\">Low (J)</td>");
    summary += tr("<td align=\"center\">Work (kJ)</td>");
    summary += tr("<td align=\"center\">Time</td>");
    summary += tr("<td align=\"center\">%</td>");
    summary += tr("<td align=\"center\">Above CP Time</td>");
    summary += tr("<td align=\"center\">%</td>");
    summary += "</tr>";

    // calc totals to use for percentages
    double duration = 0;
    foreach(double v, wtiz) {
        duration += v;
    }

    for (int zone = 0; zone < 4; zone++) {

        // alternating rows
        if (zone % 2 == 0) summary += "<tr bgcolor='" + color.name() + "'>";
        else summary += "<tr>"; 

        // basics
        summary += QString("<td align=\"center\">%1</td>").arg(wbal_zones[zone].name);
        summary += QString("<td align=\"center\">%1</td>").arg(qApp->translate("wbalzone", wbal_zones[zone].desc));
        summary += QString("<td align=\"center\">%1</td>").arg(WPRIME - (WPRIME / 100.0f * wbal_zones[zone].lo), 0, 'f', 0);

        if (zone == 3)
            summary += "<td align=\"center\">MIN</td>";
        else
            summary += QString("<td align=\"center\">%1</td>").arg(WPRIME - (WPRIME / 100.0f * wbal_zones[zone].hi), 0, 'f', 0); 
        summary += QString("<td align=\"center\">%1</td>").arg(wworktiz[zone], 0, 'f', 1);
        summary += QString("<td align=\"center\">%1</td>").arg(time_to_string((unsigned) round(wtiz[zone])));
        if (duration == 0) {
            summary += QString("<td align=\"center\">0</td>");
        } else {
            summary += QString("<td align=\"center\">%1</td>").arg((double)wtiz[zone]/duration * 100, 0, 'f', 0);
        }
        summary += QString("<td align=\"center\">%1</td>").arg(time_to_string((unsigned) round(wcptiz[zone])));
        if (wtiz[zone] == 0) {
            summary += QString("<td align=\"center\">0</td>");
        } else {
            summary += QString("<td align=\"center\">%1</td>").arg((double)wcptiz[zone]/wtiz[zone] * 100, 0, 'f', 0);
        }
        summary += "</tr>";
    }
    summary += "</table>";
    return summary;
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
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
        setPrecision(1);
    }
    void compute(const RideFile *r, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        setValue(const_cast<RideFile*>(r)->wprimeData()->minY / 1000.00f);
    }

    bool canAggregate() { return false; }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new MinWPrime(*this); }
};

class MaxWPrime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(MaxWPrime);

    public:

    MaxWPrime()
    {
        setSymbol("skiba_wprime_max"); // its expressing min W'bal as as percentage of WPrime
        setInternalName("Max W' Expended");
    }
    void initialize() {
        setName(tr("Max W' Expended"));
        setType(RideMetric::Peak);
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setPrecision(0);
    }
    void compute(const RideFile *r, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        setValue(const_cast<RideFile*>(r)->wprimeData()->maxE());
    }

    bool canAggregate() { return false; }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new MaxWPrime(*this); }
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
        setType(RideMetric::Peak);
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
        setPrecision(1);
    }
    void compute(const RideFile *r, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        setValue(const_cast<RideFile*>(r)->wprimeData()->maxMatch()/1000.00f);
    }

    bool canAggregate() { return false; }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new MaxMatch(*this); }
};

class Matches : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(Matches);

    public:

    Matches()
    {
        setSymbol("skiba_wprime_matches");
        setInternalName("W'bal Matches > 2KJ");
    }
    void initialize() {
        setName(tr("W'bal Matches"));
        setType(RideMetric::Total);
        setPrecision(0);
    }
    void compute(const RideFile *r, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        int matches=0;
        foreach(Match m, const_cast<RideFile*>(r)->wprimeData()->matches) {
            if (m.cost > 2000) matches++; // 2kj is minimum size
        }
        setValue(matches);
    }

    bool canAggregate() { return false; }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new Matches(*this); }
};

class WPrimeTau : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WPrimeTau);

    public:

    WPrimeTau()
    {
        setSymbol("skiba_wprime_tau");
        setInternalName("W'bal TAU");
    }
    void initialize() {
        setName(tr("W'bal TAU"));
        setType(RideMetric::Low);
        setMetricUnits(tr(""));
        setImperialUnits(tr(""));
        setPrecision(0);
    }
    void compute(const RideFile *r, const Zones *, int,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        setValue(const_cast<RideFile*>(r)->wprimeData()->TAU);
    }

    bool canAggregate() { return false; }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new WPrimeTau(*this); }
};

class WPrimeExp : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WPrimeExp);

    public:

    WPrimeExp()
    {
        setSymbol("skiba_wprime_exp");
        setInternalName("W' Work");
    }
    void initialize() {
        setName(tr("W' Work"));
        setType(RideMetric::Total);
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
        setPrecision(0);
    }
    void compute(const RideFile *r, const Zones *zones, int zonerange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        int cp = r->getTag("CP","0").toInt();
        if (!cp && zones && zonerange >=0) cp = zones->getCP(zonerange);

        double total = 0;
        double secs = 0;
        foreach(const RideFilePoint *point, r->dataPoints()) {
            if (cp && point->watts > cp)  {
                total += r->recIntSecs() * (point->watts - cp);
                secs += r->recIntSecs();
            }
        }
        setValue(total/1000.00f);
        setCount(secs);
    }

    bool canAggregate() { return false; }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new WPrimeExp(*this); }
};

class WPrimeWatts : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WPrimeWatts);

    public:

    WPrimeWatts()
    {
        setSymbol("skiba_wprime_watts");
        setInternalName("W' Watts");
    }
    void initialize() {
        setName(tr("W' Power"));
        setType(RideMetric::Total);
        setMetricUnits(tr("watts"));
        setImperialUnits(tr("watts"));
        setPrecision(0);
    }
    void compute(const RideFile *r, const Zones *zones, int zonerange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        int cp = r->getTag("CP","0").toInt();
        if (!cp && zones && zonerange >=0) cp = zones->getCP(zonerange);

        double total = 0;
        double secs = 0;
        foreach(const RideFilePoint *point, r->dataPoints()) {
            if (cp && point->watts > cp)  {
                total += r->recIntSecs() * (point->watts - cp);
            }
            secs += r->recIntSecs();
        }
        setValue(total/secs);
        setCount(secs);
    }

    bool canAggregate() { return false; }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new WPrimeWatts(*this); }
};

class CPExp : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(CPExp);

    public:

    CPExp()
    {
        setSymbol("skiba_cp_exp");
        setInternalName("Below CP Work");
    }
    void initialize() {
        setName(tr("Below CP Work"));
        setType(RideMetric::Total);
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
        setPrecision(0);
    }
    void compute(const RideFile *r, const Zones *zones, int zonerange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *) {

        int cp = r->getTag("CP","0").toInt();
        if (!cp && zones && zonerange >=0) cp = zones->getCP(zonerange);

        double total = 0;
        double secs = 0;
        foreach(const RideFilePoint *point, r->dataPoints()) {
            if (cp && point->watts >=0) {
                if (point->watts > cp) 
                    total += r->recIntSecs() * cp;
                else 
                    total += r->recIntSecs() * point->watts;
                secs += r->recIntSecs();
            }
        }
        setValue(total/1000.00f);
        setCount(secs);
    }

    bool canAggregate() { return false; }
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new CPExp(*this); }
};

// time in zone
class WZoneTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WZoneTime)

    int level;

    public:

    WZoneTime() : level(0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
    }
    bool isTime() const { return true; }
    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *)
    {

        double WPRIME = zoneRange >= 0 ? zones->getWprime(zoneRange) : 20000;

        // 4 zones
        QVector<double> tiz(4);
        tiz.fill(0.0f);

        RideFile *cride = const_cast<RideFile*>(ride);
        if (cride->wprimeData() && cride->wprimeData()->ydata().count()) {

            foreach(int value, cride->wprimeData()->ydata()) {

                // percent is PERCENT OF W' USED
                double percent = 100.0f - ((double (value) / WPRIME) * 100.0f);
                if (percent < 0.0f) percent = 0.0f;
                if (percent > 100.0f) percent = 100.0f;

                // and zones in 1s increments
                if (percent <= 25.0f) tiz[0]++;
                else if (percent <= 50.0f) tiz[1]++;
                else if (percent <= 75.0f) tiz[2]++;
                else tiz[3]++;
            }

        } 
        setValue(tiz[level]);
    }

    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new WZoneTime(*this); }
};

class WZoneTime1 : public WZoneTime {
    Q_DECLARE_TR_FUNCTIONS(WZoneTime1)

    public:
        WZoneTime1()
        {
            setLevel(1);
            setSymbol("wtime_in_zone_L1");
            setInternalName("W1 W'bal Low Fatigue");
        }
        void initialize ()
        {
            setName(tr("W1 W'bal Low Fatigue"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new WZoneTime1(*this); }
};
class WZoneTime2 : public WZoneTime {
    Q_DECLARE_TR_FUNCTIONS(WZoneTime2)

    public:
        WZoneTime2()
        {
            setLevel(2);
            setSymbol("wtime_in_zone_L2");
            setInternalName("W2 W'bal Moderate Fatigue");
        }
        void initialize ()
        {
            setName(tr("W2 W'bal Moderate Fatigue"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new WZoneTime2(*this); }
};
class WZoneTime3 : public WZoneTime {
    Q_DECLARE_TR_FUNCTIONS(WZoneTime3)

    public:
        WZoneTime3()
        {
            setLevel(3);
            setSymbol("wtime_in_zone_L3");
            setInternalName("W3 W'bal Heavy Fatigue");
        }
        void initialize ()
        {
            setName(tr("W3 W'bal Heavy Fatigue"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new WZoneTime3(*this); }
};
class WZoneTime4 : public WZoneTime {
    Q_DECLARE_TR_FUNCTIONS(WZoneTime4)

    public:
        WZoneTime4()
        {
            setLevel(4);
            setSymbol("wtime_in_zone_L4");
            setInternalName("W4 W'bal Severe Fatigue");
        }
        void initialize ()
        {
            setName(tr("W4 W'bal Severe Fatigue"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new WZoneTime4(*this); }
};

// time in zone
class WCPZoneTime : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WCPZoneTime)

    int level;

    public:

    WCPZoneTime() : level(0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("seconds"));
        setImperialUnits(tr("seconds"));
        setPrecision(0);
        setConversion(1.0);
    }
    bool isTime() const { return true; }
    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *)
    {

        double WPRIME = 20000;
        if (zones && zoneRange > 0) {
            WPRIME = zones->getWprime(zoneRange);
        }

        // did we override CP in metadata / metrics ?
        int oW = ride->getTag("W'","0").toInt();
        if (oW) WPRIME=oW;

        // 4 zones
        QVector<double> tiz(4);
        tiz.fill(0.0f);

        RideFile *cride = const_cast<RideFile*>(ride);
        int i=0;
        if (cride->wprimeData() && cride->wprimeData()->ydata().count()) {

            // get the power values
            foreach(int value, cride->wprimeData()->ydata()) {

                // skip if below CP
                if (cride->wprimeData()->powerValues[i++] <= 0) continue;

                // percent is PERCENT OF W' USED
                double percent = 100.0f - ((double (value) / WPRIME) * 100.0f);
                if (percent < 0.0f) percent = 0.0f;
                if (percent > 100.0f) percent = 100.0f;

                // and zones in 1s increments
                if (percent <= 25.0f) tiz[0]++;
                else if (percent <= 50.0f) tiz[1]++;
                else if (percent <= 75.0f) tiz[2]++;
                else tiz[3]++;
            }

        } 
        setValue(tiz[level]);
    }

    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new WCPZoneTime(*this); }
};

class WCPZoneTime1 : public WCPZoneTime {
    Q_DECLARE_TR_FUNCTIONS(WCPZoneTime1)

    public:
        WCPZoneTime1()
        {
            setLevel(1);
            setSymbol("wcptime_in_zone_L1");
            setInternalName("W1 Above CP W'bal Low Fatigue");
        }
        void initialize ()
        {
            setName(tr("W1 Above CP W'bal Low Fatigue"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new WCPZoneTime1(*this); }
};
class WCPZoneTime2 : public WCPZoneTime {
    Q_DECLARE_TR_FUNCTIONS(WCPZoneTime2)

    public:
        WCPZoneTime2()
        {
            setLevel(2);
            setSymbol("wcptime_in_zone_L2");
            setInternalName("W2 Above CP W'bal Moderate Fatigue");
        }
        void initialize ()
        {
            setName(tr("W2 Above CP W'bal Moderate Fatigue"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new WCPZoneTime2(*this); }
};
class WCPZoneTime3 : public WCPZoneTime {
    Q_DECLARE_TR_FUNCTIONS(WCPZoneTime3)

    public:
        WCPZoneTime3()
        {
            setLevel(3);
            setSymbol("wcptime_in_zone_L3");
            setInternalName("W3 Above CP W'bal Heavy Fatigue");
        }
        void initialize ()
        {
            setName(tr("W3 Above CP W'bal Heavy Fatigue"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new WCPZoneTime3(*this); }
};
class WCPZoneTime4 : public WCPZoneTime {
    Q_DECLARE_TR_FUNCTIONS(WCPZoneTime4)

    public:
        WCPZoneTime4()
        {
            setLevel(4);
            setSymbol("wcptime_in_zone_L4");
            setInternalName("W4 Above CP W'bal Severe Fatigue");
        }
        void initialize ()
        {
            setName(tr("W4 W'bal Severe Fatigue"));
            setMetricUnits(tr("seconds"));
            setImperialUnits(tr("seconds"));
        }
        RideMetric *clone() const { return new WCPZoneTime4(*this); }
};

// work in zone
class WZoneWork : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(WZoneWork)

    int level;

    public:

    WZoneWork() : level(0)
    {
        setType(RideMetric::Total);
        setMetricUnits(tr("kJ"));
        setImperialUnits(tr("kJ"));
        setPrecision(1);
        setConversion(1.0);
    }
    bool isTime() const { return false; }
    void setLevel(int level) { this->level=level-1; } // zones start from zero not 1
    void compute(const RideFile *ride, const Zones *zones, int zoneRange,
                 const HrZones *, int,
                 const QHash<QString,RideMetric*> &,
                 const Context *)
    {

        double WPRIME = zoneRange >= 0 ? zones->getWprime(zoneRange) : 20000;

        // 4 zones
        QVector<double> tiz(4);
        tiz.fill(0.0f);

        RideFile *cride = const_cast<RideFile*>(ride);
        if (cride->wprimeData() && cride->wprimeData()->ydata().count()) {

            int i=0;
            foreach(int value, cride->wprimeData()->ydata()) {

                // watts is joules when in 1s intervals
                double kj = cride->wprimeData()->smoothArray[i++]/1000.0f;

                // percent is PERCENT OF W' USED
                double percent = 100.0f - ((double (value) / WPRIME) * 100.0f);
                if (percent < 0.0f) percent = 0.0f;
                if (percent > 100.0f) percent = 100.0f;

                if (percent <= 25.0f) tiz[0]+=kj;
                else if (percent <= 50.0f) tiz[1]+=kj;
                else if (percent <= 75.0f) tiz[2]+=kj;
                else tiz[3]+=kj;
            }

        } 
        setValue(tiz[level]);
    }

    bool canAggregate() { return false; }
    void aggregateWith(const RideMetric &) {}
    bool isRelevantForRide(const RideItem *ride) const { return ride->present.contains("P") || (!ride->isSwim && !ride->isRun); }
    RideMetric *clone() const { return new WZoneWork(*this); }
};

class WZoneWork1 : public WZoneWork {
    Q_DECLARE_TR_FUNCTIONS(WZoneWork1)

    public:
        WZoneWork1()
        {
            setLevel(1);
            setSymbol("wwork_in_zone_L1");
            setInternalName("W1 W'bal Work Low Fatigue");
        }
        void initialize ()
        {
            setName(tr("W1 W'bal Work Low Fatigue"));
            setMetricUnits(tr("kJ"));
            setImperialUnits(tr("kJ"));
        }
        RideMetric *clone() const { return new WZoneWork1(*this); }
};
class WZoneWork2 : public WZoneWork {
    Q_DECLARE_TR_FUNCTIONS(WZoneWork2)

    public:
        WZoneWork2()
        {
            setLevel(2);
            setSymbol("wwork_in_zone_L2");
            setInternalName("W2 W'bal Work Moderate Fatigue");
        }
        void initialize ()
        {
            setName(tr("W2 W'bal Work Moderate Fatigue"));
            setMetricUnits(tr("kJ"));
            setImperialUnits(tr("kJ"));
        }
        RideMetric *clone() const { return new WZoneWork2(*this); }
};
class WZoneWork3 : public WZoneWork {
    Q_DECLARE_TR_FUNCTIONS(WZoneWork3)

    public:
        WZoneWork3()
        {
            setLevel(3);
            setSymbol("wwork_in_zone_L3");
            setInternalName("W3 W'bal Work Heavy Fatigue");
        }
        void initialize ()
        {
            setName(tr("W3 W'bal Work Heavy Fatigue"));
            setMetricUnits(tr("kJ"));
            setImperialUnits(tr("kJ"));
        }
        RideMetric *clone() const { return new WZoneWork3(*this); }
};
class WZoneWork4 : public WZoneWork {
    Q_DECLARE_TR_FUNCTIONS(WZoneWork4)

    public:
        WZoneWork4()
        {
            setLevel(4);
            setSymbol("wwork_in_zone_L4");
            setInternalName("W4 W'bal Work Severe Fatigue");
        }
        void initialize ()
        {
            setName(tr("W4 W'bal Work Severe Fatigue"));
            setMetricUnits(tr("kJ"));
            setImperialUnits(tr("kJ"));
        }
        RideMetric *clone() const { return new WZoneWork4(*this); }
};

// add to catalogue
static bool addMetrics() {
    RideMetricFactory::instance().addMetric(MinWPrime());
    RideMetricFactory::instance().addMetric(MaxWPrime()); // same thing expressed as a maximum
    RideMetricFactory::instance().addMetric(Matches());
    RideMetricFactory::instance().addMetric(MaxMatch());
    RideMetricFactory::instance().addMetric(WPrimeTau());
    RideMetricFactory::instance().addMetric(WPrimeExp());
    RideMetricFactory::instance().addMetric(WPrimeWatts());
    RideMetricFactory::instance().addMetric(WCPZoneTime1());
    RideMetricFactory::instance().addMetric(WCPZoneTime2());
    RideMetricFactory::instance().addMetric(WCPZoneTime3());
    RideMetricFactory::instance().addMetric(WCPZoneTime4());
    RideMetricFactory::instance().addMetric(WZoneTime1());
    RideMetricFactory::instance().addMetric(WZoneTime2());
    RideMetricFactory::instance().addMetric(WZoneTime3());
    RideMetricFactory::instance().addMetric(WZoneTime4());
    RideMetricFactory::instance().addMetric(WZoneWork1());
    RideMetricFactory::instance().addMetric(WZoneWork2());
    RideMetricFactory::instance().addMetric(WZoneWork3());
    RideMetricFactory::instance().addMetric(WZoneWork4());
    RideMetricFactory::instance().addMetric(CPExp());
    return true;
}

static bool added = addMetrics();
