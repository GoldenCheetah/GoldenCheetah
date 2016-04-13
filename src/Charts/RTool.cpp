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
#include "GcUpgrade.h"

#include "RideItem.h"
#include "RideFile.h"

// If no callbacks we won't play
#if !defined(RINSIDE_CALLBACKS)
#error "GC_WANT_R: RInside must have callbacks enabled (see inst/RInsideConfig.h)"
#endif

RTool::RTool(int argc, char**argv)
{
    // setup the R runtime elements
    R = new RInside(argc,argv);
    callbacks = new RCallbacks;
    R->set_callbacks(callbacks);

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
    (*R)["GC.version"] = VERSION_STRING;
    (*R)["GC.build"] = VERSION_LATEST;

    // Access into the GC data
    (*R)["GC.activity"] = Rcpp::InternalFunction(RTool::activity);

    // TBD
    // GC.metrics     - list of activities and their metrics
    // GC.activity    - single activity and its data series
    // GC.seasons     - configured seasons
    // GC.season      - currently selected season
    // GC.config      - configuration (zones, units etc)

    // the following are already set in RChart on a per call basis
    // "GC.athlete" "GC.athlete.home"

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
            time[k] = Rcpp::Datetime(f->startTime().addSecs(f->dataPoints()[k]->secs).toTime_t());
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
                    vector[j] = f->dataPoints()[j]->value(series);
                } else {
                    vector[j] = NA_REAL;
                }
            }

            // use the compatability 'name' to work with e.g. R package trackeR
            d[RideFile::seriesName(series, true).toStdString()] = vector;
        }

    }
    return d;
}
