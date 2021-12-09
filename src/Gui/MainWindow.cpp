/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

// QT
#include <QApplication>
#include <QtGui>
#include <QRegExp>
#include <QDesktopWidget>
#include <QNetworkProxyQuery>
#include <QMenuBar>
#include <QStyle>
#include <QTabBar>
#include <QStyleFactory>
#include <QRect>

// DATA STRUCTURES
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "AthleteView.h"
#include "AthleteBackup.h"

#include "Colors.h"
#include "RideCache.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "Settings.h"
#include "ErgDB.h"
#include "TodaysPlanWorkoutDownload.h"
#include "Library.h"
#include "LibraryParser.h"
#include "TrainDB.h"
#include "GcUpgrade.h"
#include "HelpWhatsThis.h"
#include "CsvRideFile.h"

// DIALOGS / DOWNLOADS / UPLOADS
#include "AboutDialog.h"
#include "ChooseCyclistDialog.h"
#include "ConfigDialog.h"
#include "AthleteConfigDialog.h"
#include "DownloadRideDialog.h"
#include "ManualRideDialog.h"
#include "RideImportWizard.h"
#include "EstimateCPDialog.h"
#include "SolveCPDialog.h"
#include "ToolsRhoEstimator.h"
#include "VDOTCalculator.h"
#include "SplitActivityWizard.h"
#include "MergeActivityWizard.h"
#include "GenerateHeatMapDialog.h"
#include "BatchExportDialog.h"
#include "TodaysPlan.h"
#include "MeasuresDownload.h"
#include "WorkoutWizard.h"
#include "ErgDBDownloadDialog.h"
#include "AddDeviceWizard.h"
#include "Dropbox.h"
#include "GoogleDrive.h"
#include "KentUniversity.h"
#include "SixCycle.h"
#include "OpenData.h"
#include "AddCloudWizard.h"
#include "LocalFileStore.h"
#include "CloudService.h"
#ifdef GC_WANT_PYTHON
#include "FixPyScriptsDialog.h"
#endif

// GUI Widgets
#include "AthleteTab.h"
#include "GcToolBar.h"
#include "NewSideBar.h"
#include "HelpWindow.h"
#include "Perspective.h"
#include "PerspectiveDialog.h"

#if !defined(Q_OS_MAC)
#include "QTFullScreen.h" // not mac!
#endif

// SEARCH / FILTER
#include "NamedSearch.h"
#include "SearchFilterBox.h"

// LTM CHART DRAG/DROP PARSE
#include "LTMChartParser.h"

// CloudDB
#ifdef GC_HAS_CLOUD_DB
#include "CloudDBCommon.h"
#include "CloudDBChart.h"
#include "CloudDBUserMetric.h"
#include "CloudDBCurator.h"
#include "CloudDBStatus.h"
#include "CloudDBTelemetry.h"
#include "CloudDBVersion.h"
#include "GcUpgrade.h"
#endif
#include "Secrets.h"

#if defined(_MSC_VER) && defined(_WIN64)
#include "WindowsCrashHandler.cpp"
#endif


// We keep track of all theopen mainwindows
QList<MainWindow *> mainwindows;
extern QDesktopWidget *desktop;
extern ConfigDialog *configdialog_ptr;
extern QString gl_version;
extern double gl_major; // 1.x 2.x 3.x - we insist on 2.x or higher to enable OpenGL

// constants for gui
static int gl_toolheight=28;

MainWindow::MainWindow(const QDir &home)
{
    /*----------------------------------------------------------------------
     *  Bootstrap
     *--------------------------------------------------------------------*/
    setAttribute(Qt::WA_DeleteOnClose);
    mainwindows.append(this);  // add us to the list of open windows
    pactive = init = false;

    // create a splash to keep user informed on first load
    // first one in middle of display, not middle of window
    setSplash(true);

#if defined(_MSC_VER) && defined(_WIN64)
    // set dbg/stacktrace directory for Windows to the athlete directory
    // don't use the GC_HOMEDIR .ini value, since we want to have a proper path
    // even if default athlete dirs are used.
    QDir varHome = home;
    varHome.cdUp();
    setCrashFilePath(varHome.canonicalPath().toStdString());

#endif

    // bootstrap
    Context *context = new Context(this);
    context->athlete = new Athlete(context, home);
    currentAthleteTab = new AthleteTab(context);

    // get rid of splash when currentTab is shown
    clearSplash();

    setWindowIcon(QIcon(":images/gc.png"));
    setWindowTitle(context->athlete->home->root().dirName());
    setContentsMargins(0,0,0,0);
    setAcceptDrops(true);

    Library::initialise(context->athlete->home->root());
    QNetworkProxyQuery npq(QUrl("http://www.google.com"));
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
    if (listOfProxies.count() > 0) {
        QNetworkProxy::setApplicationProxy(listOfProxies.first());
    }

#ifndef Q_OS_MAC
    fullScreen = new QTFullScreen(this);
#endif

    // if no workout directory is configured, default to the
    // top level GoldenCheetah directory
    if (appsettings->value(NULL, GC_WORKOUTDIR).toString() == "")
        appsettings->setValue(GC_WORKOUTDIR, QFileInfo(context->athlete->home->root().canonicalPath() + "/../").canonicalPath());

    /*----------------------------------------------------------------------
     *  GUI setup
     *--------------------------------------------------------------------*/
     if (appsettings->contains(GC_SETTINGS_MAIN_GEOM)) {
         restoreGeometry(appsettings->value(this, GC_SETTINGS_MAIN_GEOM).toByteArray());
         restoreState(appsettings->value(this, GC_SETTINGS_MAIN_STATE).toByteArray());
     } else {
         // first run -- lets set some sensible defaults...
         // lets put it in the middle of screen 1
        QRect screenSize = desktop->availableGeometry();
         struct SizeSettings app = GCColor::defaultSizes(screenSize.height(), screenSize.width());

         // center on the available screen (minus toolbar/sidebar)
         move((screenSize.width()-screenSize.x())/2 - app.width/2,
              (screenSize.height()-screenSize.y())/2 - app.height/2);

         // set to the right default
         resize(app.width, app.height);

         // set all the default font sizes
         appsettings->setValue(GC_FONT_DEFAULT_SIZE, app.defaultFont);
         appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, app.labelFont);

     }

     // store "last_openend" athlete for next time
     appsettings->setValue(GC_SETTINGS_LAST, context->athlete->home->root().dirName());

    /*----------------------------------------------------------------------
     * ScopeBar as sidebar from v3.6
     *--------------------------------------------------------------------*/

    sidebar = new NewSideBar(context, this);
    HelpWhatsThis *helpNewSideBar = new HelpWhatsThis(sidebar);
    sidebar->setWhatsThis(helpNewSideBar->getWhatsThisText(HelpWhatsThis::ScopeBar));

    sidebar->addItem(QImage(":sidebar/athlete.png"), tr("athletes"), 0, helpNewSideBar->getWhatsThisText(HelpWhatsThis::ScopeBar_Athletes));
    sidebar->setItemEnabled(1, false);

    sidebar->addItem(QImage(":sidebar/plan.png"), tr("plan"), 1), tr("Feature not implemented yet");
    sidebar->setItemEnabled(1, false);

    sidebar->addItem(QImage(":sidebar/trends.png"), tr("trends"), 2, helpNewSideBar->getWhatsThisText(HelpWhatsThis::ScopeBar_Trends));
    sidebar->addItem(QImage(":sidebar/assess.png"), tr("activities"), 3, helpNewSideBar->getWhatsThisText(HelpWhatsThis::ScopeBar_Rides));
    sidebar->setItemSelected(3, true);

    sidebar->addItem(QImage(":sidebar/reflect.png"), tr("reflect"), 4), tr("Feature not implemented yet");
    sidebar->setItemEnabled(4, false);

    sidebar->addItem(QImage(":sidebar/train.png"), tr("train"), 5, helpNewSideBar->getWhatsThisText(HelpWhatsThis::ScopeBar_Train));

    sidebar->addStretch();
    sidebar->addItem(QImage(":sidebar/apps.png"), tr("apps"), 6, tr("Feature not implemented yet"));
    sidebar->setItemEnabled(6, false);
    sidebar->addStretch();

    // we can click on the quick icons, but they aren't selectable views
    sidebar->addItem(QImage(":sidebar/sync.png"), tr("sync"), 7, helpNewSideBar->getWhatsThisText(HelpWhatsThis::ScopeBar_Sync));
    sidebar->setItemSelectable(7, false);
    sidebar->addItem(QImage(":sidebar/prefs.png"), tr("options"), 8, helpNewSideBar->getWhatsThisText(HelpWhatsThis::ScopeBar_Options));
    sidebar->setItemSelectable(8, false);

    connect(sidebar, SIGNAL(itemClicked(int)), this, SLOT(sidebarClicked(int)));
    connect(sidebar, SIGNAL(itemSelected(int)), this, SLOT(sidebarSelected(int)));

    /*----------------------------------------------------------------------
     * What's this Context Help
     *--------------------------------------------------------------------*/

    // Help for the whole window
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Default));
    // add Help Button
    QAction *myHelper = QWhatsThis::createAction (this);
    this->addAction(myHelper);


    /*----------------------------------------------------------------------
     *  Toolbar
     *--------------------------------------------------------------------*/
    head = new GcToolBar(this);

    QStyle *toolStyle = QStyleFactory::create("fusion");

    // get those icons
    sidebarIcon = iconFromPNG(":images/titlebar/sidebar.png");
    lowbarIcon = iconFromPNG(":images/titlebar/bottombar.png");
    tiledIcon = iconFromPNG(":images/titlebar/tile.png");
    backIcon = iconFromPNG(":images/mac/back.png");
    whatIcon = iconFromPNG(":images/titlebar/whatsthis.png");
    forwardIcon = iconFromPNG(":images/mac/forward.png");
    QSize isize(16 *dpiXFactor,16 *dpiYFactor);

    back = new QPushButton(this);
    back->setIcon(backIcon);
    back->setFixedHeight(gl_toolheight *dpiYFactor);
    back->setFixedWidth(gl_toolheight *dpiYFactor);
    back->setIconSize(isize);
    back->setStyle(toolStyle);
    connect(back, SIGNAL(clicked(bool)), this, SIGNAL(backClicked()));

    forward = new QPushButton(this);
    forward->setIcon(forwardIcon);
    forward->setFixedHeight(gl_toolheight *dpiYFactor);
    forward->setFixedWidth(gl_toolheight *dpiYFactor);
    forward->setIconSize(isize);
    forward->setStyle(toolStyle);
    connect(forward, SIGNAL(clicked(bool)), this, SIGNAL(forwardClicked()));

    lowbar = new QPushButton(this);
    lowbar->setIcon(lowbarIcon);
    lowbar->setFixedHeight(gl_toolheight *dpiYFactor);
    lowbar->setFixedWidth(gl_toolheight *dpiYFactor);
    lowbar->setIconSize(isize);
    lowbar->setStyle(toolStyle);
    lowbar->setToolTip(tr("Toggle Compare Pane"));
    connect(lowbar, SIGNAL(clicked(bool)), this, SLOT(toggleLowbar()));
    HelpWhatsThis *helpLowBar = new HelpWhatsThis(lowbar);
    lowbar->setWhatsThis(helpLowBar->getWhatsThisText(HelpWhatsThis::ToolBar_ToggleComparePane));

    sidelist = new QPushButton(this);
    sidelist->setIcon(sidebarIcon);
    sidelist->setFixedHeight(gl_toolheight * dpiYFactor);
    sidelist->setFixedWidth(gl_toolheight *dpiYFactor);
    sidelist->setIconSize(isize);
    sidelist->setStyle(toolStyle);
    sidelist->setToolTip(tr("Toggle Sidebar"));
    connect(sidelist, SIGNAL(clicked(bool)), this, SLOT(toggleSidebar()));
    HelpWhatsThis *helpSideBar = new HelpWhatsThis(sidelist);
    sidelist->setWhatsThis(helpSideBar->getWhatsThisText(HelpWhatsThis::ToolBar_ToggleSidebar));

    tabtile = new QPushButton(this);
    tabtile->setIcon(tiledIcon);
    tabtile->setFixedHeight(gl_toolheight *dpiYFactor);
    tabtile->setFixedWidth(gl_toolheight *dpiYFactor);
    tabtile->setIconSize(isize);
    lowbar->setStyle(toolStyle);
    tabtile->setToolTip(tr("Toggle Tab/Tile"));
    connect(tabtile, SIGNAL(clicked(bool)), this, SLOT(toggleStyle()));

    // What's this button
    whatsthis = new QPushButton(this);
    whatsthis->setIcon(whatIcon);
    whatsthis->setFixedHeight(gl_toolheight * dpiYFactor);
    whatsthis->setFixedWidth(gl_toolheight * dpiYFactor);
    whatsthis->setIconSize(isize);
    whatsthis->setStyle(toolStyle);
    whatsthis->setToolTip(tr("What's This?"));
    connect(whatsthis, SIGNAL(clicked(bool)), this, SLOT(enterWhatsThisMode()));

    // add a search box on far right, but with a little space too
    perspectiveSelector = new QComboBox(this);
    perspectiveSelector->setStyle(toolStyle);
    perspectiveSelector->setFixedWidth(200 * dpiXFactor);
    perspectiveSelector->setFixedHeight(gl_toolheight * dpiYFactor);
    connect(perspectiveSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(perspectiveSelected(int)));

    searchBox = new SearchFilterBox(this,context,false);

    searchBox->setStyle(toolStyle);
    searchBox->setFixedWidth(400 * dpiXFactor);
    searchBox->setFixedHeight(gl_toolheight * dpiYFactor);

    QWidget *space = new QWidget(this);
    space->setAutoFillBackground(false);
    space->setFixedWidth(5 * dpiYFactor);

    head->addWidget(space);
    head->addWidget(back);
    head->addWidget(forward);
    head->addWidget(perspectiveSelector);
    head->addStretch();
    head->addWidget(sidelist);
    head->addWidget(lowbar);
    head->addWidget(tabtile);
#ifdef Q_OS_MAC // no menu on mac, so lets have some breathing space
    head->setFixedHeight(searchBox->height() + (20 *dpiXFactor * 2));
#else
    head->setFixedHeight(searchBox->height() + (16 *dpiXFactor));
#endif

    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
    HelpWhatsThis *helpSearchBox = new HelpWhatsThis(searchBox);
    searchBox->setWhatsThis(helpSearchBox->getWhatsThisText(HelpWhatsThis::SearchFilterBox));

    space = new Spacer(this);
    space->setFixedWidth(5 *dpiYFactor);
    head->addWidget(space);
    head->addWidget(searchBox);
    space = new Spacer(this);
    space->setFixedWidth(5 *dpiYFactor);
    head->addWidget(space);
    head->addWidget(whatsthis);
    space = new Spacer(this);
    space->setFixedWidth(5 *dpiYFactor);
    head->addWidget(space);

#ifdef Q_OS_LINUX
    // check opengl is available with version 2 or higher
    // only do this on Linux since Windows and MacOS have opengl "issues"
    QOffscreenSurface surf;
    surf.create();

    QOpenGLContext ctx;
    ctx.create();
    ctx.makeCurrent(&surf);

    // OpenGL version number
    gl_version = QString::fromUtf8((char *)(ctx.functions()->glGetString(GL_VERSION)));
    gl_major = Utils::number(gl_version);
#endif

    /*----------------------------------------------------------------------
     * Central Widget
     *--------------------------------------------------------------------*/

    tabbar = new DragBar(this);
    tabbar->setTabsClosable(false); // use athlete view
#ifdef Q_OS_MAC
    tabbar->setDocumentMode(true);
#endif

    athleteView = new AthleteView(context);
    viewStack = new QStackedWidget(this);
    viewStack->addWidget(athleteView);

    tabStack = new QStackedWidget(this);
    viewStack->addWidget(tabStack);

    // first tab
    athletetabs.insert(currentAthleteTab->context->athlete->home->root().dirName(), currentAthleteTab);

    // stack, list and bar all share a common index
    tabList.append(currentAthleteTab);
    tabbar->addTab(currentAthleteTab->context->athlete->home->root().dirName());
    tabStack->addWidget(currentAthleteTab);
    tabStack->setCurrentIndex(0);

    connect(tabbar, SIGNAL(dragTab(int)), this, SLOT(switchAthleteTab(int)));
    connect(tabbar, SIGNAL(currentChanged(int)), this, SLOT(switchAthleteTab(int)));
    //connect(tabbar, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTabClicked(int))); // use athlete view

    /*----------------------------------------------------------------------
     * Central Widget
     *--------------------------------------------------------------------*/

    QWidget *central = new QWidget(this);
    setContentsMargins(0,0,0,0);
    central->setContentsMargins(0,0,0,0);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->addWidget(head);
    QHBoxLayout *lrlayout = new QHBoxLayout();
    mainLayout->addLayout(lrlayout);
    lrlayout->setSpacing(0);
    lrlayout->setContentsMargins(0,0,0,0);
    lrlayout->addWidget(sidebar);
    QVBoxLayout *tablayout = new QVBoxLayout();
    tablayout->setSpacing(0);
    tablayout->setContentsMargins(0,0,0,0);
    lrlayout->addLayout(tablayout);
    tablayout->addWidget(tabbar);
    tablayout->addWidget(viewStack);
    setCentralWidget(central);

    /*----------------------------------------------------------------------
     * Application Menus
     *--------------------------------------------------------------------*/
#ifdef WIN32
    QString menuColorString = (GCColor::isFlat() ? GColor(CTOOLBAR).name() : "rgba(225,225,225)");
    menuBar()->setStyleSheet(QString("QMenuBar { color: black; background: %1; }"
                             "QMenuBar::item { color: black; background: %1; }").arg(menuColorString));
    menuBar()->setContentsMargins(0,0,0,0);
#endif

    // ATHLETE (FILE) MENU
    QMenu *fileMenu = menuBar()->addMenu(tr("&Athlete"));

    openTabMenu = fileMenu->addMenu(tr("Open..."));
    connect(openTabMenu, SIGNAL(aboutToShow()), this, SLOT(setOpenTabMenu()));

    tabMapper = new QSignalMapper(this); // maps each option
    connect(tabMapper, SIGNAL(mapped(const QString &)), this, SLOT(openAthleteTab(const QString &)));

    fileMenu->addSeparator();
    backupAthleteMenu = fileMenu->addMenu(tr("Backup..."));
    connect(backupAthleteMenu, SIGNAL(aboutToShow()), this, SLOT(setBackupAthleteMenu()));
    backupMapper = new QSignalMapper(this); // maps each option
    connect(backupMapper, SIGNAL(mapped(const QString &)), this, SLOT(backupAthlete(const QString &)));

    fileMenu->addSeparator();
    deleteAthleteMenu = fileMenu->addMenu(tr("Delete..."));
    connect(deleteAthleteMenu, SIGNAL(aboutToShow()), this, SLOT(setDeleteAthleteMenu()));
    deleteMapper = new QSignalMapper(this); // maps each option
    connect(deleteMapper, SIGNAL(mapped(const QString &)), this, SLOT(deleteAthlete(const QString &)));

    fileMenu->addSeparator();
    fileMenu->addAction(tr("Save all modified activities"), this, SLOT(saveAllUnsavedRides()));
    fileMenu->addAction(tr("Settings..."), this, SLOT(athleteSettings()));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Close Window"), this, SLOT(closeWindow()));
    //fileMenu->addAction(tr("&Close Tab"), this, SLOT(closeTab())); use athlete view

    HelpWhatsThis *fileMenuHelp = new HelpWhatsThis(fileMenu);
    fileMenu->setWhatsThis(fileMenuHelp->getWhatsThisText(HelpWhatsThis::MenuBar_Athlete));

    // ACTIVITY MENU
    QMenu *rideMenu = menuBar()->addMenu(tr("A&ctivity"));
    rideMenu->addAction(tr("&Download from device..."), this, SLOT(downloadRide()), tr("Ctrl+D"));
    rideMenu->addAction(tr("&Import from file..."), this, SLOT (importFile()), tr ("Ctrl+I"));
    rideMenu->addAction(tr("&Manual entry..."), this, SLOT(manualRide()), tr("Ctrl+M"));
    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Export..."), this, SLOT(exportRide()), tr("Ctrl+E"));
    rideMenu->addAction(tr("&Batch export..."), this, SLOT(exportBatch()), tr("Ctrl+B"));

    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Save activity"), this, SLOT(saveRide()), tr("Ctrl+S"));
    rideMenu->addAction(tr("D&elete activity..."), this, SLOT(deleteRide()));
    rideMenu->addAction(tr("Split &activity..."), this, SLOT(splitRide()));
    rideMenu->addAction(tr("Combine activities..."), this, SLOT(mergeRide()));
    rideMenu->addSeparator ();
    rideMenu->addAction(tr("Find intervals..."), this, SLOT(addIntervals()), tr (""));

    HelpWhatsThis *helpRideMenu = new HelpWhatsThis(rideMenu);
    rideMenu->setWhatsThis(helpRideMenu->getWhatsThisText(HelpWhatsThis::MenuBar_Activity));

    // SHARE MENU
    QMenu *shareMenu = menuBar()->addMenu(tr("Sha&re"));

    // default options
    shareAction = new QAction(tr("Add Cloud Account..."), this);
    shareAction->setShortcut(tr("Ctrl+A"));
    connect(shareAction, SIGNAL(triggered(bool)), this, SLOT(addAccount()));
    shareMenu->addAction(shareAction);
    shareMenu->addSeparator();

    uploadMenu = shareMenu->addMenu(tr("Upload Activity..."));
    syncMenu = shareMenu->addMenu(tr("Synchronise Activities..."));
    measuresMenu = shareMenu->addMenu(tr("Get Measures..."));
    shareMenu->addSeparator();
    checkAction = new QAction(tr("Check For New Activities"), this);
    checkAction->setShortcut(tr("Ctrl-C"));
    connect(checkAction, SIGNAL(triggered(bool)), this, SLOT(checkCloud()));
    shareMenu->addAction(checkAction);


    // set the menus to reflect the configured accounts
    connect(uploadMenu, SIGNAL(aboutToShow()), this, SLOT(setUploadMenu()));
    connect(syncMenu, SIGNAL(aboutToShow()), this, SLOT(setSyncMenu()));

    connect(uploadMenu, SIGNAL(triggered(QAction*)), this, SLOT(uploadCloud(QAction*)));
    connect(syncMenu, SIGNAL(triggered(QAction*)), this, SLOT(syncCloud(QAction*)));
    connect(measuresMenu, SIGNAL(aboutToShow()), this, SLOT(setMeasuresMenu()));
    connect(measuresMenu, SIGNAL(triggered(QAction*)), this, SLOT(downloadMeasures(QAction*)));

    HelpWhatsThis *helpShare = new HelpWhatsThis(shareMenu);
    shareMenu->setWhatsThis(helpShare->getWhatsThisText(HelpWhatsThis::MenuBar_Share));

    // TOOLS MENU
    QMenu *optionsMenu = menuBar()->addMenu(tr("&Tools"));
    optionsMenu->addAction(tr("CP and W' Estimator..."), this, SLOT(showEstimateCP()));
    optionsMenu->addAction(tr("CP and W' Solver..."), this, SLOT(showSolveCP()));
    optionsMenu->addAction(tr("Air Density (Rho) Estimator..."), this, SLOT(showRhoEstimator()));
    optionsMenu->addAction(tr("VDOT and T-Pace Calculator..."), this, SLOT(showVDOTCalculator()));

    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Create a new workout..."), this, SLOT(showWorkoutWizard()));
    optionsMenu->addAction(tr("Download workouts from ErgDB..."), this, SLOT(downloadErgDB()));
    optionsMenu->addAction(tr("Download workouts from Today's Plan..."), this, SLOT(downloadTodaysPlanWorkouts()));
    optionsMenu->addAction(tr("Import workouts, videos, videoSyncs..."), this, SLOT(importWorkout()));
    optionsMenu->addAction(tr("Scan disk for workouts, videos, videoSyncs..."), this, SLOT(manageLibrary()));

    optionsMenu->addAction(tr("Create Heat Map..."), this, SLOT(generateHeatMap()), tr(""));
    optionsMenu->addAction(tr("Export Metrics as CSV..."), this, SLOT(exportMetrics()), tr(""));

#ifdef GC_HAS_CLOUD_DB
    // CloudDB options
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Cloud Status..."), this, SLOT(cloudDBshowStatus()));

    QMenu *cloudDBMenu = optionsMenu->addMenu(tr("Cloud Contributions"));
    cloudDBMenu->addAction(tr("Maintain charts"), this, SLOT(cloudDBuserEditChart()));
    cloudDBMenu->addAction(tr("Maintain user metrics"), this, SLOT(cloudDBuserEditUserMetric()));

    if (CloudDBCommon::addCuratorFeatures) {
        QMenu *cloudDBCurator = optionsMenu->addMenu(tr("Cloud Curator"));
        cloudDBCurator->addAction(tr("Curate charts"), this, SLOT(cloudDBcuratorEditChart()));
        cloudDBCurator->addAction(tr("Curate user metrics"), this, SLOT(cloudDBcuratorEditUserMetric()));
    }

#endif
#ifndef Q_OS_MAC
    optionsMenu->addSeparator();
#endif
    // options are always at the end of the tools menu
    QAction *pref = optionsMenu->addAction(tr("&Options..."), this, SLOT(showOptions()));
    pref->setMenuRole(QAction:: PreferencesRole);

    HelpWhatsThis *optionsMenuHelp = new HelpWhatsThis(optionsMenu);
    optionsMenu->setWhatsThis(optionsMenuHelp->getWhatsThisText(HelpWhatsThis::MenuBar_Tools));


    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    // Add all the data processors to the tools menu
    const DataProcessorFactory &factory = DataProcessorFactory::instance();
    QMap<QString, DataProcessor*> processors = factory.getProcessors(true);

    if (processors.count()) {

        toolMapper = new QSignalMapper(this); // maps each option
        QMapIterator<QString, DataProcessor*> i(processors);
        connect(toolMapper, SIGNAL(mapped(const QString &)), this, SLOT(manualProcess(const QString &)));

        i.toFront();
        while (i.hasNext()) {
            i.next();
            // The localized processor name is shown in menu
            QAction *action = new QAction(QString("%1...").arg(i.value()->name()), this);
            editMenu->addAction(action);
            connect(action, SIGNAL(triggered()), toolMapper, SLOT(map()));
            toolMapper->setMapping(action, i.key());
        }
    }

#ifdef GC_WANT_PYTHON
    // add custom python fix entry to edit menu
    pyFixesMenu = editMenu->addMenu(tr("Python fixes"));
    connect(editMenu, SIGNAL(aboutToShow()), this, SLOT(onEditMenuAboutToShow()));
    connect(pyFixesMenu, SIGNAL(aboutToShow()), this, SLOT(buildPyFixesMenu()));
#endif

    HelpWhatsThis *editMenuHelp = new HelpWhatsThis(editMenu);
    editMenu->setWhatsThis(editMenuHelp->getWhatsThisText(HelpWhatsThis::MenuBar_Edit));

    // VIEW MENU
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
#ifndef Q_OS_MAC
    viewMenu->addAction(tr("Toggle Full Screen"), this, SLOT(toggleFullScreen()), QKeySequence("F11"));
#else
    viewMenu->addAction(tr("Toggle Full Screen"), this, SLOT(toggleFullScreen()));
#endif
    showhideViewbar = viewMenu->addAction(tr("Show View Sidebar"), this, SLOT(showViewbar(bool)));
    showhideViewbar->setCheckable(true);
    showhideViewbar->setChecked(true);
    showhideSidebar = viewMenu->addAction(tr("Show Left Sidebar"), this, SLOT(showSidebar(bool)));
    showhideSidebar->setCheckable(true);
    showhideSidebar->setChecked(true);
    showhideLowbar = viewMenu->addAction(tr("Show Compare Pane"), this, SLOT(showLowbar(bool)));
    showhideLowbar->setCheckable(true);
    showhideLowbar->setChecked(false);
    showhideToolbar = viewMenu->addAction(tr("Show Toolbar"), this, SLOT(showToolbar(bool)));
    showhideToolbar->setCheckable(true);
    showhideToolbar->setChecked(true);
    showhideTabbar = viewMenu->addAction(tr("Show Athlete Tabs"), this, SLOT(showTabbar(bool)));
    showhideTabbar->setCheckable(true);
    showhideTabbar->setChecked(true);

    viewMenu->addSeparator();
    viewMenu->addAction(tr("Activities"), this, SLOT(selectAnalysis()));
    viewMenu->addAction(tr("Trends"), this, SLOT(selectTrends()));
    viewMenu->addAction(tr("Train"), this, SLOT(selectTrain()));
    viewMenu->addSeparator();
    viewMenu->addAction(tr("Import Perspective..."), this, SLOT(importPerspective()));
    viewMenu->addAction(tr("Export Perspective..."), this, SLOT(exportPerspective()));
    viewMenu->addSeparator();
    subChartMenu = viewMenu->addMenu(tr("Add Chart"));
    viewMenu->addAction(tr("Import Chart..."), this, SLOT(importChart()));
#ifdef GC_HAS_CLOUD_DB
    viewMenu->addAction(tr("Upload Chart..."), this, SLOT(exportChartToCloudDB()));
    viewMenu->addAction(tr("Download Chart..."), this, SLOT(addChartFromCloudDB()));
    viewMenu->addSeparator();
#endif
    viewMenu->addAction(tr("Reset Layout"), this, SLOT(resetWindowLayout()));
    styleAction = viewMenu->addAction(tr("Tabbed not Tiled"), this, SLOT(toggleStyle()));
    styleAction->setCheckable(true);
    styleAction->setChecked(true);


    connect(subChartMenu, SIGNAL(aboutToShow()), this, SLOT(setSubChartMenu()));
    connect(subChartMenu, SIGNAL(triggered(QAction*)), this, SLOT(addChart(QAction*)));

    HelpWhatsThis *viewMenuHelp = new HelpWhatsThis(viewMenu);
    viewMenu->setWhatsThis(viewMenuHelp->getWhatsThisText(HelpWhatsThis::MenuBar_View));

    // HELP MENU
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&Help Overview"), this, SLOT(helpWindow()));
    helpMenu->addAction(myHelper);
    helpMenu->addSeparator();
    helpMenu->addAction(tr("&User Guide"), this, SLOT(helpView()));
    helpMenu->addAction(tr("&Log a bug or feature request"), this, SLOT(logBug()));
    helpMenu->addAction(tr("&Discussion and Support Forum"), this, SLOT(support()));
    helpMenu->addSeparator();
    helpMenu->addAction(tr("&About GoldenCheetah"), this, SLOT(aboutDialog()));

    HelpWhatsThis *helpMenuHelp = new HelpWhatsThis(helpMenu);
    helpMenu->setWhatsThis(helpMenuHelp->getWhatsThisText(HelpWhatsThis::MenuBar_Help));

    /*----------------------------------------------------------------------
     * Lets go, choose latest ride and get GUI up and running
     *--------------------------------------------------------------------*/

    showTabbar(appsettings->value(NULL, GC_TABBAR, "0").toBool());

    //XXX!!! We really do need a mechanism for showing if a ride needs saving...
    //connect(this, SIGNAL(rideDirty()), this, SLOT(enableSaveButton()));
    //connect(this, SIGNAL(rideClean()), this, SLOT(enableSaveButton()));

    saveGCState(currentAthleteTab->context); // set to whatever we started with
    selectAnalysis();

    //grab focus
    currentAthleteTab->setFocus();

    installEventFilter(this);

    // catch global config changes
    connect(GlobalContext::context(), SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    configChanged(CONFIG_APPEARANCE);

    init = true;

    /*----------------------------------------------------------------------
     * Lets ask for telemetry and check for updates
     *--------------------------------------------------------------------*/

#if !defined(OPENDATA_DISABLE)
    OpenData::check(currentAthleteTab->context);
#else
    fprintf(stderr, "OpenData disabled, secret not defined.\n"); fflush(stderr);
#endif

#ifdef GC_HAS_CLOUD_DB
    telemetryClient = new CloudDBTelemetryClient();
    if (appsettings->value(NULL, GC_ALLOW_TELEMETRY, "undefined").toString() == "undefined" ) {
        // ask user if storing is allowed

        // check for Telemetry Storage acceptance
        CloudDBAcceptTelemetryDialog acceptDialog;
        acceptDialog.setModal(true);
        if (acceptDialog.exec() == QDialog::Accepted) {
            telemetryClient->upsertTelemetry();
        };
    } else if (appsettings->value(NULL, GC_ALLOW_TELEMETRY, false).toBool()) {
        telemetryClient->upsertTelemetry();
    }

    versionClient = new CloudDBVersionClient();
    versionClient->informUserAboutLatestVersions();



#endif

}


/*----------------------------------------------------------------------
 * GUI
 *--------------------------------------------------------------------*/

void
MainWindow::setSplash(bool first)
{
    // new frameless widget
    splash = new QWidget(NULL);

    // modal dialog with no parent so we set it up as a 'splash'
    // because QSplashScreen doesn't seem to work (!!)
    splash->setAttribute(Qt::WA_DeleteOnClose);
    splash->setWindowFlags(splash->windowFlags() | Qt::FramelessWindowHint);
#ifdef Q_OS_LINUX
    splash->setWindowFlags(splash->windowFlags() | Qt::X11BypassWindowManagerHint);
#endif

    // put widgets on it
    progress = new QLabel(splash);
    progress->setAlignment(Qt::AlignCenter);
    QHBoxLayout *l = new QHBoxLayout(splash);
    l->setSpacing(0);
    l->addWidget(progress);

    // lets go
    splash->setFixedSize(100 *dpiXFactor, 80 *dpiYFactor);

    if (first) {
        // middle of screen
        splash->move(desktop->availableGeometry().center()-QPoint(50, 25));
    } else {
        // middle of mainwindow is appropriate
        splash->move(geometry().center()-QPoint(50, 25));
    }
    splash->show();

    // reset the splash counter
    loading=1;
}

void
MainWindow::clearSplash()
{
    progress = NULL;
    splash->close();
}

void
MainWindow::toggleSidebar()
{
    currentAthleteTab->toggleSidebar();
    setToolButtons();
}
void
MainWindow::showViewbar(bool want)
{
    want ? sidebar->show() : sidebar->hide();
    showhideViewbar->setChecked(want);
    setToolButtons();
}
void
MainWindow::showSidebar(bool want)
{
    currentAthleteTab->setSidebarEnabled(want);
    showhideSidebar->setChecked(want);
    setToolButtons();
}

void
MainWindow::toggleLowbar()
{
    if (currentAthleteTab->hasBottom()) currentAthleteTab->setBottomRequested(!currentAthleteTab->isBottomRequested());
    setToolButtons();
}

void
MainWindow::showLowbar(bool want)
{
    if (currentAthleteTab->hasBottom()) currentAthleteTab->setBottomRequested(want);
    showhideLowbar->setChecked(want);
    setToolButtons();
}

void
MainWindow::enterWhatsThisMode()
{
    QWhatsThis::enterWhatsThisMode();
}

void
MainWindow::showTabbar(bool want)
{
    setUpdatesEnabled(false);
    showhideTabbar->setChecked(want);
    if (want) {
        tabbar->show();
    }
    else {
        tabbar->hide();
    }
    setUpdatesEnabled(true);
}

void
MainWindow::showToolbar(bool want)
{
    setUpdatesEnabled(false);
    showhideToolbar->setChecked(want);
    if (want) {
        head->show();
    }
    else {
        head->hide();
    }
    setUpdatesEnabled(true);
}

void
MainWindow::setChartMenu()
{
    unsigned int mask=0;

    // called when chart menu about to be shown
    // setup to only show charts that are relevant
    // to this view
    switch(currentAthleteTab->currentView()) {
        case 0 : mask = VIEW_TRENDS; break;
        default:
        case 1 : mask = VIEW_ANALYSIS; break;
        case 2 : mask = VIEW_DIARY; break;
        case 3 : mask = VIEW_TRAIN; break;
    }

    chartMenu->clear();
    if (!mask) return;

    for(int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].relevance & mask)
            chartMenu->addAction(GcWindows[i].name);
    }
}

void
MainWindow::setSubChartMenu()
{
    setChartMenu(subChartMenu);
}

void
MainWindow::setChartMenu(QMenu *menu)
{
    unsigned int mask=0;
    // called when chart menu about to be shown
    // setup to only show charts that are relevant
    // to this view
    switch(currentAthleteTab->currentView()) {
        case 0 : mask = VIEW_TRENDS; break;
        default:
        case 1 : mask = VIEW_ANALYSIS; break;
        case 2 : mask = VIEW_DIARY; break;
        case 3 : mask = VIEW_TRAIN; break;
    }

    menu->clear();
    if (!mask) return;

    for(int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].relevance & mask)
            menu->addAction(GcWindows[i].name);
    }
}

void
MainWindow::addChart(QAction*action)
{
    // & removed to avoid issues with kde AutoCheckAccelerators
    QString actionText = QString(action->text()).replace("&", "");
    GcWinID id = GcWindowTypes::None;
    for (int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].name == actionText) {
            id = GcWindows[i].id;
            break;
        }
    }
    if (id != GcWindowTypes::None)
        currentAthleteTab->addChart(id); // called from MainWindow to inset chart
}

void
MainWindow::importChart()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Chart file to import"), "", tr("GoldenCheetah Chart Files (*.gchart)"));

    if (fileName.isEmpty()) {
        QMessageBox::critical(this, tr("Import Chart"), tr("No chart file selected!"));
    } else {
        importCharts(QStringList()<<fileName);
    }
}

void
MainWindow::exportPerspective()
{
    int view = currentAthleteTab->currentView();
    AbstractView *current = NULL;

    QString typedesc;

    switch (view) {
    case 0:  current = currentAthleteTab->homeView; typedesc = "Trends"; break;
    case 1:  current = currentAthleteTab->analysisView; typedesc = "Analysis"; break;
    case 2:  current = currentAthleteTab->diaryView; typedesc = "Diary"; break;
    case 3:  current = currentAthleteTab->trainView; typedesc = "Train"; break;
    }

    // export the current perspective to a file
    QString suffix;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Persepctive"),
                       QDir::homePath()+"/"+ typedesc + " " + current->perspective_->title() + ".gchartset",
                       ("*.gchartset;;"), &suffix, QFileDialog::DontUseNativeDialog); // native dialog hangs when threads in use (!)

    if (fileName.isEmpty()) {
        QMessageBox::critical(this, tr("Export Perspective"), tr("No perspective file selected!"));
    } else {
        current->exportPerspective(current->perspective_, fileName);
    }
}

void
MainWindow::importPerspective()
{
    int view = currentAthleteTab->currentView();
    AbstractView *current = NULL;

    switch (view) {
    case 0:  current = currentAthleteTab->homeView; break;
    case 1:  current = currentAthleteTab->analysisView; break;
    case 2:  current = currentAthleteTab->diaryView; break;
    case 3:  current = currentAthleteTab->trainView; break;
    }

    // import a new perspective from a file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Perspective file to export"), "", tr("GoldenCheetah Perspective Files (*.gchartset)"));
    if (fileName.isEmpty()) {
        QMessageBox::critical(this, tr("Import Perspective"), tr("No perspective file selected!"));
    } else {

        // import and select it
        pactive = true;
        if (current->importPerspective(fileName)) {

            // on success we select the new one
            resetPerspective(view);
            //current->setPerspectives(perspectiveSelector);

            // and select remember pactive is true, so we do the heavy lifting here
            perspectiveSelector->setCurrentIndex(current->perspectives_.count()-1);
            current->perspectiveSelected(perspectiveSelector->currentIndex());
        }
        pactive = false;
    }

}

#ifdef GC_HAS_CLOUD_DB

void
MainWindow::exportChartToCloudDB()
{
    // upload the current chart selected to the chart db
    // called from the sidebar menu
    Perspective *page=currentAthleteTab->view(currentAthleteTab->currentView())->page();
    if (page->currentStyle == 0 && page->currentChart())
        page->currentChart()->exportChartToCloudDB();
}

void
MainWindow::addChartFromCloudDB()
{
    if (!(appsettings->cvalue(currentAthleteTab->context->athlete->cyclist, GC_CLOUDDB_TC_ACCEPTANCE, false).toBool())) {
       CloudDBAcceptConditionsDialog acceptDialog(currentAthleteTab->context->athlete->cyclist);
       acceptDialog.setModal(true);
       if (acceptDialog.exec() == QDialog::Rejected) {
          return;
       };
    }

    if (currentAthleteTab->context->cdbChartListDialog == NULL) {
        currentAthleteTab->context->cdbChartListDialog = new CloudDBChartListDialog();
    }

    if (currentAthleteTab->context->cdbChartListDialog->prepareData(currentAthleteTab->context->athlete->cyclist, CloudDBCommon::UserImport, currentAthleteTab->currentView())) {
        if (currentAthleteTab->context->cdbChartListDialog->exec() == QDialog::Accepted) {

            // get selected chartDef
            QList<QString> chartDefs = currentAthleteTab->context->cdbChartListDialog->getSelectedSettings();

            // parse charts into property pairs
            foreach (QString chartDef, chartDefs) {
                QList<QMap<QString,QString> > properties = GcChartWindow::chartPropertiesFromString(chartDef);
                for (int i = 0; i< properties.size(); i++) {
                    currentAthleteTab->context->mainWindow->athleteTab()->view(currentAthleteTab->currentView())->importChart(properties.at(i), false);
                }
            }
        }
    }
}
#endif

void
MainWindow::toggleStyle()
{
    currentAthleteTab->toggleTile();
    styleAction->setChecked(currentAthleteTab->isTiled());
    setToolButtons();
}

void
MainWindow::toggleFullScreen()
{
#ifdef Q_OS_MAC
    QRect screenSize = desktop->availableGeometry();
    if (screenSize.width() > frameGeometry().width() ||
        screenSize.height() > frameGeometry().height())
        showFullScreen();
    else
        showNormal();
#else
    if (fullScreen) fullScreen->toggle();
    else qDebug()<<"no fullscreen support compiled in.";
#endif
}

bool
MainWindow::eventFilter(QObject *o, QEvent *e)
{
    if (o == this) {
        if (e->type() == QEvent::WindowStateChange) {

            // if we are entering full screen mode we hide the sidebar
            if (windowState()&Qt::WindowFullScreen) showViewbar(false);
            else showViewbar(true);

            resizeEvent(NULL); // see below
        }
    }
    return false;
}

void
MainWindow::resizeEvent(QResizeEvent*)
{
    //appsettings->setValue(GC_SETTINGS_MAIN_GEOM, saveGeometry());
    //appsettings->setValue(GC_SETTINGS_MAIN_STATE, saveState());

}

void
MainWindow::showOptions()
{
    // Create a new config dialog only if it doesn't exist
    ConfigDialog *cd = configdialog_ptr ? configdialog_ptr
                                        : new ConfigDialog(currentAthleteTab->context->athlete->home->root(), currentAthleteTab->context);

    // move to the centre of the screen
    cd->move(geometry().center()-QPoint(cd->geometry().width()/2, cd->geometry().height()/2));
    cd->show();
    cd->raise();
}

void
MainWindow::moveEvent(QMoveEvent*)
{
    appsettings->setValue(GC_SETTINGS_MAIN_GEOM, saveGeometry());
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    QList<AthleteTab*> closing = tabList;
    bool needtosave = false;
    bool importrunning = false;

    // close all the tabs .. if any refuse we need to ignore
    //                       the close event
    foreach(AthleteTab *tab, closing) {

        // check for if RideImport is is process and let it finalize / or be stopped by the user
        if (tab->context->athlete->autoImport) {
            if (tab->context->athlete->autoImport->importInProcess() ) {
                importrunning = true;
                QMessageBox::information(this, tr("Activity Import"), tr("Closing of athlete window not possible while background activity import is in progress..."));
            }
        }

        // only check for unsaved if autoimport is not running any more
        if (!importrunning) {
            // do we need to save?
            if (tab->context->mainWindow->saveRideExitDialog(tab->context) == true)
                removeAthleteTab(tab);
            else
                needtosave = true;
        }
    }

    // were any left hanging around? or autoimport in action on any windows, then don't close any
    if (needtosave || importrunning) event->ignore();
    else {

        // finish off the job and leave
        // clear the clipboard if neccessary
        QApplication::clipboard()->setText("");

        // now remove from the list
        if(mainwindows.removeOne(this) == false)
            qDebug()<<"closeEvent: mainwindows list error";

        // save global mainwindow settings
        appsettings->setValue(GC_TABBAR, showhideTabbar->isChecked());
        // wait for threads.. max of 10 seconds before just exiting anyway
        for (int i=0; i<10 && QThreadPool::globalInstance()->activeThreadCount(); i++) {
            QThread::sleep(1);
        }
    }
    appsettings->setValue(GC_SETTINGS_MAIN_GEOM, saveGeometry());
    appsettings->setValue(GC_SETTINGS_MAIN_STATE, saveState());
}

MainWindow::~MainWindow()
{
    // aside from the tabs, we may need to clean
    // up any dangling widgets created in MainWindow::MainWindow (?)
    if (configdialog_ptr) configdialog_ptr->close();
}

// global search/data filter
void MainWindow::setFilter(QStringList f) { currentAthleteTab->context->setFilter(f); }
void MainWindow::clearFilter() { currentAthleteTab->context->clearFilter(); }

void
MainWindow::aboutDialog()
{
    AboutDialog *ad = new AboutDialog(currentAthleteTab->context);
    ad->exec();
}

void MainWindow::showSolveCP()
{
   SolveCPDialog *td = new SolveCPDialog(this, currentAthleteTab->context);
   td->show();
}

void MainWindow::showEstimateCP()
{
   EstimateCPDialog *td = new EstimateCPDialog();
   td->show();
}

void MainWindow::showRhoEstimator()
{
   ToolsRhoEstimator *tre = new ToolsRhoEstimator(currentAthleteTab->context);
   tre->show();
}

void MainWindow::showVDOTCalculator()
{
   VDOTCalculator *VDOTcalculator = new VDOTCalculator();
   VDOTcalculator->show();
}

void MainWindow::showWorkoutWizard()
{
   WorkoutWizard *ww = new WorkoutWizard(currentAthleteTab->context);
   ww->show();
}

void MainWindow::resetWindowLayout()
{
    QMessageBox msgBox;
    msgBox.setText(tr("You are about to reset all charts to the default setup"));
    msgBox.setInformativeText(tr("Do you want to continue?"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    if(msgBox.clickedButton() == msgBox.button(QMessageBox::Ok))
        currentAthleteTab->resetLayout(perspectiveSelector);
}

void MainWindow::manualProcess(QString name)
{
    // open a dialog box and let the users
    // configure the options to use
    // and also show the explanation
    // of what this function does
    // then call it!
    RideItem *rideitem = (RideItem*)currentAthleteTab->context->currentRideItem();
    if (rideitem) {

#ifdef GC_WANT_PYTHON
        if (name.startsWith("_fix_py_")) {
            name = name.remove(0, 8);

            FixPyScript *script = fixPySettings->getScript(name);
            if (script == nullptr) {
                return;
            }

            QString errText;
            FixPyRunner pyRunner(currentAthleteTab->context);
            if (pyRunner.run(script->source, script->iniKey, errText) != 0) {
                QMessageBox::critical(this, "GoldenCheetah", errText);
            }

            return;
        }
#endif

        ManualDataProcessorDialog *p = new ManualDataProcessorDialog(currentAthleteTab->context, name, rideitem);
        p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
        p->exec();
    }
}


void
MainWindow::helpWindow()
{
    HelpWindow* help = new HelpWindow(currentAthleteTab->context);
    help->show();
}


void
MainWindow::logBug()
{
    QDesktopServices::openUrl(QUrl("https://github.com/GoldenCheetah/GoldenCheetah/issues"));
}

void
MainWindow::helpView()
{
    QDesktopServices::openUrl(QUrl("https://github.com/GoldenCheetah/GoldenCheetah/wiki"));
}


void
MainWindow::support()
{
    QDesktopServices::openUrl(QUrl("https://groups.google.com/forum/#!forum/golden-cheetah-users"));
}

void
MainWindow::sidebarClicked(int id)
{
    // sync quick link
    if (id == 7) checkCloud();

    // prefs
    if (id == 8) showOptions();

}

void
MainWindow::sidebarSelected(int id)
{
    switch (id) {
    case 0: selectAthlete(); break;
    case 1: // plan not written yet
            break;
    case 2: selectTrends(); break;
    case 3: selectAnalysis(); break;
    case 4: // reflect not written yet
            break;
    case 5: selectTrain(); break;
    case 6: // apps not written yet
            break;
    }
}

void
MainWindow::selectAthlete()
{
    viewStack->setCurrentIndex(0);
    perspectiveSelector->hide();
}

void
MainWindow::selectAnalysis()
{
    resetPerspective(1);
    //currentTab->analysisView->setPerspectives(perspectiveSelector);
    viewStack->setCurrentIndex(1);
    sidebar->setItemSelected(3, true);
    currentAthleteTab->selectView(1);
    perspectiveSelector->show();
    setToolButtons();
}

void
MainWindow::selectTrain()
{
    resetPerspective(3);
    //currentTab->trainView->setPerspectives(perspectiveSelector);
    viewStack->setCurrentIndex(1);
    sidebar->setItemSelected(5, true);
    currentAthleteTab->selectView(3);
    perspectiveSelector->show();
    setToolButtons();
}

void
MainWindow::selectDiary()
{
    resetPerspective(2);
    //currentTab->diaryView->setPerspectives(perspectiveSelector);
    viewStack->setCurrentIndex(1);
    currentAthleteTab->selectView(2);
    perspectiveSelector->show();
    setToolButtons();
}

void
MainWindow::selectTrends()
{
    resetPerspective(0);
    //currentTab->homeView->setPerspectives(perspectiveSelector);
    viewStack->setCurrentIndex(1);
    sidebar->setItemSelected(2, true);
    currentAthleteTab->selectView(0);
    perspectiveSelector->show();
    setToolButtons();
}

void
MainWindow::setToolButtons()
{
    int select = currentAthleteTab->isTiled() ? 1 : 0;
    int lowselected = currentAthleteTab->isBottomRequested() ? 1 : 0;

    styleAction->setChecked(select);
    showhideLowbar->setChecked(lowselected);

    //if (styleSelector->isSegmentSelected(select) == false)
        //styleSelector->setSegmentSelected(select, true);

    int index = currentAthleteTab->currentView();

    //XXX WTAF! The index used is fucked up XXX
    //          hack around this and then come back
    //          and look at this as a separate fixup
#ifdef GC_HAVE_ICAL
    switch (index) {
    case 0: // home no change
    case 3: // train no change
    default:
        break;
    case 1:
        index = 2; // analysis
        break;
    case 2:
        index = 1; // diary
        break;
    }
#else
    switch (index) {
    case 0: // home no change
    case 1:
    default:
        break;
    case 3:
        index = 2; // train
    }
#endif
#ifdef Q_OS_MAC // bizarre issue with searchbox focus on tab voew change
    searchBox->clearFocus();
#endif
}

void
MainWindow::switchPerspective(int index)
{
    if (pactive) return;

    if (index >=0 && index < perspectiveSelector->count())
        perspectiveSelector->setCurrentIndex(index);
}

void
MainWindow::resetPerspective(int view, bool force)
{
    static AthleteTab *lastathlete=NULL;
    static int lastview=-1;

    if (!force && lastview == view && lastathlete == currentAthleteTab) return;

    // remember who last updated it.
    lastathlete = currentAthleteTab;
    lastview = view;

    // don't argue just reset the perspective for this view
    AbstractView *current = NULL;
    switch (view) {

    case 0:  current = currentAthleteTab->homeView; break;
    case 1:  current = currentAthleteTab->analysisView; break;
    case 2:  current = currentAthleteTab->diaryView; break;
    case 3:  current = currentAthleteTab->trainView; break;
    }

    // set the perspective
    current->setPerspectives(perspectiveSelector);
    pactive=false;
}

void
MainWindow::perspectiveSelected(int index)
{
    if (pactive) return;

    // set the perspective for the current view
    int view = currentAthleteTab->currentView();
    AbstractView *current = NULL;
    switch (view) {
    case 0:  current = currentAthleteTab->homeView; break;
    case 1:  current = currentAthleteTab->analysisView; break;
    case 2:  current = currentAthleteTab->diaryView; break;
    case 3:  current = currentAthleteTab->trainView; break;
    }

    // which perspective is currently being shown?
    int prior = current->perspectives_.indexOf(current->perspective_);

    if (index < current->perspectives_.count()) {

        // a perspectives was selected
        switch (view) {
        case 0:  current->perspectiveSelected(index); break;
        case 1:  current->perspectiveSelected(index); break;
        case 2:  current->perspectiveSelected(index); break;
        case 3:  current->perspectiveSelected(index); break;
        }

    } else {

        // manage or add perspectives selected
        pactive = true;

        // set the combo back to where it was
        perspectiveSelector->setCurrentIndex(prior);

        // now open dialog etc
        switch (index - current->perspectives_.count()) {
        case 1 : // add perspectives
            {
                QString name;
                QString expression;
                Perspective::switchenum trainswitch=Perspective::None;
                AddPerspectiveDialog *dialog= new AddPerspectiveDialog(this, currentAthleteTab->context, name, expression, current->type, trainswitch);
                int ret= dialog->exec();
                delete dialog;
                if (ret == QDialog::Accepted && name != "") {

                    // add...
                    Perspective *newone = current->addPerspective(name);
                    newone->setExpression(expression);
                    newone->setTrainSwitch(trainswitch);
                    current->setPerspectives(perspectiveSelector);

                    // and select remember pactive is true, so we do the heavy lifting here
                    perspectiveSelector->setCurrentIndex(current->perspectives_.count()-1);
                    current->perspectiveSelected(perspectiveSelector->currentIndex());
                }
            }
            break;
        case 2 : // manage perspectives
            PerspectiveDialog *dialog = new PerspectiveDialog(this, current);
            connect(dialog, SIGNAL(perspectivesChanged()), this, SLOT(perspectivesChanged())); // update the selector and view
            dialog->exec();
            break;
        }
        pactive = false;
    }
}

// manage perspectives has done something (remove/add/reorder perspectives)
// pactive MUST be true, see above
void
MainWindow::perspectivesChanged()
{
    int view = currentAthleteTab->currentView();
    AbstractView *current = NULL;

    switch (view) {
    case 0:  current = currentAthleteTab->homeView; break;
    case 1:  current = currentAthleteTab->analysisView; break;
    case 2:  current = currentAthleteTab->diaryView; break;
    case 3:  current = currentAthleteTab->trainView; break;
    }

    // which perspective is currently being selected (before we go setting the combobox)
    Perspective *prior = current->perspective_;

    // ok, so reset the combobox and force, since whilst it may have already
    // been set for this athlete+view combination the config was just changed
    // so it needs to be redone.
    resetPerspective(view, true);
    //current->setPerspectives(perspectiveSelector);

    // is the old selected perspective still available?
    int index = current->perspectives_.indexOf(prior);

    // pretend a selection was made if the index needs to change
    if (index >= 0 ) {
        // still exists, but not currently selected for some reason
        if (perspectiveSelector->currentIndex() != index)
            perspectiveSelector->setCurrentIndex(index);

        // no need to signal as its currently being shown
    } else {

        pactive = false; // dialog is active, but we need to force a change

        // need to choose first as current got deleted
        if (perspectiveSelector->currentIndex() != 0)
            perspectiveSelector->setCurrentIndex(0);
        else
            emit perspectiveSelector->currentIndexChanged(0);

        pactive = true;
    }
}

/*----------------------------------------------------------------------
 * Drag and Drop
 *--------------------------------------------------------------------*/

void
MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    bool accept = true;

    // we reject http, since we want a file!
    foreach (QUrl url, event->mimeData()->urls())
        if (url.toString().startsWith("http"))
            accept = false;

    if (accept) {
        event->acceptProposedAction(); // whatever you wanna drop we will try and process!
        raise();
    } else event->ignore();
}

void
MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    // is this a chart file ?
    QStringList filenames;
    QList<LTMSettings> imported;
    QStringList list, workouts;
    for(int i=0; i<urls.count(); i++) {

        QString filename = QFileInfo(urls.value(i).toLocalFile()).absoluteFilePath();
        fprintf(stderr, "%s\n", filename.toStdString().c_str()); fflush(stderr);

        if (filename.endsWith(".gchart", Qt::CaseInsensitive)) {
            // add to the list of charts to import
            list << filename;

        } else if (filename.endsWith(".xml", Qt::CaseInsensitive)) {

            QFile chartsFile(filename);

            // setup XML processor
            QXmlInputSource source( &chartsFile );
            QXmlSimpleReader xmlReader;
            LTMChartParser handler;
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);

            // parse and get return values
            xmlReader.parse(source);
            imported += handler.getSettings();

        // Look for Workout files only in Train view
        } else if (currentAthleteTab->currentView() == 3 && ErgFile::isWorkout(filename)) {
            workouts << filename;
        } else {
            filenames.append(filename);
        }
    }

    // import any we may have extracted
    if (imported.count()) {

        // now append to the QList and QTreeWidget
        currentAthleteTab->context->athlete->presets += imported;

        // notify we changed and tree updates
        currentAthleteTab->context->notifyPresetsChanged();

        // tell the user
        QMessageBox::information(this, tr("Chart Import"), QString(tr("Imported %1 metric charts")).arg(imported.count()));

        // switch to trend view if we aren't on it
        selectTrends();

        // now select what was added
        currentAthleteTab->context->notifyPresetSelected(currentAthleteTab->context->athlete->presets.count()-1);
    }

    // are there any .gcharts to import?
    if (list.count())  importCharts(list);

    // import workouts
    if (workouts.count()) Library::importFiles(currentAthleteTab->context, workouts, true);

    // if there is anything left, process based upon view...
    if (filenames.count()) {

        // We have something to process then
        RideImportWizard *dialog = new RideImportWizard (filenames, currentAthleteTab->context);
        dialog->process(); // do it!
    }
    return;
}

void
MainWindow::importCharts(QStringList list)
{
    QList<QMap<QString,QString> > charts;

    // parse charts into property pairs
    foreach(QString filename, list) {
        charts << GcChartWindow::chartPropertiesFromFile(filename);
    }

    // And import them with a dialog to select location
    ImportChartDialog importer(currentAthleteTab->context, charts, this);
    importer.exec();
}


/*----------------------------------------------------------------------
 * Ride Library Functions
 *--------------------------------------------------------------------*/


void
MainWindow::downloadRide()
{
    (new DownloadRideDialog(currentAthleteTab->context))->show();
}


void
MainWindow::manualRide()
{
    (new ManualRideDialog(currentAthleteTab->context))->show();
}

void
MainWindow::exportBatch()
{
    BatchExportDialog *d = new BatchExportDialog(currentAthleteTab->context);
    d->exec();
}

void
MainWindow::generateHeatMap()
{
    GenerateHeatMapDialog *d = new GenerateHeatMapDialog(currentAthleteTab->context);
    d->exec();
}

void
MainWindow::exportRide()
{
    if (currentAthleteTab->context->ride == NULL) {
        QMessageBox::critical(this, tr("Select Activity"), tr("No activity selected!"));
        return;
    }

    // what format?
    const RideFileFactory &rff = RideFileFactory::instance();
    QStringList allFormats;
    allFormats << "Export all data (*.csv)";
    allFormats << "Export W' balance (*.csv)";
    foreach(QString suffix, rff.writeSuffixes())
        allFormats << QString("%1 (*.%2)").arg(rff.description(suffix)).arg(suffix);

    QString suffix; // what was selected?
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Activity"), QDir::homePath(), allFormats.join(";;"), &suffix);

    if (fileName.length() == 0) return;

    // which file type was selected
    // extract from the suffix returned
    QRegExp getSuffix("^[^(]*\\(\\*\\.([^)]*)\\)$");
    getSuffix.exactMatch(suffix);

    QFile file(fileName);
    RideFile *currentRide = currentAthleteTab->context->ride ? currentAthleteTab->context->ride->ride() : NULL;
    bool result=false;

    // Extract suffix from chosen file name and convert to lower case
    QString fileNameSuffix;
    int lastDot = fileName.lastIndexOf(".");
    if (lastDot >=0) fileNameSuffix = fileName.mid(fileName.lastIndexOf(".")+1);
    fileNameSuffix = fileNameSuffix.toLower();

    // See if this suffix can be used to determine file type (not ok for csv since there are options)
    bool useFileTypeSuffix = false;
    if (!fileNameSuffix.isEmpty() && fileNameSuffix != "csv")
    {
        useFileTypeSuffix = rff.writeSuffixes().contains(fileNameSuffix);
    }

    if (useFileTypeSuffix)
    {
        result = RideFileFactory::instance().writeRideFile(currentAthleteTab->context, currentRide, file, fileNameSuffix);
    }
    else
    {
        // Use the value of drop down list to determine file type
        int idx = allFormats.indexOf(getSuffix.cap(0));

        if (idx>1) {

            result = RideFileFactory::instance().writeRideFile(currentAthleteTab->context, currentRide, file, getSuffix.cap(1));

        } else if (idx==0){

            CsvFileReader writer;
            result = writer.writeRideFile(currentAthleteTab->context, currentRide, file, CsvFileReader::gc);

        } else if (idx==1){

            CsvFileReader writer;
            result = writer.writeRideFile(currentAthleteTab->context, currentRide, file, CsvFileReader::wprime);

        }
    }

    if (result == false) {
        QMessageBox oops(QMessageBox::Critical, tr("Export Failed"),
                         tr("Failed to export activity, please check permissions"));
        oops.exec();
    }
}

void
MainWindow::importFile()
{
    QVariant lastDirVar = appsettings->value(this, GC_SETTINGS_LAST_IMPORT_PATH);
    QString lastDir = (lastDirVar != QVariant())
        ? lastDirVar.toString() : QDir::homePath();

    const RideFileFactory &rff = RideFileFactory::instance();
    QStringList suffixList = rff.suffixes();
    suffixList.replaceInStrings(QRegExp("^"), "*.");
    QStringList fileNames;
    QStringList allFormats;
    allFormats << QString("All Supported Formats (%1)").arg(suffixList.join(" "));
    foreach(QString suffix, rff.suffixes())
        allFormats << QString("%1 (*.%2)").arg(rff.description(suffix)).arg(suffix);
    allFormats << "All files (*.*)";
    fileNames = QFileDialog::getOpenFileNames( this, tr("Import from File"), lastDir, allFormats.join(";;"));
    if (!fileNames.isEmpty()) {
        lastDir = QFileInfo(fileNames.front()).absolutePath();
        appsettings->setValue(GC_SETTINGS_LAST_IMPORT_PATH, lastDir);
        QStringList fileNamesCopy = fileNames; // QT doc says iterate over a copy
        RideImportWizard *import = new RideImportWizard(fileNamesCopy, currentAthleteTab->context);
        import->process();
    }
}

void
MainWindow::saveRide()
{
    // no ride
    if (currentAthleteTab->context->ride == NULL) {
        QMessageBox oops(QMessageBox::Critical, tr("No Activity To Save"),
                         tr("There is no currently selected activity to save."));
        oops.exec();
        return;
    }

    // flush in-flight changes
    currentAthleteTab->context->notifyMetadataFlush();
    currentAthleteTab->context->ride->notifyRideMetadataChanged();

    // nothing to do if not dirty
    //XXX FORCE A SAVE if (currentTab->context->ride->isDirty() == false) return;

    // save
    if (currentAthleteTab->context->ride) {
        saveRideSingleDialog(currentAthleteTab->context, currentAthleteTab->context->ride); // will signal save to everyone
    }
}

void
MainWindow::athleteSettings()
{
    AthleteConfigDialog *dialog = new AthleteConfigDialog(currentAthleteTab->context->athlete->home->root(), currentAthleteTab->context);
    dialog->exec();
}

void
MainWindow::saveAllUnsavedRides()
{
    // flush in-flight changes
    currentAthleteTab->context->notifyMetadataFlush();
    currentAthleteTab->context->ride->notifyRideMetadataChanged();

    // save
    if (currentAthleteTab->context->ride) {
        saveAllFilesSilent(currentAthleteTab->context); // will signal save to everyone
    }
}

void
MainWindow::revertRide()
{
    currentAthleteTab->context->ride->close();
    currentAthleteTab->context->ride->ride(); // force re-load

    // in case reverted ride has different starttime
    currentAthleteTab->context->ride->setStartTime(currentAthleteTab->context->ride->ride()->startTime());
    currentAthleteTab->context->ride->ride()->emitReverted();

    // and notify everyone we changed which also has the side
    // effect of updating the cached values too
    currentAthleteTab->context->notifyRideSelected(currentAthleteTab->context->ride);
}

void
MainWindow::splitRide()
{
    if (currentAthleteTab->context->ride && currentAthleteTab->context->ride->ride() && currentAthleteTab->context->ride->ride()->dataPoints().count()) (new SplitActivityWizard(currentAthleteTab->context))->exec();
    else {
        if (!currentAthleteTab->context->ride || !currentAthleteTab->context->ride->ride())
            QMessageBox::critical(this, tr("Split Activity"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Split Activity"), tr("Current activity contains no data to split"));
    }
}

void
MainWindow::mergeRide()
{
    if (currentAthleteTab->context->ride && currentAthleteTab->context->ride->ride() && currentAthleteTab->context->ride->ride()->dataPoints().count()) (new MergeActivityWizard(currentAthleteTab->context))->exec();
    else {
        if (!currentAthleteTab->context->ride || !currentAthleteTab->context->ride->ride())
            QMessageBox::critical(this, tr("Split Activity"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Split Activity"), tr("Current activity contains no data to merge"));
    }
}

void
MainWindow::deleteRide()
{
    RideItem *_item = currentAthleteTab->context->ride;

    if (_item==NULL) {
        QMessageBox::critical(this, tr("Delete Activity"), tr("No activity selected!"));
        return;
    }

    RideItem *item = static_cast<RideItem*>(_item);
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to delete the activity:"));
    msgBox.setInformativeText(item->fileName);
    QPushButton *deleteButton = msgBox.addButton(tr("Delete"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
    if(msgBox.clickedButton() == deleteButton)
        currentAthleteTab->context->athlete->removeCurrentRide();
}

/*----------------------------------------------------------------------
 * Realtime Devices and Workouts
 *--------------------------------------------------------------------*/
void
MainWindow::addDevice()
{

    // lets get a new one
    AddDeviceWizard *p = new AddDeviceWizard(currentAthleteTab->context);
    p->show();

}

/*----------------------------------------------------------------------
 * Cyclists
 *--------------------------------------------------------------------*/

void
MainWindow::newCyclistTab()
{
    QDir newHome = currentAthleteTab->context->athlete->home->root();
    newHome.cdUp();
    QString name = ChooseCyclistDialog::newCyclistDialog(newHome, this);
    if (!name.isEmpty()) {
        emit newAthlete(name);
        openAthleteTab(name);
    }
}

void
MainWindow::closeWindow()
{
    // just call close, we might do more later
    appsettings->syncQSettings();
    close();
}

void
MainWindow::openAthleteTab(QString name)
{
    QDir home(gcroot);
    appsettings->initializeQSettingsGlobal(gcroot);
    home.cd(name);

    if (!home.exists()) return;
    appsettings->initializeQSettingsAthlete(gcroot, name);

    GcUpgrade v3;
    if (!v3.upgradeConfirmedByUser(home)) return;

    Context *con= new Context(this);
    con->athlete = NULL;
    emit openingAthlete(name, con);

    connect(con, SIGNAL(loadCompleted(QString,Context*)), this, SLOT(loadCompleted(QString, Context*)));

    // will emit loadCompleted when done
    con->athlete = new Athlete(con, home);
}

void
MainWindow::loadCompleted(QString name, Context *context)
{
    // athlete loaded
    currentAthleteTab = new AthleteTab(context);

    // clear splash - progress whilst loading tab
    //clearSplash();

    // first tab
    athletetabs.insert(currentAthleteTab->context->athlete->home->root().dirName(), currentAthleteTab);

    // stack, list and bar all share a common index
    tabList.append(currentAthleteTab);
    tabbar->addTab(currentAthleteTab->context->athlete->home->root().dirName());
    tabStack->addWidget(currentAthleteTab);

    // switch to newly created athlete
    tabbar->setCurrentIndex(tabList.count()-1);

    // show the tabbar if we're gonna open tabs -- but wait till the last second
    // to show it to avoid crappy paint artefacts
    showTabbar(true);

    // tell everyone
    currentAthleteTab->context->notifyLoadDone(name, context);

    // now do the automatic ride file import
    context->athlete->importFilesWhenOpeningAthlete();
}

void
MainWindow::closeTabClicked(int index)
{

    AthleteTab *tab = tabList[index];

    // check for autoimport and let it finalize
    if (tab->context->athlete->autoImport) {
        if (tab->context->athlete->autoImport->importInProcess() ) {
            QMessageBox::information(this, tr("Activity Import"), tr("Closing of athlete window not possible while background activity import is in progress..."));
            return;
        }
    }

    if (saveRideExitDialog(tab->context) == false) return;

    // lets wipe it
    removeAthleteTab(tab);
}

bool
MainWindow::closeAthleteTab(QString name)
{
    for(int i=0; i<tabbar->count(); i++) {
        if (name == tabbar->tabText(i)) {
            closeTabClicked(i);
            return true;
        }
    }
    return false;
}

bool
MainWindow::closeAthleteTab()
{
  // check for autoimport and let it finalize
    if (currentAthleteTab->context->athlete->autoImport) {
        if (currentAthleteTab->context->athlete->autoImport->importInProcess() ) {
            QMessageBox::information(this, tr("Activity Import"), tr("Closing of athlete window not possible while background activity import is in progress..."));
            return false;
        }
    }

    // wipe it down ...
    if (saveRideExitDialog(currentAthleteTab->context) == false) return false;

    // if its the last tab we close the window
    if (tabList.count() == 1)
        closeWindow();
    else {
        removeAthleteTab(currentAthleteTab);
    }
    appsettings->syncQSettings();
    // we did it
    return true;
}

// no questions asked just wipe away the current tab
void
MainWindow::removeAthleteTab(AthleteTab *tab)
{
    setUpdatesEnabled(false);

    if (tabList.count() == 2) showTabbar(false); // don't need it for one!

    // cancel ridecache refresh if its in progress
    tab->context->athlete->rideCache->cancel();

    // save the named searches
    tab->context->athlete->namedSearches->write();

    // clear the clipboard if neccessary
    QApplication::clipboard()->setText("");

    // Remember where we were
    QString name = tab->context->athlete->cyclist;

    // switch to neighbour (currentTab will change)
    int index = tabList.indexOf(tab);

    // if we're not the last then switch
    // before removing so the GUI is clean
    if (tabList.count() > 1) {
        if (index) switchAthleteTab(index-1);
        else switchAthleteTab(index+1);
    }

    // close gracefully
    tab->close();
    tab->context->athlete->close();

    // remove from state
    athletetabs.remove(name);
    tabList.removeAt(index);
    tabbar->removeTab(index);
    tabStack->removeWidget(tab);

    // delete the objects
    Context *context = tab->context;
    Athlete *athlete = tab->context->athlete;

    delete tab;
    delete athlete;
    delete context;

    setUpdatesEnabled(true);

    return;
}

void
MainWindow::setOpenTabMenu()
{
    // wipe existing
    openTabMenu->clear();

    // get a list of all cyclists
    QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {

        QString name = i.next();
        SKIP_QTWE_CACHE  // skip Folder Names created by QTWebEngine on Windows
        // new action
        QAction *action = new QAction(QString("%1").arg(name), this);

        // get the config directory
        AthleteDirectoryStructure subDirs(name);
        // icon / mugshot ?
        QString icon = QString("%1/%2/%3/avatar.png").arg(gcroot).arg(name).arg(subDirs.config().dirName());
        if (QFile(icon).exists()) action->setIcon(QIcon(icon));

        // only allow selection of cyclists which are not already open
        foreach (MainWindow *x, mainwindows) {
            QMapIterator<QString, AthleteTab*> t(x->athletetabs);
            while (t.hasNext()) {
                t.next();
                if (t.key() == name)
                    action->setEnabled(false);
            }
        }

        // add to menu
        openTabMenu->addAction(action);
        connect(action, SIGNAL(triggered()), tabMapper, SLOT(map()));
        tabMapper->setMapping(action, name);
    }

    // add create new option
    openTabMenu->addSeparator();
    openTabMenu->addAction(tr("&New Athlete..."), this, SLOT(newCyclistTab()), tr("Ctrl+N"));
}

void
MainWindow::setBackupAthleteMenu()
{
    // wipe existing
    backupAthleteMenu->clear();

    // get a list of all cyclists
    QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {

        QString name = i.next();
        SKIP_QTWE_CACHE  // skip Folder Names created by QTWebEngine on Windows
        // new action
        QAction *action = new QAction(QString("%1").arg(name), this);

        // get the config directory
        AthleteDirectoryStructure subDirs(name);
        // icon / mugshot ?
        QString icon = QString("%1/%2/%3/avatar.png").arg(gcroot).arg(name).arg(subDirs.config().dirName());
        if (QFile(icon).exists()) action->setIcon(QIcon(icon));

        // add to menu
        backupAthleteMenu->addAction(action);
        connect(action, SIGNAL(triggered()), backupMapper, SLOT(map()));
        backupMapper->setMapping(action, name);
    }

}

void
MainWindow::backupAthlete(QString name)
{
    AthleteBackup *backup = new AthleteBackup(QDir(gcroot+"/"+name));
    backup->backupImmediate();
    delete backup;
}

void
MainWindow::setDeleteAthleteMenu()
{
    // wipe existing
    deleteAthleteMenu->clear();

    // get a list of all cyclists
    QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {

        QString name = i.next();
        SKIP_QTWE_CACHE  // skip Folder Names created by QTWebEngine on Windows
        // new action
        QAction *action = new QAction(QString("%1").arg(name), this);

        // get the config directory
        AthleteDirectoryStructure subDirs(name);
        // icon / mugshot ?
        QString icon = QString("%1/%2/%3/avatar.png").arg(gcroot).arg(name).arg(subDirs.config().dirName());
        if (QFile(icon).exists()) action->setIcon(QIcon(icon));

        // only allow selection of cyclists which are not already open
        foreach (MainWindow *x, mainwindows) {
            QMapIterator<QString, AthleteTab*> t(x->athletetabs);
            while (t.hasNext()) {
                t.next();
                if (t.key() == name)
                    action->setEnabled(false);
            }
        }

        // add to menu
        deleteAthleteMenu->addAction(action);
        connect(action, SIGNAL(triggered()), deleteMapper, SLOT(map()));
        deleteMapper->setMapping(action, name);
    }
}

void
MainWindow::deleteAthlete(QString name)
{
    QDir home(gcroot);
    if(ChooseCyclistDialog::deleteAthlete(home, name, this)) {
        // notify deletion
        emit deletedAthlete(name);
    }
}

void
MainWindow::saveGCState(Context *context)
{
    // save all the current state to the supplied context
    context->showSidebar = showhideSidebar->isChecked();
    //context->showTabbar = showhideTabbar->isChecked();
    context->showLowbar = showhideLowbar->isChecked();
    context->showToolbar = showhideToolbar->isChecked();
    context->searchText = searchBox->text();
    context->style = styleAction->isChecked();
}

void
MainWindow::restoreGCState(Context *context)
{
    if (viewStack->currentIndex() != 0) {

        // not on athlete view...
        resetPerspective(currentAthleteTab->currentView()); // will lazy load, hence doing it first

        // restore window state from the supplied context
            switch(currentAthleteTab->currentView()) {
            case 0: sidebar->setItemSelected(2,true); break;
            case 1: sidebar->setItemSelected(3,true); break;
            case 2: break; // diary not an icon
            case 3: sidebar->setItemSelected(5, true); break;
            default: sidebar->setItemSelected(0, true); break;
        }
    }

    showSidebar(context->showSidebar);
    showToolbar(context->showToolbar);
    //showTabbar(context->showTabbar);
    showLowbar(context->showLowbar);
    searchBox->setContext(context);
    searchBox->setText(context->searchText);
}

void
MainWindow::switchAthleteTab(int index)
{
    if (index < 0) return;

    setUpdatesEnabled(false);

#if 0 // use athlete view, these buttons don't exist
#ifdef Q_OS_MAC // close buttons on the left on Mac
    // Only have close button on current tab (prettier)
    for(int i=0; i<tabbar->count(); i++) tabbar->tabButton(i, QTabBar::LeftSide)->hide();
    tabbar->tabButton(index, QTabBar::LeftSide)->show();
#else
    // Only have close button on current tab (prettier)
    for(int i=0; i<tabbar->count(); i++) tabbar->tabButton(i, QTabBar::RightSide)->hide();
    tabbar->tabButton(index, QTabBar::RightSide)->show();
#endif
#endif

    // save how we are
    saveGCState(currentAthleteTab->context);

    currentAthleteTab = tabList[index];
    tabStack->setCurrentIndex(index);

    // restore back
    restoreGCState(currentAthleteTab->context);

    setWindowTitle(currentAthleteTab->context->athlete->home->root().dirName());


    setUpdatesEnabled(true);
}


/*----------------------------------------------------------------------
 * MetricDB
 *--------------------------------------------------------------------*/

void
MainWindow::exportMetrics()
{
    // if the refresh process is running, try again when its completed
    if (currentAthleteTab->context->athlete->rideCache->isRunning()) {
        QMessageBox::warning(this, tr("Refresh in Progress"),
        "A metric refresh is currently running, please try again once that has completed.");
        return;
    }

    // all good lets choose a file
    QString fileName = QFileDialog::getSaveFileName( this, tr("Export Metrics"), QDir::homePath(), tr("Comma Separated Variables (*.csv)"));
    if (fileName.length() == 0) return;

    // export
    currentAthleteTab->context->athlete->rideCache->writeAsCSV(fileName);
}

/*----------------------------------------------------------------------
 * Import Workout from Disk
 *--------------------------------------------------------------------*/
void
MainWindow::importWorkout()
{
    // go look at last place we imported workouts from...
    QVariant lastDirVar = appsettings->value(this, GC_SETTINGS_LAST_WORKOUT_PATH);
    QString lastDir = (lastDirVar != QVariant())
        ? lastDirVar.toString() : QDir::homePath();

    // anything for now, we could add filters later
    QStringList allFormats;
    allFormats << "All files (*.*)";
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Import from File"), lastDir, allFormats.join(";;"));

    // lets process them
    if (!fileNames.isEmpty()) {

        // save away last place we looked
        lastDir = QFileInfo(fileNames.front()).absolutePath();
        appsettings->setValue(GC_SETTINGS_LAST_WORKOUT_PATH, lastDir);

        QStringList fileNamesCopy = fileNames; // QT doc says iterate over a copy

        // import them via the workoutimporter
        Library::importFiles(currentAthleteTab->context, fileNamesCopy);
    }
}
/*----------------------------------------------------------------------
 * ErgDB
 *--------------------------------------------------------------------*/

void
MainWindow::downloadErgDB()
{
    QString workoutDir = appsettings->value(this, GC_WORKOUTDIR).toString();

    QFileInfo fi(workoutDir);

    if (fi.exists() && fi.isDir()) {
        ErgDBDownloadDialog *d = new ErgDBDownloadDialog(currentAthleteTab->context);
        d->exec();
    } else{
        QMessageBox::critical(this, tr("Workout Directory Invalid"),
        tr("The workout directory is not configured, or the directory selected no longer exists.\n\n"
        "Please check your preference settings."));
    }
}

/*----------------------------------------------------------------------
 * TodaysPlan Workouts
 *--------------------------------------------------------------------*/

void
MainWindow::downloadTodaysPlanWorkouts()
{
    QString workoutDir = appsettings->value(this, GC_WORKOUTDIR).toString();

    QFileInfo fi(workoutDir);

    if (fi.exists() && fi.isDir()) {
        TodaysPlanWorkoutDownload *d = new TodaysPlanWorkoutDownload(currentAthleteTab->context);
        d->exec();
    } else{
        QMessageBox::critical(this, tr("Workout Directory Invalid"),
        tr("The workout directory is not configured, or the directory selected no longer exists.\n\n"
        "Please check your preference settings."));
    }
}

/*----------------------------------------------------------------------
 * Workout/Media Library
 *--------------------------------------------------------------------*/
void
MainWindow::manageLibrary()
{
    LibrarySearchDialog *search = new LibrarySearchDialog(currentAthleteTab->context);
    search->exec();
}

/*----------------------------------------------------------------------------
 * Working with Cloud Services
 * --------------------------------------------------------------------------*/

void
MainWindow::addAccount()
{
    // lets get a new cloud service account
    AddCloudWizard *p = new AddCloudWizard(currentAthleteTab->context);
    p->show();
}

void
MainWindow::checkCloud()
{
    // kick off a check
    currentAthleteTab->context->athlete->cloudAutoDownload->checkDownload();

    // and auto import too whilst we're at it
    currentAthleteTab->context->athlete->importFilesWhenOpeningAthlete();
}

void
MainWindow::importCloud()
{
    // lets get a new cloud service account
    AddCloudWizard *p = new AddCloudWizard(currentAthleteTab->context, "", true);
    p->show();
}

void
MainWindow::uploadCloud(QAction *action)
{
    // upload current ride, if we have one
    if (currentAthleteTab->context->ride) {
        // & removed to avoid issues with kde AutoCheckAccelerators
        QString actionText = QString(action->text()).replace("&", "");

        if (actionText == "University of Kent") {
            CloudService *db = CloudServiceFactory::instance().newService(action->data().toString(), currentAthleteTab->context);
            KentUniversityUploadDialog uploader(this, db, currentAthleteTab->context->ride);
            uploader.exec();
        } else {
            CloudService *db = CloudServiceFactory::instance().newService(action->data().toString(), currentAthleteTab->context);
            CloudService::upload(this, currentAthleteTab->context, db, currentAthleteTab->context->ride);
        }
    }
}

void
MainWindow::syncCloud(QAction *action)
{
    // sync with cloud
    CloudService *db = CloudServiceFactory::instance().newService(action->data().toString(), currentAthleteTab->context);
    CloudServiceSyncDialog sync(currentAthleteTab->context, db);
    sync.exec();
}


/*----------------------------------------------------------------------
 * Utility
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 * Notifiers - application level events
 *--------------------------------------------------------------------*/

void
MainWindow::configChanged(qint32)
{
    // Windows and Linux menu bar should match chrome
    QColor textCol(Qt::black);
    if (GCColor::luminance(GColor(CTOOLBAR)) < 127)  textCol = QColor(Qt::white);
    QString menuColorString = (GCColor::isFlat() ? GColor(CTOOLBAR).name() : "rgba(225,225,225)");
    menuBar()->setStyleSheet(QString("QMenuBar { color: %1; background: %2; }"
                             "QMenuBar::item { color: %1; background: %2; }")
                             .arg(textCol.name()).arg(menuColorString));
    // search filter box match chrome color
    searchBox->setStyleSheet(QString("QLineEdit { background: %1; color: %2; }").arg(GColor(CTOOLBAR).name()).arg(GCColor::invertColor(GColor(CTOOLBAR)).name()));

    // perspective selector mimics sidebar colors
    QColor selected;
    if (GCColor::invertColor(GColor(CTOOLBAR)).name() == Qt::white) selected = QColor(Qt::lightGray);
    else selected = QColor(Qt::darkGray);
    perspectiveSelector->setStyleSheet(QString("QComboBox { background: %1; color: %2; }"
                                               "QComboBox::item { background: %1; color: %2; }"
                                               "QComboBox::item::selected { background: %3; color: %1; }").arg(GColor(CTOOLBAR).name()).arg(GCColor::invertColor(GColor(CTOOLBAR)).name()).arg(selected.name()));

    QString buttonstyle = QString("QPushButton { border: none; border-radius: %2px; background-color: %1; "
                                                "padding-left: 0px; padding-right: 0px; "
                                                "padding-top:  0px; padding-bottom: 0px; }"
                                  "QPushButton:hover { background-color: %3; }"
                                  "QPushButton:hover:pressed { background-color: %3; }"
                                ).arg(GColor(CTOOLBAR).name()).arg(3 * dpiXFactor).arg(GColor(CHOVER).name());

    back->setStyleSheet(buttonstyle);
    forward->setStyleSheet(buttonstyle);
    sidelist->setStyleSheet(buttonstyle);
    tabtile->setStyleSheet(buttonstyle);
    lowbar->setStyleSheet(buttonstyle);
    whatsthis->setStyleSheet(buttonstyle);

    // All platforms
    QPalette tabbarPalette;
    tabbar->setAutoFillBackground(true);
    tabbar->setShape(QTabBar::RoundedSouth);
    tabbar->setDrawBase(false);

    tabbarPalette.setBrush(backgroundRole(), GColor(CTOOLBAR));
    tabbarPalette.setBrush(foregroundRole(), GCColor::invertColor(GColor(CTOOLBAR)));
    tabbar->setPalette(tabbarPalette);
    athleteView->setPalette(tabbarPalette);

    head->updateGeometry();
    repaint();

}

/*----------------------------------------------------------------------
 * Measures
 *--------------------------------------------------------------------*/

void
MainWindow::setMeasuresMenu()
{
    measuresMenu->clear();
    if (currentAthleteTab->context->athlete == nullptr) return;
    Measures *measures = currentAthleteTab->context->athlete->measures;
    int group = 0;
    foreach(QString name, measures->getGroupNames()) {

        // we use the group index to identify the measures group
        QAction *service = new QAction(NULL);
        service->setText(name);
        service->setData(group++);
        measuresMenu->addAction(service);
    }
}

void
MainWindow::downloadMeasures(QAction *action)
{
    // download or import from CSV file
    if (currentAthleteTab->context->athlete == nullptr) return;
    Measures *measures = currentAthleteTab->context->athlete->measures;
    int group = action->data().toInt();
    MeasuresDownload dialog(currentAthleteTab->context, measures->getGroup(group));
    dialog.exec();
}

void
MainWindow::actionClicked(int index)
{
    switch(index) {

    default:
    case 0: currentAthleteTab->addIntervals();
            break;

    case 1 : splitRide();
            break;

    case 2 : deleteRide();
            break;

    }
}

void
MainWindow::addIntervals()
{
    currentAthleteTab->addIntervals();
}

void
MainWindow::ridesAutoImport() {

    currentAthleteTab->context->athlete->importFilesWhenOpeningAthlete();

}

#ifdef GC_WANT_PYTHON
void MainWindow::onEditMenuAboutToShow()
{
    bool embedPython = appsettings->value(nullptr, GC_EMBED_PYTHON, true).toBool();
    pyFixesMenu->menuAction()->setVisible(embedPython);
}

void MainWindow::buildPyFixesMenu()
{
    pyFixesMenu->clear();

    QList<FixPyScript *> fixPyScripts = fixPySettings->getScripts();
    foreach (FixPyScript *fixPyScript, fixPyScripts) {
        QAction *action = new QAction(QString("%1...").arg(fixPyScript->name), this);
        pyFixesMenu->addAction(action);
        connect(action, SIGNAL(triggered()), toolMapper, SLOT(map()));
        toolMapper->setMapping(action, "_fix_py_" + fixPyScript->name);
    }

    pyFixesMenu->addSeparator();
    pyFixesMenu->addAction(tr("New Python Fix..."), this, SLOT (showCreateFixPyScriptDlg()));
    pyFixesMenu->addAction(tr("Manage Python Fixes..."), this, SLOT (showManageFixPyScriptsDlg()));
}

void MainWindow::showManageFixPyScriptsDlg() {
    ManageFixPyScriptsDialog dlg(currentAthleteTab->context);
    dlg.exec();
}

void MainWindow::showCreateFixPyScriptDlg() {
    EditFixPyScriptDialog dlg(currentAthleteTab->context, nullptr, this);
    dlg.exec();
}
#endif

#ifdef GC_HAS_CLOUD_DB
void
MainWindow::cloudDBuserEditChart()
{
    if (!(appsettings->cvalue(currentAthleteTab->context->athlete->cyclist, GC_CLOUDDB_TC_ACCEPTANCE, false).toBool())) {
       CloudDBAcceptConditionsDialog acceptDialog(currentAthleteTab->context->athlete->cyclist);
       acceptDialog.setModal(true);
       if (acceptDialog.exec() == QDialog::Rejected) {
          return;
       };
    }

    if (currentAthleteTab->context->cdbChartListDialog == NULL) {
        currentAthleteTab->context->cdbChartListDialog = new CloudDBChartListDialog();
    }

    // force refresh in prepare to allways get the latest data here
    if (currentAthleteTab->context->cdbChartListDialog->prepareData(currentAthleteTab->context->athlete->cyclist, CloudDBCommon::UserEdit)) {
        currentAthleteTab->context->cdbChartListDialog->exec(); // no action when closed
    }
}

void
MainWindow::cloudDBuserEditUserMetric()
{
    if (!(appsettings->cvalue(currentAthleteTab->context->athlete->cyclist, GC_CLOUDDB_TC_ACCEPTANCE, false).toBool())) {
       CloudDBAcceptConditionsDialog acceptDialog(currentAthleteTab->context->athlete->cyclist);
       acceptDialog.setModal(true);
       if (acceptDialog.exec() == QDialog::Rejected) {
          return;
       };
    }

    if (currentAthleteTab->context->cdbUserMetricListDialog == NULL) {
        currentAthleteTab->context->cdbUserMetricListDialog = new CloudDBUserMetricListDialog();
    }

    // force refresh in prepare to allways get the latest data here
    if (currentAthleteTab->context->cdbUserMetricListDialog->prepareData(currentAthleteTab->context->athlete->cyclist, CloudDBCommon::UserEdit)) {
        currentAthleteTab->context->cdbUserMetricListDialog->exec(); // no action when closed
    }
}

void
MainWindow::cloudDBcuratorEditChart()
{
    // first check if the user is a curator
    CloudDBCuratorClient *curatorClient = new CloudDBCuratorClient;
    if (curatorClient->isCurator(appsettings->cvalue(currentAthleteTab->context->athlete->cyclist, GC_ATHLETE_ID, "" ).toString())) {

        if (currentAthleteTab->context->cdbChartListDialog == NULL) {
            currentAthleteTab->context->cdbChartListDialog = new CloudDBChartListDialog();
        }

        // force refresh in prepare to allways get the latest data here
        if (currentAthleteTab->context->cdbChartListDialog->prepareData(currentAthleteTab->context->athlete->cyclist, CloudDBCommon::CuratorEdit)) {
            currentAthleteTab->context->cdbChartListDialog->exec(); // no action when closed
        }
    } else {
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Current athlete is not registered as curator - please contact the GoldenCheetah team")));

    }
}

void
MainWindow::cloudDBcuratorEditUserMetric()
{
    // first check if the user is a curator
    CloudDBCuratorClient *curatorClient = new CloudDBCuratorClient;
    if (curatorClient->isCurator(appsettings->cvalue(currentAthleteTab->context->athlete->cyclist, GC_ATHLETE_ID, "" ).toString())) {

        if (currentAthleteTab->context->cdbUserMetricListDialog == NULL) {
            currentAthleteTab->context->cdbUserMetricListDialog = new CloudDBUserMetricListDialog();
        }

        // force refresh in prepare to allways get the latest data here
        if (currentAthleteTab->context->cdbUserMetricListDialog->prepareData(currentAthleteTab->context->athlete->cyclist, CloudDBCommon::CuratorEdit)) {
            currentAthleteTab->context->cdbUserMetricListDialog->exec(); // no action when closed
        }
    } else {
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Current athlete is not registered as curator - please contact the GoldenCheetah team")));

    }
}

void
MainWindow::cloudDBshowStatus() {

    CloudDBStatusClient::displayCloudDBStatus();
}


#endif

void
MainWindow::setUploadMenu()
{
    uploadMenu->clear();
    foreach(QString name, CloudServiceFactory::instance().serviceNames()) {

        const CloudService *s = CloudServiceFactory::instance().service(name);
        if (!s || appsettings->cvalue(currentAthleteTab->context->athlete->cyclist, s->activeSettingName(), "false").toString() != "true") continue;

        if (s->capabilities() & CloudService::Upload) {

            // we need the technical name to identify the service to be called
            QAction *service = new QAction(NULL);
            service->setText(s->uiName());
            service->setData(name);

            // Kent doesn't use the standard uploader, we trap for that
            // in the upload action method
            uploadMenu->addAction(service);
        }
    }
}

void
MainWindow::setSyncMenu()
{
    syncMenu->clear();
    foreach(QString name, CloudServiceFactory::instance().serviceNames()) {

        const CloudService *s = CloudServiceFactory::instance().service(name);
        if (!s || appsettings->cvalue(currentAthleteTab->context->athlete->cyclist, s->activeSettingName(), "false").toString() != "true") continue;

        if (s->capabilities() & CloudService::Query)  {

            // We don't sync with Kent
            if (s->id() == "University of Kent") continue;

            // we need the technical name to identify the service to be called
            QAction *service = new QAction(NULL);
            service->setText(s->uiName());
            service->setData(name);
            syncMenu->addAction(service);
        }
    }
}


