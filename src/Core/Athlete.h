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

#ifndef _GC_Athlete_h
#define _GC_Athlete_h 1

#include "Measures.h"

#include <QDir>
#include <QSqlDatabase>
#include <QTreeWidget>
#include <QtGui>
#include <QUuid>
#include <QNetworkReply>
#include <QHeaderView>


class Zones;
class HrZones;
class PaceZones;
class RideFile;
class ErgFile;
class RideMetadata;
class CalendarDownload;
class ICalendar;
class CalDAV;
class Seasons;
class RideNavigator;
class NamedSearches;
class RideFileCache;
class RideItem;
class IntervalItem;
class IntervalTreeView;
class PDEstimate;
class PMCData;
class LTMSettings;
class Routes;
class AthleteDirectoryStructure;
class RideImportWizard;
class RideAutoImportConfig;
class RideCache;
class IntervalCache;
class Context;
class ColorEngine;
class AnalysisSidebar;
class Tab;
class Leaf;
class DataFilterRuntime;
class CloudServiceAutoDownload;
class Banister;

class Athlete : public QObject
{
    Q_OBJECT

    public:
        Athlete(Context *context, const QDir &homeDir);
        ~Athlete();
        void close();

        // basic athlete info
        QString cyclist; // the cyclist name
        QUuid id; // unique identifier
        bool useMetricUnits;
        AthleteDirectoryStructure *home;
        const AthleteDirectoryStructure *directoryStructure() const {return home; }

        // metadata definitions
        RideMetadata *rideMetadata_;
        ColorEngine *colorEngine;

        // zones
        const Zones *zones(bool isRun) const { return zones_[isRun]; }
        const HrZones *hrZones(bool isRun) const { return hrzones_[isRun]; }
        const PaceZones *paceZones(bool isSwim) const { return pacezones_[isSwim]; }
        Zones *zones_[2];
        HrZones *hrzones_[2];
        PaceZones *pacezones_[2];
        void setCriticalPower(int cp);

        // Data
        Seasons *seasons;
        Routes *routes;
        QList<RideFileCache*> cpxCache;
        RideCache *rideCache;
        Measures *measures;

        // cloud download
        CloudServiceAutoDownload *cloudAutoDownload;

        // Estimates
        PDEstimate getPDEstimateFor(QDate, QString model, bool wpk);
        QList<PDEstimate> getPDEstimates();

        // PMC Data
        PMCData *getPMCFor(QString metricName, int stsDays = -1, int ltsDays = -1); // no Specification used!
        PMCData *getPMCFor(Leaf *expr, DataFilterRuntime *df, int stsDays = -1, int ltsDays = -1); // no Specification used!
        QMap<QString, PMCData*> pmcData; // all the different PMC series

        // Banister Data
        Banister *getBanisterFor(QString metricName, int t1, int t2); // t1/t2 not used yet
        QMap<QString, Banister*> banisterData;

        // athlete measures
        // note ride can override if passed
        double getWeight(QDate date, RideFile *ride=NULL);
        double getHeight(RideFile *ride=NULL);

        // athlete's calendar
        CalendarDownload *calendarDownload;
#ifdef GC_HAVE_ICAL
        ICalendar *rideCalendar;
        CalDAV *davCalendar;
#endif

        // Athlete's autoimport handling
        RideImportWizard *autoImport;
        RideAutoImportConfig *autoImportConfig;

        // ride metadata definitions
        RideMetadata *rideMetadata() { return rideMetadata_; }

        // preset charts
        QList<LTMSettings> presets;
        void loadCharts(); // load charts.xml
        void translateDefaultCharts(QList<LTMSettings>&charts);

        // named filters / queries
        NamedSearches *namedSearches;

        Context *context;

        // ride collection
        void selectRideFile(QString);
        void addRide(QString name, bool signal, bool select=true, bool useTempActivities=false, bool planned=false);
        void removeCurrentRide();

        // zones etc
        void notifyZonesChanged() { zonesChanged(); }
        void notifySeasonsChanged() { seasonsChanged(); }
        void notifyNamedSearchesChanged() { namedSearchesChanged(); }

        // import rides from athlete specific directory
        void importFilesWhenOpeningAthlete();

    signals:
        void zonesChanged();
        void seasonsChanged();
        void namedSearchesChanged();

    public slots:
        void checkCPX(RideItem*ride);
        void configChanged(qint32);

};



class AthleteDirectoryStructure : public QObject {
        Q_OBJECT

        public:
            AthleteDirectoryStructure(const QDir home);
            ~AthleteDirectoryStructure();

            QDir activities() { return QDir(myhome.absolutePath()+"/"+athlete_activities); }
            QDir tmpActivities() { return QDir(myhome.absolutePath()+"/"+athlete_tmp_activities); }
            QDir imports(){ return QDir(myhome.absolutePath()+"/"+athlete_imports);}
            QDir records(){ return QDir(myhome.absolutePath()+"/"+athlete_records);}
            QDir downloads() { return QDir(myhome.absolutePath()+"/"+athlete_downloads);}
            QDir fileBackup() { return QDir(myhome.absolutePath()+"/"+athlete_fileBackup);}
            QDir config() { return QDir(myhome.absolutePath()+"/"+athlete_config);}
            QDir cache() { return QDir(myhome.absolutePath()+"/"+athlete_cache);}
            QDir calendar() { return QDir(myhome.absolutePath()+"/"+athlete_calendar);}
            QDir workouts() { return QDir(myhome.absolutePath()+"/"+athlete_workouts);}
            QDir logs() { return QDir(myhome.absolutePath()+"/"+athlete_logs);}
            QDir temp() { return QDir(myhome.absolutePath()+"/"+athlete_temp);}
            QDir quarantine() { return QDir(myhome.absolutePath()+"/"+athlete_quarantine);}
            QDir planned() { return QDir(myhome.absolutePath()+"/"+athlete_planned);}
            QDir snippets() { return QDir(myhome.absolutePath()+"/"+athlete_snippets);}
            QDir media() { return QDir(myhome.absolutePath()+"/"+athlete_media);}
            QDir root() { return myhome; }

            // supporting functions to work with the subDirs
            void createAllSubdirs();            // create all new SubDirectories (or create only missing ones)
            bool subDirsExist();                // check for all new SubDirectories
            bool upgradedDirectoriesHaveData(); // check only for /activities and /config (as the main directories which have to bee there)

        private:

            QDir myhome;

            QString athlete_activities;
            QString athlete_tmp_activities;
            QString athlete_imports;
            QString athlete_records;
            QString athlete_downloads;
            QString athlete_fileBackup;
            QString athlete_config;
            QString athlete_cache;
            QString athlete_calendar;
            QString athlete_workouts;
            QString athlete_logs;
            QString athlete_temp;
            QString athlete_quarantine;
            QString athlete_planned;
            QString athlete_snippets;
            QString athlete_media;

};


#endif
