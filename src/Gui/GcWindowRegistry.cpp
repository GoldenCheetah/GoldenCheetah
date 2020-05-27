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

#include "GoldenCheetah.h"
#include "GcWindowRegistry.h"
#include "Athlete.h"

// all the windows we have defined
#include "AerolabWindow.h"
#include "AllPlotWindow.h"
#include "CriticalPowerWindow.h"
#ifdef GC_HAVE_ICAL
#include "DiaryWindow.h"
#endif
#include "HistogramWindow.h"
#include "LTMWindow.h"
#if defined(GC_VIDEO_AV) || defined(GC_VIDEO_QUICKTIME)
#include "QtMacVideoWindow.h"
#else
#include "VideoWindow.h"
#endif
#include "PfPvWindow.h"
#include "HrPwWindow.h"
#include "RideEditor.h"
#include "RideNavigator.h"
#include "RideSummaryWindow.h"
#include "ScatterWindow.h"
#include "SummaryWindow.h"
#include "MetadataWindow.h"
#include "TreeMapWindow.h"
#include "DialWindow.h"
#include "RealtimePlotWindow.h"
#include "SpinScanPlotWindow.h"
#include "WorkoutPlotWindow.h"
#include "WorkoutWindow.h"
#include "WebPageWindow.h"
#ifdef GC_WANT_R
#include "RChart.h"
#endif
#ifdef GC_WANT_PYTHON
#include "PythonChart.h"
#endif
#include "PlanningWindow.h"
#ifdef GC_HAVE_OVERVIEW
#include "OverviewWindow.h"
#endif
#include "UserChart.h"
// Not until v4.0
//#include "RouteWindow.h"

#define VIEW_TRAIN    0x01
#define VIEW_ANALYSIS 0x02
#define VIEW_DIARY    0x04
#define VIEW_HOME     0x08
#define VIEW_INTERVAL 0x16

// GcWindows initialization is done in initialize method to enable translations
GcWindowRegistry* GcWindows;

void
GcWindowRegistry::initialize()
{
  static GcWindowRegistry GcWindowsInit[35] = {
    // name                     GcWinID
    { VIEW_HOME|VIEW_DIARY, tr("User Chart"),GcWindowTypes::UserTrends },
    { VIEW_HOME|VIEW_DIARY, tr("Trends"),GcWindowTypes::LTM },
    { VIEW_HOME|VIEW_DIARY, tr("TreeMap"),GcWindowTypes::TreeMap },
    //{ VIEW_HOME, tr("Weekly Summary"),GcWindowTypes::WeeklySummary },// DEPRECATED
    { VIEW_HOME|VIEW_DIARY,  tr("Power Duration "),GcWindowTypes::CriticalPowerSummary },
    //{ VIEW_HOME,  tr("Training Plan"),GcWindowTypes::SeasonPlan },
    //{ VIEW_HOME|VIEW_DIARY,  tr("Performance Manager"),GcWindowTypes::PerformanceManager },
    { VIEW_ANALYSIS, tr("User Chart "),GcWindowTypes::UserAnalysis },
    { VIEW_HOME|VIEW_DIARY, tr("User Defined"),GcWindowTypes::UserTrends },
    { VIEW_ANALYSIS, tr("Overview"),GcWindowTypes::Overview },
    { VIEW_ANALYSIS|VIEW_INTERVAL, tr("Summary"),GcWindowTypes::RideSummary },
    { VIEW_ANALYSIS, tr("Details"),GcWindowTypes::MetadataWindow },
    { VIEW_ANALYSIS, tr("Summary and Details"),GcWindowTypes::Summary },
    { VIEW_ANALYSIS, tr("Editor"),GcWindowTypes::RideEditor },
    { VIEW_ANALYSIS|VIEW_INTERVAL, tr("Performance"),GcWindowTypes::AllPlot },
    { VIEW_ANALYSIS, tr("Power Duration"),GcWindowTypes::CriticalPower },
    { VIEW_ANALYSIS, tr("Histogram"),GcWindowTypes::Histogram },
    { VIEW_HOME|VIEW_DIARY, tr("Distribution"),GcWindowTypes::Distribution },
    { VIEW_ANALYSIS, tr("Pedal Force vs Velocity"),GcWindowTypes::PfPv },
    { VIEW_ANALYSIS, tr("Heartrate vs Power"),GcWindowTypes::HrPw },
    { VIEW_ANALYSIS|VIEW_INTERVAL, tr("Map"),GcWindowTypes::RideMapWindow },
    { VIEW_ANALYSIS, tr("R Chart"),GcWindowTypes::RConsole },
    { VIEW_HOME, tr("R Chart "),GcWindowTypes::RConsoleSeason },
    { VIEW_ANALYSIS, tr("Python Chart"),GcWindowTypes::Python },
    { VIEW_HOME, tr("Python Chart "),GcWindowTypes::PythonSeason },
    //{ VIEW_ANALYSIS, tr("Bing Map"),GcWindowTypes::BingMap },
    { VIEW_ANALYSIS, tr("Scatter"),GcWindowTypes::Scatter },
    { VIEW_ANALYSIS, tr("Aerolab"),GcWindowTypes::Aerolab },
    { VIEW_DIARY, tr("Calendar"),GcWindowTypes::Diary },
    { VIEW_DIARY, tr("Navigator"), GcWindowTypes::ActivityNavigator },
    { VIEW_DIARY|VIEW_HOME, tr("Summary "), GcWindowTypes::DateRangeSummary },
    { VIEW_TRAIN, tr("Telemetry"),GcWindowTypes::DialWindow },
    { VIEW_TRAIN, tr("Workout"),GcWindowTypes::WorkoutPlot },
    { VIEW_TRAIN, tr("Realtime"),GcWindowTypes::RealtimePlot },
    { VIEW_TRAIN, tr("Pedal Stroke"),GcWindowTypes::SpinScanPlot },
    { VIEW_TRAIN, tr("Video Player"),GcWindowTypes::VideoPlayer },
    { VIEW_TRAIN, tr("Workout Editor"),GcWindowTypes::WorkoutWindow },
    { VIEW_ANALYSIS|VIEW_HOME|VIEW_TRAIN, tr("Web page"),GcWindowTypes::WebPageWindow },
    { 0, "", GcWindowTypes::None }};
  // initialize the global registry
  GcWindows = GcWindowsInit;
}

QStringList windowsForType(int type)
{
    QStringList returning;

    for(int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].relevance & type) 
            returning << GcWindows[i].name;
    }
    return returning;
}

QString
GcWindowRegistry::title(GcWinID id)
{
    for(int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].relevance && GcWindows[i].id == id)
            return GcWindows[i].name;
    }
    return QString("unknown");
}

QList<GcWinID> idsForType(int type)
{
    QList<GcWinID> returning;

    for(int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].relevance & type) 
            returning << GcWindows[i].id;
    }
    return returning;
}

// instantiate a new window
GcChartWindow *
GcWindowRegistry::newGcWindow(GcWinID id, Context *context)
{
    GcChartWindow *returning = NULL;

    switch(id) {
    case GcWindowTypes::Aerolab: returning = new AerolabWindow(context); break;
    case GcWindowTypes::AllPlot: returning = new AllPlotWindow(context); break;
    case GcWindowTypes::CriticalPower: returning = new CriticalPowerWindow(context, false); break;
    case GcWindowTypes::CriticalPowerSummary: returning = new CriticalPowerWindow(context, true); break;
#ifdef GC_HAVE_ICAL
    case GcWindowTypes::Diary: returning = new DiaryWindow(context); break;
#else
    case GcWindowTypes::Diary: returning = new GcChartWindow(context); break;
#endif
    case GcWindowTypes::Histogram: returning = new HistogramWindow(context); break;
#ifdef GC_WANT_R
    case GcWindowTypes::RConsole: returning = new RChart(context, true); break;
    case GcWindowTypes::RConsoleSeason: returning = new RChart(context, false); break;
#else
    case GcWindowTypes::RConsole: returning = new GcChartWindow(context); break;
    case GcWindowTypes::RConsoleSeason: returning = new GcChartWindow(context); break;
#endif
#ifdef GC_WANT_PYTHON
    case GcWindowTypes::Python: returning = new PythonChart(context, true); break;
    case GcWindowTypes::PythonSeason: returning = new PythonChart(context, false); break;
#else
    case GcWindowTypes::PythonSeason:
    case GcWindowTypes::Python: returning = new GcChartWindow(context); break;
#endif
    case GcWindowTypes::Distribution: returning = new HistogramWindow(context, true); break;
    case GcWindowTypes::PerformanceManager: 
            {
                // the old PMC is deprecated so we return an LTM with PMC curves and default settings
                returning = new LTMWindow(context);

                // a PMC LTM Setting
                QString value = "AAAACgBQAE0AQwArACsAAAAIADIAMAAwADkAJXS3AAAAAP8AJXZBAAAAAP8AAAABAAH///////////////8AAAALAAAAAwAAAAIAAAAAEgBzAGsAaQBiAGEAXwBzAHQAcwAAAC4AUwBrAGkAYgBhACAAUwBoAG8AcgB0ACAAVABlAHIAbQAgAFMAdAByAGUAcwBzAAAALgBTAGsAaQBiAGEAIABTAGgAbwByAHQAIABUAGUAcgBtACAAUwB0AHIAZQBzAHMAAAAMAFMAdAByAGUAcwBzAAAAAAABAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAQAAfyr8IRwk//////////8B/////wAA//8AAAAAAAA/8AAAAAAAAAAAAAAB//8AAAAAAAAAAAAAAAAAAAAAAgAADhD/////AAAACv////8AAAAAAAAAAAwAMgAgAFAAYQByAG0AAAAAAAAD5wAADhAAAAAAAgAAAAASAHMAawBpAGIAYQBfAGwAdABzAAAALABTAGsAaQBiAGEAIABMAG8AbgBnACAAVABlAHIAbQAgAFMAdAByAGUAcwBzAAAALABTAGsAaQBiAGEAIABMAG8AbgBnACAAVABlAHIAbQAgAFMAdAByAGUAcwBzAAAADABTAHQAcgBlAHMAcwAAAAAAAQAAAAAAAAAAAAAAAAEAAABYAAAAAwAAAEoAAAAbAAAAFP//////////Af//AAD/////AAAAAAAAP/AAAAAAAAAAAAAAAf//AAAAAAAAAAAAAAAAAAAAAAIAAA4Q/////wAAAAoAAAAAAAAAAAAAAAAMADIAIABQAGEAcgBtAAAAAAAAA+cAAA4QAAAAAAIAAAAAEABzAGsAaQBiAGEAXwBzAGIAAAAoAFMAawBpAGIAYQAgAFMAdAByAGUAcwBzACAAQgBhAGwAYQBuAGMAZQAAACgAUwBrAGkAYgBhACAAUwB0AHIAZQBzAHMAIABCAGEAbABhAG4AYwBlAAAADABTAHQAcgBlAHMAcwAAAAAAAAAAAADAjzgAAAAAAAH////A////wP///8AAAH//AAAAAP//////////Af//VVX//wAAAAAAAAAAP/AAAAAAAAAAAAAAAf//AAAAAAAAAAAAAAAAAQAAAAIAAA4Q/////wAAAAr/////AAAAAAEAAAAMADIAIABQAGEAcgBtAAAAAAAAA+cAAA4QAAAAAAAAAg==";

                // setup and apply the property
                QByteArray base64(value.toLatin1());
                QByteArray unmarshall = QByteArray::fromBase64(base64);
                QDataStream s(&unmarshall, QIODevice::ReadOnly);
                LTMSettings x;
                s >> x;
                returning->setProperty("settings", QVariant().fromValue<LTMSettings>(x));
            }
            break;
    case GcWindowTypes::LTM: returning = new LTMWindow(context); break;
    case GcWindowTypes::Model: returning = new GcChartWindow(context); break;
    case GcWindowTypes::PfPv: returning = new PfPvWindow(context); break;
    case GcWindowTypes::HrPw: returning = new HrPwWindow(context); break;
    case GcWindowTypes::RideEditor: returning = new RideEditor(context); break;
    case GcWindowTypes::RideSummary: returning = new RideSummaryWindow(context, true); break;
    case GcWindowTypes::DateRangeSummary: returning = new RideSummaryWindow(context, false); break;
    case GcWindowTypes::Scatter: returning = new ScatterWindow(context); break;
    case GcWindowTypes::Summary: returning = new SummaryWindow(context); break;
    case GcWindowTypes::TreeMap: returning = new TreeMapWindow(context); break;
    case GcWindowTypes::WeeklySummary: returning = new SummaryWindow(context); break; // deprecated
#ifdef GC_VIDEO_NONE
    case GcWindowTypes::VideoPlayer: returning = new GcChartWindow(context); break;
#else
    case GcWindowTypes::VideoPlayer: returning = new VideoWindow(context); break;
#endif
    case GcWindowTypes::DialWindow: returning = new DialWindow(context); break;
    case GcWindowTypes::MetadataWindow: returning = new MetadataWindow(context); break;
    case GcWindowTypes::RealtimeControls: returning = new GcChartWindow(context); break;
    case GcWindowTypes::RealtimePlot: returning = new RealtimePlotWindow(context); break;
    case GcWindowTypes::SpinScanPlot: returning = new SpinScanPlotWindow(context); break;
    case GcWindowTypes::WorkoutPlot: returning = new WorkoutPlotWindow(context); break;
    case GcWindowTypes::MapWindow:
    case GcWindowTypes::StreetViewWindow:
        returning = new GcChartWindow(context); break;
    // old maps (GoogleMap and BingMap) replaced by RideMapWindow
    case GcWindowTypes::GoogleMap: id=GcWindowTypes::RideMapWindow; returning = new RideMapWindow(context, RideMapWindow::GOOGLE); break; // new GoogleMapControl(context);
    case GcWindowTypes::BingMap: id=GcWindowTypes::RideMapWindow; returning = new RideMapWindow(context, RideMapWindow::OSM); break; //returning = new BingMap(context);

    case GcWindowTypes::RideMapWindow: returning = new RideMapWindow(context, RideMapWindow::OSM); break; // Deprecated Bing, default to OSM

    case GcWindowTypes::ActivityNavigator: returning = new RideNavigator(context); break;
    case GcWindowTypes::WorkoutWindow: returning = new WorkoutWindow(context); break;

    case GcWindowTypes::WebPageWindow: returning = new WebPageWindow(context); break;
#if 0 // not till v4.0
    case GcWindowTypes::RouteSegment: returning = new RouteWindow(context); break;
#else
    case GcWindowTypes::RouteSegment: returning = new GcChartWindow(context); break;
#endif
#if GC_HAVE_OVERVIEW
    case GcWindowTypes::Overview: returning = new OverviewWindow(context); break;
#else
    case GcWindowTypes::Overview: returning = new GcChartWindow(context); break;
#endif
    case GcWindowTypes::SeasonPlan: returning = new PlanningWindow(context); break;
    case GcWindowTypes::UserAnalysis: returning = new UserChart(context, false); break;
    case GcWindowTypes::UserTrends: returning = new UserChart(context, true); break;
    default: return NULL; break;
    }
    if (returning) returning->setProperty("type", QVariant::fromValue<GcWinID>(id));
    return returning;
}
