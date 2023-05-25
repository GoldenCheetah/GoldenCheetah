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

#include <QDir>
#include "Settings.h"
#include "MainWindow.h"
#include "Colors.h"
#include <QSettings>
#include <QDesktopWidget>
#include <QDebug>

#ifdef Q_OS_MAC
int OperatingSystem = OSX;
#elif defined Q_OS_WIN32
int OperatingSystem = WINDOWS;
#elif defined Q_OS_LINUX
int OperatingSystem = LINUX;
#elif defined Q_OS_OPENBSD
int OperatingSystem = OPENBSD;
#endif

double scalefactors[13] = { 0.5f, 0.6f, 0.8, 0.9, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f, 2.5f, 3.0f, 5.0f, 0 };

// -------------- Initializer for the "extern" variable "appsettings" ----------------//

static GSettings *GetApplicationSettings()
{
  GSettings *settings;
  QDir home = QDir();
    //First check to see if the Library folder exists where the executable is (for USB sticks)
  if(!home.exists("Library/GoldenCheetah"))
    settings = new GSettings(GC_SETTINGS_CO, GC_SETTINGS_APP);
  else
    settings = new GSettings(home.absolutePath()+"/gc", QSettings::IniFormat);
  return settings;
}

// local static helper routines

// define the sections and the filenames
enum SettingsType {SETTINGS_SYSTEM = 0,
                   SETTINGS_GLOBAL = 1,
                   SETTINGS_ATHLETE = 2 };

enum SettingsFilesIndexGlobal {GLOBAL_GENERAL = 0,
                               GLOBAL_TRAINMODE = 1};

enum SettingsFilesIndexAthlete { ATHLETE_GENERAL = 0,
                                 ATHLETE_LAYOUT = 1,
                                 ATHLETE_PREFERENCES = 2,
                                 ATHLETE_PRIVATE = 3};

static const QString settingFileNamesGlobal[2] = {"configglobal-general.ini","configglobal-trainmode.ini"};
static const QString settingFileNamesAthlete[4] = {"athlete-general.ini","athlete-layout.ini","athlete-preferences.ini","athlete-private.ini"};



static QString DetermineKey(QString & key, int& store, int& fileIndex) {

    store = SETTINGS_SYSTEM; // default to systemsettings
    fileIndex = 0;
    if (key.startsWith(GC_QSETTINGS_GLOBAL_GENERAL)) {
        store = SETTINGS_GLOBAL;
        fileIndex = GLOBAL_GENERAL;
    } else if (key.startsWith(GC_QSETTINGS_GLOBAL_TRAIN)) {
        store = SETTINGS_GLOBAL;
        fileIndex = GLOBAL_TRAINMODE;
    } else if (key.startsWith(GC_QSETTINGS_ATHLETE_GENERAL)) {
        store = SETTINGS_ATHLETE;
        fileIndex = ATHLETE_GENERAL;
    } else if (key.startsWith(GC_QSETTINGS_ATHLETE_LAYOUT)) {
        store = SETTINGS_ATHLETE;
        fileIndex = ATHLETE_LAYOUT;
    } else if (key.startsWith(GC_QSETTINGS_ATHLETE_PREFERENCES)) {
        store = SETTINGS_ATHLETE;
        fileIndex = ATHLETE_PREFERENCES;
    } else if (key.startsWith(GC_QSETTINGS_ATHLETE_PRIVATE)) {
        store = SETTINGS_ATHLETE;
        fileIndex = ATHLETE_PRIVATE;
    }

    // and make sure <> text is removed
    return key.remove(QRegExp("^<.*>"));

}


// -----------------------------constructor and public instance methods ------------------------//

GSettings::GSettings(QString org, QString app) : newFormat(true){
    oldsystemsettings = new QSettings(org,app);
    systemsettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, org, app);
    global = new QVector<QSettings*>();
}

GSettings::GSettings(QString file, QSettings::Format format) : newFormat(false){
    systemsettings = new QSettings(file,format);
}

GSettings::~GSettings() {
    syncQSettings();
}


QVariant
GSettings::value(const QObject * /*me*/, const QString key, const QVariant def) {

    QString keyVar = QString(key);
    if (newFormat) {
        int store;
        int file;
        keyVar = DetermineKey(keyVar, store, file);
        switch (store) {
        case SETTINGS_SYSTEM:
            return systemsettings->value(keyVar, def);
            break;
        case SETTINGS_GLOBAL:
            return global->at(file)->value(keyVar, def);
            break;
        case SETTINGS_ATHLETE:
            qDebug() << "GetValue key, keyVar, store:" << key << ":" << keyVar  << ": " << store; // error cases on code configuration
            break;
        }

    } else {
        keyVar.remove(QRegExp("^<.*>"));
        return systemsettings->value(keyVar, def);
    }
    return QVariant();
}

void
GSettings::setValue(QString key, QVariant value)
{
    QString keyVar = QString(key);
    if (newFormat) {
        int store;
        int file;
        keyVar = DetermineKey(keyVar, store, file);
        switch (store) {
        case SETTINGS_SYSTEM:
            systemsettings->setValue(keyVar,value);
            break;
        case SETTINGS_GLOBAL:
            global->at(file)->setValue(keyVar,value);
            break;
        case SETTINGS_ATHLETE:
            qDebug() << "SetValue key, keyVar, store:" << key << ":" << keyVar  << ": " << store; // error cases on code configuration
            break;

        }
    } else {
        keyVar.remove(QRegExp("^<.*>"));
        systemsettings->setValue(keyVar, value);
    }

}

// access to athlete specific config
QVariant
GSettings::cvalue(QString athleteName, QString key, QVariant def) {

    if (athleteName.isNull() || athleteName.isEmpty()) return def;

    QString keyVar = QString(key);
    if (newFormat) {
        int store;
        int file;
        keyVar = DetermineKey(keyVar, store, file);

        QHash<QString, AthleteQSettings*>::const_iterator i = athlete.find(athleteName);
        if (i != athlete.end()) {
            switch (store) {
            case SETTINGS_SYSTEM:
            case SETTINGS_GLOBAL:
                qDebug() << "GetCValue key, keyVar, store:" << key << ":" << keyVar  << ": " << store; // error cases on code configuration
                break;
            case SETTINGS_ATHLETE:
                return i.value()->getQSettings(file)->value(keyVar, def);
                break;
            }
        } else {
            // fall back to old settings - assuming that this can only happen during the upgrade of an athlete
            // and before the new /config folder exists
            return oldsystemsettings->value(athleteName+"/"+keyVar, def);
        }

    } else {
        keyVar.remove(QRegExp("^<.*>"));
        return systemsettings->value(athleteName+"/"+keyVar, def);
    }
    return QVariant();

}

void
GSettings::setCValue(QString athleteName, QString key, QVariant value) {

    QString keyVar = QString(key);
    if (newFormat) {
        int store;
        int file;
        keyVar = DetermineKey(keyVar, store, file);
        QHash<QString, AthleteQSettings*>::const_iterator i = athlete.find(athleteName);
        if (i != athlete.end()) {
            switch (store) {
            case SETTINGS_SYSTEM:
            case SETTINGS_GLOBAL:
                qDebug() << "SetCValue keyVar, store:" << key << ":" << keyVar  << ": " << store; // error cases on code configuration
                break;
            case SETTINGS_ATHLETE:
                i.value()->getQSettings(file)->setValue(keyVar, value);
                break;
            }
        } // if we do have have the athlete - then we do not store anything
    } else {
        keyVar.remove(QRegExp("^<.*>"));
        systemsettings->setValue(athleteName + "/" + keyVar,value);

    }
}

// other functions unsed from QSettings which GSettings needs to implement
QStringList
GSettings::allKeys() const {

    if (newFormat) {
        QStringList allKeys, tempKeys;
        tempKeys = systemsettings->allKeys();
        foreach (QString key, tempKeys) {
           allKeys.append(GC_QSETTINGS_SYSTEM+key);
        }
        tempKeys = global->at(GLOBAL_GENERAL)->allKeys();
        foreach (QString key, tempKeys) {
           allKeys.append(GC_QSETTINGS_GLOBAL_GENERAL+key);
        }
        tempKeys = global->at(GLOBAL_TRAINMODE)->allKeys();
        foreach (QString key, tempKeys) {
           allKeys.append(GC_QSETTINGS_GLOBAL_TRAIN+key);
        }
        QHashIterator<QString, AthleteQSettings*> i(athlete);
        i.toFront();
        while (i.hasNext())
        { i.next();
            tempKeys = i.value()->getQSettings(ATHLETE_GENERAL)->allKeys();
            foreach (QString key, tempKeys) {
               allKeys.append(GC_QSETTINGS_ATHLETE_GENERAL+key);
            }
            tempKeys = i.value()->getQSettings(ATHLETE_LAYOUT)->allKeys();
            foreach (QString key, tempKeys) {
               allKeys.append(GC_QSETTINGS_ATHLETE_LAYOUT+key);
            }
            tempKeys = i.value()->getQSettings(ATHLETE_PREFERENCES)->allKeys();
            foreach (QString key, tempKeys) {
               allKeys.append(GC_QSETTINGS_ATHLETE_PREFERENCES+key);
            }
            tempKeys = i.value()->getQSettings(ATHLETE_PRIVATE)->allKeys();
            foreach (QString key, tempKeys) {
               allKeys.append(GC_QSETTINGS_ATHLETE_PRIVATE+key);
            }
        }
        allKeys.removeDuplicates();  // remove duplicate keys from the Athlete Settings
        return allKeys;
    } else {
        return systemsettings->allKeys();
    }
    return QStringList();
}

bool
GSettings::contains(const QString & key) const {

    QString keyVar = QString(key);
    if (newFormat) {
        int store;
        int file;
        keyVar = DetermineKey(keyVar, store, file);
        switch (store) {
        case SETTINGS_SYSTEM:
            return systemsettings->contains(keyVar);
            break;
        case SETTINGS_GLOBAL:
            return global->at(file)->contains(keyVar);
            break;
        case SETTINGS_ATHLETE:
            qDebug() << "Contains Value key:" << key << "keyVar:" << keyVar; // error cases on code configuration
            return false;
            break;
        }
    } else {
        keyVar.remove(QRegExp("^<.*>"));
        return systemsettings->contains(keyVar);
    }
    return false;
}

void
GSettings::migrateQSettingsSystem() {

    if (!newFormat) return;

    // do the migration for the System Settings - if not yet done
    // - System is only migrated once per PC (since it only exists once

    bool migrateMac = false;
    QStringList currentKeys = systemsettings->allKeys();
#ifdef Q_OS_MAC
    migrateMac = true;
#endif
    if (currentKeys.size() == 0 || (migrateMac && currentKeys.size() == 1)) {
        upgradeSystem();
        systemsettings->sync();
    }
}


void
GSettings::initializeQSettingsGlobal(QString athletesRootDir) {

    if (!newFormat) return;

    if (global->isEmpty()) {

        global->append(new QSettings(athletesRootDir+"/"+settingFileNamesGlobal[GLOBAL_GENERAL],QSettings::IniFormat));
        global->append(new QSettings(athletesRootDir+"/"+settingFileNamesGlobal[GLOBAL_TRAINMODE],QSettings::IniFormat));

    }

    // do the migration for the AthleteDir / Global Settings  if not yet done
    // - Global is migrated to the root of ANY AthleteDirectory if it does not yet exist
    //   this is too support migration if a user has multiple AthleteDirectories in place
    //   but also creates a default like his previous old-style directory when new AthleteDirectory is created

    if (global->at(GLOBAL_GENERAL)->allKeys().isEmpty() && global->at(GLOBAL_TRAINMODE)->allKeys().isEmpty()) {
        upgradeGlobal();
    }
    syncQSettingsGlobal();

}

void
GSettings::initializeQSettingsAthlete(QString athletesRootDir, QString athleteName) {

    // assumption is that the directory "<athletesRootDir>/athleteName" exists //

    if (!newFormat) return;

    // handle not yet upgraded athlete folders without causing problems
    // initializing of the QSettings would work anyway - but upgrade would fail,
    // since the /config folder does not exist - so leave the upgrade for the next
    // initialization after successfull upgrade (the case should be rare anyway)
    QDir configDir(athletesRootDir+"/"+athleteName+"/config");
    if (!configDir.exists()) {
        return; // athlete has not yet been migrated and /config does not exist - so wait until next time
    }

    // create the New Athlete QSettings (if they do not yet exists and migrate the old data if required)

    QHash<QString, AthleteQSettings*>::const_iterator i = athlete.find(athleteName);
    if (i == athlete.end()) {

        initializeQSettingsNewAthlete(athletesRootDir, athleteName);

        QHash<QString, AthleteQSettings*>::const_iterator i2 = athlete.find(athleteName);
        // do the upgrade for the Athlete Properties - but only if the Settings are currently empty - don't overwrite anything
        if (i2 != athlete.end()) {
            if (i2.value()->getQSettings(ATHLETE_GENERAL)->allKeys().isEmpty() &&
                    i2.value()->getQSettings(ATHLETE_LAYOUT)->allKeys().isEmpty() &&
                    i2.value()->getQSettings(ATHLETE_PREFERENCES)->allKeys().isEmpty() &&
                    i2.value()->getQSettings(ATHLETE_PRIVATE)->allKeys().isEmpty() ) {
                upgradeAthlete(athleteName);

            }
        }
        syncQSettingsAllAthletes();
    }
}

void
GSettings::initializeQSettingsNewAthlete(QString athletesRootDir, QString athleteName) {

    if (!newFormat) return;

    // create the Athlete QSettings - they MUST not exist yet
    AthleteQSettings* athleteSettings = new AthleteQSettings();
    QString baseName = athletesRootDir + "/" + athleteName + "/config/";
    athleteSettings->setQSettings(new QSettings(baseName+settingFileNamesAthlete[ATHLETE_GENERAL], QSettings::IniFormat), ATHLETE_GENERAL );
    athleteSettings->setQSettings(new QSettings(baseName+settingFileNamesAthlete[ATHLETE_LAYOUT], QSettings::IniFormat), ATHLETE_LAYOUT );
    athleteSettings->setQSettings(new QSettings(baseName+settingFileNamesAthlete[ATHLETE_PREFERENCES], QSettings::IniFormat), ATHLETE_PREFERENCES );
    athleteSettings->setQSettings(new QSettings(baseName+settingFileNamesAthlete[ATHLETE_PRIVATE], QSettings::IniFormat), ATHLETE_PRIVATE );
    athlete.insert(athleteName, athleteSettings);

}


void
GSettings::syncQSettingsAllAthletes() {

    if (!newFormat) return;

    QHashIterator<QString, AthleteQSettings*> i(athlete);
    i.toFront();
    while (i.hasNext())
    { i.next();
        i.value()->getQSettings(ATHLETE_GENERAL)->sync();
        i.value()->getQSettings(ATHLETE_LAYOUT)->sync();
        i.value()->getQSettings(ATHLETE_PREFERENCES)->sync();
        i.value()->getQSettings(ATHLETE_PRIVATE)->sync();
    }
}

void
GSettings::syncQSettingsGlobal() {

    if (!newFormat) return;

    if (global->size() == 2) {
        global->at(GLOBAL_GENERAL)->sync();
        global->at(GLOBAL_TRAINMODE)->sync();
    };
}

void
GSettings::syncQSettings() {

    systemsettings->sync();
    syncQSettingsGlobal();
    syncQSettingsAllAthletes();

}

void
GSettings::clearGlobalAndAthletes() {

    if (!newFormat) return;
    syncQSettings();
    global->clear();
    athlete.clear();
}


/*-------------------------------- special methods for Upgrade/Migration --------------------------
 *
 * The .INI based storage of Settings has been introduced with GoldenCheetah v3.3.0
 *
 * To transition existing settings (in PLISTs (OSX) and Registry (WINDOWS) from the
 * propriety storage to the common .INI files an automatic migration of Settings takes
 * place when no Settings are found. The methods executing the migration are implemented here
 *
 * Any development starting starting after v3.3 (so v4.0 and onwards) does not need
 * to take the migration into account, since any newly defined settings are only stored
 * using the new .INI based technique.
 *
 -----------------------------------------------------------------------------------------------*/

void
GSettings::migrateValue(QString key) {

    QString oldKey = key;
    oldKey.remove(QRegExp("^<.*>"));
    if (oldsystemsettings->contains(oldKey)) {
        setValue(key, oldsystemsettings->value(oldKey));
    }
}

void
GSettings::migrateCValue(QString athlete, QString key) {

    QString oldKey = key;
    oldKey.remove(QRegExp("^<.*>"));
    if (oldsystemsettings->contains(athlete+"/"+oldKey)) {
        setCValue(athlete, key, oldsystemsettings->value(athlete+"/"+oldKey));
    }
}

void
GSettings::migrateAndRenameCValue(QString athlete, QString wrongKey, QString key) {

    wrongKey.remove(QRegExp("^<.*>"));
    if (oldsystemsettings->contains(athlete+"/"+wrongKey)) {
        setCValue(athlete, key, oldsystemsettings->value(athlete+"/"+wrongKey));
    }
}

void
GSettings::migrateValueToCValue(QString athlete, QString key) {

    QString oldKey = key;
    oldKey.remove(QRegExp("^<.*>"));
    if (oldsystemsettings->contains(oldKey)) {
        setCValue(athlete, key, oldsystemsettings->value(oldKey));
    }
}

void
GSettings::migrateCValueToValue(QString athlete, QString key) {

    // only migrate if the value does not yet exist on the target INI file
    if (!contains(key)) {
        QString oldKey = key;
        oldKey.remove(QRegExp("^<.*>"));
        oldKey = athlete+"/"+oldKey;
        if (oldsystemsettings->contains(oldKey)) {
            setValue(key, oldsystemsettings->value(oldKey));
        }
    }
}


void
GSettings::upgradeSystem() {

    // by explicitely naming all the properties, and not choosing the "allKeys()" function,
    // only the properties still in use are migrated - and not any orphans for previous releases

    // NOTE: Migrating values is only required for settings introduced in GC version until v3.3

    migrateValue(GC_HOMEDIR);
    migrateValue(GC_SETTINGS_LAST);
    migrateValue(GC_SETTINGS_MAIN_GEOM);
    migrateValue(GC_SETTINGS_MAIN_STATE);
    migrateValue(GC_SETTINGS_LAST_IMPORT_PATH);
    migrateValue(GC_SETTINGS_LAST_WORKOUT_PATH);
    migrateValue(GC_LAST_DOWNLOAD_DEVICE);
    migrateValue(GC_LAST_DOWNLOAD_PORT);
    migrateValue(GC_BE_LASTDIR);
    migrateValue(GC_BE_LASTFMT);
    migrateValue(GC_FONT_DEFAULT);
    migrateValue(GC_FONT_DEFAULT_SIZE);
    migrateValue(GC_FONT_CHARTLABELS);
    migrateValue(GC_FONT_CHARTLABELS_SIZE);
    //DEPRECATED IN V3.5 migrateValue(GC_FONT_TITLES);
    //DEPRECATED IN V3.5 migrateValue(GC_FONT_CHARTMARKERS);
    //DEPRECATED IN V3.5 migrateValue(GC_FONT_CALENDAR);
    //DEPRECATED IN V3.5 migrateValue(GC_FONT_TITLES_SIZE);
    //DEPRECATED IN V3.5 migrateValue(GC_FONT_CHARTMARKERS_SIZE);
    //DEPRECATED IN V3.5 migrateValue(GC_FONT_CALENDAR_SIZE);

    QStringList colorProperties = GCColor::getConfigKeys();
    QStringListIterator colorIterator(colorProperties);
    while (colorIterator.hasNext()) {
        QString key = QString(colorIterator.next().data());
        migrateValue(key);
    }
}

void
GSettings::upgradeGlobal() {

    // by explicitely naming all the properties, and not choosing the "allKeys()" function,
    // only the properties still in use are migrated - and not any orphans for previous releases

    // NOTE: Migrating values is only required for settings introduced in GC version until v3.3
    migrateValue(GC_SETTINGS_FAVOURITE_METRICS);
    migrateValue(GC_TABBAR);
    migrateValue(GC_WBALFORM);
    migrateValue(GC_BIKESCOREDAYS);
    migrateValue(GC_BIKESCOREMODE);
    migrateValue(GC_WARNCONVERT);
    migrateValue(GC_WARNEXIT);
    migrateValue(GC_HIST_BIN_WIDTH);
    migrateValue(GC_WORKOUTDIR);
    migrateValue(GC_LINEWIDTH);
    migrateValue(GC_ANTIALIAS);
    migrateValue(GC_RIDEBG);
    migrateValue(GC_RIDESCROLL);
    migrateValue(GC_RIDEHEAD);
    migrateValue(GC_SHADEZONES);
    migrateValue(GC_GARMIN_SMARTRECORD);
    migrateValue(GC_GARMIN_HWMARK);
    migrateValue(GC_DPFG_TOLERANCE);
    migrateValue(GC_DPFG_STOP);
    migrateValue(GC_DPFS_MAX);
    migrateValue(GC_DPFS_VARIANCE);
    migrateValue(GC_DPTA);
    migrateValue(GC_DPPA);
    migrateValue(GC_DPFHRS_MAX);
    migrateValue(GC_DPDP_BIKEWEIGHT);
    migrateValue(GC_DPDP_CRR);
    migrateValue(GC_LANG);
    migrateValue(GC_PACE);
    migrateValue(GC_SWIMPACE);
    migrateValue(GC_ELEVATION_HYSTERESIS);
    migrateValue(GC_START_HTTP);

    // Handle the Dataprocessor dp/%1/apply keys
    // Handle the RideEditor colmap/%1 keys
    QStringList dpKeys = oldsystemsettings->allKeys();
    QStringListIterator dpKeysIterator(dpKeys);
    while (dpKeysIterator.hasNext()) {
        QString key = QString(dpKeysIterator.next().data());
        if (key.startsWith("dp/") || key.startsWith("colmap/")) {
            migrateValue(GC_QSETTINGS_GLOBAL_GENERAL+key);
        }
    }

    // handle the Device configuration
    migrateValue(GC_DEV_COUNT);
    QString devCountKey = GC_DEV_COUNT;
    devCountKey.remove(QRegExp("^<.*>"));
    QVariant configVal = oldsystemsettings->value(devCountKey);
    int devicecount;
    if (configVal.isNull()) {
        devicecount=0;
    } else {
        devicecount = configVal.toInt();
    }
    for (int i = 0; i < devicecount; i++ ) {
        QString configStr = QString("%1%2").arg(GC_DEV_NAME).arg(i+1);
        migrateValue(configStr);
        configStr = QString("%1%2").arg(GC_DEV_TYPE).arg(i+1);
        migrateValue(configStr);
        configStr = QString("%1%2").arg(GC_DEV_WHEEL).arg(i+1);
        migrateValue(configStr);
        configStr = QString("%1%2").arg(GC_DEV_SPEC).arg(i+1);
        migrateValue(configStr);
        configStr = QString("%1%2").arg(GC_DEV_PROF).arg(i+1);
        migrateValue(configStr);
        configStr = QString("%1%2").arg(GC_DEV_VIRTUAL).arg(i+1);
        migrateValue(configStr);
    }

    migrateValue(FORTIUS_FIRMWARE);
    migrateValue(TRAIN_MULTI);

}


void
GSettings::upgradeAthlete(QString athlete) {

    // by explicitely naming all the properties, and not choosing the "allKeys()" function,
    // only the properties still in use are migrated - and not any orphans for previous releases

    // NOTE: Migrating values is only required for settings introduced in GC version until v3.3

    migrateCValue(athlete, GC_VERSION_USED);
    migrateCValue(athlete, GC_SAFEEXIT);
    migrateCValue(athlete, GC_UPGRADE_FOLDER_SUCCESS);
    migrateCValue(athlete, GC_LTM_LAST_DATE_RANGE);
    migrateCValue(athlete, GC_LTM_AUTOFILTERS);
    migrateCValue(athlete, GC_BLANK_ANALYSIS);
    migrateCValue(athlete, GC_BLANK_TRAIN);
    migrateCValue(athlete, GC_BLANK_HOME);
    migrateCValue(athlete, GC_BLANK_DIARY);
    migrateCValue(athlete, GC_NICKNAME);
    migrateCValue(athlete, GC_DOB);
    migrateCValue(athlete, GC_WEIGHT);
    migrateCValue(athlete, GC_HEIGHT);
    migrateCValue(athlete, GC_WBALTAU);
    migrateCValue(athlete, GC_SEX);
    migrateCValue(athlete, GC_BIO);
    migrateCValue(athlete, GC_AVATAR);
    migrateCValue(athlete, GC_DISCOVERY);
    migrateCValue(athlete, GC_SB_TODAY);
    migrateCValue(athlete, GC_LTS_DAYS);
    migrateCValue(athlete, GC_STS_DAYS);
    migrateCValue(athlete, GC_NAVHEADINGS);
    migrateCValue(athlete, GC_NAVGROUPBY);
    migrateCValue(athlete, GC_SORTBY);
    migrateCValue(athlete, GC_WEBCAL_URL);
    migrateCValue(athlete, GC_DIARY_VIEW);
    migrateCValue(athlete, GC_USE_CP_FOR_FTP);

    migrateAndRenameCValue(athlete, "bavigator/headingwidths", GC_NAVHEADINGWIDTHS);
    migrateCValueToValue(athlete, GC_UNIT);

    // Handle the splittersizes keys
    QStringList splitterKeys = oldsystemsettings->allKeys();
    QStringListIterator splitterKeysIterator(splitterKeys);
    while (splitterKeysIterator.hasNext()) {
        QString key = QString(splitterKeysIterator.next().data());
        if (key.startsWith(athlete + "/mainwindow/splitterSizes") || key.startsWith(athlete+"/splitter")) {
            key.remove(0, athlete.size()+1); // remove the Athlete name and / from the old setting !
            migrateCValue(athlete, GC_QSETTINGS_ATHLETE_LAYOUT+key);
        }
    }

    // --- private --- //
    migrateCValue(athlete, GC_RWGPSUSER);
    migrateCValue(athlete, GC_RWGPSPASS);
    migrateCValue(athlete, GC_VELOHEROUSER);
    migrateCValue(athlete, GC_VELOHEROPASS);
    migrateCValue(athlete, GC_WIURL);
    migrateCValue(athlete, GC_WIUSER);
    migrateCValue(athlete, GC_WIKEY);
    migrateCValue(athlete, GC_DVURL);
    migrateCValue(athlete, GC_DVUSER);
    migrateCValue(athlete, GC_DVPASS);
    migrateCValue(athlete, GC_DVCALDAVTYPE);
    migrateCValue(athlete, GC_STRAVA_TOKEN);
    migrateCValue(athlete, GC_CYCLINGANALYTICS_TOKEN);

    // migrate from system/global to athlete specific settings
    migrateValueToCValue(athlete, GC_CRANKLENGTH);
    migrateValueToCValue(athlete, GC_WHEELSIZE);

}

static char *fontfamilyfallback[] = {
#ifdef Q_OS_LINUX
    // try pretty fonts first (you never know)
    "Noto Sans Display",
    "DejaVu Sans",
    "Liberation Sans",

    // then distro specific ones
    "Ubuntu",
    "Red Hat Display",
    "Trebuchet MS",

#endif
#ifdef Q_OS_WIN
    "Segoe UI",
    "Calibri",
    "Microsoft Sans Serif",
    "Trebuchet MS",
#endif
#ifdef Q_OS_MAC
    "SF Pro Display",
    "Helvetica Neue",
    "Helvetica",
    "Trebuchet MS",
#endif

    // on all OS these two should exist at a minimum
    "Verdana",
    "Arial",
    NULL
};

AppearanceSettings
GSettings::defaultAppearanceSettings()
{
    AppearanceSettings returning;

    // lets get the geometry of the window next
    // since its used to scale and set other
    // appearance settings
    QRect screensize = QApplication::desktop()->availableGeometry();

    // leave 12% of the screen free to the left and right of the main window
    // and same number of pixels above and below
    returning.windowsize.setWidth(screensize.width() * 0.88);
    double margin = (screensize.width() - returning.windowsize.width()) / 2;
    returning.windowsize.setHeight(screensize.height() - (margin * 2));
    returning.windowsize.setX(margin);
    returning.windowsize.setY(margin);

    // sidebars should be about 20% of width and no more
    returning.sidebarwidth = returning.windowsize.width() / 5;

    // lets find an appropriate font
    returning.fontfamily = QFont().toString(); // ultimately fall back to QT default
    for(int i=0; fontfamilyfallback[i] != NULL; i++) {

        QFont font(fontfamilyfallback[i]);
        if (font.exactMatch()) {
            returning.fontfamily = fontfamilyfallback[i];
            break;
        }
    }
    returning.fontpointsize = 11; // default

    // scaling only applies on hidpi displays
    returning.fontscale = 1.0;
    returning.xfactor = 1.0;
    returning.yfactor = 1.0;

    if (desktop->screen()->devicePixelRatio() <= 1 && screensize.width() > 2160) {
       // we're on a hidpi screen - lets create a multiplier - always use smallest
       returning.xfactor = screensize.width() / 1280.0;
       returning.yfactor = screensize.height() / 1024.0;

        // always make the same, use smallest scaling when x and y differ
       if (returning.yfactor < returning.xfactor) returning.xfactor = returning.yfactor;
       else if (returning.xfactor < returning.yfactor) returning.yfactor = returning.xfactor;

       // set default font size -- all others will scale off this
       double height = screensize.height() / 70;

       // points = height in inches * dpi
       returning.fontpointsize = (height / QApplication::desktop()->logicalDpiY()) * 72;

    }

    // best settings for UI as now designed
    returning.theme = 5; // team purple colors
    returning.antialias = true;
    returning.scrollbar = true;
    returning.head = false;
    returning.sideanalysis = false;
    returning.sidetrend = false;
    returning.sidetrain = true;

    return returning;
}

//----------------------------------------------------------------------------------------------//

// initialise with no athlete
GSettings *appsettings = GetApplicationSettings();
