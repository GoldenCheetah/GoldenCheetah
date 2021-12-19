/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#include "Utils.h"
#include "Statistic.h"
#include "DataFilter.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideNavigator.h"
#include "RideFileCache.h"
#include "PMCData.h"
#include "Banister.h"
#include "LTMSettings.h"
#include "VDOTCalculator.h"
#include "DataProcessor.h"
#include "GenericSelectTool.h" // for generic calculator
#include "SearchFilterBox.h" // for SearchFilterBox::matches
#include <QDebug>
#include <QMutex>
#include "lmcurve.h"
#include "LTMTrend.h" // for LR when copying CP chart filtering mechanism
#include "WPrime.h" // for LR when copying CP chart filtering mechanism
#include "FastKmeans.h" // for kmeans(...)

#ifdef GC_HAVE_SAMPLERATE
// we have libsamplerate
#include <samplerate.h>
#endif
#ifdef GC_WANT_PYTHON
#include "PythonEmbed.h"
QMutex pythonMutex;
#endif

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>

#include "Zones.h"
#include "PaceZones.h"
#include "HrZones.h"
#include "UserChart.h"

#include "DataFilter_yacc.h"

// control access to runtimes to avoid calls from multiple threads

// v4 functions
static struct {

    QString name;
    int parameters; // -1 is end of list, 0 is variable number, >0 is number of parms needed

} DataFilterFunctions[] = {

    // ALWAYS ADD TO BOTTOM OF LIST AS ARRAY OFFSET
    // USED IN SWITCH STATEMENT AT RUNTIME DO NOT BE
    // TEMPTED TO ADD IN THE MIDDLE !!!!

    // math.h
    { "cos", 1 },
    { "tan", 1 },
    { "sin", 1 },
    { "acos", 1 },
    { "atan", 1 },
    { "asin", 1 },
    { "cosh", 1 },
    { "tanh", 1 },
    { "sinh", 1 },
    { "acosh", 1 },
    { "atanh", 1 },
    { "asinh", 1 },

    { "exp", 1 },
    { "log", 1 },
    { "log10", 1 },

    { "ceil", 1 },
    { "floor", 1 },
    { "round", 0 }, // round(x) or round(x, dp)

    { "fabs", 1 },
    { "isinf", 1 },
    { "isnan", 1 },

    // primarily for working with vectors, but can
    // have variable number of parameters so probably
    // quite useful for a number of things
    { "sum", 0 },
    { "mean", 0 },
    { "max", 0 },
    { "min", 0 },
    { "count", 0 },

    // PMC functions
    { "lts", 1 },
    { "sts", 1 },
    { "sb", 1 },
    { "rr", 1 },

    // estimate
    { "estimate", 2 }, // estimate(model, (cp|ftp|w'|pmax|x))

    // more vector operations
    { "which", 0 }, // which(expr, ...) - create vector contain values that pass expr

    // set
    { "set", 3 }, // set(symbol, value, filter)
    { "unset", 2 }, // unset(symbol, filter)
    { "isset", 1 }, // isset(symbol) - is the metric or metadata overridden/defined

    // VDOT and time/distance functions
    { "vdottime", 2 }, // vdottime(VDOT, distance[km]) - result is seconds
    { "besttime", 1 }, // besttime(distance[km]) - result is seconds

    // XDATA access
    { "XDATA", 3 },     // e.g. xdata("WEATHER","HUMIDITY", repeat|sparse|interpolate |resample)

    // print to qDebug for debugging
    { "print", 1 },     // print(..) to qDebug for debugging

    // Data Processor functions on filtered activiies
    { "autoprocess", 1 }, // autoprocess(filter) to run auto data processors
    { "postprocess", 2 }, // postprocess(processor, filter) to run processor

    // XDATA access
    { "XDATA_UNITS", 2 }, // e.g. xdata("WEATHER", "HUMIDITY") returns "Relative Humidity"

    // Daily Measures Access by date
    { "measure", 3 },   // measure(DATE, "Hrv", "RMSSD")

    // how many performance tests in the ride?
    { "tests", 0 },     // tests() -or- tests(user|bests, duration|power) - with no parameters will
                        // just return the number of tests in a ride/date range, or with 2
                        // parameters will retrieve user defined or bests found by algorithm
                        // the last parameter defines if duration (secs) or power (watts) values are returned

    // banister function
    { "banister", 3 }, // banister(load_metric, perf_metric, nte|pte|perf|cp)

    // working with vectors

    { "c", 0 }, // return an array from concatated from paramaters (same as R) e.g. c(1,2,3,4,5)
    { "seq", 3 }, // create a vector with a range seq(start,stop,step)
    { "rep", 2 }, // create a vector of repeated values rep(value, n)
    { "length", 1}, // get length of a vector (can be zero where isnumber not a vector)

    { "append", 0}, // append vector append(symbol, expr [, at]) -- must reference a symbol
    { "remove", 3}, // remove vector elements remove(symbol, start, count) -- must reference a symbol
    { "mid", 3}, // subset of a vector mid(a,pos,count) -- returns a vector of size count from pos ion a

    { "samples", 1 }, // e.g. samples(POWER) - when on analysis view get vector of samples for the current activity
    { "metrics", 0 }, // metrics(Metrics [,start [,stop]]) - returns a vector of values for the metric specified
                      // if no start/stop is supplied it uses the currently selected date range, otherwise start thru today
                      // or start - stop.

    { "argsort", 2 }, // argsort(ascend|descend, list) - return a sorting index (ala numpy.argsort).

    { "multisort", 0 }, // multisort(ascend|descend, list1 [, list2, listn]) - sorts each list together, based upon list1, no limit to the
                        // number of lists but they must have the same length. the first list contains the values that define
                        // the sort order. since the sort is 'in-situ' the lists must all be user symbols. returns number of items
                        // sorted. this is impure from a functional programming perspective, but allows us to avoid using dataframes
                        // to manage x,y,z,t style data series when sorting.

    { "head", 2 }, // head(list, n) - returns vector of first n elements of list (or fewer if not so big)
    { "tail", 2 }, // tail(list, n) - returns vector of last n elements of list (or fewer if not so big)

    { "meanmax", 0 }, // meanmax(POWER|date [,start, stop]) - when on trend view get a vector of meanmaximal data for the specific series
                      // meanmax(x,y) - create a meanmaximal power curve from x/y data, x is seconds, y is value
                      // because the returned vector is at 1s resolution the data is interpolated using linear interpolation
                      // and resampled to 1s samples.

    { "pmc", 2 },  // pmc(symbol, stress|lts|sts|sb|rr|date) - get a vector of PMC series for the metric in symbol for the current date range.

    { "sapply", 2 }, // sapply(vector, expr) - returns a vector where expr has been applied to every element. x and i
                     // are both available in the expr for element value and index position.

    { "lr", 2 },   // lr(xlist, ylist) - linear regression on x,y co-ords returns vector [slope, intercept, r2, see]

    { "smooth", 0 }, // smooth(list, algorithm, ... parameters) - returns smoothed data.

    { "sqrt", 1 }, // sqrt(x) - returns square root, for vectors returns the sqrt of the sum

    { "lm", 3 }, // lm(formula, xseries, yseries) - fit using LM and return fit goodness.
                 // formula is an expression involving existing user symbols, their current values
                 // will be used as the starting values by the fit. once the fit has been completed
                 // the user symbols will contain the estimated parameters and lm will return some
                 // diagnostics goodness of fit measures [ success, RMSE, CV ] where a non-zero value
                 // for success means true, if it is false, RMSE and CV will be set to -1

    { "bool", 1 }, // bool(e) - will turn the passed parameter into a boolean with value 0 or 1
                   // this is useful for embedding logical expresissions into formulas. Since the
                   // grammar does not support (a*x>1), instead we can use a*bool(x>1). All non
                   // zero expressions will evaluate to 1.

    { "annotate", 0 }, // current supported annotations:
                       // annotate(label, string1, string2 .. stringn) - adds label at top of a chart
                       // annotate(voronoi, centers) - associated with a series on a user chart
                       // annotate(hline, label, style, value) - associated with a series on a user chart
                       // annotate(vline, label, style, value) - associated with a series on a user chart (see linestyle for vals below)
                       // annotate(lr, style, "colorname") - plot a linear regression for the series

    { "arguniq", 1 },  // returns an index of the uniq values in a vector, in the same way
                       // argsort returns an index, can then be used to select from samples
                       // or activity vectors

    { "multiuniq", 0 },// stable uniq will keep original sequence but remove duplicates, does
                       // not need the data to be sorted, as it uses argsort internally. As
                       // you can pass multiple vectors they are uniqued in sync with the first list.

    { "variance", 1 }, // variance(v) - calculates the variance for the elements in the vector.
    { "stddev", 1 },   // stddev(v) - calculates the standard deviation for elements in the vector.

    { "curve", 2 },    // curve(series, x|y|z|d|t) - fetch the computed values for another curve
                       // will need to be on a user chart, and the series will need to have already
                       // been computed.

    { "lowerbound", 2 }, // lowerbound(list, value) - returns the index of the first entry in list
                         // that does not compare less than value, analogous to std::lower_bound
                         // will return -1 if no value found. works with strings and numbers.

    { "cumsum", 1 },  // cumsum(v) - returns a vector of cumulative sum for vector v

    { "measures", 2 }, // measures(group, field|date) - returns vector of measures; where group
                       // is the class of measures e.g. "Hrv" or "Body", and field is the field
                       // name you want to retrieve e.g. "WeightKg" for "Body" and "RMSSD" for "Hrv"

    { "week", 1 },      // some date arithmetic functions, week and month convert a date (days since 01/01/1970
    { "month", 1 },     // to the week or month since 01/01/1970, and in reverse weekdate and monthdate
    { "weekdate", 1 },  // convert the week or month number to a date (days since 01/01/1970).
    { "monthdate", 1 },

    { "aggregate", 3 }, // aggregate(v, by, mean|sum|max|min|count) - returns an aggregate of vector
                        // v using the values in by to group, applies the func mean, sum etc when
                        // aggregating, by will not be sorted, so will aggregate as it is.
                        // by is coerced to a string vector to simplify the code

    { "exists", 1 },    // check if function or variable exists. returns 1 if true 0 if false.}

    { "mlr", 0 },       // mlr(yvector, xvector1 .. xvectorn) - multiple linear regression returns
                        // the beta (coefficients) for each x series 1-n, the covariance matrix
                        // is discarded for now. we could look at that later

    { "match", 2 },     // match(vector1, vector2) - returns a vector of indexes. For every element in vector1
                        // that is in vector2, the index of the first occurrence is returned.

    { "nonzero", 1 },   // nonzero(vector) - returns a vector of indexes to the zero values. this is a
                        // convenience function since it can be replicated using sapply, but this is much faster

    { "dist", 2 },      // dist(SERIES, data|bins) - get a distribution of data for the specific series
                        // e.g. HEARTRATE, SPEED et al, data returns the distribution data as a vector
                        // whilst bins returns the start value used for each bin

    { "median", 0 },    // median(v ..) - get the median value using the quickselect algorithm
    { "mode", 0 },      // mode(v ..) - get the mode average.

    { "bests", 0 },     // bests(date [, start [, stop] ]) -or- bests(SERIES, duration [, start [, stop]])
                        // this returns the peak values for the given duration across the currently selected
                        // date range, or for the given date range.
    { "daterange", 0 }, // daterange(start|stop) or daterange(from,to,expression) - first form gets the
                        // currently selected start/stop, second form sets from and to when executing the
                        // expression.
    { "quantile", 2 },  // quantile(vector, quantiles) - quantiles can be a number or a vector of numbers
                        // the vector does not need to be sorted as it will be sorted internally.

    { "bin", 2 },       // bin(values, bins) - returns a binned vector with values binned into bins passed
                        // each bin represents the lower value in the range, so a first bin of 0 will mean
                        // and values less than zero will be discarded, for the last bin any value greater
                        // than the value will be included. It is up to the user to manage this.

    { "rev", 1 },       // rev(vector) - returns vector with sequence reversed
    { "random", 1 },    // random(n) - generate a vector of random values (between 0 and 1) of size n

    { "interpolate", 4 }, // interpolate(algorithm, xvector, yvector, xvalues) - returns interpolated vector
                          // of yvalues for every value in xvalues by applying the algorithm for the data
                          // passed in xvector,yvector. The algorithm can be one of:
                          // linear, akima, steffen, more may be added later.
    { "resample", 3 },     // resample(old, new, vector) returns the vector resampled from old sample durations
                          // to new sample durations.

    { "estimates", 2 }, // estimates(model, (cp|ftp|w'|pmax|date)) - as per estimate above but returns a
                        // vector for all estimates for the curently selected date range.

    { "rank", 2 }, // rank(ascend|descend, list) - returns ranks for the list

    { "sort", 2 },     // sort(ascend|descend, list) - returns sorted list
    { "uniq", 1 },      // uniq(list) - returns only uniq values in the list, stable and preserves order

    // introduced to support string vectors
    { "isNumber", 1 }, // is the passed thing a number or list of numbers ?
    { "isString", 1 }, // is the passed thing a string or list of strings ?
    { "metadata", 1 }, // get metadata value for current ride, or a vector for all rides.
    { "tolower", 1 },  // convert strings to lower case
    { "toupper", 1 },  // convert strings to upper case
    { "join", 2 },     // join(list, sep) - join a vector of strings into a single string separated by sep
    { "split", 2 },    // split(string, sep) - split a string into a vector of strings
    { "trim", 1 },     // trim whitespace from front/back of strings or vectors of strings.
    { "replace", 3 },  // replace(list|string, s1, s2) -replace s1 with s2 in list or string

    { "filename", 0 }, // filename() - returns a string or vector of strings for a range, can be used
                       // when plotting on trends chart to enable click thru to activity view

    { "xdata", 2 },    // xdata("series", "column" | km | secs) - get xdata samples without any
                       // kind of interpolation applied (resample/interpolate are available to
                       // do that if needed.

    { "store", 2 },    // store("name", value) stores a global value that can be retrieved via
                       // the fetch("name") function below. Useful for passing data across data series
                       // in a user chart.
    { "fetch", 1 },    // fetch("name") retrieves a value previously stored returns 0 if the value
                       // is not in the athlete store

    { "metricname", 0 },     // metricname(Average_Power, BikeStress) returns the metric name, crucially in the local
                       // language. Will return a vector for multiple values

    { "metricunit", 0 },     // unit(Average_Power, BikeStress) returns the metric unit name, crucially in the local
                       // language. Will return a vector for multiple values

    { "datestring", 1 }, // datestring(a) will return a vector of strings converting the passed parameter
                        // from days since to a string

    { "timestring", 1 }, // timestring(a) will return a vector of strings converting the passed parameter
                        // from secs since midnight to a time string

    { "asstring", 0 }, // asstring(Average_Power, BikeStress) returns the metric value as a string
                       // so honouring decimal places and conversion to metric/imperial etc.

    { "metricstrings", 0 }, // metricstrings(Metrics [,start [,stop]]) - same as metrics above but instead of
                            // returning a vector of numbers, the values are converted to strings as
                            // appropriate for the metric (e.g. rowing pace xxx/500m).

    { "zones", 2 },   // zones(power|hr|pace|fatigue, name|description|low|high|unit|time|percent) - returns a vector
                      // of the zone details and the time in zone and percentage metric.

    { "string", 1 }, // string(a) will convert the passed entries to a string if they are numeric.

    { "activities", 2 }, // activities("expression", expr) - provides a closure for expr using the filter in expression
                         // for example filter("Workout_Code = \"FTP\"", metrics(BikeStress)) will return a vector of
                         // BikeStress values for activities that have the metadata "Workout_Code"

    { "intervals", 0 }, // intervals(symbol|name|start|stop|type|test|color|route|selected|date|filename [,start [,stop]])
                        // - returns a vector of values for the metric or field specified for each interval
                        // if no start/stop is supplied it uses the currently selected date range or activity.

    { "intervalstrings", 0 }, // intervalstring(symbol|name|start|stop|type|test|color|route|selected|date|filename [,start [,stop]])
                              //  - same as intervals above but instead of returning a vector of numbers, the values
                              //  are converted to strings as appropriate for the metric (e.g. Pace_Rowing mm:ss/500m).

    { "powerindex", 2 }, // powerindex(power, secs) - returns an array or value representing the power and duration
                         //                           represented as a power index

    { "aggmetrics", 0 },        // aggregate metrics before returning a single value, see metrics above
    { "aggmetricstrings", 0 },  // aggregate metrics and return as a string value, see metricstringsabove
    { "asaggstring", 0 },       // asaggstring(metric1, metricn) - aggregates for metrics listed
    { "normalize", 3 },      // normalize(vector, min, max) - unity based normalize to values between 0 and 1 for the vector or value
                             // based upon the min and max values. anything below min will be mapped to 0 and anything
                             // above max will be mapped to 1
    { "pdfnormal", 2 },           // pdfnormal(sigma, x) returns the probability density function for value x
    { "cdfnormal", 2 },           // cdfnormal(sigma, x) returns the cumulative density function for value x
    { "pdfbeta", 3 },           // pdfbeta(a,b, x) as above for the beta distribution
    { "cdfbeta", 3 },           // cdfbeta(a,b, x) as above for the beta distribution
    { "pdfgamma", 3 },           // pdfgamma(a,b, x) as above for the gamma distribution
    { "cdfgamma", 3 },           // cdfgamma(a,b, x) as above for the gamma distribution

    { "kmeans", 0 },        // kmeans(centers|assignments, k, dim1, dim2, dim3 .. dimn) - return the centers or cluster assignment
                            // from a k means cluser of the data with n dimensions (but commonly just 2- x and y)


    // add new ones above this line
    { "", -1 }
};

static QStringList pdmodels(Context *context)
{
    QStringList returning;

    returning << CP2Model(context).code();
    returning << CP3Model(context).code();
    returning << MultiModel(context).code();
    returning << ExtendedModel(context).code();
    returning << WSModel(context).code();
    return returning;
}

// whenever we use a line style
static struct {
    const char *name;
    Qt::PenStyle type;
} linestyles_[] = {
    { "solid", Qt::SolidLine },
    { "dash", Qt::DashLine },
    { "dot", Qt::DotLine },
    { "dashdot", Qt::DashDotLine },
    { "dashdotdot", Qt::DashDotDotLine },
    { "", Qt::NoPen },
};

static Qt::PenStyle linestyle(QString name)
{
    int index=0;
    while (linestyles_[index].type != Qt::NoPen) {
        if (name == linestyles_[index].name)
            return linestyles_[index].type;
        index++;
    }
    return Qt::NoPen; // not known
}

QStringList
DataFilter::builtins(Context *context)
{
    QStringList returning;

    // add special functions
    returning <<"isRide"<<"isSwim"<<"isXtrain"; // isRun is included in RideNavigator

    for(int i=0; DataFilterFunctions[i].parameters != -1; i++) {

        if (i == 30 || i == 95) { // special case 'estimate' and 'estimates' we describe it

            if (i==30) { foreach(QString model, pdmodels(context)) returning << "estimate(" + model + ", cp|ftp|w'|pmax|x)"; }
            if (i==95) { foreach(QString model, pdmodels(context)) returning << "estimates(" + model + ", cp|ftp|w'|pmax|x|date)"; }

        } else if (i == 31) { // which example
            returning << "which(x>0, ...)";

        } else if (i == 32) { // set example
            returning <<"set(field, value, expr)";
        } else if (i == 37) {
            returning << "XDATA(\"xdata\", \"series\", sparse|repeat|interpolate|resample)";
        } else if (i == 41) {
            returning << "XDATA_UNITS(\"xdata\", \"series\")";
        } else if (i == 42) {
            Measures measures = Measures();
            QStringList groupSymbols = measures.getGroupSymbols();
            for (int g=0; g<groupSymbols.count(); g++)
                foreach (QString fieldSymbol, measures.getFieldSymbols(g))
                    returning << QString("measure(Date, \"%1\", \"%2\")").arg(groupSymbols[g]).arg(fieldSymbol);

        } else if (i == 43) {
            // tests
            returning << "tests(user|bests, duration|power)";

        } else if (i == 44) {
            // banister
            returning << "banister(load_metric, perf_metric, nte|pte|perf|cp|date)";

        } else if (i == 45) {

            // concat
            returning << "c(...)";

        } else if (i == 46) {

            // seq
            returning << "seq(start,stop,step)";

        } else if (i == 47) {

            // seq
            returning << "rep(value,n)";

        } else if (i == 48) {

            // length
            returning << "length(vector)";

        } else if (i == 49) {
            // append
            returning << "append(a,b,[pos])"; // position is optional

        } else if (i == 50) {
            // remove
            returning << "remove(a,pos,count)";

        } else if (i == 51) {
            // mid
            returning << "mid(a,pos,count)"; // subset

        } else if (i == 52) {

            // get a vector of data series samples
            returning << "samples(POWER|HR etc)";

        } else if (i == 53) {

            // get a vector of activity metrics - date is integer days since 1900
            returning << "metrics(symbol|date [,start [,stop]])";

        } else if (i == 54) {

            // argsort
            returning << "argsort(ascend|descend, list)";

        } else if (i == 55) {

            // multisort
            returning << "multisort(ascend|descend, list [, list2 .. ,listn])";

        } else if (i == 56) {

            // head
            returning << "head(list, n)";

        } else if (i == 57) {

            // tail
            returning << "tail(list, n)";

        } else if (i== 58) {
            // meanmax
            returning << "meanmax(POWER|WPK|HR|CADENCE|SPEED [,start, stop]) or meanmax(xvector, yvector)";

        } else if (i == 59) {

            // pmc
            returning << "pmc(metric, stress|lts|sts|sb|rr|date)";

        } else if (i == 60) {

            // sapply
            returning << "sapply(list, expr)";

        } else if (i == 61) {

            returning << "lr(xlist, ylist)";

        } else if (i == 64) {

            returning << "lm(formula, xlist, ylist)";

        } else if (i == 66) {

            returning << "annotate(label|lr|hline|vline|voronoi, ...)";

        } else if (i == 67) {

            returning << "arguniq(list)";

        } else if (i == 68) {

            returning << "multiuniq(list [,list n])";

        } else if (i == 71) {

            returning << "curve(series, x|y|z|d|t)";

        } else if (i == 72) {

            returning << "lowerbound(list, value";

        } else if (i == 73) {

            returning << "cumsum(vector)";

        } else if (i == 74) {

            Measures measures = Measures();
            QStringList groupSymbols = measures.getGroupSymbols();
            for (int g=0; g<groupSymbols.count(); g++) {
                QStringList fields = measures.getFieldSymbols(g);
                fields.insert(0, "date");
                foreach (QString fieldSymbol, fields)
                    returning << QString("measures(\"%1\", \"%2\")").arg(groupSymbols[g]).arg(fieldSymbol);
            }

        } else if (i == 79) {

            returning << "aggregate(vector, by, mean|sum|max|min|count)";

        } else if (i == 80) {

            returning << "exists(\"symbol\")";

        } else if (i == 81) {

            returning << "mlr(yvector, xvector .. xvector)";

        } else if (i == 82) {

            returning << "match(vector1, vector2)";

        } else if (i == 83) {

            returning << "nonzero(vector)";

        } else if (i == 84) {

            returning << "dist(POWER|WPK|HR|CADENCE|SPEED, data|bins)";

            // 85 - median
            // 86 - mode
        } else if (i == 87) { // Gronk!

            returning << "bests(POWER|WPK|HR|CADENCE|SPEED, duration [,start [,stop] ])";

            // 88 - daterange
        } else if (i == 89) {

            returning << "quantile(vector, quantiles)";

            // 90 - bin
            // 91 - rev
            // 92 - random
        } else if (i == 93) {

            returning << "interpolate(linear|cubic|akima|steffen, xvector, yvector, xvalues)";

            // 94 resample
            // 95 estimates (see above) - also Kyle !!!!

        } else if (i == 96) {

            // rank
            returning << "rank(ascend|descend, list)";

        } else if (i == 97) {

            // sort
            returning << "sort(ascend|descend, list)";

        } else if (i == 108) {

            // filename (or vector of names)
            returning << "filename()";

        } else if (i== 118) {
            // zone details
            returning << "zones(hr|power|pace|fatigue, name|description|units|low|high|time|percent)";

        } else if (i  == 127) {

            // heat
           returning << "normalize(min, max, v)";

        } else {

            QString function;
            function = DataFilterFunctions[i].name + "(";
            for(int j=0; j<DataFilterFunctions[i].parameters; j++) {
                if (j) function += ", ";
                function += QString("p%1").arg(j+1);
            }
            if (DataFilterFunctions[i].parameters) function += ")";
            else function += "...)";
            returning << function;
        }
    }
    return returning;
}

// LEXER VARIABLES WE INTERACT WITH
// Standard yacc/lex variables / functions
extern int DataFilterlex(); // the lexer aka yylex()
extern char *DataFiltertext; // set by the lexer aka yytext

extern void DataFilter_setString(QString);
extern void DataFilter_clearString();

// PARSER STATE VARIABLES
QStringList DataFiltererrors;
extern int DataFilterparse();

Leaf *DataFilterroot; // root node for parsed statement

static RideFile::SeriesType nameToSeries(QString name)
{
    if (!name.compare("power", Qt::CaseInsensitive)) return RideFile::watts;
    if (!name.compare("apower", Qt::CaseInsensitive)) return RideFile::aPower;
    if (!name.compare("cadence", Qt::CaseInsensitive)) return RideFile::cad;
    if (!name.compare("hr", Qt::CaseInsensitive)) return RideFile::hr;
    if (!name.compare("speed", Qt::CaseInsensitive)) return RideFile::kph;
    if (!name.compare("torque", Qt::CaseInsensitive)) return RideFile::nm;
    if (!name.compare("IsoPower", Qt::CaseInsensitive)) return RideFile::IsoPower;
    if (!name.compare("xPower", Qt::CaseInsensitive)) return RideFile::xPower;
    if (!name.compare("VAM", Qt::CaseInsensitive)) return RideFile::vam;
    if (!name.compare("wpk", Qt::CaseInsensitive)) return RideFile::wattsKg;
    if (!name.compare("lrbalance", Qt::CaseInsensitive)) return RideFile::lrbalance;

    return RideFile::none;

}

bool
Leaf::isDynamic(Leaf *leaf)
{
    switch(leaf->type) {
    default:
    case Leaf::Symbol :
        return leaf->dynamic;
    case Leaf::UnaryOperation :
        return leaf->isDynamic(leaf->lvalue.l);
    case Leaf::Logical  :
        if (leaf->op == 0)
            return leaf->isDynamic(leaf->lvalue.l);
        // intentional fallthrough
    case Leaf::Operation :
    case Leaf::BinaryOperation :
        return (leaf->isDynamic(leaf->lvalue.l) ||
                leaf->isDynamic(leaf->rvalue.l));
    case Leaf::Function :
        if (leaf->series && leaf->lvalue.l)
            return leaf->isDynamic(leaf->lvalue.l);
        else
            return leaf->dynamic;
    case Leaf::Conditional :
        return (leaf->isDynamic(leaf->cond.l) ||
                leaf->isDynamic(leaf->lvalue.l) ||
                (leaf->rvalue.l && leaf->isDynamic(leaf->rvalue.l)));
    case Leaf::Script :
        return true;

    }
    return false;
}

void
DataFilter::setSignature(QString &query)
{
    sig = fingerprint(query);
}

QString
DataFilter::fingerprint(QString &query)
{
    QString sig;

    bool incomment=false;
    bool instring=false;

    for (int i=0; i<query.length(); i++) {

        // get out of comments and strings at end of a line
        if (query[i] == '\n') instring = incomment=false;

        // get into comments
        if (!instring && query[i] == '#') incomment=true;

        // not in comment and not escaped
        if (query[i] == '"' && !incomment && (((i && query[i-1] != '\\') || i==0))) instring = !instring;

        // keep anything that isn't whitespace, or a comment
        if (instring || (!incomment && !query[i].isSpace())) sig += query[i];
    }

    return sig;
}

void
DataFilter::colorSyntax(QTextDocument *document, int pos)
{
    // matched bracket position
    int bpos = -1;

    // matched brace position
    int cpos = -1;

    // for looking for comments
    QString string = document->toPlainText();

    // clear formatting and color comments
    QTextCharFormat normal;
    normal.setFontWeight(QFont::Normal);
    normal.setUnderlineStyle(QTextCharFormat::NoUnderline);
    normal.setForeground(Qt::black);

    QTextCharFormat cyanbg;
    cyanbg.setBackground(Qt::cyan);
    QTextCharFormat redbg;
    redbg.setBackground(QColor(255,153,153));

    QTextCharFormat function;
    function.setUnderlineStyle(QTextCharFormat::NoUnderline);
    function.setForeground(Qt::blue);

    QTextCharFormat symbol;
    symbol.setUnderlineStyle(QTextCharFormat::NoUnderline);
    symbol.setForeground(Qt::red);

    QTextCharFormat literal;
    literal.setFontWeight(QFont::Normal);
    literal.setUnderlineStyle(QTextCharFormat::NoUnderline);
    literal.setForeground(Qt::magenta);

    QTextCharFormat comment;
    comment.setFontWeight(QFont::Normal);
    comment.setUnderlineStyle(QTextCharFormat::NoUnderline);
    comment.setForeground(Qt::darkGreen);

    QTextCursor cursor(document);

    // set all black
    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    cursor.selectionStart();
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.selectionEnd();
    cursor.setCharFormat(normal);

    // color comments
    bool instring=false;
    bool innumber=false;
    bool incomment=false;
    bool insymbol=false;
    int commentstart=0;
    int stringstart=0;
    int numberstart=0;
    int symbolstart=0;
    int brace=0;
    int brack=0;
    int sbrack=0;

    for(int i=0; i<string.length(); i++) {

        // enter into symbol
        if (!insymbol && !incomment && !instring && string[i].isLetter()) {
            insymbol = true;
            symbolstart = i;

            // it starts with numbers but ends with letters - number becomes symbol
            if (innumber) { symbolstart=numberstart; innumber=false; }
        }

        // end of symbol ?
        if (insymbol && (!string[i].isLetterOrNumber() && string[i] != '_')) {

            bool isfunction = false;
            insymbol = false;
            QString sym = string.mid(symbolstart, i-symbolstart);

            bool found=false;
            QString lookup = rt.lookupMap.value(sym, "");
            if (lookup == "") {

                // isRun isa special, we may add more later (e.g. date)
                if (!sym.compare("Date", Qt::CaseInsensitive) ||
                    !sym.compare("Time", Qt::CaseInsensitive) ||
                    !sym.compare("banister", Qt::CaseInsensitive) ||
                    !sym.compare("best", Qt::CaseInsensitive) ||
                    !sym.compare("tiz", Qt::CaseInsensitive) ||
                    !sym.compare("const", Qt::CaseInsensitive) ||
                    !sym.compare("config", Qt::CaseInsensitive) ||
                    !sym.compare("ctl", Qt::CaseInsensitive) ||
                    !sym.compare("tsb", Qt::CaseInsensitive) ||
                    !sym.compare("atl", Qt::CaseInsensitive) ||
                    !sym.compare("Today", Qt::CaseInsensitive) ||
                    !sym.compare("Current", Qt::CaseInsensitive) ||
                    !sym.compare("RECINTSECS", Qt::CaseInsensitive) ||
                    !sym.compare("NA", Qt::CaseInsensitive) ||
                    sym == "isRide" || sym == "isSwim" ||
                    sym == "isRun" || sym == "isXtrain") {
                    isfunction = found = true;
                }

                // ride sample symbol
                if (rt.dataSeriesSymbols.contains(sym)) found = true;

                // still not found ?
                // is it a function then ?
                for(int j=0; DataFilterFunctions[j].parameters != -1; j++) {
                    if (DataFilterFunctions[j].name == sym) {
                        isfunction = found = true;
                        break;
                    }
                }
            } else {
                found = true;
            }

            if (found) {

                // lets color it red, its a literal.
                cursor.setPosition(symbolstart, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.setCharFormat(isfunction ? function : symbol);
            }
        }

        // numeric literal
        if (!insymbol && string[i].isNumber()) {

            // start of number ?
            if (!incomment && !instring && !innumber) {

                innumber = true;
                numberstart = i;

            }

        } else if (!insymbol && !incomment && !instring &&innumber) {

            // now out of number

            innumber = false;

            // lets color it red, its a literal.
            cursor.setPosition(numberstart, QTextCursor::MoveAnchor);
            cursor.selectionStart();
            cursor.setPosition(i, QTextCursor::KeepAnchor);
            cursor.selectionEnd();
            cursor.setCharFormat(literal);
        }

        // watch for being in a string, but remember escape!
        if ((i==0 || string[i-1] != '\\') && string[i] == '"') {

            if (!incomment && instring) {

                instring = false;

                // lets color it red, its a literal.
                cursor.setPosition(stringstart, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.setCharFormat(literal);

            } else if (!incomment && !instring) {

                stringstart=i;
                instring=true;
            }
        }

        // watch for being in a comment
        if (string[i] == '#' && !instring && !incomment) {
            incomment = true;
            commentstart=i;
        }

        // end of text or line
        if (i+1 == string.length() || (string[i] == '\r' || string[i] == '\n')) {

            // mark if we have a comment
            if (incomment) {

                // we have a line of comments to here from commentstart
                incomment = false;

                // comments are blue
                cursor.setPosition(commentstart, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.setCharFormat(comment);

            } else {

                int start=0;
                if (instring) start=stringstart;
                if (innumber) start=numberstart;

                // end of string ...
                if (instring || innumber) {

                    innumber = instring = false; // always quit on end of line

                    // lets color it red, its a literal.
                    cursor.setPosition(start, QTextCursor::MoveAnchor);
                    cursor.selectionStart();
                    cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                    cursor.selectionEnd();
                    cursor.setCharFormat(literal);
                }
            }
        }

        // are the brackets balanced  ( ) ?
        if (!instring && !incomment && string[i]=='(') {
            brack++;

            // match close/open if over cursor
            if (i==pos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run forward looking for match
                int bb=0;
                for(int j=i; j<string.length(); j++) {
                    if (string[j]=='(') bb++;
                    if (string[j]==')') {
                        bb--;
                        if (bb == 0) {
                            bpos = j; // matched brack here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }
            }
        }
        if (!instring && !incomment && string[i]==')') {
            brack--;

            if (i==pos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run backward looking for match
                int bb=0;
                for(int j=i; j>=0; j--) {
                    if (string[j]==')') bb++;
                    if (string[j]=='(') {
                        bb--;
                        if (bb == 0) {
                            bpos = j; // matched brack here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }

            } else if (brack < 0 && i != bpos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }

        // are the brackets balanced  [ ] ?
        if (!instring && !incomment && string[i]=='[') {
            sbrack++;

            // match close/open if over cursor
            if (i==pos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run forward looking for match
                int bb=0;
                for(int j=i; j<string.length(); j++) {
                    if (string[j]=='[') bb++;
                    if (string[j]==']') {
                        bb--;
                        if (bb == 0) {
                            bpos = j; // matched brack here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }
            }
        }
        if (!instring && !incomment && string[i]==']') {
            sbrack--;

            if (i==pos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run backward looking for match
                int bb=0;
                for(int j=i; j>=0; j--) {
                    if (string[j]==']') bb++;
                    if (string[j]=='[') {
                        bb--;
                        if (bb == 0) {
                            bpos = j; // matched brack here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }

            } else if (sbrack < 0 && i != bpos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }

        // are the braces balanced  ( ) ?
        if (!instring && !incomment && string[i]=='{') {
            brace++;

            // match close/open if over cursor
            if (i==pos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run forward looking for match
                int bb=0;
                for(int j=i; j<string.length(); j++) {
                    if (string[j]=='{') bb++;
                    if (string[j]=='}') {
                        bb--;
                        if (bb == 0) {
                            cpos = j; // matched brace here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }
            }
        }
        if (!instring && !incomment && string[i]=='}') {
            brace--;

            if (i==pos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(cyanbg);

                // run backward looking for match
                int bb=0;
                for(int j=i; j>=0; j--) {
                    if (string[j]=='}') bb++;
                    if (string[j]=='{') {
                        bb--;
                        if (bb == 0) {
                            cpos = j; // matched brace here, don't change color!

                            cursor.setPosition(j, QTextCursor::MoveAnchor);
                            cursor.selectionStart();
                            cursor.setPosition(j+1, QTextCursor::KeepAnchor);
                            cursor.selectionEnd();
                            cursor.mergeCharFormat(cyanbg);
                            break;
                        }
                    }
                }

            } else if (brace < 0 && i != cpos-1) {

                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }
    }

    // unbalanced braces - do same as above, backwards?
    //XXX braces in comments fuck things up ... XXX
    if (brace > 0) {
        brace = 0;
        for(int i=string.length(); i>=0; i--) {

            if (string[i] == '}') brace++;
            if (string[i] == '{') brace--;

            if (brace < 0 && string[i] == '{' && i != pos-1 && i != cpos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }
    }

    if (brack > 0) {
        brack = 0;
        for(int i=string.length(); i>=0; i--) {

            if (string[i] == ')') brack++;
            if (string[i] == '(') brack--;

            if (brack < 0 && string[i] == '(' && i != pos-1 && i != bpos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }
    }

    if (sbrack > 0) {
        sbrack = 0;
        for(int i=string.length(); i>=0; i--) {

            if (string[i] == ']') sbrack++;
            if (string[i] == '[') sbrack--;

            if (sbrack < 0 && string[i] == '[' && i != pos-1 && i != bpos-1) {
                cursor.setPosition(i, QTextCursor::MoveAnchor);
                cursor.selectionStart();
                cursor.setPosition(i+1, QTextCursor::KeepAnchor);
                cursor.selectionEnd();
                cursor.mergeCharFormat(redbg);
            }
        }
    }

    // apply selective coloring to the symbols and expressions
    if(treeRoot) treeRoot->color(treeRoot, document);
}

void Leaf::color(Leaf *leaf, QTextDocument *document)
{
    // nope
    if (leaf == NULL) return;

    QTextCharFormat apply;

    switch(leaf->type) {
    case Leaf::Float :
    case Leaf::Integer :
    case Leaf::String :
                    break;

    case Leaf::Symbol :
                    break;

    case Leaf::Logical  :
                    leaf->color(leaf->lvalue.l, document);
                    if (leaf->op) leaf->color(leaf->rvalue.l, document);
                    return;
                    break;

    case Leaf::Operation :
                    leaf->color(leaf->lvalue.l, document);
                    leaf->color(leaf->rvalue.l, document);
                    return;
                    break;

    case Leaf::UnaryOperation :
                    leaf->color(leaf->lvalue.l, document);
                    return;
                    break;
    case Leaf::BinaryOperation :
                    leaf->color(leaf->lvalue.l, document);
                    leaf->color(leaf->rvalue.l, document);
                    return;
                    break;

    case Leaf::Function :
                    if (leaf->series) {
                        if (leaf->lvalue.l) leaf->color(leaf->lvalue.l, document);
                        return;
                    } else {
                        foreach(Leaf*l, leaf->fparms) leaf->color(l, document);
                    }
                    break;

    case Leaf::Conditional :
        {
                    leaf->color(leaf->cond.l, document);
                    leaf->color(leaf->lvalue.l, document);
                    if (leaf->rvalue.l) leaf->color(leaf->rvalue.l, document);
                    return;
        }
        break;

    case Leaf::Compound :
        {
            foreach(Leaf *statement, *(leaf->lvalue.b)) leaf->color(statement, document);
            // don't return in case the whole thing is a mess...
        }
        break;

    default:
        return;
        break;

    }

    // all we do now is highlight if in error.
    if (leaf->inerror == true) {

        QTextCursor cursor(document);

        // highlight this token and apply
        cursor.setPosition(leaf->loc, QTextCursor::MoveAnchor);
        cursor.selectionStart();
        cursor.setPosition(leaf->leng+1, QTextCursor::KeepAnchor);
        cursor.selectionEnd();

        apply.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        apply.setUnderlineColor(Qt::red);

        cursor.mergeCharFormat(apply);
    }
}

// convert expression to string, without white space
// this can be used as a signature when caching values etc

QString
Leaf::toString()
{
    switch(type) {
    case Leaf::Script : return *lvalue.s;
    case Leaf::Float : return QString("%1").arg(lvalue.f); break;
    case Leaf::Integer : return QString("%1").arg(lvalue.i); break;
    case Leaf::String : return *lvalue.s; break;
    case Leaf::Symbol : return *lvalue.n; break;
    case Leaf::Logical  :
    case Leaf::Operation :
    case Leaf::BinaryOperation :
                    return QString("%1%2%3")
                    .arg(lvalue.l->toString())
                    .arg(op)
                    .arg(op ? rvalue.l->toString() : "");
                    break;
    case Leaf::UnaryOperation :
                    return QString("-%1").arg(lvalue.l->toString());
                    break;
    case Leaf::Function :
                    if (series) {

                        if (lvalue.l) {
                            return QString("%1(%2,%3)")
                                       .arg(function)
                                       .arg(*(series->lvalue.n))
                                       .arg(lvalue.l->toString());
                        } else {
                            return QString("%1(%2)")
                                       .arg(function)
                                       .arg(*(series->lvalue.n));
                        }

                    } else {
                        QString f= function + "(";
                        bool first=true;
                        foreach(Leaf*l, fparms) {
                            f+= (first ? "," : "") + l->toString();
                            first = false;
                        }
                        f += ")";
                        return f;
                    }
                    break;

    case Leaf::Conditional : //qDebug()<<"cond:"<<op;
        {
                    if (rvalue.l) {
                        return QString("%1?%2:%3")
                        .arg(cond.l->toString())
                        .arg(lvalue.l->toString())
                        .arg(rvalue.l->toString());
                    } else {
                        return QString("%1?%2")
                        .arg(cond.l->toString())
                        .arg(lvalue.l->toString());
                    }
        }
        break;

    default:
        break;

    }
    return "";
}

void
Leaf::findSymbols(QStringList &symbols)
{
    switch(type) {
    case Leaf::Script :
        break;
    case Leaf::Compound :
        foreach(Leaf *p, *(lvalue.b)) p->findSymbols(symbols);
        break;

    case Leaf::Float :
    case Leaf::Integer :
    case Leaf::String :
        break;
    case Leaf::Symbol :
        {
            symbols << (*lvalue.n);
        }
        break;
    case Leaf::Operation:
    case Leaf::BinaryOperation:
    case Leaf::Logical :
        lvalue.l->findSymbols(symbols);
        if (op) rvalue.l->findSymbols(symbols);
        break;
        break;
    case Leaf::UnaryOperation:
        lvalue.l->findSymbols(symbols);
        break;
    case Leaf::Function:
        foreach(Leaf* l, fparms) l->findSymbols(symbols);
        break;
    case Leaf::Index:
    case Leaf::Select:
        lvalue.l->findSymbols(symbols);
        fparms[0]->findSymbols(symbols);
        break;
    case Leaf::Conditional:
        cond.l->findSymbols(symbols);
        lvalue.l->findSymbols(symbols);
        if (rvalue.l) rvalue.l->findSymbols(symbols);
        break;

    default:
        break;
    }
}

void Leaf::print(int level, DataFilterRuntime *df)
{
    qDebug()<<"LEVEL"<<level;
    switch(type) {
    case Leaf::Script :
        qDebug()<<lvalue.s;
        break;
    case Leaf::Compound :
        qDebug()<<"{";
        foreach(Leaf *p, *(lvalue.b))
            p->print(level+1, df);
        qDebug()<<"}";
        break;

    case Leaf::Float :
        qDebug()<<"float"<<lvalue.f<<dynamic;
        break;
    case Leaf::Integer :
        qDebug()<<"integer"<<lvalue.i<<dynamic;
        break;
    case Leaf::String :
        qDebug()<<"string"<<*lvalue.s<<dynamic;
        break;
    case Leaf::Symbol :
        {
            Result value = df->symbols.value(*lvalue.n), Result(11);
            if (value.isNumber) {
                qDebug()<<"symbol"<<*lvalue.n<<dynamic<<value.number();

                // output vectors, truncate to 20 els
                if (value.asNumeric().count()) {
                    QString output = QString("Vector[%1]:").arg(value.asNumeric().count());
                    for(int i=0; i<value.asNumeric().count() && i<20; i++)
                        output += QString("%1, ").arg(value.asNumeric()[i]);
                    qDebug() <<output;
                }
            } else {
                qDebug()<<"symbol"<<*lvalue.n<<dynamic<<value.string();

                // output vectors, truncate to 20 els
                if (value.asString().count()) {
                    QString output = QString("Strings[%1]:").arg(value.asString().count());
                    for(int i=0; i<value.asString().count() && i<20; i++)
                        output += QString("%1, ").arg(value.asString()[i]);
                    qDebug() <<output;
                }

            }
        }
        break;
    case Leaf::Logical :
        qDebug()<<"lop"<<op;
        lvalue.l->print(level+1, df);
        if (op) // nonzero ?
            rvalue.l->print(level+1, df);
        break;
    case Leaf::Operation:
        qDebug()<<"cop"<<op;
        lvalue.l->print(level+1, df);
        rvalue.l->print(level+1, df);
        break;
    case Leaf::UnaryOperation:
        qDebug()<<"uop"<<op;
        lvalue.l->print(level+1, df);
        break;
    case Leaf::BinaryOperation:
        qDebug()<<"bop"<<op;
        lvalue.l->print(level+1, df);
        rvalue.l->print(level+1, df);
        break;
    case Leaf::Function:
        if (series) {
            qDebug()<<"function"<<function<<"parm="<<*(series->lvalue.n);
            if (lvalue.l)
                lvalue.l->print(level+1, df);
        } else {
            qDebug()<<"function"<<function<<"parms:"<<fparms.count();
            foreach(Leaf* l, fparms)
                l->print(level+1, df);
        }
        break;
    case Leaf::Select:
        qDebug()<<"select";
        lvalue.l->print(level+1, df);
        fparms[0]->print(level+1,df);
        break;
    case Leaf::Index:
        qDebug()<<"index";
        lvalue.l->print(level+1, df);
        fparms[0]->print(level+1,df);
        break;
    case Leaf::Conditional:
        qDebug()<<"cond"<<op;
        cond.l->print(level+1, df);
        lvalue.l->print(level+1, df);
        if (rvalue.l)
            rvalue.l->print(level+1, df);
        break;

    default:
        break;

    }
}

static bool isCoggan(QString symbol)
{
    if (!symbol.compare("ctl", Qt::CaseInsensitive)) return true;
    if (!symbol.compare("tsb", Qt::CaseInsensitive)) return true;
    if (!symbol.compare("atl", Qt::CaseInsensitive)) return true;
    return false;
}

bool Leaf::isNumber(DataFilterRuntime *df, Leaf *leaf)
{

    switch(leaf->type) {
    case Leaf::Script : return true;
    case Leaf::Compound : return true; // last statement is value of block
    case Leaf::Float : return true;
    case Leaf::Integer : return true;
    case Leaf::String :
        {
            // strings that evaluate as a date
            // will be returned as a number of days
            // since 1900!
            QString string = *(leaf->lvalue.s);
            if (QDate::fromString(string, "yyyy/MM/dd").isValid())
                return true;
            else
                return false;
        }
    case Leaf::Symbol :
        {
            QString symbol = *(leaf->lvalue.n);
            if (df->symbols.contains(symbol)) return true;
            if (symbol == "isRide" || symbol == "isSwim" ||
                symbol == "isRun" || symbol == "isXtrain") return true;
            if (symbol == "x" || symbol == "i") return true;
            else if (!symbol.compare("Date", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("Time", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("Today", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("Current", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("RECINTSECS", Qt::CaseInsensitive)) return true;
            else if (!symbol.compare("NA", Qt::CaseInsensitive)) return true;
            else if (isCoggan(symbol)) return true;
            else if (df->dataSeriesSymbols.contains(symbol)) return true;
            else return df->lookupType.value(symbol, false);
        }
        break;
    case Leaf::Logical  : return true; // not possible!
    case Leaf::Operation : return true;
    case Leaf::UnaryOperation : return true;
    case Leaf::BinaryOperation : return true;
    case Leaf::Function : return true;
    case Leaf::Select :
    case Leaf::Index :
    case Leaf::Conditional :
        {
            return true;
        }
        break;

    default:
        return false;
        break;

    }
}

void Leaf::clear(Leaf *leaf)
{
Q_UNUSED(leaf);
#if 0 // memory leak!!!
    switch(leaf->type) {
    case Leaf::String : delete leaf->lvalue.s; break;
    case Leaf::Symbol : delete leaf->lvalue.n; break;
    case Leaf::Logical  :
    case Leaf::BinaryOperation :
    case Leaf::Operation : clear(leaf->lvalue.l);
                           clear(leaf->rvalue.l);
                           delete(leaf->lvalue.l);
                           delete(leaf->rvalue.l);
                           break;
    case Leaf::Function :  clear(leaf->lvalue.l);
                           delete(leaf->lvalue.l);
                            break;
    default:
        break;
    }
#endif
}

void Leaf::validateFilter(Context *context, DataFilterRuntime *df, Leaf *leaf)
{
    leaf->inerror = false;

    switch(leaf->type) {

    case Leaf::Script : // just assume script is well formed...
        return;

    case Leaf::Symbol :
        {
            // are the symbols correct?
            // if so set the type to meta or metric
            // and save the technical name used to do
            // a lookup at execution time
            QString symbol = *(leaf->lvalue.n);
            QString lookup = df->lookupMap.value(symbol, "");
            if (lookup == "") {

                // isRun isa special, we may add more later (e.g. date)
                if (symbol.compare("Date", Qt::CaseInsensitive) &&
                    symbol.compare("Time", Qt::CaseInsensitive) &&
                    symbol.compare("x", Qt::CaseInsensitive) && // used by which and [lexpr]
                    symbol.compare("i", Qt::CaseInsensitive) && // used by which and [lexpr]
                    symbol.compare("Today", Qt::CaseInsensitive) &&
                    symbol.compare("Current", Qt::CaseInsensitive) &&
                    symbol.compare("RECINTSECS", Qt::CaseInsensitive) &&
                    symbol.compare("NA", Qt::CaseInsensitive) &&
                    !df->dataSeriesSymbols.contains(symbol) &&
                    symbol != "isRide" && symbol != "isSwim" &&
                    symbol != "isRun" && symbol != "isXtrain" &&
                    !isCoggan(symbol)) {

                    // unknown, is it user defined ?
                    if (!df->symbols.contains(symbol)) {
                        DataFiltererrors << QString(tr("%1 is an unknown symbol")).arg(symbol);
                        leaf->inerror = true;
                    }
                }

                if (symbol.compare("Current", Qt::CaseInsensitive))
                    leaf->dynamic = true;
            }
        }
        break;

    case Leaf::Select:
        {
            // anything goes really.
            leaf->validateFilter(context, df, leaf->fparms[0]);
            leaf->validateFilter(context, df, leaf->lvalue.l);
        }
        return;

    case Leaf::Index :
        {
            //if (leaf->lvalue.l->type != Leaf::Symbol) {
            leaf->validateFilter(context, df, leaf->fparms[0]);
            if (!Leaf::isNumber(df, leaf->fparms[0])) {
                leaf->fparms[0]->inerror = true;
                DataFiltererrors << QString(tr("Index must be numeric."));
            }
            leaf->validateFilter(context, df, leaf->lvalue.l);
        }
        return;

    case Leaf::Function :
        {
            // is the symbol valid?
            QRegExp bestValidSymbols("^(apower|power|hr|cadence|speed|torque|vam|xpower|isopower|wpk)$", Qt::CaseInsensitive);
            QRegExp tizValidSymbols("^(power|hr)$", Qt::CaseInsensitive);
            QRegExp configValidSymbols("^(cranklength|cp|aetp|ftp|w\\'|pmax|cv|aetv|height|weight|lthr|aethr|maxhr|rhr|units|dob|sex)$", Qt::CaseInsensitive);
            QRegExp constValidSymbols("^(e|pi)$", Qt::CaseInsensitive); // just do basics for now
            QRegExp dateRangeValidSymbols("^(start|stop)$", Qt::CaseInsensitive); // date range
            QRegExp pmcValidSymbols("^(stress|lts|sts|sb|rr|date)$", Qt::CaseInsensitive);
            QRegExp smoothAlgos("^(sma|ewma)$", Qt::CaseInsensitive);
            QRegExp annotateTypes("^(label|lr|hline|vline|voronoi)$", Qt::CaseInsensitive);
            QRegExp curveData("^(x|y|z|d|t)$", Qt::CaseInsensitive);
            QRegExp aggregateFunc("^(mean|sum|max|min|count)$", Qt::CaseInsensitive);
            QRegExp interpolateAlgorithms("^(linear|cubic|akima|steffen)$", Qt::CaseInsensitive);

            if (leaf->series) { // old way of hand crafting each function in the lexer including support for literal parameter e.g. (power, 1)

                QString symbol = leaf->series->lvalue.n->toLower();

                if (leaf->function == "best" && !bestValidSymbols.exactMatch(symbol)) {
                    DataFiltererrors << QString(tr("invalid data series for best(): %1")).arg(symbol);
                    leaf->inerror = true;
                }

                if (leaf->function == "tiz" && !tizValidSymbols.exactMatch(symbol)) {
                    DataFiltererrors << QString(tr("invalid data series for tiz(): %1")).arg(symbol);
                    leaf->inerror = true;
                }

                if (leaf->function == "config" && !configValidSymbols.exactMatch(symbol)) {
                    DataFiltererrors << QString(tr("invalid literal for config(): %1")).arg(symbol);
                    leaf->inerror = true;
                }

                if (leaf->function == "const") {
                    if (!constValidSymbols.exactMatch(symbol)) {
                        DataFiltererrors << QString(tr("invalid literal for const(): %1")).arg(symbol);
                        leaf->inerror = true;
                    } else {

                        // convert to a float
                        leaf->type = Leaf::Float;
                        leaf->lvalue.f = 0.0L;
                        if (symbol == "e") leaf->lvalue.f = (float)MATHCONST_E;
                        if (symbol == "pi") leaf->lvalue.f = (float)MATHCONST_PI;
                    }
                }

                if (leaf->function == "best" || leaf->function == "tiz") {
                    // now set the series type used as parameter 1 to best/tiz
                    leaf->seriesType = nameToSeries(symbol);
                }

            } else { // generic functions, math etc

                bool found=false;

                if (leaf->function == "activities") {

                    if (leaf->fparms.count() != 2 || leaf->fparms[0]->type != Leaf::String) {
                        inerror=true;
                        DataFiltererrors << tr("activities(\"fexpr\", expr) - where fexpr is a filter expression");
                    }

                    // validate the expression
                    if (leaf->fparms.count() == 2) validateFilter(context,df,leaf->fparms[1]);

                } else if (leaf->function == "daterange") {

                    if (leaf->fparms.count()==1) {

                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("daterange(start|stop)"));
                        } else {
                            QString symbol = *(leaf->fparms[0]->lvalue.n);
                            if (!dateRangeValidSymbols.exactMatch(symbol)) {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("daterange(start|stop) - unknown symbol '%1'")).arg(symbol);
                            }
                        }

                    } else if (leaf->fparms.count() == 3) {

                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                        validateFilter(context, df, leaf->fparms[2]);

                    } else {
                        DataFiltererrors << QString(tr("daterange(start|stop) or daterange(datefrom, dateto, expression)"));
                        leaf->inerror = true;
                    }
                } else if (leaf->function == "datestring" || leaf->function == "timestring") {

                    if (leaf->fparms.count() != 1) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("%1 needs a single parameter")).arg(leaf->function);
                    }

                } else if (leaf->function == "filename") {

                    if (leaf->fparms.count() != 0) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("filename() has no parameters"));
                    }

                } else if (leaf->function == "zones") {

                    // need exactly 2 symbols
                    if (leaf->fparms.count() != 2 ||
                        leaf->fparms[0]->type != Leaf::Symbol ||
                        leaf->fparms[1]->type != Leaf::Symbol) {
                        leaf->inerror = true;

                    } else {

                        QRegExp reseries("^(hr|power|pace|fatigue)$", Qt::CaseInsensitive);
                        QRegExp refield("^(name|description|units|low|high|time|percent)$", Qt::CaseInsensitive);

                        // lets check the combinations
                        QString series = *leaf->fparms[0]->lvalue.n;
                        QString field = *leaf->fparms[1]->lvalue.n;

                        if (!reseries.exactMatch(series)) inerror=true;
                        if (!refield.exactMatch(field)) inerror=true;
                    }

                    // same error for any badly formed function call
                    if (leaf->inerror) DataFiltererrors << QString(tr("zones(hr|power|pace|fatigue, name|description|low|high|units|time|percent) needs 2 specific parameters"));

                } else if (leaf->function == "exists") {

                    // needs one parameter and must be a string constant
                    if (leaf->fparms.count() != 1) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("exists(\"symbol\") supports only 1 parameter."));
                    } else if (leaf->fparms[0]->type != Leaf::String) {

                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("exists(\"symbol\") parameter must be a constant string."));
                    }
                } else if (leaf->function == "which") {

                    // 2 or more
                    if (leaf->fparms.count() < 2) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("which function has at least 2 parameters."));
                    }

                    // still normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "c") {

                    // pretty much anything goes, can be empty or a list..
                    // .. but still check parameters
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "rep") {

                    if (leaf->fparms.count() != 2) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be rep(value, n)"));
                    }

                    // still normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "seq") {

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be seq(start, stop, step)"));
                    }

                    // still normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "length") {

                    if (leaf->fparms.count() != 1) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be length(expr)"));
                    }

                    // still normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                } else if (leaf->function == "cumsum") {

                    if (leaf->fparms.count() != 1) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be cumsum(vector)"));
                    }

                } else if (leaf->function == "aggregate") {

                    if (leaf->fparms.count() != 3) {

                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be aggregate(vector, byvector, mean|sum|max|min|count)"));

                    } else {

                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);

                        // just check the 3rd param is a valid symbol
                        if (leaf->fparms[2]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("aggregate(vector, by, func) func must be one of mean|sum|max|min|count."));
                        } else {
                            QString symbol = *(leaf->fparms[2]->lvalue.n);
                            if (!aggregateFunc.exactMatch(symbol)) {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("unknown function '%1', must be one of mean|sum|max|min|count.").arg(symbol));
                            }
                        }
                    }

                } else if (leaf->function == "round") {

                    // can  be either round(expr) or round(expr, dp)
                    // where expr evaluates to numeric and dp is a number
                    if (leaf->fparms.count() != 1 && leaf->fparms.count() != 2) {

                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("round(v) or round(v, dp)"));

                    } else {
                        // validate the parameters
                        for(int i=0; i<leaf->fparms.count(); i++) {
                            validateFilter(context, df, leaf->fparms[i]);
                        }
                    }

                } else if (leaf->function == "interpolate") {

                    if (leaf->fparms.count() != 4) {

                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("interpolate(algorithm, xvector, yvector, xvalues)"));

                    } else if (leaf->fparms[0]->type != Leaf::Symbol) {

                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("interpolate(algorithm, xvector, yvector, xvalues) - must specify and algorithm"));

                    } else {

                        // 4 parameters and first is a symbol, lets check we know it
                        QString symbol = *(leaf->fparms[0]->lvalue.n);
                        if (!interpolateAlgorithms.exactMatch(symbol)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("unknown algorithm '%1', must be one of linear, cubic, akima or steffen.").arg(symbol));
                        } else {

                            validateFilter(context, df, leaf->fparms[1]);
                            validateFilter(context, df, leaf->fparms[2]);
                            validateFilter(context, df, leaf->fparms[3]);

                        }

                    }
                } else if (leaf->function == "append") {

                    if (leaf->fparms.count() != 2 && leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be append(a,b,[pos])"));
                    } else {

                        // still normal parm check !
                        foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                        // check parameter 1 is actually a symbol
                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("append(a,b,[pos]) but 'a' must be a symbol"));
                        } else {
                            QString symbol = *(leaf->fparms[0]->lvalue.n);
                            if (!df->symbols.contains(symbol)) {
                                DataFiltererrors << QString(tr("append(a,b,[pos]) but 'a' must be a user symbol"));
                                leaf->inerror = true;
                            }
                        }
                    }

                } else if (leaf->function == "remove") {

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be remove(a,pos,count)"));
                    } else {
                        // still normal parm check !
                        foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);

                        // check parameter 1 is actually a symbol
                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("remove(a,pos,count) but 'a' must be a symbol"));
                        } else {
                            QString symbol = *(leaf->fparms[0]->lvalue.n);
                            if (!df->symbols.contains(symbol)) {
                                DataFiltererrors << QString(tr("remove(a,pos, count) but 'a' must be a user symbol"));
                                leaf->inerror = true;
                            }

                        }

                    }

                } else if (leaf->function == "mid") {

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be mid(a,pos,count)"));
                    }

                } else if (leaf->function == "xdata") {

                    if (leaf->fparms.count() != 2) {

                        DataFiltererrors << QString(tr("XDATA expects two parameters"));
                        leaf->inerror = true;

                    } else {

                        // will only get here if we have 2 parameters
                        Leaf *first=leaf->fparms[0];
                        Leaf *second=leaf->fparms[1];

                        if (first->type != Leaf::String) {
                            DataFiltererrors << QString(tr("XDATA expects a string for the first parameters"));
                            leaf->inerror = true;
                        }

                        if (second->type == Leaf::Symbol) {

                            QString symbol = *(leaf->fparms[1]->lvalue.n);
                            if (symbol != "km" && symbol != "secs") {
                                DataFiltererrors << QString(tr("xdata expects a string, 'km' or 'secs' for second parameters"));
                                leaf->inerror = true;
                            }

                        } else if (second->type != Leaf::String) {

                            DataFiltererrors << QString(tr("xdata expects a string, 'km' or 'secs' for second parameters"));
                            leaf->inerror = true;
                        }
                    }

                } else if (leaf->function == "samples") {

                    if (leaf->fparms.count() < 1) {
                        leaf->inerror=true;
                        DataFiltererrors << QString(tr("samples(SERIES), SERIES should be POWER, SECS, HEARTRATE etc."));

                    } else if (leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("samples(SERIES), SERIES should be POWER, SECS, HEARTRATE etc."));
                    } else {
                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        if (symbol == "WBAL")  leaf->seriesType=RideFile::wbal; // special case
                        else if (symbol == "WBALSECS")  leaf->seriesType=RideFile::none; // extra special case ;)
                        else {
                            leaf->seriesType = RideFile::seriesForSymbol(symbol);
                            if (leaf->seriesType==RideFile::none) {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("invalid series name '%1'").arg(symbol));
                            }
                        }
                    }

                } else if (leaf->function == "tests") {

                    if (leaf->fparms.count() != 0 && leaf->fparms.count() != 2) {
                        leaf->inerror=true;
                        DataFiltererrors << QString(tr("tests(user|bests, duration|power)"));
                    } else if (leaf->fparms.count() == 2) {

                        // user or best?
                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                            leaf->inerror=true;
                            DataFiltererrors << QString(tr("tests() first parameter must be 'user' or 'bests'."));
                        } else {
                            // check its a match
                            QString symbol=*(leaf->fparms[0]->lvalue.n);
                            if (symbol != "user" && symbol != "bests") {
                                // not known
                                leaf->inerror=true;
                                DataFiltererrors << QString(tr("tests() first parameter must be 'user' or 'bests'."));
                            }
                        }

                        // date or power
                        if (leaf->fparms[1]->type != Leaf::Symbol) {
                            leaf->inerror=true;
                            DataFiltererrors << QString(tr("tests() second parameter must be 'duration' or 'power'."));
                        } else {
                            // check its a match
                            QString symbol=*(leaf->fparms[1]->lvalue.n);
                            if (symbol != "duration" && symbol != "power") {
                                // not known
                                leaf->inerror=true;
                                DataFiltererrors << QString(tr("tests() second parameter must be 'duration' or 'power'."));
                            }
                        }
                    }

                } else if (leaf->function == "metricname" || leaf->function == "metricunit" || leaf->function == "asstring" || leaf->function == "asaggstring") {

                    // check the parameters are all valid metric names
                    if (leaf->fparms.count() < 1) {
                        // need at least one metric name
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("no metric specified, %1(symbol) symbol should be a metric name")).arg(leaf->function);

                    } else {

                        // check they are all valid metrics
                        QRegExp symbols("^(name|start|stop|type|test|color|route|selected|date|time|filename)$");
                        for(int i=0; i<leaf->fparms.count(); i++) {
                            if (leaf->fparms[i]->type != Leaf::Symbol) {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("only metric names are supported")).arg(leaf->function);
                            } else {
                                QString symbol=*(leaf->fparms[0]->lvalue.n);
                                if (!symbols.exactMatch(symbol) && df->lookupMap.value(symbol,"") == "") {
                                    inerror = true;
                                    DataFiltererrors << QString(tr("unknown metric %1")).arg(symbol);
                                }
                            }
                        }
                    }
                } else if (leaf->function == "kmeans") {

                    if (leaf->fparms.count() < 4 || leaf->fparms[0]->type != Leaf::Symbol) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("kmeans(centers|assignments, k, dim1, dim2, dimn)"));
                    } else {
                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        if (symbol != "centers" && symbol != "assignments") {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("kmeans(centers|assignments, k, dim1, dim2, dimn) - %s unknown")).arg(symbol);
                        } else {
                            for(int i=1; i<leaf->fparms.count(); i++) validateFilter(context, df, leaf->fparms[i]);
                        }
                    }

                } else if (leaf->function == "metrics" || leaf->function == "metricstrings" ||
                           leaf->function == "aggmetrics" || leaf->function == "aggmetricstrings") {

                    // is the param a symbol and either a metric name or 'date'
                    if (leaf->fparms.count() < 1 || leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("%1(symbol|date), symbol should be a metric name")).arg(leaf->function);

                    } else if (leaf->fparms.count() >= 1) {

                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        if (symbol != "date" && symbol != "time" && df->lookupMap.value(symbol,"") == "") {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("invalid symbol '%1', should be either a metric name or 'time' or 'date'").arg(symbol));
                        }
                    } else if (leaf->fparms.count() >= 2) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[1]);

                    } else if (leaf->fparms.count() == 3) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[2]);

                    } else if (leaf->fparms.count() > 3) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("too many parameters: %1(symbol|date, start, stop)")).arg(leaf->function);
                    }

                } else if (leaf->function == "intervals" || leaf->function == "intervalstrings") {

                    // is the param a symbol and either a metric name or 'date'
                    if (leaf->fparms.count() < 1 || leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("%1(symbol|name|start|stop|type|test|color|route|selected|date|filename), symbol should be a metric name").arg(leaf->function));

                    } else if (leaf->fparms.count() >= 1) {

                        QRegExp symbols("^(name|start|stop|type|test|color|route|selected|date|time|filename)$");
                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        if (!symbols.exactMatch(symbol) && df->lookupMap.value(symbol,"") == "") {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("invalid symbol '%1', should be either a metric name or 'name|start|stop|type|test|color|route|selected|date|time|filename''").arg(symbol));

                        }
                    } else if (leaf->fparms.count() >= 2) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[1]);

                    } else if (leaf->fparms.count() == 3) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[2]);

                    } else if (leaf->fparms.count() > 3) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("too many parameters: %1(symbol, start, stop)").arg(leaf->function));
                    }

                } else if (leaf->function == "bests") {

                    int po=0;

                    // is the param a symbol and either a series name or 'date'
                    if (leaf->fparms.count() < 1 || leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("bests() - first parameters is a symbol should be a series name or 'date'"));

                    }

                    if (leaf->fparms.count() >= 1) {

                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        if (symbol == "date") {
                            leaf->seriesType = RideFile::none; // ok, want date
                        } else {
                            po = 1;
                            leaf->seriesType = RideFile::seriesForSymbol(symbol); // set the series type, used on execute.
                            if (leaf->seriesType==RideFile::none) {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("invalid series name '%1'").arg(symbol));
                            }
                        }
                    }

                    if (leaf->fparms.count() >= 2) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[1]);

                    }
                    if (leaf->fparms.count() >= 3) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[2]);

                    }
                    if (po && leaf->fparms.count() >= 4) {

                        // validate what was passed as second value - can be number or datestring
                        validateFilter(context, df, leaf->fparms[2]);

                    }

                    if ((po == 0 && leaf->fparms.count() >= 4) || leaf->fparms.count() >= 5) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("too many parameters"));
                    }

                } else if (leaf->function == "measures") {

                    if (leaf->fparms.count() != 2) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("measures(group, field) - must have group and field parameters."));

                    } else {

                        // check first field is group...
                        int group=-1;
                        QString group_symbol;
                        if (leaf->fparms[0]->type != String) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("measures group must be a string."));
                        } else {
                            group_symbol = *(leaf->fparms[0]->lvalue.s);
                            group = context->athlete->measures->getGroupSymbols().indexOf(group_symbol);
                            if (group < 0) {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("invalid measures group '%1'.").arg(group_symbol));
                            }
                        }

                        // check second field is valid...
                        if (leaf->fparms[1]->type != String) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("measures field must be a string."));
                        } else if (group >=0) {
                            QString field_symbol = *(leaf->fparms[1]->lvalue.s);
                            int field = context->athlete->measures->getFieldSymbols(group).indexOf(field_symbol);
                            if (field < 0 && field_symbol != "date") {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("invalid measures field '%1' for group '%2', should be one of: %3.").arg(field_symbol).arg(group_symbol)
                                .arg(context->athlete->measures->getFieldSymbols(group).join(", ")));
                            }
                        }
                    }

                } else if (leaf->function == "quantile") {

                    if (leaf->fparms.count() != 2) {

                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("quantil(vector, quantiles)"));
                    } else {
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "multisort") {

                    if (leaf->fparms.count() < 2) {
                        leaf->inerror = true;
                       DataFiltererrors << QString(tr("multisort(ascend|descend, list [, .. list n])"));
                    }

                    // need ascend|descend then a list
                    if (leaf->fparms.count() > 0 && leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("multisort(ascend|descend, list [, .. list n]), need to specify ascend or descend"));
                    }

                    // need all remaining parameters to be symbols
                    for(int i=1; i<leaf->fparms.count(); i++) {

                        // make sure the parameter makes sense
                        validateFilter(context, df, leaf->fparms[i]);

                        // check parameter is actually a symbol
                        if (leaf->fparms[i]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("multisort: list arguments must be a symbol"));
                        } else {
                            QString symbol = *(leaf->fparms[i]->lvalue.n);
                            if (!df->symbols.contains(symbol)) {
                                DataFiltererrors << QString(tr("'%1' is not a user symbol").arg(symbol));
                                leaf->inerror = true;
                            }
                        }
                    }

                } else if (leaf->function == "sort") {

                    // need ascend|descend then a list
                    if (leaf->fparms.count() != 2 || leaf->fparms[0]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("sort(ascend|descend, list), need to specify ascend or descend"));

                    }  else {

                       validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "rank") {

                    // need ascend|descend then a list
                    if (leaf->fparms.count() != 2 || leaf->fparms[0]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("rank(ascend|descend, list), need to specify ascend or descend"));

                    }  else {

                       validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "argsort") {

                    // need ascend|descend then a list
                    if (leaf->fparms.count() != 2 || leaf->fparms[0]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("argsort(ascend|descend, list), need to specify ascend or descend"));

                    }  else {

                       validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "multiuniq") {

                    if (leaf->fparms.count() < 1) {
                        leaf->inerror = true;
                       DataFiltererrors << QString(tr("multiuniq(list [, .. list n])"));
                    }

                    // need all remaining parameters to be symbols
                    for(int i=0; i<leaf->fparms.count(); i++) {

                        // check parameter is actually a symbol
                        if (leaf->fparms[i]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("multiuniq: list arguments must be a symbol"));
                        } else {
                            QString symbol = *(leaf->fparms[i]->lvalue.n);
                            if (!df->symbols.contains(symbol)) {
                                DataFiltererrors << QString(tr("'%1' is not a user symbol").arg(symbol));
                                leaf->inerror = true;
                            }

                        }
                    }

                } else if (leaf->function == "arguniq") {

                    // need ascend|descend then a list
                    if (leaf->fparms.count() != 1 ||  leaf->fparms[0]->type != Leaf::Symbol) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("argsort(ascend|descend, list), need to specify ascend or descend"));
                    } else {
                        QString symbol = *(leaf->fparms[0]->lvalue.n);
                        if (!df->symbols.contains(symbol)) {
                            DataFiltererrors << QString(tr("'%1' is not a user symbol").arg(symbol));
                            leaf->inerror = true;
                        }
                    }

                } else if (leaf->function == "curve") {

                    // on a user chart we can access computed values
                    // for other series, reduces overhead etc
                    if (leaf->fparms.count() != 2 || leaf->fparms[0]->type != Leaf::Symbol || leaf->fparms[1]->type != Leaf::Symbol) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("curve(seriesname, x|y|z|d|t), need to specify series name and data."));
                    } else {
                        // check series data
                        QString symbol = *(leaf->fparms[1]->lvalue.n);
                        if (!curveData.exactMatch(symbol)) {
                            DataFiltererrors << QString(tr("'%1' is not a valid, x, y, z, d or t expected").arg(symbol));
                            leaf->inerror = true;
                        }
                    }

                } else if (leaf->function == "meanmax") {

                    if (leaf->fparms.count() == 0 || leaf->fparms.count() > 3) {
                        // no
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("meanmax(SERIES|data [,start, stop]) or meanmax(xvector,yvector)"));

                    } else if (leaf->fparms.count() == 1 || leaf->fparms.count()==3) {

                        // is the param 1 a valid data series?
                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                           leaf->inerror = true;
                           DataFiltererrors << QString(tr("meanmax(SERIES), SERIES should be POWER, HEARTRATE etc."));
                        } else {
                            QString symbol=*(leaf->fparms[0]->lvalue.n);
                            leaf->seriesType = RideFile::seriesForSymbol(symbol);
                            if (symbol != "efforts" && leaf->seriesType==RideFile::none) {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("invalid series name '%1'").arg(symbol));
                            }
                        }

                        if (leaf->fparms.count() == 3) {

                            validateFilter(context, df, leaf->fparms[1]);
                            validateFilter(context, df, leaf->fparms[2]);
                        }
                    } else if (leaf->fparms.count() == 2) {

                        // generate from raw x,y data
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "dist") {

                    if (leaf->fparms.count() != 2) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("dist(series, data|bins), both parameters are required."));
                    }

                    // is the param 1 a valid data series?
                    if (leaf->fparms.count() < 1 || leaf->fparms[0]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("dist(series, data|bins), series should be one ofPOWER, HEARTRATE etc."));

                    } else {

                        QString symbol=*(leaf->fparms[0]->lvalue.n);
                        leaf->seriesType = RideFile::seriesForSymbol(symbol); // set the series type, used on execute.
                        if (leaf->seriesType==RideFile::none) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("invalid series name '%1'").arg(symbol));
                        }
                    }

                    if (leaf->fparms.count() == 2) {
                        if (leaf->fparms[1]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("dist(series, data|bins), second parameter must be ether 'data' or 'bins'"));
                        } else {
                            // check the symbol
                            QString symbol=*(leaf->fparms[1]->lvalue.n);
                            if (symbol != "data" && symbol != "bins") {
                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("dist(series, data|bins), second parameter must be ether 'data' or 'bins'"));

                            }
                        }
                    }

                } else if (leaf->function == "annotate") {

                    if (leaf->fparms.count() < 2 || leaf->fparms[0]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("annotate(label|hline|vline|voronoi, ...) need at least 2 parameters."));

                    } else {

                        QString type = *(leaf->fparms[0]->lvalue.n);

                        // is the type of annotation supported?
                        if (!annotateTypes.exactMatch(type)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("annotation type '%1' not available").arg(type));
                        } else {

                            // its valid type, but what about the parameters?
                            if (type == "voronoi" && leaf->fparms.count() != 2) { // VORONOI

                                leaf->inerror = true;
                                DataFiltererrors << QString(tr("annotate(voronoi, centers)"));

                            } else if (type == "lr") { // LINEAR REGRESSION LINE

                                if (leaf->fparms.count() != 3 || linestyle(*(leaf->fparms[1]->lvalue.n)) == Qt::NoPen) {

                                    leaf->inerror = true;
                                    DataFiltererrors << QString(tr("annotate(lr, solid|dash|dot|dashdot|dashdotdot, \"colorname\")"));

                                }

                            } else if (type == "hline" || type == "vline") { // HLINE and VLINE

                                // just make sure the type of line is supported, the other parameters
                                // can be coerced from whatever the user passed anyway
                                if (leaf->fparms.count() != 4 || leaf->fparms[2]->type != Leaf::Symbol
                                                                  || linestyle(*(leaf->fparms[2]->lvalue.n)) == Qt::NoPen) {
                                    leaf->inerror = true;
                                    DataFiltererrors << QString(tr("annotate(hline|vline, 'label', solid|dash|dot|dashdot|dashdotdot, value)"));
                                } else {
                                    // make sure the parms are well formed
                                    validateFilter(context, df, leaf->fparms[1]);
                                    validateFilter(context, df, leaf->fparms[3]);
                                }

                            } else { // Any other types, e.g LABEL

                                for(int i=1; i<leaf->fparms.count(); i++) validateFilter(context, df, leaf->fparms[i]);
                            }
                        }
                    }

                } else if (leaf->function == "smooth") {

                    if (leaf->fparms.count() < 2 || leaf->fparms[1]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("smooth(list, algorithm [,parameters]) need at least 2 parameters."));

                    } else {

                        QString algo = *(leaf->fparms[1]->lvalue.n);
                        if (!smoothAlgos.exactMatch(algo)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("smoothing algorithm '%1' not available").arg(algo));
                        } else {

                            if (algo == "sma") {
                                // smooth(list, sma, centred|forward|backward, window)
                                if (leaf->fparms.count() != 4 || leaf->fparms[2]->type != Leaf::Symbol) {
                                    leaf->inerror = true;
                                    DataFiltererrors << QString(tr("smooth(list, sma, forward|centered|backward, windowsize"));

                                } else {
                                    QRegExp parms("^(forward|centered|backward)$");
                                    QString parm1 = *(leaf->fparms[2]->lvalue.n);
                                    if (!parms.exactMatch(parm1)) {

                                        leaf->inerror = true;
                                        DataFiltererrors << QString(tr("smooth(list, sma, forward|centered|backward, windowsize"));
                                    }

                                    // check list and windowsize
                                    validateFilter(context, df, leaf->fparms[0]);
                                    validateFilter(context, df, leaf->fparms[3]);
                                }

                            } else if (algo == "ewma") {
                                // smooth(list, ewma, alpha)

                                if (leaf->fparms.count() != 3) {
                                    leaf->inerror = true;
                                    DataFiltererrors << QString(tr("smooth(list, ewma, alpha (between 0 and 1)"));
                                } else {
                                    // check list and windowsize
                                    validateFilter(context, df, leaf->fparms[0]);
                                    validateFilter(context, df, leaf->fparms[2]);
                                }
                            }
                        }
                    }

                } else if (leaf->function == "lowerbound") {

                    if (leaf->fparms.count() != 2) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("lowerbound(list, value), need list and value to find"));

                    } else {
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "lr") {

                    if (leaf->fparms.count() != 2) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("lr(x, y), need x and y vectors."));

                    } else {
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "mlr") {

                    if (leaf->fparms.count() < 2) {

                        leaf->inerror =true;
                        DataFiltererrors << QString(tr("mlr(yvector, xvector1 .. xvectorn), need at least 1 xvector and y vectors."));

                    } else {
                        for(int i=0; i<leaf->fparms.count(); i++)
                            validateFilter(context, df, leaf->fparms[i]);
                    }

                } else if (leaf->function == "lm") {

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("lm(expr, xlist, ylist), need formula, x and y data to fit to."));

                    } else {

                        // at this point pretty much anything goes, so long as it is
                        // syntactically correct. user needs to know what they are doing !
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                        validateFilter(context, df, leaf->fparms[2]);

                        QStringList symbols;
                        leaf->fparms[0]->findSymbols(symbols);
                        symbols.removeDuplicates();

                        // do we have any parameters that are not x ????
                        int count=0;
                        foreach(QString s, symbols)  if (s != "x") count++;

                        if (count == 0) {
                            leaf->inerror= true;
                            DataFiltererrors << QString(tr("lm(expr, xlist, ylist), formula must have at least one parameter to estimate.\n"));
                        }
                    }

                } else if (leaf->function == "sapply") {

                    if (leaf->fparms.count() != 2) {
                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("sapply(list, expr), need 2 parameters."));
                    } else {
                        validateFilter(context, df, leaf->fparms[0]);
                        validateFilter(context, df, leaf->fparms[1]);
                    }

                } else if (leaf->function == "pmc") {

                    if (leaf->fparms.count() < 2 || leaf->fparms[1]->type != Leaf::Symbol) {

                       leaf->inerror = true;
                       DataFiltererrors << QString(tr("pmc(metric, stress|lts|sts|sb|rr|date), need to specify a metric and series."));

                    } else {

                        // expression good?
                        validateFilter(context, df, leaf->fparms[0]);

                        QString symbol=*(leaf->fparms[1]->lvalue.n);
                        if (!pmcValidSymbols.exactMatch(symbol)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("invalid PMC series '%1'").arg(symbol));
                        }
                    }

                } else if (leaf->function == "banister") {

                    // 3 parameters
                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("should be banister(load_metric, perf_metric, nte|pte|perf|cp|date)"));
                    } else {

                        Leaf *first=leaf->fparms[0];
                        Leaf *second=leaf->fparms[1];
                        Leaf *third=leaf->fparms[2];

                        // check load metric name is valid
                        QString metric = first->signature();
                        QString lookup = df->lookupMap.value(metric, "");
                        if (lookup == "") {
                            leaf->inerror = true;
                            DataFiltererrors << QString("unknown load metric '%1'.").arg(metric);
                        }

                        // check perf metric name is valid
                        metric = second->signature();
                        lookup = df->lookupMap.value(metric, "");
                        if (lookup == "") {
                            leaf->inerror = true;
                            DataFiltererrors << QString("unknown perf metric '%1'.").arg(metric);
                        }

                        // check value
                        QString value = third->signature();
                        QRegExp banSymbols("^(nte|pte|perf|cp|date)$", Qt::CaseInsensitive);
                        if (!banSymbols.exactMatch(value)) {
                            leaf->inerror = true;
                            DataFiltererrors << QString("unknown %1, should be nte,pte,perf or cp.").arg(value);
                        }
                    }

                } else if (leaf->function == "XDATA") {

                    leaf->dynamic = false;

                    if (leaf->fparms.count() != 3) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("XDATA needs 3 parameters."));
                    } else {

                        // are the first two strings ?
                        Leaf *first=leaf->fparms[0];
                        Leaf *second=leaf->fparms[1];
                        if (first->type != Leaf::String || second->type != Leaf::String) {
                            DataFiltererrors << QString(tr("XDATA expects a string for first two parameters"));
                            leaf->inerror = true;
                        }

                        // is the third a symbol we like?
                        Leaf *third=leaf->fparms[2];
                        if (third->type != Leaf::Symbol) {
                            DataFiltererrors << QString(tr("XDATA expects a symbol, one of sparse, repeat, interpolate or resample for third parameter."));
                            leaf->inerror = true;
                        } else {
                            QStringList xdataValidSymbols;
                            xdataValidSymbols << "sparse" << "repeat" << "interpolate" << "resample";
                            QString symbol = *(third->lvalue.n);
                            if (!xdataValidSymbols.contains(symbol, Qt::CaseInsensitive)) {
                                DataFiltererrors << QString(tr("XDATA expects one of sparse, repeat, interpolate or resample for third parameter. (%1)").arg(symbol));
                                leaf->inerror = true;
                            } else {
                                // remember what algorithm was selected
                                int index = xdataValidSymbols.indexOf(symbol, Qt::CaseInsensitive);
                                // we're going to set explicitly rather than by cast
                                // so we don't need to worry about ordering the enum and stringlist
                                switch(index) {
                                case 0: leaf->xjoin = RideFile::SPARSE; break;
                                case 1: leaf->xjoin = RideFile::REPEAT; break;
                                case 2: leaf->xjoin = RideFile::INTERPOLATE; break;
                                case 3: leaf->xjoin = RideFile::RESAMPLE; break;
                                }
                            }
                        }
                    }

                } else if (leaf->function == "XDATA_UNITS") {

                    leaf->dynamic = false;

                    if (leaf->fparms.count() != 2) {
                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("XDATA_UNITS needs 2 parameters."));
                    } else {

                        // are the first two strings ?
                        Leaf *first=leaf->fparms[0];
                        Leaf *second=leaf->fparms[1];
                        if (first->type != Leaf::String || second->type != Leaf::String) {
                            DataFiltererrors << QString(tr("XDATA_UNITS expects a string for first two parameters"));
                            leaf->inerror = true;
                        }

                    }

                } else if (leaf->function == "isset" || leaf->function == "set" || leaf->function == "unset") {

                    // don't run it everytime a ride is selected!
                    leaf->dynamic = false;

                    // is the first a symbol ?
                    if (leaf->fparms.count() > 0) {
                        if (leaf->fparms[0]->type != Leaf::Symbol) {
                            leaf->inerror = true;
                            DataFiltererrors << QString(tr("isset/set/unset function first parameter is field/metric to set."));
                        } else {
                            QString symbol = *(leaf->fparms[0]->lvalue.n);

                            //  some specials are not allowed
                            if (!symbol.compare("Date", Qt::CaseInsensitive) ||
                                !symbol.compare("Time", Qt::CaseInsensitive) ||
                                !symbol.compare("x", Qt::CaseInsensitive) || // used by which
                                !symbol.compare("i", Qt::CaseInsensitive) || // used by which
                                !symbol.compare("Today", Qt::CaseInsensitive) ||
                                !symbol.compare("Current", Qt::CaseInsensitive) ||
                                !symbol.compare("RECINTSECS", Qt::CaseInsensitive) ||
                                !symbol.compare("NA", Qt::CaseInsensitive) ||
                                df->dataSeriesSymbols.contains(symbol) ||
                                symbol == "isRide" || symbol == "isSwim" ||
                                symbol == "isRun" || symbol == "isXtrain" ||
                                isCoggan(symbol)) {
                                DataFiltererrors << QString(tr("%1 is not supported in isset/set/unset operations")).arg(symbol);
                                leaf->inerror = true;
                            }
                        }
                    }

                    // make sure we have the right parameters though!
                    if (leaf->function == "issset" && leaf->fparms.count() != 1) {

                        leaf->inerror = true;
                        DataFiltererrors << QString(tr("isset has one parameter, a symbol to check."));

                    } else if ((leaf->function == "set" && leaf->fparms.count() != 3) ||
                        (leaf->function == "unset" && leaf->fparms.count() != 2)) {

                        leaf->inerror = true;
                        DataFiltererrors << (leaf->function == "set" ?
                            QString(tr("set function needs 3 paramaters; symbol, value and expression.")) :
                            QString(tr("unset function needs 2 paramaters; symbol and expression.")));

                    } else {

                        // still normal parm check !
                        foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);
                    }

                } else if (leaf->function == "estimate" || leaf->function == "estimates") {

                    // we only want two parameters and they must be
                    // a model name and then either ftp, cp, pmax, w'
                    // or a duration
                    QString name=leaf->function;
                    if (leaf->fparms.count() > 0) {
                        // check the model name
                        if (leaf->fparms[0]->type != Leaf::Symbol) {

                            leaf->fparms[0]->inerror = true;
                            DataFiltererrors << QString(tr("%1 function expects model name as first parameter.")).arg(name);

                        } else {

                            if (!pdmodels(context).contains(*(leaf->fparms[0]->lvalue.n))) {
                                leaf->inerror = leaf->fparms[0]->inerror = true;
                                DataFiltererrors << QString(tr("%1 function expects model name as first parameter")).arg(name);
                            }
                        }

                        if (leaf->fparms.count() > 1) {

                            // check symbol name if it is a symbol
                            if (leaf->fparms[1]->type == Leaf::Symbol) {
                                QRegExp estimateValidSymbols(name == "estimate" ? "^(cp|ftp|pmax|w')$" : "^(cp|ftp|pmax|w'|date)", Qt::CaseInsensitive);
                                if (!estimateValidSymbols.exactMatch(*(leaf->fparms[1]->lvalue.n))) {
                                    leaf->inerror = leaf->fparms[1]->inerror = true;
                                    DataFiltererrors << QString(tr("%1 function expects parameter or duration as second parameter")).arg(name);
                                }
                            } else {
                                validateFilter(context, df, leaf->fparms[1]);
                            }
                        }
                    }

                } else {

                    // normal parm check !
                    foreach(Leaf *p, leaf->fparms) validateFilter(context, df, p);
                }

                // does it exist?
                for(int i=0; DataFilterFunctions[i].parameters != -1; i++) {
                    if (DataFilterFunctions[i].name == leaf->function) {

                        // with the right number of parameters?
                        if (DataFilterFunctions[i].parameters && leaf->fparms.count() != DataFilterFunctions[i].parameters) {
                            DataFiltererrors << QString(tr("function '%1' expects %2 parameter(s) not %3")).arg(leaf->function)
                                                .arg(DataFilterFunctions[i].parameters).arg(fparms.count());
                            leaf->inerror = true;
                        }
                        found = true;
                        break;
                    }
                }

                // calling a user defined function, does it exist >=?
                if (found == false && !df->functions.contains(leaf->function)) {

                    DataFiltererrors << QString(tr("unknown function %1")).arg(leaf->function);
                    leaf->inerror = true;
                }
            }
        }
        break;

    case Leaf::UnaryOperation :
        { // unary minus needs a number, unary ! is happy with anything
            if (leaf->op == '-' && !Leaf::isNumber(df, leaf->lvalue.l)) {
                DataFiltererrors << QString(tr("unary negation on a string!"));
                leaf->inerror = true;
            }
        }
        break;
    case Leaf::BinaryOperation  :
    case Leaf::Operation  :
        {
            if (leaf->op == ASSIGN) {

                // assigm to user symbol - also creates first reference
                if (leaf->lvalue.l->type == Leaf::Symbol) {

                    // add symbol
                    QString symbol = *(leaf->lvalue.l->lvalue.n);
                    df->symbols.insert(symbol, Result(0));

                // assign to symbol[i] - must be to a user symbol
                } else if (leaf->lvalue.l->type == Leaf::Index) {

                    // this is being used in assignment so MUST reference
                    // a user symbol to be of any use
                    if (leaf->lvalue.l->lvalue.l->type != Leaf::Symbol) {

                        DataFiltererrors << QString(tr("array assignment must be to symbol."));
                        leaf->inerror = true;

                    } else {

                        // lets make sure it exists as a user symbol
                        QString symbol = *(leaf->lvalue.l->lvalue.l->lvalue.n);
                        if (!df->symbols.contains(symbol)) {
                            DataFiltererrors << QString(tr("'%1' unknown variable").arg(symbol));
                            leaf->inerror = true;
                        }
                    }
                } else if (leaf->lvalue.l->type == Leaf::Select) {

                        DataFiltererrors << QString(tr("assign to selection not supported at present. sorry."));
                        leaf->inerror = true;
                }

                // validate the lhs anyway
                validateFilter(context, df, leaf->lvalue.l);

                // and check the rhs is good too
                validateFilter(context, df, leaf->rvalue.l);

            } else {

                validateFilter(context, df, leaf->lvalue.l);
                validateFilter(context, df, leaf->rvalue.l);
            }
        }
        break;

    case Leaf::Logical :
        {
            validateFilter(context, df, leaf->lvalue.l);
            if (leaf->op) validateFilter(context, df, leaf->rvalue.l);
        }
        break;

    case Leaf::Conditional :
        {
            // three expressions to validate
            validateFilter(context, df, leaf->cond.l);
            validateFilter(context, df, leaf->lvalue.l);
            if (leaf->rvalue.l) validateFilter(context, df, leaf->rvalue.l);
        }
        break;

    case Leaf::Compound :
        {
            // is this a user defined function ?
            if (leaf->function != "") {

                // register it
                df->functions.insert(leaf->function, leaf);
            }

            // a list of statements, the last of which is what we
            // evaluate to for the purposes of filtering etc
            foreach(Leaf *p, *(leaf->lvalue.b)) validateFilter(context, df, p);
        }
        break;

    default:
        break;
    }
}

DataFilter::DataFilter(QObject *parent, Context *context) : QObject(parent), context(context), treeRoot(NULL), parent_(parent)
{
    // let folks know who owns this rumtime for signalling
    rt.owner = this;
    rt.chart = NULL;

    // be sure not to enable this by accident!
    rt.isdynamic = false;

    // set up the models we support
    rt.models << new CP2Model(context);
    rt.models << new CP3Model(context);
    rt.models << new MultiModel(context);
    rt.models << new ExtendedModel(context);
    rt.models << new WSModel(context);

    // random number generator
    gsl_rng_env_setup();
    unsigned long mySeed = QDateTime::currentMSecsSinceEpoch();
    T = gsl_rng_default; // Generator setup
    r = gsl_rng_alloc (T);
    gsl_rng_set(r, mySeed);

    configChanged(CONFIG_FIELDS);
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    //connect(context, SIGNAL(rideSelected(RideItem*)), this, SLOT(dynamicParse()));
}

DataFilter::DataFilter(QObject *parent, Context *context, QString formula) : QObject(parent), context(context), treeRoot(NULL), parent_(parent)
{
    // let folks know who owns this rumtime for signalling
    rt.owner = this;
    rt.chart = NULL;

    // be sure not to enable this by accident!
    rt.isdynamic = false;

    // set up the models we support
    rt.models << new CP2Model(context);
    rt.models << new CP3Model(context);
    rt.models << new MultiModel(context);
    rt.models << new ExtendedModel(context);
    rt.models << new WSModel(context);

    gsl_rng_env_setup();
    unsigned long mySeed = QDateTime::currentMSecsSinceEpoch();
    T = gsl_rng_default; // Generator setup
    r = gsl_rng_alloc (T);
    gsl_rng_set(r, mySeed);

    configChanged(CONFIG_FIELDS);

    // regardless of success or failure set signature
    setSignature(formula);

    DataFiltererrors.clear(); // clear out old errors
    DataFilter_setString(formula);
    DataFilterparse();
    DataFilter_clearString();
    treeRoot = DataFilterroot;

    // if it parsed (syntax) then check logic (semantics)
    if (treeRoot && DataFiltererrors.count() == 0)
        treeRoot->validateFilter(context, &rt, treeRoot);
    else
        treeRoot=NULL;

    // save away the results if it passed semantic validation
    if (DataFiltererrors.count() != 0)
        treeRoot= NULL;
}

Result DataFilter::evaluate(RideItem *item, RideFilePoint *p)
{
    if (!item || !treeRoot || DataFiltererrors.count())
        return Result(0);

    // reset stack
    rt.stack = 0;

    Result res(0);

    // if we are a set of functions..
    if (rt.functions.count()) {

        // ... start at main
        if (rt.functions.contains("main"))
            res = treeRoot->eval(&rt, rt.functions.value("main"), Result(0), 0, item, p);

    } else {

        // otherwise just evaluate the entire tree
        res = treeRoot->eval(&rt, treeRoot, Result(0), 0, item, p);
    }

    return res;
}

Result DataFilter::evaluate(Specification spec, DateRange dr)
{
    // if there is no current ride item then there is no data
    // so it really is ok to baulk at no current ride item here
    // we must always have a ride since context is used
    if (context->currentRideItem() == NULL || !treeRoot || DataFiltererrors.count()) return Result(0);

    Result res(0);

    // if we are a set of functions..
    if (rt.functions.count()) {

        // ... start at main
        if (rt.functions.contains("main"))
            res = treeRoot->eval(&rt, rt.functions.value("main"), Result(0), 0, const_cast<RideItem*>(context->currentRideItem()), NULL, NULL, spec, dr);

    } else {

        // otherwise just evaluate the entire tree
        res = treeRoot->eval(&rt, treeRoot, Result(0), 0, const_cast<RideItem*>(context->currentRideItem()), NULL, NULL, spec, dr);
    }

    return res;
}

Result DataFilter::evaluate(DateRange dr, QString filter)
{
    // reset stack
    rt.stack = 0;

    Specification spec;
    spec.setDateRange(dr);
    if (filter != "")  spec.addMatches(SearchFilterBox::matches(context, filter));

    return evaluate(spec, dr);
}

QStringList DataFilter::check(QString query)
{
    // since we may use it afterwards
    setSignature(query);

    // remember where we apply
    rt.isdynamic=false;

    // Parse from string
    DataFiltererrors.clear(); // clear out old errors
    DataFilter_setString(query);
    DataFilterparse();
    DataFilter_clearString();

    // save away the results
    treeRoot = DataFilterroot;

    // if it passed syntax lets check semantics
    if (treeRoot && DataFiltererrors.count() == 0) treeRoot->validateFilter(context, &rt, treeRoot);

    // ok, did it pass all tests?
    if (!treeRoot || DataFiltererrors.count() > 0) { // nope

        // no errors just failed to finish
        if (!treeRoot) DataFiltererrors << tr("malformed expression.");

    }

    errors = DataFiltererrors;
    return errors;
}

QStringList DataFilter::parseFilter(Context *context, QString query, QStringList *list)
{
    // remember where we apply
    this->list = list;
    rt.isdynamic=false;
    rt.symbols.clear();

    // regardless of fail/pass set the signature
    setSignature(query);

    //DataFilterdebug = 2; // no debug -- needs bison -t in src.pro
    DataFilterroot = NULL;

    // if something was left behind clear it up now
    clearFilter();

    // Parse from string
    DataFiltererrors.clear(); // clear out old errors
    DataFilter_setString(query);
    DataFilterparse();
    DataFilter_clearString();

    // save away the results
    treeRoot = DataFilterroot;

    // if it passed syntax lets check semantics
    if (treeRoot && DataFiltererrors.count() == 0) treeRoot->validateFilter(context, &rt, treeRoot);

    // ok, did it pass all tests?
    if (!treeRoot || DataFiltererrors.count() > 0) { // nope
        // no errors just failed to finish
        if (!treeRoot) DataFiltererrors << tr("malformed expression.");

        // Bzzzt, malformed
        emit parseBad(DataFiltererrors);
        clearFilter();

    } else { // yep! .. we have a winner!

        rt.isdynamic = treeRoot->isDynamic(treeRoot);

        // successfully parsed, lets check semantics
        //treeRoot->print(0,NULL);
        emit parseGood();

        // clear current filter list
        filenames.clear();

        // get all fields...
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // evaluate each ride...
            Result result = treeRoot->eval(&rt, treeRoot, Result(0), 0,item, NULL);
            if (result.isNumber && result.number()) {
                filenames << item->fileName;
            }
        }
        emit results(filenames);
        if (list) *list = filenames;
    }

    errors = DataFiltererrors;
    return errors;
}

void
DataFilter::dynamicParse()
{
    if (rt.isdynamic) {
        // need to reapply on current state

        // clear current filter list
        filenames.clear();

        // get all fields...
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // evaluate each ride...
            Result result = treeRoot->eval(&rt, treeRoot, Result(0), 0, item, NULL);
            if (result.isNumber && result.number())
                filenames << item->fileName;
        }
        emit results(filenames);
        if (list) *list = filenames;
    }
}

void DataFilter::clearFilter()
{
    if (treeRoot) {
        treeRoot->clear(treeRoot);
        treeRoot = NULL;
    }
    rt.isdynamic = false;
    sig = "";
}

void DataFilter::configChanged(qint32)
{
    rt.lookupMap.clear();
    rt.lookupType.clear();

    // create lookup map from 'friendly name' to INTERNAL-name used in summaryMetrics
    // to enable a quick lookup && the lookup for the field type (number, text)
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        QString symbol = factory.metricName(i);
        QString name = factory.rideMetric(symbol)->internalName();
        rt.lookupMap.insert(name.replace(" ","_"), symbol);
        rt.lookupType.insert(name.replace(" ","_"), true);
    }

    // now add the ride metadata fields -- should be the same generally
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
            QString underscored = field.name;
            if (!GlobalContext::context()->specialFields.isMetric(underscored)) {

                // translate to internal name if name has non Latin1 characters
                underscored = GlobalContext::context()->specialFields.internalName(underscored);
                field.name = GlobalContext::context()->specialFields.internalName((field.name));

                rt.lookupMap.insert(underscored.replace(" ","_"), field.name);
                rt.lookupType.insert(underscored.replace(" ","_"), (field.type > 2)); // true if is number
            }
    }

    // sample date series
    rt.dataSeriesSymbols = RideFile::symbols();
}

void
Result::vectorize(int count)
{
    // already vector of sufficient size
    if ((isNumber && asNumeric().count() >= count) ||
        (!isNumber && asString().count() >= count)) return;

    // ok, so must have at least 1 element to repeat
    if (isNumber && asNumeric().count() == 0) asNumeric() << number();
    if (!isNumber && asString().count() == 0) asString() << "";

    // repeat for size
    int it=0;
    int n=isNumber ? asNumeric().count() : asString().count();

    // repeat whatever we have
    while ((isNumber && asNumeric().count() < count) || (!isNumber && asString().count() < count)) {

        if (isNumber) {
            asNumeric() << asNumeric()[it];
            number() += asNumeric()[it];
        } else {
            asString() << asString()[it];
        }

        // loop thru wot we have
        it++; if (it == n) it=0;
    }
}

// date arithmetic, a bit of a brute force, but need to rely upon
// QDate arithmetic for handling months (so we don't have to)
static int monthsTo(QDate from, QDate to)
{
    int sign = from < to ? +1 : -1;
    int months = 0;
    for(QDate x=from; x.daysTo(to)* sign >=0; x=x.addMonths(sign)) {
        if (months*sign > 40000) break;
        months += sign;
    }
    months -= sign; // always goes past, so wind it back 1 step

    return months;
}

Result Leaf::eval(DataFilterRuntime *df, Leaf *leaf, const Result &x, long it, RideItem *m, RideFilePoint *p, const QHash<QString,RideMetric*> *c, const  Specification &s, const DateRange &d)
{
    // if error state all bets are off
    //if (inerror) return Result(0);

    switch(leaf->type) {

    //
    // LOGICAL EXPRESSION
    //
    case Leaf::Logical  :
    {
        switch (leaf->op) {
            case AND :
            {
                Result left = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);
                if (left.isNumber && left.number()) {
                    Result right = eval(df, leaf->rvalue.l,x, it, m, p, c, s, d);
                    if (right.isNumber && right.number()) return Result(true);
                }
                return Result(false);
            }
            case OR :
            {
                Result left = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);
                if (left.isNumber && left.number()) return Result(true);

                Result right = eval(df, leaf->rvalue.l,x, it, m, p, c, s, d);
                if (right.isNumber && right.number()) return Result(true);

                return Result(false);
            }

            default : // parenthesis
                return (eval(df, leaf->lvalue.l,x, it, m, p, c, s, d));
        }
    }
    break;

    //
    // FUNCTIONS
    //
    case Leaf::Function :
    {
        double duration;

        // calling a user defined function
        if (df->functions.contains(leaf->function)) {

            // going down
            df->stack += 1;

            // stack overflow
            if (df->stack > 500) {
                qDebug()<<"stack overflow";
                df->stack = 0;
                return Result(0);
            }

            Result res = eval(df, df->functions.value(leaf->function),x, it, m, p, c, s, d);

            // pop stack - if we haven't overflowed and reset
            if (df->stack > 0) df->stack -= 1;

            return res;
        }

        if (leaf->function == "isNumber") {
            return eval(df, leaf->fparms[0],x, it, m, p, c, s, d).isNumber;
        }

        if (leaf->function == "isString") {
            return !eval(df, leaf->fparms[0],x, it, m, p, c, s, d).isNumber;
        }


        // string functions
        if (leaf->function == "tolower") {

            Result returning = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);

            // coerce to string and then force returning as a string
            returning.asString();
            returning.isNumber=false;

            // update
            if (returning.isVector()) {
                for(int i=0; i<returning.asString().count(); i++)
                    returning.asString()[i]=returning.asString().at(i).toLower();
            } else {
                returning.string()=returning.string().toLower();
            }
            return returning;
        }

        if (leaf->function == "toupper") {

            Result returning = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);

            // coerce to string and then force returning as a string
            returning.asString();
            returning.isNumber=false;

            // update
            if (returning.isVector()) {
                for(int i=0; i<returning.asString().count(); i++)
                    returning.asString()[i]=returning.asString().at(i).toUpper();
            } else {
                returning.string()=returning.string().toUpper();
            }
            return returning;

        }

        if (leaf->function == "join") {

            Result returning = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            QString sep = eval(df, leaf->fparms[1],x, it, m, p, c, s, d).string();

            // coerce to string and then force returning as a string
            returning.asString();
            returning.isNumber=false;

            // only join vectors
            if (returning.isVector()) {
                returning.string() = "";
                bool first = true;
                for(int i=0; i<returning.asString().count(); i++) {
                    if (!first) returning.string() += sep;
                    returning.string() += returning.asString().at(i);
                    first = false;
                }
                returning.asString().clear();
            }
            return returning;
        }

        if (leaf->function == "split") {

            Result returning = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            QString sep = eval(df, leaf->fparms[1],x, it, m, p, c, s, d).string();

            // coerce to string and then force returning as a string
            returning.asString();
            returning.isNumber=false;

            QStringList tokens = returning.string().split(sep);
            returning.asString().clear();
            returning.string()="";

            // populate vector
            foreach(QString token, tokens) returning.asString() << token;

            return returning;
        }

        if (leaf->function == "trim") {

            Result returning = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);

            // coerce to string and then force returning as a string
            returning.asString();
            returning.isNumber=false;

            // update
            if (returning.isVector()) {
                for(int i=0; i<returning.asString().count(); i++)
                    returning.asString()[i]=returning.asString().at(i).trimmed();
            } else {
                returning.string()=returning.string().trimmed();
            }
            return returning;
        }

        if (leaf->function == "replace") {

            Result returning = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            QString s1 =  eval(df, leaf->fparms[1],x, it, m, p, c, s, d).string();
            QString s2 =  eval(df, leaf->fparms[2],x, it, m, p, c, s, d).string();

            // coerce to string and then force returning as a string
            returning.asString();
            returning.isNumber=false;

            // update
            if (returning.isVector()) {
                for(int i=0; i<returning.asString().count(); i++)
                    returning.asString()[i].replace(s1,s2);
            } else {
                returning.string().replace(s1,s2);
            }
            return returning;
        }

        if (leaf->function == "exists") {
            // get symbol name
            QString symbol =  *(leaf->fparms[0]->lvalue.s);

            // does it exist - as a function or symbol?
            return df->functions.contains(symbol) || df->symbols.contains(symbol);
        }

        if (leaf->function == "datestring" || leaf->function == "timestring") {

            Result returning;

            QDate earliest(1900,01,01);
            bool wantdate = leaf->function == "datestring";
            Result r=eval(df, leaf->fparms[0],x, it, m, p, c, s, d);

            if (r.isVector()) {
                QVector<QString> list;
                foreach(double n, r.asNumeric()) {
                    if (wantdate) list << earliest.addDays(n).toString("dd MMM yyyy");
                    else list << time_to_string(n);
                }
                returning = Result(list);
            } else {

                float n = r.number();
                if (wantdate) returning = Result(earliest.addDays(n).toString("dd MMM yyyy"));
                else returning = Result(time_to_string(n));
            }
            return returning;
        }

        // aggregate strings is easier to separate
        if (leaf->function == "asaggstring") {

            Result returning(0);
            returning.isNumber = false;

            // loop through rides and aggregate
            FilterSet fs;
            if (m) {
                fs.addFilter(m->context->isfiltered, m->context->filters);
                fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
            }
            Specification spec;
            spec.setFilterSet(fs);
            spec.setDateRange(d);  // current date range selected

            // we get passed a list of metrics that we need to aggregate and return
            // one value for each parameter passed
            for(int i=0; i<leaf->fparms.count(); i++) {

                // symbol dereference
                QString symbol=*(leaf->fparms[i]->lvalue.n);
                QString o_symbol = df->lookupMap.value(symbol,"");
                RideMetricFactory &factory = RideMetricFactory::instance();
                const RideMetric *e = factory.rideMetric(o_symbol);

                // loop through
                double withduration=0;
                double totalduration=0;
                double runningtotal=0;
                double minimum=0;
                double maximum=0;
                double count=0;

                // loop through rides for daterange
                foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                    if (!s.pass(ride)) continue; // relies upon the daterange being passed to eval...
                    if (!spec.pass(ride)) continue; // relies upon the daterange being passed to eval...


                    double value=0;
                    QString asstring;
                    value =  ride->getForSymbol(df->lookupMap.value(symbol,""), GlobalContext::context()->useMetricUnits);

                    // keep count of time for ride, useful when averaging
                    count++;
                    double duration = ride->getForSymbol("workout_time");
                    totalduration += duration;
                    withduration += value * duration;
                    runningtotal += value;
                    if (count==1) {
                        minimum = maximum = value;
                    } else {
                        if (value <minimum) minimum=value;
                        if (value >maximum) maximum=value;
                    }
                }

                // aggregate results
                double aggregate=0;
                switch(e ? e->type() : RideMetric::Average) {
                case RideMetric::Total:
                case RideMetric::RunningTotal:
                    aggregate = runningtotal;
                    break;
                default:
                case RideMetric::Average:
                    {
                    // aggregate taking into account duration
                    aggregate = withduration / totalduration;
                    break;
                    }
                case RideMetric::Low:
                    aggregate = minimum;
                    break;
                case RideMetric::Peak:
                    aggregate = maximum;
                    break;
                }

                // format and return
                returning.asString() << (e ? e->toString(aggregate) : "(null)");
            }

            return returning;
        }

        if (leaf->function == "metricname" || leaf->function == "metricunit" || leaf->function == "asstring") {

            bool wantname = (leaf->function == "metricname");
            bool wantunit = (leaf->function == "metricunit");

            QVector<QString> list;

            QRegExp symbols("^(name|start|stop|type|test|color|route|selected|date|time|filename)$");
            for(int i=0; i<leaf->fparms.count(); i++) {

                // symbol dereference
                QString symbol=*(leaf->fparms[i]->lvalue.n);
                QString o_symbol = df->lookupMap.value(symbol,"");
                RideMetricFactory &factory = RideMetricFactory::instance();
                const RideMetric *e = factory.rideMetric(o_symbol);

                if (wantname  && symbol == "date") list << tr("Date");
                else if (wantname  && symbol == "time") list << tr("Time");
                else if (wantname  && symbol == "name") list << tr("Name");
                else if (wantname  && symbol == "start") list << tr("Start");
                else if (wantname  && symbol == "stop") list << tr("Stop");
                else if (wantname  && symbol == "type") list << tr("Type");
                else if (wantname  && symbol == "test") list << tr("Test");
                else if (wantname  && symbol == "color") list << tr("Color");
                else if (wantname  && symbol == "route") list << tr("Route");
                else if (wantname  && symbol == "selected") list << tr("Selected");
                else if (wantname  && symbol == "filename") list << tr("File Name");
                else if (wantunit && symbols.exactMatch(symbol))
                    list << "";
                else {
                    if (e) {
                        if (wantname) list << e->name();
                        else if (wantunit) list << e->units(GlobalContext::context()->useMetricUnits);
                        else {
                            // get metric value then convert to string
                            double value = 0;
                            if (c) value = RideMetric::getForSymbol(o_symbol, c);
                            else value = m->getForSymbol(o_symbol, GlobalContext::context()->useMetricUnits);

                            // use metric converter to get as string with correct dp etc
                            list << e->toString(value);
                        }
                    }
                    else list << "(null)";
                }
            }

            // return a single value, or a list
            if (list.count() == 1) return Result(list[0]);
            else return Result(list);
        }

        if (leaf->function == "activities") {

            // filters activities using an expression, in the same way the
            // daterange function filters on date, in fact the daterange
            // closure could be implemented using activities and an expression

            // the user will have ecaped quotes to embed in a string
            QString prog = *(leaf->fparms[0]->lvalue.s);

            // now compile it
            DataFilter filter(df->owner->parent(), df->owner->context, prog);

            // failed to parse
            if (filter.root() == NULL) {
                return Result(0);
            }

            // get a filter list
            QStringList filters;
            foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                Result r = filter.evaluate(ride, NULL);
                if (r.number()) filters << ride->fileName;
            }
            Specification spec = s;
            spec.addMatches(filters);

            // now evaluate- but using an updated specification
            return  eval(df, leaf->fparms[1],x, it, m, p, c, spec, d);
        }

        if (leaf->function == "daterange") {

            // cannot get a context via ride as none selected or available
            if (m == NULL) return Result(0);

            // sets the daterange for the expression, a bit like a closure
            // so we don't have to add parameters to functions that do things
            // differently when working in trends view. e.g. measures, metrics etc
            if (leaf->fparms.count() == 1) {

                QString symbol =  *(leaf->fparms[0]->lvalue.s);
                if (symbol == "start") return Result(QDate(1900,01,01).daysTo(m->context->currentDateRange().from));
                else if (symbol == "stop") return Result(QDate(1900,01,01).daysTo(m->context->currentDateRange().to));

                return Result(0);

            } else if (leaf->fparms.count() == 3) {

                Result from =eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
                Result to =eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

                // so work out the date range
                QDate earliest(1900,01,01);
                DateRange ourdaterange(earliest.addDays(from.number()), earliest.addDays(to.number()));

                // return the expression, evaluated using our daterange
                return eval(df, leaf->fparms[2],x, it, m, p, c, s, ourdaterange);

            }
            return Result(0);
        }

        if (leaf->function == "config") {
            //
            // Get CP and W' estimates for date of ride
            //
            double CP = 0;
            double AeTP = 0;
            double FTP = 0;
            double WPRIME = 0;
            double PMAX = 0;
            int zoneRange;

            if (m->context->athlete->zones(m->sport)) {

                // if range is -1 we need to fall back to a default value
                zoneRange = m->context->athlete->zones(m->sport)->whichRange(m->dateTime.date());
                FTP = CP = zoneRange >= 0 ? m->context->athlete->zones(m->sport)->getCP(zoneRange) : 0;
                AeTP = zoneRange >= 0 ? m->context->athlete->zones(m->sport)->getAeT(zoneRange) : 0;
                WPRIME = zoneRange >= 0 ? m->context->athlete->zones(m->sport)->getWprime(zoneRange) : 0;
                PMAX = zoneRange >= 0 ? m->context->athlete->zones(m->sport)->getPmax(zoneRange) : 0;

                // use CP for FTP, or is it configured separately
                bool useCPForFTP = (appsettings->cvalue(m->context->athlete->cyclist,
                                    m->context->athlete->zones(m->sport)->useCPforFTPSetting(), 0).toInt() == 0);
                if (zoneRange >= 0 && !useCPForFTP) {
                    FTP = m->context->athlete->zones(m->sport)->getFTP(zoneRange);
                }

                // did we override CP in metadata ?
                int oCP = m->getText("CP","0").toInt();
                int oW = m->getText("W'","0").toInt();
                int oPMAX = m->getText("Pmax","0").toInt();
                if (oCP) CP=oCP;
                if (oW) WPRIME=oW;
                if (oPMAX) PMAX=oPMAX;
            }
            //
            // LTHR, AeTHR, MaxHR, RHR
            //
            int hrZoneRange = m->context->athlete->hrZones(m->sport) ?
                              m->context->athlete->hrZones(m->sport)->whichRange(m->dateTime.date())
                              : -1;

            int LTHR = hrZoneRange != -1 ?  m->context->athlete->hrZones(m->sport)->getLT(hrZoneRange) : 0;
            int AeTHR = hrZoneRange != -1 ?  m->context->athlete->hrZones(m->sport)->getAeT(hrZoneRange) : 0;
            int RHR = hrZoneRange != -1 ?  m->context->athlete->hrZones(m->sport)->getRestHr(hrZoneRange) : 0;
            int MaxHR = hrZoneRange != -1 ?  m->context->athlete->hrZones(m->sport)->getMaxHr(hrZoneRange) : 0;

            //
            // CV
            //
            int paceZoneRange = m->context->athlete->paceZones(m->isSwim) ?
                                m->context->athlete->paceZones(m->isSwim)->whichRange(m->dateTime.date()) :
                                -1;

            double CV = (paceZoneRange != -1) ? m->context->athlete->paceZones(m->isSwim)->getCV(paceZoneRange) : 0.0;
            double AeTV = (paceZoneRange != -1) ? m->context->athlete->paceZones(m->isSwim)->getAeT(paceZoneRange) : 0.0;

            //
            // HEIGHT and WEIGHT
            //
            double HEIGHT = m->getText("Height","0").toDouble();
            if (HEIGHT == 0) HEIGHT = m->context->athlete->getHeight(NULL);
            double WEIGHT = m->getWeight();

            QString symbol = leaf->series->lvalue.n->toLower();

            if (symbol == "cranklength") {
                return Result(appsettings->cvalue(m->context->athlete->cyclist, GC_CRANKLENGTH, 175.00f).toDouble() / 1000.0);
            }

            //
            // DOB and SEX
            //
            double DOB = QDate(1900,1,1).daysTo(appsettings->cvalue(m->context->athlete->cyclist, GC_DOB).toDate());
            QString SEX = appsettings->cvalue(m->context->athlete->cyclist, GC_SEX).toInt() ? "Female" : "Male";

            if (symbol == "cp") {
                return Result(CP);
            }
            if (symbol == "aetp") {
                return Result(AeTP);
            }
            if (symbol == "ftp") {
                return Result(FTP);
            }
            if (symbol == "w'") {
                return Result(WPRIME);
            }
            if (symbol == "pmax") {
                return Result(PMAX);
            }
            if (symbol == "cv") {
                return Result(CV);
            }
            if (symbol == "aetv") {
                return Result(AeTV);
            }
            if (symbol == "lthr") {
                return Result(LTHR);
            }
            if (symbol == "aethr") {
                return Result(AeTHR);
            }
            if (symbol == "rhr") {
                return Result(RHR);
            }
            if (symbol == "maxhr") {
                return Result(MaxHR);
            }
            if (symbol == "weight") {
                return Result(WEIGHT);
            }
            if (symbol == "height") {
                return Result(HEIGHT);
            }
            if (symbol == "units") {
                return Result(GlobalContext::context()->useMetricUnits ? 1 : 0);
            }
            if (symbol == "dob") {
                return Result(DOB);
            }
            if (symbol == "sex") {
                return Result(SEX);
            }
        }

        // zone descriptions and high / lows, but not cp/cv, w'/d' et al
        if (leaf->function == "zones") {
            // parms
            QString series = *leaf->fparms[0]->lvalue.n;
            QString field = *leaf->fparms[1]->lvalue.n;

            // what we will ultimately return
            QVector<QString> strings;

            // if m is null all bets are off
            if (m == NULL) return Result(0);

            // we want to minimise duplicated code
            // so get the zones and ranges setup
            // then loop through the zones in a single
            // loop, so lets get the control variables here
            QString sport;
            QDate date;
            const Zones *powerzones;
            const HrZones *hrzones;
            const PaceZones *pacezones;

            // which sport and date?
            if (d == DateRange()) {
                // Actvities view: activity sport and date
                sport = m->sport;
                date = m->dateTime.date();
            } else {
                // Trends view: sport from included activities and range end date
                int nActivities, nRides, nRuns, nSwims;
                m->context->athlete->rideCache->getRideTypeCounts(s, nActivities, nRides, nRuns, nSwims, sport);
                date = d.to;
            }

            // which range and how many zones are there
            int range=-1, nzones = 0;

            // metric prefix
            QString metricprefix;

            // metric/imperial pace setting for runs and swims
            bool metricpace;

            // setup
            if (series == "power") {

                // power zones are for Run or Bike, but not Swim
                powerzones = m->context->athlete->zones(sport);
                range= powerzones->whichRange(date);
                if (range >= 0) nzones = powerzones->numZones(range);
                metricprefix = "time_in_zone_L";

            } else if (series == "hr") {

                // hr zones are also for run or bike, but not Swim
                hrzones = m->context->athlete->hrZones(sport);
                range= hrzones->whichRange(date);
                if (range >= 0) nzones = hrzones->numZones(range);
                metricprefix = "time_in_zone_H";

            } else if (series == "pace") {

                // pace zones are for run or swim
                pacezones = m->context->athlete->paceZones(sport == "Swim");
                range= pacezones->whichRange(date);
                if (range >= 0) nzones = pacezones->numZones(range);
                metricprefix = "time_in_zone_P";
                metricpace = appsettings->value(nullptr, pacezones->paceSetting(), GlobalContext::context()->useMetricUnits).toBool();

            } else if (series == "fatigue") {

                int WPRIME = 0;
                powerzones = m->context->athlete->zones(sport);
                range = powerzones->whichRange(date);
                if (range >= 0) WPRIME = powerzones->getWprime(range);
                // easy for us, they are hardcoded
                for (int i=0; i<WPrime::zoneCount(); i++) {
                    if (field == "name") strings << WPrime::zoneName(i);
                    else if (field == "description") strings << WPrime::zoneDesc(i);
                    else if (field == "low") strings << QString("%1").arg(WPrime::zoneLo(i, WPRIME));
                    else if (field == "high") strings << QString("%1").arg(WPrime::zoneHi(i, WPRIME));
                }
                metricprefix = "wtime_in_zone_L";
                range=-1;
                nzones=WPrime::zoneCount(); // for metric lookup

            }

            if (field == "units") {

                // Units are fixed, except for pace where they depend on sport
                if (series == "power") return Result(RideFile::unitName(RideFile::watts, nullptr));
                else if (series == "hr") return Result(RideFile::unitName(RideFile::hr, nullptr));
                else if (series == "pace") return Result(pacezones->paceUnits(metricpace));
                else if (series == "fatigue") return Result(RideFile::unitName(RideFile::wprime, nullptr));

            } else if (field == "time" ) {

                // time in zones and percent in zones use metrics
                for(int n=0; n<nzones; n++) {
                    QString name = QString("%1%2").arg(metricprefix).arg(n+1);
                    double value = (d==DateRange()) ? m->getForSymbol(name, true)
                                                    : m->context->athlete->rideCache->getAggregate(name, s, true, true).toDouble();
                    strings << time_to_string(value);
                }

            } else if (field == "percent") {

                QString totalMetric = (series == "fatigue") ? "workout_time" : "time_recording";

                double total = (d==DateRange()) ? m->getForSymbol(totalMetric, true)
                                                : m->context->athlete->rideCache->getAggregate(totalMetric, s, true, true).toDouble();
                for(int n=0; n<nzones; n++) {
                    QString name = QString("%1%2").arg(metricprefix).arg(n+1);
                    double value = (d==DateRange()) ? m->getForSymbol(name, true)
                                                    : m->context->athlete->rideCache->getAggregate(name, s, true, true).toDouble();
                    double percent = round(value/total * 100.0);
                    strings << QString("%1").arg(percent);
                }

            } else {

                // all other fields use zoneinfo

                for(int n=0; range >=0 && n<nzones; n++) {

                    // placeholders for various zoneinfo calls
                    QString name, desc;
                    int ilow, ihigh;
                    double low,high;
                    double trimp; // ignored

                    if (series == "power") { powerzones->zoneInfo(range, n, name, desc, ilow, ihigh); low=ilow; high=ihigh; }
                    else if (series == "hr") { hrzones->zoneInfo(range, n, name, desc, ilow, ihigh, trimp);low=ilow; high=ihigh; }
                    else if (series == "pace") pacezones->zoneInfo(range, n, name, desc, low, high);


                    // so now we can do our thing - use the scheme for the names...
                    if (field == "name")  strings << name;

                    else if (field == "description") strings << desc;

                    else if (field == "low") {
                        if (series != "pace") strings << QString("%1").arg(low);
                        else strings << pacezones->kphToPaceString(low, metricpace);

                    } else if (field == "high") {
                        if (n==nzones-1) strings << ""; // infinite, so make blank
                        else {
                            if (series != "pace") strings << QString("%1").arg(high);
                            else strings << pacezones->kphToPaceString(high, metricpace);
                        }
                    }
                }

            }

            // returning what was collected
            return Result(strings);
        }

        // bool(expr) - convert to boolean
        if (leaf->function == "bool") {
            Result r=eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (r.isVector()) {
                Result returning;
                for(int i=0; i<r.asNumeric().count(); i++) {
                    int v = r.asNumeric().at(i) != 0 ? 1 : 0;
                    returning.asNumeric() << v;
                    returning.number() += v;
                }
                return returning;
            } else {
                if (r.number() != 0) return Result(1);
                else return Result(0);
            }
        }

        // string(expr) convert to string
        if (leaf->function == "string") {
            Result r=eval(df, leaf->fparms[0],x, it, m, p, c, s, d);

            if (r.isNumber) {
                if (r.isVector()) {
                    QVector<QString> results;
                    for(int i=0; i<r.asNumeric().count(); i++) {
                        QString asstring = Utils::removeDP(QString("%1").arg(r.asNumeric().at(i)));
                        results << asstring;
                    }
                    return Result(results);
                } else {
                    return Result(Utils::removeDP(QString("%1").arg("g", r.number())));
                }
            }

            return r; // just return what it is
        }

        // c (concat into a vector)
        // since we now support string and numeric values and vectors
        // we create a numeric vector by default.
        // If any values are found that are strings we convert all values
        // to strings avoid double looping
        if (leaf->function == "c") {

            // the return value of a vector is always its sum
            // so we need to keep that up to date too
            Result returning(0);

            // first evaluate all the parameters
            // so we can iterate over them
            QVector<Result> results;
            bool asstring=false;
            for(int i=0; i<leaf->fparms.count(); i++) {
                Result r = eval(df, leaf->fparms[i],x, it, m, p, c, s, d);
                results << r;
                if (!r.isNumber) asstring=true;
            }

            // now iterate and store
            if (asstring) returning.isNumber = false;
            for(int i=0; i<results.count(); i++) {
                Result v = results.at(i);
                if (v.isVector()) {
                    if (asstring) returning.asString().append(v.asString());
                    else  returning.asNumeric().append(v.asNumeric());
                } else {
                    if (asstring) returning.asString().append(v.string());
                    else  returning.asNumeric().append(v.number());
                }
                if (!asstring) returning.number() += v.number();
            }
            return returning;
        }

        if (leaf->function == "pdfbeta") {

            Result returning(0);
            double a= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number();
            double b= eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();
            Result v= eval(df, leaf->fparms[2],x, it, m, p, c, s, d);

            if (v.isVector() && v.isNumber) {

                // vector
                foreach(double val, v.asNumeric()) {
                    double f = gsl_ran_beta_pdf(val, a, b);
                    returning.asNumeric() << f;
                    returning.number() += f;
                }

            } else if (v.isNumber) returning.number() = gsl_ran_beta_pdf(v.number(), a,b);

            return returning;
        }

        if (leaf->function == "cdfbeta") {

            Result returning(0);
            double a= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number();
            double b= eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();
            Result v= eval(df, leaf->fparms[2],x, it, m, p, c, s, d);

            if (v.isVector() && v.isNumber) {

                // vector
                foreach(double val, v.asNumeric()) {
                    double f = gsl_cdf_beta_P(val, a, b);
                    returning.asNumeric() << f;
                    returning.number() += f;
                }

            } else if (v.isNumber) returning.number() = gsl_cdf_beta_P(v.number(), a,b);

            return returning;
        }

        if (leaf->function == "pdfgamma") {

            Result returning(0);
            double a= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number();
            double b= eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();
            Result v= eval(df, leaf->fparms[2],x, it, m, p, c, s, d);

            if (v.isVector() && v.isNumber) {

                // vector
                foreach(double val, v.asNumeric()) {
                    double f = gsl_ran_gamma_pdf(val, a, b);
                    returning.asNumeric() << f;
                    returning.number() += f;
                }

            } else if (v.isNumber) returning.number() = gsl_ran_gamma_pdf(v.number(), a,b);

            return returning;
        }

        if (leaf->function == "cdfgamma") {

            Result returning(0);
            double a= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number();
            double b= eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();
            Result v= eval(df, leaf->fparms[2],x, it, m, p, c, s, d);

            if (v.isVector() && v.isNumber) {

                // vector
                foreach(double val, v.asNumeric()) {
                    double f = gsl_cdf_gamma_P(val, a, b);
                    returning.asNumeric() << f;
                    returning.number() += f;
                }

            } else if (v.isNumber) returning.number() = gsl_cdf_gamma_P(v.number(), a,b);

            return returning;
        }

        if (leaf->function == "cdfnormal") {

            Result returning(0);
            double sigma= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number();
            Result v= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            if (v.isVector() && v.isNumber) {

                // vector
                foreach(double val, v.asNumeric()) {
                    double f = gsl_cdf_gaussian_P(val, sigma);
                    returning.asNumeric() << f;
                    returning.number() += f;
                }

            } else if (v.isNumber) returning.number() = gsl_cdf_gaussian_P(v.number(), sigma);

            return returning;
        }

        if (leaf->function == "pdfnormal") {

            Result returning(0);
            double sigma= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number();
            Result v= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            if (v.isVector() && v.isNumber) {

                // vector
                foreach(double val, v.asNumeric()) {
                    double f = gsl_ran_gaussian_pdf(val, sigma);
                    returning.asNumeric() << f;
                    returning.number() += f;
                }

            } else if (v.isNumber) returning.number() = gsl_ran_gaussian_pdf(v.number(), sigma);

            return returning;
        }

        // seq
        if (leaf->function == "seq") {
            Result returning(0);

            double start= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number();
            double stop= eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();
            double step= eval(df, leaf->fparms[2],x, it, m, p, c, s, d).number();

            if (step == 0) return returning; // nope!
            if (start > stop && step >0) return returning; // nope
            if (stop > start && step <0) return returning; // nope

            // ok lets go
            if (step > 0) {
                while(start <= stop) {
                    returning.asNumeric().append(start);
                    start += step;
                    returning.number() += start;
                }
            } else {
                while (start >= stop) {
                    returning.asNumeric().append(start);
                    start += step;
                    returning.number() += start;
                }
            }

            // sequence
            return returning;
        }

        // rep
        if (leaf->function == "rep") {
            Result returning(0);

            Result value= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            double count= eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();

            if (count <= 0) return returning;
            while (count >0) {
                if (value.isNumber) {
                    returning.asNumeric().append(value.number());
                    returning.number() += value.number();
                } else {
                    returning.isNumber = false;
                    returning.asString().append(value.string());
                }
                count--;
            }
            return returning;
        }

        // rev - reverse the vector
        if (leaf->function == "rev") {
            Result returning(0);
            Result value= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (value.isVector()) {
                if (value.isNumber) {
                    for(int i=value.asNumeric().count()-1; i>=0; i--) {
                        double v = value.asNumeric().at(i);
                        returning.asNumeric() << v;
                        returning.number() += v;
                    }
                } else {
                    returning.isNumber = false;
                    for(int i=value.asString().count()-1; i>=0; i--) {
                        QString v = value.asString().at(i);
                        returning.asString() << v;
                    }
                }
            }
            return returning;
        }

        // length
        if (leaf->function == "length") {
            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (v.isNumber) return Result(v.asNumeric().count());
            else return Result(v.asString().count());
        }

        // cumsum
        if (leaf->function == "cumsum") {
            Result returning(0);

            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (v.asNumeric().count() == 0) return Result(v.number());

            double cumsum = 0;
            for(int i=0; i < v.asNumeric().count(); i++) {
                cumsum += v.asNumeric()[i];
                returning.number() += cumsum;
                returning.asNumeric() << cumsum;
            }
            return returning;
        }

        // bin
        if (leaf->function == "bin") {
            // parm 1 - values
            // parm 2 - bins
            // values and bins must contain > 1 entry ! (returns 0 otherwise)
            // any value < bins[0] is discarded
            // any value > bins[last] is included in bins[last]

            Result values = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result bins = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            Result returning(0);

            // lets bin
            if (bins.asNumeric().count() > 1 && values.asNumeric().count() > 1) {

                // returns a vector with same number of bins as the bins vector
                returning.asNumeric().fill(0, bins.asNumeric().size());

                // loop across the values updating the count for the relevant bin
                for(int i=0; i<values.asNumeric().count(); i++) {
                    double value=values.asNumeric().at(i);

                    // must be greater, values less than first bin are discarded
                    for(int bin=bins.asNumeric().count()-1; bin>=0; bin--) {
                        if (value > bins.asNumeric().at(bin)) {
                            returning.asNumeric()[bin]++;
                            break;
                        }
                    }
                }
            }
            return returning;
        }

        // aggregate
        if (leaf->function == "aggregate") {

            // returns an aggregated vector, using by as the group by value
            // and func defines how we aggregate
            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result by = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            QString func = (*(leaf->fparms[2]->lvalue.n)).toLower();

            Result returning(0);

            // anything other than a vector makes no sense, so
            // lets turn a number into a vector of 1 value
            if (v.asNumeric().count()==0) v.asNumeric() << v.number();

            // by is coerced to strings if it isn't a string list already
            // this simplifies all the logic for watching it change
            if (by.asString().count()==0) by.asString() << by.string();

            // state as we loop through a group
            QString laststring =by.asString()[0];

            double count=0, value=0;
            bool first=true;

            for(int byit=0,it=0; it < v.asNumeric().count(); byit++,it++) {

                // repeat by, in cases where fewer entries than in
                // the vector being aggregated
                if (byit >= by.asString().count()) byit=0;

                // add last and reset for next group
                if (laststring != by.asString()[byit]) {
                    returning.number() += value; // sum
                    returning.asNumeric() << value;
                    value=0;
                    count=0;
                    first=true; // first in group
                }

                // update the aggregate for this group
                double xx = v.asNumeric()[it];
                count++;

                // mean
                if (func == "mean")  value = ((value * (count-1)) + xx) / count;

                // sum
                if (func == "sum") value += xx;

                // max
                if (func == "max") {
                    if (first) value = xx;
                    else if (xx > value) value = xx;
                }

                // min
                if (func == "min") {
                    if (first) value = xx;
                    else if (xx < value) value = xx;
                }

                // count
                if (func == "count") {
                    value++;
                }

                // on to the next
                laststring = by.asString()[byit];
                first = false;
            }

            // the last
            returning.number() += value; // sum
            returning.asNumeric() << value;

            return returning;
        }

        // append
        if (leaf->function == "append") {

            // append (symbol, stuff, pos)

            // lets get the symbol and a pointer to it's value
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            Result current = df->symbols.value(symbol);

            // where to place it? -1 on end (not passed as a parameter)
            int pos=-1;
            if (leaf->fparms.count() == 3) pos = eval(df, leaf->fparms[2],x, it, m, p, c, s, d).number();

            // check for bounds
            if (pos <0 || pos >current.asNumeric().count()) pos=-1;

            // ok, what to append
            Result append = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            // do it...
            if (append.isVector()) {

                current.number() += append.number();

                if (pos==-1) {
                    if (current.isNumber) current.asNumeric().append(append.asNumeric());
                    else current.asString().append(append.asString());
                } else {
                    // insert at pos
                    if (current.isNumber) {
                        for(int i=0; i<append.asNumeric().count(); i++)
                            current.asNumeric().insert(pos+i, append.asNumeric().at(i));
                    } else {

                        for(int i=0; i<append.asString().count(); i++)
                            current.asString().insert(pos+i, append.asString().at(i));
                    }
                }

            } else {

                current.number() += append.number();

                if (current.isNumber) {
                    if (pos == -1) current.asNumeric().append(append.number()); // just a single number
                    else current.asNumeric().insert(pos, append.number()); // just a single number
                } else {
                    if (pos == -1) current.asString().append(append.string()); // just a single string
                    else current.asString().insert(pos, append.string()); // just a single string
                }
            }

            // update value
            df->symbols.insert(symbol, current);

            return Result(current.asNumeric().count());
        }

        // remove
        if (leaf->function == "remove") {

            // remove (symbol, pos, count)

            // lets get the symbol and a pointer to it's value
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            Result current = df->symbols.value(symbol);

            // where to place it? -1 on end (not passed as a parameter)
            long pos = eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();

            // ok, what to append
            long count = eval(df, leaf->fparms[2],x, it, m, p, c, s, d).number();


            // check.. and return unchanged if out of bounds
            if (pos < 0 || (current.isNumber && (pos > current.asNumeric().count() || pos+count >current.asNumeric().count()))
                        || (!current.isNumber && (pos > current.asString().count() || pos+count >current.asString().count()))) {
                return Result(current.asNumeric().count());
            }

            // so lets do it
            // do it...
            if (current.isNumber) current.asNumeric().remove(pos, count);
            else current.asString().remove(pos, count);

            // update value
            df->symbols.insert(symbol, current);

            return Result(current.isNumber ? current.asNumeric().count() : current.asString().count());
        }

        // mid
        if (leaf->function == "mid") {

            Result returning(0);

            // mid (a, pos, count)
            // where to place it? -1 on end (not passed as a parameter)
            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            long pos = eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();
            long count = eval(df, leaf->fparms[2],x, it, m, p, c, s, d).number();

            // return same type
            returning.isNumber = v.isNumber;

            // check.. and return unchanged if out of bounds
            if (pos < 0 || (v.isNumber && (pos > v.asNumeric().count() || pos+count >v.asNumeric().count()))
                        || (!v.isNumber && (pos > v.asString().count() || pos+count >v.asString().count()))) {
                return returning;
            }

            // so lets do it- remember to sum
            if (v.isNumber) {
                returning.asNumeric() = v.asNumeric().mid(pos, count);
                returning.number() = 0;
                for(int i=0; i<returning.asNumeric().count(); i++) returning.number() += returning.asNumeric()[i];
            } else {
                returning.asString() = v.asString().mid(pos,count);
            }

            return returning;
        }

        if (leaf->function == "xdata") {
            Result returning(0);

            QString name = *(leaf->fparms[0]->lvalue.s);
            QString series; // name, km or secs
            bool km=false, secs=false;

            if (leaf->fparms[1]->type == Leaf::String) series = *(leaf->fparms[1]->lvalue.s);
            if (leaf->fparms[1]->type == Leaf::Symbol) {
                series = *(leaf->fparms[1]->lvalue.n);
                if (series == "km") km = true;
                if (series == "secs") secs = true;
            }

            // lets get the xdata series - only if the item is already open to avoid accidentally
            // iterating over all ride data, same approach as in the samples function below
            if (m == NULL || !m->isOpen() || m->ride(false) == NULL) {
                return Result(0);

            } else {
                XDataSeries *xds = m->ride()->xdata(name);
                if (xds == NULL) return returning;

                // now we need to get all the values into returning
                int index=0;
                if (km || secs || (index=xds->valuename.indexOf(series)) != -1) {
                    foreach(XDataPoint *p, xds->datapoints) {

                        // honor interval boundaries when limits are set
                        if (p->secs < s.secsStart()) continue;
                        if (s.secsEnd() > -1 && p->secs > s.secsEnd()) break;

                        double value=0;
                        if (km) value = p->km;
                        else if (secs) value = p->secs;
                        else if (index >=0)  value =p->number[index];

                        returning.asNumeric() << value;
                        returning.number()  += value;
                    }
                }
            }
            return returning;
        }
        if (leaf->function == "samples") {

            // nothing to return -- note we check if the ride is open
            // this is to avoid misuse outside of a filter when working
            // with a specific ride.
            if (m == NULL || !m->isOpen() || m->ride(false) == NULL || m->ride(false)->dataPoints().count() == 0) {
                return Result(0);

            } else {

                // create a vector for the currently selected ride.
                // should be used with care by the user !!
                // if they use it in a filter or metric sample() function
                // it could get ugly.. but thats no reason to avoid
                // the usefulness of getting the entire data series
                // in one hit for those that want to work with vectors
                Result returning(0);

                if (leaf->seriesType == RideFile::wbal || leaf->seriesType == RideFile::none) {

                    // W'Bal and W'Bal time
                    returning.asNumeric() = leaf->seriesType == RideFile::wbal ? m->ride()->wprimeData()->ydata() : m->ride()->wprimeData()->xdata(false);
                    for(int i=0; i<returning.asNumeric().count(); i++) {
                        // convert x values from minutes to seconds
                        if (leaf->seriesType == RideFile::none)  returning.asNumeric()[i] = returning.asNumeric().at(i) * 60.0;
                        // calculate sum for both
                        returning.number() += returning.asNumeric().at(i); // sum
                    }

                } else {

                    // usual activity samples; HR, Power etc
                    if (!s.isEmpty(m->ride())) {

                        // spec may limit to an interval
                        RideFileIterator it(m->ride(), s);
                        while(it.hasNext()) {
                            struct RideFilePoint *p = it.next();
                            double value=p->value(leaf->seriesType);
                            returning.number() += value;
                            returning.asNumeric().append(value);
                        }
                    }
                }
                return returning;
            }
        }

        if (leaf->function == "metadata") {

            Result returning("");
            returning.isNumber = false;
            QString name = eval(df, leaf->fparms[0],x, it, m, p, c, s, d).string();

            if (d.from==QDate() && d.to==QDate()) {

                // ride only
                returning.string() = m->getText(name, "");

            } else {

                // vector for a date range
                FilterSet fs;
                fs.addFilter(m->context->isfiltered, m->context->filters);
                fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
                Specification spec;
                spec.setFilterSet(fs);

                // date range
                spec.setDateRange(d);
                foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                    if (!s.pass(ride)) continue; // relies upon the daterange being passed to eval...
                    if (!spec.pass(ride)) continue; // relies upon the daterange being passed to eval...

                    QString tag = ride->getText(name, "");
                    returning.asString() << tag;
                }
            }
            return returning;
        }

        // normalize values to between 0 and 1
        // use when generating a heatmap in a data overview table
        // but potentially for other things in the future
        if (leaf->function == "normalize") {

            Result min =  eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result max =  eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            Result value =  eval(df, leaf->fparms[2],x, it, m, p, c, s, d);
            Result returning(0);

            // return as number or vector
            if (value.isVector()) {

                foreach(double v, value.asNumeric()) returning.asNumeric() << Utils::heat(min.number(), max.number(), v);

            } else {
                returning = Result(Utils::heat(min.number(), max.number(), value.number()));
            }

            return returning;
        }

        if (leaf->function == "filename") {

            Result returning("");
            returning.isNumber = false;

            if (d.from==QDate() && d.to==QDate()) {

                // ride only
                returning.string() = m->fileName;

            } else {

                // vector for a date range
                FilterSet fs;
                fs.addFilter(m->context->isfiltered, m->context->filters);
                fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
                Specification spec;
                spec.setFilterSet(fs);

                // date range
                spec.setDateRange(d);
                foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                    if (!s.pass(ride)) continue; // relies upon the daterange being passed to eval...
                    if (!spec.pass(ride)) continue; // relies upon the daterange being passed to eval...

                    returning.asString() << ride->fileName;
                }
            }
            return returning;
        }

        if (leaf->function == "kmeans") {
            // kmeans(centers|assignments, k, dim1, dim2, dim3)

            Result returning(0);

            QString symbol = *(leaf->fparms[0]->lvalue.n);
            bool wantcenters=false;
            if (symbol == "centers") wantcenters=true;

            // get k
            int k = eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();

            FastKmeans *kmeans = new FastKmeans();

            // loop through the dimensions
            for(int i=2; i<leaf->fparms.count(); i++)
                kmeans->addDimension(eval(df, leaf->fparms[i],x, it, m, p, c, s, d).asNumeric());

            // calculate
            if (kmeans->run(k)) {
                if (wantcenters) returning = kmeans->centers();
                else returning = kmeans->assignments();
            }

            return returning;
        }

        if (leaf->function == "metrics" || leaf->function == "metricstrings" ||
            leaf->function == "aggmetrics" || leaf->function == "aggmetricstrings") {

            bool wantstrings = (leaf->function.endsWith("strings"));
            QDate earliest(1900,01,01);
            bool wantdate=false;
            bool wanttime=false;
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            if (symbol == "date") wantdate=true;
            if (symbol == "time") wanttime=true;

            // find the metric
            QString o_symbol = df->lookupMap.value(symbol,"");
            RideMetricFactory &factory = RideMetricFactory::instance();
            const RideMetric *e = factory.rideMetric(o_symbol);

            // only aggregate if its a metric!
            bool wantaggregate = e != NULL && (leaf->function.startsWith("agg"));

            // returning numbers or strings
            Result returning(0);
            Result durations(0); // for aggregating
            if (wantstrings) returning.isNumber=false;

            FilterSet fs;
            if (m) {
                fs.addFilter(m->context->isfiltered, m->context->filters);
                fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
            }
            Specification spec;
            spec.setFilterSet(fs);

            // date range can be controlled, if no date range is set then we just
            // use the currently selected date range, otherwise start - today or start - stop
            if (leaf->fparms.count() == 3 && Leaf::isNumber(df, leaf->fparms[1]) && Leaf::isNumber(df, leaf->fparms[2])) {

                // start to stop
                Result b = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
                QDate start = earliest.addDays(b.number());

                Result e = eval(df, leaf->fparms[2],x, it, m, p, c, s, d);
                QDate stop = earliest.addDays(e.number());

                spec.setDateRange(DateRange(start,stop));

            } else if (leaf->fparms.count() == 2 && Leaf::isNumber(df, leaf->fparms[1])) {

                // start to today
                Result b = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
                QDate start = earliest.addDays(b.number());
                QDate stop = QDate::currentDate();

                spec.setDateRange(DateRange(start,stop));

            } else {
                spec.setDateRange(d); // fallback to daterange selected
            }

            // no ride selected, or none available
            if (m == NULL) return Result(0);

            // for aggregating
            double withduration=0;
            double totalduration=0;
            double runningtotal=0;
            double minimum=0;
            double maximum=0;
            double count=0;

            // loop through rides for daterange
            foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                if (!s.pass(ride)) continue; // relies upon the daterange being passed to eval...
                if (!spec.pass(ride)) continue; // relies upon the daterange being passed to eval...


                double value=0;
                QString asstring;
                if(wantdate) {
                    value= QDate(1900,01,01).daysTo(ride->dateTime.date());
                    if (wantstrings) asstring = ride->dateTime.date().toString("dd MMM yyyy");
                } else if (wanttime) {
                    value= QTime(0,0,0).secsTo(ride->dateTime.time());
                    if (wantstrings) asstring = ride->dateTime.toString("hh:mm:ss");
                } else {
                    value =  ride->getForSymbol(df->lookupMap.value(symbol,""), GlobalContext::context()->useMetricUnits);
                    if (wantstrings) e ? asstring = e->toString(value) : "(null)";
                }

                // keep count of time for ride, useful when averaging
                count++;
                double duration = ride->getForSymbol("workout_time");
                totalduration += duration;
                withduration += value * duration;
                runningtotal += value;
                if (count==1) {
                    minimum = maximum = value;
                } else {
                    if (value <minimum) minimum=value;
                    if (value >maximum) maximum=value;
                }

                if (wantstrings) { // capture strings as we go, only if we don't aggregate
                    returning.asString().append(asstring);
                } else {
                    returning.number() += value;
                    returning.asNumeric().append(value);
                }
            }

            // return an aggregate?
            if (wantaggregate) {
                double aggregate=0;
                switch(e->type()) {
                case RideMetric::Total:
                case RideMetric::RunningTotal:
                    aggregate = runningtotal;
                    break;
                default:
                case RideMetric::Average:
                    {
                    // aggregate taking into account duration
                    aggregate = withduration / totalduration;
                    break;
                    }
                case RideMetric::Low:
                    aggregate = minimum;
                    break;
                case RideMetric::Peak:
                    aggregate = maximum;
                    break;
                }

                // format and return
                if (wantstrings) returning = Result(e->toString(aggregate));
                else returning = Result(aggregate);
            }

            return returning;
        }

        if (leaf->function == "intervals" || leaf->function == "intervalstrings") {

            bool currentride = false;
            bool wantstrings = (leaf->function == "intervalstrings");
            QDate earliest(1900,01,01);
            QString symbol = *(leaf->fparms[0]->lvalue.n);

            // find the metric
            QString o_symbol = df->lookupMap.value(symbol,"");
            RideMetricFactory &factory = RideMetricFactory::instance();
            const RideMetric *e = factory.rideMetric(o_symbol);

            // returning numbers or strings
            Result returning(0);
            if (wantstrings) returning.isNumber=false;

            FilterSet fs;
            fs.addFilter(m->context->isfiltered, m->context->filters);
            fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
            Specification spec;
            spec.setFilterSet(fs);

            // date range can be controlled, if no date range is set then we just
            // use the currently selected date range if valid, otherwise current activity
            if (leaf->fparms.count() == 3 && Leaf::isNumber(df, leaf->fparms[1]) && Leaf::isNumber(df, leaf->fparms[2])) {

                // start to stop
                Result b = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
                QDate start = earliest.addDays(b.number());

                Result e = eval(df, leaf->fparms[2],x, it, m, p, c, s, d);
                QDate stop = earliest.addDays(e.number());

                spec.setDateRange(DateRange(start,stop));

            } else if (leaf->fparms.count() == 2 && Leaf::isNumber(df, leaf->fparms[1])) {

                // start to today
                Result b = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
                QDate start = earliest.addDays(b.number());
                QDate stop = QDate::currentDate();

                spec.setDateRange(DateRange(start,stop));

            } else if (d != DateRange()) {
                spec.setDateRange(d); // fallback to daterange selected, if provided
            } else {
                currentride = true;
            }

            // no rides available or selected
            if (m == NULL) return Result(0);

            // loop through rides for daterange or currentride
            foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                // when current ride is requested we loop one time to avoid code duplication,
                // otherwise filters are honored and only passing rides are processed
                if (currentride) ride = m;
                else if (!s.pass(ride)) continue;
                else if (!spec.pass(ride)) continue;

                foreach(IntervalItem *ii, ride->intervals()) {

                    double value=0;
                    QString asstring;
                    if(symbol == "date") {
                        value= QDate(1900,01,01).daysTo(ride->dateTime.date());
                        if (wantstrings) asstring = ride->dateTime.date().toString("dd MMM yyyy");
                    } else if(symbol == "time") {
                        value= QTime(0,0,0).secsTo(ride->dateTime.time().addSecs(ii->start));
                        if (wantstrings) asstring = ride->dateTime.time().addSecs(ii->start).toString("hh:mm:ss");
                    } else if(symbol == "filename") {
                        asstring = ride->fileName;
                    } else if(symbol == "name") {
                        asstring = ii->name;
                    } else if(symbol == "start") {
                        value = ii->start;
                        asstring = time_to_string(ii->start);
                    } else if(symbol == "stop") {
                        value = ii->stop;
                        asstring = time_to_string(ii->stop);
                    } else if(symbol == "type") {
                        asstring = RideFileInterval::typeDescription(ii->type);
                    } else if(symbol == "test") {
                        value = ii->test;
                        asstring = QString("%1").arg(ii->test);
                    } else if(symbol == "color") {
                        // apply item color, remembering that 1,1,1 means use default (reverse in this case)
                        if (ii->color == QColor(1,1,1,1)) {
                            // use the inverted color, not plot marker as that hideous
                            QColor col =GCColor::invertColor(GColor(CPLOTBACKGROUND));
                            // white is jarring on a dark background!
                            if (col==QColor(Qt::white)) col=QColor(127,127,127);
                            asstring = col.name();
                        } else {
                            asstring = ii->color.name();
                        }
                    } else if(symbol == "route") {
                        asstring = ii->route.toString();
                    } else if(symbol == "selected") {
                        value = ii->selected;
                        asstring = QString("%1").arg(ii->selected);
                    } else {
                        value = ii->getForSymbol(df->lookupMap.value(symbol,""), GlobalContext::context()->useMetricUnits);
                        if (wantstrings) e ? asstring = e->toString(value) : "(null)";
                    }

                    if (wantstrings) {
                        returning.asString().append(asstring);
                    } else {
                        returning.number() += value;
                        returning.asNumeric().append(value);
                    }

                }

                // when current ride is requested we are done
                if (currentride) break;
            }
            return returning;
        }

        // measures
        if (leaf->function == "measures") {

            Result returning(0);
            QDate earliest(1900,01,01);
            bool wantdate=false;

            FilterSet fs;
            fs.addFilter(m->context->isfiltered, m->context->filters);
            fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
            Specification spec;
            spec.setFilterSet(fs);

            spec.setDateRange(d); // fallback to daterange selected

            // group number
            QString group_symbol = *(leaf->fparms[0]->lvalue.s);
            int group = m->context->athlete->measures->getGroupSymbols().indexOf(group_symbol);

            // field number -- -1 means its date
            QString field_symbol = *(leaf->fparms[1]->lvalue.s);
            int field = m->context->athlete->measures->getFieldSymbols(group).indexOf(field_symbol);
            if (field < 0) wantdate = true;

            // what dates do we have measures for ?
            QDate firstDate = m->context->athlete->measures->getStartDate(group);
            QDate lastDate = m->context->athlete->measures->getEndDate(group);

            // get measures
            if (firstDate == QDate() || lastDate == QDate()) return returning;

            for(QDate date=firstDate; date <= lastDate; date=date.addDays(1)) {

                if (!spec.pass(date)) continue;
                double value;
                if (wantdate) value = earliest.daysTo(date);
                else value = m->context->athlete->measures->getFieldValue(group,date,field);

                returning.number() += value;
                returning.asNumeric() << value;
            }
            return returning;
        }

        // retrieve best meanmax effort for a given duration and daterange
        if (leaf->function == "bests") {

            // work out what the date range is...
            QDate earliest(1900,01,01);
            Result returning(0);
            int duration = 0;
            int po = 0;

            // if want dates, the series and duration are not relevant
            // otherwise we need to get duration and all parameters are
            // offset by one in the parameters list
            if (leaf->seriesType != RideFile::none) {
                po=1;
                duration = eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();
            }

            FilterSet fs;
            fs.addFilter(m->context->isfiltered, m->context->filters);
            fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
            Specification spec;
            spec.setFilterSet(fs);

            // date range can be controlled, if no date range is set then we just
            // use the currently selected date range, otherwise start - today or start - stop
            if (leaf->fparms.count() == (3+po) && Leaf::isNumber(df, leaf->fparms[1+po]) && Leaf::isNumber(df, leaf->fparms[2+po])) {

                // start to stop
                Result b = eval(df, leaf->fparms[1+po],x, it, m, p, c, s, d);
                QDate start = earliest.addDays(b.number());

                Result e = eval(df, leaf->fparms[2+po],x, it, m, p, c, s, d);
                QDate stop = earliest.addDays(e.number());

                spec.setDateRange(DateRange(start,stop));

            } else if (leaf->fparms.count() == (2+po) && Leaf::isNumber(df, leaf->fparms[1+po])) {

                // start to today
                Result b = eval(df, leaf->fparms[1+po],x, it, m, p, c, s, d);
                QDate start = earliest.addDays(b.number());
                QDate stop = QDate::currentDate();

                spec.setDateRange(DateRange(start,stop));

            } else {
                spec.setDateRange(d); // fallback to daterange selected
            }

            // get the cache, for the selected date range
            returning.asNumeric() =  RideFileCache::getAllBestsFor(m->context, leaf->seriesType, duration, spec);
            for(int i=0; i<returning.asNumeric().count(); i++) returning.number() += returning.asNumeric().at(i); // for sum

            return returning;

        }

        // meanmax array
        if (leaf->function == "meanmax") {

            Result returning(0);

            if (leaf->fparms.count() == 1 || leaf->fparms.count() == 3) { // retrieve from the ridefilecache, or aggregate across a date range
                                                                          // where the data range can be provided

                QString symbol = *(leaf->fparms[0]->lvalue.n);

                // go get it for the current date range
                if (leaf->fparms.count() == 1 && d.from==QDate() && d.to==QDate()) {

                    // the ride mean max
                    if (symbol == "efforts") {

                        // keep all from a ride -- XXX TODO averaging tails removal...
                        returning.asNumeric() = m->fileCache()->meanMaxArray(RideFile::watts);
                        for(int i=0; i<returning.asNumeric().count(); i++) returning.asNumeric()[i]=i;

                    } else returning.asNumeric() = m->fileCache()->meanMaxArray(leaf->seriesType);

                } else {

                    // default date range
                    QDate from=d.from, to=d.to;
                    QDate earliest(1900,01,01);

                    if (leaf->fparms.count() == 3) {
                        // get the date range
                        Result start =  eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
                        Result stop =  eval(df, leaf->fparms[2],x, it, m, p, c, s, d);

                        from = earliest.addDays(start.number());
                        to = earliest.addDays(stop.number());
                    }

                    // use a season meanmax
                    RideFileCache bestsCache(m->context, from, to, false, QStringList(), true, NULL);

                    // get meanmax, unless its efforts, where we do rather more...
                    if (symbol != "efforts") returning.asNumeric() = bestsCache.meanMaxArray(leaf->seriesType);

                    else {

                        // get power anyway
                        returning.asNumeric() = bestsCache.meanMaxArray(RideFile::watts);

                        // need more than 2 entries
                        if (returning.asNumeric().count() > 3) {

                            // we need to return an index to use to filter values
                            // in the meanmax array; remove sub-maximals by
                            // averaging tails or clearly submaximal using the
                            // same filter that is used in the CP plot

                            // get data to filter
                            QVector<double> t;
                            t.resize(returning.asNumeric().size());
                            for (int i=0; i<t.count(); i++) t[i]=i;

                            QVector<double> p = returning.asNumeric();
                            QVector<QDate> w = bestsCache.meanMaxDates(leaf->seriesType);
                            t.remove(0);
                            p.remove(0);
                            w.remove(0);


                            // linear regression of the full data, to help determine
                            // the maximal point on the MMP curve for each day
                            // using brace to set scope and descope temporary variables
                            // as we use a fair few, but not worth making a function
                            double slope=0, intercept=0;
                            {
                                // we want 2m to 20min data (check bounds)
                                int want = p.count() > 1200 ? 1200-121 : p.count()-121;
                                QVector<double> j = p.mid(120, want);
                                QVector<double> ts = t.mid(120, want);

                                // convert time data to seconds (is in minutes)
                                // and power to joules (power x time)
                                for(int i=0; i<j.count(); i++) {
                                    ts[i] = ts[i];
                                    j[i] = (j[i] * ts[i]) ;
                                }

                                // LTMTrend does a linear regression for us, lets reuse it
                                // I know, we see, to have a zillion ways to do this...
                                LTMTrend regress(ts.data(), j.data(), ts.count());

                                // save away the slope and intercept
                                slope = regress.slope();
                                intercept = regress.intercept();
                            }

                            // filter out efforts on same day that are the furthest
                            // away from a linear regression

                            // the best we found is stored in here
                            struct { int i; double p, t, d, pix; } keep;

                            for(int i=0; i<t.count(); i++) {

                                // reset our holding variable - it will be updated
                                // with the maximal point we want to retain for the
                                // day we are filtering for. Initial means no value
                                // has been set yet, so the first point will set it.
                                if (w[i] != QDate()) {

                                    // lets filter all on today, use first one to set the best found so far
                                    keep.d = (p[i] * t[i] ) - ((slope * t[i] ) + intercept);
                                    keep.i=i;
                                    keep.p=p[i];
                                    keep.t=t[i];
                                    keep.pix=powerIndex(keep.p, keep.t);

                                    // but clear since we iterate beyond
                                    if (i>0) { // always keep pmax point
                                        p[i]=0;
                                        t[i]=0;
                                    }

                                    // from here to the end of all the points, lets see if there is one further away?
                                    for(int z=i+1; z<t.count(); z++) {

                                        if (w[z] == w[i]) {

                                            // if its beloe the line multiply distance by -1
                                            double d = (p[z] * t[z] ) - ((slope * t[z]) + intercept);
                                            double pix = powerIndex(p[z],t[z]);

                                            // use the regression for shorter durations and 3p for longer
                                            if ((keep.t < 120 && keep.d < d) || (keep.t >= 120 && keep.pix < pix)) {
                                                keep.d = d;
                                                keep.i = z;
                                                keep.p = p[z];
                                                keep.t = t[z];
                                            }

                                            if (z>0) { // always keep pmax point
                                                w[z] = QDate();
                                                p[z] = 0;
                                                t[z] = 0;
                                            }
                                        }
                                    }

                                    // reinstate best we found
                                    p[keep.i] = keep.p;
                                    t[keep.i] = keep.t;
                                }
                            }

                            // so lets send over the indexes
                            // we keep t[0] as it gets removed in
                            // all cases below, saves a bit of
                            // torturous logic later
                            returning.asNumeric().clear();
                            returning.asNumeric() << 0;// it gets removed
                            for(int i=0; i<t.count(); i++)  if (t[i] > 0) returning.asNumeric() << i;
                        }
                    }
                }

                // really annoying that it starts from 0s not 1s, this is a legacy
                // bug that we cannot fix easily, but this is new, so lets not
                // have that damned 0s 0w entry!
                if (returning.asNumeric().count()>0) returning.asNumeric().remove(0);

                // compute the sum, ugh.
                for(int i=0; i<returning.asNumeric().count(); i++) returning.number() += returning.asNumeric()[i];

            } else if (leaf->fparms.count() == 2) { // calculate a meanmax curve using the passed x and y values

                Result xvector =  eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
                Result yvector =  eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

                // so first off, lets do a linear interpolation and resampling of
                // the t,y data to 1 second samples and remove negative values (set to 0)
                // needs gsl libraries but they will be mandatory dependencies
                // soon enough.
                // populate x and y by pulling x to start at 0
                // and setting y to have no negative values (truncate to zero)
                QVector<double>xdata;
                QVector<double>ydata;

                double offset=0;
                double maxx=0, lastx=0;
                bool interpolate=false;
                for(int i=0; i < xvector.asNumeric().count(); i++) {

                    // truncate y values if x values are missing
                    if (i >= yvector.asNumeric().count()) break;

                    // set the offset
                    if (i==0 && xvector.asNumeric().at(0) > 0) offset=xvector.asNumeric().at(0);

                    double xv = xvector.asNumeric().at(i) - offset;
                    if (xv >= 24*3600) break; // thats enough 24hr activity is long enough

                    if (xv >=0) {

                        // if gaps not 1s then we will need to interpolate
                        if (xv-lastx > 0) interpolate = true;

                        // we discard negative values of x
                        double yv = yvector.asNumeric().at(i);
                        if (yv <0) yv=0;

                        // add into x and y
                        xdata << xv;
                        ydata << yv;

                        if (xv > maxx) maxx = xv;
                    }

                    if (xv >0) lastx = xv;
                }

                // we now have xdata and ydata truncated to start at 0s
                // with no value longer than 24 hours and all y values removed
                // the data ranges from 0s to maxx seconds
                // time to interpolate (just in case)
                if (interpolate) {

                    // take a copy of the data since we will use it as
                    // working data for the gsl interpolation routines
                    // we are only going to do linear interpolation, if
                    // the user wants something fancier they should prepare
                    // the data themselves.

                    QVector<double> yp = ydata;

                    // setup the GSL interpolator (just linear for now)
                    gsl_interp *interpolation = gsl_interp_alloc (gsl_interp_linear,xdata.count());
                    gsl_interp_init(interpolation, xdata.constData(), yp.constData(), xdata.count());
                    gsl_interp_accel *accelerator =  gsl_interp_accel_alloc();

                    // truncate ydata as we will refill them
                    ydata.resize(0);
                    for(int i=0; i<=maxx; i++) {
                        // snaffle away- can place into input when GSL is no longer optional
                        ydata << gsl_interp_eval(interpolation, xdata.constData(), yp.constData(), i, accelerator);
                    }

                    // free the GSL interpolator
                    gsl_interp_free(interpolation);
                    gsl_interp_accel_free(accelerator);
                }

                // finally, we can call the meanmax computer - but need to use ints.. so
                // lets scale up by 1000 to save 3 decimal places, then scale down at the end
                QVector<int> input, bests, offsets; // offsets are discarded
                for(int i=0; i<ydata.count(); i++) input <<ydata[i]*double(1000);

                // lets do it...
                RideFileCache::fastSearch(input, bests, offsets);

                // now truncate back to 3 dps
                // we start at pos 1 (1s value) a really old bug in the way mmp data is
                // stored and shared (0th element in arrayis for 0s) that is baked in to
                // lots of other charts, and now here too :)
                for(int i=1; i<bests.count()-1; i++) {
                    double value = double(bests[i])/double(1000);
                    if (value >0) {
                        returning.asNumeric() << value;
                        returning.number() += value;
                    }
                }
            }

            // return a vector
            return returning;
        }

        // interpolation
        if (leaf->function == "interpolate") {

            // interpolate(algo, xvector, yvector, xvalues) - returns yvalues for each xvalue
            Result returning(0);

            // unpack parameters
            QString algo= *(leaf->fparms[0]->lvalue.n);
            Result xvector =  eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            Result yvector =  eval(df, leaf->fparms[2],x, it, m, p, c, s, d);
            Result xvalues =  eval(df, leaf->fparms[3],x, it, m, p, c, s, d);

            int n = yvector.asNumeric().count() < xvector.asNumeric().count() ? yvector.asNumeric().count() : xvector.asNumeric().count();

            if (n >2) {

                // ok, so now lets setup
                gsl_interp *interpolation = NULL;
                if (algo == "akima") interpolation = gsl_interp_alloc (gsl_interp_akima,n);
                if (algo == "cubic") interpolation = gsl_interp_alloc (gsl_interp_cspline,n);
                else if (algo == "steffen") interpolation = gsl_interp_alloc (gsl_interp_akima,n);
                else interpolation = gsl_interp_alloc (gsl_interp_linear,n); // linear is the fallback, always

                gsl_interp_init(interpolation, xvector.asNumeric().constData(), yvector.asNumeric().constData(), n);
                gsl_interp_accel *accelerator =  gsl_interp_accel_alloc();

                // truncate ydata as we will refill them
                for(int i=0; i<xvalues.asNumeric().count(); i++) {

                    // snaffle away- can place into input when GSL is no longer optional
                    double value = gsl_interp_eval(interpolation, xvector.asNumeric().constData(),
                                                   yvector.asNumeric().constData(), xvalues.asNumeric().at(i), accelerator);
                    returning.asNumeric() << value;
                    returning.number() += value;
                }

                // free the GSL interpolator
                gsl_interp_free(interpolation);
                gsl_interp_accel_free(accelerator);
            }
            return returning;
        }


        if (leaf->function == "resample") {
#ifdef GC_HAVE_SAMPLERATE

            Result returning(0);

            // resample(from, to, yvector)
            // we return yvector resampled
            double from =  eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number();
            double to =   eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number();
            Result y =eval(df, leaf->fparms[2],x, it, m, p, c, s, d);

            // if in doubt just return it unchanged.
            if (y.asNumeric().count() < 3 || from <= 0 || to <= 0 || from == to) return y;

            // lets prepare data for libsamplerate
            SRC_DATA data;

            float *input, *output, *source, *target;
            float insamples = y.asNumeric().count();
            float outsamples = 1+ ((y.asNumeric().count() * from) / to); // 1+ for rounding up

            // allocate memory
            source = input = (float*)malloc(sizeof(float) * insamples);
            target = output = (float*)malloc(sizeof(float) * outsamples);

            // create the input array (float not double)
            for(int i=0; i<y.asNumeric().count(); i++)  *source++ = float(y.asNumeric().at(i));

            //
            // THE MAGIC HAPPENS HERE ... resample to new recording interval
            //
            data.src_ratio = from / to;
            data.data_in = input;
            data.input_frames = y.asNumeric().count();
            data.data_out = output;
            data.output_frames = outsamples;
            data.input_frames_used = 0;
            data.output_frames_gen = 0;
            int ret = src_simple(&data, SRC_LINEAR, 1);

            if (ret) { // failed

                free(input);
                free(output);
                return y; // return unchanged

            } else { // success, lets unpack

                // unpack the data series
                for(int frame=0; frame < data.output_frames_gen; frame++) {

                    double value = *target++;
                    returning.asNumeric() << value;
                    returning.number() += value;
                }
            }

            // free memory
            free(input);
            free(output);
            return returning; // resampled !
#else
            return Result(-1); // nothing resampled
#endif
        }

        // distribution
        if (leaf->function == "dist") {

            Result returning(0);

            // get the two symbols
            QString want = *(leaf->fparms[1]->lvalue.n);


            // working with an activity
            if (d.from==QDate() && d.to==QDate()) {

                if (want == "bins") {

                    int length = m->fileCache()->distributionArray(leaf->seriesType).count();
                    double delta = RideFileCache::binsize(leaf->seriesType);
                    for (double it=0; it <length; it++) {
                        returning.asNumeric() << delta * it;
                        returning.number() += delta *it;
                    }

                } else {

                    // the ride mean max
                    returning.asNumeric() = m->fileCache()->distributionArray(leaf->seriesType);
                    for(int i=0; i<returning.asNumeric().count(); i++) returning.number() += returning.asNumeric().at(i);
                }

            } else {

                RideFileCache bestsCache(m->context, d.from, d.to, false, QStringList(), true, NULL);
                if (want == "bins") {

                    int length = bestsCache.distributionArray(leaf->seriesType).count();
                    double delta = RideFileCache::binsize(leaf->seriesType);
                    for (double it=0; it <length; it++) {
                        returning.asNumeric() << delta * it;
                        returning.number() += delta *it;
                    }

                } else {
                    // working with a date range
                    returning.asNumeric() = bestsCache.distributionArray(leaf->seriesType);
                    for(int i=0; i<returning.asNumeric().count(); i++) returning.number() += returning.asNumeric().at(i);
                }
            }
            return returning;
        }

        // argsort
        if (leaf->function == "argsort") {
            Result returning(0);

            // ascending or descending?
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            bool ascending= (symbol=="ascend") ? true : false;

            // use the utils function to actually do it
            Result v = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            QVector<int> r;
            if (v.isNumber) r = Utils::argsort(v.asNumeric(), ascending);
            else r = Utils::argsort(v.asString(), ascending);

            // put the index into the result we are returning.
            foreach(int x, r) {
                returning.asNumeric() << static_cast<double>(x);
                returning.number() += x;
            }
            return returning;
        }

        // rank
        if (leaf->function == "rank") {
            Result returning(0);

            // ascending or descending?
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            bool ascending= (symbol=="ascend") ? true : false;

            // use the utils function to actually do it
            Result v = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            QVector<int> r = Utils::rank(v.asNumeric(), ascending);

            // put the index into the result we are returning.
            foreach(int x, r) {
                returning.number() += x;
                returning.asNumeric() << static_cast<double>(x);
            }

            return returning;
        }

        // sort
        if (leaf->function == "sort") {
            Result returning(0);

            // ascending or descending?
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            bool ascending= (symbol=="ascend") ? true : false;

            // use the utils function to actually do it
            Result v = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            returning.isNumber = v.isNumber;

            if (v.isNumber) {
                if (ascending) std::sort(v.asNumeric().begin(), v.asNumeric().end(), Utils::doubleascend);
                else std::sort(v.asNumeric().begin(), v.asNumeric().end(), Utils::doubledescend);
            } else {
                if (ascending) std::sort(v.asString().begin(), v.asString().end(), Utils::qstringascend);
                else std::sort(v.asString().begin(), v.asString().end(), Utils::qstringdescend);
            }

            // put the index into the result we are returning.
            if (v.isNumber) {
                foreach(double x, v.asNumeric()) {
                    returning.number() += x;
                    returning.asNumeric() << x;
                }
            } else {
                returning.asString() = v.asString();
            }

            return returning;
        }

        // uniq - returns vector
        if (leaf->function == "uniq") {


            // evaluate all the lists
            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            QVector<int> index = v.isNumber ? Utils::arguniq(v.asNumeric()) : Utils::arguniq(v.asString());

            Result returning(0);
            returning.isNumber = v.isNumber;

            for(int idx=0; idx<index.count(); idx++) {

                if (v.isNumber) {
                    double value = v.asNumeric()[index[idx]];
                    returning.asNumeric() << value;
                    returning.number() += value;
                } else {
                    QString value = v.asString()[index[idx]];
                    returning.asString() << value;
                }
            }

            return returning;
        }

        // arguniq
        if (leaf->function == "arguniq") {
            Result returning(0);

            // get vector and an argsort
            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            QVector<int> r;
            if (v.isNumber) r = Utils::arguniq(v.asNumeric());
            else r = Utils::arguniq(v.asString());

            for(int i=0; i<r.count(); i++) {
                returning.asNumeric() << r[i];
                returning.number() += r[i];
            }
            return returning;
        }

        // multiuniq
        if (leaf->function == "multiuniq") {

            // evaluate all the lists
            for(int i=0; i<leaf->fparms.count(); i++) eval(df, leaf->fparms[i],x, it, m, p, c, s, d);

            // get first and argsort it
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            Result current = df->symbols.value(symbol);
            long len = current.isNumber ? current.asNumeric().count() : current.asString().count();
            QVector<int> index = current.isNumber ? Utils::arguniq(current.asNumeric()) : Utils::arguniq(current.asString());

            // sort all the lists in place
            int count=0;
            for (int i=0; i<leaf->fparms.count(); i++) {
                // get the vector
                symbol = *(leaf->fparms[i]->lvalue.n);
                Result current = df->symbols.value(symbol);

                // diff length?
                if ((current.isNumber && current.asNumeric().count() != len) || (!current.isNumber && current.asString().count() != len)) {
                    fprintf(stderr, "multiuniq list '%s': not the same length, ignored\n", symbol.toStdString().c_str()); fflush(stderr);
                    continue;
                }

                // ok so now we can adjust
                if (current.isNumber) {
                    QVector<double> replace;
                    for(int idx=0; idx<index.count(); idx++) replace << current.asNumeric()[index[idx]];
                    current.asNumeric() = replace;
                } else {
                    QVector<QString> replace;
                    for(int idx=0; idx<index.count(); idx++) replace << current.asString()[index[idx]];
                    current.asString() = replace;
                }

                // replace
                df->symbols.insert(symbol, current);

                count++;
            }
            return Result(count);
        }

        // store/fetch from athlete storage
        if (leaf->function == "store") {
            // name and value in parameter 1 and 2
            QString name= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).string();
            Result value= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            // store it away
            m->context->athlete->dfcache.insert(name, value);

            return Result(0);
        }

        if (leaf->function == "fetch") {
            // name and value in parameter 1 and 2
            QString name= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).string();

            // store it away
            Result returning = m->context->athlete->dfcache.value(name, Result(0));

            return returning;
        }

        // access user chart curve data, if it's there
        if (leaf->function == "curve") {

            // not on a chart m8
            if (df->chart == NULL) return Result(0);

            Result returning(0);

            // lets see if we can find the series
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            for(int i=0; i<df->chart->seriesinfo.count(); i++) {
                if (df->chart->seriesinfo[i].name == symbol) {
                    // woop!
                    QString data=*(leaf->fparms[1]->lvalue.n);
                    if (data == "x") returning.asNumeric() = df->chart->seriesinfo[i].xseries;
                    if (data == "y") returning.asNumeric() = df->chart->seriesinfo[i].yseries;
                    // z d t still todo XXX
                }
            }
            return returning;
        }

        // lowerbound
        if (leaf->function == "lowerbound") {

            Result returning(-1);

            Result list= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result value= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            // empty list - error
            if (!list.isVector()) return returning;

            if (list.isNumber) {
                // lets do it with std::lower_bound then
                QVector<double>::const_iterator i = std::lower_bound(list.asNumeric().begin(), list.asNumeric().end(), value.number(), Utils::comparedouble());
                if (i == list.asNumeric().end()) return Result(list.asNumeric().size());
                return Result(i - list.asNumeric().begin());
            } else {
                QVector<QString>::const_iterator i = std::lower_bound(list.asString().begin(), list.asString().end(), value.string(), Utils::compareqstring());
                if (i == list.asString().end()) return Result(list.asNumeric().size());
                return Result(i - list.asString().begin());
            }
        }

        // random
        if (leaf->function == "random") {

            int n= eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number(); // how many ?
            Result returning(0);

            // Random number function based on the GNU Scientific Library
            while(n>0) {
                double random = gsl_rng_uniform(df->owner->r); // Generate it!
                returning.number() += random;
                returning.asNumeric() << random;
                n--;
            }
            return returning;

        }

        // quantile
        if (leaf->function == "quantile") {

            Result         v= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result quantiles= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            Result returning(0);

            if (v.asNumeric().count() > 0) {
                // sort the vector first
                std::sort(v.asNumeric().begin(), v.asNumeric().end());

                if (quantiles.asNumeric().count() ==0) {
                    double quantile = quantiles.number();
                    if (quantile < 0) quantile=0;
                    if (quantile > 1) quantile=1;

                    double value = gsl_stats_quantile_from_sorted_data(v.asNumeric().constData(), 1, v.asNumeric().count(), quantile);
                    returning.number() = value;

                } else {
                    for (int i=0; i<quantiles.asNumeric().count(); i++) {
                        double quantile= quantiles.asNumeric().at(i);
                        if (quantile < 0) quantile=0;
                        if (quantile > 1) quantile=1;
                        double value = gsl_stats_quantile_from_sorted_data(v.asNumeric().constData(), 1, v.asNumeric().count(), quantile);
                        returning.number() += value;
                        returning.asNumeric() << value;
                    }
                }
            }
            return returning;
        }

        // sort
        if (leaf->function == "multisort") {

            // ascend/descend?
            QString symbol = *(leaf->fparms[0]->lvalue.n);
            bool ascending=symbol=="ascend" ? true : false;

            // evaluate all the lists
            for(int i=1; i<fparms.count(); i++) eval(df, leaf->fparms[i],x, it, m, p, c, s, d);

            // get first and argsort it
            symbol = *(leaf->fparms[1]->lvalue.n);
            Result current = df->symbols.value(symbol);
            long len = current.isNumber ? current.asNumeric().count() : current.asString().count();
            QVector<int> index = current.isNumber ? Utils::argsort(current.asNumeric(), ascending) : Utils::argsort(current.asString(), ascending);

            // sort all the lists in place
            int count=0;
            for (int i=1; i<leaf->fparms.count(); i++) {
                // get the vector
                symbol = *(leaf->fparms[i]->lvalue.n);
                Result current = df->symbols.value(symbol);

                // diff length?
                if ((current.isNumber && current.asNumeric().count() != len) || (!current.isNumber && current.asString().count() != len)) {
                    fprintf(stderr, "multisort list '%s': not the same length, ignored\n", symbol.toStdString().c_str()); fflush(stderr);
                    continue;
                }

                // ok so now we can adjust
                if (current.isNumber) {
                    QVector<double> replace = current.asNumeric();
                    for(int idx=0; idx<index.count(); idx++) replace[idx] = current.asNumeric()[index[idx]];
                    current.asNumeric() = replace;
                } else {
                    QVector<QString> replace = current.asString();
                    for(int idx=0; idx<index.count(); idx++) replace[idx] = current.asString()[index[idx]];
                    current.asString() = replace;
                }

                // replace
                df->symbols.insert(symbol, current);

                count++;
            }
            return Result(count);
        }

        if (leaf->function == "head") {

            Result returning(0);

            Result list= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result count= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            int n=count.number();
            if (list.isNumber && n > list.asNumeric().count()) n=list.asNumeric().count();
            if (!list.isNumber && n > list.asString().count()) n=list.asString().count();

            if (n<=0) return Result(0);// nope

            if (list.isNumber) {
                returning.asNumeric() = list.asNumeric().mid(0, n);
                for(int i=0; i<returning.asNumeric().count(); i++) returning.number() += returning.asNumeric()[i];
            } else {
                returning.asString() = list.asString().mid(0, n);
                returning.isNumber = false;
            }

            return returning;
        }

        if (leaf->function == "tail") {

            Result returning(0);

            Result list= eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result count= eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
            int n=count.number();
            if (list.isNumber && n > list.asNumeric().count()) n=list.asNumeric().count();
            if (!list.isNumber && n > list.asString().count()) n=list.asString().count();

            if (n<=0) return Result(0);// nope

            if (list.isNumber) {
                returning.asNumeric() = list.asNumeric().mid(list.asNumeric().count()-n, n);
                for(int i=0; i<returning.asNumeric().count(); i++) returning.number() += returning.asNumeric()[i];
            } else {
                returning.asString() = list.asString().mid(list.asString().count()-n, n);
                returning.isNumber = false;
            }

            return returning;
        }

        // sapply
        if (leaf->function == "sapply") {
            Result returning(0);

            Result value = eval(df,leaf->fparms[0],x, it, m, p, c, s, d); // lhs might also be a symbol
            returning.isNumber = value.isNumber;

            // need a vector, always
            if (!value.isVector()) return returning;

            // loop and evaluate, non-zero we keep, zero we lose
            for(int i=0; (value.isNumber && i<value.asNumeric().count()) || (!value.isNumber && i<value.asString().count()); i++) {
                Result x;
                if (value.isNumber) x = Result(value.asNumeric().at(i));
                else x = Result(value.asString().at(i));
                Result r = eval(df,leaf->fparms[1],x, i, m, p, c, s, d);

                // we want it
                if (value.isNumber) {
                    returning.asNumeric() << r.number();
                    returning.number() += r.number();
                } else {
                    returning.asString() << r.string();
                }
            }
            return returning;
        }

        // match
        if (leaf->function == "match") {

            // for every value in vector 1 return the index for it
            // in vector 2, if it is not there then it will not be
            // included in the returned index

            Result returning(0);

            Result v1 = eval(df,leaf->fparms[0],x, it, m, p, c, s, d); // lhs might also be a symbol
            Result v2 = eval(df,leaf->fparms[1],x, it, m, p, c, s, d); // lhs might also be a symbol

            // should be same type
            if (v1.isNumber != v2.isNumber) return returning;

            // lets search
            if (v1.isNumber) {
                for(int i=0; i<v1.asNumeric().count(); i++) {
                    double find = v1.asNumeric()[i];
                    for(int i2=0; i2<v2.asNumeric().count(); i2++) {
                        if (v2.asNumeric()[i2] == find) {
                            returning.number() += i2;
                            returning.asNumeric() << i2;
                            break;
                        }
                    }
                }
            } else {
                for(int i=0; i<v1.asString().count(); i++) {
                    QString find = v1.asString()[i];
                    for(int i2=0; i2<v2.asString().count(); i2++) {
                        if (v2.asString()[i2] == find) {
                            returning.asNumeric() << i2;
                            break;
                        }
                    }
                }
            }
            return returning;
        }

        // non-zero - return index to non-zero values
        if (leaf->function == "nonzero") {

            Result returning(0);
            Result v = eval(df,leaf->fparms[0],x, it, m, p, c, s, d); // lhs might also be a symbol

            if (v.asNumeric().count() > 0) {
                for (int i=0; i < v.asNumeric().count(); i++) {
                    if (v.asNumeric()[i] != 0) {
                        returning.asNumeric() << i;
                        returning.number() += i;
                    }
                }
            }

            return returning;
        }

        // annotate
        if (leaf->function == "annotate") {

            QString type = *(leaf->fparms[0]->lvalue.n);

            if (type == "label") {

                QStringList list;

                // loop through parameters
                for(int i=1; i<leaf->fparms.count(); i++) {

                    if (leaf->fparms[i]->type == Leaf::String) {

                        // a string
                        list << *(leaf->fparms[i]->lvalue.s);
                    } else {

                        // evaluate expression to get value/vector
                        Result value = eval(df,leaf->fparms[i],x, it, m, p, c, s, d);
                        if (value.isNumber) {
                            if (value.isVector()) {
                                for(int ii=0; ii<value.asNumeric().count(); ii++)
                                    list << Utils::removeDP(QString("%1").arg(value.asNumeric().at(ii)));
                            } else {
                                list << Utils::removeDP(QString("%1").arg(value.number()));
                            }
                        } else {
                            if (value.isVector()) {
                                for(int ii=0; ii<value.asString().count(); ii++)
                                    list << value.asString().at(ii);
                            } else {
                                list << value.string();
                            }
                        }
                    }
                }

                // send the signal.
                if (list.count())  {
                    GenericAnnotationInfo label(GenericAnnotationInfo::Label);
                    label.labels = list;
                    df->owner->annotate(label);
                }
            }

            if (type == "voronoi") {
                Result centers = eval(df,leaf->fparms[1],x, it, m, p, c, s, d);

                if (centers.isVector() && centers.isNumber && centers.asNumeric().count() >=2 && centers.asNumeric().count()%2 == 0) {

                    GenericAnnotationInfo voronoi(GenericAnnotationInfo::Voronoi);
                    int n=centers.asNumeric().count()/2;
                    for(int i=0; i<n; i++) {
                        voronoi.vx << centers.asNumeric()[i];
                        voronoi.vy << centers.asNumeric()[i+n];
                    }

                    // send signal
                    df->owner->annotate(voronoi);
                }
            }

            if (type == "hline" || type == "vline") {

                GenericAnnotationInfo line(type == "vline" ? GenericAnnotationInfo::VLine : GenericAnnotationInfo::HLine);
                line.text =  eval(df,leaf->fparms[1],x, it, m, p, c, s, d).string();
                line.linestyle = linestyle(*(leaf->fparms[2]->lvalue.n));
                line.value = eval(df,leaf->fparms[3],x, it, m, p, c, s, d).number();

                // send signal
                df->owner->annotate(line);
            }

            if (type == "lr") {
                GenericAnnotationInfo lr(GenericAnnotationInfo::LR);
                lr.linestyle = linestyle(*(leaf->fparms[1]->lvalue.n));
                lr.color = eval(df,leaf->fparms[2],x, it, m, p, c, s, d).string();

                // send signal
                df->owner->annotate(lr);
            }

        }

        // smooth
        if (leaf->function == "smooth") {

            Result returning(0);

            // moving average
            if (*(leaf->fparms[1]->lvalue.n) == "sma") {

                QString type =  *(leaf->fparms[2]->lvalue.n);
                int window = eval(df,leaf->fparms[3],x, it, m, p, c, s, d).number();
                Result data = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
                int pos=2; // fallback

                if (type=="backward") pos=0;
                if (type=="forward") pos=1;
                if (type=="centered") pos=2;

                returning.asNumeric() = Utils::smooth_sma(data.asNumeric(), pos, window);

            } else if (*(leaf->fparms[1]->lvalue.n) == "ewma") {

                // exponentially weighted moving average
                double alpha = eval(df,leaf->fparms[2],x, it, m, p, c, s, d).number();
                Result data = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);

                returning.asNumeric() = Utils::smooth_ewma(data.asNumeric(), alpha);
            }

            // sum. ugh.
            for(int i=0; i<returning.asNumeric().count(); i++) returning.number() += returning.asNumeric()[i];

            return returning;
        }

        // levenberg-marquardt nls
        if (leaf->function == "lm") {
            Result returning(0);
            returning.asNumeric() << 0 << -1 << -1 ; // assume failure

            Leaf *formula = leaf->fparms[0];
            Result xv = eval(df,leaf->fparms[1],x, it, m, p, c, s, d);
            Result yv = eval(df,leaf->fparms[2],x, it, m, p, c, s, d);

            // check ok to proceed
            if (xv.asNumeric().count() < 3 || xv.asNumeric().count() != yv.asNumeric().count()) return returning;

            // use the power duration model using for a data filter expression
            DFModel model(m, formula, df);
            bool success = model.fitData(xv.asNumeric(), yv.asNumeric());

            if (success) {
                // first  entry is sucess
                returning.asNumeric()[0] = 1;

                // second entry is RMSE
                double sume2=0, sum=0;
                for(int index=0; index<xv.asNumeric().count(); index++) {
                    double predict = eval(df,formula, Result(xv.asNumeric()[index]), 0, m, p, c, s, d).number();
                    double actual = yv.asNumeric()[index];
                    double error = predict - actual;
                    sume2 +=  pow(error, 2);
                    sum += predict;
                }
                double mean = sum / xv.asNumeric().count();

                // RMSE
                returning.asNumeric()[1] = sqrt(sume2 / double(xv.asNumeric().count()-2));

                // CV
                returning.asNumeric()[2] = (returning.asNumeric()[1] / mean) * 100.0;
                //fprintf(stderr, "RMSE=%f, CV=%f%% \n", returning.vector[1], returning.vector[2]); fflush(stderr);

            }

            return returning;
        }

        // linear regression
        if (leaf->function == "lr") {
            Result returning(0);
            returning.asNumeric() << 0 << 0 << 0 << 0; // set slope, intercept, r2 and see to 0

            Result xv = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
            Result yv = eval(df,leaf->fparms[1],x, it, m, p, c, s, d);

            // check ok to proceed
            if (xv.asNumeric().count() < 2 || xv.asNumeric().count() != yv.asNumeric().count()) return returning;

            // use the generic calculator, its quick and easy
            GenericCalculator calc;
            calc.initialise();
            for (int i=0; i< xv.asNumeric().count(); i++)
                calc.addPoint(QPointF(xv.asNumeric()[i], yv.asNumeric()[i]));
            calc.finalise();

            // extract LR results
            returning.asNumeric()[0]=calc.m;
            returning.asNumeric()[1]=calc.b;
            returning.asNumeric()[2]=calc.r2;
            returning.asNumeric()[3]=calc.see;
            returning.number() = calc.m + calc.b + calc.r2 + calc.see; // sum

            return returning;
        }

        if (leaf->function == "mlr") {

            // return
            Result returning(0);

            // get y vector
            Result yv = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);

            int n = yv.asNumeric().count();
            int xn = leaf->fparms.count()-1; // first parm is yvector
            gsl_matrix *X = gsl_matrix_calloc(n, xn);
            gsl_vector *Y = gsl_vector_alloc(n);
            gsl_vector *coeff = gsl_vector_alloc(xn); // the coefficients we want to return

            // setup the y vector
            for (int i = 0; i < n; i++) gsl_vector_set(Y, i, yv.asNumeric()[i]);

            // populate the x matrix, 1 column per predictor, n rows of datavalues
            // if xvector is too small, we pad with 0 values - no repeating here ?fix later?
            for (int xi=1; xi<leaf->fparms.count(); xi++) {
                Result xv = eval(df,leaf->fparms[xi],x, it, m, p, c, s, d);
                for (int i=0; i < n; i++) {
                    double value=0;
                    if (i < xv.asNumeric().count()) value= xv.asNumeric()[i];
                    gsl_matrix_set(X, i, xi-1, value);
                }
            }

            double chisq;
            gsl_matrix *cov = gsl_matrix_alloc(xn, xn);
            gsl_multifit_linear_workspace * wspc = gsl_multifit_linear_alloc(n, xn);
            gsl_multifit_linear(X, Y, coeff, cov, &chisq, wspc);

            // snaffle away the coeefficents, we discard chi-squared and the
            // covariance matrix for now, may look to pass them back later
            for (int i = 0; i < xn; i++) {
                double value= gsl_vector_get(coeff, i);
                returning.asNumeric() << value;
                returning.number() += value;
            }

            gsl_matrix_free(X);
            gsl_matrix_free(cov);
            gsl_vector_free(Y);
            gsl_vector_free(coeff);
            gsl_multifit_linear_free(wspc);

            return returning;
        }

        // stddev
        if (leaf->function == "variance") {
            // array
            Result v = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
            Statistic calc;
            return calc.variance(v.asNumeric(), v.asNumeric().count());
        }

        if (leaf->function == "stddev") {
            // array
            Result v = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
            Statistic calc;
            return calc.standarddeviation(v.asNumeric(), v.asNumeric().count());
        }

        // pmc
        if (leaf->function == "pmc") {

            if (d.from==QDate() || d.to==QDate()) return Result(0);

            QString series = *(leaf->fparms[1]->lvalue.n);
            QDateTime earliest(QDate(1900,01,01),QTime(0,0,0));
            PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df); // use default days
            Result returning(0);
            int  si=0;

            for(QDate date=pmcData->start(); date < pmcData->end(); date=date.addDays(1)) {
                // index
                if (date >= d.from && date <= d.to) {
                    double value=0;

                    // lets copy into our array
                    if (series == "date") value = earliest.daysTo(QDateTime(date, QTime(0,0,0)));
                    if (series == "lts") value = pmcData->lts()[si];
                    if (series == "stress") value = pmcData->stress()[si];
                    if (series == "sts") value = pmcData->sts()[si];
                    if (series == "rr") value = pmcData->rr()[si];
                    if (series == "sb") value = pmcData->sb()[si];

                    returning.asNumeric() << value;
                    returning.number() += value;
                }

                si++;
            }
            return returning;
        }

        // banister
        if (leaf->function == "banister") {
            Leaf *first=leaf->fparms[0];
            Leaf *second=leaf->fparms[1];
            Leaf *third=leaf->fparms[2];

            // check metric name is valid
            QString metric = df->lookupMap.value(first->signature(), "");
            QString perf_metric = df->lookupMap.value(second->signature(), "");
            QString value = third->signature();
            Banister *banister = m->context->athlete->getBanisterFor(metric, perf_metric, 0,0);
            QDateTime earliest(QDate(1900,01,01),QTime(0,0,0));

            // prepare result
            Result returning(0);
            int  si=0;

            for(QDate date=banister->start; date < banister->stop; date=date.addDays(1)) {
                // index
                if (date >= d.from && date <= d.to) {
                    double x=0;

                    // lets copy into our array
                    if (value == "nte") x =banister->data[si].nte;
                    if (value == "pte") x =banister->data[si].pte;
                    if (value == "perf") x =banister->data[si].perf;
                    if (value == "cp") x = banister->data[si].perf ? (banister->data[si].perf * 261.0 / 100.0) : 0;
                    if (value == "date") x = earliest.daysTo(QDateTime(date, QTime(0,0,0)));

                    returning.asNumeric() << x;
                    returning.number() += x;
                }

                si++;
            }
            return returning;
        }

        // date handling functions
        if (leaf->function == "week") {

            // convert number or vector of dates to weeks since 1900
            QDate earliest(1900,01,01);
            Result returning(0);

            if (leaf->fparms.count() != 1) return returning;

            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (v.asNumeric().count()) {
                for(int i=0; i<v.asNumeric().count(); i++) {
                    double value = std::floor(earliest.daysTo(earliest.addDays(v.asNumeric()[i])) / 7.0);
                    returning.number() += value; // for sum
                    returning.asNumeric() << value;
                }
            } else {
                returning.number() = std::floor(earliest.daysTo(earliest.addDays(v.number())) / 7.0);
            }

            return returning;
        }

        if (leaf->function == "weekdate") {

            // convert number or vector of dates to weeks since 1900
            QDate earliest(1900,01,01);
            Result returning(0);

            if (leaf->fparms.count() != 1) return returning;

            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (v.asNumeric().count()) {
                for(int i=0; i<v.asNumeric().count(); i++) {
                    double value = std::floor(earliest.daysTo(earliest.addDays(v.asNumeric()[i]* 7.0)));
                    returning.number() += value; // for sum
                    returning.asNumeric() << value;
                }
            } else {
                returning.number() = std::floor(earliest.daysTo(earliest.addDays(v. number()* 7.0)));
            }

            return returning;
        }
        if (leaf->function == "month") {

            // convert number or vector of dates to weeks since 1900
            QDate earliest(1900,01,01);
            Result returning(0);

            if (leaf->fparms.count() != 1) return returning;

            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (v.asNumeric().count()) {
                for(int i=0; i<v.asNumeric().count(); i++) {
                    double value = std::floor(monthsTo(earliest, earliest.addDays(v.asNumeric()[i])));
                    returning.number() += value; // for sum
                    returning.asNumeric() << value;
                }
            } else {
                returning.number() = std::floor(monthsTo(earliest, earliest.addDays(v.number())));
            }

            return returning;
        }

        if (leaf->function == "monthdate") {

            // convert number or vector of dates to weeks since 1900
            QDate earliest(1900,01,01);
            Result returning(0);

            if (leaf->fparms.count() != 1) return returning;

            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (v.asNumeric().count()) {
                for(int i=0; i<v.asNumeric().count(); i++) {
                    QDate dd = earliest.addMonths(v.asNumeric()[i]);
                    double value = earliest.daysTo(QDate(dd.year(), dd.month(), 1));
                    returning.number() += value; // for sum
                    returning.asNumeric() << value;
                }
            } else {
                QDate dd = earliest.addMonths(v.number());
                returning.number() = earliest.daysTo(QDate(dd.year(), dd.month(), 1));
            }

            return returning;
        }

        // powerindex(power,duration) - return value or vector of values translating to powerindex
        if (leaf->function == "powerindex") {

            Result returning(0);
            Result power = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            Result duration = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

            if (power.isVector() && duration.isVector()) {
                // return a vector
                QVector<double> powerindexes;
                for(int i=0; i<power.asNumeric().count() && i<duration.asNumeric().count(); i++)
                    powerindexes << powerIndex(power.asNumeric().at(i), duration.asNumeric().at(i), false);
                returning = Result(powerindexes);
            } else {
                // return a value
                returning = Result(powerIndex(power.number(), duration.number(), false));
            }
            return returning;
        }

        // get here for tiz and best
        if (leaf->function == "best" || leaf->function == "tiz") {

            switch (leaf->lvalue.l->type) {

                default:
                case Leaf::Function :
                {
                    duration = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d).number(); // duration will be zero if string
                }
                break;

                case Leaf::Symbol :
                {
                    QString rename;
                    // get symbol value
                    if (df->lookupType.value(*(leaf->lvalue.l->lvalue.n)) == true) {
                        // numeric
                        if (c) duration = RideMetric::getForSymbol(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""), c);
                        else duration = m->getForSymbol(rename=df->lookupMap.value(*(leaf->lvalue.l->lvalue.n),""));
                    } else if (*(leaf->lvalue.l->lvalue.n) == "x") {
                        duration = Result(x).number();
                    } else if (*(leaf->lvalue.l->lvalue.n) == "i") {
                        duration = it;
                    } else {
                        duration = 0;
                    }
                }
                break;

                case Leaf::Float :
                    duration = leaf->lvalue.l->lvalue.f;
                    break;

                case Leaf::Integer :
                    duration = leaf->lvalue.l->lvalue.i;
                    break;

                case Leaf::String :
                    duration = (leaf->lvalue.l->lvalue.s)->toDouble();
                    break;

                break;
            }

            if (leaf->function == "best")
                return Result(RideFileCache::best(m->context, m->fileName, leaf->seriesType, duration));

            if (leaf->function == "tiz") // duration is really zone number
                return Result(RideFileCache::tiz(m->context, m->fileName, leaf->seriesType, duration));
        }

        if (leaf->function == "round") {
            // round(expr) or round(expr, dp)
            Result returning(0);

            double factor = 1; // 0 decimal places
            if (leaf->fparms.count() == 2) {
                Result dpv = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);
                factor=pow(10, dpv.number()); // multiply by then divide
            }

            Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
            if (v.asNumeric().count()) {
                for(int i=0; i<v.asNumeric().count(); i++) {
                    double r = round(v.asNumeric()[i]*factor)/factor;
                    returning.asNumeric() << r;
                    returning.number() += r;
                }
            } else {
                returning.number() =  round(v.number()*factor)/factor;
            }
            return returning;
        }

        // if we get here its general function handling
        // what function is being called?
        int fnum=-1;
        for (int i=0; DataFilterFunctions[i].parameters != -1; i++) {
            if (DataFilterFunctions[i].name == leaf->function) {

                // parameter mismatch not allowed; function signature mismatch
                // should be impossible...
                if (DataFilterFunctions[i].parameters && DataFilterFunctions[i].parameters != leaf->fparms.count())
                    fnum=-1;
                else
                    fnum = i;
                break;
            }
        }

        // not found...
        if (fnum < 0) return Result(0);

        switch (fnum) {
            case 0 : case 1 : case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:
            case 11 : case 12: case 13: case 14: case 15: case 16: case 18: case 19: case 20:
            {
                Result returning(0);


                // TRIG FUNCTIONS

                // bit ugly but cleanest way of doing this without repeating
                // looping stuff - we use a function pointer to save that...
                double (*func)(double);
                switch (fnum) {
                default:
                case 0: func = cos; break;
                case 1 : func = tan; break;
                case 2 : func = sin; break;
                case 3 : func = acos; break;
                case 4 : func = atan; break;
                case 5 : func = asin; break;
                case 6 : func = cosh; break;
                case 7 : func = tanh; break;
                case 8 : func = sinh; break;
                case 9 : func = acosh; break;
                case 10 : func = atanh; break;
                case 11 : func = asinh; break;

                case 12 : func = exp; break;
                case 13 : func = log; break;
                case 14 : func = log10; break;

                case 15 : func = ceil; break;
                case 16 : func = floor; break;

                case 18 : func = fabs; break;
                case 19 : func = Utils::myisinf; break;
                case 20 : func = Utils::myisnan; break;
                }

                Result v = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
                if (v.asNumeric().count()) {
                    for(int i=0; i<v.asNumeric().count(); i++) {
                        double r = func(v.asNumeric()[i]);
                        returning.asNumeric() << r;
                        returning.number() += r;
                    }
                } else {
                    returning.number() =  func(v.number());
                }
                return returning;
            }
            break;



        case 21 : { /* SUM( ... ) */
                    double sum=0;

                    foreach(Leaf *l, leaf->fparms) {
                        sum += eval(df, l,x, it, m, p, c, s, d).number(); // for vectors number is sum
                    }
                    return Result(sum);
                  }
                  break;

        case 22 : { /* MEAN( ... ) */
                    double sum=0;
                    int count=0;

                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l,x, it, m, p, c, s, d); // for vectors number is sum
                        sum += res.number();
                        if (res.asNumeric().count()) count += res.asNumeric().count();
                        else count++;
                    }
                    return count ? Result(sum/double(count)) : Result(0);
                  }
                  break;

        case 85 : { /* MEDIAN */
                        Result vector(0);

                        // collect the values
                        foreach(Leaf *l, leaf->fparms) {
                            Result res = eval(df, l,x, it, m, p, c, s, d); // for vectors number is sum
                            if (res.asNumeric().count()) vector.asNumeric().append(res.asNumeric());
                            else vector.asNumeric() << res.number();
                        }

                        if (vector.asNumeric().count() < 1) return Result(0);
                        if (vector.asNumeric().count() == 1) return Result(vector.asNumeric().at(0));

                        // sort and find the one in the middle
                        std::sort(vector.asNumeric().begin(), vector.asNumeric().end());

                        // let gsl do it
                        double median = gsl_stats_median_from_sorted_data(vector.asNumeric().constData(), 1, vector.asNumeric().count());
                        return Result(median);
                  }
                  break;

        case 86 : { /* MODE */
                        Result vector(0);

                        // collect the values
                        foreach(Leaf *l, leaf->fparms) {
                            Result res = eval(df, l,x, it, m, p, c, s, d); // for vectors number is sum
                            if (res.asNumeric().count()) vector.asNumeric().append(res.asNumeric());
                            else vector.asNumeric() << res.number();
                        }

                        // lets get a count going
                        QMap<double, int> counter;
                        foreach(double value, vector.asNumeric()){
                            int now = counter.value(value, 0);
                            now++;
                            counter.insert(value, now);
                        }

                        // lets find the max
                        QMapIterator<double, int>it(counter);
                        int maxcount=0;
                        while (it.hasNext()) {
                            it.next();
                            if (it.value() > maxcount) {
                                maxcount = it.value();
                            }
                        }

                        // now lets average the results
                        double sum = 0;
                        double count = 0;
                        it.toFront();
                        while(it.hasNext()) {
                            it.next();
                            if (it.value() == maxcount) {
                                sum += it.key();
                                count++;
                            }
                        }
                        return Result(sum / count);
                  }
                  break;

        case 23 : { /* MAX( ... ) */
                    double max=0;
                    bool set=false;

                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l,x, it, m, p, c, s, d);
                        if (res.asNumeric().count()) {
                            foreach(double x, res.asNumeric()) {
                                if (set && x>max) max=x;
                                else if (!set) { set=true; max=x; }
                            }

                        } else {
                            if (set && res.number()>max) max=res.number();
                            else if (!set) { set=true; max=res.number(); }
                        }
                    }
                    return Result(max);
                  }
                  break;

        case 24 : { /* MIN( ... ) */
                    double min=0;
                    bool set=false;

                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l,x, it, m, p, c, s, d);
                        if (res.asNumeric().count()) {
                            foreach(double x, res.asNumeric()) {
                                if (set && x<min) min=x;
                                else if (!set) { set=true; min=x; }
                            }

                        } else {
                            if (set && res.number()<min) min=res.number();
                            else if (!set) { set=true; min=res.number(); }
                        }
                    }
                    return Result(min);
                  }
                  break;

        case 25 : { /* COUNT( ... ) */

                    int count = 0;
                    foreach(Leaf *l, leaf->fparms) {
                        Result res = eval(df, l,x, it, m, p, c, s, d);
                        if (res.asNumeric().count()) count += res.asNumeric().count();
                        else count++;
                    }
                    return Result(count);
                  }
                  break;

        case 26 : { /* LTS (expr) */
                    PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df);
                    return Result(pmcData->lts(m->dateTime.date()));
                  }
                  break;

        case 27 : { /* STS (expr) */
                    PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df);
                    return Result(pmcData->sts(m->dateTime.date()));
                  }
                  break;

        case 28 : { /* SB (expr) */
                    PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df);
                    return Result(pmcData->sb(m->dateTime.date()));
                  }
                  break;

        case 29 : { /* RR (expr) */
                    PMCData *pmcData = m->context->athlete->getPMCFor(leaf->fparms[0], df);
                    return Result(pmcData->rr(m->dateTime.date()));
                  }
                  break;

        case 30 :
        case 95 :
                { /* ESTIMATE( model, CP | FTP | W' | PMAX | duration ) */
                  /* ESTIMATES( model, CP | FTP | W' | PMAX | duration | date) */

                    // which model ?
                    QString model = *leaf->fparms[0]->lvalue.n;

                    // what we looking for ?
                    QString parm = leaf->fparms[1]->type == Leaf::Symbol ? *leaf->fparms[1]->lvalue.n : "";
                    bool toDuration = parm == "" ? true : false;
                    double duration = toDuration ? eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number() : 0;

                    if (fnum == 30) {

                        // get the PD Estimate for this date - note we always work with the absolulte
                        // power estimates in formulas, since the user can just divide by config(weight)
                        // or Athlete_Weight (which takes into account values stored in ride files.
                        // Bike or Run models are used according to activity type
                        PDEstimate pde = m->context->athlete->getPDEstimateFor(m->dateTime.date(), model, false, m->isRun);

                        // no model estimate for this date
                        if (pde.parameters.count() == 0) return Result(0);

                        // get a duration
                        if (toDuration == true) {

                            double value = 0;

                            // we need to find the model
                            foreach(PDModel *pdm, df->models) {

                                // not the one we want
                                if (pdm->code() != model) continue;

                                // set the parameters previously derived
                                pdm->loadParameters(pde.parameters);

                                // use seconds
                                pdm->setMinutes(false);

                                // get the model estimate for our duration
                                value = pdm->y(duration);

                                // our work here is done
                                return Result(value);
                            }

                        } else {

                            if (parm == "cp") return Result(pde.CP);
                            if (parm == "w'") return Result(pde.WPrime);
                            if (parm == "ftp") return Result(pde.FTP);
                            if (parm == "pmax") return Result(pde.PMax);
                        }

                    } else {

                        Result returning(0);

                        if (m == NULL) return returning; // no ride

                        // date range, returning a vector
                        foreach(PDEstimate pde, m->context->athlete->getPDEstimates()) {

                            // does it match our criteria?
                            if (pde.model == model && pde.parameters.count() != 0 && pde.from <= d.to && pde.to >= d.from && pde.run==m->isRun && pde.wpk==false) {

                                // overlaps, but truncate the dates we return
                                int dfrom, dto;
                                QDate earliest(1900,01,01);
                                dfrom = earliest.daysTo(pde.from < d.from ? d.from : pde.from);
                                dto = earliest.daysTo(pde.to > d.to ? d.to : pde.to);

                                double v1, v2;

                                // get a duration
                                if (toDuration == true) {

                                    // we need to find the model
                                    foreach(PDModel *pdm, df->models) {

                                        // not the one we want
                                        if (pdm->code() != model) continue;

                                        // set the parameters previously derived
                                        pdm->loadParameters(pde.parameters);

                                        // use seconds
                                        pdm->setMinutes(false);

                                        // get the model estimate for our duration
                                        v1=v2 = pdm->y(duration);
                                    }

                                } else {

                                    if (parm == "cp") v1=v2=pde.CP;
                                    if (parm == "w'") v1=v2=pde.WPrime;
                                    if (parm == "ftp") v1=v2=pde.FTP;
                                    if (parm == "pmax") v1=v2=pde.PMax;
                                    if (parm == "date") { v1=dfrom; v2=dto; }
                                }

                                returning.number() += v1+v2;
                                returning.asNumeric() << v1 << v2;
                            }
                        }
                        return returning;
                    }
                }
                break;

        case 31 :
                {   // WHICH ( expr, ... )
                    Result returning(0);

                    // runs through all parameters, evaluating expression
                    // in first param, and if true, adding to the results
                    // this is a select statement.
                    // e.g. which(x > 0, 1,2,3,-5,-6,-7) would return
                    //      (1,2,3). More meaningfully it is used when
                    //      working with vectors
                    if (leaf->fparms.count() < 2) return returning;

                    for(int i=1; i< leaf->fparms.count(); i++) {

                        // evaluate the parameter
                        Result ex = eval(df, leaf->fparms[i],x, it, m, p, c, s, d);

                        if (ex.asNumeric().count()) {

                            // tiz a vector
                            foreach(double x, ex.asNumeric()) {

                                // did it get selected?
                                Result which = eval(df, leaf->fparms[0],Result(x), it, m, p, c, s, d);
                                if (which.number()) {
                                    returning.asNumeric() << x;
                                    returning.number() += x;
                                }
                            }

                        } else {

                            // does the parameter get selected ?
                            Result which = eval(df, leaf->fparms[0], Result(ex.number()), it, m, p, c, s); //XXX it should be local index
                            if (which.number()) {
                                returning.asNumeric() << ex.number();
                                returning.number() += ex.number();
                            }
                        }
                    }
                    return Result(returning);
                }
                break;

        case 32 :
                {   // SET (field, value, expression ) returns expression evaluated
                    Result returning(0);

                    if (leaf->fparms.count() < 3) return returning;
                    else returning = eval(df, leaf->fparms[2],x, it, m, p, c, s, d);

                    if (returning.number()) {

                        // symbol we are setting
                        QString symbol = *(leaf->fparms[0]->lvalue.n);

                        // lookup metrics (we override them)
                        QString o_symbol = df->lookupMap.value(symbol,"");
                        RideMetricFactory &factory = RideMetricFactory::instance();
                        const RideMetric *e = factory.rideMetric(o_symbol);

                        // ack ! we need to set, so open the ride
                        RideFile *f = m->ride();

                        if (!f) return Result(0); // eek!

                        // evaluate second argument, its the value
                        Result r = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

                        // now set an override or a tag
                        if (o_symbol != "" && e) { // METRIC OVERRIDE

                            // lets set the override
                            QMap<QString,QString> override;
                            override  = f->metricOverrides.value(o_symbol);

                            // clear and reset override value for this metric
                            override.insert("value", QString("%1").arg(r.number())); // add metric value

                            // update overrides for this metric in the main QMap
                            f->metricOverrides.insert(o_symbol, override);

                            // rideFile is now dirty!
                            m->setDirty(true);

                            // get refresh done, coz overrides state has changed
                            m->notifyRideMetadataChanged();

                        } else { // METADATA TAG

                            // need to set metadata tag
                            bool isnumeric = df->lookupType.value(symbol);

                            // are we using the right types ?
                            if (r.isNumber && isnumeric) {
                                f->setTag(o_symbol, QString("%1").arg(r.number()));
                            } else if (!r.isNumber && !isnumeric) {
                                f->setTag(o_symbol, r.string());
                            } else {
                                // nope
                                return Result(0); // not changing it !
                            }

                            // rideFile is now dirty!
                            m->setDirty(true);

                            // get refresh done, coz overrides state has changed
                            m->notifyRideMetadataChanged();

                        }
                    }
                    return returning;
                }
                break;
        case 33 :
                {   // UNSET (field, expression ) remove override or tag
                    Result returning(0);

                    if (leaf->fparms.count() < 2) return returning;
                    else returning = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

                    if (returning.number()) {

                        // symbol we are setting
                        QString symbol = *(leaf->fparms[0]->lvalue.n);

                        // lookup metrics (we override them)
                        QString o_symbol = df->lookupMap.value(symbol,"");
                        RideMetricFactory &factory = RideMetricFactory::instance();
                        const RideMetric *e = factory.rideMetric(o_symbol);

                        // ack ! we need to set, so open the ride
                        RideFile *f = m->ride();

                        if (!f) return Result(0); // eek!

                        // now remove the override
                        if (o_symbol != "" && e) { // METRIC OVERRIDE

                            // update overrides for this metric in the main QMap
                            f->metricOverrides.remove(o_symbol);

                            // rideFile is now dirty!
                            m->setDirty(true);

                            // get refresh done, coz overrides state has changed
                            m->notifyRideMetadataChanged();

                        } else { // METADATA TAG

                            // remove the tag
                            f->removeTag(o_symbol);

                            // rideFile is now dirty!
                            m->setDirty(true);

                            // get refresh done, coz overrides state has changed
                            m->notifyRideMetadataChanged();

                        }
                    }
                    return returning;
                }
                break;

        case 34 :
                {   // ISSET (field) is the metric overriden or metadata set ?

                    if (leaf->fparms.count() != 1) return Result(0);

                    // symbol we are setting
                    QString symbol = *(leaf->fparms[0]->lvalue.n);

                    // lookup metrics (we override them)
                    QString o_symbol = df->lookupMap.value(symbol,"");
                    RideMetricFactory &factory = RideMetricFactory::instance();
                    const RideMetric *e = factory.rideMetric(o_symbol);

                    // now remove the override
                    if (o_symbol != "" && e) { // METRIC OVERRIDE

                        return Result (m->overrides_.contains(o_symbol) == true);

                    } else { // METADATA TAG

                        return Result (m->hasText(o_symbol));
                    }
                }
                break;

        case 35 :
                {   // VDOTTIME (VDOT, distance[km])

                    if (leaf->fparms.count() != 2) return Result(0);

                    return Result (60*VDOTCalculator::eqvTime(eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number(), 1000*eval(df, leaf->fparms[1],x, it, m, p, c, s, d).number()));
                }
                break;

        case 36 :
                {   // BESTTIME (distance[km])

                    if (leaf->fparms.count() != 1 || m->fileCache() == NULL) return Result(0);

                    return Result (m->fileCache()->bestTime(eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number()));
                 }

        case 37 :
                {   // XDATA ("XDATA", "XDATASERIES", (sparse, repeat, interpolate, resample)

                    if (!p) {

                        // processing ride item (e.g. filter, formula)
                        // we return true or false if the xdata series exists for the ride in question
                        QString xdata = *(leaf->fparms[0]->lvalue.s);
                        QString series = *(leaf->fparms[1]->lvalue.s);

                        if (m->xdataMatch(xdata, series, xdata, series)) return Result(1);
                        else return Result(0);

                    } else {

                        // get iteration state from datafilter runtime
                        int idx = df->indexes.value(this, 0);

                        QString xdata = *(leaf->fparms[0]->lvalue.s);
                        QString series = *(leaf->fparms[1]->lvalue.s);

                        double returning = 0;

                        // get the xdata value for this sample (if it exists)
                        if (m->xdataMatch(xdata, series, xdata, series))
                            returning = m->ride()->xdataValue(p, idx, xdata,series, leaf->xjoin);

                        // update state
                        df->indexes.insert(this, idx);

                        return Result(returning);

                    }
                    return Result(0);
                }
                break;
        case 38:  // PRINT(x) to qDebug
                {

                    // what is the parameter?
                    if (leaf->fparms.count() != 1) qDebug()<<"bad print.";

                    // symbol we are setting
                    leaf->fparms[0]->print(0, df);
                }
                break;

        case 39 :
                {   // AUTOPROCESS(expression) to run automatic data processors
                    Result returning(0);

                    if (leaf->fparms.count() != 1) return returning;
                    else returning = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);

                    if (returning.number()) {

                        // ack ! we need to autoprocess, so open the ride
                        RideFile *f = m->ride();

                        if (!f) return Result(0); // eek!

                        // now run auto data processors
                        if (DataProcessorFactory::instance().autoProcess(f, "Auto", "UPDATE")) {
                            // rideFile is now dirty!
                            m->setDirty(true);
                        }
                    }
                    return returning;
                }
                break;

        case 40 :
                {   // POSTPROCESS (processor, expression ) run processor
                    Result returning(0);

                    if (leaf->fparms.count() < 2) return returning;
                    else returning = eval(df, leaf->fparms[1],x, it, m, p, c, s, d);

                    if (returning.number()) {

                        // processor we are running
                        QString dp_name = *(leaf->fparms[0]->lvalue.n);

                        // lookup processor
                        DataProcessor* dp = DataProcessorFactory::instance().getProcessors().value(dp_name, NULL);

                        if (!dp) return Result(0); // No such data processor

                        // ack ! we need to autoprocess, so open the ride
                        RideFile *f = m->ride();

                        if (!f) return Result(0); // eek!

                        // now run the data processor
                        if (dp->postProcess(f)) {
                            // rideFile is now dirty!
                            m->setDirty(true);
                        }
                    }
                    return returning;
                }
                break;

        case 41 :
                {   // XDATA_UNITS ("XDATA", "XDATASERIES")

                    if (p) { // only valid when iterating

                        // processing ride item (e.g. filter, formula)
                        // we return true or false if the xdata series exists for the ride in question
                        QString xdata = *(leaf->fparms[0]->lvalue.s);
                        QString series = *(leaf->fparms[1]->lvalue.s);

                        if (m->xdataMatch(xdata, series, xdata, series)) {

                            // we matched, xdata and series contain what was matched
                            XDataSeries *xs = m->ride()->xdata(xdata);

                            if (xs && m->xdata().value(xdata,QStringList()).contains(series)) {
                                int idx = m->xdata().value(xdata,QStringList()).indexOf(series);
                                QString units;
                                const int count = xs->unitname.count();
                                if (idx >= 0 && idx < count)
                                    units = xs->unitname[idx];
                                return Result(units);
                            }

                        } else return Result("");

                    } else return Result(""); // not for filtering
                }
                break;

        case 42 :
                {   // MEASURE (DATE, GROUP, FIELD) get measure
                    if (leaf->fparms.count() < 3) return Result(0);

                    Result days = eval(df, leaf->fparms[0],x, it, m, p, c, s, d);
                    if (!days.isNumber) return Result(0); // invalid date
                    QDate date = QDate(1900,01,01).addDays(days.number());
                    if (!date.isValid()) return Result(0); // invalid date

                    if (leaf->fparms[1]->type != String) return Result(0);
                    QString group_symbol = *(leaf->fparms[1]->lvalue.s);
                    int group = m->context->athlete->measures->getGroupSymbols().indexOf(group_symbol);
                    if (group < 0) return Result(0); // unknown group

                    if (leaf->fparms[2]->type != String) return Result(0);
                    QString field_symbol = *(leaf->fparms[2]->lvalue.s);
                    int field = m->context->athlete->measures->getFieldSymbols(group).indexOf(field_symbol);
                    if (field < 0) return Result(0); // unknown field

                    // retrieve measure value
                    double value = m->context->athlete->measures->getFieldValue(group, date, field);
                    return Result(value);
                }
                break;

        case 43 :
                {
                    // if no parameters just return the number of tests either in the current
                    // date range -or- for the current ride
                    if (leaf->fparms.count() == 0) {

                        // activity
                        if (d.from == QDate() && d.to == QDate()) {
                            int count=0;
                            foreach(IntervalItem *i, m->intervals())
                                if (i->istest()) count++;
                            return Result(count);

                        } else {

                            // date range
                            FilterSet fs;
                            fs.addFilter(m->context->isfiltered, m->context->filters);
                            fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
                            Specification spec;
                            spec.setFilterSet(fs);

                            spec.setDateRange(d); // fallback to daterange selected

                            // loop through rides for daterange
                            int count=0;
                            foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                                if (!s.pass(ride)) continue; // relies upon the daterange being passed to eval...
                                if (!spec.pass(ride)) continue; // relies upon the daterange being passed to eval...

                                foreach(IntervalItem *i, ride->intervals())
                                    if (i->istest()) count++;
                            }
                            return Result(count);
                        }

                    } else {

                        // want to return a vector of dates or powers
                        // for tests that are available
                        QString symbol1 = *(leaf->fparms[0]->lvalue.s);
                        QString symbol2 = *(leaf->fparms[1]->lvalue.s);
                        bool wantuser = symbol1 == "user" ? true : false; // user | best
                        bool wantduration = symbol2 == "duration" ? true : false; // date | power
                        Result returning(0);

                        if (d.from == QDate() && d.to == QDate()) {

                            // for the date of an activity
                            if (wantuser) {
                                // look for tests
                                foreach(IntervalItem *i, m->intervals()) {
                                    if (i->istest()) {
                                        double value= wantduration ? i->getForSymbol("workout_time") : i->getForSymbol("average_power");
                                        returning.number() += value;
                                        returning.asNumeric() << value;
                                    }
                                }
                            } else {
                                // look for bests on the same day
                                Performance onday = m->context->athlete->rideCache->estimator->getPerformanceForDate(m->dateTime.date(), false); //XXX fixme for runs
                                if (onday.duration >0) {
                                    double value = wantduration ? onday.duration : onday.power;
                                    returning.number() += value;
                                    returning.asNumeric() << value;
                                }
                            }

                        } else {

                            FilterSet fs;
                            fs.addFilter(m->context->isfiltered, m->context->filters);
                            fs.addFilter(m->context->ishomefiltered, m->context->homeFilters);
                            Specification spec;
                            spec.setFilterSet(fs);
                            spec.setDateRange(d); // fallback to daterange selected

                            // for a date range
                            if (wantuser) {

                                // user marked intervals

                                // loop through rides for daterange
                                foreach(RideItem *ride, m->context->athlete->rideCache->rides()) {

                                    if (!s.pass(ride)) continue; // relies upon the daterange being passed to eval...
                                    if (!spec.pass(ride)) continue; // relies upon the daterange being passed to eval...

                                    foreach(IntervalItem *i, ride->intervals()) {
                                        if (i->istest()) {
                                            double value= wantduration ? i->getForSymbol("workout_time") : i->getForSymbol("average_power");
                                            returning.number() += value;
                                            returning.asNumeric() << value;
                                        }
                                    }
                                }

                            } else {

                                // weekly best performances
                                QList<Performance> perfs = m->context->athlete->rideCache->estimator->allPerformances();
                                foreach(Performance p, perfs) {
                                    if (p.submaximal == false && p.run == false && p.when >= d.from && p.when <= d.to) { // XXX fixme p.run == false
                                        double value = wantduration ? p.duration : p.power;
                                        returning.number() += value;
                                        returning.asNumeric() << value;
                                    }
                                }
                            }
                        }
                        return returning;
                    }
                }
                break;
        case 63 : { return Result(sqrt(eval(df, leaf->fparms[0],x, it, m, p, c, s, d).number())); } // SQRT(x)

        default:
            return Result(0);
        }
    }
    break;

    //
    // SCRIPT
    //
    case Leaf::Script :
    {

        // run a script
 #ifdef GC_WANT_PYTHON
        if (leaf->function == "python")  return Result(df->runPythonScript(m->context, *leaf->lvalue.s, m, c, s));
 #endif
        return Result(0);
    }
    break;

    //
    // SYMBOLS
    //
    case Leaf::Symbol :
    {
        double lhsdouble=0.0f;
        bool lhsisNumber=false;
        QString lhsstring;
        QString rename;
        QString symbol = *(leaf->lvalue.n);

        // ride series name when running through sample override metrics etc
        if (p && (lhsisNumber = df->dataSeriesSymbols.contains(*(leaf->lvalue.n))) == true) {

            RideFile::SeriesType type = RideFile::seriesForSymbol((*(leaf->lvalue.n)));
            if (type == RideFile::index) return Result(m->ride()->dataPoints().indexOf(p));
            return Result(p->value(type));
        }

        // user defined symbols override all others !
        if (df->symbols.contains(symbol)) return Result(df->symbols.value(symbol));

        // is it isRun ?
        if (symbol == "i") {

            lhsdouble = it;
            lhsisNumber = true;

        } else if (symbol == "x") {

            if (x.isNumber) {
                lhsdouble = Result(x).number();
                lhsisNumber = true;
            } else {
                lhsstring = Result(x).string();
                lhsisNumber = false;
            }

        } else if (symbol == "isRide") {
            lhsdouble = m->isBike ? 1 : 0;
            lhsisNumber = true;

        } else if (symbol == "isRun") {
            lhsdouble = m->isRun ? 1 : 0;
            lhsisNumber = true;

        } else if (symbol == "isSwim") {
            lhsdouble = m->isSwim ? 1 : 0;
            lhsisNumber = true;

        } else if (symbol == "isXtrain") {
            lhsdouble = m->isXtrain ? 1 : 0;
            lhsisNumber = true;

        } else if (!symbol.compare("NA", Qt::CaseInsensitive)) {

            lhsdouble = RideFile::NA;
            lhsisNumber = true;

        } else if (!symbol.compare("RECINTSECS", Qt::CaseInsensitive)) {

            lhsdouble = 1; // if in doubt
            if (m->ride(false)) lhsdouble = m->ride(false)->recIntSecs();
            lhsisNumber = true;

        } else if (!symbol.compare("Current", Qt::CaseInsensitive)) {

            if (m->context->currentRideItem())
                lhsdouble = QDate(1900,01,01).
                daysTo(m->context->currentRideItem()->dateTime.date());
            else
                lhsdouble = 0;
            lhsisNumber = true;

        } else if (!symbol.compare("Today", Qt::CaseInsensitive)) {

            lhsdouble = QDate(1900,01,01).daysTo(QDate::currentDate());
            lhsisNumber = true;

        } else if (!symbol.compare("Date", Qt::CaseInsensitive)) {

            lhsdouble = QDate(1900,01,01).daysTo(m->dateTime.date());
            lhsisNumber = true;

        } else if (!symbol.compare("Time", Qt::CaseInsensitive)) {

            lhsdouble = QTime(0,0,0).secsTo(m->dateTime.time());
            lhsisNumber = true;

        } else if (isCoggan(symbol)) {
            // a coggan PMC metric
            PMCData *pmcData = m->context->athlete->getPMCFor("coggan_tss");
            if (!symbol.compare("ctl", Qt::CaseInsensitive)) lhsdouble = pmcData->lts(m->dateTime.date());
            if (!symbol.compare("atl", Qt::CaseInsensitive)) lhsdouble = pmcData->sts(m->dateTime.date());
            if (!symbol.compare("tsb", Qt::CaseInsensitive)) lhsdouble = pmcData->sb(m->dateTime.date());
            lhsisNumber = true;

        } else if ((lhsisNumber = df->lookupType.value(*(leaf->lvalue.n))) == true) {
            // get symbol value
            // check metadata string to number first ...
            QString meta = m->getText(rename=df->lookupMap.value(symbol,""), "unknown");
            if (meta == "unknown")
                if (c) lhsdouble = RideMetric::getForSymbol(rename=df->lookupMap.value(symbol,""), c);
                else lhsdouble = m->getForSymbol(rename=df->lookupMap.value(symbol,""));
            else
                lhsdouble = meta.toDouble();
            lhsisNumber = true;

            //qDebug()<<"symbol" << *(lvalue.n) << "is" << lhsdouble << "via" << rename;
        } else {
            // string symbol will evaluate to zero as unary expression
            lhsstring = m->getText(rename=df->lookupMap.value(symbol,""), "");
            //qDebug()<<"symbol" << *(lvalue.n) << "is" << lhsstring << "via" << rename;
        }
        if (lhsisNumber) return Result(lhsdouble);
        else return Result(lhsstring);
    }
    break;

    //
    // LITERALS
    //
    case Leaf::Float :
    {
        return Result(leaf->lvalue.f);
    }
    break;

    case Leaf::Integer :
    {
        return Result(leaf->lvalue.i);
    }
    break;

    case Leaf::String :
    {
        QString string = *(leaf->lvalue.s);

        // dates are returned as numbers
        QDate date = QDate::fromString(string, "yyyy/MM/dd");
        if (date.isValid()) return Result(QDate(1900,01,01).daysTo(date));
        else return Result(string);
    }
    break;

    //
    // UNARY EXPRESSION
    //
    case Leaf::UnaryOperation :
    {
        // get result
        Result lhs = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);

        // unary minus
        if (leaf->op == '-') return Result(lhs.number() * -1);

        // unary not
        if (leaf->op == '!') return Result(!lhs.number());

        // unknown
        return(Result(0));
    }
    break;

    //
    // BINARY EXPRESSION
    //
    case Leaf::BinaryOperation :
    case Leaf::Operation :
    {
        // lhs and rhs
        Result lhs;
        if (leaf->op != ASSIGN) lhs = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);

        // if elvis we only evaluate rhs if we are null
        Result rhs;
        if (leaf->op != ELVIS || lhs.number() == 0) {
            rhs = eval(df, leaf->rvalue.l,x, it, m, p, c, s, d);
        }

        // NOW PERFORM OPERATION
        switch (leaf->op) {

        case ASSIGN:
        {
            // LHS MUST be a symbol...
            if (leaf->lvalue.l->type == Leaf::Symbol || leaf->lvalue.l->type == Leaf::Index) {

                if (leaf->lvalue.l->type == Leaf::Symbol) {

                    // update the symbol value
                    QString symbol = *(leaf->lvalue.l->lvalue.n);
                    df->symbols.insert(symbol, rhs);

                } else {

                    // for an index we need the symbol first to update its vector
                    QString symbol = *(leaf->lvalue.l->lvalue.l->lvalue.n);

                    // we may have multiple indexes to assign!
                    Result indexes = eval(df,leaf->lvalue.l->fparms[0],x, it, m, p, c, s, d);

                    // generic symbol
                    if (df->symbols.contains(symbol)) {
                        Result sym = df->symbols.value(symbol);

                        // assigning number to strings, need to coerce rhs to string
                        if (rhs.isNumber && !sym.isNumber) {
                            rhs.string();
                            rhs.isNumber = false;
                        }

                        // assigning a string to a numeric vector - need to convert sym to strings
                        if (sym.isNumber && !rhs.isNumber) {
                            sym.asString(); // will coerce
                            sym.isNumber = false;
                        }

                        // is it a single value e.g. a[10] or a range e.g. a[x>2]
                        QVector<double> selected;
                        if (indexes.asNumeric().count()) selected=indexes.asNumeric();
                        else selected << indexes.number();

                        int rindex=0;
                        for(int i=0; i< selected.count(); i++) {

                            int index=static_cast<int>(selected[i]);

                            // get the next value to apply - needs to work with vectors
                            // and vectors will be repeated if they are too short
                            double number;
                            QString string;
                            if (rhs.isVector()) {
                                if (rhs.isNumber) {
                                    if (rindex > rhs.asNumeric().count()) rindex=0;
                                    number = rhs.asNumeric()[rindex++];
                                } else {
                                    if (rindex > rhs.asString().count()) rindex=0;
                                    string = rhs.asString()[rindex++];
                                }
                            } else {
                                if (rhs.isNumber) number = rhs.number();
                                else string = rhs.string();
                            }

                            // working with numbers on both sides
                            if (rhs.isNumber && sym.isNumber) {
                                if (sym.asNumeric().count() <= index) { sym.asNumeric().resize(index+1); }
                                sym.asNumeric()[index] = number;
                            }

                            // working with strings on both sides
                            if (!sym.isNumber && !rhs.isNumber) {
                                if (sym.asString().count() <= index) { sym.asString().resize(index+1); }
                                sym.asString()[index] = string;
                            }
                        }

                        // update
                        df->symbols.insert(symbol, sym);
                    }
                }
                return rhs;
            }
            // shouldn't get here!
            return Result(RideFile::NA);
        }
        break;

        break;

        // basic operations should all work with vectors or numbers
        case ADD:
        case SUBTRACT:
        case DIVIDE:
        case MULTIPLY:
        case POW:
        {
            Result returning(0);

            // only if numberic on both sides
            if (lhs.isNumber && rhs.isNumber) {


                // its a vector operation...
                if (lhs.asNumeric().count() || rhs.asNumeric().count()) {

                    int size = lhs.asNumeric().count() > rhs.asNumeric().count() ? lhs.asNumeric().count() : rhs.asNumeric().count();

                    // coerce both into a vector of matching size
                    lhs.vectorize(size);
                    rhs.vectorize(size);

                    for(int i=0; i<size; i++) {
                        double left = lhs.asNumeric()[i];
                        double right = rhs.asNumeric()[i];
                        double value = 0;

                        switch (leaf->op) {
                        case ADD: value = left + right; break;
                        case SUBTRACT: value = left - right; break;
                        case DIVIDE: value = right ? left / right : 0; break;
                        case MULTIPLY: value = left * right; break;
                        case POW: value = pow(left,right); break;
                        }
                        returning.asNumeric() << value;
                        returning.number() += value;
                    }

                } else {
                    switch (leaf->op) {
                    case ADD: returning.number() = lhs.number() + rhs.number(); break;
                    case SUBTRACT: returning.number() = lhs.number() - rhs.number(); break;
                    case DIVIDE: returning.number() = rhs.number() ? lhs.number() / rhs.number() : 0; break;
                    case MULTIPLY: returning.number() = lhs.number() * rhs.number(); break;
                    case POW: returning.number() = pow(lhs.number(), rhs.number()); break;
                    }
                }
            } else {

                // either the left or rhs is not a number, it is a string
                // so we need to return a string result
                returning.isNumber = false;

                // basically add is the only meaningful operation to apply
                // to string values; for vectors append, for string just concatenate
                if (leaf->op == ADD) {
                    if (lhs.isVector() || rhs.isVector()) {

                        // create a bigger vector
                        if (lhs.isVector()) returning.asString() << lhs.asString();
                        else returning.asString() << lhs.string();
                        if (rhs.isVector()) returning.asString() << rhs.asString();
                        else returning.asString() << rhs.string();

                    } else {
                        // cat strings
                        returning.string() = lhs.string() + rhs.string();
                    }
                } else {
                    // just return the lhs
                    returning = lhs;
                }
            }
            return returning;
        }
        break;

        case EQ:
        {
            if (lhs.isNumber) return Result(lhs.number() == rhs.number());
            else return Result(lhs.string() == rhs.string());
        }
        break;

        case NEQ:
        {
            if (lhs.isNumber) return Result(lhs.number() != rhs.number());
            else return Result(lhs.string() != rhs.string());
        }
        break;

        case LT:
        {
            if (lhs.isNumber) return Result(lhs.number() < rhs.number());
            else return Result(lhs.string() < rhs.string());
        }
        break;
        case LTE:
        {
            if (lhs.isNumber) return Result(lhs.number() <= rhs.number());
            else return Result(lhs.string() <= rhs.string());
        }
        break;
        case GT:
        {
            if (lhs.isNumber) return Result(lhs.number() > rhs.number());
            else return Result(lhs.string() > rhs.string());
        }
        break;
        case GTE:
        {
            if (lhs.isNumber) return Result(lhs.number() >= rhs.number());
            else return Result(lhs.string() >= rhs.string());
        }
        break;

        case ELVIS:
        {
            // it was evaluated above, which is kinda cheating
            // but its optimal and this is a special case.
            if (lhs.isNumber && lhs.number()) return Result(lhs.number());
            else return Result(rhs.number());
        }
        case MATCHES:
            if (!lhs.isNumber && !rhs.isNumber) return Result(QRegExp(rhs.string()).exactMatch(lhs.string()));
            else return Result(false);
            break;

        case ENDSWITH:
            if (!lhs.isNumber && !rhs.isNumber) return Result(lhs.string().endsWith(rhs.string()));
            else return Result(false);
            break;

        case BEGINSWITH:
            if (!lhs.isNumber && !rhs.isNumber) return Result(lhs.string().startsWith(rhs.string()));
            else return Result(false);
            break;

        case CONTAINS:
            {
            if (!lhs.isNumber && !rhs.isNumber) {
                if (lhs.isVector()) return Result(lhs.asString().contains(rhs.string()));
                else return Result(lhs.string().contains(rhs.string()) ? true : false);
            } else return Result(false);
            }
            break;

        default:
            break;
        }
    }
    break;

    //
    // CONDITIONAL TERNARY / IF .. ELSE ../ WHILE
    //
    case Leaf::Conditional :
    {

        switch(leaf->op) {

        case IF_:
        case 0 :
            {
                Result cond = eval(df, leaf->cond.l,x, it, m, p, c, s, d);
                if (cond.isNumber && cond.number()) return eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);
                else {

                    // conditional may not have an else clause!
                    if (leaf->rvalue.l) return eval(df, leaf->rvalue.l,x, it, m, p, c, s, d);
                    else return Result(0);
                }
            }
        case WHILE :
            {
                // we bound while to make sure it doesn't consume all
                // CPU and 'hang' for badly written code..
                static int maxwhile = 1000000;
                int count=0;
                QTime timer;
                timer.start();

                Result returning(0);
                while (count++ < maxwhile && eval(df, leaf->cond.l,x, it, m, p, c, s, d).number()) {
                    returning = eval(df, leaf->lvalue.l,x, it, m, p, c, s, d);
                }

                // we had to terminate warn user !
                if (count >= maxwhile) {
                    qDebug()<<"WARNING: "<< "[ loops="<<count<<"ms="<<timer.elapsed() <<"] runaway while loop terminated, check formula/filter.";
                }

                return returning;
            }
        }
    }
    break;

    // INDEXING INTO VECTORS
    case Leaf::Index :
    {
        Result index = eval(df,leaf->fparms[0],x, it, m, p, c, s, d);
        Result value = eval(df,leaf->lvalue.l,x, it, m, p, c, s, d); // lhs might also be a symbol

        // are we returning the value or a vector of values?
        if (index.asNumeric().count()) {

            Result returning(0);
            if (!value.isNumber) returning.isNumber = false;

            // a range
            for(int i=0; i<index.asNumeric().count(); i++) {
                int ii=index.asNumeric()[i];

                // ignore out of bounds
                if (ii < 0 || (value.isNumber && ii >= value.asNumeric().count()) || (!value.isNumber && ii >= value.asString().count())) continue;

                if (value.isNumber) {
                    // numbers do sum
                    returning.asNumeric() << value.asNumeric()[ii];
                    returning.number() += value.asNumeric()[ii];
                } else {
                    returning.asString() << value.asString()[ii];
                }
            }

            return returning;

        } else {
            // a single value
            if (value.isNumber) {
                if (index.number() < 0 || index.number() >= value.asNumeric().count()) return Result(0);
                return Result(value.asNumeric()[index.number()]);
            } else {
                if (index.number() < 0 || index.number() >= value.asString().count()) return Result("");
                return Result(value.asString()[index.number()]);

            }
        }
    }

    // SELECTING FROM VECTORS
    case Leaf::Select :
    {
        Result returning(0);

        //int index = eval(df,leaf->fparms[0],x, it, m, p, c, s, d).number;
        Result value = eval(df,leaf->lvalue.l,x, it, m, p, c, s, d); // lhs might also be a symbol

        // return the same type
        returning.isNumber = value.isNumber;

        // need a vector, always
        if (!value.isVector()) return returning;

        // loop and evaluate, non-zero we keep, zero we lose
        for(int i=0; (value.isNumber && i<value.asNumeric().count()) || (!value.isNumber && i<value.asString().count()) ; i++) {

            // we pass around x for the logical expression
            Result x = Result(0);
            x.isNumber = value.isNumber;
            if (value.isNumber)  x.number() = value.asNumeric().at(i);
            else x.string() = value.asString().at(i);

            int boolresult = eval(df,leaf->fparms[0],x, i, m, p, c, s, d).number();

            // we want it
            if (boolresult != 0) {
                if (value.isNumber) {
                    returning.asNumeric() << x.number();
                    returning.number() += x.number();
                } else {
                    returning.asString() << x.string();
                }
            }
        }

        return returning;

    }
    break;

    //
    // COMPOUND EXPRESSION
    //
    case Leaf::Compound :
    {
        Result returning(0);

        // evaluate each statement
        foreach(Leaf *statement, *(leaf->lvalue.b)) returning = eval(df, statement,x, it, m, p, c, s, d);

        // compound statements evaluate to the value of the last statement
        return returning;
    }
    break;

    default: // we don't need to evaluate any lower - they are leaf nodes handled above
        break;
    }
    return Result(0); // false
}

DFModel::DFModel(RideItem *item, Leaf *formula, DataFilterRuntime *df) : PDModel(item->context), item(item), formula(formula), df(df)
{
    // extract info from the passed params
    formula->findSymbols(parameters);
    parameters.removeDuplicates();
    parameters.removeOne("x"); // not a parameter we declare to lmfit

}

double
DFModel::f(double t, const double *parms)
{
    // set parms in the runtime
    for (int i=0; i< parameters.count(); i++)  df->symbols.insert(parameters[i], Result(parms[i]));

    // calulcate
    return formula->eval(df, formula, Result(t), 0, item, NULL, NULL, Specification(), DateRange()).number();
}

double
DFModel::y(double t) const
{
    // calculate using current runtime
    return formula->eval(df, formula, Result(t), 0, item, NULL, NULL, Specification(), DateRange()).number();
}

bool
DFModel::fitData(QVector<double>&x, QVector<double>&y)
{
    // lets call the fitting routine
    if (parameters.count() < 1) return false;

    // unpack starting parameters
    QVector<double> startingparms;
    foreach(QString symbol, parameters) {
        Result p =df->symbols.value(symbol);
        startingparms << p.number();
    }

    // get access to lmfit, single threaded :(
    lm_control_struct control = lm_control_double;
    lm_status_struct status;

    // use forwarder via global variable, so mutex around this !
    calllmfit.lock();
    calllmfitmodel = this;

    //fprintf(stderr, "Fitting ...\n" ); fflush(stderr);
    lmcurve(parameters.count(), const_cast<double*>(startingparms.constData()), x.count(), x.constData(), y.constData(), calllmfitf, &control, &status);

    // release for others
    calllmfit.unlock();

    // starting parms now contain final output lets
    // update the runtime to get them back to the user
    int i=0;
    foreach(QString symbol, parameters) {
        Result value(startingparms[i]);
        df->symbols.insert(symbol, value);
        i++;
    }

    //fprintf(stderr, "Results:\n" );
    //fprintf(stderr, "status after %d function evaluations:\n  %s\n",
    //        status.nfev, lm_infmsg[status.outcome] ); fflush(stderr);
    //fprintf(stderr,"obtained parameters:\n"); fflush(stderr);
    //for (int i = 0; i < this->nparms(); ++i)
    //    fprintf(stderr,"  par[%i] = %12g\n", i, startingparms[i]);
    //fprintf(stderr,"obtained norm:\n  %12g\n", status.fnorm ); fflush(stderr);

    if (status.outcome < 4) return true;
    return false;
}

#ifdef GC_WANT_PYTHON
double
DataFilterRuntime::runPythonScript(Context *context, QString script, RideItem *m, const QHash<QString,RideMetric*> *metrics, Specification spec)
{
    if (python == NULL) return(0);

    // get the lock
    pythonMutex.lock();

    // return result
    double result = 0;

    // run it !!
    python->canvas = NULL;
    python->chart = NULL;
    python->result = 0;

    try {

        // run it
        python->runline(ScriptContext(context, m, metrics, spec), script);
        result = python->result;

    } catch(std::exception& ex) {
        Q_UNUSED(ex)

        python->messages.clear();

    } catch(...) {

        python->messages.clear();

    }

    // clear context
    python->canvas = NULL;
    python->chart = NULL;

    // free up the interpreter
    pythonMutex.unlock();

    return result;
}
#endif
