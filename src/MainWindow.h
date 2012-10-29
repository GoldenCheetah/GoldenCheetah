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

#ifndef _GC_MainWindow_h
#define _GC_MainWindow_h 1
#include "GoldenCheetah.h"

#include <QDir>
#include <QSqlDatabase>
#include <QtGui>
#include <QNetworkReply>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "RideItem.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "GcWindowRegistry.h"
#include "QuarqdClient.h"
#include "RealtimeData.h"
#include "SpecialFields.h"
#include <boost/shared_ptr.hpp>

#ifdef Q_OS_MAC
// What versions are supported by this SDK?
#include <AvailabilityMacros.h>
#endif

class MetricAggregator;
class Zones;
class HrZones;
class RideFile;
class ErgFile;
class RideMetadata;
class WithingsDownload;
class CalendarDownload;
class DiaryWindow;
class ICalendar;
class CalDAV;
class HomeWindow;
class GcWindowTool;
class Seasons;
class IntervalSummaryWindow;
class RideNavigator;
class GcToolBar;
class GcCalendar;
class GcBubble;
class LionFullScreen;
class QTFullScreen;
class TrainTool;
class Lucene;
class DataFilter;
class SearchBox;

extern QList<MainWindow *> mainwindows; // keep track of all the MainWindows we have open

class MainWindow : public QMainWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        MainWindow(const QDir &home);

        // *********************************************
        // ATHLETE INFO
        // *********************************************

        // general data
        QString cyclist; // the cyclist name
        bool useMetricUnits;  // metric/imperial prefs
        QDir home;
        const Zones *zones() const { return zones_; }
        const HrZones *hrZones() const { return hrzones_; }
        void setCriticalPower(int cp);
        QSqlDatabase db;
        RideNavigator *listView;
        MetricAggregator *metricDB;
        Seasons *seasons;

        // athlete's ride library
        void addRide(QString name, bool bSelect=true);
        void removeCurrentRide();
        void getBSFactors(double &timeBS, double &distanceBS,
                          double &timeDP, double &distanceDP);

        // athlete's calendar
        CalendarDownload *calendarDownload;
#ifdef GC_HAVE_ICAL
        ICalendar *rideCalendar;
        CalDAV *davCalendar;
#endif

        // *********************************************
        // ATHLETE RIDE LIBRARY
        // *********************************************

        // save a ride to disk
        void saveSilent(RideItem *);
        bool saveRideSingleDialog(RideItem *);

        // currently selected ride item, files, metadata
        void selectRideFile(QString);
        QTreeWidget *rideTreeWidget() { return treeWidget; }
        const QTreeWidgetItem *allRideItems() { return allRides; }
        RideItem *rideItem() const { return ride; }
        const RideFile *currentRide();
        const RideItem *currentRideItem() { return ride; }
        void updateRideFileIntervals();
        RideMetadata *rideMetadata() { return _rideMetadata; }

        // ride intervals
        const QTreeWidgetItem *allIntervalItems() { return allIntervals; }
        QTreeWidget *intervalTreeWidget() { return intervalWidget; }
        QTreeWidgetItem *mutableIntervalItems() { return allIntervals; }

        // *********************************************
        // MAINWINDOW STATE / GUI DATA
        // *********************************************

        // Top-level views
        HomeWindow *homeWindow;
        HomeWindow *diaryWindow;
        HomeWindow *trainWindow;
        HomeWindow *analWindow;
        HomeWindow *currentWindow;  // tracks the curerntly showing window

        // state data
        SpecialFields specialFields;
        int session;
        bool isclean;
        bool ismultisave;

        void setBubble(QString text, QPoint pos = QPoint(), Qt::Orientation o = Qt::Horizontal);

#if (defined Q_OS_MAC) && (defined GC_HAVE_LION)
        LionFullScreen *fullScreen;
#endif
#ifndef Q_OS_MAC
        QTFullScreen *fullScreen;
#endif
        TrainTool *trainTool;
#ifdef GC_HAVE_LUCENE
        SearchBox *searchBox;
        Lucene *lucene;
        DataFilter *datafilter;
#endif

        // *********************************************
        // APPLICATION EVENTS
        // *********************************************

        // MainWindow signals are used to notify
        // widgets of important events, these methods
        // can be called to raise signals
        void notifyConfigChanged(); // used by ConfigDialog to notify MainWindow
                                    // when config has changed - and to get a
                                    // signal emitted to notify its children
        void notifyRideSelected();  // used by RideItem to notify when
                                    // rideItem date/time changes
        void notifyRideClean() { rideClean(); }
        void notifyRideDirty() { rideDirty(); }

        // realtime signals
        void notifyTelemetryUpdate(const RealtimeData &rtData) { telemetryUpdate(rtData); }
        void notifyErgFileSelected(ErgFile *x) { workout=x; ergFileSelected(x); }
        ErgFile *currentErgFile() { return workout; }
        void notifyMediaSelected( QString x) { mediaSelected(x); }
        void notifySetNow(long x) { now = x; setNow(x); }
        long getNow() { return now; }
        void notifyNewLap() { emit newLap(); }
        void notifyStart() { emit start(); }
        void notifyUnPause() { emit unpause(); }
        void notifyPause() { emit pause(); }
        void notifyStop() { emit stop(); }
        void notifySeek(long x) { emit seek(x); }



    protected:

        Zones *zones_;
        HrZones *hrzones_;

        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);

    signals:

        void intervalSelected();
        void intervalZoom(IntervalItem*);
        void intervalsChanged();
        void zonesChanged();
        void seasonsChanged();
        void configChanged();
        void rideAdded(RideItem *);
        void rideDeleted(RideItem *);
        void rideDirty();
        void rideClean();

        // search
#ifdef GC_HAVE_LUCENE
        void searchSubmit(QString);
        void searchResults(QStringList);
        void searchClear();
#endif

        // realtime
        void telemetryUpdate(RealtimeData rtData);
        void ergFileSelected(ErgFile *);
        void mediaSelected(QString);
        void setNow(long);
        void seek(long);
        void newLap();
        void start();
        void unpause();
        void pause();
        void stop();

    public slots:
        void showTreeContextMenuPopup(const QPoint &);
        void closeAll();

    private slots:
        void rideTreeWidgetSelectionChanged();
        void intervalTreeWidgetSelectionChanged();
        void splitterMoved(int, int);
        void newCyclist();
        void openCyclist();
        void downloadRide();
        void manualRide();
        void exportRide();
        void exportBatch();
        void exportMetrics();
        void uploadStrava();
        void uploadRideWithGPSAction();
        void uploadTtb();
        void downloadErgDB();
        void manualProcess(QString);
#ifdef GC_HAVE_SOAP
        void uploadTP();
        void downloadTP();
#endif
#ifdef GC_HAVE_ICAL
        void uploadCalendar(); // upload ride to calendar
#endif
        void importFile();
        void findBestIntervals();
        void addIntervals();
        void addIntervalForPowerPeaksForSecs(RideFile *ride, int windowSizeSecs, QString name);
        void findPowerPeaks();
        void splitRide();
        void deleteRide();
        void aboutDialog();
        void saveRide();                        // save current ride menu item
        void revertRide();
        void enableSaveButton();                // enable/disable save button when ride changes etc
        bool saveRideExitDialog();              // save dirty rides on exit dialog
        void showOptions();
        void showTools();
        void showRhoEstimator();
        void toggleSidebar();
        void toggleRSidebar();
        void showSidebar(bool want);
        void showRSidebar(bool want);
        void showToolbar(bool want);
        void showWorkoutWizard();
        void resetWindowLayout();
        void dateChanged(const QDate &);
        void showContextMenuPopup(const QPoint &);
        void deleteInterval();
        void renameInterval();
        void zoomInterval();
        void frontInterval();
        void backInterval();
        void intervalEdited(QTreeWidgetItem *, int);
#ifdef GC_HAVE_LIBOAUTH
        void tweetRide();
#endif

        // working with measures, not rides
        void recordMeasure();
        void downloadMeasures();
        void exportMeasures();
        void importMeasures();

        // get calendars
        void refreshCalendar();
        void importCalendar();
        void exportCalendar();

        void helpView();
        void logBug();

        void selectHome();
        void selectDiary();
        void selectAnalysis();
        void selectTrain();
        void selectAthlete();

        void setActivityMenu();
        void setWindowMenu();
        void selectWindow(QAction*);

        void setChartMenu();
        void setSubChartMenu();
        void addChart(QAction*);

        void toggleStyle();
        void setStyle();
        void showDock();
#ifndef Q_OS_MAC
        void toggleFullScreen();
#endif

#ifdef GC_HAVE_LUCENE
        void filterSubmitted(QString);
        void filterCleared();
        void searchSubmitted(QString);
        void searchCleared();
#endif

    protected:

        static QString notesFileName(QString rideFileName);

    private:
        boost::shared_ptr<QSettings> settings;
        IntervalItem *activeInterval; // currently active for context menu popup
        RideItem *activeRide; // currently active for context menu popup
        RideItem *ride;  // the currently selected ride

        ErgFile *workout; // the currently selected workout file
        long now;

        QToolBox *toolBox;
        GcToolBar *toolbar;
        QDockWidget *dock;
        QAction *homeAct, *diaryAct, *analysisAct, *measuresAct, *trainAct, *athleteAct, *helpAct, *configAct;
        QAction *styleAction;
        QAction *showhideToolbar;
        QAction *showhideSidebar;
        QAction *showhideRSidebar;

        // toolbar butttons
        QPushButton *side, *rside, *style, *full, *saveButton;
        QWidget *analButtons;
        QMenu *chartMenu;
        QMenu *subChartMenu;

        QStackedWidget *views;
        QAction *sideView;
        QAction *toolView;
        QAction *stravaAction, *rideWithGPSAction;
        QAction *ttbAction;
        QMenu *windowMenu;
        GcBubble *bubble;
        GcCalendar *gcCalendar;

        // each view has its own controls XXX more to come
        QStackedWidget *masterControls,
                       *analysisControls,
                       *trainControls,
                       *diaryControls,
                       *homeControls;

        // sidebar
        QTreeWidgetItem *allRides;
        QTreeWidgetItem *allIntervals;
        IntervalSummaryWindow *intervalSummaryWindow;
        QSplitter *leftLayout;
        QWidget *rightBar;
        RideMetadata *_rideMetadata;
        GcWindowTool *chartTool;

        QSplitter *summarySplitter;
        QSplitter *splitter;
        QSplitter *metaSplitter;
        QTreeWidget *treeWidget;
        QSplitter *intervalSplitter;
        IntervalTreeView *intervalWidget;

        // Miscellany
        QuarqdClient *client;
        QSignalMapper *toolMapper;
        WithingsDownload *withingsDownload;
        bool parseRideFileName(const QString &name, QString *notesFileName, QDateTime *dt);
};

#endif // _GC_MainWindow_h
