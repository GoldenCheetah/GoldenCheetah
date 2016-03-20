/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#ifndef Gc_GcUpgrade_h
#define Gc_GcUpgrade_h
#include "GoldenCheetah.h"
#include "RideMetadata.h"
#include "Athlete.h"
#include <QString>

#ifdef NOWEBKIT
#include <QWebEngineView>
#include <QWebEngineSettings>
#else
#include <QWebView>
#include <QWebFrame>
#include <QWebSettings>
#endif


// Build ID History
//
// 3001 - V3.0 RC1
// 3002 - V3.0 RC2
// 3003 - V3.0 RC3
// 3004 - V3.0 RC4 / 4X
// 3005 - V3.0 RC5 / 5X
// 3006 - V3.0 RC6
// 3007 - V3.0 RC7
// 3010 - V3.0 RELEASE (June 7 2013)
// 3020 - V3.0 SP1 RC1
// 3030 - V3.0 SP1 RELEASE (December 2013)
// 3032 - V3.0 SP2 RELEASE (March 2014)
// 3040 - V3.1 DEVELOPMENT
// 3050 - V3.1 DEVELOPMENT (accidentally pushed)
// 3051 - V3.1 DEVELOPMENT
// 3055 - V3.1 RC1
// 3056 - V3.1 RC2
// 3057 - V3.1 RC3
// 3058 - V3.1 RC3X
// 3059 - V3.1 RC4
// 3100 - V3.1 RELEASE (August 17 2014)
// 3101 - V3.11 DEVELOPMENT
// 3102 - V3.11 DEVELOPMENT INCL RIDECACHE
// 3103 - V3.2 RC1 (renamed from 3.11)
// 3104 - V3.2 RC1X (after late commits)
// 3105 - V3.2 RC2
// 3106 - V3.2 RC3
// 3107 - V3.2 RC4 (lost due to push fail by Mark)
// 3108 - V3.2 RC4X (some problems with tagging)
// 3200 - V3.2 RELEASE (August 12 2015)
// 3900 - V3.3 DEVELOPMENT STARTS
// 3901 - V3.3 PUBLIC DEVELOPMENT BUILD
// 3902 - V3.3 RC1
// 3903 - V3.3 RC2
// 3904 - V3.3 RC3
// 3933 - V3.3 RELEASE (November 29 2015)
// 3940 - V4.0 DEVELOPMENT STARTS

#define VERSION3_BUILD    3010 // released
#define VERSION3_SP1      3030 // released
#define VERSION3_SP2      3032 // released
#define VERSION31_UPG     3100 // first build with 3.1 upgrade process
#define VERSION311_BUILD  3101 // first build with 3.1 upgrade process
#define VERSION32_BUILD   3200 // released
#define VERSION33_BUILD   3933 // development release
#define VERSION40_BUILD   3940 // development release

// will keep changing during testing and before final release
#define VERSION31_BUILD VERSION31_UPG

// these three will change until we release
#define VERSION_LATEST 3940
#define VERSION_STRING "V4.0 DEV"

// default config for this release is available and maintained here:
#define VERSION_CONFIG_PREFIX "http://www.goldencheetah.org/defaults/4.0"

class GcUpgradeLogDialog : public QDialog
{
    Q_OBJECT

    public:
        GcUpgradeLogDialog(QDir);
        void enableButtons();
        void append(QString, int level=0);


    public slots:
        void saveAs();
        void linkClickedSlot( QUrl );

    private:

#ifdef NOWEBKIT
        QWebEngineView *report;
#else
        QWebView *report;
#endif
        AthleteDirectoryStructure home;
        QString reportText;
        QPushButton *proceedButton;
        QPushButton *saveAsButton;

};



class GcUpgrade
{
    Q_DECLARE_TR_FUNCTIONS(GcUpgrade)

	public:
        GcUpgrade() { upgradeError = false; errorCount = 0;}
        bool upgradeConfirmedByUser(const QDir &home); // upgrade warning in case of critical updates which need a backup
        int upgrade(const QDir &home);                 // standard upgrade steps
        int upgradeLate(Context *context);             // final upgrade steps which need a full context to be available first (exceptional use only !)
        static int version() { return VERSION_LATEST; }
        static QString versionString() { return VERSION_STRING; }
        void removeIndex(QFile&);

    private:

        bool moveFile(const QString &source, const QString &target);
        GcUpgradeLogDialog *upgradeLog;
        bool upgradeError;
        int errorCount;

};


class GcUpgradeExecuteDialog : public QDialog
{
    Q_OBJECT

    public:
        GcUpgradeExecuteDialog(QDir);

    private:
        QScrollArea *scrollText;
        QPushButton *proceedButton;
        QPushButton *abortButton;

};



#endif
