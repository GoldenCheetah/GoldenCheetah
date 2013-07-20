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
#include "RideItem.h"
#include "TimeUtils.h"

#ifdef Q_OS_MAC
// What versions are supported by this SDK?
#include <AvailabilityMacros.h>
#endif

class HomeWindow;
class GcToolBar;
class DiarySidebar;
class LTMSidebar;
class AnalysisSidebar;
class LionFullScreen;
class QTFullScreen;
class TrainSidebar;
class ChartSettings;
class QtMacSegmentedButton;
class QtMacButton;
class GcScopeBar;
class Library;
class BlankStateAnalysisPage;
class BlankStateHomePage;
class BlankStateDiaryPage;
class BlankStateTrainPage;
class QtSegmentControl;
class SaveSingleDialogWidget;

class MainWindow;
class Athlete;
class Context;

extern QList<MainWindow *> mainwindows; // keep track of all the MainWindows we have open
extern QDesktopWidget *desktop;         // how many screens / res etc

class MainWindow : public QMainWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        MainWindow(const QDir &home);
        ~MainWindow(); // temp to zap db - will move to tab //

        // global filters
        bool isfiltered; // used by lots of charts
        QStringList filters; // used by lots of charts

        // temporary access to chart settings
        ChartSettings *chartsettings() { return chartSettings; } // by HomeWindow

        // temporary access to the sidebars
        AnalysisSidebar *analysissidebar() {return analysisSidebar; } // by SearchBox
        TrainSidebar *trainsidebar() {return trainSidebar; } // by ErgDBDownloadDialog

        // temporary access to the context
        Context *contextmain() { return context; } // by ChooseCyclistDialog

    protected:

        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);

    signals:
        void filterChanged(QStringList&);

    public slots:

        // GUI
#ifndef Q_OS_MAC
        void toggleFullScreen();
#endif
        void splitterMoved(int, int);
        void aboutDialog();
        void helpView();
        void logBug();
        void closeAll();
        void actionClicked(int);

        // Search / Filter
#ifdef Q_OS_MAC
        void searchTextChanged(QString); // Mac Native Support
#endif
        void searchResults(QStringList); // from global search
        void searchClear();


        void checkBlankState();
        void closeBlankTrain();
        void closeBlankAnal();
        void closeBlankDiary();
        void closeBlankHome();
        void selectHome();
        void selectDiary();
        void selectAnalysis();
        void selectTrain();

        void setChartMenu();
        void addChart(QAction*);
        void setWindowMenu();
        void selectWindow(QAction*);
        void setSubChartMenu();
        void dateChanged(const QDate &);


        void showOptions();
        void toggleSidebar();
        void showSidebar(bool want);
        void showToolbar(bool want);
        void resetWindowLayout();
        void toggleStyle();
        void setStyle();
        void setStyleFromSegment(int); // special case for linux/win qtsegmentcontrol toggline

        // Analysis View
        void setActivityMenu();
        void showTools();
        void showRhoEstimator();

        // Training View
        void addDevice();
        void downloadErgDB();
        void manageLibrary();
        void showWorkoutWizard();
        void importWorkout();

        // Diary View
        void dateRangeChangedDiary(DateRange);
        void refreshCalendar();
#ifdef GC_HAVE_ICAL
        void uploadCalendar(); // upload ride to calendar
#endif

        // LTM View
        void dateRangeChangedLTM(DateRange);

        // Measures View
        void recordMeasure();
        void downloadMeasures();
        void downloadMeasuresFromZeo();

        // Athlete Collection
        void newCyclist();
        void openCyclist();

        // Activity Collection
        void rideSelected(RideItem*ride);
        bool saveRideSingleDialog(RideItem *);
        void saveSilent(RideItem *);
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
        void importFile();
        void splitRide();
        void mergeRide();
        void deleteRide();
        void saveRide();                        // save current ride menu item
        void revertRide();
        bool saveRideExitDialog();              // save dirty rides on exit dialog

    private:

        Context *context;

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

        // splitter for sidebar and main view
        QSplitter *splitter;

        // sidebar and views
        QStackedWidget *toolBox; // contains all left sidebars
        QStackedWidget *views;   // contains all the views

        // sidebars
        AnalysisSidebar *analysisSidebar;
        TrainSidebar *trainSidebar; // train view
        LTMSidebar *ltmSidebar; // home view
        DiarySidebar *diarySidebar; // diary

#if (defined Q_OS_MAC) && (defined GC_HAVE_LION)
        LionFullScreen *fullScreen;
#endif
#ifndef Q_OS_MAC
        QTFullScreen *fullScreen;
#endif

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
        QAction *rideWithGPSAction;
        QAction *ttbAction;

        // each view has its own controls
        QStackedWidget *masterControls,
                       *analysisControls,
                       *trainControls,
                       *diaryControls,
                       *homeControls;

        // Miscellany
        QSignalMapper *toolMapper;
};

#endif // _GC_MainWindow_h
