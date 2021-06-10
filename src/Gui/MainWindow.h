/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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
#include <QMainWindow>
#include <QStackedWidget>
#include "RideItem.h"
#include "TimeUtils.h"
#include "DragBar.h"
#ifdef GC_HAS_CLOUD_DB
#include "CloudDBChart.h"
#include "CloudDBUserMetric.h"
#include "CloudDBVersion.h"
#include "CloudDBTelemetry.h"
#endif

#ifdef Q_OS_MAC
// What versions are supported by this SDK?
#include <AvailabilityMacros.h>
#endif

#if defined(WIN32)
// Macro to avoid Code Duplication in multiple files
// QtWebEngine puts it's cache into the User directory (only on Windows) - so do not show in list
# define SKIP_QTWE_CACHE \
   QStringList webEngineDirs; \
   webEngineDirs << "QtWebEngine" << "cache"; \
   if (webEngineDirs.contains(name)) continue;
#else
# define SKIP_QTWE_CACHE
#endif



class QTFullScreen;
class GcToolBar;
class Library;
class QtSegmentControl;
class SaveSingleDialogWidget;
class ChooseCyclistDialog;
class SearchFilterBox;
class NewSideBar;
class AthleteView;


class MainWindow;
class Athlete;
class AthleteLoader;
class Context;
class Tab;


extern QList<MainWindow *> mainwindows; // keep track of all the MainWindows we have open
extern QDesktopWidget *desktop;         // how many screens / res etc
extern QString gcroot;                  // root directory for gc

class MainWindow : public QMainWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        MainWindow(const QDir &home);
        ~MainWindow(); // temp to zap db - will move to tab //

        void byebye() { close(); } // go bye bye for a restart
        bool init; // if constructor has completed set to true

        // when loading athlete
        QLabel *progress;
        int loading;

        // currently selected tab
        Tab *athleteTab() { return currentTab; }
        NewSideBar *newSidebar() { return sidebar; }

        // tab view keeps this up to date
        QAction *showhideSidebar;

    protected:

        // used by ChooseCyclistDialog to see which athletes
        // have already been opened
        friend class ::ChooseCyclistDialog;
        friend class ::AthleteLoader;
        QMap<QString,Tab*> tabs;
        Tab *currentTab;
        QList<Tab*> tabList;

        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);

        // working with splash screens
        QWidget *splash;
        void setSplash(bool first=false);
        void clearSplash();

    signals:
        void backClicked();
        void forwardClicked();
        void openingAthlete(QString, Context *);

    public slots:

        bool eventFilter(QObject*,QEvent*);

        // GUI
        void toggleFullScreen();
        void aboutDialog();
        void helpWindow();
        void helpView();
        void logBug();
        void support();
        void actionClicked(int);

        // chart importing
        void importCharts(QStringList);

        // open and closing windows and tabs
        void closeWindow();

        void setOpenTabMenu(); // set the Open Tab menu
        void newCyclistTab();  // create a new Cyclist
        void openTab(QString name);
        void loadCompleted(QString name, Context *context);
        void closeTabClicked(int index); // user clicked to close tab
        bool closeTab(QString name); // close named athlete
        bool closeTab();       // close current, might not if the user
                               // changes mind if there are unsaved changes.
        void removeTab(Tab*);  // remove without question
        void switchTab(int index); // for switching between one tab and another

        // sidebar selecting views and actions
        void sidebarClicked(int id);
        void sidebarSelected(int id);

        // Athlete Backup
        void setBackupAthleteMenu();
        void backupAthlete(QString name);

        // Search / Filter
        void setFilter(QStringList);
        void clearFilter();

        void selectAthlete();
        void selectHome();
        void selectDiary();
        void selectAnalysis();
        void selectTrain();
        void selectInterval();

        void setChartMenu();
        void setSubChartMenu();
        void setChartMenu(QMenu *);
        void addChart(QAction*);
        void importChart();

        // menus to reflect cloud
        void setUploadMenu();
        void setSyncMenu();
        void checkCloud();
        void importCloud(); // used to setup and sync in one on first run (see BlankState.cpp)

        void showOptions();

        void toggleSidebar();
        void showSidebar(bool want);
        void showToolbar(bool want);
        void showTabbar(bool want);
        void resetWindowLayout();
        void toggleStyle();
        void setToolButtons(); // set toolbar buttons to match tabview
        void setStyleFromSegment(int); // special case for linux/win qtsegmentcontrol toggline
        void toggleLowbar();
        void showLowbar(bool want);

        // Analysis View
        void showEstimateCP();
        void showSolveCP();
        void showRhoEstimator();
        void showVDOTCalculator();

        // Training View
        void addDevice();
        void downloadErgDB();
        void downloadTodaysPlanWorkouts();
        void manageLibrary();
        void showWorkoutWizard();
        void importWorkout();

        // Measures
        void setMeasuresMenu();
        void downloadMeasures(QAction *);

        // cloud
        void uploadCloud(QAction *);
        void syncCloud(QAction *);

        // Activity Collection
        void addIntervals(); // pass thru to tab
        bool saveRideSingleDialog(Context *, RideItem *);
        void saveSilent(Context *, RideItem *);
        void saveAllFilesSilent(Context *);
        void downloadRide();
        void manualRide();
        void exportRide();
        void exportBatch();
        void generateHeatMap();
        void exportMetrics();
        void addAccount();
        void manualProcess(QString);
        void importFile();
        void splitRide();
        void mergeRide();
        void deleteRide();
        void saveRide();                        // save current ride menu item
        void saveAllUnsavedRides();
        void revertRide();
        bool saveRideExitDialog(Context *);              // save dirty rides on exit dialog

        // autoload rides from athlete specific directory (preferences)
        void ridesAutoImport();

#ifdef GC_WANT_PYTHON
        // Python fix scripts
        void onEditMenuAboutToShow();
        void buildPyFixesMenu();
        void showManageFixPyScriptsDlg();
        void showCreateFixPyScriptDlg();
#endif

#ifdef GC_HAS_CLOUD_DB
        // CloudDB actions
        void cloudDBuserEditChart();
        void cloudDBcuratorEditChart();
        void cloudDBuserEditUserMetric();
        void cloudDBcuratorEditUserMetric();
        void cloudDBshowStatus();
        void addChartFromCloudDB();
        void exportChartToCloudDB();
#endif
        // save and restore state to context
        void saveGCState(Context *);
        void restoreGCState(Context *);

        void configChanged(qint32);

    private:

        NewSideBar *sidebar;
        AthleteView *athleteView;

#ifndef Q_OS_MAC
        QTFullScreen *fullScreen;
#endif

        SearchFilterBox *searchBox;

        // Not on Mac so use other types
        QPushButton *sidelist, *lowbar;
        QPushButton *back, *forward;
        QtSegmentControl *styleSelector;
        GcToolBar *head;

        // the icons
        QIcon backIcon, forwardIcon, sidebarIcon, lowbarIcon, tabbedIcon, tiledIcon;

        // tab bar (that supports swtitching on drag and drop)
        DragBar *tabbar;
        QStackedWidget *viewStack, *tabStack;

        // window and tab menu
        QMenu *openTabMenu;
        QSignalMapper *tabMapper;

        // upload and sync menu
        QMenu *uploadMenu, *syncMenu, *measuresMenu;

        // backup
        QMenu *backupAthleteMenu;
        QSignalMapper *backupMapper;

        // chart menus
        QMenu *chartMenu;
        QMenu *subChartMenu;

        // Toolbar state checkables in View menu / context
        QAction *styleAction;
        QAction *showhideLowbar;
        QAction *showhideToolbar;
        QAction *showhideTabbar;

        QAction *shareAction;
        QAction *checkAction;

        // Miscellany
        QSignalMapper *toolMapper;

#ifdef GC_WANT_PYTHON
        QMenu *pyFixesMenu;
#endif

#ifdef GC_HAS_CLOUD_DB
        CloudDBVersionClient *versionClient;
        CloudDBTelemetryClient *telemetryClient;
#endif

};

#endif // _GC_MainWindow_h
