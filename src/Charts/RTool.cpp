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

        // set the "GC" object and methods
        context = NULL;
        canvas = NULL;
        (*R)["GC.version"] = VERSION_STRING;
        (*R)["GC.build"] = VERSION_LATEST;

        // Access into the GC data
        (*R)["GC.activities"] = Rcpp::InternalFunction(RTool::activities);
        (*R)["GC.activity"] = Rcpp::InternalFunction(RTool::activity);

        // All metrics
        (*R)["GC.metrics"] = Rcpp::InternalFunction(RTool::metrics);

        // TBD
        // GC.metrics     - list of activities and their metrics
        // GC.activity    - single activity and its data series
        // GC.seasons     - configured seasons
        // GC.season      - currently selected season
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

Rcpp::DatetimeVector
RTool::activities()
{

    if (rtool->context && rtool->context->athlete && rtool->context->athlete->rideCache) {

        // allocate
        Rcpp::DatetimeVector l(rtool->context->athlete->rideCache->count());

        // fill
        int i=0;
        foreach(RideItem *item, rtool->context->athlete->rideCache->rides()) {
            l[i++] = Rcpp::Datetime(item->dateTime.toUTC().toTime_t());
        }

        return l;
    }
    return Rcpp::DatetimeVector(0);
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

Rcpp::DataFrame
RTool::activity()
{
    Rcpp::DataFrame d;

    // access via global as this is a static function
    if(rtool->context && rtool->context->currentRideItem() && const_cast<RideItem*>(rtool->context->currentRideItem())->ride()) {

        // get the ride
        RideFile *f = const_cast<RideItem*>(rtool->context->currentRideItem())->ride();
        f->recalculateDerivedSeries();
        int points = f->dataPoints().count();

        // add in actual time in POSIXct format (via Rcpp::Datetime)
        Rcpp::DatetimeVector time(points);
        for(int k=0; k<points; k++)
            time[k] = Rcpp::Datetime(f->startTime().addSecs(f->dataPoints()[k]->secs).toUTC().toTime_t());
        d["time"] = time;

        // now run through each data series adding to the frame, if the
        // series does not exist we set all values to NA
        for(int i=0; i < static_cast<int>(RideFile::none); i++) {

            // what series we working with?
            RideFile::SeriesType series = static_cast<RideFile::SeriesType>(i);

            // lets not add lots of NA for the more obscure data series
            if (i > 15 && !f->isDataPresent(series)) continue;

            // set a vector
            Rcpp::NumericVector vector(points);
            for(int j=0; j<points; j++) {
                if (f->isDataPresent(series)) {
                    if (f->dataPoints()[j]->value(series) == 0 && (series == RideFile::lat || series == RideFile::lon))
                        vector[j] = NA_REAL;
                    else
                        vector[j] = f->dataPoints()[j]->value(series);
                } else {
                    vector[j] = NA_REAL;
                }
            }

            // use the compatability 'name' to work with e.g. R package trackeR
            d[RideFile::seriesName(series, true).toStdString()] = vector;
        }

    }

    // d is basically a list, this is about the only way to really
    // return a valid data.frame
    Rcpp::DataFrame returning(d);
    return returning;
}
