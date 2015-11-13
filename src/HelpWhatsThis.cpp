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
         return text.arg("ScopeBar_Views#trends").arg(tr("Analysis of a number of activities (e.g. a date range ) like PMC, long term metrics view, ... and data summaries"));
    case ScopeBar_Diary:
         return text.arg("ScopeBar_Views#diary").arg(tr("Extended Calendar view and configurable activity list, plus long term metrics charts and diagram types"));
    case ScopeBar_Rides:
         return text.arg("ScopeBar_Views#activities").arg(tr("Analysis of a single activity with diagrams like activity plot, W'bal, ... and Chung's Aerolab"));
    case ScopeBar_Intervals:
        return text.arg("ScopeBar_Views#intervals").arg("Intervals");
    case ScopeBar_Train:
        return text.arg("ScopeBar_Views#train").arg(tr("Ride indoors, following pre-programmed workouts - with multi device and video playback support"));

    // Tool Bar
    case ToolBar_Download:
        return text.arg("First-Steps_Download-or-import#downloading-a-activity-from-device").arg(tr("Direct download from Powertap, SRM, Joule, Joule GPS, Moxy Monitor or Macro-X device"));
    case ToolBar_Manual:
        return text.arg("Menu%20Bar_Activity").arg(tr("Import any activity file - supported by Golden Cheetah - mass import is supported here"));
    case ToolBar_ToggleSidebar:
        return text.arg("Menu%20Bar_View").arg(tr("Activate / De-activate the Sidebar - which provides different sub-sections to select data shown in the main view"));
    case ToolBar_ToggleComparePane:
        return text.arg("Compare-Pane_General").arg(tr("Activate / De-activate the Compare Pane - which allows to compare activities, intervals or date ranges - also across athletes"));

    // Menus
    case MenuBar_Athlete:
        return text.arg("Menu%20Bar_Athlete").arg(tr("Athlete Management to open/close an existing or create a new athlete - either in a Tab or in a new Window"));

    case MenuBar_Activity:
        return text.arg("Menu%20Bar_Activity").arg(tr("Main functions related to activities like Activity Creation, Sharing and Export, and Splitting or Combining Activities"));
    case MenuBar_Activity_Download:
        return text.arg("First-Steps_Download-or-import#downloading-a-activity-from-device").arg(tr("Direct download from Powertap, SRM, Joule, Joule GPS, Moxy Monitor or Macro-X device"));
    case MenuBar_Activity_Import:
        return text.arg("First-Steps_Download-or-import#importing-from-a-file").arg(tr("Import any activity file - supported by Golden Cheetah - mass import is supported here"));
    case MenuBar_Activity_Manual:
        return text.arg("Menu%20Bar_Activity").arg(tr("Manual creation of an activity where the most relevant data can be added in this dialog"));
    case MenuBar_Activity_Manual_LapsEditor:
        return text.arg("Menu%20Bar_Activity").arg(tr("Laps Editor allows to enter a sequence of work-rest intervals series -defined by number of repetitions (reps), distance (dist, units according to preferences in Pace Zones) and duration (min and sec)- to generate the data points for the activity"));

    case MenuBar_Activity_BatchExport:
        return text.arg("Menu%20Bar_Activity").arg(tr("Exports a (selectable) set of activties in one of the supported export formats"));
    case MenuBar_Activity_SplitRide:
        return text.arg("Menu%20Bar_Activity").arg(tr("Wizard to split an activity into multiple activities based on configurable criteria"));
    case MenuBar_Activity_CombineRides:
        return text.arg("Menu%20Bar_Activity").arg(tr("Wizard to combine data with the currently selected activity in multiple ways"));

    case MenuBar_Share:
            return text.arg("Menu%20Bar_Share").arg(tr("All functions related to sharing of activities with cloud services"));
    case MenuBar_Share_Online:
        return text.arg("Special%20Topics_Upload_Download%20to_from%20external%20web-sites#execution")
                .arg(tr("Sharing an activity with other trainingsites - only sites for which the authorization has been configured can be seleted for sharing here"));

    case MenuBar_Tools:
        return text.arg("Menu%20Bar_Tools").arg(tr("A set of functions related different features in GoldenCheetah - please check the details for more information"));
    case MenuBar_Tools_CP_EST:
        return text.arg("Menu%20Bar_Tools").arg(tr("Estimation of critical power using the Monod/Scherrer power model"));
    case MenuBar_Tools_AirDens_EST:
        return text.arg("Menu%20Bar_Tools").arg(tr("Estimation of Air Density (Rho)"));
    case MenuBar_Tools_VDOT_CALC:
        return text.arg("Menu%20Bar_Tools").arg(tr("Calculation of VDOT and Threshold Pace according to Daniels' Running Formula"));
    case MenuBar_Tools_Download_ERGDB:
        return text.arg("Menu%20Bar_Tools").arg(tr("Downloading of Workouts for the ERGDB (online workout DB) for Train - Indoor Riding"));
    case MenuBar_Tools_CreateWorkout:
        return text.arg("Menu%20Bar_Tools").arg(tr("Creation of a new Workout for Train - Indoor Riding"));
    case MenuBar_Tools_ScanDisk_WorkoutVideo:
        return text.arg("Menu%20Bar_Tools").arg(tr("Search for Workout files and Video files in a configurable set of folders and add to the Train - Indoor Riding - Workout/Video library"));
    case MenuBar_Tools_CreateHeatMap:
        return text.arg("Menu%20Bar_Tools")
                .arg(tr("Creates an activity heat map using the selected activities and stores it in the choosen folder - \"HeatMap.htm\". Opened in a Web-Browser the map shows where most activity took place."));

    case MenuBar_Edit:
        return text.arg("Menu%20Bar_Edit").arg(tr("Wizards which fix, adjust, add series data of the current activity"));
    case MenuBar_Edit_AddTorqueValues:
        return text.arg("Menu%20Bar_Edit").arg(tr("Add Torque Values"));
    case MenuBar_Edit_AdjustPowerValues:
        return text.arg("Menu%20Bar_Edit").arg(tr("Adjust Power Values"));
    case MenuBar_Edit_AdjustTorqueValues:
        return text.arg("Menu%20Bar_Edit").arg(tr("Adjust Torque Values"));
    case MenuBar_Edit_EstimatePowerValues:
        return text.arg("Menu%20Bar_Edit").arg(tr("Estimate Power Values"));
    case MenuBar_Edit_EstimateDistanceValues:
        return text.arg("Menu%20Bar_Edit").arg(tr("Estimate Distance Values"));
    case MenuBar_Edit_FixElevationErrors:
        return text.arg("Menu%20Bar_Edit").arg(tr("Fix Elevation Errors"));
    case MenuBar_Edit_FixGapsInRecording:
        return text.arg("Menu%20Bar_Edit").arg(tr("Fix Gaps in Recording"));
    case MenuBar_Edit_FixGPSErrors:
        return text.arg("Menu%20Bar_Edits").arg(tr("Fix GPS Errors"));
    case MenuBar_Edit_FixHRSpikes:
        return text.arg("Menu%20Bar_Edit").arg(tr("Fix HR Spikes"));
    case MenuBar_Edit_FixPowerSpikes:
        return text.arg("Menu%20Bar_Edit").arg(tr("Fix Power Spikes"));
    case MenuBar_Edit_FixSpeed:
        return text.arg("Menu%20Bar_Edit").arg(tr("Fix Speed"));
    case MenuBar_Edit_FixFreewheeling:
        return text.arg("Menu%20Bar_Edit").arg(tr("Fix Freewheeling from Power and Speed"));
    case MenuBar_Edit_FixMoxy:
        return text.arg("Menu%20Bar_Edit").arg(tr("Fix Moxy data by moving the moxy values from speed"
                                                  " and cadence into the Moxy series"));

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
    case ChartTrends_MetricTrends_Curves_Settings:
        return text.arg("ChartTypes_Trends#curves-details").arg(tr("Individual curve configuration"));
    case ChartTrends_MetricTrends_User_Data:
        return text.arg("ChartTypes_Trends#user-data").arg(tr("User defined formulas"));

    case ChartTrends_CollectionTreeMap:
        return text.arg("ChartTypes_Trends#collection-tree-map").arg(tr("Tree map visulation of activity data by two selectable dimensions for a configurable metric"));

    case ChartTrends_Critical_MM:
        return text.arg("ChartTypes_Trends#critical-mean-maximal").arg(tr("Critical Mean Maximal Power Curve"));
    case ChartTrends_Critical_MM_Config_Settings:
        return text.arg("ChartTypes_Trends#critical-mean-maximal").arg(tr("Basic configuration like date range, what series to use and how to plot"));
    case ChartTrends_Critical_MM_Config_Model:
        return text.arg("ChartTypes_Trends#critical-mean-maximal").arg(tr("Configuration of the CP Model to be used to plot the curve"));
    case ChartTrends_Distribution:
        return text.arg("ChartTypes_Trends#distribution").arg(tr("Distribution of activity data samples or metrics according time spent in a certain segment"));
     case ChartTrends_DateRange:
        return text.arg("ChartTypes_Trends#date-range-selection").arg(tr("Definition which date range is used for this particular chart"));
    case ChartDiary_Calendar:
        return text.arg("ChartTypes_Diary#calendar").arg(tr("Calendar"));
    case ChartDiary_Navigator:
        return text.arg("ChartTypes_Diary#navigator").arg(tr("Configurable activity log - with build in search capabilities"));
    case ChartRides_Summary:
        return text.arg("ChartTypes_Activities#activity-summary").arg(tr("Detailed information of a single activity - the metrics shown here are configurable"));
    case ChartRides_Details:
        return text.arg("ChartTypes_Activities#details").arg("Configurable tabbed view of activity detail data, plus technical details and change log");
    case ChartRides_Editor:
        return text.arg("ChartTypes_Activities#editor").arg(tr("Editor for activity file data - allowing to change/correct data, find entries and find anomalies"));

    case ChartRides_Performance:
        return text.arg("ChartTypes_Activities#performance").arg(tr("Plot of all activity data series in various ways"));
    case ChartRides_Performance_Config_Basic:
        return text.arg("ChartTypes_Activities#performance-basic").arg(tr("Selection how the power data series is shown in the plot and general settings on the diagram structure"));
    case ChartRides_Performance_Config_Series:
        return text.arg("ChartTypes_Activities#performance-series").arg(tr("Selection of all additional curves to be shown in the diagram - plotted only in case data is available in the activity file"));

    case ChartRides_Critical_MM:
        return text.arg("ChartTypes_Activities#critical-mean-maximals").arg(tr("Critical Mean Maximal Power Curve"));
    case ChartRides_Critical_MM_Config_Settings:
        return text.arg("ChartTypes_Activities#critical-mean-maximal").arg(tr("Basic configuration like date range, what series to use and how to plot"));
    case ChartRides_Critical_MM_Config_Model:
        return text.arg("ChartTypes_Activities#critical-mean-maximal").arg(tr("Configuration of the CP Model to be used to plot the curve"));

    case ChartRides_Histogram:
        return text.arg("ChartTypes_Activities#histogram").arg(tr("Distribution of activity data samples or metrics according time spent in a certain segment"));
    case ChartRides_PFvV:
        return text.arg("ChartTypes_Activities#pedal-force-vs-velocity").arg(tr("Quadrant analysis of pedal velocity vs. effective pedal force"));
    case ChartRides_HRvsPw:
        return text.arg("ChartTypes_Activities#heartrate-vs-power").arg(tr("Analysis of heartrate vs. power along the activity data"));
    case ChartRides_Map:
        return text.arg("ChartTypes_Activities#google-map--bing-map").arg(tr("Map of activity"));
    case ChartRides_2D:
        return text.arg("ChartTypes_Activities#2d-plot").arg(tr("Configurable 2D scatter plot of the current activity"));
    case ChartRides_3D:
        return text.arg("ChartTypes_Activities#3d-plot").arg(tr("Configurable 3D plot of the current activity"));
    case ChartRides_Aerolab:
        return text.arg("ChartTypes_Activities#aerolab-chung-analysis").arg(tr("Chung's Aerolab analysis"));

    case Chart_Summary:
        return text.arg("ChartTypes_Trends#summary").arg(tr("Overview/summary of the selected data range - data shown in 'Athlete's Best' are configurable"));
    case Chart_Summary_Config:
        return text.arg("ChartTypes_Trends#summary").arg(tr("Chart specific filter/search and date range settings"));

    // Sidebars
    case SideBarTrendsView_DateRanges:
        return text.arg("Side-Bar_Trends-view#date-ranges").arg(tr("Predefined and configurable set of data ranges for selection of activities to be analysed"));
    case SideBarTrendsView_Events:
        return text.arg("Side-Bar_Trends-view#events").arg(tr("Definition of points in time 'Events' which are marked explicitely on time related diagrams"));
    case SideBarTrendsView_Summary:
        return text.arg("Side-Bar_Trends-view#summary").arg(tr("Simple summary view"));
    case SideBarTrendsView_Filter:
        return text.arg("Side-Bar_Trends-view#filters").arg(tr("Powerful filter and search engine to determine the activities which are considered in diagram"));
    case SideBarTrendsView_Charts:
        return text.arg("Side-Bar_Trends-view#charts").arg(tr("Alternative access to the charts created for the main view"));
    case SideBarRidesView_Calendar:
        return text.arg("Side-Bar_Activities-view#calendar").arg(tr("Calendar"));
    case SideBarRidesView_Rides:
        return text.arg("Side-Bar_Activities-view#activities").arg(tr("Configurable list of activities"));
    case SideBarRidesView_Intervals:
        return text.arg("Side-Bar_Activities-view#intervals").arg(tr("Display the available and add new intervals using simple query methods"));
    case SideBarDiaryView_Calendar:
        return text.arg("Side%20Bar_Diary%20view#calendar").arg(tr("Calendar"));
    case SideBarDiaryView_Summary:
        return text.arg("Side%20Bar_Diary%20view#summary").arg(tr("Simple summary view"));

    // Cross Functions
    case SearchFilterBox:
        return text.arg("Special-Topics_SearchFilter").arg(tr("Entry field for sophisticated Searching and Filtering of activities"));
    case FindIntervals:
        return text.arg("Side-Bar_Activities-view#intervals").arg(tr("Adding intervals to an activity using simple query methods"));

    // Preferences
    case Preferences_General:
        return text.arg("Preferences_General").arg(tr("General"));
    case Preferences_Athlete_About:
        return text.arg("Preferences_Athlete").arg(tr("Athlete"));
    case Preferences_Athlete_About_Phys:
        return text.arg("Preferences_Athlete_Phys").arg(tr("Athlete"));
    case Preferences_Athlete_TrainingZones_Power:
        return text.arg("Preferences_Athlete_Training-Zones#power-zones").arg(tr("Training Zone definition for power"));
    case Preferences_Athlete_TrainingZones_HR:
        return text.arg("Preferences_Athlete_Training-Zones#heartrate-zones").arg(tr("Training Zone definition for heartrate"));
    case Preferences_Athlete_TrainingZones_Pace:
        return text.arg("Preferences_Athlete_Training-Zones#pace-zones").arg(tr("Training Zone definition for Swim and Run"));
    case Preferences_Athlete_Autoimport:
        return text.arg("Preferences_Athlete_Autoimport").arg(tr("Autoimport"));
    case Preferences_Passwords:
        return text.arg("Preferences_Passwords").arg(tr("Passwords"));
    case Preferences_Appearance:
        return text.arg("Preferences_Appearance").arg(tr("Appearance"));
    case Preferences_Intervals:
        return text.arg("Preferences_Intervals").arg(tr("Automatic Interval Detection"));
    case Preferences_DataFields:
        return text.arg("Preferences_Data%20Fields").arg(tr("Data Fields"));
    case Preferences_DataFields_Fields:
        return text.arg("Preferences_Data%20Fields#fields").arg(tr("Data Fields"));
    case Preferences_DataFields_Notes_Keywords:
        return text.arg("Preferences_Data%20Fields#notes-keywords").arg(tr("Definition of coloring rules for activities"));
    case Preferences_DataFields_Defaults:
        return text.arg("Preferences_Data%20Fields#defaults").arg(tr("Definition of default value(s) for data fields"));
    case Preferences_DataFields_Processing:
        return text.arg("Preferences_Data%20Fields#processing").arg(tr("Definition of processing default parameters for the fix, adjust,... tools"));
    case Preferences_Metrics:
        return text.arg("Preferences_Metrics").arg(tr("Metrics"));
    case Preferences_Metrics_Best:
        return text.arg("Preferences_Metrics#bests").arg(tr("Metrics shown in 'Bests'"));
    case Preferences_Metrics_Summary:
        return text.arg("Preferences_Metrics#summary").arg(tr("Metrics shown in 'Summary'"));
    case Preferences_Metrics_Intervals:
        return text.arg("Preferences_Metrics#intervals").arg(tr("Metrics shown in 'Intervals'"));
    case Preferences_TrainDevices:
        return text.arg("Preferences_Train Devices").arg(tr("Train Devices"));

    }

    return text.arg("").arg("Golden Cheetah Wiki");

}

