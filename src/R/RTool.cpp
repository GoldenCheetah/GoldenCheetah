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
#include "RideFileCache.h"
#include "Colors.h"
#include "RideMetric.h"
#include "RideMetadata.h"
#include "PMCData.h"
#include "WPrime.h"
#include "Season.h"
#include "DataFilter.h"
#include "Specification.h"

RTool::RTool()
{
    // setup the R runtime elements
    failed = false;
    starting = true;

    try {

        // yikes, self referenced during construction (!)
        rtool = this;

        // set default width and height
        width = height = 500;

        // initialise
        R = new REmbed();

        // failed to load
        if (R->loaded == false) {
            failed=true;
            return;
        }

        // capture all output and input to our methods
#ifndef WIN32
        ptr_R_Suicide = &RTool::R_Suicide;
        ptr_R_ShowMessage = &RTool::R_ShowMessage;
        ptr_R_ReadConsole = &RTool::R_ReadConsole;
        ptr_R_WriteConsole = &RTool::R_WriteConsole;
        ptr_R_WriteConsoleEx = &RTool::R_WriteConsoleEx;
        ptr_R_ResetConsole = &RTool::R_ResetConsole;
        ptr_R_FlushConsole = &RTool::R_FlushConsole;
        ptr_R_ClearerrConsole = &RTool::R_ClearerrConsole;
        ptr_R_Busy = &RTool::R_Busy;

        // turn off stderr io
        R_Outputfile = NULL;
        R_Consolefile = NULL;
#endif

        dev = new RGraphicsDevice();

        // register our functions
        R_CMethodDef cMethods[] = {
            { "GC.display", (DL_FUNC) &RGraphicsDevice::GCdisplay, 0 ,0, 0 },
            { "GC.page", (DL_FUNC) &RTool::pageSize, 0 ,0, 0 },
            { "GC.athlete", (DL_FUNC) &RTool::athlete, 0 ,0, 0 },
            { "GC.activities", (DL_FUNC) &RTool::activities, 0 ,0, 0 },
            { "GC.activity", (DL_FUNC) &RTool::activity, 0 ,0, 0 },
            { "GC.activity.metrics", (DL_FUNC) &RTool::activityMetrics, 0 ,0, 0 },
            { "GC.activity.meanmax", (DL_FUNC) &RTool::activityMeanmax, 0 ,0, 0 },
            { "GC.activity.wbal", (DL_FUNC) &RTool::activityWBal, 0 ,0, 0 },
            { "GC.season", (DL_FUNC) &RTool::season, 0 ,0, 0 },
            { "GC.season.metrics", (DL_FUNC) &RTool::metrics, 0 ,0, 0 },
            { "GC.season.pmc", (DL_FUNC) &RTool::pmc, 0 ,0, 0 },
            { "GC.season.meanmax", (DL_FUNC) &RTool::seasonMeanmax, 0 ,0, 0 },
            { "GC.season.peaks", (DL_FUNC) &RTool::seasonPeaks, 0 ,0, 0 },
            { NULL, NULL, 0, 0, 0 }
        };
        R_CallMethodDef callMethods[] = {
            { "GC.display", (DL_FUNC) &RGraphicsDevice::GCdisplay, 0 },
            { "GC.page", (DL_FUNC) &RTool::pageSize, 2 },

            // athlete
            { "GC.athlete", (DL_FUNC) &RTool::athlete, 0 },

            // if activity is passed compare=TRUE it returns a list of rides
            // currently in the compare pane if compare is enabled or
            // just a 1 item list with the current ride
            { "GC.activities", (DL_FUNC) &RTool::activities, 1 },
            { "GC.activity", (DL_FUNC) &RTool::activity, 2 },
            { "GC.activity.metrics", (DL_FUNC) &RTool::activityMetrics, 1 },
            { "GC.activity.meanmax", (DL_FUNC) &RTool::activityMeanmax, 1 },
            { "GC.activity.wbal", (DL_FUNC) &RTool::activityWBal, 1 },

            // all=FALSE, compare=FALSE
            { "GC.season", (DL_FUNC) &RTool::season, 2 },
            { "GC.season.metrics", (DL_FUNC) &RTool::metrics, 2 },
            { "GC.season.meanmax", (DL_FUNC) &RTool::seasonMeanmax, 2 },
            { "GC.season.peaks", (DL_FUNC) &RTool::seasonPeaks, 4 },
            // return a data.frame of pmc series (all=FALSE, metric="TSS")
            { "GC.season.pmc", (DL_FUNC) &RTool::pmc, 2 },
            { NULL, NULL, 0 }
        };

        // set them up
        DllInfo *info = R_getEmbeddingDllInfo();
        R_registerRoutines(info, cMethods, callMethods, NULL, NULL);

        // reinstate R method for getting version since we are
        // dynamically loading now, so the version may not be
        // the same as the version we built with.
        R->parseEvalNT("print(R.version.string)");
        QStringList strings = rtool->messages;
        if (strings.count() == 3) {
            QRegExp exp("^.*([0-9]+\\.[0-9]+\\.[0-9]+).*$");
            if (exp.exactMatch(strings[1])) version = exp.cap(1);
            else version = strings[1];
        }
        rtool->messages.clear();

        // load the dynamix library and create function wrapper
        // we should put this into a source file (.R)
        R->parseEvalNT(QString("options(\"repos\"=\"%3\")\n"

                                // graphics device
                               "GC.display <- function() { .Call(\"GC.display\") }\n"
                               "GC.page <- function(width=500, height=500) { .Call(\"GC.page\", width, height) }\n"

                               // athlete
                               "GC.athlete <- function() { .Call(\"GC.athlete\") }\n"

                               // activity
                               "GC.activities <- function(filter=\"\") { .Call(\"GC.activities\", filter) }\n"
                               "GC.activity <- function(activity=0, compare=FALSE) { .Call(\"GC.activity\", activity, compare) }\n"
                               "GC.activity.metrics <- function(compare=FALSE) { .Call(\"GC.activity.metrics\", compare) }\n"
                               "GC.activity.meanmax <- function(compare=FALSE) { .Call(\"GC.activity.meanmax\", compare) }\n"
                               "GC.activity.wbal <- function(compare=FALSE) { .Call(\"GC.activity.wbal\", compare) }\n"

                               // season
                               "GC.season <- function(all=FALSE, compare=FALSE) { .Call(\"GC.season\", all, compare) }\n"
                               "GC.season.metrics <- function(all=FALSE, compare=FALSE) { .Call(\"GC.season.metrics\", all, compare) }\n"
                               "GC.season.pmc <- function(all=FALSE, metric=\"TSS\") { .Call(\"GC.season.pmc\", all, metric) }\n"
                               "GC.season.meanmax <- function(all=FALSE,compare=FALSE) { .Call(\"GC.season.meanmax\", all, compare) }\n"
                               // find peaks does a few validation checks on the R side
                               "GC.season.peaks <- function(all=FALSE, compare=FALSE, series, duration) {\n"
                               "   if (missing(series)) stop(\"series must be specified.\")\n"
                               "   if (missing(duration)) stop(\"duration must be specified.\")\n"
                               "   if (!is.numeric(duration)) stop(\"duration must be numeric.\")\n"
                               "   .Call(\"GC.season.peaks\", all, compare, series, duration)"
                               "}\n"
                               // these 2 added for backward compatibility, may be deprecated
                               "GC.metrics <- function(all=FALSE, compare=FALSE) { .Call(\"GC.season.metrics\", all, compare) }\n"
                               "GC.pmc <- function(all=FALSE, metric=\"TSS\") { .Call(\"GC.season.pmc\", all, metric) }\n"

                               // version and build
                               "GC.version <- function() { return(\"%1\") }\n"
                               "GC.build <- function() { return(%2) }\n"
                               "par.default <- par()\n")
                       .arg(VERSION_STRING)
                       .arg(VERSION_LATEST)
                       .arg("https://cloud.r-project.org/"));

        rtool->messages.clear();

        // set the "GC" object and methods
        context = NULL;
        canvas = NULL;

        configChanged();

    } catch(std::exception& ex) {

        qDebug()<<"Parse error:"  << ex.what();
        failed = true;

    } catch(...) {

        failed = true;
    }

    // ack, disable R runtime
    if (failed) {
        qDebug() << "R Embed failed to start, RConsole disabled.";
        version = "none";
        R = NULL;
    }
    starting = false;
}

void
RTool::configChanged()
{
    // update global R appearances
    QString parCommand=QString("par(par.default)\n"
                               "par(bg=\"%1\", "
                               "    col=\"%2\", "
                               "    fg=\"%2\", "
                               "    col.main=\"%2\", "
                               "    col.sub=\"%3\", "
                               "    col.lab=\"%3\", "
                               "    col.axis=\"%3\")\n"
                               "par.gc <- par()\n"
                            ).arg(GColor(CPLOTBACKGROUND).name())
                             .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name())
                             .arg(GColor(CPLOTMARKER).name());

    // fire and forget, don't care if it fails or not !!
    rtool->R->parseEvalNT(parCommand);
}

SEXP
RTool::athlete()
{
    if (rtool == NULL || rtool->context == NULL)   return Rf_allocVector(INTSXP, 0);

    // name, home, dob, height, weight, gender
    SEXP ans, names;
    PROTECT(ans=Rf_allocList(6));
    PROTECT(names=Rf_allocVector(STRSXP, 6));

    // next and nextS
    SEXP item;
    int next=0;
    SEXP nextS = ans;

    // NAME
    PROTECT(item=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(item, 0, Rf_mkChar(rtool->context->athlete->cyclist.toLatin1().constData()));
    SETCAR(nextS, item); nextS=CDR(nextS);
    SET_STRING_ELT(names, next++, Rf_mkChar("name"));
    UNPROTECT(1);

    // HOME
    PROTECT(item=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(item, 0, Rf_mkChar(rtool->context->athlete->home->root().absolutePath().toLatin1().constData()));
    SETCAR(nextS, item); nextS=CDR(nextS);
    SET_STRING_ELT(names, next++, Rf_mkChar("home"));
    UNPROTECT(1);

    // DOB
    PROTECT(item=Rf_allocVector(INTSXP, 1));
    QDate d1970(1970,01,01);
    INTEGER(item)[0] = d1970.daysTo(appsettings->cvalue(rtool->context->athlete->cyclist, GC_DOB).toDate());
    SEXP dclas;
    PROTECT(dclas=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(dclas, 0, Rf_mkChar("Date"));
    Rf_classgets(item,dclas);
    SETCAR(nextS, item); nextS=CDR(nextS);
    SET_STRING_ELT(names, next++, Rf_mkChar("dob"));
    UNPROTECT(2);

    // WEIGHT
    PROTECT(item=Rf_allocVector(REALSXP, 1));
    REAL(item)[0] = appsettings->cvalue(rtool->context->athlete->cyclist, GC_WEIGHT).toDouble();
    SETCAR(nextS, item); nextS=CDR(nextS);
    SET_STRING_ELT(names, next++, Rf_mkChar("weight"));
    UNPROTECT(1);

    // HEIGHT
    PROTECT(item=Rf_allocVector(REALSXP, 1));
    REAL(item)[0] = appsettings->cvalue(rtool->context->athlete->cyclist, GC_HEIGHT).toDouble();
    SETCAR(nextS, item); nextS=CDR(nextS);
    SET_STRING_ELT(names, next++, Rf_mkChar("height"));
    UNPROTECT(1);

    // GENDER
    int isfemale = appsettings->cvalue(rtool->context->athlete->cyclist, GC_SEX).toInt();
    PROTECT(item=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(item, 0, isfemale ? Rf_mkChar("female") : Rf_mkChar("male"));
    SETCAR(nextS, item); nextS=CDR(nextS);
    SET_STRING_ELT(names, next++, Rf_mkChar("gender"));
    UNPROTECT(1);

    // set the names
    Rf_namesgets(ans, names);

    // ans + names
    UNPROTECT(2);
    return ans;
}

SEXP
RTool::pageSize(SEXP width, SEXP height)
{
    width = Rf_coerceVector(width, INTSXP);
    rtool->width = INTEGER(width)[0];

    height = Rf_coerceVector(height, INTSXP);
    rtool->height = INTEGER(height)[0];

    // fail
    return Rf_allocVector(INTSXP, 0);
}

SEXP
RTool::activities(SEXP filter)
{
    SEXP dates=NULL;
    SEXP clas;


    if (rtool->context && rtool->context->athlete && rtool->context->athlete->rideCache) {

        // filters
        // apply any global filters
        Specification specification;
        FilterSet fs;
        fs.addFilter(rtool->context->isfiltered, rtool->context->filters);
        fs.addFilter(rtool->context->ishomefiltered, rtool->context->homeFilters);

        // did call contain any filters?
        PROTECT(filter=Rf_coerceVector(filter, STRSXP));
        for(int i=0; i<Rf_length(filter); i++) {

            // if not empty write a filter
            QString f(CHAR(STRING_ELT(filter,i)));
            if (f != "") {

                DataFilter dataFilter(rtool->canvas, rtool->context);
                QStringList files;
                dataFilter.parseFilter(rtool->context, f, &files);
                fs.addFilter(true, files);
            }
        }
        specification.setFilterSet(fs);
        UNPROTECT(1);

        // how many pass?
        int count=0;
        foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {

            // apply filters
            if (!specification.pass(item)) continue;
            count++;
        }

        // allocate space for a vector of dates
        PROTECT(dates=Rf_allocVector(REALSXP, count));

        // fill with values for date and class
        int i=0;
        foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {

            // apply filters
            if (!specification.pass(item)) continue;

            // add to the list
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

SEXP
RTool::dfForRideItem(const RideItem *ri)
{
    RideItem *item = const_cast<RideItem*>(ri);

    const RideMetricFactory &factory = RideMetricFactory::instance();
    int rides = rtool->context->athlete->rideCache->count();
    int metrics = factory.metricCount();

    // count the number of meta fields to add
    int meta = 0;
    if (rtool->context && rtool->context->athlete->rideMetadata()) {

        // count active fields
        foreach(FieldDefinition def, rtool->context->athlete->rideMetadata()->getFields()) {
            if (def.name != "" && def.tab != "" && !rtool->context->specialFields.isSpecial(def.name) &&
                !rtool->context->specialFields.isMetric(def.name))
                meta++;
        }
    }

    // just this ride !
    rides = 1;

    // get a listAllocated
    SEXP ans;
    SEXP names; // column names
    SEXP rownames; // row names (numeric)

    // +3 is for date and datetime and color
    PROTECT(ans=Rf_allocList(metrics+meta+3));
    PROTECT(names = Rf_allocVector(STRSXP, metrics+meta+3));

    // we have to give a name to each row
    PROTECT(rownames = Rf_allocVector(STRSXP, rides));
    for(int i=0; i<rides; i++) {
        QString rownumber=QString("%1").arg(i+1);
        SET_STRING_ELT(rownames, i, Rf_mkChar(rownumber.toLatin1().constData()));
    }

    // next name, nextS is next metric
    int next=0;
    SEXP nextS = ans;

    // DATE
    SEXP date;
    PROTECT(date=Rf_allocVector(INTSXP, rides));
    QDate d1970(1970,01,01);
    INTEGER(date)[0] = d1970.daysTo(item->dateTime.date());

    SEXP dclas;
    PROTECT(dclas=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(dclas, 0, Rf_mkChar("Date"));
    Rf_classgets(date,dclas);

    // add to the data.frame and give it a name
    SETCAR(nextS, date); nextS=CDR(nextS);
    SET_STRING_ELT(names, next++, Rf_mkChar("date"));

    // TIME
    SEXP time;
    PROTECT(time=Rf_allocVector(REALSXP, rides));
    REAL(time)[0] = item->dateTime.toUTC().toTime_t();

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
    UNPROTECT(4);

    //
    // METRICS
    //
    for(int i=0; i<factory.metricCount();i++) {

        // set a vector
        SEXP m;
        PROTECT(m=Rf_allocVector(REALSXP, rides));

        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = rtool->context->specialFields.internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        bool useMetricUnits = rtool->context->athlete->useMetricUnits;
        REAL(m)[0] = item->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion()) + (useMetricUnits ? 0.0f : metric->conversionSum());

        // add to the list
        SETCAR(nextS, m);
        nextS = CDR(nextS);

        // give it a name
        SET_STRING_ELT(names, next, Rf_mkChar(name.toLatin1().constData()));

        next++;

        // vector
        UNPROTECT(1);
    }

    //
    // META
    //
    foreach(FieldDefinition field, rtool->context->athlete->rideMetadata()->getFields()) {

        // don't add incomplete meta definitions or metric override fields
        if (field.name == "" || field.tab == "" || rtool->context->specialFields.isSpecial(field.name) ||
            rtool->context->specialFields.isMetric(field.name)) continue;

        // Create a string vector
        SEXP m;
        PROTECT(m=Rf_allocVector(STRSXP, rides));
        SET_STRING_ELT(m, 0, Rf_mkChar(item->getText(field.name, "").toLatin1().constData()));

        // add to the list
        SETCAR(nextS, m);
        nextS = CDR(nextS);

        // give it a name
        SET_STRING_ELT(names, next, Rf_mkChar(field.name.replace(" ","_").toLatin1().constData()));

        next++;

        // vector
        UNPROTECT(1);
    }

    // add Color
    SEXP color;
    PROTECT(color=Rf_allocVector(STRSXP, rides));

    // apply item color, remembering that 1,1,1 means use default (reverse in this case)
    if (item->color == QColor(1,1,1,1)) {

        // use the inverted color, not plot marker as that hideous
        QColor col =GCColor::invertColor(GColor(CPLOTBACKGROUND));

        // white is jarring on a dark background!
        if (col==QColor(Qt::white)) col=QColor(127,127,127);

        SET_STRING_ELT(color, 0, Rf_mkChar(col.name().toLatin1().constData()));
    } else
        SET_STRING_ELT(color, 0, Rf_mkChar(item->color.name().toLatin1().constData()));

    // add to the list and name it
    SETCAR(nextS, color);
    nextS = CDR(nextS);
    SET_STRING_ELT(names, next, Rf_mkChar("color"));
    next++;

    UNPROTECT(1);

    // turn the list into a data frame + set column names
    Rf_setAttrib(ans, R_ClassSymbol, Rf_mkString("data.frame"));
    Rf_setAttrib(ans, R_RowNamesSymbol, rownames);
    Rf_namesgets(ans, names);

    // ans + names
    UNPROTECT(3);

    // return it
    return ans;
}

SEXP
RTool::dfForDateRange(bool all, DateRange range)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    int rides = rtool->context->athlete->rideCache->count();
    int metrics = factory.metricCount();

    // count the number of meta fields to add
    int meta = 0;
    if (rtool->context && rtool->context->athlete->rideMetadata()) {

        // count active fields
        foreach(FieldDefinition def, rtool->context->athlete->rideMetadata()->getFields()) {
            if (def.name != "" && def.tab != "" && !rtool->context->specialFields.isSpecial(def.name) &&
                !rtool->context->specialFields.isMetric(def.name))
                meta++;
        }
    }

    // how many rides to return if we're limiting to the
    // currently selected date range ?

    // apply any global filters
    Specification specification;
    FilterSet fs;
    fs.addFilter(rtool->context->isfiltered, rtool->context->filters);
    fs.addFilter(rtool->context->ishomefiltered, rtool->context->homeFilters);
    specification.setFilterSet(fs);

    // we need to count rides that are in range...
    rides = 0;
    foreach(RideItem *ride, rtool->context->athlete->rideCache->rides()) {
        if (!specification.pass(ride)) continue;
        if (all || range.pass(ride->dateTime.date())) rides++;
    }

    // get a listAllocated
    SEXP ans;
    SEXP names; // column names
    SEXP rownames; // row names (numeric)

    // +3 is for date and datetime and color
    PROTECT(ans=Rf_allocList(metrics+meta+3));
    PROTECT(names = Rf_allocVector(STRSXP, metrics+meta+3));

    // we have to give a name to each row
    PROTECT(rownames = Rf_allocVector(STRSXP, rides));
    for(int i=0; i<rides; i++) {
        QString rownumber=QString("%1").arg(i+1);
        SET_STRING_ELT(rownames, i, Rf_mkChar(rownumber.toLatin1().constData()));
    }

    // next name, nextS is next metric
    int next=0;
    SEXP nextS = ans;

    // DATE
    SEXP date;
    PROTECT(date=Rf_allocVector(INTSXP, rides));

    int k=0;
    QDate d1970(1970,01,01);
    foreach(RideItem *ride, rtool->context->athlete->rideCache->rides()) {
        if (!specification.pass(ride)) continue;
        if (all || range.pass(ride->dateTime.date()))
            INTEGER(date)[k++] = d1970.daysTo(ride->dateTime.date());
    }

    SEXP dclas;
    PROTECT(dclas=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(dclas, 0, Rf_mkChar("Date"));
    Rf_classgets(date,dclas);

    // add to the data.frame and give it a name
    SETCAR(nextS, date); nextS=CDR(nextS);
    SET_STRING_ELT(names, next++, Rf_mkChar("date"));

    // TIME
    SEXP time;
    PROTECT(time=Rf_allocVector(REALSXP, rides));

    // fill with values for date and class if its one we need to return
    k=0;
    foreach(RideItem *ride, rtool->context->athlete->rideCache->rides()) {
        if (!specification.pass(ride)) continue;
        if (all || range.pass(ride->dateTime.date()))
            REAL(time)[k++] = ride->dateTime.toUTC().toTime_t();
    }

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
    UNPROTECT(4);

    //
    // METRICS
    //
    for(int i=0; i<factory.metricCount();i++) {

        // set a vector
        SEXP m;
        PROTECT(m=Rf_allocVector(REALSXP, rides));

        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = rtool->context->specialFields.internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        bool useMetricUnits = rtool->context->athlete->useMetricUnits;

        int index=0;
        foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {
            if (!specification.pass(item)) continue;
            if (all || range.pass(item->dateTime.date())) {
                REAL(m)[index++] = item->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion())
                                                      + (useMetricUnits ? 0.0f : metric->conversionSum());
            }
        }

        // add to the list
        SETCAR(nextS, m);
        nextS = CDR(nextS);

        // give it a name
        SET_STRING_ELT(names, next, Rf_mkChar(name.toLatin1().constData()));

        next++;

        // vector
        UNPROTECT(1);
    }

    //
    // META
    //
    foreach(FieldDefinition field, rtool->context->athlete->rideMetadata()->getFields()) {

        // don't add incomplete meta definitions or metric override fields
        if (field.name == "" || field.tab == "" || rtool->context->specialFields.isSpecial(field.name) ||
            rtool->context->specialFields.isMetric(field.name)) continue;

        // Create a string vector
        SEXP m;
        PROTECT(m=Rf_allocVector(STRSXP, rides));

        int index=0;
        foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {
            if (!specification.pass(item)) continue;
            if (all || range.pass(item->dateTime.date())) {
                SET_STRING_ELT(m, index++, Rf_mkChar(item->getText(field.name, "").toLatin1().constData()));
            }
        }

        // add to the list
        SETCAR(nextS, m);
        nextS = CDR(nextS);

        // give it a name
        SET_STRING_ELT(names, next, Rf_mkChar(field.name.replace(" ","_").toLatin1().constData()));

        next++;

        // vector
        UNPROTECT(1);
    }

    // add Color
    SEXP color;
    PROTECT(color=Rf_allocVector(STRSXP, rides));

    int index=0;
    foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {
        if (!specification.pass(item)) continue;
        if (all || range.pass(item->dateTime.date())) {

            // apply item color, remembering that 1,1,1 means use default (reverse in this case)
            if (item->color == QColor(1,1,1,1)) {

                // use the inverted color, not plot marker as that hideous
                QColor col =GCColor::invertColor(GColor(CPLOTBACKGROUND));

                // white is jarring on a dark background!
                if (col==QColor(Qt::white)) col=QColor(127,127,127);

                SET_STRING_ELT(color, index++, Rf_mkChar(col.name().toLatin1().constData()));
            } else
                SET_STRING_ELT(color, index++, Rf_mkChar(item->color.name().toLatin1().constData()));
        }
    }

    // add to the list and name it
    SETCAR(nextS, color);
    nextS = CDR(nextS);
    SET_STRING_ELT(names, next, Rf_mkChar("color"));
    next++;

    UNPROTECT(1);

    // turn the list into a data frame + set column names
    Rf_setAttrib(ans, R_ClassSymbol, Rf_mkString("data.frame"));
    Rf_setAttrib(ans, R_RowNamesSymbol, rownames);
    Rf_namesgets(ans, names);

    // ans + names
    UNPROTECT(3);

    // return it
    return ans;
}


// returns a data frame of season info
SEXP
RTool::season(SEXP pAll, SEXP pCompare)
{
    // p1 - all=TRUE|FALSE - return all metrics or just within
    //                       the currently selected date range
    pAll = Rf_coerceVector(pAll, LGLSXP);
    bool all = LOGICAL(pAll)[0];

    // p2 - all=TRUE|FALSE - return list of compares (or current if not active)
    pCompare = Rf_coerceVector(pCompare, LGLSXP);
    bool compare = LOGICAL(pCompare)[0];

    // data frame for season: color, name, start, end
    // XXX TODO type needs adding, but we need to unpick the
    //          phase/season object model first, will do later
    SEXP df;
    PROTECT(df=Rf_allocList(4));

    // names
    SEXP names;
    PROTECT(names=Rf_allocVector(STRSXP, 4));
    SET_STRING_ELT(names, 0, Rf_mkChar("name"));
    SET_STRING_ELT(names, 1, Rf_mkChar("start"));
    SET_STRING_ELT(names, 2, Rf_mkChar("end"));
    SET_STRING_ELT(names, 3, Rf_mkChar("color"));

    // worklist of date ranges to return
    // XXX TODO use a Season worklist one the phase/season
    //          object model is fixed
    QList<DateRange> worklist;

    if (compare) {
        // return a list, even if just one
        if (rtool->context->isCompareDateRanges) {
            foreach(CompareDateRange p, rtool->context->compareDateRanges)
                worklist << DateRange(p.start, p.end, p.name, p.color);
        } else {
            // if compare not active just return current selection
            worklist << rtool->context->currentDateRange();
        }

    } else if (all) {
        // list all seasons
        foreach(Season season, rtool->context->athlete->seasons->seasons) {
            worklist << DateRange(season.start, season.end, season.name, QColor(127,127,127));
        }

    } else {

        // just the currently selected season please
        worklist << rtool->context->currentDateRange();
    }

    SEXP rownames, start, end, name, color;
    PROTECT(start=Rf_allocVector(INTSXP, worklist.count()));
    PROTECT(end=Rf_allocVector(INTSXP, worklist.count()));
    PROTECT(name=Rf_allocVector(STRSXP, worklist.count()));
    PROTECT(color=Rf_allocVector(STRSXP, worklist.count()));
    PROTECT(rownames = Rf_allocVector(STRSXP, worklist.count()));

    int index=0;
    QDate d1970(1970,1,1);

    foreach(DateRange p, worklist){

        INTEGER(start) [index] = d1970.daysTo(p.from);
        INTEGER(end) [index] = d1970.daysTo(p.to);
        SET_STRING_ELT(name, index, Rf_mkChar(p.name.toLatin1().constData()));
        SET_STRING_ELT(color, index, Rf_mkChar(p.color.name().toLatin1().constData()));
        QString rownumber=QString("%1").arg(index+1);
        SET_STRING_ELT(rownames, index, Rf_mkChar(rownumber.toLatin1().constData()));

        index++;
    }

    SEXP dclas;
    PROTECT(dclas=Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(dclas, 0, Rf_mkChar("Date"));
    Rf_classgets(start,dclas);
    Rf_classgets(end,dclas);

    SEXP next=df;
    SETCAR(next, name); next=CDR(next);
    SETCAR(next, start); next=CDR(next);
    SETCAR(next, end); next=CDR(next);
    SETCAR(next, color); next=CDR(next);

    // list into a data.frame
    Rf_namesgets(df, names);
    Rf_setAttrib(df, R_RowNamesSymbol, rownames);
    Rf_setAttrib(df, R_ClassSymbol, Rf_mkString("data.frame"));

    UNPROTECT(8); // df + names

    // fail
    return df;
}

SEXP
RTool::metrics(SEXP pAll, SEXP pCompare)
{
    // p1 - all=TRUE|FALSE - return all metrics or just within
    //                       the currently selected date range
    pAll = Rf_coerceVector(pAll, LGLSXP);
    bool all = LOGICAL(pAll)[0];

    // p2 - all=TRUE|FALSE - return list of compares (or current if not active)
    pCompare = Rf_coerceVector(pCompare, LGLSXP);
    bool compare = LOGICAL(pCompare)[0];

    // want a list of compares not a dataframe
    if (compare && rtool->context) {

        // only return compares if its actually active
        if (rtool->context->isCompareDateRanges) {

            // how many to return?
            int count=0;
            foreach(CompareDateRange p, rtool->context->compareDateRanges) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, count));

            // start at the front
            SEXP nextS = list;

            // a named list with data.frame 'metrics' and color 'color'
            SEXP namedlist;

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("metrics"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // create a data.frame for each and add to list
            foreach(CompareDateRange p, rtool->context->compareDateRanges) {
                if (p.isChecked()) {

                    // create a named list
                    PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
                    SEXP offset = namedlist;

                    // add the ride
                    SEXP df = rtool->dfForDateRange(all, DateRange(p.start, p.end));
                    SETCAR(offset, df);
                    offset=CDR(offset);

                    // add the color
                    SEXP color;
                    PROTECT(color=Rf_allocVector(STRSXP, 1));
                    SET_STRING_ELT(color, 0, Rf_mkChar(p.color.name().toLatin1().constData()));
                    SETCAR(offset, color);

                    // name them
                    Rf_namesgets(namedlist, names);

                    // add to back and move on
                    SETCAR(nextS, namedlist);
                    nextS=CDR(nextS);

                    UNPROTECT(2);
                }
            }
            UNPROTECT(2); // list and names

            return list;

        } else { // compare isn't active...

            // otherwise return the current metrics in a compare list
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, 1));

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("metrics"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // named list of metrics and color
            SEXP namedlist;
            PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
            SEXP offset = namedlist;

            // add the metrics
            DateRange range = rtool->context->currentDateRange();
            SEXP df = rtool->dfForDateRange(all, range);
            SETCAR(offset, df);
            offset=CDR(offset);

            // add the color
            SEXP color;
            PROTECT(color=Rf_allocVector(STRSXP, 1));
            SET_STRING_ELT(color, 0, Rf_mkChar("#FF00FF"));
            SETCAR(offset, color);

            // name them
            Rf_namesgets(namedlist, names);

            // add to back and move on
            SETCAR(list, namedlist);
            UNPROTECT(4);

            return list;
        }

    } else if (rtool->context && rtool->context->athlete && rtool->context->athlete->rideCache) {

        // just a datafram of metrics
        DateRange range = rtool->context->currentDateRange();
        return rtool->dfForDateRange(all, range);

    }

    // fail
    return Rf_allocVector(INTSXP, 0);
}

SEXP
RTool::dfForActivity(RideFile *f)
{
    // return a data frame for the ride passed
    SEXP ans;

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
    else return Rf_allocVector(INTSXP, 0);

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

    // add rownames
    SEXP rownames;
    PROTECT(rownames = Rf_allocVector(STRSXP, points));
    for(int i=0; i<points; i++) {
        QString rownumber=QString("%1").arg(i+1);
        SET_STRING_ELT(rownames, i, Rf_mkChar(rownumber.toLatin1().constData()));
    }

    // turn the list into a data frame + set column names
    Rf_setAttrib(ans, R_RowNamesSymbol, rownames);
    Rf_setAttrib(ans, R_ClassSymbol, Rf_mkString("data.frame"));
    Rf_namesgets(ans, names);

    // ans + names + rownames
    UNPROTECT(3);

    // return a valid result
    return ans;
}

QList<RideItem *>
RTool::activitiesFor(SEXP datetime)
{
    QList<RideItem*> returning;

    PROTECT(datetime=Rf_coerceVector(datetime, INTSXP));

    for(int i=0; i<Rf_length(datetime); i++) {

        long dt = INTEGER(datetime)[i];
        if (dt==0) continue;

        // we need to find this one !
        QDateTime asdt = QDateTime::fromTime_t(dt);

        foreach(RideItem*item, rtool->context->athlete->rideCache->rides()) {
            if (item->dateTime.toUTC() == asdt.toUTC()) {
                returning << const_cast<RideItem*>(item);
                break;
            }
        }
    }
    UNPROTECT(1);

    // return a list of activities to process
    return returning;
}

SEXP
RTool::activity(SEXP datetime, SEXP pCompare)
{
    // a dataframe to return
    SEXP ans=NULL;


    // p1 - compare=TRUE|FALSE - return list of compare rides if active, or just current
    pCompare = Rf_coerceVector(pCompare, LGLSXP);
    bool compare = LOGICAL(pCompare)[0];

    // get a list of activitie to return - user specified ALWAYS gets a list
    // even if they only provided a single date to process
    bool userlist = false;
    QList<RideItem*>activities = rtool->activitiesFor(datetime);
    if (activities.count()) userlist=true; // use compare mode code to create a list of rides

    // user requested specific activities?
    if (userlist) {

            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, activities.count()));

            // start at the front
            SEXP nextS = list;

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, activities.count()));

            // create a data.frame for each and add to list
            int index=0;
            foreach(RideItem *item, activities) {

                // give it a name
                SET_STRING_ELT(names, index, Rf_mkChar(QString("%1").arg(index+1).toLatin1().constData()));
                index++;

                // we open, if it wasn't open we also close
                // to make sure we don't exhause memory
                bool close = (item->isOpen() == false);
                SEXP df = rtool->dfForActivity(item->ride());
                if (close) item->close();

                // add to back and move on
                SETCAR(nextS, df);
                nextS=CDR(nextS);

            }

            // we have to give a name to each row
            SEXP rownames;
            PROTECT(rownames = Rf_allocVector(STRSXP, activities.count()));
            for(int i=0; i<activities.count(); i++) {
                QString rownumber=QString("%1").arg(i+1);
                SET_STRING_ELT(rownames, i, Rf_mkChar(rownumber.toLatin1().constData()));
            }

            // turn the list into a data frame + set column names
            Rf_setAttrib(list, R_ClassSymbol, Rf_mkString("data.frame"));
            Rf_setAttrib(list, R_RowNamesSymbol, rownames);
            Rf_namesgets(list, names);

            UNPROTECT(3); // list and names and rownames

            return list;

    } else if (compare && rtool->context) {

        if (rtool->context->isCompareIntervals) {

            // how many to return?
            int count=0;
            foreach(CompareInterval p, rtool->context->compareIntervals) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, count));

            // start at the front
            SEXP nextS = list;

            // a named list with data.frame 'activity' and color 'color'
            SEXP namedlist;

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("activity"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // create a data.frame for each and add to list
            foreach(CompareInterval p, rtool->context->compareIntervals) {
                if (p.isChecked()) {

                    // create a named list
                    PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
                    SEXP offset = namedlist;

                    // add the ride
                    SEXP df = rtool->dfForActivity(p.rideItem->ride());
                    SETCAR(offset, df);
                    offset=CDR(offset);

                    // add the color
                    SEXP color;
                    PROTECT(color=Rf_allocVector(STRSXP, 1));
                    SET_STRING_ELT(color, 0, Rf_mkChar(p.color.name().toLatin1().constData()));
                    SETCAR(offset, color);

                    // name them
                    Rf_namesgets(namedlist, names);

                    // add to back and move on
                    SETCAR(nextS, namedlist);
                    nextS=CDR(nextS);

                    UNPROTECT(2);
                }
            }
            UNPROTECT(2); // list and names

            return list;

        } else if (rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

            // just return a list of one ride
            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, 1));

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("activity"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // named list of activity and color
            SEXP namedlist;
            PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
            SEXP offset = namedlist;

            // add the ride
            RideFile *f = const_cast<RideItem*>(rtool->context->currentRideItem())->ride();
            f->recalculateDerivedSeries();
            SEXP df = rtool->dfForActivity(f);
            SETCAR(offset, df);
            offset=CDR(offset);

            // add the color
            SEXP color;
            PROTECT(color=Rf_allocVector(STRSXP, 1));
            SET_STRING_ELT(color, 0, Rf_mkChar("#FF00FF"));
            SETCAR(offset, color);

            // name them
            Rf_namesgets(namedlist, names);

            // add to back and move on
            SETCAR(list, namedlist);
            UNPROTECT(4);

            return list;
        }

    } else if (!compare) { // not compare, so just return a dataframe

        // access via global as this is a static function
        if(rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

            // get the ride
            RideFile *f = const_cast<RideItem*>(rtool->context->currentRideItem())->ride();
            f->recalculateDerivedSeries();

            // get as a data frame
            ans = rtool->dfForActivity(f);
            return ans;
        }
    }

    // nothing to return
    return Rf_allocVector(INTSXP, 0);
}

SEXP
RTool::dfForActivityMeanmax(const RideItem *i)
{
    return dfForRideFileCache(const_cast<RideItem*>(i)->fileCache());

}

SEXP
RTool::dfForDateRangeMeanmax(bool all, DateRange range)
{
    // construct the date range and then get a ridefilecache
    if (all) range = DateRange(QDate(1900,01,01), QDate(2100,01,01));

    // RideFileCache for a date range with no filtering etc
    RideFileCache cache(rtool->context, range.from, range.to, false, QStringList(), false, NULL);

    return dfForRideFileCache(&cache);

    // nothing to return
    return Rf_allocVector(INTSXP, 0);
}


SEXP
RTool::dfForRideFileCache(RideFileCache *cache)
{
    // how many series and how big are they?
    unsigned int seriescount=0, size=0;

    // get the meanmax array
    if (cache != NULL) {
        // how many points in the ridefilecache and how many series to return
        foreach(RideFile::SeriesType series, cache->meanMaxList()) {
            QVector <double> values = cache->meanMaxArray(series);
            if (values.count()) {
                if (static_cast<unsigned int>(values.count()) > size) size = values.count();
                seriescount++;
            }
        }

    } else {

        // fail
        return Rf_allocVector(INTSXP, 0);
    }

    // we return a list of series vectors
    SEXP ans;
    PROTECT(ans = Rf_allocList(seriescount));

    // we collect the names as we go
    SEXP names;
    PROTECT(names = Rf_allocVector(STRSXP, seriescount)); // names attribute (column names)
    int next=0;
    SEXP nextS = ans;

    //
    // Now we need to add vectors to the ans list...
    //

    foreach(RideFile::SeriesType series, cache->meanMaxList()) {

        QVector <double> values = cache->meanMaxArray(series);

        // don't add empty ones
        if (values.count()==0) continue;


        // set a vector
        SEXP vector;
        PROTECT(vector=Rf_allocVector(REALSXP, values.count()));

        // will have different sizes e.g. when a daterange
        // since longest ride with e.g. power may be different
        // to longest ride with heartrate
        for(int j=0; j<values.count(); j++) REAL(vector)[j] = values[j];

        // add to the list
        SETCAR(nextS, vector);
        nextS = CDR(nextS);

        // give it a name
        SET_STRING_ELT(names, next, Rf_mkChar(RideFile::seriesName(series, true).toLatin1().constData()));

        next++;

        // vector
        UNPROTECT(1);
    }

    // add rownames
    SEXP rownames;
    PROTECT(rownames = Rf_allocVector(STRSXP, size));
    for(unsigned int i=0; i<size; i++) {
        QString rownumber=QString("%1").arg(i+1);
        SET_STRING_ELT(rownames, i, Rf_mkChar(rownumber.toLatin1().constData()));
    }

    // turn the list into a data frame + set column names
    Rf_setAttrib(ans, R_RowNamesSymbol, rownames);
    //Rf_setAttrib(ans, R_ClassSymbol, Rf_mkString("data.frame"));
    Rf_namesgets(ans, names);

    // ans + names + rownames
    UNPROTECT(3);

    // return a valid result
    return ans;
}

SEXP
RTool::seasonPeaks(SEXP pAll, SEXP pCompare, SEXP pSeries, SEXP pDuration)
{
    // check parameters !
    pAll = Rf_coerceVector(pAll, LGLSXP);
    bool all = LOGICAL(pAll)[0];

    pCompare = Rf_coerceVector(pCompare, LGLSXP);
    bool compare = LOGICAL(pCompare)[0];

    // lets get a Map of names to series
    QMap<QString, RideFile::SeriesType> snames;
    foreach(RideFile::SeriesType s, RideFileCache::meanMaxList()) {
        snames.insert(RideFile::seriesName(s, true), s);
    }

    // extract as QStrings
    QList<RideFile::SeriesType> series;
    pSeries = Rf_coerceVector(pSeries, STRSXP);
    for(int i=0; i <Rf_length(pSeries); i++) {
        QString name = CHAR(STRING_ELT(pSeries, i));
        RideFile::SeriesType stype;
        if ((stype=snames.value(name, RideFile::none)) == RideFile::none) {

            Rf_error("Invalid mean maximal series passed to GC.season.peaks.");
            return Rf_allocVector(INTSXP, 0);

        } else {
            series << stype;
        }
    }

    // extract as integers
    QList<int>durations;
    pDuration = Rf_coerceVector(pDuration, REALSXP);
    for(int i=0; i<Rf_length(pDuration); i++) {
        durations << REAL(pDuration)[i];
    }

    // want a list of compares not a dataframe
    if (compare && rtool->context) {

        // only return compares if its actually active
        if (rtool->context->isCompareDateRanges) {

            // how many to return?
            int count=0;
            foreach(CompareDateRange p, rtool->context->compareDateRanges) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, count));

            // start at the front
            SEXP nextS = list;

            // a named list with data.frame 'metrics' and color 'color'
            SEXP namedlist;

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("peaks"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // create a data.frame for each and add to list
            foreach(CompareDateRange p, rtool->context->compareDateRanges) {
                if (p.isChecked()) {

                    // create a named list
                    PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
                    SEXP offset = namedlist;

                    // add the ride
                    SEXP df = rtool->dfForDateRangePeaks(all, DateRange(p.start, p.end), series, durations);
                    SETCAR(offset, df);
                    offset=CDR(offset);

                    // add the color
                    SEXP color;
                    PROTECT(color=Rf_allocVector(STRSXP, 1));
                    SET_STRING_ELT(color, 0, Rf_mkChar(p.color.name().toLatin1().constData()));
                    SETCAR(offset, color);

                    // name them
                    Rf_namesgets(namedlist, names);

                    // add to back and move on
                    SETCAR(nextS, namedlist);
                    nextS=CDR(nextS);

                    UNPROTECT(2);
                }
            }
            UNPROTECT(2); // list and names

            return list;

        } else { // compare isn't active...

            // otherwise return the current metrics in a compare list
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, 1));

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("peaks"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // named list of metrics and color
            SEXP namedlist;
            PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
            SEXP offset = namedlist;

            // add the metrics
            DateRange range = rtool->context->currentDateRange();
            SEXP df = rtool->dfForDateRangePeaks(all, range, series, durations);
            SETCAR(offset, df);
            offset=CDR(offset);

            // add the color
            SEXP color;
            PROTECT(color=Rf_allocVector(STRSXP, 1));
            SET_STRING_ELT(color, 0, Rf_mkChar("#FF00FF"));
            SETCAR(offset, color);

            // name them
            Rf_namesgets(namedlist, names);

            // add to back and move on
            SETCAR(list, namedlist);
            UNPROTECT(4);

            return list;
        }

    } else if (rtool->context && rtool->context->athlete && rtool->context->athlete->rideCache) {

        // just a datafram of metrics
        DateRange range = rtool->context->currentDateRange();
        return rtool->dfForDateRangePeaks(all, range, series, durations);

    }

    // fail
    return Rf_allocVector(INTSXP, 0);
}

SEXP
RTool::dfForDateRangePeaks(bool all, DateRange range, QList<RideFile::SeriesType> series, QList<int> durations)
{
    // so how many vectors in the frame ? +1 is the datetime of the peak
    int listsize=series.count() * durations.count() + 1;
    SEXP df;
    PROTECT(df=Rf_allocList(listsize));
    SEXP nextS=df;

    // and each one needs a name
    SEXP names;
    PROTECT(names=Rf_allocVector(STRSXP, listsize));
    SET_STRING_ELT(names, 0, Rf_mkChar("time"));
    int next=1;

    // how many rides ?
    Specification specification;
    FilterSet fs;
    fs.addFilter(rtool->context->isfiltered, rtool->context->filters);
    fs.addFilter(rtool->context->ishomefiltered, rtool->context->homeFilters);
    specification.setFilterSet(fs);

    // how many pass?
    int size=0;
    foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {

        // apply filters
        if (!specification.pass(item)) continue;

        // do we want this one ?
        if (all || range.pass(item->dateTime.date()))  size++;
    }


    // dates first
    SEXP dates;
    PROTECT(dates=Rf_allocVector(REALSXP, size));

    // fill with values for date and class
    int i=0;
    foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {
        // apply filters
        if (!specification.pass(item)) continue;

        if (all || range.pass(item->dateTime.date())) {
            REAL(dates)[i++] = item->dateTime.toUTC().toTime_t();
        }
    }

    // POSIXct class
    SEXP clas;
    PROTECT(clas=Rf_allocVector(STRSXP, 2));
    SET_STRING_ELT(clas, 0, Rf_mkChar("POSIXct"));
    SET_STRING_ELT(clas, 1, Rf_mkChar("POSIXt"));
    Rf_classgets(dates,clas);
    Rf_setAttrib(dates, Rf_install("tzone"), Rf_mkString("UTC"));

    SETCAR(nextS, dates);
    nextS=CDR(nextS);

    foreach(RideFile::SeriesType pseries, series) {

        foreach(int pduration, durations) {

            // create a vector
            SEXP vector;
            PROTECT(vector=Rf_allocVector(REALSXP, size));

            // give it a name
            QString name = QString("peak_%1_%2").arg(RideFile::seriesName(pseries, true)).arg(pduration);
            SET_STRING_ELT(names, next++, Rf_mkChar(name.toLatin1().constData()));

            // fill with values
            // get the value for the series and duration requested, although this is called
            int index=0;
            foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {

                // apply filters
                if (!specification.pass(item)) continue;

                // do we want this one ?
                if (all || range.pass(item->dateTime.date())) {

                    // for each series/duration independently its pretty quick since it lseeks to
                    // the actual value, so /should't/ be too expensive.........
                    REAL(vector)[index++] = RideFileCache::best(item->context, item->fileName, pseries, pduration);
                }
            }

            // add named vector to the list
            SETCAR(nextS, vector);
            nextS=CDR(nextS);

            UNPROTECT(1);

        }
    }

    // set names + data.frame
    SEXP rownames;
    PROTECT(rownames = Rf_allocVector(STRSXP, size));
    for(int i=0; i<size; i++) {
        QString rownumber=QString("%1").arg(i+1);
        SET_STRING_ELT(rownames, i, Rf_mkChar(rownumber.toLatin1().constData()));
    }

    // turn the list into a data frame + set column names
    Rf_setAttrib(df, R_ClassSymbol, Rf_mkString("data.frame"));
    Rf_setAttrib(df, R_RowNamesSymbol, rownames);
    Rf_namesgets(df, names);


    // df + names + dates + clas + rownames
    UNPROTECT(5);
    return df;
}

SEXP
RTool::seasonMeanmax(SEXP pAll, SEXP pCompare)
{
    // p1 - all=TRUE|FALSE - return all metrics or just within
    //                       the currently selected date range
    pAll = Rf_coerceVector(pAll, LGLSXP);
    bool all = LOGICAL(pAll)[0];

    // p2 - all=TRUE|FALSE - return list of compares (or current if not active)
    pCompare = Rf_coerceVector(pCompare, LGLSXP);
    bool compare = LOGICAL(pCompare)[0];

    // want a list of compares not a dataframe
    if (compare && rtool->context) {

        // only return compares if its actually active
        if (rtool->context->isCompareDateRanges) {

            // how many to return?
            int count=0;
            foreach(CompareDateRange p, rtool->context->compareDateRanges) if (p.isChecked()) count++;

            // cool we can return a list of meanaxes to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, count));

            // start at the front
            SEXP nextS = list;

            // a named list with data.frame 'metrics' and color 'color'
            SEXP namedlist;

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("meanmax"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // create a data.frame for each and add to list
            foreach(CompareDateRange p, rtool->context->compareDateRanges) {
                if (p.isChecked()) {

                    // create a named list
                    PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
                    SEXP offset = namedlist;

                    // add the ride
                    SEXP df = rtool->dfForDateRangeMeanmax(all, DateRange(p.start, p.end));
                    SETCAR(offset, df);
                    offset=CDR(offset);

                    // add the color
                    SEXP color;
                    PROTECT(color=Rf_allocVector(STRSXP, 1));
                    SET_STRING_ELT(color, 0, Rf_mkChar(p.color.name().toLatin1().constData()));
                    SETCAR(offset, color);

                    // name them
                    Rf_namesgets(namedlist, names);

                    // add to back and move on
                    SETCAR(nextS, namedlist);
                    nextS=CDR(nextS);

                    UNPROTECT(2);
                }
            }
            UNPROTECT(2); // list and names

            return list;

        } else { // compare isn't active...

            // otherwise return the current season meanmax in a compare list
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, 1));

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("meanmax"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // named list of metrics and color
            SEXP namedlist;
            PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
            SEXP offset = namedlist;

            // add the meanmaxes
            DateRange range = rtool->context->currentDateRange();
            SEXP df = rtool->dfForDateRangeMeanmax(all, range);
            SETCAR(offset, df);
            offset=CDR(offset);

            // add the color
            SEXP color;
            PROTECT(color=Rf_allocVector(STRSXP, 1));
            SET_STRING_ELT(color, 0, Rf_mkChar("#FF00FF"));
            SETCAR(offset, color);

            // name them
            Rf_namesgets(namedlist, names);

            // add to back and move on
            SETCAR(list, namedlist);
            UNPROTECT(4);

            return list;
        }

    } else if (rtool->context && rtool->context->athlete && rtool->context->athlete->rideCache) {

        // just a datafram of meanmax
        DateRange range = rtool->context->currentDateRange();
        return rtool->dfForDateRangeMeanmax(all, range);

    }

    // fail
    return Rf_allocVector(INTSXP, 0);
}


SEXP
RTool::activityMeanmax(SEXP pCompare)
{
    // a dataframe to return
    SEXP ans=NULL;

    // p1 - compare=TRUE|FALSE - return list of compare rides if active, or just current
    pCompare = Rf_coerceVector(pCompare, LGLSXP);
    bool compare = LOGICAL(pCompare)[0];

    // return a list
    if (compare && rtool->context) {


        if (rtool->context->isCompareIntervals) {

            // how many to return?
            int count=0;
            foreach(CompareInterval p, rtool->context->compareIntervals) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, count));

            // start at the front
            SEXP nextS = list;

            // a named list with data.frame 'activity' and color 'color'
            SEXP namedlist;

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("meanmax"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // create a data.frame for each and add to list
            foreach(CompareInterval p, rtool->context->compareIntervals) {
                if (p.isChecked()) {

                    // create a named list
                    PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
                    SEXP offset = namedlist;

                    // add the ride
                    SEXP df = rtool->dfForActivityMeanmax(p.rideItem);
                    SETCAR(offset, df);
                    offset=CDR(offset);

                    // add the color
                    SEXP color;
                    PROTECT(color=Rf_allocVector(STRSXP, 1));
                    SET_STRING_ELT(color, 0, Rf_mkChar(p.color.name().toLatin1().constData()));
                    SETCAR(offset, color);

                    // name them
                    Rf_namesgets(namedlist, names);

                    // add to back and move on
                    SETCAR(nextS, namedlist);
                    nextS=CDR(nextS);

                    UNPROTECT(2);
                }
            }
            UNPROTECT(2); // list and names

            return list;

        } else if(rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

            // just return a list of one ride
            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, 1));

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("meanmax"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // named list of activity and color
            SEXP namedlist;
            PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
            SEXP offset = namedlist;

            // add the ride
            SEXP df = rtool->dfForActivityMeanmax(rtool->context->currentRideItem());
            SETCAR(offset, df);
            offset=CDR(offset);

            // add the color
            SEXP color;
            PROTECT(color=Rf_allocVector(STRSXP, 1));
            SET_STRING_ELT(color, 0, Rf_mkChar("#FF00FF"));
            SETCAR(offset, color);

            // name them
            Rf_namesgets(namedlist, names);

            // add to back and move on
            SETCAR(list, namedlist);
            UNPROTECT(4);

            return list;
        }

    } else if (!compare) { // not compare, so just return a dataframe

        // access via global as this is a static function
        if(rtool->context && rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

            // get as a data frame
            ans = rtool->dfForActivityMeanmax(rtool->context->currentRideItem());
            return ans;
        }
    }

    // nothing to return
    return Rf_allocVector(INTSXP, 0);
}

SEXP
RTool::activityMetrics(SEXP pCompare)
{
    // a dataframe to return
    SEXP ans=NULL;

    // p1 - compare=TRUE|FALSE - return list of compare rides if active, or just current
    pCompare = Rf_coerceVector(pCompare, LGLSXP);
    bool compare = LOGICAL(pCompare)[0];

    // return a list
    if (compare && rtool->context) {


        if (rtool->context->isCompareIntervals) {

            // how many to return?
            int count=0;
            foreach(CompareInterval p, rtool->context->compareIntervals) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, count));

            // start at the front
            SEXP nextS = list;

            // a named list with data.frame 'activity' and color 'color'
            SEXP namedlist;

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("metrics"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // create a data.frame for each and add to list
            foreach(CompareInterval p, rtool->context->compareIntervals) {
                if (p.isChecked()) {

                    // create a named list
                    PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
                    SEXP offset = namedlist;

                    // add the ride
                    SEXP df = rtool->dfForRideItem(p.rideItem);
                    SETCAR(offset, df);
                    offset=CDR(offset);

                    // add the color
                    SEXP color;
                    PROTECT(color=Rf_allocVector(STRSXP, 1));
                    SET_STRING_ELT(color, 0, Rf_mkChar(p.color.name().toLatin1().constData()));
                    SETCAR(offset, color);

                    // name them
                    Rf_namesgets(namedlist, names);

                    // add to back and move on
                    SETCAR(nextS, namedlist);
                    nextS=CDR(nextS);

                    UNPROTECT(2);
                }
            }
            UNPROTECT(2); // list and names

            return list;

        } else if(rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

            // just return a list of one ride
            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, 1));

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("metrics"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // named list of activity and color
            SEXP namedlist;
            PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
            SEXP offset = namedlist;

            // add the ride
            SEXP df = rtool->dfForRideItem(rtool->context->currentRideItem());
            SETCAR(offset, df);
            offset=CDR(offset);

            // add the color
            SEXP color;
            PROTECT(color=Rf_allocVector(STRSXP, 1));
            SET_STRING_ELT(color, 0, Rf_mkChar("#FF00FF"));
            SETCAR(offset, color);

            // name them
            Rf_namesgets(namedlist, names);

            // add to back and move on
            SETCAR(list, namedlist);
            UNPROTECT(4);

            return list;
        }

    } else if (!compare) { // not compare, so just return a dataframe

        // access via global as this is a static function
        if(rtool->context && rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

            // get as a data frame
            ans = rtool->dfForRideItem(rtool->context->currentRideItem());
            return ans;
        }
    }

    // nothing to return
    return Rf_allocVector(INTSXP, 0);
}

SEXP
RTool::pmc(SEXP pAll, SEXP pMetric)
{
    // parse parameters
    // p1 - all=TRUE|FALSE - return all metrics or just within
    //                       the currently selected date range
    pAll = Rf_coerceVector(pAll, LGLSXP);
    bool all = LOGICAL(pAll)[0];

    // get the currently selected date range
    DateRange range(rtool->context->currentDateRange());

    // p2 - all=TRUE|FALSE - return list of compares (or current if not active)
    pMetric = Rf_coerceVector(pMetric, STRSXP);
    QString metric (CHAR(STRING_ELT(pMetric,0)));

    // return a dataframe with PMC data for all or the current season
    // XXX uses the default half-life
    if (rtool->context) {

        // convert the name to a symbol, if not found just leave as it is
        const RideMetricFactory &factory = RideMetricFactory::instance();
        for (int i=0; i<factory.metricCount(); i++) {
            QString symbol = factory.metricName(i);
            QString name = rtool->context->specialFields.internalName(factory.rideMetric(symbol)->name());
            name.replace(" ","_");

            if (name == metric) {
                metric = symbol;
                break;
            }
        }

        // create the data
        PMCData pmcData(rtool->context, Specification(), metric);

        // how many entries ?
        QDate d1970(1970,01,01);

        // not unsigned coz date could be configured < 1970 (!)
        int from =d1970.daysTo(range.from);
        int to =d1970.daysTo(range.to);
        unsigned int size = all ? pmcData.days() : (to - from + 1);

        // returning a dataframe with
        // date, lts, sts, sb, rr
        SEXP ans, names;

        // date, stress, lts, sts, sb, rr
        PROTECT(ans=Rf_allocList(6));
        SEXP nextS = ans;

        // set ther names
        PROTECT(names = Rf_allocVector(STRSXP, 6));
        SET_STRING_ELT(names, 0, Rf_mkChar("date"));
        SET_STRING_ELT(names, 1, Rf_mkChar("stress"));
        SET_STRING_ELT(names, 2, Rf_mkChar("lts"));
        SET_STRING_ELT(names, 3, Rf_mkChar("sts"));
        SET_STRING_ELT(names, 4, Rf_mkChar("sb"));
        SET_STRING_ELT(names, 5, Rf_mkChar("rr"));

        // DATE - 1 a day from start
        SEXP date;
        PROTECT(date=Rf_allocVector(INTSXP, size));
        unsigned int start = d1970.daysTo(all ? pmcData.start() : range.from);
        for(unsigned int k=0; k<size; k++) INTEGER(date)[k] = start + k;

        SEXP dclas;
        PROTECT(dclas=Rf_allocVector(STRSXP, 1));
        SET_STRING_ELT(dclas, 0, Rf_mkChar("Date"));
        Rf_classgets(date,dclas);

        // add to the data.frame
        SETCAR(nextS, date); nextS=CDR(nextS);

        // PMC DATA

        SEXP stress, lts, sts, sb, rr;
        PROTECT(stress=Rf_allocVector(REALSXP, size));
        PROTECT(lts=Rf_allocVector(REALSXP, size));
        PROTECT(sts=Rf_allocVector(REALSXP, size));
        PROTECT(sb=Rf_allocVector(REALSXP, size));
        PROTECT(rr=Rf_allocVector(REALSXP, size));

            int index=0;
        if (all) {

            // just copy
            for(unsigned int k=0; k<size; k++)  REAL(stress)[k] = pmcData.stress()[k];
            for(unsigned int k=0; k<size; k++)  REAL(lts)[k] = pmcData.lts()[k];
            for(unsigned int k=0; k<size; k++)  REAL(sts)[k] = pmcData.sts()[k];
            for(unsigned int k=0; k<size; k++)  REAL(sb)[k] = pmcData.sb()[k];
            for(unsigned int k=0; k<size; k++)  REAL(rr)[k] = pmcData.rr()[k];

        } else {

            int day = d1970.daysTo(pmcData.start());
            for(int k=0; k < pmcData.days(); k++) {

                // day today
                if (day >= from && day <= to) {

                    REAL(stress)[index] = pmcData.stress()[k];
                    REAL(lts)[index] = pmcData.lts()[k];
                    REAL(sts)[index] = pmcData.sts()[k];
                    REAL(sb)[index] = pmcData.sb()[k];
                    REAL(rr)[index] = pmcData.rr()[k];
                    index++;
                }
                day++;
            }
        }

        // add to the list
        SETCAR(nextS, stress); nextS = CDR(nextS);
        SETCAR(nextS, lts); nextS = CDR(nextS);
        SETCAR(nextS, sts); nextS = CDR(nextS);
        SETCAR(nextS, sb); nextS = CDR(nextS);
        SETCAR(nextS, rr); nextS = CDR(nextS);

        SEXP rownames;
        PROTECT(rownames = Rf_allocVector(STRSXP, size));
        for(unsigned int i=0; i<size; i++) {
            QString rownumber=QString("%1").arg(i+1);
            SET_STRING_ELT(rownames, i, Rf_mkChar(rownumber.toLatin1().constData()));
        }

        // turn the list into a data frame + set column names
        Rf_setAttrib(ans, R_ClassSymbol, Rf_mkString("data.frame"));
        Rf_setAttrib(ans, R_RowNamesSymbol, rownames);
        Rf_namesgets(ans, names);

        UNPROTECT(10);

        // return it
        return ans;
    }

    // nothing to return
    return Rf_allocVector(INTSXP, 0);
}

SEXP
RTool::activityWBal(SEXP pCompare)
{
    SEXP ans=NULL;

    // p1 - compare=TRUE|FALSE - return list of compare rides if active, or just current
    pCompare = Rf_coerceVector(pCompare, LGLSXP);
    bool compare = LOGICAL(pCompare)[0];

    // return a list
    if (compare && rtool->context) {


        if (rtool->context->isCompareIntervals) {

            // how many to return?
            int count=0;
            foreach(CompareInterval p, rtool->context->compareIntervals) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, count));

            // start at the front
            SEXP nextS = list;

            // a named list with data.frame 'activity' and color 'color'
            SEXP namedlist;

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("activity"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // create a data.frame for each and add to list
            foreach(CompareInterval p, rtool->context->compareIntervals) {
                if (p.isChecked()) {

                    // create a named list
                    PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
                    SEXP offset = namedlist;

                    // add the ride
                    SEXP df = rtool->dfForActivityWBal(p.rideItem->ride());
                    SETCAR(offset, df);
                    offset=CDR(offset);

                    // add the color
                    SEXP color;
                    PROTECT(color=Rf_allocVector(STRSXP, 1));
                    SET_STRING_ELT(color, 0, Rf_mkChar(p.color.name().toLatin1().constData()));
                    SETCAR(offset, color);

                    // name them
                    Rf_namesgets(namedlist, names);

                    // add to back and move on
                    SETCAR(nextS, namedlist);
                    nextS=CDR(nextS);

                    UNPROTECT(2);
                }
            }
            UNPROTECT(2); // list and names

            return list;

        } else if(rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

            // just return a list of one ride
            // cool we can return a list of intervals to compare
            SEXP list;
            PROTECT(list=Rf_allocVector(LISTSXP, 1));

            // names
            SEXP names;
            PROTECT(names=Rf_allocVector(STRSXP, 2));
            SET_STRING_ELT(names, 0, Rf_mkChar("activity"));
            SET_STRING_ELT(names, 1, Rf_mkChar("color"));

            // named list of activity and color
            SEXP namedlist;
            PROTECT(namedlist=Rf_allocVector(LISTSXP, 2));
            SEXP offset = namedlist;

            // add the ride
            RideFile *f = const_cast<RideItem*>(rtool->context->currentRideItem())->ride();
            f->recalculateDerivedSeries();
            SEXP df = rtool->dfForActivityWBal(f);
            SETCAR(offset, df);
            offset=CDR(offset);

            // add the color
            SEXP color;
            PROTECT(color=Rf_allocVector(STRSXP, 1));
            SET_STRING_ELT(color, 0, Rf_mkChar("#FF00FF"));
            SETCAR(offset, color);

            // name them
            Rf_namesgets(namedlist, names);

            // add to back and move on
            SETCAR(list, namedlist);
            UNPROTECT(4);

            return list;
        }

    } else if (!compare) { // not compare, so just return a dataframe

        // access via global as this is a static function
        if(rtool->context && rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

            // get the ride
            RideFile *f = const_cast<RideItem*>(rtool->context->currentRideItem())->ride();
            f->recalculateDerivedSeries();

            // get as a data frame
            ans = rtool->dfForActivityWBal(f);
            return ans;
        }
    }

    // nothing to return
    return Rf_allocVector(INTSXP, 0);
}

SEXP
RTool::dfForActivityWBal(RideFile*f)
{
    // return a data frame with wpbal in 1s samples
    if(f && f->wprimeData()) {

        // get as a data frame
        WPrime *w = f->wprimeData();

        if (w && w->ydata().count() >0) {

                // construct a vector
                SEXP ans;
                PROTECT(ans=Rf_allocVector(REALSXP, w->ydata().count()));

                // add values
                for(int i=0; i<w->ydata().count(); i++) REAL(ans)[i] = w->ydata()[i];

                UNPROTECT(1);

                return ans;
        }
    }

    // nothing to return
    return Rf_allocVector(INTSXP, 0);
}
