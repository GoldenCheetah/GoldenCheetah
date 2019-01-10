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

//
// for info on Banister impulse response modeling see:
// "Modeling human performance in running" (1990)
//     R. Hugh Morton, John R. Fitz Clarke and Eric W. Banister.
//
// for advice on implementation and optimization see:
// "Rationale and resources for teaching the mathematical modeling
//  of athletic training and performance" (2013)
//     David C. Clarke, Philip F. Skiba
//
// see the spreadsheet "Clarke_Rationale_2013s.xlsx" in this repo
// in the doc/contrib folder.
//

#include "Banister.h"

#include "RideMetric.h"
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "RideCache.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "IntervalItem.h"
#include "LTMOutliers.h"
#include "LTMSettings.h"
#include "Units.h"
#include "Zones.h"
#include "cmath"
#include <assert.h>
#include <algorithm>
#include <QVector>
#include <QMutex>
#include <QApplication>
#include "lmcurve.h"

// the mean athlete from opendata analysis
const double typical_CP = 261,
             typical_WPrime = 15500,
             typical_Pmax = 1100;

// minimum number of days that is gap in seasons
const int typical_SeasonBreak = 42;

// for debugging
#ifndef BANISTER_DEBUG
#define BANISTER_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (BANISTER_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (BANISTER_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif
//
// Banister IR model
//
// Translates training impulse (I) for e.g. BikeScore into performance response (R) e.g. CP
//
// Two curves Negative Training Effect (NTE) and Positive Training Effect (PTE)
// are computed and added together give a Performance Response P(t)
//
// Each curve has two parameters; a coefficient to map I to R (k) and a decay (t)
//
// This class collects data and performs a model fit to estimate k1,k2,t1,t2.
//
// At present the Response is fixed (Power Index) and the Impulse is user definable
// we are likely to expand this to allow multiple performance measures to allow
// the model to look at IR between specific type of impulse and a specific response
// e.g. supramaximal work to pmax, above threshold to v02max etc.
//
//
Banister::Banister(Context *context, QString symbol, double t1, double t2, double k1, double k2) :
    symbol(symbol), k1(k1), k2(k2), t1(t1), t2(t2), days(0), context(context)
{
    refresh();
}

// get at the computed value
double
Banister::value(QDate date, int type)
{
    // check in range
    if (date > stop || date < start) return 0;
    int index=date.toJulianDay()-start.toJulianDay();

    switch(type) {
    case BANISTER_NTE: return data[index].nte;
    case BANISTER_PTE: return data[index].pte;
    case BANISTER_CP: return data[index].perf * typical_CP / 100;
    default:
    case BANISTER_PERFORMANCE: return data[index].perf;
    }
}

void
Banister::init()
{
    // ok we can initialise properly
    start = QDate();
    stop = QDate();
    rides = 0;
    meanscore = 0;
    days = 0;
    data.resize(0);
    windows.clear();

    // default values
    k1=0.2;
    k2=0.2;
    t1=31;
    t2=15;
}

void
Banister::refresh()
{
    // clear
    init();

    // all data
    QDate f, l;
    if (context->athlete->rideCache->rides().count()) {

        // set date range - extend to a year after last ride
        f= context->athlete->rideCache->rides().first()->dateTime.date();
        l= context->athlete->rideCache->rides().last()->dateTime.date().addYears(1);

    } else
        return;

    // resize the data
    if (l.toJulianDay() - f.toJulianDay() <= 0) return;

    // ok we can initialise properly
    start = f;
    stop = l;
    days = stop.toJulianDay() - start.toJulianDay();
    data.resize(days);

    // now initialise with zero values
    data.fill(banisterData());

    // guess we will have only half as performances
    performanceDay.resize(days/2);
    performanceScore.resize(days/2);
    performances=0;

    //
    // 1. COLLECT IMPULSE AND RESPONSE DATA
    //

    // we can now fill with ride values
    foreach(RideItem *item, context->athlete->rideCache->rides()) {

        // load measure
        double score = item->getForSymbol(symbol);
        long day = item->dateTime.date().toJulianDay() - start.toJulianDay();
        data[day].score = score;

        // average out measures
        if (score>0) {
            rides++;
            meanscore += score; // averaged at end
        }

        // get best scoring performance *test* in the ride
        double todaybest=0;
        foreach(IntervalItem *i, item->intervals()) {
            if (i->istest()) {
                double pix=i->getForSymbol("power_index");
                if (pix > todaybest)  todaybest = pix;
            }
        }

        // is there a performance test already there for today?
        if (todaybest > 0) {
            if (performances > 0 && performanceDay[performances-1] == day) {
                if (performanceScore[performances-1] < todaybest)
                    performanceScore[performances-1] = todaybest;
            } else {
                performanceDay[performances] = day;
                performanceScore[performances] = todaybest;
                performances++;
            }
        }

        // if we didn't find a performance test in the ride lets see if there
        // is a weekly performance already identified
        if (!(todaybest > 0)) {
            Performance p = context->athlete->rideCache->estimator->getPerformanceForDate(item->dateTime.date());
            if (!p.submaximal && p.powerIndex > 0) {

                // its not submax
                todaybest = p.powerIndex;
                if (performances > 0 && performanceDay[performances-1] == day) {
                    if (performanceScore[performances-1] < todaybest) // validating with '<=' not '<' is vital for 2-a-days
                        performanceScore[performances-1] = todaybest;
                } else {
                    performanceDay[performances] = day;
                    performanceScore[performances] = todaybest;
                    performances++;
                }
            }
        }

        // add more space
        if (performances == performanceDay.size()) {
            performanceDay.resize(performanceDay.size() + (days/2));
            performanceScore.resize(performanceDay.size() + (days/2));
        }
    }

    // average out meanscore after accumulated above
    meanscore /= double(rides);

    // add performances to the banister data, might be useful later
    // the performanceDay and performanceScore vectors are just there
    // for model fitting calls to lmcurve.
    for(int i=0; i<performances; i++) {
        printd("Performance %d on day=%g score=%g\n", i, performanceDay[i], performanceScore[i]);
        data[performanceDay[i]].test = performanceScore[i];
    }

    //for (int i=0; i < data.length(); i++) printd("%d, %g, %g\n", i, data[i].score, data[i].test);
    printd("%ld non-zero rides, average score %g, %d performance tests\n", rides, meanscore, performances);

    //
    // 2. FIND FITTING WINDOWS
    //
    bool inwindow=false;
    long lastworkoutday=0;
    banisterFit adding(this);
    int testoffset=0;
    for(int i=0; i<data.length(); i++) {

        if (data[i].score>0) lastworkoutday=i;

        // found the beginning of a season
        if (!inwindow && data[i].score > 0) {
            inwindow=true;
            lastworkoutday=i;
            adding=banisterFit(this);
            adding.startIndex=i;
            adding.startDate=start.addDays(i);
        }

        // found a test in this season
        if (inwindow && data[i].test >0) {
            if (adding.testoffset==-1) adding.testoffset=testoffset;
            adding.tests++;
        }

        // move testoffset on
        if (data[i].test >0) testoffset++;

        // found the end of a season
        if (inwindow && data[i].score <= 0 && i-lastworkoutday > typical_SeasonBreak) {

            // only useful if we have 2 or more tests to fit to
            if (adding.tests >= 2) {
                adding.stopIndex=i;
                adding.stopDate=start.addDays(i);
                windows<<adding;
            }
            inwindow=false;
        }
    }

    foreach(banisterFit f, windows) {
        printd("window: %s to %s, %d tests (from %d) season %ld days\n",
                f.startDate.toString().toStdString().c_str(),
                f.stopDate.toString().toStdString().c_str(),
                f.tests, f.testoffset,
                f.stopIndex-f.startIndex);
    }

    //
    // EXPAND A FITTING WINDOW WHERE <5 TESTS
    //
    int i=0;
    while(i < windows.length() && windows.count() > 1) {

        // combine and remove prior
        if (windows[i].tests < 5 && i>0) {
            windows[i].combine(windows[i-1]);
            windows.removeAt(--i);
        } else i++;
    }

    foreach(banisterFit f, windows) {
        printd("post combined window: %s to %s, %d tests (from %d) season %ld days\n",
                f.startDate.toString().toStdString().c_str(),
                f.stopDate.toString().toStdString().c_str(),
                f.tests, f.testoffset,
                f.stopIndex-f.startIndex);
    }
}


double
banisterFit::f(double d, const double *parms)
{
    if (k1 > parms[0] || k1 < parms[0] ||
        k2 > parms[1] || k2 < parms[1] ||
        t1 > parms[2] || t1 < parms[2] ||
        t2 > parms[3] || t2 < parms[3] ||
        p0 > parms[4] || p0 < parms[4]) {

        // we'll keep the parameters passed
        k1 = parms[0];
        k2 = parms[1];
        t1 = parms[2];
        t2 = parms[3];
        p0 = parms[4];

        //printd("fit iter %s to %s [k1=%g k2=%g t1=%g t2=%g p0=%g]\n", startDate.toString().toStdString().c_str(), stopDate.toString().toStdString().c_str(), k1,k2,t1,t2,p0); // too much info even in debug, unless you want it
        compute(startIndex, stopIndex);
    }

    // return previously computed
    //printd("result perf(%s)=%g vs test=%g\n", parent->start.addDays(d).toString().toStdString().c_str(),parent->data[int(d)].perf, parent->data[int(d)].test);
    return parent->data[int(d)].perf;
}

void
banisterFit::compute(long start, long stop)
{
    // ack, we need to recompute our window using the parameters supplied
    bool first = true;
    for (int index=start; index < stop; index++) {

        // g and h are just accumulated training load with different decay parameters
        if (first) {
            parent->data[index].g =  parent->data[index].h = 0;
            first = false;
        } else {
            parent->data[index].g = (parent->data[index-1].g * exp (-1/t1)) + parent->data[index].score;
            parent->data[index].h = (parent->data[index-1].h * exp (-1/t2)) + parent->data[index].score;
        }

        // apply coefficients
        parent->data[index].pte = parent->data[index].g * k1;
        parent->data[index].nte = parent->data[index].h * k2;
        parent->data[index].perf = p0 + parent->data[index].pte - parent->data[index].nte;
    }
}

void
banisterFit::combine(banisterFit other)
{
    if (other.startIndex < startIndex) { // before
        // after
        startIndex = other.startIndex;
        startDate = other.startDate;
        tests += other.tests;
        testoffset = other.testoffset;

    } else if (other.startIndex > startIndex){ // after
        // before
    }
}

// used to wrap a function call when deriving parameters
static QMutex calllmfit;
static banisterFit *calllmfitmodel = NULL;
static double calllmfitf(double t, const double *p) {
return static_cast<banisterFit*>(calllmfitmodel)->f(t, p);

}
void Banister::fit()
{
    for(int i=0; i<windows.length(); i++) {

        double prior[5]={ k1, k2, t1, t2, performanceScore[windows[i].testoffset] };

        lm_control_struct control = lm_control_double;
        control.patience = 1000; // more than this and there really is a problem
        lm_status_struct status;

        printd("fitting window %d start=%s [k1=%g k2=%g t1=%g t2=%g p0=%g]\n", i, windows[i].startDate.toString().toStdString().c_str(), prior[0], prior[1], prior[2], prior[3], prior[4]);

        // use forwarder via global variable, so mutex around this !
        calllmfit.lock();
        calllmfitmodel=&windows[i];


        //fprintf(stderr, "Fitting ...\n" ); fflush(stderr);
        lmcurve(5, prior, windows[i].tests, performanceDay.constData()+windows[i].testoffset, performanceScore.constData()+windows[i].testoffset,
                calllmfitf, &control, &status);

        // release for others
        calllmfit.unlock();

        if (status.outcome >= 0) {
            printd("window %d %s [k1=%g k2=%g t1=%g t2=%g p0=%g]\n", i, lm_infmsg[status.outcome], prior[0], prior[1], prior[2], prior[3], prior[4]);
        }
    }

#if 0 // doesn't really make sense for now
    // fill curves
    for(int i=0; i<windows.length(); i++) {
        if (i < (windows.length()-1))
            windows[i].compute(windows[i].stopIndex, windows[i+1].startIndex);
        else
            windows[i].compute(windows[i].stopIndex, data.length());
    }
#endif
}

//
// Convert power-duration of test, interval, ride to a percentage comparison
// to the mean athlete from opendata (100% would be a mean effort, whilst a
// score <100 is below average and a score >100 is above average
//
// For TTE tests in the 2-20 minute range we would expect the scores to
// be largely similar; so useful for evaluating a set of maximal efforts
// and also useful to normalise TTEs for use in the Banister model as a
// performance measurement.
//

// power index metric
double powerIndex(double averagepower, double duration)
{
    // so now lets work out what the 3p model says the
    // typical athlete would do for the same duration
    //
    // P(t) = W' / (t - (W'/(CP-Pmax))) + CP
    double typicalpower = (typical_WPrime / (duration - (typical_WPrime/(typical_CP-typical_Pmax)))) + typical_CP;

    // make sure we got a sensible value
    if (typicalpower < 0 || typicalpower > 2500)  return 0;

    // we could convert to linear work time model before
    // indexing, but they cancel out so no value in doing so
    return(100.0 * averagepower/typicalpower);
}

class PowerIndex : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(PowerIndex)
    public:

    PowerIndex()
    {
        setSymbol("power_index");
        setInternalName("Power Index");
        setPrecision(1);
        setType(Peak); // not even sure aggregation makes sense
    }
    void initialize() {
        setName(tr("Power Index"));
        setMetricUnits(tr("%"));
        setImperialUnits(tr("%"));
        setDescription(tr("Power Index"));
    }

    void compute(RideItem *item, Specification spec, const QHash<QString,RideMetric*> &) {

        // no ride or no samples
        if (spec.isEmpty(item->ride())) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // calculate for this interval/ride
        double duration=0, averagepower=0;
        if (item->ride()->areDataPresent()->watts) {

            // loop through and count
            RideFileIterator it(item->ride(), spec);
            while (it.hasNext()) {
                struct RideFilePoint *point = it.next();
                // use duration as count for now, and accumulate power
                duration++;
                averagepower += point->watts;
            }
            if (duration >0) {
                averagepower /= duration; // convert to AP
                duration *= item->ride()->recIntSecs(); // convert to seconds
            }
        }

        // failed to get duration and average power
        if (duration <= 0 || averagepower <= 0) {
            setValue(RideFile::NIL);
            setCount(0);
            return;
        }

        // calculate power index, 0=out of bounds
        double pix = powerIndex(averagepower, duration);

        // we could convert to linear work time model before
        // indexing, but they cancel out so no value in doing so
        setValue(pix);
        setCount(pix > 0 ? 1 : 0);
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new PowerIndex(*this); }
};

static bool countAdded = RideMetricFactory::instance().addMetric(PowerIndex());
