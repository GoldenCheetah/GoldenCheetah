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
#include "BingMap.h"

#define VIEW_TRAIN    0x01
#define VIEW_ANALYSIS 0x02
#define VIEW_DIARY    0x04
#define VIEW_HOME     0x08

// GcWindows initialization is done in initialize method to enable translations
GcWindowRegistry* GcWindows;

void
GcWindowRegistry::initialize()
{
  static GcWindowRegistry GcWindowsInit[30] = {
    // name                     GcWinID
    { VIEW_HOME|VIEW_DIARY, tr("Long Term Metrics"),GcWindowTypes::LTM },
    { VIEW_HOME, tr("Performance Manager"),GcWindowTypes::PerformanceManager },
    { VIEW_HOME|VIEW_DIARY, tr("Collection TreeMap"),GcWindowTypes::TreeMap },
    //{ VIEW_HOME, tr("Weekly Summary"),GcWindowTypes::WeeklySummary },// DEPRECATED
    { VIEW_HOME|VIEW_DIARY,  tr("Critical Mean Maximal"),GcWindowTypes::CriticalPowerSummary },
    { VIEW_ANALYSIS, tr("Activity Summary"),GcWindowTypes::RideSummary },
    { VIEW_ANALYSIS, tr("Details"),GcWindowTypes::MetadataWindow },
    { VIEW_ANALYSIS, tr("Summary and Details"),GcWindowTypes::Summary },
    { VIEW_ANALYSIS, tr("Editor"),GcWindowTypes::RideEditor },
    { VIEW_ANALYSIS, tr("Performance"),GcWindowTypes::AllPlot },
    { VIEW_ANALYSIS, tr("Critical Mean Maximals"),GcWindowTypes::CriticalPower },
    { VIEW_ANALYSIS, tr("Histogram"),GcWindowTypes::Histogram },
    { VIEW_HOME|VIEW_DIARY, tr("Distribution"),GcWindowTypes::Distribution },
    { VIEW_ANALYSIS, tr("Pedal Force vs Velocity"),GcWindowTypes::PfPv },
    { VIEW_ANALYSIS, tr("Heartrate vs Power"),GcWindowTypes::HrPw },
    { VIEW_ANALYSIS, tr("Google Map"),GcWindowTypes::GoogleMap },
    { VIEW_ANALYSIS, tr("Bing Map"),GcWindowTypes::BingMap },
    { VIEW_ANALYSIS, tr("2d Plot"),GcWindowTypes::Scatter },
    { VIEW_ANALYSIS, tr("3d Plot"),GcWindowTypes::Model },
    { VIEW_ANALYSIS, tr("Aerolab Chung Analysis"),GcWindowTypes::Aerolab },
    { VIEW_DIARY, tr("Calendar"),GcWindowTypes::Diary },
    { VIEW_DIARY, tr("Navigator"), GcWindowTypes::ActivityNavigator },
    { VIEW_DIARY|VIEW_HOME, tr("Summary"), GcWindowTypes::DateRangeSummary },
    { VIEW_TRAIN, tr("Telemetry"),GcWindowTypes::DialWindow },
    { VIEW_TRAIN, tr("Workout"),GcWindowTypes::WorkoutPlot },
    { VIEW_TRAIN, tr("Realtime"),GcWindowTypes::RealtimePlot },
    { VIEW_TRAIN, tr("Pedal Stroke"),GcWindowTypes::SpinScanPlot },
    { VIEW_TRAIN, tr("Map"), GcWindowTypes::MapWindow },
    { VIEW_TRAIN, tr("StreetView"), GcWindowTypes::StreetViewWindow },
    { VIEW_TRAIN, tr("Video Player"),GcWindowTypes::VideoPlayer },
    { 0, "", GcWindowTypes::None }};
  // initialize the global registry
  GcWindows = GcWindowsInit;
}

// instantiate a new window
GcWindow *
GcWindowRegistry::newGcWindow(GcWinID id, MainWindow *main)
{
    GcWindow *returning = NULL;

    switch(id) {
    case GcWindowTypes::Aerolab: returning = new AerolabWindow(main); break;
    case GcWindowTypes::AllPlot: returning = new AllPlotWindow(main); break;
    case GcWindowTypes::CriticalPower: returning = new CriticalPowerWindow(main->athlete->home, main); break;
    case GcWindowTypes::CriticalPowerSummary: returning = new CriticalPowerWindow(main->athlete->home, main, true); break;
#ifdef GC_HAVE_ICAL
    case GcWindowTypes::Diary: returning = new DiaryWindow(main); break;
#else
    case GcWindowTypes::Diary: returning = new GcWindow(); break;
#endif
    case GcWindowTypes::GoogleMap: returning = new GoogleMapControl(main); break;
    case GcWindowTypes::Histogram: returning = new HistogramWindow(main); break;
    case GcWindowTypes::Distribution: returning = new HistogramWindow(main, true); break;
    case GcWindowTypes::LTM: returning = new LTMWindow(main); break;
#ifdef GC_HAVE_QWTPLOT3D
    case GcWindowTypes::Model: returning = new ModelWindow(main, main->athlete->home); break;
#else
    case GcWindowTypes::Model: returning = new GcWindow(); break;
#endif
    case GcWindowTypes::PerformanceManager: returning = new PerformanceManagerWindow(main); break;
    case GcWindowTypes::PfPv: returning = new PfPvWindow(main); break;
    case GcWindowTypes::HrPw: returning = new HrPwWindow(main); break;
    case GcWindowTypes::RideEditor: returning = new RideEditor(main); break;
    case GcWindowTypes::RideSummary: returning = new RideSummaryWindow(main, true); break;
    case GcWindowTypes::DateRangeSummary: returning = new RideSummaryWindow(main, false); break;
    case GcWindowTypes::Scatter: returning = new ScatterWindow(main, main->athlete->home); break;
    case GcWindowTypes::Summary: returning = new SummaryWindow(main); break;
    case GcWindowTypes::TreeMap: returning = new TreeMapWindow(main); break;
    case GcWindowTypes::WeeklySummary: returning = new SummaryWindow(main); break; // deprecated
#if defined Q_OS_MAC || defined GC_HAVE_VLC // mac uses Quicktime / Win/Linux uses VLC
    case GcWindowTypes::VideoPlayer: returning = new VideoWindow(main, main->athlete->home); break;
#else
    case GcWindowTypes::VideoPlayer: returning = new GcWindow(); break;
#endif
    case GcWindowTypes::DialWindow: returning = new DialWindow(main); break;
    case GcWindowTypes::MetadataWindow: returning = new MetadataWindow(main); break;
    case GcWindowTypes::RealtimeControls: returning = new GcWindow(); break;
    case GcWindowTypes::RealtimePlot: returning = new RealtimePlotWindow(main); break;
    case GcWindowTypes::SpinScanPlot: returning = new SpinScanPlotWindow(main); break;
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
