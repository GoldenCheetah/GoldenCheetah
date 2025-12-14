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
#include <QRect>


/*
 *  Global GC Properties which are not stored as GSettings are defined here
 */

#include "SettingsConstants.h"

// --------------------------------------------------------------------------------
#include <QSettings>
#include <QFileInfo>

// Helper Class for the Athlete QSettings

extern double scalefactors[];

class AthleteQSettings {
    public:

      AthleteQSettings() {}
      ~AthleteQSettings() {}

      void setQSettings(QSettings* settings, int index) {athlete[index] = settings; }
      QSettings* getQSettings(int index) const { return athlete[index]; }

    private:
      QSettings* athlete[4];  // we support exactly 4 Athlete specific QSettings right now

};


typedef struct {

    // gui sizing
    double xfactor, yfactor; // scaling of widgets
    QRect windowsize; // size of the window in pixels

    // fonts
    QString fontfamily;
    int fontpointsize;
    double fontscale; // scaling of fonts
    int fontscaleindex; // index into the list

    // ui elements, mostly standard defaults
    int theme; // theme number to use
    bool antialias; // antialias fonts- always true
    bool macForms; // simulate MAC-style forms
    bool scrollbar; //scroller on ride list- always false
    bool head; // false for MAC, true for everyone else
    int sidebarwidth; // width of sidebars in pixels
    bool sideanalysis;
    bool sidetrend;
    bool sideplan;
    bool sidetrain;
    double linewidth; // default line width

} AppearanceSettings;

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
    QVariant value(const QObject *me, const QString key, const QVariant def = 0);
    void setValue(QString key, QVariant value);
    void remove(const QString &key);

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

    // reset appearance settings like theme, font, color and geometry
    static AppearanceSettings defaultAppearanceSettings();

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

#define OS_STYLE "Fusion"
#endif // _GC_Settings_h
