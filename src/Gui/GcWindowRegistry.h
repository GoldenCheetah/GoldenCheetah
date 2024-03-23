/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_GcWindowRegistry_h
#define _GC_GcWindowRegistry_h

#include "GoldenCheetah.h"
#include <QApplication>

class Context;

// all the windows we have defined
namespace GcWindowTypes {
enum gcwinid {
        None = 0,
        Aerolab = 1,
        AllPlot = 2,
        CriticalPower =3,
        Diary =4,
        GoogleMap =5,
        Histogram =6,
        LTM =7,
        Model =8, // deprecated
        PerformanceManager =9,
        PfPv =10,
        Race =11, // deprecated
        RideEditor =13, // deprecated - include in MetadataWindow (23)
        RideSummary =14,
        Scatter =15,
        Summary =16,
        Train =17,
        TreeMap =18,
        WeeklySummary =19,
        HrPw =20,
        VideoPlayer =21,
        DialWindow =22,
        MetadataWindow = 23,
        RealtimePlot = 24,
        WorkoutPlot = 25,
        MapWindow = 26,
        StreetViewWindow = 27,
        BingMap = 28, // deprecated
        RealtimeControls = 29,
        ActivityNavigator = 30,
        SpinScanPlot = 31,
        DateRangeSummary = 32,
        CriticalPowerSummary = 33,
        Distribution = 34,
        RouteSegment = 35,
        WorkoutWindow = 36,
        RideMapWindow = 37,
        RConsole = 38,
        RConsoleSeason = 39,
        SeasonPlan = 40,
        WebPageWindow = 41,
        Overview = 42,
        Python = 43,
        PythonSeason = 44,
        UserTrends=45,
        UserAnalysis=46,
        OverviewTrends=47,
        LiveMapWebPageWindow = 48,
        OverviewAnalysisBlank=49,
        OverviewTrendsBlank=50

};
};
typedef enum GcWindowTypes::gcwinid GcWinID;
Q_DECLARE_METATYPE(GcWinID)

// when declaring a window, what view is it relevant for?
#define VIEW_TRAIN    0x01
#define VIEW_ANALYSIS 0x02
#define VIEW_DIARY    0x04
#define VIEW_TRENDS   0x08

class GcChartWindow;
class GcWindowRegistry {
    Q_DECLARE_TR_FUNCTIONS(GcWindowRegistry)
    public:

    unsigned int relevance;
    QString name;
    GcWinID id;

    static void initialize(); // initialize global registry
    static GcChartWindow *newGcWindow(GcWinID id, Context *context);
    static QStringList windowsForType(int type);
    static QList<GcWinID> idsForType(int type);
    static QString title(GcWinID id);
};

extern GcWindowRegistry* GcWindows;
#endif
