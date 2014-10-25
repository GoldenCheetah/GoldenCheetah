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

#include <QDir>
#include <QSqlDatabase>
#include <QTreeWidget>
#include <QtGui>
#include <QNetworkReply>
#include <QHeaderView>

class MetricAggregator;
class Zones;
class HrZones;
class PaceZones;
class RideFile;
class ErgFile;
class RideMetadata;
class WithingsDownload;
class ZeoDownload;
class CalendarDownload;
class ICalendar;
class CalDAV;
class Seasons;
class RideNavigator;
class Lucene;
class NamedSearches;
class RideFileCache;
class RideItem;
class IntervalItem;
class IntervalTreeView;
class QSqlTableModel;
class PDEstimate;
class LTMSettings;
class Routes;
class AthleteDirectoryStructure;

class Context;

class Athlete : public QObject
{
    Q_OBJECT

    public:
        Athlete(Context *context, const QDir &homeDir);
        ~Athlete();
        void close();

        // basic athlete info
        QString cyclist; // the cyclist name
        bool useMetricUnits;
        AthleteDirectoryStructure *home;
        const AthleteDirectoryStructure *directoryStructure() const {return home; }

        // zones
        const Zones *zones() const { return zones_; }
        const HrZones *hrZones() const { return hrzones_; }
        const PaceZones *paceZones() const { return pacezones_; }
        Zones *zones_;
        HrZones *hrzones_;
        PaceZones *pacezones_;
        void setCriticalPower(int cp);

        bool isclean;
        MetricAggregator *metricDB;
        QSqlTableModel *sqlModel;
        RideMetadata *rideMetadata_;
        Seasons *seasons;
        QList<PDEstimate> PDEstimates;
        Routes *routes;
        QList<RideFileCache*> cpxCache;

        // athlete's calendar
        CalendarDownload *calendarDownload;
        WithingsDownload *withingsDownload;
        ZeoDownload *zeoDownload;
#ifdef GC_HAVE_ICAL
        ICalendar *rideCalendar;
        CalDAV *davCalendar;
#endif

        // ride metadata definitions
        RideMetadata *rideMetadata() { return rideMetadata_; }

        // preset charts
        QList<LTMSettings> presets;
        void translateDefaultCharts(QList<LTMSettings>&charts);

        // indexes / filters
#ifdef GC_HAVE_LUCENE
        Lucene *lucene;
        NamedSearches *namedSearches;
#endif
        Context *context;

        // The ride collection -- transitionary
        QTreeWidget *treeWidget;
        QTreeWidgetItem *allRides;
        QTreeWidgetItem *allIntervals;
        IntervalTreeView *intervalWidget;

        // access to the ride collection
        void selectRideFile(QString);
        void addRide(QString name, bool bSelect=true);
        void removeCurrentRide();

        QTreeWidget *rideTreeWidget() { return treeWidget; }
        const QTreeWidgetItem *allRideItems() { return allRides; }
        const QTreeWidgetItem *allIntervalItems() { return allIntervals; }
        const RideFile * currentRide();
        IntervalTreeView *intervalTreeWidget() { return intervalWidget; }
        QTreeWidgetItem *mutableIntervalItems() { return allIntervals; }

        void notifyZonesChanged() { zonesChanged(); }
        void notifySeasonsChanged() { seasonsChanged(); }
        void notifyNamedSearchesChanged() { namedSearchesChanged(); }

        // import rides from athlete specific directory
        void importFilesWithoutDialog();

    signals:
        void zonesChanged();
        void seasonsChanged();
        void namedSearchesChanged();

    public slots:
        void rideTreeWidgetSelectionChanged();
        void intervalTreeWidgetSelectionChanged();
        void checkCPX(RideItem*ride);
        void updateRideFileIntervals();
        void configChanged();


};

// SubDirs into which the different file types of GC will be stored
// root() is still the .../AthleteDirectory/AthleteName like before

// static const QString Athlete_Activities = "/activities";
// static const QString Athlete_Imports = "/imports";
// static const QString Athlete_Downloads = "/downloads";
// static const QString Athlete_Config = "/config";
// static const QString Athlete_Cache = "/cache";
// static const QString Athlete_Calendar = "/calendar";
// static const QString Athlete_Workouts = "/workouts";
// static const QString Athlete_Logs = "/logs";
// static const QString Athlete_Temp = "/temp";




// For code adoption to new HomeDir Object, don't add subdir's yet
// the change needs migration from root() to subdirs being ready

static const QString Athlete_Activities = "";
static const QString Athlete_Imports = "";
static const QString Athlete_Downloads = "";
static const QString Athlete_Config = "";
static const QString Athlete_Cache = "";
static const QString Athlete_Calendar = "";
static const QString Athlete_Workouts = "";
static const QString Athlete_Temp = "";
static const QString Athlete_Logs = "";


class AthleteDirectoryStructure : public QObject {
        Q_OBJECT

        public:
            AthleteDirectoryStructure(const QDir home);
            ~AthleteDirectoryStructure();

            QDir activities() { return QDir(myhome.absolutePath()+Athlete_Activities); }
            QDir imports(){ return QDir(myhome.absolutePath()+Athlete_Imports);}
            QDir downloads() { return QDir(myhome.absolutePath()+Athlete_Downloads);}
            QDir config() { return QDir(myhome.absolutePath()+Athlete_Config);}
            QDir cache() { return QDir(myhome.absolutePath()+Athlete_Cache);}
            QDir calendar() { return QDir(myhome.absolutePath()+Athlete_Calendar);}
            QDir workouts() { return QDir(myhome.absolutePath()+Athlete_Workouts);}
            QDir logs() { return QDir(myhome.absolutePath()+Athlete_Logs);}
            QDir temp() { return QDir(myhome.absolutePath()+Athlete_Temp);}
            QDir root() { return myhome; }

        private:

            QDir myhome;

};


#endif
