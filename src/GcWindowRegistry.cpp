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

// all the windows we have defined
#include "AerolabWindow.h"
#include "AllPlotWindow.h"
#include "CriticalPowerWindow.h"
#ifdef GC_HAVE_ICAL
#include "DiaryWindow.h"
#endif
#include "GoogleMapControl.h"
#include "HistogramWindow.h"
#include "LTMWindow.h"
#ifdef GC_HAVE_QWTPLOT3D
#include "ModelWindow.h"
#endif
#ifdef GC_HAVE_VLC
#include "VideoWindow.h"
#endif
#ifdef Q_OS_MAC
#include "QtMacVideoWindow.h"
#endif
#include "PerformanceManagerWindow.h"
#include "PfPvWindow.h"
#include "HrPwWindow.h"
#include "RaceWindow.h" // XXX not done
#include "RideEditor.h"
#include "RideNavigator.h"
#include "RideSummaryWindow.h"
#include "ScatterWindow.h"
#include "SummaryWindow.h"
#include "MetadataWindow.h"
#include "TreeMapWindow.h"
#include "WeeklySummaryWindow.h"
#include "DialWindow.h"
#include "RealtimePlotWindow.h"
#include "WorkoutPlotWindow.h"
#include "BingMap.h"

GcWindowRegistry GcWindows[] = {
    // name                     GcWinID
    { "Calendar",GcWindowTypes::Diary },
    { "Navigator", GcWindowTypes::ActivityNavigator },
    { "Summary",GcWindowTypes::RideSummary },
    { "Details",GcWindowTypes::MetadataWindow },
    { "Editor",GcWindowTypes::RideEditor },
    { "Summary & Details",GcWindowTypes::Summary },
    { "Performance",GcWindowTypes::AllPlot },
    { "Pedal Force vs Velocity",GcWindowTypes::PfPv },
    { "Critical Mean Maximals",GcWindowTypes::CriticalPower },
    { "Histogram",GcWindowTypes::Histogram },
    { "Google Map",GcWindowTypes::GoogleMap },
    { "Bing Map",GcWindowTypes::BingMap },
    { "2d Plot",GcWindowTypes::Scatter },
    { "3d Plot",GcWindowTypes::Model },
    { "Heartrate vs Power",GcWindowTypes::HrPw },
    { "Weekly Summary",GcWindowTypes::WeeklySummary },
    { "Long Term Metrics",GcWindowTypes::LTM },
    { "Performance Manager",GcWindowTypes::PerformanceManager },
    { "Collection TreeMap",GcWindowTypes::TreeMap },
    { "Aerolab Chung Analysis",GcWindowTypes::Aerolab },
    { "Realtime Dial",GcWindowTypes::DialWindow },
    { "Realtime Plot",GcWindowTypes::RealtimePlot },
    { "Workout Plot",GcWindowTypes::WorkoutPlot },
    { "Train Map Window", GcWindowTypes::MapWindow },
    { "Train StreetView Window", GcWindowTypes::StreetViewWindow },
    { "Video Player",GcWindowTypes::VideoPlayer },
    { "", GcWindowTypes::None }};

// instantiate a new window
GcWindow *
GcWindowRegistry::newGcWindow(GcWinID id, MainWindow *main) //XXX mainWindow will go soon
{
    GcWindow *returning = NULL;

    switch(id) {
    case GcWindowTypes::Aerolab: returning = new AerolabWindow(main); break;
    case GcWindowTypes::AllPlot: returning = new AllPlotWindow(main); break;
    case GcWindowTypes::CriticalPower: returning = new CriticalPowerWindow(main->home, main); break;
#ifdef GC_HAVE_ICAL
    case GcWindowTypes::Diary: returning = new DiaryWindow(main); break;
#else
    case GcWindowTypes::Diary: returning = new GcWindow(); break;
#endif
    case GcWindowTypes::GoogleMap: returning = new GoogleMapControl(main); break;
    case GcWindowTypes::Histogram: returning = new HistogramWindow(main); break;
    case GcWindowTypes::LTM: returning = new LTMWindow(main, main->useMetricUnits, main->home); break;
#ifdef GC_HAVE_QWTPLOT3D
    case GcWindowTypes::Model: returning = new ModelWindow(main, main->home); break;
#else
    case GcWindowTypes::Model: returning = new GcWindow(); break;
#endif
    case GcWindowTypes::PerformanceManager: returning = new PerformanceManagerWindow(main); break;
    case GcWindowTypes::PfPv: returning = new PfPvWindow(main); break;
    case GcWindowTypes::HrPw: returning = new HrPwWindow(main); break;
    case GcWindowTypes::RideEditor: returning = new RideEditor(main); break;
    case GcWindowTypes::RideSummary: returning = new RideSummaryWindow(main); break;
    case GcWindowTypes::Scatter: returning = new ScatterWindow(main, main->home); break;
    case GcWindowTypes::Summary: returning = new SummaryWindow(main); break;
    case GcWindowTypes::TreeMap: returning = new TreeMapWindow(main, main->useMetricUnits, main->home); break;
    case GcWindowTypes::WeeklySummary: returning = new WeeklySummaryWindow(main->useMetricUnits, main); break;
#if defined Q_OS_MAC || defined GC_HAVE_VLC // mac uses Quicktime / Win/Linux uses VLC
    case GcWindowTypes::VideoPlayer: returning = new VideoWindow(main, main->home); break;
#else
    case GcWindowTypes::VideoPlayer: returning = new GcWindow(); break;
#endif
    case GcWindowTypes::DialWindow: returning = new DialWindow(main); break;
    case GcWindowTypes::MetadataWindow: returning = new MetadataWindow(main); break;
    case GcWindowTypes::RealtimeControls: returning = new GcWindow(); break;
    case GcWindowTypes::RealtimePlot: returning = new RealtimePlotWindow(main); break;
    case GcWindowTypes::WorkoutPlot: returning = new WorkoutPlotWindow(main); break;
    case GcWindowTypes::BingMap: returning = new BingMap(main); break;
    case GcWindowTypes::MapWindow: returning = new MapWindow(main); break;
    case GcWindowTypes::StreetViewWindow: returning = new StreetViewWindow(main); break;
    case GcWindowTypes::ActivityNavigator: returning = new RideNavigator(main); break;
    default: return NULL; break;
    }
    if (returning) returning->setProperty("type", QVariant::fromValue<GcWinID>(id));
    return returning;
}
