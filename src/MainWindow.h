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
#include <qwt_plot_curve.h>
#include "RideItem.h"
#include "GcWindowRegistry.h"
#include "RealtimeData.h"
#include "SpecialFields.h"
#include "TimeUtils.h"

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
class ZeoDownload;
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
class GcMultiCalendar;
class GcBubble;
class LTMSidebar;
class LionFullScreen;
class QTFullScreen;
class TrainTool;
class Lucene;
class NamedSearches;
class ChartSettings;
class QtMacSegmentedButton;
class QtMacButton;
class GcScopeBar;
class RideFileCache;
class Library;
class BlankStateAnalysisPage;
class BlankStateHomePage;
class BlankStateDiaryPage;
class BlankStateTrainPage;
class GcSplitter;
class GcSplitterItem;
class QtSegmentControl;
class IntervalItem;
class IntervalTreeView;

extern QList<MainWindow *> mainwindows; // keep track of all the MainWindows we have open
extern QDesktopWidget *desktop;         // how many screens / res etc

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
        bool useMetricUnits;
        QDir home;
        const Zones *zones() const { return zones_; }
        const HrZones *hrZones() const { return hrzones_; }
        void setCriticalPower(int cp);
        QSqlDatabase db;
        RideNavigator *listView;
        MetricAggregator *metricDB;
        RideMetadata *_rideMetadata;
        Seasons *seasons;
        QList<RideFileCache*> cpxCache;

        // athlete's calendar
        CalendarDownload *calendarDownload;
#ifdef GC_HAVE_ICAL
        ICalendar *rideCalendar;
        CalDAV *davCalendar;
#endif

        // *********************************************
        // ATHLETE RIDE LIBRARY
        // *********************************************

        // athlete's ride library
        void addRide(QString name, bool bSelect=true);
        void removeCurrentRide();

        // save a ride to disk
        void saveSilent(RideItem *);
        bool saveRideSingleDialog(RideItem *);

        // currently selected ride item, ride list
        void selectRideFile(QString);
        QTreeWidget *rideTreeWidget() { return treeWidget; }
        const QTreeWidgetItem *allRideItems() { return allRides; }
        RideItem *rideItem() const { return ride; }
        const RideFile *currentRide();
        const RideItem *currentRideItem() { return ride; }

        // last date range selected in diary/home view
        DateRange currentDateRange() { return _dr; }

        // ride intervals
        const QTreeWidgetItem *allIntervalItems() { return allIntervals; }
        IntervalTreeView *intervalTreeWidget() { return intervalWidget; }
        QTreeWidgetItem *mutableIntervalItems() { return allIntervals; }
        void updateRideFileIntervals();

        // ride metadata definitions
        RideMetadata *rideMetadata() { return _rideMetadata; }

        // *********************************************
        // MAINWINDOW STATE / GUI DATA
        // *********************************************

        // Top-level views
        HomeWindow *homeWindow;
        HomeWindow *diaryWindow;
        HomeWindow *trainWindow;
        HomeWindow *analWindow;
        HomeWindow *currentWindow;  // tracks the curerntly showing window

        BlankStateAnalysisPage *blankStateAnalysisPage;
        BlankStateHomePage *blankStateHomePage;
        BlankStateDiaryPage *blankStateDiaryPage;
        BlankStateTrainPage *blankStateTrainPage;

        bool showBlankAnal;
        bool showBlankTrain;
        bool showBlankHome;
        bool showBlankDiary;

        ChartSettings *chartSettings;

        // state data
        SpecialFields specialFields;
        int session;
        bool isclean;
        bool ismultisave;

        void setBubble(QString text, QPoint pos = QPoint(), Qt::Orientation o = Qt::Horizontal);

#ifdef GC_HAVE_LUCENE
        Lucene *lucene;
        NamedSearches *namedSearches;
#endif

        // splitter for sidebar and main view
        QSplitter *splitter;

        // sidebar and views
        QStackedWidget *toolBox; // contains all left sidebars
        QStackedWidget *views;   // contains all the views

        // sidebars
        TrainTool *trainTool; // train view
        LTMSidebar *ltmSidebar; // home view
        GcCalendar *gcCalendar; // diary

        // analysis view
        // constructed from parts in mainwindow
        GcSplitter *analSidebar;
        GcSplitterItem *analItem, *intervalItem;
        QSplitter *intervalSplitter;
        IntervalTreeView *intervalWidget;
        GcMultiCalendar *gcMultiCalendar;

#if (defined Q_OS_MAC) && (defined GC_HAVE_LION)
        LionFullScreen *fullScreen;
#endif
#ifndef Q_OS_MAC
        QTFullScreen *fullScreen;
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
        void notifyZonesChanged() { zonesChanged(); }

        // realtime signals
        void notifyTelemetryUpdate(const RealtimeData &rtData) { telemetryUpdate(rtData); }
        void notifyErgFileSelected(ErgFile *x) { workout=x; ergFileSelected(x); }
        ErgFile *currentErgFile() { return workout; }
        void notifyMediaSelected( QString x) { mediaSelected(x); }
        void notifySelectVideo(QString x) { selectMedia(x); }
        void notifySelectWorkout(QString x) { selectWorkout(x); }
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

        // realtime
        void telemetryUpdate(RealtimeData rtData);
        void ergFileSelected(ErgFile *);
        void mediaSelected(QString);
        void selectWorkout(QString); // ask traintool to select this
        void selectMedia(QString); // ask traintool to select this
        void setNow(long);
        void seek(long);
        void newLap();
        void start();
        void unpause();
        void pause();
        void stop();

    public slots:
        void checkCPX(RideItem*);
        void showTreeContextMenuPopup(const QPoint &);
        void closeAll();
        void addDevice();

        void checkBlankState();
        void closeBlankTrain();
        void closeBlankAnal();
        void closeBlankDiary();
        void closeBlankHome();

        void downloadErgDB();
        void manageLibrary();
        void showWorkoutWizard();

        void analysisPopup();
        void intervalPopup();

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
        void downloadStrava();
        void uploadRideWithGPSAction();
        void uploadTtb();
        void manualProcess(QString);
#ifdef GC_HAVE_SOAP
        void uploadTP();
        void downloadTP();
#endif
#ifdef GC_HAVE_ICAL
        void uploadCalendar(); // upload ride to calendar
#endif
        void importFile();
        void importWorkout();
        void addIntervals();
        void addIntervalForPowerPeaksForSecs(RideFile *ride, int windowSizeSecs, QString name);
        void findPowerPeaks();
        void splitRide();
        void deleteRide();
        void aboutDialog();
        void saveRide();                        // save current ride menu item
        void revertRide();
        bool saveRideExitDialog();              // save dirty rides on exit dialog
        void showOptions();
        void showTools();
        void showRhoEstimator();
        void toggleSidebar();
        void showSidebar(bool want);
        void showToolbar(bool want);
        void resetWindowLayout();
        void dateChanged(const QDate &);
        void showContextMenuPopup(const QPoint &);
        void editInterval(); // from right click
        void deleteInterval(); // from right click
        void renameInterval(); // from right click
        void zoomInterval(); // from right click
        void sortIntervals(); // from menu popup
        void renameIntervalSelected(void); // from menu popup
        void renameIntervalsSelected(void); // from menu popup -- rename a series
        void editIntervalSelected(); // from menu popup
        void deleteIntervalSelected(void); // from menu popup
        void zoomIntervalSelected(void); // from menu popup
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
        void downloadMeasuresFromZeo();

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

        void setActivityMenu();
        void setWindowMenu();
        void selectWindow(QAction*);

        void setChartMenu();
        void setSubChartMenu();
        void addChart(QAction*);

        void toggleStyle();
        void setStyle();
        // special case for linux/win qtsegmentcontrol toggline
        void setStyleFromSegment(int);

#ifndef Q_OS_MAC
        void toggleFullScreen();
#else
        // Mac Native Support
        void searchTextChanged(QString);
#endif
        void actionClicked(int);

        void dateRangeChangedDiary(DateRange);
        void dateRangeChangedLTM(DateRange);

    protected:

    private:

        // active when right clicked
        IntervalItem *activeInterval; // currently active for context menu popup
        RideItem *activeRide; // currently active for context menu popup

        // current selections
        RideItem *ride;  // the currently selected ride
        DateRange _dr;   // the currently selected date range
        ErgFile *workout; // the currently selected workout file

        long now; // point in time during train session

#ifdef Q_OS_MAC
        // Mac Native Support
        QtMacButton *import, *compose, *sidebar;
        QtMacSegmentedButton *actbuttons, *styleSelector;
        QToolBar *head;
#else
        // Not on Mac so use other types
        QPushButton *import, *compose, *sidebar;
        QtSegmentControl *actbuttons, *styleSelector;
        GcToolBar *head;
        QPushButton *full;

        // the icons
        QIcon importIcon, composeIcon, intervalIcon, splitIcon,
              deleteIcon, sidebarIcon, tabbedIcon, tiledIcon;
#endif
        GcScopeBar *scopebar;

        // chart menus
        QMenu *chartMenu;
        QMenu *subChartMenu;

        // Application menu
        QMenu *windowMenu;

        // Application actions
        // only keeping those used outside of mainwindow constructor
        QAction *styleAction;
        QAction *showhideSidebar;
        QAction *stravaAction;
        QAction *rideWithGPSAction;
        QAction *ttbAction;

        GcBubble *bubble;

        // each view has its own controls
        QStackedWidget *masterControls,
                       *analysisControls,
                       *trainControls,
                       *diaryControls,
                       *homeControls;

        // central data structure
        QTreeWidget *treeWidget;
        QTreeWidgetItem *allRides;
        QTreeWidgetItem *allIntervals;
        IntervalSummaryWindow *intervalSummaryWindow;

        // Miscellany
        QSignalMapper *groupByMapper;
        QSignalMapper *toolMapper;
        WithingsDownload *withingsDownload;
        ZeoDownload *zeoDownload;
        bool parseRideFileName(const QString &name, QDateTime *dt);

};

#endif // _GC_MainWindow_h
