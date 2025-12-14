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

#ifndef _GC_Context_h
#define _GC_Context_h 1

#include "DataContext.h"
#include "ContextConstants.h"

// Forward declarations to reduce includes
class RideFile;
class RealtimeData;
class RideItem;
class IntervalItem;
class ErgFile;
class ErgFileBase;
class VideoSyncFile;
class Athlete;
class Season;

#ifdef GC_HAS_CLOUD_DB
class CloudDBChartListDialog;
class CloudDBUserMetricListDialog;
#endif

class Context;
class MainWindow;
class NewSideBar;
class QAction;
class AthleteTab;
class NavigationModel;
class RideMetadata;
class ColorEngine;
class ModelFilter;
class QWebEngineProfile;


class GlobalContext : public QObject
{
    Q_OBJECT

    public:

        GlobalContext();
        static GlobalContext *context();
        void notifyConfigChanged(qint32);

        // metadata etc
        RideMetadata *rideMetadata;
        ColorEngine *colorEngine;

        // metric units
        bool useMetricUnits;

    public slots:
        void readConfig(qint32);
        void userMetricsConfigChanged();

    signals:
        void configChanged(qint32); // for global widgets that aren't athlete specific

};

class RideNavigator;
class Context : public DataContext
{
    Q_OBJECT;

    public:
        Context(MainWindow *mainWindow);
        ~Context();

        // check if valid (might be deleted)
        static bool isValid(Context *);

        // mainwindow state
        NavigationModel *nav;
        int viewIndex;
        int style;
        QString searchText;
        QString workoutFilterText;
        bool scopehighlighted;
        bool showSidebar;
        bool showLowbar;
        bool showToolbar;

        // current selections and widgetry
        RideNavigator *rideNavigator;
        AthleteTab *tab;

        // API to access MainWindow functionality without exposing the pointer
        QWidget *mainWidget() const;
        void fillinWorkoutFilterBox(const QString &text);

    public slots:
        void switchToTrainView();
        void switchToTrendsView();
        void switchToAnalysisView();
        void switchToDiaryView();
        bool isCurrent() const;
        void requestImportFile();
        void requestDownloadRide();

        void saveRide();
        void revertRide();
        void deleteRide();
        void splitRide();

        // More proxies
        void resetPerspective(int index);
        void setToolButtons();
        NewSideBar *sidebar();
        QAction *showHideSidebarAction();
        bool isMainWindowInitialized();

        // Proxies for slots previously accessed directly
        void importFile(); 
        void importCloud();
        void downloadRide();
        void addDevice();
        void manageLibrary();
        void downloadTrainerDay();
        void importChart();
        void addChart(QAction *action);
        void setChartMenu(QMenu *menu);
        void openAthleteTab(QString path);
        void closeAthleteTab(QString path);
        void setSidebarVisible(bool show);
        void setLowbarVisible(bool show);

        // Additional Train view proxies
        void importWorkout();
        void showWorkoutWizard();
        void downloadStravaRoutes();
        void switchPerspective(int index);

        // Forwarding protected events from MainWindow (used by ChartSpace)
        void forwardDragEnter(QDragEnterEvent *event);
        void forwardDragLeave(QDragLeaveEvent *event);
        void forwardDragMove(QDragMoveEvent *event);
        void forwardDrop(QDropEvent *event);

        bool isStarting() const;
        void saveSilent(RideItem *item);
        bool saveRideSingleDialog(RideItem *item);

    public:
        // Public data members (not slots)
        bool isfiltered;
        bool ishomefiltered;
        QStringList filters; // searchBox filters
        QStringList homeFilters; // homewindow sidebar filters


#ifdef GC_HAS_CLOUD_DB
        // CloudDB - common data
        CloudDBChartListDialog *cdbChartListDialog;
        CloudDBUserMetricListDialog *cdbUserMetricListDialog;
#endif

        // WebEngineProfile for this user
        QWebEngineProfile* webEngineProfile;

    public slots:

        // *********************************************
        // APPLICATION EVENTS
        // *********************************************
        void notifyConfigChanged(qint32); // Global and athlete specific changes communicated via this signal

        // athlete load/close
        void notifyLoadProgress(QString folder, double progress) { emit loadProgress(folder,progress); }
        void notifyLoadCompleted(QString folder, Context *context) { emit loadCompleted(folder,context); } // Athlete loaded
        void notifyAthleteClose(QString folder, Context *context) { emit athleteClose(folder,context); }
        void notifyLoadDone(QString folder, Context *context) { emit loadDone(folder, context); } // MainWindow finished

        // Realtime/Train slots
        void notifyTelemetryUpdate(const RealtimeData &rtData);
        void notifySetNotification(QString, int=0);

        // preset charts
        void notifyPresetsChanged() { emit presetsChanged(); }
        void notifyPresetSelected(int n) { emit presetSelected(n); }

        // filters
        void setHomeFilter(QStringList&f) { homeFilters=f; ishomefiltered=true; emit homeFilterChanged(); }
        void clearHomeFilter() { homeFilters.clear(); ishomefiltered=false; emit homeFilterChanged(); }

        void setFilter(QStringList&f) { filters=f; isfiltered=true; emit filterChanged(); }
        void clearFilter() { filters.clear(); isfiltered=false; emit filterChanged(); }

        void setWorkoutFilters(QList<ModelFilter*> &f) { emit workoutFiltersChanged(f); }
        void clearWorkoutFilters() { emit workoutFiltersRemoved(); }

        // user metrics - cascade
        void notifyUserMetricsChanged() { emit userMetricsChanged(); }

        // view changed
        void setIndex(int i) { viewIndex = i; emit viewChanged(i); }

        void notifyIntervalZoom(IntervalItem*x) { emit intervalZoom(x); }
        void notifyZoomOut() { emit zoomOut(); }
        void notifyIntervalSelected() { intervalSelected(); }
        void notifyIntervalItemSelectionChanged(IntervalItem*x) { intervalItemSelectionChanged(x); }
        void notifyIntervalHover(IntervalItem *x) { emit intervalHover(x); }


        void notifyAutoDownloadStart() { emit autoDownloadStart(); }
        void notifyAutoDownloadEnd() { emit autoDownloadEnd(); }
        void notifyAutoDownloadProgress(QString s, double x, int i, int n) { emit autoDownloadProgress(s, x, i, n); }

        void notifyRefreshStart() { emit refreshStart(); }
        void notifyRefreshEnd() { emit refreshEnd(); }
        void notifyRefreshUpdate(QDate date) { emit refreshUpdate(date); }

        void notifyRMessage(QString x) { emit rMessage(x); }

        void notifySteerScroll(int scrollAmount) { emit steerScroll(scrollAmount); }
        void notifyEstimatesRefreshed() { emit estimatesRefreshed(); }

    protected:

        // we need to act since the user metric config changed
        // and we need to notify other contexts !
        void userMetricsConfigChanged();

    signals:

        // loading an athlete
        void loadProgress(QString,double);
        void loadCompleted(QString, Context*);
        void loadDone(QString, Context*);
        void athleteClose(QString, Context*);

        // global filter changed
        void filterChanged();
        void homeFilterChanged();

        // workout filters
        void workoutFiltersChanged(QList<ModelFilter*>&);
        void workoutFiltersRemoved();

        void workoutsChanged(); // added or deleted a workout in train view
        void VideoSyncChanged(); // added or deleted a workout in train view
        void presetsChanged();
        void presetSelected(int);

        // user metrics
        void configChanged(qint32);
        void userMetricsChanged();

        // view changed
        void viewChanged(int);

        // refreshing stats
        void refreshStart();
        void refreshEnd();
        void refreshUpdate(QDate);
        void estimatesRefreshed();

        // cloud download
        void autoDownloadStart();
        void autoDownloadEnd();
        void autoDownloadProgress(QString, double, int, int);

        void intervalSelected();
        void intervalHover(IntervalItem*);
        void intervalZoom(IntervalItem*);
        void intervalItemSelectionChanged(IntervalItem*);
        void zoomOut();

        void setNotification(const QString &msg, int timeout);
        void clearNotification();

        // R messages
        void rMessage(QString);

        // Trainer controls
        void steerScroll(int);

    private:
        MainWindow *mainWindow;
};
#endif // _GC_Context_h
