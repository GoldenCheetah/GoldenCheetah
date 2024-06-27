/*
 * Copyright (c) 2014 Joern Rischmueller(joern.rm@gmail.com)
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

#ifndef _GC_HelpWhatsThis_h
#define _GC_HelpWhatsThis_h

#include <QWhatsThis>
#include <QEvent>
#include <QWhatsThisClickedEvent>
#include <QWidget>
#include <QAction>
#include <QUrl>

class HelpWhatsThis : public QObject
{
Q_OBJECT

 public:
    enum GCHelp{ Default,

                 ScopeBar,
                 ScopeBar_Athletes,
                 ScopeBar_Trends,
                 ScopeBar_Rides,
                 ScopeBar_Train,
                 ScopeBar_Equipment,
                 ScopeBar_Sync,
                 ScopeBar_Options,

                 ToolBar_Back,
                 ToolBar_Forward,
                 ToolBar_PerspectiveSelector,
                 ToolBar_ToggleSidebar,
                 ToolBar_ToggleComparePane,
                 ToolBar_TabTile,

                 MenuBar_Athlete,

                 MenuBar_Activity,
                 MenuBar_Activity_Download,
                 MenuBar_Activity_Import,
                 MenuBar_Activity_Manual,
                 MenuBar_Activity_Manual_LapsEditor,
                 MenuBar_Activity_BatchProcessing,
                 MenuBar_Activity_SplitRide,
                 MenuBar_Activity_CombineRides,

                 MenuBar_Share,
                 MenuBar_Share_Online,


                 MenuBar_Tools,
                 MenuBar_Tools_CP_EST,
                 MenuBar_Tools_AirDens_EST,
                 MenuBar_Tools_VDOT_CALC,
                 MenuBar_Tools_Download_BodyMeasures,
                 MenuBar_Tools_Download_Measures,
                 MenuBar_Tools_Download_ERGDB,
                 MenuBar_Tools_Download_TP,
                 MenuBar_Tools_Download_StravaRoutes,
                 MenuBar_Tools_CreateWorkout,
                 MenuBar_Tools_ScanDisk_WorkoutVideo,
                 MenuBar_Tools_CreateHeatMap,

                 MenuBar_Edit,
                 MenuBar_Edit_AddTorqueValues,
                 MenuBar_Edit_AdjustPowerValues,
                 MenuBar_Edit_AdjustTorqueValues,
                 MenuBar_Edit_EstimatePowerValues,
                 MenuBar_Edit_EstimateDistanceValues,
                 MenuBar_Edit_EstimateHeadwindValues,
                 MenuBar_Edit_FixElevationErrors,
                 MenuBar_Edit_FixGapsInRecording,
                 MenuBar_Edit_FixGPSErrors,
                 MenuBar_Edit_FixHRSpikes,
                 MenuBar_Edit_FixPowerSpikes,
                 MenuBar_Edit_FixSpeed,
                 MenuBar_Edit_FixLapSwim,
                 MenuBar_Edit_FixFreewheeling,
                 MenuBar_Edit_FixMoxy,
                 MenuBar_Edit_FilterHrv,
                 MenuBar_Edit_FixSmO2,
                 MenuBar_Edit_FixCdA,


                 MenuBar_View,
                 MenuBar_Help,

                 ChartTrends_MetricTrends,
                 ChartTrends_MetricTrends_Config_Basic,
                 ChartTrends_MetricTrends_Config_Preset,
                 ChartTrends_MetricTrends_Config_Curves,
                 ChartTrends_MetricTrends_Curves_Settings,
                 ChartTrends_MetricTrends_User_Data,

                 ChartTrends_CollectionTreeMap,
                 ChartTrends_Critical_MM,
                 ChartTrends_Critical_MM_Config_Settings,
                 ChartTrends_Critical_MM_Config_Model,

                 ChartTrends_Distribution,
                 ChartTrends_DateRange,

                 ChartDiary_Calendar,
                 ChartDiary_Navigator,

                 ChartRides_Overview,
                 ChartRides_Overview_Config,
                 ChartRides_Summary,
                 ChartRides_Details,
                 ChartRides_Editor,
                 ChartRides_Performance,
                 ChartRides_Performance_Config_Basic,
                 ChartRides_Performance_Config_Series,
                 ChartRides_Critical_MM,
                 ChartRides_Critical_MM_Config_Settings,
                 ChartRides_Critical_MM_Config_Model,

                 ChartRides_Histogram,
                 ChartRides_PFvV,
                 ChartRides_HRvsPw,
                 ChartRides_Map,
                 ChartRides_2D,
                 ChartRides_3D,
                 ChartRides_Aerolab,

                 Chart_Summary,
                 Chart_Summary_Config,
                 Chart_Overview,
                 Chart_Overview_Config,
                 Chart_R,
                 Chart_Python,
                 Chart_User,
                 Chart_Web,

                 ChartTrain_Telemetry,
                 ChartTrain_Workout,
                 ChartTrain_Realtime,
                 ChartTrain_PedalStroke,
                 ChartTrain_VideoPlayer,
                 ChartTrain_WorkoutEditor,
                 ChartTrain_LiveMap,

				 EquipmentWindow_Details,

                 SideBarTrendsView_DateRanges,
                 SideBarTrendsView_Events,
                 SideBarTrendsView_Summary,
                 SideBarTrendsView_Filter,
                 SideBarTrendsView_Charts,
                 SideBarRidesView_Calendar,
                 SideBarRidesView_Rides,
                 SideBarRidesView_Intervals,
                 SideBarDiaryView_Calendar,
                 SideBarDiaryView_Summary,
                 SideBarTrainView_Devices,
                 SideBarTrainView_Workouts,
                 SideBarTrainView_Media,
                 SideBarTrainView_VideoSync,
				 SideBarEquipmentView_Eq,

                 ComparePane,
                 WorkoutControl,

                 SearchFilterBox,
                 FindIntervals,

                 Preferences_General,
                 Preferences_Athlete_About,
                 Preferences_Athlete_Measures,
                 Preferences_Athlete_About_Model,
                 Preferences_Athlete_TrainingZones_Power,
                 Preferences_Athlete_TrainingZones_HR,
                 Preferences_Athlete_TrainingZones_Pace,
                 Preferences_Athlete_Autoimport,
                 Preferences_Athlete_Backup,
                 Preferences_Passwords,
                 Preferences_Appearance,
                 Preferences_Intervals,
                 Preferences_Measures,
                 Preferences_DataFields,
                 Preferences_DataFields_Fields,
                 Preferences_DataFields_Notes_Keywords,
                 Preferences_DataFields_Defaults,
                 Preferences_DataFields_Processing,

                 Preferences_Metrics,
                 Preferences_Metrics_Favourites,
                 Preferences_Metrics_Custom,
                 Preferences_Metrics_UserMetrics,

                 Preferences_Training,
                 Preferences_Training_Preferences,
                 Preferences_Training_RemoteControls,
                 Preferences_Training_TrainDevices,
                 Preferences_Training_AddDeviceWizard,
                 Preferences_Training_VirtualBicycleSpecifications,

                 };

    HelpWhatsThis (QObject *object = 0);

    QString getWhatsThisText(GCHelp chapter);

 protected:
    bool eventFilter( QObject*, QEvent* );

 private:
    static QString getText(GCHelp chapter);

};


#endif
