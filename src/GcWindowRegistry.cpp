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
#include "PerformanceManagerWindow.h"
#include "PfPvWindow.h"
#include "RaceWindow.h" // XXX not done
#include "RealtimeWindow.h"
#include "RideEditor.h"
#include "RideSummaryWindow.h"
#include "ScatterWindow.h"
#include "SummaryWindow.h"
#include "TrainWindow.h" // XXX not done
#include "TreeMapWindow.h"
#include "WeeklySummaryWindow.h"

GcWindowRegistry GcWindows[] = {
    // name                     GcWinID
    { "Aerolab",                GcWindowTypes::Aerolab },
    { "Ride",                   GcWindowTypes::AllPlot },
    { "Critical Power",         GcWindowTypes::CriticalPower },
    { "Diary",                  GcWindowTypes::Diary },
    { "Map",                    GcWindowTypes::GoogleMap },
    { "Histogram",              GcWindowTypes::Histogram },
    { "Metrics",                GcWindowTypes::LTM },
    { "3d",                     GcWindowTypes::Model },
    { "Performance Manager",    GcWindowTypes::PerformanceManager },
    { "PfPv",                   GcWindowTypes::PfPv },
    { "Training",               GcWindowTypes::Training },
    { "Ride Editor",            GcWindowTypes::RideEditor },
    { "Ride Summary",           GcWindowTypes::RideSummary },
    { "Scatter",                GcWindowTypes::Scatter },
    { "Ride Summary & Fields",  GcWindowTypes::Summary },
    { "Treemap",                GcWindowTypes::TreeMap },
    { "Weekly Summary",         GcWindowTypes::WeeklySummary },
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
    case GcWindowTypes::Training: returning = new TrainWindow(main, main->home); break;
    case GcWindowTypes::RideEditor: returning = new RideEditor(main); break;
    case GcWindowTypes::RideSummary: returning = new RideSummaryWindow(main); break;
    case GcWindowTypes::Scatter: returning = new ScatterWindow(main, main->home); break;
    case GcWindowTypes::Summary: returning = new SummaryWindow(main); break;
    case GcWindowTypes::TreeMap: returning = new TreeMapWindow(main, main->useMetricUnits, main->home); break;
    case GcWindowTypes::WeeklySummary: returning = new WeeklySummaryWindow(main->useMetricUnits, main); break;
    default: return NULL; break;
    }
    if (returning) returning->setProperty("type", QVariant::fromValue<GcWinID>(id));
    return returning;
}
