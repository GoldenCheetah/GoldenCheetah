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

class LionFullScreen;
class QTFullScreen;
class QtMacSegmentedButton;
class QtMacButton;
class GcToolBar;
class GcScopeBar;
class Library;
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

        // temporary access to the context
        Context *contextmain() { return context; } // by ChooseCyclistDialog
        void byebye() { close(); } // go bye bye for a restart

    protected:

        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);

    public slots:

        // GUI
#ifndef Q_OS_MAC
        void toggleFullScreen();
#endif
        void aboutDialog();
        void helpView();
        void logBug();
        void closeAll();
        void actionClicked(int);

        // Search / Filter
        void setFilter(QStringList);
        void clearFilter();


        void selectHome();
        void selectDiary();
        void selectAnalysis();
        void selectTrain();

        void setChartMenu();
        void setSubChartMenu();
        void addChart(QAction*);
        void setWindowMenu();
        void selectWindow(QAction*);

        void showOptions();

        void toggleSidebar();
        void showSidebar(bool want);
        void showToolbar(bool want);
        void resetWindowLayout();
        void toggleStyle();
        void setToolButtons(); // set toolbar buttons to match tabview
        void setStyleFromSegment(int); // special case for linux/win qtsegmentcontrol toggline
        void toggleLowbar();
        void showLowbar(bool want);

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
        void refreshCalendar();
#ifdef GC_HAVE_ICAL
        void uploadCalendar(); // upload ride to calendar
#endif

        // Measures View
        void downloadMeasures();
        void downloadMeasuresFromZeo();

        // Athlete Collection
        void newCyclist();
        void openCyclist();

        // Activity Collection
        void addIntervals(); // pass thru to tab
        void rideSelected(RideItem*ride);
        bool saveRideSingleDialog(RideItem *);
        void saveSilent(RideItem *);
        void downloadRide();
        void manualRide();
        void exportRide();
        void exportBatch();
        void exportMetrics();
#ifdef GC_HAVE_LIBOAUTH
        void tweetRide();
        void share();
#endif
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
        GcScopeBar *scopebar;
        Tab *tab;

#if (defined Q_OS_MAC) && (defined GC_HAVE_LION)
        LionFullScreen *fullScreen;
#endif
#ifndef Q_OS_MAC
        QTFullScreen *fullScreen;
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

        // chart menus
        QMenu *chartMenu;
        QMenu *subChartMenu;

        // Application menu
        QMenu *windowMenu;

        // Application actions
        // only keeping those used outside of mainwindow constructor
        QAction *styleAction;
        QAction *showhideSidebar;
        QAction *showhideLowbar;
        QAction *tweetAction;
        QAction *shareAction;
        QAction *ttbAction;

        // Miscellany
        QSignalMapper *toolMapper;
};

#endif // _GC_MainWindow_h
