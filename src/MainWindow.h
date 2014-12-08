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

#ifdef Q_OS_MAC
// What versions are supported by this SDK?
#include <AvailabilityMacros.h>
#endif

class QTFullScreen;
class QtMacSegmentedButton;
class QtMacButton;
class GcToolBar;
class GcScopeBar;
class Library;
class QtSegmentControl;
class SaveSingleDialogWidget;
class ChooseCyclistDialog;
class SearchFilterBox;

class MainWindow;
class Athlete;
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

    protected:

        // used by ChooseCyclistDialog to see which athletes
        // have already been opened
        friend class ::ChooseCyclistDialog;
        QMap<QString,Tab*> tabs;

        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);

    public slots:

        bool eventFilter(QObject*,QEvent*);

        // GUI
#ifndef Q_OS_MAC
        void toggleFullScreen();
#endif
        void aboutDialog();
        void helpView();
        void logBug();
        void support();
        void actionClicked(int);

        // open and closing windows and tabs
        void closeAll();    // close all windows and tabs

        void setOpenWindowMenu(); // set the Open Window menu
        void newCyclistWindow();  // create a new Cyclist
        void openWindow(QString name);
        void closeWindow();

        void setOpenTabMenu(); // set the Open Tab menu
        void newCyclistTab();  // create a new Cyclist
        void openTab(QString name);
        void closeTabClicked(int index); // user clicked to close tab
        bool closeTab();       // close current, might not if the user 
                               // changes mind if there are unsaved changes.
        void removeTab(Tab*);  // remove without question

        void switchTab(int index); // for switching between one tab and another

        // Search / Filter
        void setFilter(QStringList);
        void clearFilter();

        void selectHome();
        void selectDiary();
        void selectAnalysis();
        void selectTrain();
        void selectInterval();

        void setChartMenu();
        void setSubChartMenu();
        void setChartMenu(QMenu *);
        void addChart(QAction*);

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
        void showTools();
        void showRhoEstimator();

        // Training View
        void addDevice();
        void downloadErgDB();
        void manageLibrary();
        void showWorkoutWizard();
        void importWorkout();

        // Diary View
        void refreshCalendar();
#ifdef GC_HAVE_ICAL
        void uploadCalendar(); // upload ride to calendar
#endif

        // Measures View
        void downloadMeasures();

        // Activity Collection
        void addIntervals(); // pass thru to tab
        bool saveRideSingleDialog(Context *, RideItem *);
        void saveSilent(Context *, RideItem *);
        void downloadRide();
        void manualRide();
        void exportRide();
        void exportBatch();
        void generateHeatMap();
        void exportMetrics();
#ifdef GC_HAVE_LIBOAUTH
        void tweetRide();
#endif
        void share();
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
        bool saveRideExitDialog(Context *);              // save dirty rides on exit dialog

        // autoload rides from athlete specific directory (preferences)
        void ridesAutoImport();

        // save and restore state to context
        void saveGCState(Context *);
        void restoreGCState(Context *);

        void configChanged();

    private:

        GcScopeBar *scopebar;
        Tab *currentTab;
        QList<Tab*> tabList;

#ifndef Q_OS_MAC
        QTFullScreen *fullScreen;
#endif

#ifdef GC_HAVE_LUCENE
        SearchFilterBox *searchBox;
#endif

#ifdef Q_OS_MAC
        // Mac Native Support
        QtMacButton *import, *compose, *sidebar, *lowbar;
        QtMacSegmentedButton *actbuttons, *styleSelector;
        QToolBar *head;
#else
        // Not on Mac so use other types
        QPushButton *import, *compose, *sidebar, *lowbar;
        QtSegmentControl *actbuttons, *styleSelector;
        GcToolBar *head;

        // the icons
        QIcon importIcon, composeIcon, intervalIcon, splitIcon,
              deleteIcon, sidebarIcon, lowbarIcon, tabbedIcon, tiledIcon;
#endif
        // tab bar (that supports swtitching on drag and drop)
        DragBar *tabbar;
        QStackedWidget *tabStack;

        // window and tab menu
        QMenu *openWindowMenu, *openTabMenu;
        QSignalMapper *windowMapper, *tabMapper;

        // chart menus
        QMenu *chartMenu;
        QMenu *subChartMenu;

        // Toolbar state checkables in View menu / context
        QAction *styleAction;
        QAction *showhideSidebar;
        QAction *showhideLowbar;
        QAction *showhideToolbar;
        QAction *showhideTabbar;

        QAction *tweetAction;
        QAction *shareAction;

        // Miscellany
        QSignalMapper *toolMapper;

#if (defined Q_OS_MAC) && (QT_VERSION >= 0x50201)
        QWidget *blackline;
#endif
};

#endif // _GC_MainWindow_h
