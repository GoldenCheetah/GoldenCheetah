/*
 * Copyright (c)  2014 Joern Rischmueller(joern.rm@gmail.com)
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

#include <QDesktopServices>
#include <QWidget>
#include <QWhatsThis>
#include "HelpWhatsThis.h"


HelpWhatsThis::HelpWhatsThis(QObject *object) : QObject(object) {
    object->installEventFilter(this);
}

bool
HelpWhatsThis::eventFilter ( QObject *object, QEvent *event) {

    if( event->type() == QEvent::WhatsThisClicked )
    {
      QWhatsThisClickedEvent *helpEvent = static_cast<QWhatsThisClickedEvent*>(event);
      QUrl url;
      url.setUrl(helpEvent->href());
      if (!QDesktopServices::openUrl( url )) {
          // if not successful, try default URL
          QDesktopServices::openUrl(QUrl("https://github.com/GoldenCheetah/GoldenCheetah/wiki"));
      };
      // anyway the event was successfully processed - don't go on
      return true;
    }

    // pass it on
    return QObject::eventFilter( object, event );

}

QString
HelpWhatsThis::getWhatsThisText(GCHelp chapter) {
    return getText(chapter);
}


// private STATIC function to determine the texts (since the texts and links are fixed values)

QString
HelpWhatsThis::getText(GCHelp chapter) {

    QString text = "<center>%2<br><a href=\"https://github.com/GoldenCheetah/GoldenCheetah/wiki/UG_%1\" target=\"_blank\">" + tr("More Help") + "</a></center>";

    switch (chapter) {

    case Default:
        return text.arg("Main-Page_Table-of-contents").arg("Table of Contents");

    // Scope Bar
    case ScopeBar_Trends:
         return text.arg("ScopeBar_Views#trends").arg(tr("Analysis of a number (e.g. date range of activities) like PMC, long term metrics view, ... and data summaries"));
    case ScopeBar_Diary:
         return text.arg("ScopeBar_Views#diary").arg(tr("Extended Calendar view and configurable activity list, plus long term metrics charts and diagram types"));
    case ScopeBar_Rides:
         return text.arg("ScopeBar_Views#rides").arg(tr("Analysis of a single activity - specifically ride and run - with diagrams like ride plot, W'bal, ... and Chung's Aerolab"));
    case ScopeBar_Intervals:
        return text.arg("ScopeBar_Views#intervals").arg("Intervals");
    case ScopeBar_Train:
        return text.arg("ScopeBar_Views#train").arg(tr("Ride indoors, following pre-programmed workouts - with multi device and video playback support"));

    // Tool Bar
    case ToolBar_Download:
        return text.arg("First-Steps_Download-or-import#downloading-a-ride-from-device").arg(tr("Direct download from Powertap, SRM, Joule, Joule GPS, Moxy Monitor or Macro-X device"));
    case ToolBar_Manual:
        return text.arg("Menu%20Bar_Activity").arg(tr("Import any activity file - supported by Golden Cheetah - mass import is supported here"));
    case ToolBar_ToggleSidebar:
        return text.arg("Menu%20Bar_View").arg(tr("Activate / De-activate the Sidebar - which provides different sub-sections to select data shown in the main view"));
    case ToolBar_ToggleComparePane:
        return text.arg("Compare-Pane_General").arg(tr("Activate / De-activate the Compare Pane - which allows to compare rides, intervals or date ranges - also across athletes"));

    // Menus
    case MenuBar_Athlete:
        return text.arg("Menu%20Bar_Athlete").arg(tr("Athlete Management to open/close an existing or create a new athlete - either in a Tab or in a new Window"));

    case MenuBar_Activity:
        return text.arg("Menu%20Bar_Activity").arg(tr("Main functions related to activities like Activity Creation, Sharing and Export, and Splitting or Combining Activities"));
    case MenuBar_Activity_Download:
        return text.arg("First-Steps_Download-or-import#downloading-a-ride-from-device").arg(tr("Direct download from Powertap, SRM, Joule, Joule GPS, Moxy Monitor or Macro-X device"));
    case MenuBar_Activity_Import:
        return text.arg("First-Steps_Download-or-import#importing-from-a-file").arg(tr("Import any activity file - supported by Golden Cheetah - mass import is supported here"));
    case MenuBar_Activity_Manual:
        return text.arg("Menu%20Bar_Activity").arg(tr("Manual creation of an activity where the most relevant data can be added in this dialog"));
    case MenuBar_Activity_Share:
        return text.arg("Special%20Topics_Upload_Download%20to_from%20external%20web-sites#execution")
                .arg(tr("Sharing an activity with other Ride related sites - only sites for which the authorization has been configured can be seleted for sharing here"));
    case MenuBar_Activity_BatchExport:
        return text.arg("Menu%20Bar_Activity").arg(tr("Exports a (selectable) set of activties in one of the supported export formats"));
    case MenuBar_Activity_SplitRide:
        return text.arg("Menu%20Bar_Activity").arg(tr("Wizard to split an activity/ride into multiple rides based on configurable criteria"));
    case MenuBar_Activity_CombineRides:
        return text.arg("Menu%20Bar_Activity").arg(tr("Wizard to combine data with the currently selected activity in multiple ways"));

    case MenuBar_Tools:
        return text.arg("Menu%20Bar_Tools").arg(tr("A set of functions related different features in GoldenCheetah - please check the details for more information"));
    case MenuBar_Tools_CP_EST:
        return text.arg("Menu%20Bar_Tools").arg(tr("Estimation of critical power using the Monod/Scherrer power model"));
    case MenuBar_Tools_AirDens_EST:
        return text.arg("Menu%20Bar_Tools").arg(tr("Estimation of Air Density (Rho)"));
    case MenuBar_Tools_Download_ERGDB:
        return text.arg("Menu%20Bar_Tools").arg(tr("Downloading of Workouts for the ERGDB (online workout DB) for Train - Indoor Riding"));
    case MenuBar_Tools_CreateWorkout:
        return text.arg("Menu%20Bar_Tools").arg(tr("Creation of a new Workout for Train - Indoor Riding"));
    case MenuBar_Tools_ScanDisk_WorkoutVideo:
        return text.arg("Menu%20Bar_Tools").arg(tr("Search for Workout files and Video files in a configurable set of folders and add to the Train - Indoor Riding - Workout/Video library"));
    case MenuBar_Tools_CreateHeatMap:
        return text.arg("Menu%20Bar_Tools")
                .arg(tr("Creates a ride heat map using the selected rides and stored in the choosen folder - \"HeatMap.htm\". Opened in a Web-Browser the map shows where most activity took place."));

    case MenuBar_Edit:
        return text.arg("Menu%20Bar_Edit").arg(tr("Wizards which fix, adjust, add series data of the current activity"));
    case MenuBar_Edit_AddTorqueValues:
        return text.arg("Menu%20Bar_Tools#tool-add-torque").arg(tr("Add Torque Values"));
    case MenuBar_Edit_AdjustPowerValues:
        return text.arg("Menu%20Bar_Tools#tool-adjust-torque").arg(tr("Adjust Power Values"));
    case MenuBar_Edit_AdjustTorqueValues:
        return text.arg("Menu%20Bar_Tools#tool-adjust-power").arg(tr("Adjust Torque Values"));
    case MenuBar_Edit_EstimatePowerValues:
        return text.arg("Menu%20Bar_Tools#tool-estimate-power").arg(tr("Estimate Power Values"));
    case MenuBar_Edit_FixElevationErrors:
        return text.arg("Menu%20Bar_Tools#tool-fix-elevation-errors").arg(tr("Fix Elevation Errors"));
    case MenuBar_Edit_FixGapsInRecording:
        return text.arg("Menu%20Bar_Tools#tool-fix-gaps-in-recording").arg(tr("Fix Gaps in Recording"));
    case MenuBar_Edit_FixGPSErrors:
        return text.arg("Menu%20Bar_Tools#tool-fix-gps-errors").arg(tr("Fix GPS Errors"));
    case MenuBar_Edit_FixHRSpikes:
        return text.arg("Menu%20Bar_Tools#tool-fix-hr-spikes").arg(tr("Fix HR Spikes"));
    case MenuBar_Edit_FixPowerSpikes:
        return text.arg("Menu%20Bar_Tools").arg(tr("Fix Power Spikes#tool-fix-power-spikes"));

    case MenuBar_View:
        return text.arg("Menu%20Bar_View").arg(tr("Options to show/hide views (e.g. Sidebar) as well as adding charts and resetting chart layouts to factory settings"));
    case MenuBar_Help:
        return text.arg("Menu%20Bar_Help").arg(tr("Help options of GoldenCheetah"));

    // Charts
    case ChartTrends_MetricTrends:
        return text.arg("ChartTypes_Trends#metric-trends").arg(tr("Full configurable chart type to track performance and trends for metrics, user-definable best durations and model estimates"));
    case ChartTrends_MetricTrends_Config_Basic:
        return text.arg("ChartTypes_Trends#basic-settings").arg(tr("Date range, data grouping settings which apply to a single chart"));
    case ChartTrends_MetricTrends_Config_Preset:
        return text.arg("ChartTypes_Trends#presets").arg(tr("Prefined sets of curves which can be applied as chart definition, or as starting point for individual adjustments"));
    case ChartTrends_MetricTrends_Config_Curves:
        return text.arg("ChartTypes_Trends#curves").arg(tr("Curves which are plotted for the specific chart - based on presets or individually added and modified here"));

    case ChartTrends_CollectionTreeMap:
        return text.arg("ChartTypes_Trends#collection-tree-map").arg(tr("Tree map visulation of ride data by two selectable dimensions for a configurable metric"));

    case ChartTrends_Critical_MM:
        return text.arg("ChartTypes_Trends#critical-mean-maximal").arg("Trends - Critical Mean Maximal");
    case ChartTrends_Critical_MM_Config_Settings:
        return text.arg("ChartTypes_Trends#critical-mean-maximal").arg("Trends - Critical Mean Maximal - Config - Settings");
    case ChartTrends_Critical_MM_Config_Model:
        return text.arg("ChartTypes_Trends#critical-mean-maximal").arg("Trends - Critical Mean Maximal - Config - Model");
    case ChartTrends_Distribution:
        return text.arg("ChartTypes_Trends#distribution").arg("Trends - Distribution");
     case ChartTrends_DateRange:
        return text.arg("ChartTypes_Trends#date-range-selection").arg("Trends - Date Range");
    case ChartDiary_Calendar:
        return text.arg("ChartTypes_Diary#calendar").arg("Diary - Calendar");
    case ChartDiary_Navigator:
        return text.arg("ChartTypes_Diary#navigator").arg("Diary - Navigator");
    case ChartRides_Summary:
        return text.arg("ChartTypes_Rides#ride-summary").arg("Rides - Summary");
    case ChartRides_Details:
        return text.arg("ChartTypes_Rides#details").arg("Rides - Details");
    case ChartRides_Summary_Details:
        return text.arg("ChartTypes_Rides#summary-and-details").arg("Rides - Summary/Details");
    case ChartRides_Editor:
        return text.arg("ChartTypes_Rides#editor").arg("Rides - Editor");

    case ChartRides_Performance:
        return text.arg("ChartTypes_Rides#performance").arg("Rides - Performance");
    case ChartRides_Performance_Config_Basic:
        return text.arg("ChartTypes_Rides#performance-basic").arg("Rides - Performance - Basic");
    case ChartRides_Performance_Config_Series:
        return text.arg("ChartTypes_Rides#performance-series").arg("Rides - Performance - Series");

    case ChartRides_Critical_MM:
        return text.arg("ChartTypes_Rides#critical-mean-maximals").arg("Rides - Critical Mean Maximals");
    case ChartRides_Critical_MM_Config_Settings:
        return text.arg("ChartTypes_Rides#critical-mean-maximal").arg("Rides - Critical Mean Maximal - Config - Settings");
    case ChartRides_Critical_MM_Config_Model:
        return text.arg("ChartTypes_Rides#critical-mean-maximal").arg("Rides - Critical Mean Maximal - Config - Model");

    case ChartRides_Histogram:
        return text.arg("ChartTypes_Rides#histogram").arg("Rides - Histogram");
    case ChartRides_PFvV:
        return text.arg("ChartTypes_Rides#pedal-force-vs-velocity").arg("Rides - PFvsV");
    case ChartRides_HRvsPw:
        return text.arg("ChartTypes_Rides#heartrate-vs-power").arg("Rides - HRvsPw");
    case ChartRides_Map:
        return text.arg("ChartTypes_Rides#google-map--bing-map").arg("Rides - Map");
    case ChartRides_2D:
        return text.arg("ChartTypes_Rides#2d-plot").arg("Rides - 2d Plot");
    case ChartRides_3D:
        return text.arg("ChartTypes_Rides#3d-plot").arg("Rides - 3d Plot");
    case ChartRides_Aerolab:
        return text.arg("ChartTypes_Rides#aerolab-chung-analysis").arg("Rides - Aerolab");

    case Chart_Summary:
        return text.arg("ChartTypes_Trends#summary").arg("Summary");
    case Chart_Summary_Config:
        return text.arg("ChartTypes_Trends#summary-config").arg("Summary Config");

    // Sidebars
    case SideBarTrendsView_DateRanges:
        return text.arg("Side-Bar_Trends-view#date-ranges").arg("Date Ranges");
    case SideBarTrendsView_Events:
        return text.arg("Side-Bar_Trends-view#events").arg("Events");
    case SideBarTrendsView_Summary:
        return text.arg("Side-Bar_Trends-view#summary").arg("Summary");
    case SideBarTrendsView_Filter:
        return text.arg("Side-Bar_Trends-view#filters").arg("Filters");
    case SideBarTrendsView_Charts:
        return text.arg("Side-Bar_Trends-view#charts").arg("Charts");
    case SideBarRidesView_Calendar:
        return text.arg("Side-Bar_Rides-view#calendar").arg("Calendar");
    case SideBarRidesView_Rides:
        return text.arg("Side-Bar_Rides-view#rides").arg("Rides");
    case SideBarRidesView_Intervals:
        return text.arg("Side-Bar_Rides-view#intervals").arg("Intervals");
    case SideBarDiaryView_Calendar:
        return text.arg("Side%20Bar_Diary%20view#calendar").arg("Calendar");
    case SideBarDiaryView_Summary:
        return text.arg("Side%20Bar_Diary%20view#summary").arg("Summary");

    // Cross Functions
    case SearchFilterBox:
        return text.arg("Special-Topics_SearchFilter").arg("Entry field for Searching and Filtering of activities");
    case FindIntervals:
        return text.arg("Side-Bar_Rides-view#intervals").arg("Intervals");

    // Preferences
    case Preferences_General:
        return text.arg("Preferences_General").arg("Preferences - General");
    case Preferences_Athlete_About:
        return text.arg("Preferences_Athlete").arg("Preferences - Athlete");
    case Preferences_Athlete_TrainingZones_Power:
        return text.arg("Preferences_Athlete_Training%20Zones").arg("Preferences - Training Zones");
    case Preferences_Athlete_TrainingZones_HR:
        return text.arg("Preferences_Athlete_Training%20Zones").arg("Preferences - Training Zones");
    case Preferences_Athlete_TrainingZones_Pace:
        return text.arg("Preferences_Athlete_Training%20Zones").arg("Preferences - Training Zones");
    case Preferences_Athlete_Autoimport:
        return text.arg("Preferences_Athlete_Autoimport").arg("Preferences - Autoimport");
    case Preferences_Passwords:
        return text.arg("Preferences_Passwords").arg("Preferences - Passwords");
    case Preferences_Appearance:
        return text.arg("Preferences_ppearance").arg("Preferences - Appearance");
    case Preferences_DataFields:
        return text.arg("Preferences_Data%20Fields").arg("Preferences - Data Fields");
    case Preferences_Metrics:
        return text.arg("Preferences_Metrics").arg("Preferences - Metrics");
    case Preferences_TrainDevices:
        return text.arg("Preferences_Train Devices").arg("Preferences - Train Devices");

    }

    return text.arg("").arg("Golden Cheetah Wiki");

}

