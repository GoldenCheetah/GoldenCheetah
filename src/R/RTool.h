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

        Context *context;
        QString version;

        static SEXP pageSize(SEXP width, SEXP height);

        // athlete
        static SEXP athlete();

        // activities
        static SEXP activities(SEXP filter);
        static SEXP activity(SEXP datetime, SEXP compare);
        static SEXP activityMeanmax(SEXP compare);
        static SEXP activityWBal(SEXP compare);
        static SEXP activityMetrics(SEXP compare);

        // seasons
        static SEXP season(SEXP all, SEXP compare);
        static SEXP metrics(SEXP all, SEXP compare);
        static SEXP seasonMeanmax(SEXP all, SEXP compare);
        static SEXP seasonPeaks(SEXP all, SEXP compare, SEXP series, SEXP duration);
        static SEXP pmc(SEXP all, SEXP metric);

        bool starting;
        bool failed;

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
        static void R_Busy(int) { }
        static void R_Callback() { }
        static int  R_YesNoCancel(const char *) { return 0; }

        QStringList messages;


    protected:

        // return a dataframe for the ride passed
        QList<RideItem *> activitiesFor(SEXP datetime);   // find the rideitem requested by the user
        SEXP dfForActivity(RideFile *f);                // returns date series for an activity
        SEXP dfForActivityWBal(RideFile *f);            // returns w' bal series for an activity
        SEXP dfForActivityMeanmax(const RideItem *i);   // returns mean maximals for an activity
        SEXP dfForRideItem(const RideItem *i);          // returns metrics and meradata for an activity
        SEXP dfForDateRange(bool all, DateRange range); // returns metrics and metadata for a season
        SEXP dfForDateRangeMeanmax(bool all, DateRange range); // returns the meanmax for a season
        SEXP dfForDateRangePeaks(bool all, DateRange range, QList<RideFile::SeriesType> series, QList<int> durations);
        SEXP dfForRideFileCache(RideFileCache *p);      // returns meanmax for a cache
};

// there is a global instance created in main
extern RTool *rtool;

#endif
