/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_Settings_h
#define _GC_Settings_h 1

#define GC_SETTINGS_CO              "goldencheetah.org"
#define GC_SETTINGS_APP             "GoldenCheetah"
#define GC_SETTINGS_LAST            "mainwindow/lastOpened"
#define GC_SETTINGS_MAIN_WIDTH      "mainwindow/width"
#define GC_SETTINGS_MAIN_HEIGHT     "mainwindow/height"
#define GC_SETTINGS_MAIN_X          "mainwindow/x"
#define GC_SETTINGS_MAIN_Y          "mainwindow/y"
#define GC_SETTINGS_MAIN_GEOM       "mainwindow/geometry"
#define GC_SETTINGS_SPLITTER_SIZES  "mainwindow/splitterSizes"
#define GC_SETTINGS_SUMMARYSPLITTER_SIZES "mainwindow/summarysplittersizes"
#define GC_SETTINGS_CALENDAR_SIZES  "mainwindow/calendarSizes"
#define GC_TABS_TO_HIDE             "mainwindow/tabsToHide"
#define GC_SETTINGS_SUMMARY_METRICS "rideSummaryWindow/summaryMetrics"
#define GC_SETTINGS_INTERVAL_METRICS "rideSummaryWindow/intervalMetrics"
#define GC_RIDE_PLOT_SMOOTHING       "ridePlot/Smoothing"
#define GC_RIDE_PLOT_STACK           "ridePlot/Stack"
#define GC_PERF_MAN_METRIC           "performanceManager/metric"
#define GC_HIST_BIN_WIDTH            "histogamWindow/binWidth"
#define GC_SETTINGS_SUMMARY_METRICS_DEFAULT "skiba_xpower,skiba_relative_intensity,skiba_bike_score,daniels_points,daniels_equivalent_power,trimp_points,aerobic_decoupling"
#define GC_SETTINGS_INTERVAL_METRICS_DEFAULT "workout_time,total_distance,total_work,average_power,skiba_xpower,max_power,average_hr,ninety_five_percent_hr,average_cad,average_speed"
#define GC_DATETIME_FORMAT          "ddd MMM dd, yyyy, hh:mm AP"
#define GC_UNIT                     "unit"
#define GC_LANG                     "lang"
#define GC_NICKNAME                 "nickname"
#define GC_DOB                      "dob"
#define GC_WEIGHT                   "weight"
#define GC_SEX                      "sex"
#define GC_BIO                      "bio"
#define GC_AVATAR                   "avatar"
#define GC_SETTINGS_LAST_IMPORT_PATH "mainwindow/lastImportPath"
#define GC_ALLRIDES_ASCENDING       "allRidesAscending"
#define GC_CRANKLENGTH              "crankLength"
#define GC_BIKESCOREDAYS	    "bikeScoreDays"
#define GC_BIKESCOREMODE	    "bikeScoreMode"
#define GC_SB_TODAY             "PMshowSBtoday"
#define GC_PM_DAYS              "pmDays"
#define GC_INITIAL_LTS		    "initialLTS"
#define GC_INITIAL_STS		    "initialSTS"
#define GC_LTS_DAYS		    "LTSdays"
#define GC_STS_DAYS		    "STSdays"
#define GC_STS_NAME			"STSname"
#define GC_STS_ACRONYM			"STS"
#define GC_LTS_NAME			"LTSname"
#define GC_LTS_ACRONYM			"LTS"
#define GC_SB_NAME			"SBname"
#define GC_SB_ACRONYM			"SB"
#define GC_WARNCONVERT      "warnconvert"
#define GC_WARNEXIT      "warnexit"
#define GC_WORKOUTDIR      "workoutDir"
#define GC_TRAIN_SPLITTER_SIZES  "trainwindow/splitterSizes"
#define GC_LTM_SPLITTER_SIZES  "ltmwindow/splitterSizes"


// device Configurations NAME/SPEC/TYPE/DEFI/DEFR all get a number appended
// to them to specify which configured device i.e. devices1 ... devicesn where
// n is defined in GC_DEV_COUNT
#define GC_DEV_COUNT "devices"
#define GC_DEV_NAME  "devicename"
#define GC_DEV_SPEC  "devicespec"
#define GC_DEV_PROF  "deviceprof"
#define GC_DEV_TYPE  "devicetype"
#define GC_DEV_DEFI  "devicedefi"
#define GC_DEV_DEFR  "devicedefr"
#define GC_DEV_VIRTUAL  "devicepostProcess"

// data processor config
#define GC_DPFG_TOLERANCE "dataprocess/fixgaps/tolerance"
#define GC_DPFG_STOP      "dataprocess/fixgaps/stop"
#define GC_DPFS_MAX       "dataprocess/fixspikes/max"
#define GC_DPFS_VARIANCE  "dataprocess/fixspikes/variance"
#define GC_DPTA           "dataprocess/torqueadjust/adjustment"

//Twitter oauth keys
#define GC_TWITTER_CONSUMER_KEY    "qbbmhDt8bG8ZBcT3r9nYw" //< consumer key
#define GC_TWITTER_CONSUMER_SECRET "IWXu2G6mQC5xvhM8V0ohA0mPTUOqAFutiuKIva3LQg"
#define GC_TWITTER_TOKEN "twitter_token"
#define GC_TWITTER_SECRET "twitter_secret"

// Tcx Smart recording
#define GC_GARMIN_SMARTRECORD "garminSmartRecord"
#define GC_GARMIN_HWMARK "garminHWMark"

// Map Interval period
#define GC_MAP_INTERVAL "mapInterval"

#include <QSettings>
#include <boost/shared_ptr.hpp>

inline boost::shared_ptr<QSettings> GetApplicationSettings()
{
  boost::shared_ptr<QSettings> settings;
  QDir home = QDir();
    //First check to see if the Library folder exists where the executable is (for USB sticks)
  if(!home.exists("Library/GoldenCheetah"))
    settings = boost::shared_ptr<QSettings>(new QSettings(GC_SETTINGS_CO, GC_SETTINGS_APP));
  else
    settings = boost::shared_ptr<QSettings>(new QSettings(home.absolutePath()+"/gc", QSettings::IniFormat));

  return settings;
}

#endif // _GC_Settings_h
