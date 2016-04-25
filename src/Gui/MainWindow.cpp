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
#include "AthleteBackup.h"

#include "Colors.h"
#include "RideCache.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "Settings.h"
#include "ErgDB.h"
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
#ifdef GC_HAVE_KQOAUTH
#include "TwitterDialog.h"
#endif
#include "ShareDialog.h"
#include "WithingsDownload.h"
#include "WorkoutWizard.h"
#include "ErgDBDownloadDialog.h"
#include "AddDeviceWizard.h"
#ifdef GC_HAVE_SOAP
#include "TPUploadDialog.h"
#include "TPDownloadDialog.h"
#endif
#ifdef GC_HAVE_ICAL
#include "CalDAV.h"
#endif
#include "CalendarDownload.h"
#if QT_VERSION > 0x050000
#include "Dropbox.h"
#include "GoogleDrive.h"
#endif
#include "LocalFileStore.h"
#include "FileStore.h"

// GUI Widgets
#include "Tab.h"
#include "GcToolBar.h"
#include "HelpWindow.h"
#include "HomeWindow.h"
#include "GcScopeBar.h"
#ifdef Q_OS_MAC
#include "QtMacButton.h" // mac
#include "QtMacSegmentedButton.h" // mac
#else
#include "QTFullScreen.h" // not mac!
#include "../qtsolutions/segmentcontrol/qtsegmentcontrol.h"
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
#include "CloudDBCurator.h"
#include "CloudDBStatus.h"
#endif


// We keep track of all theopen mainwindows
QList<MainWindow *> mainwindows;
QDesktopWidget *desktop = NULL;

MainWindow::MainWindow(const QDir &home)
{
    /*----------------------------------------------------------------------
     *  Bootstrap
     *--------------------------------------------------------------------*/
    setAttribute(Qt::WA_DeleteOnClose);
    mainwindows.append(this);  // add us to the list of open windows
    init = false;
    if (desktop == NULL) desktop = QApplication::desktop();
#ifdef Q_OS_MAC
    head = NULL; // early resize event causes a crash
#endif

    // create a splash to keep user informed on first load
    // first one in middle of display, not middle of window
    setSplash(true);

    // bootstrap
    Context *context = new Context(this);
    context->athlete = new Athlete(context, home);
    currentTab = new Tab(context);

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

    static const QIcon hideIcon(":images/toolbar/main/hideside.png");
    static const QIcon rhideIcon(":images/toolbar/main/hiderside.png");
    static const QIcon showIcon(":images/toolbar/main/showside.png");
    static const QIcon rshowIcon(":images/toolbar/main/showrside.png");
    static const QIcon tabIcon(":images/toolbar/main/tab.png");
    static const QIcon tileIcon(":images/toolbar/main/tile.png");
    static const QIcon fullIcon(":images/toolbar/main/togglefull.png");

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
         QRect screenSize = desktop->availableGeometry();
         // first run -- lets set some sensible defaults...
         // lets put it in the middle of screen 1
         struct SizeSettings app = GCColor::defaultSizes(screenSize.height(), screenSize.width());

         // center on the available screen (minus toolbar/sidebar)
         move((screenSize.width()-screenSize.x())/2 - app.width/2,
              (screenSize.height()-screenSize.y())/2 - app.height/2);

         // set to the right default
         resize(app.width, app.height);

         // set all the default font sizes
         appsettings->setValue(GC_FONT_DEFAULT_SIZE, app.defaultFont);
         appsettings->setValue(GC_FONT_TITLES_SIZE, app.titleFont);
         appsettings->setValue(GC_FONT_CHARTMARKERS_SIZE, app.markerFont);
         appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, app.labelFont);
         appsettings->setValue(GC_FONT_CALENDAR_SIZE, app.calendarFont);

     }

     // store "last_openend" athlete for next time
     appsettings->setValue(GC_SETTINGS_LAST, context->athlete->home->root().dirName());

    /*----------------------------------------------------------------------
     * ScopeBar
     *--------------------------------------------------------------------*/
    scopebar = new GcScopeBar(context);
    connect(scopebar, SIGNAL(selectDiary()), this, SLOT(selectDiary()));
    connect(scopebar, SIGNAL(selectHome()), this, SLOT(selectHome()));
    connect(scopebar, SIGNAL(selectAnal()), this, SLOT(selectAnalysis()));
    connect(scopebar, SIGNAL(selectTrain()), this, SLOT(selectTrain()));
    connect(scopebar, SIGNAL(selectInterval()), this, SLOT(selectInterval()));

    /*----------------------------------------------------------------------
     * What's this Context Help
     *--------------------------------------------------------------------*/

    // Help for the whole window
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Default));
    // add Help Button
    QAction *myHelper = QWhatsThis::createAction (this);
    this->addAction(myHelper);

#if 0
    // Add chart is on the scope bar
    chartMenu = new QMenu(this);
    QStyle *styler = QStyleFactory::create("fusion");
    QPushButton *newchart = new QPushButton("+", this);
    scopebar->addWidget(newchart);
    newchart->setStyle(styler);
    newchart->setFixedHeight(20);
    newchart->setFixedWidth(24);
    newchart->setFlat(true);
    newchart->setFocusPolicy(Qt::NoFocus);
    newchart->setToolTip(tr("Add Chart"));
    newchart->setAutoFillBackground(false);
    newchart->setAutoDefault(false);
    newchart->setMenu(chartMenu);
    connect(chartMenu, SIGNAL(aboutToShow()), this, SLOT(setChartMenu()));
    connect(chartMenu, SIGNAL(triggered(QAction*)), this, SLOT(addChart(QAction*)));
#endif

    /*----------------------------------------------------------------------
     *  Mac Toolbar
     *--------------------------------------------------------------------*/
#ifdef Q_OS_MAC
#if QT_VERSION > 0x50000
#if QT_VERSION >= 0x50201
    setUnifiedTitleAndToolBarOnMac(true);
#endif
    head = addToolBar(context->athlete->cyclist);
    head->setObjectName(context->athlete->cyclist);
    head->setContentsMargins(20,0,20,0);
    head->setFloatable(false);
    head->setMovable(false);
#else
    setUnifiedTitleAndToolBarOnMac(true);
    head = addToolBar(context->athlete->cyclist);
    head->setContentsMargins(0,0,0,0);
#endif

    sidebar = new QtMacButton(this, QtMacButton::TexturedRounded);
    QPixmap *sidebarImg = new QPixmap(":images/mac/sidebar.png");
    sidebar->setImage(sidebarImg);
    sidebar->setMinimumSize(24, 24);
    sidebar->setMaximumSize(24, 24);
    sidebar->setToolTip("Sidebar");
    sidebar->setSelected(true); // assume always start up with sidebar selected
    HelpWhatsThis *helpSideBar = new HelpWhatsThis(sidebar);
    sidebar->setWhatsThis(helpSideBar->getWhatsThisText(HelpWhatsThis::ToolBar_ToggleSidebar));

    lowbar = new QtMacButton(this, QtMacButton::TexturedRounded);
    QPixmap *lowbarImg = new QPixmap(":images/mac/lowbar.png");
    lowbar->setImage(lowbarImg);
    lowbar->setMinimumSize(25, 25);
    lowbar->setMaximumSize(25, 25);
    lowbar->setToolTip("Compare");
    lowbar->setSelected(false); // assume always start up with lowbar deselected
    HelpWhatsThis *helpLowBar = new HelpWhatsThis(lowbar);
    lowbar->setWhatsThis(helpLowBar->getWhatsThisText(HelpWhatsThis::ToolBar_ToggleComparePane));

    QWidget *viewsel = new QWidget(this);
    viewsel->setContentsMargins(0,0,0,0);
    QHBoxLayout *pq = new QHBoxLayout(viewsel);
    pq->setContentsMargins(0,0,0,0);
    pq->setSpacing(5);
    QHBoxLayout *ps = new QHBoxLayout;
    ps->setContentsMargins(0,0,0,0);
    ps->setSpacing (2); // low and sidebar button close together
    ps->addWidget(sidebar);
    ps->addWidget(lowbar);
    pq->addLayout(ps);

    styleSelector = new QtMacSegmentedButton(2, viewsel);
    styleSelector->setWidth(80); // actually its 80 but we want a 30px space between is and the searchbox
    styleSelector->setImage(0, new QPixmap(":images/mac/tabbed.png"), 24);
    styleSelector->setImage(1, new QPixmap(":images/mac/tiled.png"), 24);
    pq->addWidget(styleSelector);
    connect(sidebar, SIGNAL(clicked(bool)), this, SLOT(toggleSidebar()));
    connect(lowbar, SIGNAL(clicked(bool)), this, SLOT(toggleLowbar()));
    connect(styleSelector, SIGNAL(clicked(int,bool)), this, SLOT(toggleStyle()));

    // setup Mac thetoolbar
    head->addWidget(new Spacer(this));
    head->addWidget(new Spacer(this));
    head->addWidget(scopebar);
    head->addWidget(new Spacer(this));
    head->addWidget(viewsel);

    // SearchBox and its animator
    searchBox = new SearchFilterBox(this,context,false);
    anim = new QPropertyAnimation(searchBox, "xwidth", this);

#if QT_VERSION > 0x50000
    QStyle *toolStyle = QStyleFactory::create("fusion");
#else
    QStyle *toolStyle = QStyleFactory::create("Cleanlooks");
#endif
    searchBox->setStyle(toolStyle);
    searchBox->setFixedWidth(150);
    head->addWidget(searchBox);
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
    connect(searchBox->searchbox, SIGNAL(haveFocus()), this, SLOT(searchFocusIn()));
    connect(searchBox->searchbox, SIGNAL(lostFocus()), this, SLOT(searchFocusOut()));

#endif

    /*----------------------------------------------------------------------
     *  Windows and Linux Toolbar
     *--------------------------------------------------------------------*/
#ifndef Q_OS_MAC

    head = new GcToolBar(this);

#if QT_VERSION > 0x50000
    QStyle *toolStyle = QStyleFactory::create("fusion");
#else
    QStyle *toolStyle = QStyleFactory::create("Cleanlooks");
#endif
    QPalette metal;
    metal.setColor(QPalette::Button, QColor(215,215,215));

    // get those icons
    sidebarIcon = iconFromPNG(":images/mac/sidebar.png");
    lowbarIcon = iconFromPNG(":images/mac/lowbar.png");
    tabbedIcon = iconFromPNG(":images/mac/tabbed.png");
    tiledIcon = iconFromPNG(":images/mac/tiled.png");
    QSize isize(19,19);

    lowbar = new QPushButton(this);
    lowbar->setIcon(lowbarIcon);
    lowbar->setIconSize(isize);
    lowbar->setFixedHeight(24);
    lowbar->setStyle(toolStyle);
    lowbar->setToolTip(tr("Toggle Compare Pane"));
    lowbar->setPalette(metal);
    connect(lowbar, SIGNAL(clicked(bool)), this, SLOT(toggleLowbar()));
    HelpWhatsThis *helpLowBar = new HelpWhatsThis(lowbar);
    lowbar->setWhatsThis(helpLowBar->getWhatsThisText(HelpWhatsThis::ToolBar_ToggleComparePane));

    sidebar = new QPushButton(this);
    sidebar->setIcon(sidebarIcon);
    sidebar->setIconSize(isize);
    sidebar->setFixedHeight(24);
    sidebar->setStyle(toolStyle);
    sidebar->setToolTip(tr("Toggle Sidebar"));
    sidebar->setPalette(metal);
    connect(sidebar, SIGNAL(clicked(bool)), this, SLOT(toggleSidebar()));
    HelpWhatsThis *helpSideBar = new HelpWhatsThis(sidebar);
    sidebar->setWhatsThis(helpSideBar->getWhatsThisText(HelpWhatsThis::ToolBar_ToggleSidebar));

    styleSelector = new QtSegmentControl(this);
    styleSelector->setStyle(toolStyle);
    styleSelector->setIconSize(isize);
    styleSelector->setCount(2);
    styleSelector->setSegmentIcon(0, tabbedIcon);
    styleSelector->setSegmentIcon(1, tiledIcon);
    styleSelector->setSegmentToolTip(0, tr("Tabbed View"));
    styleSelector->setSegmentToolTip(1, tr("Tiled View"));
    styleSelector->setSelectionBehavior(QtSegmentControl::SelectOne); //wince. spelling. ugh
    styleSelector->setFixedHeight(24);
    styleSelector->setPalette(metal);
    connect(styleSelector, SIGNAL(segmentSelected(int)), this, SLOT(setStyleFromSegment(int))); //avoid toggle infinitely

    head->addWidget(new Spacer(this));
    head->addStretch();
    head->addWidget(scopebar);
    head->addStretch();
    head->addWidget(sidebar);
    head->addWidget(lowbar);
    head->addWidget(styleSelector);

    // add a search box on far right, but with a little space too
    searchBox = new SearchFilterBox(this,context,false);
    anim = new QPropertyAnimation(searchBox, "xwidth", this);

    searchBox->setStyle(toolStyle);
    searchBox->setFixedWidth(150);
    head->addWidget(searchBox);
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
    connect(searchBox->searchbox, SIGNAL(haveFocus()), this, SLOT(searchFocusIn()));
    connect(searchBox->searchbox, SIGNAL(lostFocus()), this, SLOT(searchFocusOut()));
    HelpWhatsThis *helpSearchBox = new HelpWhatsThis(searchBox);
    searchBox->setWhatsThis(helpSearchBox->getWhatsThisText(HelpWhatsThis::SearchFilterBox));

    Spacer *spacer = new Spacer(this);
    spacer->setFixedWidth(5);
    head->addWidget(spacer);
#endif

    /*----------------------------------------------------------------------
     * Central Widget
     *--------------------------------------------------------------------*/

    tabbar = new DragBar(this);
    tabbar->setTabsClosable(true);
#ifdef Q_OS_MAC
    tabbar->setDocumentMode(true);
#endif

    tabStack = new QStackedWidget(this);

    // first tab
    tabs.insert(currentTab->context->athlete->home->root().dirName(), currentTab);

    // stack, list and bar all share a common index
    tabList.append(currentTab);
    tabbar->addTab(currentTab->context->athlete->home->root().dirName());
    tabStack->addWidget(currentTab);
    tabStack->setCurrentIndex(0);

    connect(tabbar, SIGNAL(dragTab(int)), this, SLOT(switchTab(int)));
    connect(tabbar, SIGNAL(currentChanged(int)), this, SLOT(switchTab(int)));
    connect(tabbar, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTabClicked(int)));

    /*----------------------------------------------------------------------
     * Central Widget
     *--------------------------------------------------------------------*/

    QWidget *central = new QWidget(this);
    setContentsMargins(0,0,0,0);
    central->setContentsMargins(0,0,0,0);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // nonmac toolbar on main view -- its not
                 // unified with the title bar.
    mainLayout->addWidget(head);
#endif
    mainLayout->addWidget(tabbar);
#if (defined Q_OS_MAC) && (QT_VERSION >= 0x50201)
    blackline = new QWidget(this);
    blackline->setContentsMargins(0,0,0,0);
    blackline->setFixedHeight(1);
    QPalette linePalette;
    linePalette.setBrush(backgroundRole(), Qt::darkGray);
    blackline->setPalette(linePalette);
    blackline->setAutoFillBackground(true);
    mainLayout->addWidget(blackline);
#endif
    mainLayout->addWidget(tabStack);
    setCentralWidget(central);

    /*----------------------------------------------------------------------
     * Application Menus
     *--------------------------------------------------------------------*/
#ifdef WIN32
    QString menuColorString = (GCColor::isFlat() ? GColor(CCHROME).name() : "rgba(225,225,225)");
    menuBar()->setStyleSheet(QString("QMenuBar { color: black; background: %1; }"
                             "QMenuBar::item { color: black; background: %1; }").arg(menuColorString));
    menuBar()->setContentsMargins(0,0,0,0);
#endif

    // ATHLETE (FILE) MENU
    QMenu *fileMenu = menuBar()->addMenu(tr("&Athlete"));

    openWindowMenu = fileMenu->addMenu(tr("Open &Window"));
    openTabMenu = fileMenu->addMenu(tr("Open &Tab"));
    connect(openWindowMenu, SIGNAL(aboutToShow()), this, SLOT(setOpenWindowMenu()));
    connect(openTabMenu, SIGNAL(aboutToShow()), this, SLOT(setOpenTabMenu()));

    windowMapper = new QSignalMapper(this); // maps each option
    connect(windowMapper, SIGNAL(mapped(const QString &)), this, SLOT(openWindow(const QString &)));

    tabMapper = new QSignalMapper(this); // maps each option
    connect(tabMapper, SIGNAL(mapped(const QString &)), this, SLOT(openTab(const QString &)));

    fileMenu->addSeparator();
    backupAthleteMenu = fileMenu->addMenu(tr("Backup Athlete Data"));
    connect(backupAthleteMenu, SIGNAL(aboutToShow()), this, SLOT(setBackupAthleteMenu()));
    backupMapper = new QSignalMapper(this); // maps each option
    connect(backupMapper, SIGNAL(mapped(const QString &)), this, SLOT(backupAthlete(const QString &)));

    fileMenu->addSeparator();
    fileMenu->addAction(tr("Close Window"), this, SLOT(closeWindow()));
    fileMenu->addAction(tr("&Close Tab"), this, SLOT(closeTab()));
    fileMenu->addAction(tr("&Quit All Windows"), this, SLOT(closeAll()), tr("Ctrl+Q"));

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
    shareAction = new QAction(tr("Share Online..."), this);
    shareAction->setShortcut(tr("Ctrl+U"));
    connect(shareAction, SIGNAL(triggered(bool)), this, SLOT(share()));
    shareMenu->addAction(shareAction);
#ifdef GC_HAVE_KQOAUTH
    tweetAction = new QAction(tr("Tweet activity"), this);
    connect(tweetAction, SIGNAL(triggered(bool)), this, SLOT(tweetRide()));
    shareMenu->addAction(tweetAction);
#endif
#ifdef GC_HAVE_ICAL
    shareMenu->addSeparator();
    shareMenu->addAction(tr("Upload Activity to Calendar"), this, SLOT(uploadCalendar()), tr (""));
    //optionsMenu->addAction(tr("Import Calendar..."), this, SLOT(importCalendar()), tr ("")); // planned for v3.1
    //optionsMenu->addAction(tr("Export Calendar..."), this, SLOT(exportCalendar()), tr ("")); // planned for v3.1
    shareMenu->addAction(tr("Refresh Calendar"), this, SLOT(refreshCalendar()), tr (""));
#endif
    shareMenu->addSeparator ();
    shareMenu->addAction(tr("Write to Local Store"), this, SLOT(uploadLocalFileStore()));
    shareMenu->addAction(tr("Synchronise Local Store..."), this, SLOT(syncLocalFileStore()));
#if QT_VERSION > 0x050000
    shareMenu->addSeparator ();
    shareMenu->addAction(tr("Upload to &Dropbox"), this, SLOT(uploadDropbox()), tr("Ctrl+R"));
    shareMenu->addAction(tr("Synchronise Dropbox..."), this, SLOT(syncDropbox()), tr("Ctrl+O"));
    shareMenu->addSeparator ();
    shareMenu->addAction(tr("Upload to &GoogleDrive"), this,
                         SLOT(uploadGoogleDrive()), tr(""));
    shareMenu->addAction(tr("Synchronise GoogleDrive..."), this,
                         SLOT(syncGoogleDrive()), tr("Ctrl+P"));
#endif
#ifdef GC_HAVE_SOAP
    shareMenu->addSeparator ();
    shareMenu->addAction(tr("&Upload to TrainingPeaks"), this, SLOT(uploadTP()), tr("Ctrl+T"));
    shareMenu->addAction(tr("Synchronise TrainingPeaks..."), this, SLOT(downloadTP()), tr("Ctrl+L"));
#endif

    HelpWhatsThis *helpShare = new HelpWhatsThis(rideMenu);
    shareMenu->setWhatsThis(helpShare->getWhatsThisText(HelpWhatsThis::MenuBar_Share));

    // TOOLS MENU
    QMenu *optionsMenu = menuBar()->addMenu(tr("&Tools"));
    optionsMenu->addAction(tr("CP and W' Estimator..."), this, SLOT(showEstimateCP()));
    optionsMenu->addAction(tr("CP and W' Solver..."), this, SLOT(showSolveCP()));
    optionsMenu->addAction(tr("Air Density (Rho) Estimator..."), this, SLOT(showRhoEstimator()));
    optionsMenu->addAction(tr("VDOT and T-Pace Calculator..."), this, SLOT(showVDOTCalculator()));

    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Get &Withings Data..."), this,
                        SLOT (downloadMeasures()));
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Create a new workout..."), this, SLOT(showWorkoutWizard()));
    optionsMenu->addAction(tr("Download workouts from ErgDB..."), this, SLOT(downloadErgDB()));
    optionsMenu->addAction(tr("Import workouts, videos, videoSyncs..."), this, SLOT(importWorkout()));
    optionsMenu->addAction(tr("Scan disk for workouts, videos, videoSyncs..."), this, SLOT(manageLibrary()));

    optionsMenu->addAction(tr("Create Heat Map..."), this, SLOT(generateHeatMap()), tr(""));
    optionsMenu->addAction(tr("Export Metrics as CSV..."), this, SLOT(exportMetrics()), tr(""));

#ifdef GC_HAS_CLOUD_DB
    // CloudDB options
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("CloudDB Status..."), this, SLOT(cloudDBshowStatus()));
    QMenu *cloudDBMenu = optionsMenu->addMenu(tr("CloudDB Contributions"));
    cloudDBMenu->addAction(tr("Maintain charts"), this, SLOT(cloudDBuserEditChart()));

    if (CloudDBCommon::addCuratorFeatures) {
        QMenu *cloudDBCurator = optionsMenu->addMenu(tr("CloudDB Curator"));
        cloudDBCurator->addAction(tr("Curate charts"), this, SLOT(cloudDBcuratorEditChart()));
    }

#endif
#ifndef Q_OS_MAC
    optionsMenu->addSeparator();
#endif
    // options are always at the end of the tools menu
    optionsMenu->addAction(tr("&Options..."), this, SLOT(showOptions()));

    HelpWhatsThis *optionsMenuHelp = new HelpWhatsThis(optionsMenu);
    optionsMenu->setWhatsThis(optionsMenuHelp->getWhatsThisText(HelpWhatsThis::MenuBar_Tools));


    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    // Add all the data processors to the tools menu
    const DataProcessorFactory &factory = DataProcessorFactory::instance();
    QMap<QString, DataProcessor*> processors = factory.getProcessors();

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

    HelpWhatsThis *editMenuHelp = new HelpWhatsThis(editMenu);
    editMenu->setWhatsThis(editMenuHelp->getWhatsThisText(HelpWhatsThis::MenuBar_Edit));

    // VIEW MENU
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
#ifndef Q_OS_MAC
    viewMenu->addAction(tr("Toggle Full Screen"), this, SLOT(toggleFullScreen()), QKeySequence("F11"));
#else
    viewMenu->addAction(tr("Toggle Full Screen"), this, SLOT(toggleFullScreen()));
#endif
    showhideSidebar = viewMenu->addAction(tr("Show Left Sidebar"), this, SLOT(showSidebar(bool)));
    showhideSidebar->setCheckable(true);
    showhideSidebar->setChecked(true);
    showhideLowbar = viewMenu->addAction(tr("Show Compare Pane"), this, SLOT(showLowbar(bool)));
    showhideLowbar->setCheckable(true);
    showhideLowbar->setChecked(false);
#if (!defined Q_OS_MAC) || (QT_VERSION >= 0x50201) // not on a Mac
    showhideToolbar = viewMenu->addAction(tr("Show Toolbar"), this, SLOT(showToolbar(bool)));
    showhideToolbar->setCheckable(true);
    showhideToolbar->setChecked(true);
#endif
    showhideTabbar = viewMenu->addAction(tr("Show Athlete Tabs"), this, SLOT(showTabbar(bool)));
    showhideTabbar->setCheckable(true);
    showhideTabbar->setChecked(true);

    //connect(showhideSidebar, SIGNAL(triggered(bool)), this, SLOT(showSidebar(bool)));
    viewMenu->addSeparator();
    viewMenu->addAction(tr("Activities"), this, SLOT(selectAnalysis()));
    viewMenu->addAction(tr("Trends"), this, SLOT(selectHome()));
    viewMenu->addAction(tr("Train"), this, SLOT(selectTrain()));
#ifdef GC_HAVE_ICAL
    viewMenu->addAction(tr("Diary"), this, SLOT(selectDiary()));
#endif
    viewMenu->addSeparator();
    subChartMenu = viewMenu->addMenu(tr("Add Chart"));
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

    saveGCState(currentTab->context); // set to whatever we started with
    selectAnalysis();

    //grab focus
    currentTab->setFocus();

    installEventFilter(this);

    // catch config changes
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    configChanged(CONFIG_APPEARANCE);

    init = true;
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
    l->addWidget(progress);

    // lets go
    splash->setFixedSize(100,50);

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
    currentTab->toggleSidebar();
    setToolButtons();
}

void
MainWindow::showSidebar(bool want)
{
    currentTab->setSidebarEnabled(want);
    showhideSidebar->setChecked(want);
    setToolButtons();
}

void
MainWindow::toggleLowbar()
{
    if (currentTab->hasBottom()) currentTab->setBottomRequested(!currentTab->isBottomRequested());
    setToolButtons();
}

void
MainWindow::showLowbar(bool want)
{
    if (currentTab->hasBottom()) currentTab->setBottomRequested(want);
    showhideLowbar->setChecked(want);
    setToolButtons();
}

void
MainWindow::showTabbar(bool want)
{
    setUpdatesEnabled(false);
    showhideTabbar->setChecked(want);
    if (want) {
#ifdef Q_OS_MAC
    setDocumentMode(true);
    tabbar->setDocumentMode(true);
#if QT_VERSION >= 0x50201
    if (!GCColor::isFlat()) blackline->hide();
#endif
#endif
        tabbar->show();
    }
    else {
#ifdef Q_OS_MAC
    setDocumentMode(false);
    tabbar->setDocumentMode(false);
#if QT_VERSION >= 0x50201
    if (!GCColor::isFlat()) blackline->show();
#endif
#endif
        tabbar->hide();
    }
    setUpdatesEnabled(true);
}

void
MainWindow::showToolbar(bool want)
{
#if (!defined Q_OS_MAC) || (QT_VERSION >= 0x50201)
    setUpdatesEnabled(false);
    showhideToolbar->setChecked(want);
    if (want) {
        head->show();
    }
    else {
        head->hide();
    }
    setUpdatesEnabled(true);
#endif
}

void
MainWindow::setChartMenu()
{
    unsigned int mask=0;

    // called when chart menu about to be shown
    // setup to only show charts that are relevant
    // to this view
    switch(currentTab->currentView()) {
        case 0 : mask = VIEW_HOME; break;
        default:
        case 1 : mask = VIEW_ANALYSIS; break;
        case 2 : mask = VIEW_DIARY; break;
        case 3 : mask = VIEW_TRAIN; break;
        case 4 : mask = VIEW_INTERVAL; break;
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
    switch(currentTab->currentView()) {
        case 0 : mask = VIEW_HOME; break;
        default:
        case 1 : mask = VIEW_ANALYSIS; break;
        case 2 : mask = VIEW_DIARY; break;
        case 3 : mask = VIEW_TRAIN; break;
        case 4 : mask = VIEW_INTERVAL; break;
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
    GcWinID id = GcWindowTypes::None;
    for (int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].name == action->text()) {
            id = GcWindows[i].id;
            break;
        }
    }
    if (id != GcWindowTypes::None)
        currentTab->addChart(id); // called from MainWindow to inset chart
}

void
MainWindow::setStyleFromSegment(int segment)
{
    currentTab->setTiled(segment);
    styleAction->setChecked(!segment);
}

void
MainWindow::toggleStyle()
{
    currentTab->toggleTile();
    styleAction->setChecked(currentTab->isTiled());
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
        if (e->type() == QEvent::WindowStateChange) resizeEvent(NULL); // see below
    }
    return false;
}

void
MainWindow::resizeEvent(QResizeEvent*)
{
// on a mac we hide/show the toolbar on fullscreen mode
// when using QT5 since it has problems rendering
#if (defined Q_OS_MAC) && (QT_VERSION >= 0x50201)
    if (head) {
        QRect screenSize = desktop->availableGeometry();
        if ((screenSize.width() == frameGeometry().width() || screenSize.height() == frameGeometry().height()) && // fullscreen
           (head->isVisible())) {// and it is visible
            head->hide();
            head->updateGeometry();
            head->show();
            head->updateGeometry();
        }

        // painting
        head->repaint();
    }
#endif

    //appsettings->setValue(GC_SETTINGS_MAIN_GEOM, saveGeometry());
    //appsettings->setValue(GC_SETTINGS_MAIN_STATE, saveState());
}

void
MainWindow::showOptions()
{
    ConfigDialog *cd = new ConfigDialog(currentTab->context->athlete->home->root(), currentTab->context);

    // move to the centre of the screen
    cd->move(geometry().center()-QPoint(cd->geometry().width()/2, cd->geometry().height()/2));
    cd->show();
}

void
MainWindow::moveEvent(QMoveEvent*)
{
    appsettings->setValue(GC_SETTINGS_MAIN_GEOM, saveGeometry());
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    QList<Tab*> closing = tabList;
    bool needtosave = false;
    bool importrunning = false;

    // close all the tabs .. if any refuse we need to ignore
    //                       the close event
    foreach(Tab *tab, closing) {

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
                removeTab(tab);
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
#if QT_VERSION > 0x050200
        // wait for threads.. max of 10 seconds before just exiting anyway
        for (int i=0; i<10 && QThreadPool::globalInstance()->activeThreadCount(); i++) {
            QThread::sleep(1);
        }
#endif
    }
    appsettings->setValue(GC_SETTINGS_MAIN_GEOM, saveGeometry());
    appsettings->setValue(GC_SETTINGS_MAIN_STATE, saveState());
}

MainWindow::~MainWindow()
{
    // aside from the tabs, we may need to clean
    // up any dangling widgets created in MainWindow::MainWindow (?)
}

// global search/data filter
void MainWindow::setFilter(QStringList f) { currentTab->context->setFilter(f); }
void MainWindow::clearFilter() { currentTab->context->clearFilter(); }

void
MainWindow::closeAll()
{
    QList<MainWindow *> windows = mainwindows; // get a copy, since it is updated as closed

    foreach(MainWindow *window, windows)
        if (window != this)
            window->closeWindow();

    // now close us down!
    closeWindow();
}

void
MainWindow::aboutDialog()
{
    AboutDialog *ad = new AboutDialog(currentTab->context);
    ad->exec();
}

void MainWindow::showSolveCP()
{
   SolveCPDialog *td = new SolveCPDialog(this, currentTab->context);
   td->show();
}

void MainWindow::showEstimateCP()
{
   EstimateCPDialog *td = new EstimateCPDialog();
   td->show();
}

void MainWindow::showRhoEstimator()
{
   ToolsRhoEstimator *tre = new ToolsRhoEstimator(currentTab->context);
   tre->show();
}

void MainWindow::showVDOTCalculator()
{
   VDOTCalculator *VDOTcalculator = new VDOTCalculator();
   VDOTcalculator->show();
}

void MainWindow::showWorkoutWizard()
{
   WorkoutWizard *ww = new WorkoutWizard(currentTab->context);
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
        currentTab->resetLayout();
}

void MainWindow::manualProcess(QString name)
{
    // open a dialog box and let the users
    // configure the options to use
    // and also show the explanation
    // of what this function does
    // then call it!
    RideItem *rideitem = (RideItem*)currentTab->context->currentRideItem();
    if (rideitem) {
        ManualDataProcessorDialog *p = new ManualDataProcessorDialog(currentTab->context, name, rideitem);
        p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
        p->exec();
    }
}


void
MainWindow::helpWindow()
{
    HelpWindow* help = new HelpWindow(currentTab->context);
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
MainWindow::selectAnalysis()
{
    currentTab->selectView(1);
    setToolButtons();
}

void
MainWindow::selectTrain()
{
    currentTab->selectView(3);
    setToolButtons();
}

void
MainWindow::selectDiary()
{
    currentTab->selectView(2);
    setToolButtons();
}

void
MainWindow::selectHome()
{
    currentTab->selectView(0);
    setToolButtons();
}

void
MainWindow::selectInterval()
{
    currentTab->selectView(4);
    setToolButtons();
}

void
MainWindow::setToolButtons()
{
    int select = currentTab->isTiled() ? 1 : 0;
    int lowselected = currentTab->isBottomRequested() ? 1 : 0;

    styleAction->setChecked(select);
    showhideLowbar->setChecked(lowselected);

#ifdef Q_OS_MAC
    styleSelector->setSelected(select, true);
    lowbar->setSelected(lowselected);
#else
    if (styleSelector->isSegmentSelected(select) == false)
        styleSelector->setSegmentSelected(select, true);
#endif

    int index = currentTab->currentView();

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
    anim->stop();
    searchBox->clearFocus();
    searchFocusOut();
    scopebar->setFocus(Qt::TabFocusReason);
#endif
    scopebar->setSelected(index);
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
    for(int i=0; i<urls.count(); i++) {

        QString filename = QFileInfo(urls.value(i).toLocalFile()).absoluteFilePath();

        if (filename.endsWith(".xml", Qt::CaseInsensitive)) {

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

        } else {
            filenames.append(filename);
        }
    }

    // import any we may have extracted
    if (imported.count()) {

        // now append to the QList and QTreeWidget
        currentTab->context->athlete->presets += imported;

        // notify we changed and tree updates
        currentTab->context->notifyPresetsChanged();

        // tell the user
        QMessageBox::information(this, tr("Chart Import"), QString(tr("Imported %1 charts")).arg(imported.count()));

        // switch to trend view if we aren't on it
        selectHome();

        // now select what was added
        currentTab->context->notifyPresetSelected(currentTab->context->athlete->presets.count()-1);
    }


    // if there is anything left, process based upon view...
    if (filenames.count()) {

        if (currentTab->currentView() != 3) { // we're not on train view

            // We have something to process then
            RideImportWizard *dialog = new RideImportWizard (filenames, currentTab->context);
            dialog->process(); // do it!

        } else {
            Library::importFiles(currentTab->context, filenames);
        }
    }
    return;
}

/*----------------------------------------------------------------------
 * Ride Library Functions
 *--------------------------------------------------------------------*/


void
MainWindow::downloadRide()
{
    (new DownloadRideDialog(currentTab->context))->show();
}


void
MainWindow::manualRide()
{
    (new ManualRideDialog(currentTab->context))->show();
}

void
MainWindow::exportBatch()
{
    BatchExportDialog *d = new BatchExportDialog(currentTab->context);
    d->exec();
}

void
MainWindow::generateHeatMap()
{
    GenerateHeatMapDialog *d = new GenerateHeatMapDialog(currentTab->context);
    d->exec();
}

void
MainWindow::exportRide()
{
    if (currentTab->context->ride == NULL) {
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
    RideFile *currentRide = currentTab->context->ride ? currentTab->context->ride->ride() : NULL;
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
        result = RideFileFactory::instance().writeRideFile(currentTab->context, currentRide, file, fileNameSuffix);
    }
    else
    {
        // Use the value of drop down list to determine file type
        int idx = allFormats.indexOf(getSuffix.cap(0));

        if (idx>1) {

            result = RideFileFactory::instance().writeRideFile(currentTab->context, currentRide, file, getSuffix.cap(1));

        } else if (idx==0){

            CsvFileReader writer;
            result = writer.writeRideFile(currentTab->context, currentRide, file, CsvFileReader::gc);

        } else if (idx==1){

            CsvFileReader writer;
            result = writer.writeRideFile(currentTab->context, currentRide, file, CsvFileReader::wprime);

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
        RideImportWizard *import = new RideImportWizard(fileNamesCopy, currentTab->context);
        import->process();
    }
}

void
MainWindow::saveRide()
{
    // no ride
    if (currentTab->context->ride == NULL) {
        QMessageBox oops(QMessageBox::Critical, tr("No Activity To Save"),
                         tr("There is no currently selected activity to save."));
        oops.exec();
        return;
    }

    // flush in-flight changes
    currentTab->context->notifyMetadataFlush();
    currentTab->context->ride->notifyRideMetadataChanged();

    // nothing to do if not dirty
    //XXX FORCE A SAVE if (currentTab->context->ride->isDirty() == false) return;

    // save
    if (currentTab->context->ride) {
        saveRideSingleDialog(currentTab->context, currentTab->context->ride); // will signal save to everyone
    }
}

void
MainWindow::revertRide()
{
    currentTab->context->ride->close();
    currentTab->context->ride->ride(); // force re-load

    // in case reverted ride has different starttime
    currentTab->context->ride->setStartTime(currentTab->context->ride->ride()->startTime());
    currentTab->context->ride->ride()->emitReverted();

    // and notify everyone we changed which also has the side
    // effect of updating the cached values too
    currentTab->context->notifyRideSelected(currentTab->context->ride);
}

void
MainWindow::splitRide()
{
    if (currentTab->context->ride && currentTab->context->ride->ride() && currentTab->context->ride->ride()->dataPoints().count()) (new SplitActivityWizard(currentTab->context))->exec();
    else {
        if (!currentTab->context->ride || !currentTab->context->ride->ride())
            QMessageBox::critical(this, tr("Split Activity"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Split Activity"), tr("Current activity contains no data to split"));
    }
}

void
MainWindow::mergeRide()
{
    if (currentTab->context->ride && currentTab->context->ride->ride() && currentTab->context->ride->ride()->dataPoints().count()) (new MergeActivityWizard(currentTab->context))->exec();
    else {
        if (!currentTab->context->ride || !currentTab->context->ride->ride())
            QMessageBox::critical(this, tr("Split Activity"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Split Activity"), tr("Current activity contains no data to merge"));
    }
}

void
MainWindow::deleteRide()
{
    RideItem *_item = currentTab->context->ride;

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
        currentTab->context->athlete->removeCurrentRide();
}

/*----------------------------------------------------------------------
 * Realtime Devices and Workouts
 *--------------------------------------------------------------------*/
void
MainWindow::addDevice()
{

    // lets get a new one
    AddDeviceWizard *p = new AddDeviceWizard(currentTab->context);
    p->show();

}

/*----------------------------------------------------------------------
 * Cyclists
 *--------------------------------------------------------------------*/

void
MainWindow::newCyclistWindow()
{
    QDir newHome = currentTab->context->athlete->home->root();
    newHome.cdUp();
    QString name = ChooseCyclistDialog::newCyclistDialog(newHome, this);
    if (!name.isEmpty()) openWindow(name);
}

void
MainWindow::newCyclistTab()
{
    QDir newHome = currentTab->context->athlete->home->root();
    newHome.cdUp();
    QString name = ChooseCyclistDialog::newCyclistDialog(newHome, this);
    if (!name.isEmpty()) openTab(name);
}

void
MainWindow::openWindow(QString name)
{
    // open window...
    QDir home(gcroot);
    appsettings->initializeQSettingsGlobal(gcroot);
    home.cd(name);

    if (!home.exists()) return;
    appsettings->initializeQSettingsAthlete(gcroot, name);

    GcUpgrade v3;
    if (!v3.upgradeConfirmedByUser(home)) return;

    // main window will register itself
    MainWindow *main = new MainWindow(home);
    main->show();
    main->ridesAutoImport();
}

void
MainWindow::closeWindow()
{
    // just call close, we might do more later
    appsettings->syncQSettings();
    close();
}

void
MainWindow::openTab(QString name)
{
    QDir home(gcroot);
    appsettings->initializeQSettingsGlobal(gcroot);
    home.cd(name);

    if (!home.exists()) return;
    appsettings->initializeQSettingsAthlete(gcroot, name);

    GcUpgrade v3;
    if (!v3.upgradeConfirmedByUser(home)) return;

    setUpdatesEnabled(false);

    // splash screen - progress whilst loading tab
    setSplash();

    // bootstrap
    Context *context = new Context(this);
    context->athlete = new Athlete(context, home);

    // now open up a new tab
    currentTab = new Tab(context);

    // clear splash - progress whilst loading tab
    clearSplash();

    // first tab
    tabs.insert(currentTab->context->athlete->home->root().dirName(), currentTab);

    // stack, list and bar all share a common index
    tabList.append(currentTab);
    tabbar->addTab(currentTab->context->athlete->home->root().dirName());
    tabStack->addWidget(currentTab);

    // switch to newly created athlete
    tabbar->setCurrentIndex(tabList.count()-1);

    // kick off on analysis
    setWindowTitle(currentTab->context->athlete->home->root().dirName());
    selectAnalysis(); // sets scope bar ..

    // now apply current
    saveGCState(currentTab->context);
    restoreGCState(currentTab->context);

    // show the tabbar if we're gonna open tabs -- but wait till the last second
    // to show it to avoid crappy paint artefacts
    showTabbar(true);

    setUpdatesEnabled(true);

    // now do the automatic ride file import
    context->athlete->importFilesWhenOpeningAthlete();
}

void
MainWindow::closeTabClicked(int index)
{

    Tab *tab = tabList[index];

    // check for autoimport and let it finalize
    if (tab->context->athlete->autoImport) {
        if (tab->context->athlete->autoImport->importInProcess() ) {
            QMessageBox::information(this, tr("Activity Import"), tr("Closing of athlete window not possible while background activity import is in progress..."));
            return;
        }
    }

    if (saveRideExitDialog(tab->context) == false) return;

    // lets wipe it
    removeTab(tab);
}

bool
MainWindow::closeTab()
{
  // check for autoimport and let it finalize
    if (currentTab->context->athlete->autoImport) {
        if (currentTab->context->athlete->autoImport->importInProcess() ) {
            QMessageBox::information(this, tr("Activity Import"), tr("Closing of athlete window not possible while background activity import is in progress..."));
            return false;
        }
    }

    // wipe it down ...
    if (saveRideExitDialog(currentTab->context) == false) return false;

    // if its the last tab we close the window
    if (tabList.count() == 1)
        closeWindow();
    else {
        removeTab(currentTab);
    }
    appsettings->syncQSettings();
    // we did it
    return true;
}

// no questions asked just wipe away the current tab
void
MainWindow::removeTab(Tab *tab)
{
    setUpdatesEnabled(false);

    if (tabList.count() == 2) showTabbar(false); // don't need it for one!

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
        if (index) switchTab(index-1);
        else switchTab(index+1);
    }

    // close gracefully
    tab->close();
    tab->context->athlete->close();

    // remove from state
    tabs.remove(name);
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
MainWindow::setOpenWindowMenu()
{
    // wipe existing
    openWindowMenu->clear();

    // get a list of all cyclists
    QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {

        QString name = i.next();

        // new action
        QAction *action = new QAction(QString("%1").arg(name), this);

        // get the config directory
        AthleteDirectoryStructure subDirs(name);
        // icon / mugshot ?
        QString icon = QString("%1/%2/%3/avatar.png").arg(gcroot).arg(name).arg(subDirs.config().dirName());
        if (QFile(icon).exists()) action->setIcon(QIcon(icon));

        // only allow selection of cyclists which are not already open
        foreach (MainWindow *x, mainwindows) {
            QMapIterator<QString, Tab*> t(x->tabs);
            while (t.hasNext()) {
                t.next();
                if (t.key() == name)
                    action->setEnabled(false);
            }
        }

        // add to menu
        openWindowMenu->addAction(action);
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, name);
    }

    // add create new option
    openWindowMenu->addSeparator();
    openWindowMenu->addAction(tr("&New Athlete..."), this, SLOT(newCyclistWindow()), tr("Ctrl+N"));
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

        // new action
        QAction *action = new QAction(QString("%1").arg(name), this);

        // get the config directory
        AthleteDirectoryStructure subDirs(name);
        // icon / mugshot ?
        QString icon = QString("%1/%2/%3/avatar.png").arg(gcroot).arg(name).arg(subDirs.config().dirName());
        if (QFile(icon).exists()) action->setIcon(QIcon(icon));

        // only allow selection of cyclists which are not already open
        foreach (MainWindow *x, mainwindows) {
            QMapIterator<QString, Tab*> t(x->tabs);
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
MainWindow::saveGCState(Context *context)
{
    // save all the current state to the supplied context
    context->showSidebar = showhideSidebar->isChecked();
    //context->showTabbar = showhideTabbar->isChecked();
    context->showLowbar = showhideLowbar->isChecked();
#if (!defined Q_OS_MAC) || (QT_VERSION >= 0x50201) // not on a Mac
    context->showToolbar = showhideToolbar->isChecked();
#else
    context->showToolbar = true;
#endif
    context->searchText = searchBox->text();
    context->style = styleAction->isChecked();
    context->setIndex(scopebar->selected());
}

void
MainWindow::restoreGCState(Context *context)
{
    // restore window state from the supplied context
    showSidebar(context->showSidebar);
    showToolbar(context->showToolbar);
    //showTabbar(context->showTabbar);
    showLowbar(context->showLowbar);
    scopebar->setSelected(context->viewIndex);
    scopebar->setContext(context);
    scopebar->setHighlighted(); // to reflect context
    searchBox->setContext(context);
    searchBox->setText(context->searchText);
}

void
MainWindow::switchTab(int index)
{
    if (index < 0) return;

    setUpdatesEnabled(false);

#ifdef Q_OS_MAC // close buttons on the left on Mac
    // Only have close button on current tab (prettier)
    for(int i=0; i<tabbar->count(); i++) tabbar->tabButton(i, QTabBar::LeftSide)->hide();
    tabbar->tabButton(index, QTabBar::LeftSide)->show();
#else
    // Only have close button on current tab (prettier)
    for(int i=0; i<tabbar->count(); i++) tabbar->tabButton(i, QTabBar::RightSide)->hide();
    tabbar->tabButton(index, QTabBar::RightSide)->show();
#endif

    // save how we are
    saveGCState(currentTab->context);

    currentTab = tabList[index];
    tabStack->setCurrentIndex(index);

    // restore back
    restoreGCState(currentTab->context);

    setWindowTitle(currentTab->context->athlete->home->root().dirName());

    setUpdatesEnabled(true);
}


/*----------------------------------------------------------------------
 * MetricDB
 *--------------------------------------------------------------------*/

void
MainWindow::exportMetrics()
{
    // if the refresh process is running, try again when its completed
    if (currentTab->context->athlete->rideCache->isRunning()) {
        QMessageBox::warning(this, tr("Refresh in Progress"),
        "A metric refresh is currently running, please try again once that has completed.");
        return;
    }

    // all good lets choose a file
    QString fileName = QFileDialog::getSaveFileName( this, tr("Export Metrics"), QDir::homePath(), tr("Comma Separated Variables (*.csv)"));
    if (fileName.length() == 0) return;

    // export
    currentTab->context->athlete->rideCache->writeAsCSV(fileName);
}

/*----------------------------------------------------------------------
 * Twitter
 *--------------------------------------------------------------------*/
#ifdef GC_HAVE_KQOAUTH
void
MainWindow::tweetRide()
{
    RideItem *_item = currentTab->context->ride;
    if (_item==NULL) return;

    RideItem *item = dynamic_cast<RideItem*>(_item);

    if (item) { // menu is disabled anyway, but belt and braces
        TwitterDialog *twitterDialog = new TwitterDialog(currentTab->context, item);
        twitterDialog->setWindowModality(Qt::ApplicationModal);
        twitterDialog->exec();
    }
}
#endif

/*----------------------------------------------------------------------
* Share : Twitter, Strava, RideWithGPS
*--------------------------------------------------------------------*/

void
MainWindow::share()
{
    if (currentTab->context->ride) { // menu is disabled anyway, but belt and braces
        ShareDialog d(currentTab->context, currentTab->context->ride);
        d.exec();
    }
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
        Library::importFiles(currentTab->context, fileNamesCopy);
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
        ErgDBDownloadDialog *d = new ErgDBDownloadDialog(currentTab->context);
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
    LibrarySearchDialog *search = new LibrarySearchDialog(currentTab->context);
    search->exec();
}

/*----------------------------------------------------------------------
 * Dropbox.com
 *--------------------------------------------------------------------*/
#if QT_VERSION > 0x050000
void
MainWindow::uploadDropbox()
{
    // upload current ride, if we have one
    if (currentTab->context->ride) {
        Dropbox db(currentTab->context);
        FileStore::upload(this, &db, currentTab->context->ride);
    }
}

void
MainWindow::syncDropbox()
{
    Dropbox db(currentTab->context);
    FileStoreSyncDialog upload(currentTab->context, &db);
    upload.exec();
}

void
MainWindow::uploadGoogleDrive()
{
    // upload current ride, if we have one
    if (currentTab->context->ride) {
        GoogleDrive gd(currentTab->context);
        QStringList errors;        
        if (gd.open(errors) && errors.empty()) {
            // NOTE(gille): GoogleDrive is a little "wonky". We need to read
            // the directory before we can upload to it. It's just how it is..
            gd.readdir(gd.home(), errors);
            if (errors.empty()) {
                FileStore::upload(this, &gd, currentTab->context->ride);
            }
        } // TODO(gille): How to bail properly?
    }
}

void
MainWindow::syncGoogleDrive()
{
    GoogleDrive gd(currentTab->context);
    FileStoreSyncDialog upload(currentTab->context, &gd);
    upload.exec();
}
#endif

/*----------------------------------------------------------------------
 * Network File Share (e.g. a mounted WebDAV folder)
 *--------------------------------------------------------------------*/
void
MainWindow::uploadLocalFileStore()
{
    // upload current ride, if we have one
    if (currentTab->context->ride) {
        LocalFileStore db(currentTab->context);
        FileStore::upload(this, &db, currentTab->context->ride);
    }
}

void
MainWindow::syncLocalFileStore()
{
    // upload current ride, if we have one
    LocalFileStore db(currentTab->context);
    FileStoreSyncDialog upload(currentTab->context, &db);
    upload.exec();
}


/*----------------------------------------------------------------------
 * TrainingPeaks.com
 *--------------------------------------------------------------------*/

#ifdef GC_HAVE_SOAP
void
MainWindow::uploadTP()
{
    if (currentTab->context->ride) {
        TPUploadDialog uploader(currentTab->context->athlete->cyclist, currentTab->context->ride->ride(), currentTab->context);
        uploader.exec();
    }
}

void
MainWindow::downloadTP()
{
    TPDownloadDialog downloader(currentTab->context);
    downloader.exec();
}
#endif


/*----------------------------------------------------------------------
 * Utility
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 * Notifiers - application level events
 *--------------------------------------------------------------------*/

void
MainWindow::configChanged(qint32)
{

// Windows
#ifdef WIN32
    QString menuColorString = (GCColor::isFlat() ? GColor(CCHROME).name() : "rgba(225,225,225)");
    menuBar()->setStyleSheet(QString("QMenuBar { color: black; background: %1; }"
                             "QMenuBar::item { color: black; background: %1; }").arg(menuColorString));
#endif

// Mac
#ifdef Q_OS_MAC
    if (GCColor::isFlat()) {

#if (QT_VERSION >= 0x50201)
        // flat mode
        head->setStyleSheet(QString(" QToolBar:active { border: 0px; background-color: %1; } "
                            " QToolBar:!active { border: 0px; background-color: %1; }").arg(GColor(CCHROME).name()));
        blackline->hide();
#endif

    } else {

        // metallic mode
#if QT_VERSION >= 0x50201
        // black line back, but only if we aren't showing the tabbar
        if (!showhideTabbar->isChecked()) blackline->show();
        head->setStyleSheet(" QToolBar:!active { border: 0px; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F0F0F0, stop: 1 #E8E8E8 ); } "
                            " QToolBar:active { border: 0px; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #D9D9D9, stop: 1 #B5B5B5 ); } ");
#endif
    }
#endif

#ifndef Q_OS_MAC
    QPalette tabbarPalette;
    tabbar->setAutoFillBackground(true);
    tabbar->setShape(QTabBar::RoundedSouth);
    tabbar->setDrawBase(false);

    if (GCColor::isFlat())
        tabbarPalette.setBrush(backgroundRole(), GColor(CCHROME));
    else
        tabbarPalette.setBrush(backgroundRole(), QColor("#B3B4B6"));
    tabbar->setPalette(tabbarPalette);
#endif

    // set the default fontsize
    QFont font;
    font.fromString(appsettings->value(NULL, GC_FONT_DEFAULT, QFont().toString()).toString());
    font.setPointSize(appsettings->value(NULL, GC_FONT_DEFAULT_SIZE, 10).toInt());
    QApplication::setFont(font); // set default font

    head->updateGeometry();
    repaint();

}

/*----------------------------------------------------------------------
 * Measures
 *--------------------------------------------------------------------*/

void
MainWindow::downloadMeasures()
{
    currentTab->context->athlete->withingsDownload->download();
}

void
MainWindow::refreshCalendar()
{
#ifdef GC_HAVE_ICAL
    currentTab->context->athlete->davCalendar->download();
    currentTab->context->athlete->calendarDownload->download();
#endif
}

/*----------------------------------------------------------------------
 * Calendar
 *--------------------------------------------------------------------*/

#ifdef GC_HAVE_ICAL
void
MainWindow::uploadCalendar()
{
    if (currentTab->context->currentRideItem())
        currentTab->context->athlete->davCalendar->upload((RideItem*)currentTab->context->currentRideItem()); // remove const coz it updates the ride
                                               // to set GID and upload date
}
#endif

void
MainWindow::actionClicked(int index)
{
    switch(index) {

    default:
    case 0: currentTab->addIntervals();
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
    currentTab->addIntervals();
}

void
MainWindow::ridesAutoImport() {

    currentTab->context->athlete->importFilesWhenOpeningAthlete();

}

// grow/shrink searchbox if there is space...
void
MainWindow::searchFocusIn()
{
    if (searchBox->searchbox->hasFocus()) {
        anim->setDuration(300);
        anim->setEasingCurve(QEasingCurve::InOutQuad);
        anim->setStartValue(searchBox->width());
        anim->setEndValue(500);
        anim->start();
    }
}

void
MainWindow::searchFocusOut()
{
    anim->stop();
    searchBox->setFixedWidth(150);
}

#ifdef GC_HAS_CLOUD_DB
void
MainWindow::cloudDBuserEditChart()
{
    if (!(appsettings->cvalue(currentTab->context->athlete->cyclist, GC_CLOUDDB_TC_ACCEPTANCE, false).toBool())) {
       CloudDBAcceptConditionsDialog acceptDialog(currentTab->context->athlete->cyclist);
       acceptDialog.setModal(true);
       if (acceptDialog.exec() == QDialog::Rejected) {
          return;
       };
    }

    if (currentTab->context->cdbChartListDialog == NULL) {
        currentTab->context->cdbChartListDialog = new CloudDBChartListDialog();
    }

    // force refresh in prepare to allways get the latest data here
    if (currentTab->context->cdbChartListDialog->prepareData(currentTab->context->athlete->cyclist, CloudDBCommon::UserEdit)) {
        currentTab->context->cdbChartListDialog->exec(); // no action when closed
    }
}

void
MainWindow::cloudDBcuratorEditChart()
{
    // first check if the user is a curator
    CloudDBCuratorClient *curatorClient = new CloudDBCuratorClient;
    if (curatorClient->isCurator(appsettings->cvalue(currentTab->context->athlete->cyclist, GC_ATHLETE_ID, "" ).toString())) {

        if (currentTab->context->cdbChartListDialog == NULL) {
            currentTab->context->cdbChartListDialog = new CloudDBChartListDialog();
        }

        // force refresh in prepare to allways get the latest data here
        if (currentTab->context->cdbChartListDialog->prepareData(currentTab->context->athlete->cyclist, CloudDBCommon::CuratorEdit)) {
            currentTab->context->cdbChartListDialog->exec(); // no action when closed
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

