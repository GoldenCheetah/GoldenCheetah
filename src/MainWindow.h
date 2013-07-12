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
class SaveSingleDialogWidget;
class SplitActivityWizard;

class MainWindow;
class Athlete;
class Context;

extern QList<MainWindow *> mainwindows; // keep track of all the MainWindows we have open
extern QDesktopWidget *desktop;         // how many screens / res etc

class MainWindow : public QMainWindow
{
    Q_OBJECT
    G_OBJECT

    // transitional
    friend class ::Context;
    friend class ::SaveSingleDialogWidget;
    friend class ::SplitActivityWizard;

    public:

        MainWindow(const QDir &home);

        // transitionary
        Context *context;

        // currently selected ride item, ride list
        void selectRideFile(QString);
        QTreeWidget *rideTreeWidget() { return treeWidget; }
        const QTreeWidgetItem *allRideItems() { return allRides; }

        // ride intervals
        const QTreeWidgetItem *allIntervalItems() { return allIntervals; }
        IntervalTreeView *intervalTreeWidget() { return intervalWidget; }
        QTreeWidgetItem *mutableIntervalItems() { return allIntervals; }
        void updateRideFileIntervals();

        // filters changed
        void notifyFilter(QStringList f) { filters = f; emit filterChanged(f); }

        // *********************************************
        // MAINWINDOW STATE / GUI DATA
        // *********************************************

        RideNavigator *listView;

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
        bool init; // we built children (i.e. didn't quit at upgrade)

        // global filters
        bool isfiltered;
        QStringList filters;

        void setBubble(QString text, QPoint pos = QPoint(), Qt::Orientation o = Qt::Horizontal);

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

        // transitionary

    protected:

        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);

        const RideFile *currentRide();

    signals:
        void filterChanged(QStringList&);

        // transitionary will move too XXX
        void intervalSelected();
        void intervalZoom(IntervalItem*);
        void intervalsChanged();
        void rideAdded(RideItem *);
        void rideDeleted(RideItem *);

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

        // transitionary
        void addRide(QString name, bool bSelect=true);
        bool saveRideSingleDialog(RideItem *);
        void removeCurrentRide();
        void saveSilent(RideItem *);

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

        // from global search
        void searchResults(QStringList);
        void searchClear();


    protected:

    private:

        // active when right clicked
        IntervalItem *activeInterval; // currently active for context menu popup
        RideItem *activeRide; // currently active for context menu popup

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
        bool parseRideFileName(const QString &name, QDateTime *dt);

};

#endif // _GC_MainWindow_h
