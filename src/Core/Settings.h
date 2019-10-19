/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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


/*
 *  Global GC Properties which are not stored as GSettings are defined here
 */

#define GC_SETTINGS_CO                        "goldencheetah.org"
#ifndef GC_SETTINGS_APP
#define GC_SETTINGS_APP                       "GoldenCheetah"
#endif
#define GC_SETTINGS_BESTS_METRICS_DEFAULT "5s_critical_power,1m_critical_power,5m_critical_power,20m_critical_power,60m_critical_power,3m_critical_pace,20m_critical_pace,3m_critical_pace_swim,20m_critical_pace_swim"
#define GC_SETTINGS_SUMMARY_METRICS_DEFAULT "triscore,skiba_xpower,skiba_relative_intensity,xPace,swimscore_xpace,trimp_points,aerobic_decoupling"
#define GC_SETTINGS_INTERVAL_METRICS_DEFAULT "workout_time,total_distance,total_work,average_power,average_hr,average_cad,average_speed,pace,pace_swim,distance_swim"
#define GC_UNIT_METRIC                       "Metric"
#define GC_UNIT_IMPERIAL                     "Imperial"

//Google Calendar-CALDAV oauthkeys / see also Athlete parameter
#define GC_GOOGLE_CALENDAR_CLIENT_ID      "426009671216-c588t1u6hafep30tfs7g0g1nuo72s8ko.apps.googleusercontent.com"

//Today's Plan
#define GC_TODAYSPLAN_CLIENT_ID    "GoldenCheetah" // app id

//Cycling Analytics / see also Athlete parameter
#define GC_CYCLINGANALYTICS_CLIENT_ID    "1504958" // app id

//Withings oauth keys
#ifndef GC_WITHINGS_CONSUMER_KEY
#define GC_WITHINGS_CONSUMER_KEY    "292875b6883b87e27cefd2555d1cb872b2282f02fee25b4906871314934" //< consumer key
#endif

//Nokia
#ifndef GC_NOKIA_CLIENT_ID
#define GC_NOKIA_CLIENT_ID    "eaa338e1f64bcdb8579d21550df661c84d1d28e2053c5d0ede4600c1ebc8b6cd"
#endif

// Dropbox id
#ifndef GC_DROPBOX_CLIENT_ID
#define GC_DROPBOX_CLIENT_ID "753fbblhri06ah3"
#endif

// PolarFlow client ID
#ifndef GC_POLARFLOW_CLIENT_ID
#define GC_POLARFLOW_CLIENT_ID "Not defined"
#endif

/*
 *  GoldenCheetah Properties are stored in different locations, depending on the prefix defined in the property name. The following different prefixes are supported
 *  <system>
 *  - Storage: the property value is stored in the system specific QSettings - e.g. Registry on Windows, PList on Mac,...
 *  - Use: such properties relate to GC instance running / e.g. dependent on the hardware/screen-size,...
 *
 *  <global-$filename>
 *  - Storage: the property value is stored in the .INI file with name "global_$filename.gc", which means that any change of the athlete directory means the settings are lost / have to be copied
 *  - Use: such properties are shared between all Athletes in the same athlete Directory and are the same for all GC instance which point to this Athlete Directory
 *    The supported filenames (substitute for $filename) are defined as GC_SETTINGS_... below - ANYTHING ELSE WILL CAUSE ERRORS - OR JUST NOT STORE ANYTHING WORK
 *
 *  <athlete-$filename>
 *  - Storage: the property is stored in .INI file which is store in the athlete/config directory - the name of the file is "athlete-$filename.gc"
 *  - Use: such properties are Athlete specific - the separation in different config files is aimed to help sharing such properties with others and easier backup/restore
 *    The supported filenames (substitute for $filename) are defined as GC_SETTINGS_... below - - ANYTHING ELSE WILL CAUSE ERRORS - OR JUST NOT STORE ANYTHING WORK

 *
 *  Settings which get special coded treatment:
 *  - Colors.h/Color.cpp -> stored in <system> since every PC/Mac may look different / similar to Fonts
 *
 */

// - Prefixes to be used for the QSettings in any code - to identify which QSettings are to be used

#define GC_QSETTINGS_SYSTEM              "<system>"
#define GC_QSETTINGS_GLOBAL_GENERAL      "<global-general>"
#define GC_QSETTINGS_GLOBAL_TRAIN        "<global-trainmode>"
#define GC_QSETTINGS_ATHLETE_GENERAL     "<athlete-general>"
#define GC_QSETTINGS_ATHLETE_LAYOUT      "<athlete-layout>"
#define GC_QSETTINGS_ATHLETE_PREFERENCES "<athlete-preferences>"
#define GC_QSETTINGS_ATHLETE_PRIVATE     "<athlete-private>"

// -----------------------------------------------------------------------------------------------------------------------
//    System specific properties - OS/PC dependent - stored in .ini file in the OS specific directory defined by QSettings
// -----------------------------------------------------------------------------------------------------------------------

#define GC_HOMEDIR                      "<system>homedirectory"
#define GC_START_HTTP                   "<system>starthttp"
#define GC_EMBED_R                      "<system>embedR"
#define GC_EMBED_PYTHON                 "<system>embedPython"

#define GC_SETTINGS_LAST                "<system>mainwindow/lastOpened"
#define GC_SETTINGS_MAIN_GEOM           "<system>mainwindow/geometry"
#define GC_SETTINGS_MAIN_STATE          "<system>mainwindow/state"
#define GC_SETTINGS_LAST_IMPORT_PATH    "<system>mainwindow/lastImportPath"
#define GC_SETTINGS_LAST_WORKOUT_PATH   "<system>mainwindow/lastWorkoutPath"
#define GC_LAST_DOWNLOAD_DEVICE         "<system>mainwindow/lastDownloadDevice"
#define GC_LAST_DOWNLOAD_PORT           "<system>mainwindow/lastDownloadPort"

// batch export last options
#define GC_BE_LASTDIR                   "<system>batchexport/lastdir"
#define GC_BE_LASTFMT                   "<system>batchexport/lastfmt"
// Fonts
#define GC_FONT_DEFAULT                 "<system>font/default"
#define GC_FONT_CHARTLABELS             "<system>font/chartlabels"
#define GC_FONT_DEFAULT_SIZE            "<system>font/defaultsize"
#define GC_FONT_CHARTLABELS_SIZE        "<system>font/chartlabelssize"
#define GC_FONT_SCALE                   "<system>font/scale"

// Colors/Chrome - see special treatment sections (also stored in <system>)
#define GC_CHROME                       "<system>chrome" // mac or flat only so far

// Location of R Installation - follows R_HOME semantics
#define GC_R_HOME                       "<system>r_home"

// Location of Python Installation - follows PYTHONHOME semantics
#define GC_PYTHON_HOME                       "<system>pythonhome"

// --------------------------------------------------------------------
// Global Properties - Stored in "root" of the active Athlete Directory
// --------------------------------------------------------------------


#define GC_SETTINGS_SUMMARY_METRICS     "<global-general>rideSummaryWindow/summaryMetrics"
#define GC_SETTINGS_BESTS_METRICS       "<global-general>rideSummaryWindow/bestsMetrics"
#define GC_SETTINGS_INTERVAL_METRICS    "<global-general>rideSummaryWindow/intervalMetrics"
#define GC_TABBAR                       "<global-general>show/tabbar"                        // show tabbar
#define GC_WBALFORM                     "<global-general>wbal/formula"                       // wbal formula to use
#define GC_BIKESCOREDAYS                    "<global-general>bikeScoreDays"
#define GC_BIKESCOREMODE                    "<global-general>bikeScoreMode"
#define GC_WARNCONVERT                  "<global-general>warnconvert"
#define GC_WARNEXIT                     "<global-general>warnexit"
#define GC_HIST_BIN_WIDTH               "<global-general>histogamWindow/binWidth"
#define GC_WORKOUTDIR                   "<global-general>workoutDir"                         // used for Workouts and Videosyn files
#define GC_LINEWIDTH                    "<global-general>linewidth"
#define GC_ANTIALIAS                    "<global-general>antialias"
#define GC_RIDEBG                       "<global-general>rideBG"
#define GC_RIDESCROLL                   "<global-general>rideScroll"
#define GC_RIDEHEAD                     "<global-general>rideHead"
#define GC_SHADEZONES                   "<global-general>shadezones"
#define GC_LANG                         "<global-general>lang"
#define GC_PACE                         "<global-general>pace"
#define GC_SWIMPACE                     "<global-general>swimpace"
#define GC_ELEVATION_HYSTERESIS         "<global-general>elevationHysteresis"
#define GC_UNIT                         "<global-general>unit"
#define GC_ALLOW_TELEMETRY              "<global-general>telemetryAllowed"
#define GC_ALLOW_TELEMETRY_DATE         "<global-general>telemetryDecisionDate"
#define GC_TELEMETRY_ID                 "<global-general>telemetryId"
#define GC_TELEMETRY_UPDATE_COUNTER     "<global-general>telemetryUpdateCounter"
#define GC_LAST_VERSION_CHECKED         "<global-general>lastVersionChecked"
#define GC_LAST_VERSION_CHECK_DATE      "<global-general>lastVersionCheckDate"



// Fit/Tcx Smart recording
#define GC_GARMIN_SMARTRECORD           "<global-general>garminSmartRecord"
#define GC_GARMIN_HWMARK                "<global-general>garminHWMark"

// data processor config
#define GC_DPFG_TOLERANCE               "<global-general>dataprocess/fixgaps/tolerance"
#define GC_DPFG_STOP                    "<global-general>dataprocess/fixgaps/stop"
#define GC_DPFS_MAX                     "<global-general>dataprocess/fixspikes/max"
#define GC_DPFS_VARIANCE                "<global-general>dataprocess/fixspikes/variance"
#define GC_DPTA                         "<global-general>dataprocess/torqueadjust/adjustment"
#define GC_DPPA                         "<global-general>dataprocess/poweradjust/adjustment"
#define GC_DPPA_ABS                     "<global-general>dataprocess/poweradjust/adjustment_abs"
#define GC_DPFHRS_MAX                   "<global-general>dataprocess/fixhrspikes/max"
#define GC_DPDP_BIKEWEIGHT              "<global-general>dataprocess/fixderivepower/bikewheight"
#define GC_DPDP_CRR                     "<global-general>dataprocess/fixderivepower/crr"
#define GC_DPDP_CDA                     "<global-general>dataprocess/fixderivepower/cda"
#define GC_DPDP_DRAFTM                  "<global-general>dataprocess/fixderivepower/draftm"
#define GC_DPRP_EQUIPWEIGHT             "<global-general>dataprocess/fixrunningpower/equipwheight"
#define GC_DPDR_DRAFTM                  "<global-general>dataprocess/fixrunningpower/draftm"
#define GC_DPFV_MA                      "<global-general>dataprocess/fixspeed/ma"
#define GC_CAD2SMO2                     "<global-general>dataprocess/fixmoxy/cad2smo2"
#define GC_SPD2THB		            	"<global-general>dataprocess/fixmoxy/spd2thb"
#define GC_DPFLS_PL                     "<global-general>dataprocess/fixlapswim/pool_length"
#define GC_RR_MAX                       "<global-general>dataprocess/filterhrv/rr_max"                 //
#define GC_RR_MIN                       "<global-general>dataprocess/filterhrv/rr_min"                 //
#define GC_RR_FILT                      "<global-general>dataprocess/filterhrv/rr_filt"                 //
#define GC_RR_WINDOW                    "<global-general>dataprocess/filterhrv/rr_window"
#define GC_RR_SET_REST_HRV              "<global-general>dataprocess/filterhrv/rr_set_rest_hrv"
#define GC_MOXY_FIX_SMO2                "<global-general>dataprocess/fixmoxydata/fix_smo2"
#define GC_MOXY_FIX_THB                 "<global-general>dataprocess/fixmoxydata/fix_thb"
#define GC_MOXY_FIX_THB_MAX             "<global-general>dataprocess/fixmoxydata/fix_thb_max"


// device Configurations NAME/SPEC/TYPE/DEFI/DEFR all get a number appended
// to them to specify which configured device i.e. devices1 ... devicesn where
// n is defined in GC_DEV_COUNT
#define GC_DEV_COUNT                    "<global-trainmode>devices"
#define GC_DEV_NAME                     "<global-trainmode>devicename"
#define GC_DEV_SPEC                     "<global-trainmode>devicespec"
#define GC_DEV_PROF                     "<global-trainmode>deviceprof"
#define GC_DEV_TYPE                     "<global-trainmode>devicetype"
#define GC_DEV_STRIDE                   "<global-trainmode>devicestride"
#define GC_DEV_DEF                      "<global-trainmode>devicedef"
#define GC_DEV_WHEEL                    "<global-trainmode>devicewheel"
#define GC_DEV_VIRTUAL                  "<global-trainmode>devicepostProcess"
#define FORTIUS_FIRMWARE                "<global-trainmode>fortius/firmware"
#define IMAGIC_FIRMWARE                 "<global-trainmode>imagic/firmware"
#define TRAIN_MULTI                     "<global-trainmode>train/multi"
#define TRAIN_AUTOCONNECT               "<global-trainmode>train/autoconnect"
#define TRAIN_AUTOHIDE                  "<global-trainmode>train/autohide"
#define TRAIN_LAPALERT                  "<global-trainmode>train/lapalert"
#define GC_REMOTE_START                 "<global-trainmode>remote/start"
#define GC_REMOTE_STOP                  "<global-trainmode>remote/stop"
#define GC_REMOTE_LAP                   "<global-trainmode>remote/lap"
#define GC_REMOTE_HIGHER                "<global-trainmode>remote/higher"
#define GC_REMOTE_LOWER                 "<global-trainmode>remote/lower"
#define GC_REMOTE_CALIBRATE             "<global-trainmode>remote/calibrate"

// --------------------------------------------------------------------------------
// Athlete Specific Properties - Stored in /config subfolder of the related athlete
// --------------------------------------------------------------------------------

#define GC_ATHLETE_ID                   "<athlete-general>id"
#define GC_VERSION_USED                 "<athlete-general>versionused"
#define GC_SAFEEXIT                     "<athlete-general>safeexit"
#define GC_UPGRADE_FOLDER_SUCCESS       "<athlete-general>upgradesuccess/folder"     // success tracking of folder upgrade stored on athlete level
#define GC_ATHLETE_SNIPPETID            "<athlete-general>snippetid"

#define GC_OPENDATA_GRANTED             "<athlete-general>opendata/allowed"         // did the user grant permission? (Y, N, X)
                                                                                    // X means we haven't asked yet (default)
#define GC_OPENDATA_RUNCOUNT            "<athlete-general>opendata/runcount"        // times launched, so ask after a while and
                                                                                    // make sure they're a regular user
#define GC_OPENDATA_LASTPOSTED          "<athlete-general>opendata/postingdate"     // when did we last send data?
#define GC_OPENDATA_LASTPOSTCOUNT       "<athlete-general>opendata/count"           // when we last posted how many workouts?
#define GC_OPENDATA_LASTPOSTVERSION     "<athlete-general>opendata/version"         // when we last posted how many workouts?

#define GC_LTM_LAST_DATE_RANGE          "<athlete-layout>ltmwindow/lastDateRange"
#define GC_LTM_AUTOFILTERS              "<athlete-layout>ltmwindow/autofilters"
#define GC_BLANK_ANALYSIS               "<athlete-layout>blank/analysis"
#define GC_BLANK_TRAIN                  "<athlete-layout>blank/train"
#define GC_BLANK_HOME                   "<athlete-layout>blank/home"
#define GC_BLANK_DIARY                  "<athlete-layout>blank/diary"
#define GC_SETTINGS_SPLITTER_SIZES      "<athlete-layout>mainwindow/splitterSizes"


#define GC_NICKNAME                     "<athlete-preferences>nickname"
#define GC_DOB                          "<athlete-preferences>dob"
#define GC_WEIGHT                       "<athlete-preferences>weight"
#define GC_HEIGHT                       "<athlete-preferences>height"
#define GC_WBALTAU                      "<athlete-preferences>wbaltau"
#define GC_SEX                          "<athlete-preferences>sex"
#define GC_BIO                          "<athlete-preferences>bio"
#define GC_AVATAR                       "<athlete-preferences>avatar"
#define GC_DISCOVERY                    "<athlete-preferences>intervals/discovery"   // intervals to discover
#define GC_SB_TODAY                     "<athlete-preferences>PMshowSBtoday"
#define GC_LTS_DAYS                             "<athlete-preferences>LTSdays"
#define GC_STS_DAYS                             "<athlete-preferences>STSdays"
#define GC_CRANKLENGTH                  "<athlete-preferences>crankLength"
#define GC_WHEELSIZE                    "<athlete-preferences>wheelsize"
#define GC_USE_CP_FOR_FTP               "<athlete-preferences>cp/useforftp"                       // use CP for FTP
#define GC_USE_CP_FOR_FTP_RUN           "<athlete-preferences>cp/useforftprun"                    // use CP for FTP
#define GC_NETWORKFILESTORE_FOLDER      "<athlete-preferences>networkfilestore/folder"            // folder to sync with
#define GC_AUTOBACKUP_FOLDER            "<athlete-preferences>autobackup/folder"
#define GC_AUTOBACKUP_PERIOD            "<athlete-preferences>autobackup/period"                  // how often is the Athlete Folder backuped up / 0 == never
#define GC_AUTOBACKUP_COUNTER           "<athlete-preferences>autobackup/counter"                 // counts to the next backup

#define GC_CLOUDDB_TC_ACCEPTANCE       "<athlete-preferences>clouddb/acceptance"                  // bool
#define GC_CLOUDDB_TC_ACCEPTANCE_DATE  "<athlete-preferences>clouddb/acceptancedate"              // date/time string of acceptance
#define GC_CLOUDDB_EMAIL               "<athlete-preferences>clouddb/email"

// ride navigator
#define GC_NAVHEADINGS                  "<athlete-preferences>navigator/headings"
#define GC_NAVHEADINGWIDTHS             "<athlete-preferences>navigator/headingwidths"
#define GC_NAVGROUPBY                   "<athlete-preferences>navigator/groupby"
#define GC_SORTBY                       "<athlete-preferences>navigator/sortby"
#define GC_SORTBYORDER                  "<athlete-preferences>navigator/sortbyorder"
// Calendar sync
#define GC_WEBCAL_URL                   "<athlete-preferences>webcal_url"
// Default view on Diary
#define GC_DIARY_VIEW                   "<athlete-preferences>diaryview"

// OSM Tileserver
#define GC_OSM_TS_DEFAULT               "<athlete-preferences>osmts/default"
#define GC_OSM_TS_A                     "<athlete-preferences>osmts/a"
#define GC_OSM_TS_B                     "<athlete-preferences>osmts/b"
#define GC_OSM_TS_C                     "<athlete-preferences>osmts/c"

// BodyMeasures Download
#define GC_BM_LAST_TYPE                 "<athlete-preferences>bm/last_type"
#define GC_BM_LAST_TIMEFRAME            "<athlete-preferences>bm/last_timeframe"


#define GC_RWGPSUSER                    "<athlete-private>rwgps/user"
#define GC_RWGPSPASS                    "<athlete-private>rwgps/pass"
#define GC_TTBUSER                      "<athlete-private>ttb/user"
#define GC_TTBPASS                      "<athlete-private>ttb/pass"
#define GC_VELOHEROUSER                 "<athlete-private>velohero/user"
#define GC_VELOHEROPASS                 "<athlete-private>velohero/pass"
#define GC_SPORTPLUSHEALTHUSER          "<athlete-private>sph/user"
#define GC_SPORTPLUSHEALTHPASS          "<athlete-private>sph/pass"
#define GC_SELUSER                      "<athlete-private>sel/user"
#define GC_SELPASS                      "<athlete-private>sel/pass"
#define GC_WIURL                        "<athlete-private>wi/url"
#define GC_WIUSER                       "<athlete-private>wi/user"
#define GC_WIKEY                        "<athlete-private>wi/key"
#define GC_DVURL                        "<athlete-private>dv/url"
#define GC_DVUSER                       "<athlete-private>dv/user"
#define GC_DVPASS                       "<athlete-private>dv/pass"
#define GC_DVCALDAVTYPE                 "<athlete-private>dv/type"
#define GC_DVGOOGLE_CALID               "<athlete-private>dv/googlecalid"
#define GC_DVGOOGLE_DRIVE               "<athlete-private>dv/googledrive"
//Dropbox oauth keys
#define GC_DROPBOX_TOKEN                "<athlete-private>dropbox/token"
#define GC_DROPBOX_FOLDER               "<athlete-private>dropbox/folder"

//Google oauth keys
#define GC_GOOGLE_DRIVE_AUTH_SCOPE      "<athlete-private>google-drive/auth_scope"
#define GC_GOOGLE_DRIVE_ACCESS_TOKEN   "<athlete-private>google-drive/access_token"
#define GC_GOOGLE_DRIVE_REFRESH_TOKEN   "<athlete-private>google-drive/refresh_token"
#define GC_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH "<athlete-private>google-drive/last_access_token_refresh"

#define GC_GOOGLE_DRIVE_FOLDER          "<athlete-private>google-drive/folder"
#define GC_GOOGLE_DRIVE_FOLDER_ID       "<athlete-private>google-drive/folder_id"

//University of Kent (same as google drive)
#define GC_UOK_CONSENT                      "<athlete-private>uok-google-drive/consent"
#define GC_UOK_GOOGLE_DRIVE_AUTH_SCOPE      "<athlete-private>uok-google-drive/auth_scope"
#define GC_UOK_GOOGLE_DRIVE_ACCESS_TOKEN   "<athlete-private>uok-google-drive/access_token"
#define GC_UOK_GOOGLE_DRIVE_REFRESH_TOKEN   "<athlete-private>uok-google-drive/refresh_token"
#define GC_UOK_GOOGLE_DRIVE_LAST_ACCESS_TOKEN_REFRESH "<athlete-private>uok-google-drive/last_access_token_refresh"

#define GC_UOK_GOOGLE_DRIVE_FOLDER          "<athlete-private>uok-google-drive/folder"
#define GC_UOK_GOOGLE_DRIVE_FOLDER_ID       "<athlete-private>uok-google-drive/folder_id"

//Withings
#define GC_WITHINGS_TOKEN               "<athlete-private>withings_token"
#define GC_WITHINGS_SECRET              "<athlete-private>withings_secret"
//Nokia
#define GC_NOKIA_TOKEN                  "<athlete-private>nokia_token"
#define GC_NOKIA_REFRESH_TOKEN          "<athlete-private>nokia_refresh_token"
//Google Calendar-CALDAV oauthkeys
#define GC_GOOGLE_CALENDAR_REFRESH_TOKEN  "<athlete-private>google_cal_refresh_token"
//Strava
#define GC_STRAVA_TOKEN                 "<athlete-private>strava_token"
#define GC_STRAVA_ACTIVITY_NAME         "<athlete-private>strava_metaname"
//Cycling Analytics
#define GC_CYCLINGANALYTICS_TOKEN       "<athlete-private>cyclinganalytics_token"
//Today's Plan
#define GC_TODAYSPLAN_TOKEN             "<athlete-private>todaysplan_token"
#define GC_TODAYSPLAN_URL               "<athlete-private>todaysplan_url"
#define GC_TODAYSPLAN_USERKEY           "<athlete-private>todaysplan_userkey"
#define GC_TODAYSPLAN_ATHLETE_ID        "<athlete-private>todaysplan_athlete_id"
#define GC_TODAYSPLAN_ATHLETE_NAME      "<athlete-private>todaysplan_athlete_name"

//SixCycle
#define GC_SIXCYCLE_USER                "<athlete-private>sixcycle_user"
#define GC_SIXCYCLE_PASS                "<athlete-private>sixcycle_pass"
#define GC_SIXCYCLE_URL                 "<athlete-private>sixcycle_url"
// Polar Flow
#define GC_POLARFLOW_TOKEN             "<athlete-private>polarflow_token"
#define GC_POLARFLOW_USER_ID           "<athlete-private>polarflow_user_id"
// SportTracks
#define GC_SPORTTRACKS_TOKEN           "<athlete-private>sporttracks/sporttracks_token"
#define GC_SPORTTRACKS_REFRESH_TOKEN   "<athlete-private>sporttracks/refresh_token"
#define GC_SPORTTRACKS_LAST_REFRESH    "<athlete-private>sporttracks/last_refresh"
// Xert
#define GC_XERTUSER                    "<athlete-private>xert/user"
#define GC_XERTPASS                    "<athlete-private>xert/pass"
#define GC_XERT_TOKEN                  "<athlete-private>xert/xert_token"
#define GC_XERT_REFRESH_TOKEN          "<athlete-private>xert/refresh_token"
#define GC_XERT_LAST_REFRESH           "<athlete-private>xert/last_refresh"
// --------------------------------------------------------------------------------
#include <QSettings>
#include <QFileInfo>

// Helper Class for the Athlete QSettings

class AthleteQSettings {
    public:

      AthleteQSettings() {}
      ~AthleteQSettings() {}

      void setQSettings(QSettings* settings, int index) {athlete[index] = settings; }
      QSettings* getQSettings(int index) const { return athlete[index]; }

    private:
      QSettings* athlete[4];  // we support exactly 4 Athlete specific QSettings right now

};


// wrap the standard QSettings so we can offer members
// to get global or atheleteName specific settings
// via value() and cvalue()

class GSettings
{

public:
    // 2 Variants are supported - still the "old" one where ALL properties are in ONE .INI file and the
    // new one were the properties are distributed as explained above (this is to keep compatibility)
    GSettings(QString org, QString app);
    GSettings(QString file, QSettings::Format format);
    ~GSettings();

    // standard access to global config
    QVariant value(const QObject *me, const QString key, const QVariant def = 0) ;
    void setValue(QString key, QVariant value);

    // access to athleteName specific config
    QVariant cvalue(QString athleteName, QString key, QVariant def = 0);

    void setCValue(QString athleteName, QString key, QVariant value);

    // add QSettings methods - which cannot be inherited since not a single, but multiple QSettings make GSettings
    QStringList allKeys() const;
    bool contains(const QString & key) const;

    // initialization of global and athlete QSettings can only happen after they path and the athlete is known
    void migrateQSettingsSystem();
    void initializeQSettingsGlobal(QString athletesRootDir);
    void initializeQSettingsAthlete(QString athletesRootDir, QString athleteName);
    void initializeQSettingsNewAthlete(QString athletesRootDir, QString athleteName);

    // QSettings need to be synched for all QSetting objects
    void syncQSettings();
    void syncQSettingsGlobal();
    void syncQSettingsAllAthletes();

    // Cleanup if AthleteDir is changed
    void clearGlobalAndAthletes();

private:
    bool newFormat;
    QSettings *systemsettings;
    QSettings *oldsystemsettings;
    QVector<QSettings*> *global;
    QHash<QString, AthleteQSettings*> athlete;

    // special methods for Migration/Upgrade
    void migrateValue(QString key);
    void migrateCValue(QString athleteName, QString key);
    void migrateAndRenameCValue(QString athleteName, QString wrongKey, QString key);
    void migrateValueToCValue(QString athleteName, QString key);
    void migrateCValueToValue(QString athleteName, QString key);

    void upgradeSystem();
    void upgradeGlobal();
    void upgradeAthlete(QString athleteName);

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
