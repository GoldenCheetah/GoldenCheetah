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
#include "GoldenCheetah.h"
#include <QDebug>

#define GC_HOMEDIR                  "homedirectory"
#define GC_VERSION_USED             "versionused"
#define GC_SAFEEXIT                 "safeexit"
#define GC_SETTINGS_CO              "goldencheetah.org"
#define GC_SETTINGS_APP             "GoldenCheetah"
#define GC_SETTINGS_LAST            "mainwindow/lastOpened"
#define GC_SETTINGS_MAIN_WIDTH      "mainwindow/width"
#define GC_SETTINGS_MAIN_HEIGHT     "mainwindow/height"
#define GC_SETTINGS_MAIN_X          "mainwindow/x"
#define GC_SETTINGS_MAIN_Y          "mainwindow/y"
#define GC_SETTINGS_MAIN_GEOM       "mainwindow/geometry"
#define GC_SETTINGS_MAIN_STATE      "mainwindow/state"
#define GC_SETTINGS_SPLITTER_SIZES  "mainwindow/splitterSizes"
#define GC_SETTINGS_CALENDAR_SIZES  "mainwindow/calendarSizes"
#define GC_TABS_TO_HIDE             "mainwindow/tabsToHide"
#define GC_ELEVATION_HYSTERESIS     "elevationHysteresis"
#define GC_SETTINGS_SUMMARY_METRICS "rideSummaryWindow/summaryMetrics"
#define GC_SETTINGS_BESTS_METRICS    "rideSummaryWindow/bestsMetrics"
#define GC_SETTINGS_INTERVAL_METRICS "rideSummaryWindow/intervalMetrics"
#define GC_RIDE_PLOT_SMOOTHING       "ridePlot/Smoothing"
#define GC_RIDE_PLOT_STACK           "ridePlot/Stack"
#define GC_PERF_MAN_METRIC           "performanceManager/metric"
#define GC_HIST_BIN_WIDTH            "histogamWindow/binWidth"
#define GC_SETTINGS_BESTS_METRICS_DEFAULT "5s_critical_power,1m_critical_power,5m_critical_power,20m_critical_power,60m_critical_power"
#define GC_SETTINGS_SUMMARY_METRICS_DEFAULT "skiba_xpower,skiba_relative_intensity,skiba_bike_score,daniels_points,daniels_equivalent_power,trimp_points,aerobic_decoupling"
#define GC_SETTINGS_INTERVAL_METRICS_DEFAULT "workout_time,total_distance,total_work,average_power,skiba_xpower,max_power,average_hr,ninety_five_percent_hr,average_cad,average_speed"
#define GC_DATETIME_FORMAT          "ddd MMM dd, yyyy, hh:mm"
#define GC_UNIT                     "unit"
#define GC_LANG                     "lang"
#define GC_NICKNAME                 "nickname"
#define GC_DOB                      "dob"
#define GC_WEIGHT                   "weight"
#define GC_SEX                      "sex"
#define GC_BIO                      "bio"
#define GC_AVATAR                   "avatar"
#define GC_IMPORTDIR                "importDir"
#define GC_IMPORTSETTINGS           "importSettings"
#define GC_SETTINGS_LAST_IMPORT_PATH "mainwindow/lastImportPath"
#define GC_SETTINGS_LAST_WORKOUT_PATH "mainwindow/lastWorkoutPath"
#define GC_LAST_DOWNLOAD_DEVICE      "mainwindow/lastDownloadDevice"
#define GC_LAST_DOWNLOAD_PORT        "mainwindow/lastDownloadPort"
#define GC_CRANKLENGTH              "crankLength"
#define GC_WHEELSIZE                "wheelsize"
#define GC_BIKESCOREDAYS	    "bikeScoreDays"
#define GC_BIKESCOREMODE	    "bikeScoreMode"
#define GC_SB_TODAY             "PMshowSBtoday"
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
#define GC_LTM_LAST_DATE_RANGE "ltmwindow/lastDateRange"
#define GC_LTM_AUTOFILTERS "ltmwindow/autofilters"
#define GC_BLANK_ANALYSIS "blank/analysis"
#define GC_BLANK_TRAIN    "blank/train"
#define GC_BLANK_HOME     "blank/home"
#define GC_BLANK_DIARY    "blank/diary"
#define GC_LINEWIDTH      "linewidth"
#define GC_ANTIALIAS      "antialias"
#define GC_CHROME         "chrome" // mac or flat only so far
#define GC_RIDEBG         "rideBG"
#define GC_RIDESCROLL     "rideScroll"
#define GC_RIDEHEAD       "rideHead"
#define GC_DROPSHADOW     "dropshadow"
#define GC_SHADEZONES     "shadezones"
#define GC_PROXYTYPE      "proxy/type"
#define GC_PROXYHOST      "proxy/host"
#define GC_PROXYPORT      "proxy/port"
#define GC_PROXYUSER      "proxy/user"
#define GC_PROXYPASS      "proxy/pass"
#define GC_GCURL          "gc/url"
#define GC_GCUSER         "gc/user"
#define GC_GCPASS         "gc/pass"
#define GC_TPURL          "tp/url"
#define GC_TPUSER         "tp/user"
#define GC_TPPASS         "tp/pass"
#define GC_TPTYPE         "tp/type"
#define GC_TWURL          "tw/url"
#define GC_TWUSER         "tw/user"
#define GC_TWPASS         "tw/pass"
#define GC_STRUSER        "str/user"
#define GC_STRPASS        "str/pass"
#define GC_RWGPSUSER      "rwgps/user"
#define GC_RWGPSPASS      "rwgps/pass"
#define GC_TTBUSER        "ttb/user"
#define GC_TTBPASS        "ttb/pass"
#define GC_SELUSER        "sel/user"
#define GC_SELPASS        "sel/pass"
#define GC_WIURL          "wi/url"
#define GC_WIUSER         "wi/user"
#define GC_WIKEY          "wi/key"
#define GC_DVURL          "dv/url"
#define GC_DVUSER         "dv/user"
#define GC_DVPASS         "dv/pass"
#define GC_ZEOURL         "zeo/url"
#define GC_ZEOUSER        "zeo/user"
#define GC_ZEOPASS        "zeo/pass"

#define GC_UNIT_METRIC    "Metric"
#define GC_UNIT_IMPERIAL  "Imperial"

// device Configurations NAME/SPEC/TYPE/DEFI/DEFR all get a number appended
// to them to specify which configured device i.e. devices1 ... devicesn where
// n is defined in GC_DEV_COUNT
#define GC_DEV_COUNT "devices"
#define GC_DEV_NAME  "devicename"
#define GC_DEV_SPEC  "devicespec"
#define GC_DEV_PROF  "deviceprof"
#define GC_DEV_TYPE  "devicetype"
#define GC_DEV_DEF   "devicedef"
#define GC_DEV_WHEEL "devicewheel"
#define GC_DEV_VIRTUAL  "devicepostProcess"

// data processor config
#define GC_DPFG_TOLERANCE "dataprocess/fixgaps/tolerance"
#define GC_DPFG_STOP      "dataprocess/fixgaps/stop"
#define GC_DPFS_MAX       "dataprocess/fixspikes/max"
#define GC_DPFS_VARIANCE  "dataprocess/fixspikes/variance"
#define GC_DPTA           "dataprocess/torqueadjust/adjustment"
#define GC_DPPA           "dataprocess/poweradjust/adjustment"
#define GC_DPFHRS_MAX     "dataprocess/fixhrspikes/max"

// ride navigator
#define GC_NAVHEADINGS    "navigator/headings"
#define GC_NAVHEADINGWIDTHS "bavigator/headingwidths"
#define GC_NAVGROUPBY       "navigator/groupby"
#define GC_SORTBY           "navigator/sortby"
#define GC_SORTBYORDER      "navigator/sortbyorder"

//Twitter oauth keys
#define GC_TWITTER_CONSUMER_KEY    "qbbmhDt8bG8ZBcT3r9nYw" //< consumer key
#define GC_TWITTER_CONSUMER_SECRET "IWXu2G6mQC5xvhM8V0ohA0mPTUOqAFutiuKIva3LQg"
#define GC_TWITTER_TOKEN "twitter_token"
#define GC_TWITTER_SECRET "twitter_secret"

//Strava
#define GC_STRAVA_CLIENT_ID    "83" // client id
#define GC_STRAVA_TOKEN "strava_token"

//Cycling Analytics
#define GC_CYCLINGANALYTICS_CLIENT_ID    "1504958" // app id
#define GC_CYCLINGANALYTICS_TOKEN "cyclinganalytics_token"

// Tcx Smart recording
#define GC_GARMIN_SMARTRECORD "garminSmartRecord"
#define GC_GARMIN_HWMARK "garminHWMark"

// Calendar sync
#define GC_WEBCAL_URL "webcal_url"

// Default view on Diary
#define GC_DIARY_VIEW "diaryview"

// Fonts
#define GC_FONT_DEFAULT           "font/default"
#define GC_FONT_TITLES            "font/titles"
#define GC_FONT_CHARTMARKERS      "font/chartmarkers"
#define GC_FONT_CHARTLABELS       "font/chartlabels"
#define GC_FONT_CALENDAR          "font/calendar"
#define GC_FONT_DEFAULT_SIZE      "font/defaultsize"
#define GC_FONT_TITLES_SIZE       "font/titlessize"
#define GC_FONT_CHARTMARKERS_SIZE "font/chartmarkerssize"
#define GC_FONT_CHARTLABELS_SIZE  "font/chartlabelssize"
#define GC_FONT_CALENDAR_SIZE     "font/calendarsize"

// TreeMap selection
#define GC_TM_FIRST               "tm/first"
#define GC_TM_SECOND              "tm/second"
#define GC_TM_METRIC              "tm/metric"

#define FORTIUS_FIRMWARE          "fortius/firmware"
#define TRAIN_MULTI               "train/multi"

// batch export last options
#define GC_BE_LASTDIR             "batchexport/lastdir"
#define GC_BE_LASTFMT             "batchexport/lastfmt"

// show tabbar
#define GC_TABBAR                 "show/tabbar"

// wbal formula to use
#define GC_WBALFORM               "wbal/formula"

#include <QSettings>
#include <QFileInfo>

// wrap the standard QSettings so we can offer members
// to get global or cyclist specific settings
// via value() and cvalue()
class GSettings : public QSettings
{
    public:
    GSettings(QString org, QString scope) : QSettings(org,scope) { }
    GSettings(QString file, Format format) : QSettings(file,format) { }
    ~GSettings() { QSettings::sync(); }

    // standard access to global config
    QVariant value(const QObject *me, const QString key, const QVariant def = 0) const ;
    void setValue(QString key, QVariant value) {
        QSettings::setValue(key,value);
    }

    // access to cyclist specific config
    QVariant cvalue(QString cyclist, QString key, QVariant def = 0) {
        return QSettings::value(cyclist+"/"+key, def);
    }
    void setCValue(QString cyclist, QString key, QVariant value) {
        QSettings::setValue(cyclist + "/" + key,value);
    }

};

extern GSettings *appsettings;
extern int OperatingSystem;

#define WINDOWS 1
#define LINUX   2
#define OSX     3
#define OPENBSD 4

#if QT_VERSION > 0x050000
#define OS_STYLE "Fusion"
#else
#define OS_STYLE "Plastique"
#endif
#endif // _GC_Settings_h
