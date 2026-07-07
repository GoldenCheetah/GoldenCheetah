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

#include "RChart.h"
#include "Context.h"

#ifndef _GC_RTool_h

class RGraphicsDevice;
class RTool;
extern RTool *rtool;

class RTool {


    public:
        RTool();
        void  configChanged();

        REmbed *R;
        RGraphicsDevice *dev;

        // the canvas to plot on, it may be null
        // if no canvas is active
        RCanvas *canvas;
        Perspective *perspective;
        RChart *chart;

        Context *context;
        QString version;

        // layout and page size
        static SEXP windowSize();
        static SEXP pageSize(SEXP width, SEXP height);

        // athlete
        static SEXP athlete();
        static SEXP zones(SEXP date, SEXP sport);

        // intervals
        static SEXP intervalType(SEXP type);

        // activities
        static SEXP activities(SEXP filter);
        static SEXP activity(SEXP datetime, SEXP compare, SEXP split, SEXP join);
        static SEXP activityMeanmax(SEXP compare);
        static SEXP activityWBal(SEXP compare);
        static SEXP activityXData(SEXP name, SEXP compare);
        static SEXP activityMetrics(SEXP compare);
        static SEXP activityIntervals(SEXP type, SEXP datetime);

        // seasons
        static SEXP season(SEXP all, SEXP compare);
        static SEXP metrics(SEXP all, SEXP filter, SEXP compare);
        static SEXP seasonIntervals(SEXP type, SEXP compare);
        static SEXP seasonMeanmax(SEXP all, SEXP filter, SEXP compare);
        static SEXP seasonPeaks(SEXP all, SEXP filter, SEXP compare, SEXP series, SEXP duration);
        static SEXP pmc(SEXP all, SEXP metric, SEXP type);
        static SEXP measures(SEXP all, SEXP group);

        // charts
        static SEXP setChart(SEXP title, SEXP type, SEXP animate, SEXP legpos, SEXP stack, SEXP orientation);
        static SEXP addCurve(SEXP name, SEXP xseries, SEXP yseries, SEXP fseries, SEXP xname, SEXP yname, SEXP labels, SEXP colors,
                             SEXP line, SEXP symbol, SEXP size, SEXP color, SEXP opacity, SEXP opengl, SEXP legend, SEXP datalabels, SEXP fill);
        static SEXP configureAxis(SEXP name, SEXP visible, SEXP align, SEXP min, SEXP max,
                                  SEXP type, SEXP labelcolor, SEXP color, SEXP log, SEXP categories);

        static SEXP annotate(SEXP type, SEXP p1, SEXP p2, SEXP p3);

        bool starting;
        bool failed;
        bool cancelled;

        int width, height;

        // handling console output from the R runtime
        static void R_Suicide(const char *) {}
        static void R_ShowMessage(const char *text) { rtool->messages << QString(text); }
        // yep. Windows and Unix definitions in RStartup/Rinterface are different
        //      the R codebase is a mess.
        static int R_ReadConsole(const char *, unsigned char *, int, int) { return 0; }
        static int R_ReadConsoleWin(const char *, char *, int, int) { return 0; }
        static void R_WriteConsole(const char *text, int) { rtool->messages << QString(text); }
        static void R_WriteConsoleEx(const char *text, int, int) { rtool->messages << QString(text); }
        static void R_ResetConsole() { }
        static void R_FlushConsole() { }
        static void R_ClearerrConsole() { }
        static void R_ProcessEvents();
        static void R_Busy(int) { }
        static int  R_YesNoCancel(const char *) { return 0; }
        static void cancel();

        QStringList messages;


    protected:

        // return a dataframe for the ride passed
        QList<RideItem *> activitiesFor(SEXP datetime);   // find the rideitem requested by the user
        QList<SEXP> dfForActivity(RideFile *f, int split, QString join); // returns date series for an activity
        SEXP dfForActivityWBal(RideFile *f);            // returns w' bal series for an activity
        SEXP dfForActivityXData(RideFile *f, QString name); // returns XData series by name for an activity
        SEXP dfForActivityMeanmax(const RideItem *i);   // returns mean maximals for an activity
        SEXP dfForRideItem(const RideItem *i);          // returns metrics and meradata for an activity
        SEXP dfForDateRange(bool all, DateRange range, SEXP filter); // returns metrics and metadata for a season
        SEXP dfForDateRangeIntervals(DateRange range, QStringList types); // returns metrics and metadata for a season
        SEXP dfForDateRangeMeanmax(bool all, DateRange range, SEXP filter); // returns the meanmax for a season
        SEXP dfForDateRangePeaks(bool all, DateRange range, SEXP filter, QList<RideFile::SeriesType> series, QList<int> durations);
        SEXP dfForRideFileCache(RideFileCache *p);      // returns meanmax for a cache

};

// there is a global instance created in main
extern RTool *rtool;

#endif
