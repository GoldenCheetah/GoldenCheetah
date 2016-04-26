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

#include "RTool.h"
#include "RGraphicsDevice.h"
#include "GcUpgrade.h"

#include "RideCache.h"
#include "RideItem.h"
#include "RideFile.h"
#include "Colors.h"
#include "RideMetric.h"

#include "Rdefines.h"

// If no callbacks we won't play
#if !defined(RINSIDE_CALLBACKS)
#error "GC_WANT_R: RInside must have callbacks enabled (see inst/RInsideConfig.h)"
#endif

RTool::RTool(int argc, char**argv)
{
    // setup the R runtime elements
    bool failed = false;
    starting = true;

    try {

        // yikes, self referenced during construction (!)
        rtool = this;

        R = new RInside(argc,argv);
        callbacks = new RCallbacks;
        R->set_callbacks(callbacks);
        dev = new RGraphicsDevice();

        // lets get the version early for the about dialog
        R->parseEvalNT("print(R.version.string)");
        QStringList &strings = callbacks->getConsoleOutput();
        if (strings.count() == 3) {
            QRegExp exp("^.*([0-9]+\\.[0-9]+\\.[0-9]+).*$");
            if (exp.exactMatch(strings[1])) version = exp.cap(1);
            else version = strings[1];
        }
        strings.clear();

        // load the dynamix library and create function wrapper
        // we should put this into a source file (.R)
        R->parseEvalNT(QString(".First <- function() {\n"
                       "    dyn.load(\"RGoldenCheetah.so\")\n"
                       "}\n"
                       "GC.display <- function() { .Call(\"GC.display\") }\n"
                       "GC.athlete <- function() { .Call(\"GC.athlete\") }\n"
                       "GC.athlete.home <- function() { .Call(\"GC.athlete.home\") }\n"
                       "GC.activities <- function() { .Call(\"GC.activities\") }\n"
                       "GC.activity <- function() { .Call(\"GC.activity\") }\n"
                       "GC.version <- function() {\n"
                       "    return(\"%1\")\n"
                       "}\n"
                       "GC.build <- function() {\n"
                       "    return(%2)\n"
                       "}\n")
                       .arg(VERSION_STRING)
                       .arg(VERSION_LATEST).toStdString());

        strings.clear();

        // set the "GC" object and methods
        context = NULL;
        canvas = NULL;

        // Access into the GC data
        (*R)["GC.activity"] = Rcpp::InternalFunction(RTool::activity);
        (*R)["GC.metrics"] = Rcpp::InternalFunction(RTool::metrics);

        // TBD
        // GC.seasons     - configured seasons
        // GC.config      - configuration (zones, units etc)

        // the following are already set in RChart on a per call basis
        // "GC.athlete" "GC.athlete.home"

        configChanged();

    } catch(std::exception& ex) {

        qDebug()<<"RInside error:"  << ex.what();
        failed = true;

    } catch(...) {

        failed = true;
    }

    // ack, disable R runtime
    if (failed) {
        qDebug() << "RInside/Rcpp failed to start, RConsole disabled.";
        version = "none";
        R = NULL;
    }
    starting = false;
}

extern "C" {
int assigndl(SEXP (**p)(SEXP(*[])()), DL_FUNC x)
{
    *p = (SEXP(*)(SEXP(*[])()))(*x);
    return 0;
}

};

void
RTool::registerRoutines()
{
    // the dynamic libray is loaded so we should be able to find
    // the initialisation function now

    // get the value
    DL_FUNC dd = R_GetCCallable("RGoldenCheetah", "GCinitialiseFunctions");

    // change signature
    SEXP (*p)(SEXP(*[])());

    // cast
    assigndl(&p, dd);

    // array of all the function pointers (just 1 for now)
    SEXP (*fn[5])() = { &RGraphicsDevice::GCdisplay,
                        &RTool::athlete,
                        &RTool::athleteHome,
                        &RTool::activities,
                        &RTool::activity };

    // dereference and call, if not found all is lost ....
    if (p) *p(fn);
}

void
RTool::configChanged()
{
    // update global R appearances
    QString parCommand=QString("par(bg=\"%1\", "
                               "    col=\"%2\", "
                               "    fg=\"%2\", "
                               "    col.main=\"%2\", "
                               "    col.sub=\"%3\", "
                               "    col.lab=\"%3\", "
                               "    col.axis=\"%3\")"
                            ).arg(GColor(CPLOTBACKGROUND).name())
                             .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name())
                             .arg(GColor(CPLOTMARKER).name());

    // fire and forget, don't care if it fails or not !!
    rtool->R->parseEvalQNT(parCommand.toStdString());
}

SEXP
RTool::athleteHome()
{
    QString returning = ".";
    if (rtool->context) returning = rtool->context->athlete->home->root().absolutePath();

    // convert to R type and return, yuck.
    SEXP ans;
    PROTECT(ans=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(ans, 0, Rf_mkChar(returning.toLatin1().constData()));
    UNPROTECT(1);
    return ans;
}

SEXP
RTool::athlete()
{
    QString returning = "none";
    if (rtool->context) returning = rtool->context->athlete->cyclist;

    // convert to R type and return, yuck.
    SEXP ans;
    PROTECT(ans=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(ans, 0, Rf_mkChar(returning.toLatin1().constData()));
    UNPROTECT(1);
    return ans;
}



SEXP
RTool::activities()
{
    SEXP dates=NULL;
    SEXP clas;

    if (rtool->context && rtool->context->athlete && rtool->context->athlete->rideCache) {

        // allocate space for a vector of dates
        PROTECT(dates=Rf_allocVector(REALSXP, rtool->context->athlete->rideCache->count()));

        // fill with values for date and class
        int i=0;
        foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {
            REAL(dates)[i++] = item->dateTime.toUTC().toTime_t();

        }

        // POSIXct class
        PROTECT(clas=Rf_allocVector(STRSXP, 2));
        SET_STRING_ELT(clas, 0, Rf_mkChar("POSIXct"));
        SET_STRING_ELT(clas, 1, Rf_mkChar("POSIXt"));
        Rf_classgets(dates,clas);

        // we use "UTC" for all timezone
        Rf_setAttrib(dates, Rf_install("tzone"), Rf_mkString("UTC"));

        UNPROTECT(2);
    }

    return dates;
}

Rcpp::DataFrame
RTool::metrics()
{
    Rcpp::DataFrame d;

    if (rtool->context && rtool->context->athlete && rtool->context->athlete->rideCache) {

        // allocate
        Rcpp::DatetimeVector l(rtool->context->athlete->rideCache->count());

        // with date first
        int i=0;
        foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {
            l[i++] = Rcpp::Datetime(item->dateTime.toUTC().toTime_t());
        }
        // use the compatability 'name' to work with e.g. R package trackeR
        d["time"] = l;

        // now add a vector for every metric
        const RideMetricFactory &factory = RideMetricFactory::instance();
        for(int i=0; i<factory.metricCount();i++) {

            Rcpp::NumericVector m(rtool->context->athlete->rideCache->rides().count());
            QString symbol = factory.metricName(i);
            const RideMetric *metric = factory.rideMetric(symbol);
            QString name = rtool->context->specialFields.internalName(factory.rideMetric(symbol)->name());
            bool useMetricUnits = rtool->context->athlete->useMetricUnits;

            int index=0;
            foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {
                m[index++] = item->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion())
                                                + (useMetricUnits ? 0.0f : metric->conversionSum());
            }

            // use the compatability 'name' to work with e.g. R package trackeR
            d[name.toStdString()] = m;
        }
    }

    // d is basically a list, this is about the only way to really
    // return a valid data.frame
    Rcpp::DataFrame returning(d);
    return returning;
}

SEXP
RTool::activity()
{
    // a dataframe to return
    SEXP ans;

    // access via global as this is a static function
    if(rtool->context && rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

        // get the ride
        RideFile *f = const_cast<RideItem*>(rtool->context->currentRideItem())->ride();
        f->recalculateDerivedSeries();
        int points = f->dataPoints().count();

        // how many series?
        int seriescount=0;
        for(int i=0; i<static_cast<int>(RideFile::none); i++) {
            RideFile::SeriesType series = static_cast<RideFile::SeriesType>(i);
            if (i > 15 && !f->isDataPresent(series)) continue;
            seriescount++;
        }

        // if we have any series we will continue and add a 'time' series
        if (seriescount) seriescount++;
        else return ans; // nowt to return

        // we return a list of series vectors
        PROTECT(ans = Rf_allocList(seriescount));

        // we collect the names as we go
        SEXP names;
        PROTECT(names = Rf_allocVector(STRSXP, seriescount)); // names attribute (column names)
        int next=0;
        SEXP nextS = ans;

        //
        // Now we need to add vectors to the ans list...
        //

        // TIME

        // add in actual time in POSIXct format
        SEXP time;
        PROTECT(time=Rf_allocVector(REALSXP, points));

        // fill with values for date and class
        for(int k=0; k<points; k++) REAL(time)[k] = f->startTime().addSecs(f->dataPoints()[k]->secs).toUTC().toTime_t();

        // POSIXct class
        SEXP clas;
        PROTECT(clas=Rf_allocVector(STRSXP, 2));
        SET_STRING_ELT(clas, 0, Rf_mkChar("POSIXct"));
        SET_STRING_ELT(clas, 1, Rf_mkChar("POSIXt"));
        Rf_classgets(time,clas);

        // we use "UTC" for all timezone
        Rf_setAttrib(time, Rf_install("tzone"), Rf_mkString("UTC"));

        // add to the data.frame and give it a name
        SETCAR(nextS, time); nextS=CDR(nextS);
        SET_STRING_ELT(names, next++, Rf_mkChar("time"));

        // time + clas, but not ans!
        UNPROTECT(2);

        // add to the end

        // PRESENT SERIES
        for(int i=0; i < static_cast<int>(RideFile::none); i++) {

            // what series we working with?
            RideFile::SeriesType series = static_cast<RideFile::SeriesType>(i);

            // lets not add lots of NA for the more obscure data series
            if (i > 15 && !f->isDataPresent(series)) continue;


            // set a vector
            SEXP vector;
            PROTECT(vector=Rf_allocVector(REALSXP, points));

            for(int j=0; j<points; j++) {
                if (f->isDataPresent(series)) {
                    if (f->dataPoints()[j]->value(series) == 0 && (series == RideFile::lat || series == RideFile::lon))
                        REAL(vector)[j] = NA_REAL;
                    else
                        REAL(vector)[j] = f->dataPoints()[j]->value(series);
                } else {
                    REAL(vector)[j] = NA_REAL;
                }
            }

            // add to the list
            SETCAR(nextS, vector);
            nextS = CDR(nextS);

            // give it a name
            SET_STRING_ELT(names, next, Rf_mkChar(f->seriesName(series, true).toLatin1().constData()));

            next++;

            // vector
            UNPROTECT(1);
        }

        // turn the list into a data frame + set column names
        Rf_setAttrib(ans, R_ClassSymbol, Rf_mkString("data.frame"));
        Rf_namesgets(ans, names);

        // ans + names
        UNPROTECT(2);
    }

    // NULL if nothing to return
    return ans;
}
