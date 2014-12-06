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

// DATA STRUCTURES
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"

#include "Colors.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "Settings.h"
#include "ErgDB.h"
#include "Library.h"
#include "LibraryParser.h"
#include "TrainDB.h"
#include "GcUpgrade.h"

// DIALOGS / DOWNLOADS / UPLOADS
#include "AboutDialog.h"
#include "ChooseCyclistDialog.h"
#include "ConfigDialog.h"
#include "DownloadRideDialog.h"
#include "ManualRideDialog.h"
#include "RideImportWizard.h"
#include "ToolsDialog.h"
#include "ToolsRhoEstimator.h"
#include "SplitActivityWizard.h"
#include "MergeActivityWizard.h"
#include "GenerateHeatMapDialog.h"
#include "BatchExportDialog.h"
#include "TwitterDialog.h"
#include "ShareDialog.h"
#include "WithingsDownload.h"
#include "ZeoDownload.h"
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
#ifdef GC_HAVE_LUCENE
#include "Lucene.h"
#include "NamedSearch.h"
#include "SearchFilterBox.h"
#endif

#ifdef GC_HAVE_WFAPI
#include "WFApi.h"
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
#ifdef Q_OS_MAC
    head = NULL; // early resize event causes a crash
#endif

    // bootstrap
    Context *context = new Context(this);
    GCColor *GCColorSet = new GCColor(context); // get/keep colorset, before athlete...
    context->athlete = new Athlete(context, home);

    setWindowIcon(QIcon(":images/gc.png"));
    setWindowTitle(context->athlete->home->root().dirName());
    setContentsMargins(0,0,0,0);
    setAcceptDrops(true);

    #ifdef GC_HAVE_WFAPI
    WFApi *w = WFApi::getInstance(); // ensure created on main thread
    w->apiVersion();//shutup compiler
    #endif
    GCColorSet->colorSet(); // shut up the compiler
    Library::initialise(context->athlete->home->root());
    QNetworkProxyQuery npq(QUrl("http://www.google.com"));
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
    if (listOfProxies.count() > 0) {
        QNetworkProxy::setApplicationProxy(listOfProxies.first());
    }

    if (desktop == NULL) desktop = QApplication::desktop();
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

    // widgets
    QWidget *macAnalButtons = new QWidget(this);
    macAnalButtons->setContentsMargins(20,5,20,0);

#else
    setUnifiedTitleAndToolBarOnMac(true);
    head = addToolBar(context->athlete->cyclist);
    head->setContentsMargins(0,0,0,0);

    // widgets
    QWidget *macAnalButtons = new QWidget(this);
    macAnalButtons->setContentsMargins(0,0,20,0);

#endif

    // lhs buttons
    QHBoxLayout *lb = new QHBoxLayout(macAnalButtons);
    lb->setContentsMargins(0,0,10,8);
    lb->setSpacing(0);
    import = new QtMacButton(this, QtMacButton::TexturedRounded);
    QPixmap *importImg = new QPixmap(":images/mac/download.png");
    import->setImage(importImg);
    import->setToolTip("Download");
    lb->addWidget(import);
    lb->addWidget(new Spacer(this));
    compose = new QtMacButton(this, QtMacButton::TexturedRounded);
    QPixmap *composeImg = new QPixmap(":images/mac/compose.png");
    compose->setImage(composeImg);
    compose->setToolTip("Create");
    lb->addWidget(compose);

    // connect to actions
    connect(import, SIGNAL(clicked(bool)), this, SLOT(downloadRide()));
    connect(compose, SIGNAL(clicked(bool)), this, SLOT(manualRide()));

    lb->addWidget(new Spacer(this));

    // activity actions .. peaks, split, delete
    QWidget *acts = new QWidget(this);
    acts->setContentsMargins(0,0,0,0);
    QHBoxLayout *pp = new QHBoxLayout(acts);
    pp->setContentsMargins(0,0,0,0);
    pp->setContentsMargins(0,0,0,0);
    pp->setSpacing(5);
    sidebar = new QtMacButton(this, QtMacButton::TexturedRounded);
    QPixmap *sidebarImg = new QPixmap(":images/mac/sidebar.png");
    sidebar->setImage(sidebarImg);
    sidebar->setMinimumSize(25, 25);
    sidebar->setMaximumSize(25, 25);
    sidebar->setToolTip("Sidebar");
    sidebar->setSelected(true); // assume always start up with sidebar selected

    lowbar = new QtMacButton(this, QtMacButton::TexturedRounded);
    QPixmap *lowbarImg = new QPixmap(":images/mac/lowbar.png");
    lowbar->setImage(lowbarImg);
    lowbar->setMinimumSize(25, 25);
    lowbar->setMaximumSize(25, 25);
    lowbar->setToolTip("Compare");
    lowbar->setSelected(false); // assume always start up with lowbar deselected

    actbuttons = new QtMacSegmentedButton(3, acts);
    actbuttons->setWidth(115);
    actbuttons->setNoSelect();
    actbuttons->setImage(0, new QPixmap(":images/mac/stop.png"));
    actbuttons->setImage(1, new QPixmap(":images/mac/split.png"));
    actbuttons->setImage(2, new QPixmap(":images/mac/trash.png"));
    pp->addWidget(actbuttons);
    lb->addWidget(acts);
    connect(actbuttons, SIGNAL(clicked(int,bool)), this, SLOT(actionClicked(int)));

    lb->addWidget(new Spacer(this));

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
    ps->addStretch();
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
    head->addWidget(macAnalButtons);
    head->addWidget(new Spacer(this));
    head->addWidget(scopebar);
    head->addWidget(new Spacer(this));
    head->addWidget(viewsel);

#ifdef GC_HAVE_LUCENE
    searchBox = new SearchFilterBox(this,context,false);
#if QT_VERSION > 0x50000
    QStyle *toolStyle = QStyleFactory::create("fusion");
#else
    QStyle *toolStyle = QStyleFactory::create("Cleanlooks");
#endif
    searchBox->setStyle(toolStyle);
    searchBox->setFixedWidth(200);
    head->addWidget(searchBox);
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
#endif

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
    importIcon = iconFromPNG(":images/mac/download.png");
    composeIcon = iconFromPNG(":images/mac/compose.png");
    intervalIcon = iconFromPNG(":images/mac/stop.png");
    splitIcon = iconFromPNG(":images/mac/split.png");
    deleteIcon = iconFromPNG(":images/mac/trash.png");
    sidebarIcon = iconFromPNG(":images/mac/sidebar.png");
    lowbarIcon = iconFromPNG(":images/mac/lowbar.png");
    tabbedIcon = iconFromPNG(":images/mac/tabbed.png");
    tiledIcon = iconFromPNG(":images/mac/tiled.png");
    QSize isize(19,19);

    Spacer *spacerl = new Spacer(this);
    spacerl->setFixedWidth(5);

    import = new QPushButton(this);
    import->setIcon(importIcon);
    import->setIconSize(isize);
    import->setFixedHeight(25);
    import->setStyle(toolStyle);
    import->setToolTip(tr("Download from Device"));
    import->setPalette(metal);
    connect(import, SIGNAL(clicked(bool)), this, SLOT(downloadRide()));

    compose = new QPushButton(this);
    compose->setIcon(composeIcon);
    compose->setIconSize(isize);
    compose->setFixedHeight(25);
    compose->setStyle(toolStyle);
    compose->setToolTip(tr("Create Manual Ride"));
    compose->setPalette(metal);
    connect(compose, SIGNAL(clicked(bool)), this, SLOT(manualRide()));

    lowbar = new QPushButton(this);
    lowbar->setIcon(lowbarIcon);
    lowbar->setIconSize(isize);
    lowbar->setFixedHeight(25);
    lowbar->setStyle(toolStyle);
    lowbar->setToolTip(tr("Toggle Compare Pane"));
    lowbar->setPalette(metal);
    connect(lowbar, SIGNAL(clicked(bool)), this, SLOT(toggleLowbar()));

    sidebar = new QPushButton(this);
    sidebar->setIcon(sidebarIcon);
    sidebar->setIconSize(isize);
    sidebar->setFixedHeight(25);
    sidebar->setStyle(toolStyle);
    sidebar->setToolTip(tr("Toggle Sidebar"));
    sidebar->setPalette(metal);
    connect(sidebar, SIGNAL(clicked(bool)), this, SLOT(toggleSidebar()));

    actbuttons = new QtSegmentControl(this);
    actbuttons->setStyle(toolStyle);
    actbuttons->setIconSize(isize);
    actbuttons->setCount(3);
    actbuttons->setSegmentIcon(0, intervalIcon);
    actbuttons->setSegmentIcon(1, splitIcon);
    actbuttons->setSegmentIcon(2, deleteIcon);
    actbuttons->setSelectionBehavior(QtSegmentControl::SelectNone); //wince. spelling. ugh
    actbuttons->setFixedHeight(25);
    actbuttons->setSegmentToolTip(0, tr("Find Intervals..."));
    actbuttons->setSegmentToolTip(1, tr("Split Ride..."));
    actbuttons->setSegmentToolTip(2, tr("Delete Ride"));
    actbuttons->setPalette(metal);
    connect(actbuttons, SIGNAL(segmentSelected(int)), this, SLOT(actionClicked(int)));

    styleSelector = new QtSegmentControl(this);
    styleSelector->setStyle(toolStyle);
    styleSelector->setIconSize(isize);
    styleSelector->setCount(2);
    styleSelector->setSegmentIcon(0, tabbedIcon);
    styleSelector->setSegmentIcon(1, tiledIcon);
    styleSelector->setSegmentToolTip(0, tr("Tabbed View"));
    styleSelector->setSegmentToolTip(1, tr("Tiled View"));
    styleSelector->setSelectionBehavior(QtSegmentControl::SelectOne); //wince. spelling. ugh
    styleSelector->setFixedHeight(25);
    styleSelector->setPalette(metal);
    connect(styleSelector, SIGNAL(segmentSelected(int)), this, SLOT(setStyleFromSegment(int))); //avoid toggle infinitely

    head->addWidget(spacerl);
    head->addWidget(import);
    head->addWidget(compose);
    head->addWidget(actbuttons);

    head->addStretch();
    head->addWidget(scopebar);
    head->addStretch();
    head->addWidget(sidebar);
    head->addWidget(lowbar);
    head->addWidget(styleSelector);

#ifdef GC_HAVE_LUCENE
    // add a search box on far right, but with a little space too
    searchBox = new SearchFilterBox(this,context,false);
    searchBox->setStyle(toolStyle);
    searchBox->setFixedWidth(200);
    head->addWidget(searchBox);
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
#endif
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
    currentTab = new Tab(context);

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
    fileMenu->addAction(tr("Close Window"), this, SLOT(closeWindow()));
    fileMenu->addAction(tr("&Close Tab"), this, SLOT(closeTab()));
    fileMenu->addAction(tr("&Quit All Windows"), this, SLOT(closeAll()), tr("Ctrl+Q"));

    // ACTIVITY MENU
    QMenu *rideMenu = menuBar()->addMenu(tr("A&ctivity"));
    rideMenu->addAction(tr("&Download from device..."), this, SLOT(downloadRide()), tr("Ctrl+D"));
    rideMenu->addAction(tr("&Import from file..."), this, SLOT (importFile()), tr ("Ctrl+I"));
    rideMenu->addAction(tr("&Manual ride entry..."), this, SLOT(manualRide()), tr("Ctrl+M"));
    rideMenu->addSeparator ();
    shareAction = new QAction(tr("Share Online..."), this);
    shareAction->setShortcut(tr("Ctrl+U"));
    connect(shareAction, SIGNAL(triggered(bool)), this, SLOT(share()));
    rideMenu->addAction(shareAction);
    rideMenu->addAction(tr("&Export..."), this, SLOT(exportRide()), tr("Ctrl+E"));
    rideMenu->addAction(tr("&Batch export..."), this, SLOT(exportBatch()), tr("Ctrl+B"));
#ifdef GC_HAVE_SOAP
    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Upload to TrainingPeaks"), this, SLOT(uploadTP()), tr("Ctrl+T"));
    rideMenu->addAction(tr("Synchronise TrainingPeaks..."), this, SLOT(downloadTP()), tr(""));
#endif

#ifdef GC_HAVE_LIBOAUTH
    tweetAction = new QAction(tr("Tweet Ride"), this);
    connect(tweetAction, SIGNAL(triggered(bool)), this, SLOT(tweetRide()));
    rideMenu->addAction(tweetAction);
#endif

    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Save ride"), this, SLOT(saveRide()), tr("Ctrl+S"));
    rideMenu->addAction(tr("D&elete ride..."), this, SLOT(deleteRide()));
    rideMenu->addAction(tr("Split &ride..."), this, SLOT(splitRide()));
    rideMenu->addAction(tr("Combine rides..."), this, SLOT(mergeRide()));
    rideMenu->addSeparator ();

    // TOOLS MENU
    QMenu *optionsMenu = menuBar()->addMenu(tr("&Tools"));
    optionsMenu->addAction(tr("&Options..."), this, SLOT(showOptions()));
    optionsMenu->addAction(tr("CP and W' Estimator..."), this, SLOT(showTools()));
    optionsMenu->addAction(tr("Air Density (Rho) Estimator..."), this, SLOT(showRhoEstimator()));

    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Get &Withings Data..."), this,
                        SLOT (downloadMeasures()));
    optionsMenu->addAction(tr("Get &Zeo Data..."), this,
                        SLOT (downloadMeasuresFromZeo()));
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Create a new workout..."), this, SLOT(showWorkoutWizard()));
    optionsMenu->addAction(tr("Download workouts from ErgDB..."), this, SLOT(downloadErgDB()));
    optionsMenu->addAction(tr("Import workouts or videos..."), this, SLOT(importWorkout()));
    optionsMenu->addAction(tr("Scan disk for videos and workouts..."), this, SLOT(manageLibrary()));

#ifdef GC_HAVE_ICAL
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Upload Ride to Calendar"), this, SLOT(uploadCalendar()), tr (""));
    //optionsMenu->addAction(tr("Import Calendar..."), this, SLOT(importCalendar()), tr ("")); // planned for v3.1
    //optionsMenu->addAction(tr("Export Calendar..."), this, SLOT(exportCalendar()), tr ("")); // planned for v3.1
    optionsMenu->addAction(tr("Refresh Calendar"), this, SLOT(refreshCalendar()), tr (""));
#endif
    optionsMenu->addAction(tr("Create Heat Map..."), this, SLOT(generateHeatMap()), tr(""));
    optionsMenu->addAction(tr("Export Metrics as CSV..."), this, SLOT(exportMetrics()), tr(""));
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Find intervals..."), this, SLOT(addIntervals()), tr (""));

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

    // VIEW MENU
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
#ifndef Q_OS_MAC
    viewMenu->addAction(tr("Toggle Full Screen"), this, SLOT(toggleFullScreen()), QKeySequence("F11"));
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
    viewMenu->addAction(tr("Rides"), this, SLOT(selectAnalysis()));
    viewMenu->addAction(tr("Trends"), this, SLOT(selectHome()));
    viewMenu->addAction(tr("Intervals"), this, SLOT(selectInterval()));
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

    // HELP MENU
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&User Guide"), this, SLOT(helpView()));
    helpMenu->addAction(tr("&Log a bug or feature request"), this, SLOT(logBug()));
    helpMenu->addAction(tr("&Discussion and Support Forum"), this, SLOT(support()));
    helpMenu->addSeparator();
    helpMenu->addAction(tr("&About GoldenCheetah"), this, SLOT(aboutDialog()));

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
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    configChanged();

    init = true;
}

/*----------------------------------------------------------------------
 * GUI
 *--------------------------------------------------------------------*/

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
    if (currentTab->hasBottom()) currentTab->setShowBottom(!currentTab->isShowBottom());
    setToolButtons();
}

void
MainWindow::showLowbar(bool want)
{
    if (currentTab->hasBottom()) currentTab->setShowBottom(want);
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

#ifndef Q_OS_MAC
void
MainWindow::toggleFullScreen()
{
    if (fullScreen) fullScreen->toggle();
    else qDebug()<<"no fullscreen support compiled in.";
}
#endif

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
#ifdef Q_OS_MAC
    if (head) {
        head->updateGeometry();
        repaint();
    }
#endif
    appsettings->setValue(GC_SETTINGS_MAIN_GEOM, saveGeometry());
    appsettings->setValue(GC_SETTINGS_MAIN_STATE, saveState());
}

void
MainWindow::showOptions()
{
    ConfigDialog *cd = new ConfigDialog(currentTab->context->athlete->home->root(), currentTab->context->athlete->zones_, currentTab->context);

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

    // close all the tabs .. if any refuse we need to ignore
    //                       the close event
    foreach(Tab *tab, closing) {

        // do we need to save?
        if (tab->context->mainWindow->saveRideExitDialog(tab->context) == true)
            removeTab(tab);
        else
            needtosave = true;
    }

    // were any left hanging around?
    if (needtosave) event->ignore();
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

void MainWindow::showTools()
{
   ToolsDialog *td = new ToolsDialog();
   td->show();
}

void MainWindow::showRhoEstimator()
{
   ToolsRhoEstimator *tre = new ToolsRhoEstimator(currentTab->context);
   tre->show();
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
MainWindow::logBug()
{
    QDesktopServices::openUrl(QUrl("https://github.com/GoldenCheetah/GoldenCheetah/issues"));
}

void
MainWindow::helpView()
{
    QDesktopServices::openUrl(QUrl("http://www.goldencheetah.org/wiki.html"));
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
    int lowselected = currentTab->isShowBottom() ? 1 : 0;

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

    if (currentTab->currentView() != 3) { // we're not on train view
        // We have something to process then
        RideImportWizard *dialog = new RideImportWizard (&urls, currentTab->context);
        dialog->process(); // do it!
    } else {
        QStringList filenames;
        for (int i=0; i<urls.count(); i++)
            filenames.append(QFileInfo(urls.value(i).toLocalFile()).absoluteFilePath());
        Library::importFiles(currentTab->context, filenames);
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
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    // what format?
    const RideFileFactory &rff = RideFileFactory::instance();
    QStringList allFormats;
    foreach(QString suffix, rff.writeSuffixes())
        allFormats << QString("%1 (*.%2)").arg(rff.description(suffix)).arg(suffix);

    QString suffix; // what was selected?
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Ride"), QDir::homePath(), allFormats.join(";;"), &suffix);

    if (fileName.length() == 0) return;

    // which file type was selected
    // extract from the suffix returned
    QRegExp getSuffix("^[^(]*\\(\\*\\.([^)]*)\\)$");
    getSuffix.exactMatch(suffix);

    QFile file(fileName);
    RideFile *currentRide = currentTab->context->ride ? currentTab->context->ride->ride() : NULL;
    bool result = RideFileFactory::instance().writeRideFile(currentTab->context, currentRide, file, getSuffix.cap(1));

    if (result == false) {
        QMessageBox oops(QMessageBox::Critical, tr("Export Failed"),
                         tr("Failed to export ride, please check permissions"));
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
    // only save if neccessary
    if (currentTab->context->ride->isDirty()) currentTab->context->notifyMetadataFlush();
    else return;

    if (currentTab->context->ride)
        saveRideSingleDialog(currentTab->context, currentTab->context->ride); // will signal save to everyone
    else {
        QMessageBox oops(QMessageBox::Critical, tr("No Ride To Save"),
                         tr("There is no currently selected ride to save."));
        oops.exec();
        return;
    }
}

void
MainWindow::revertRide()
{
    currentTab->context->ride->close();
    currentTab->context->ride->ride(); // force re-load

    // in case reverted ride has different starttime
    currentTab->context->ride->setStartTime(currentTab->context->ride->ride()->startTime()); // Note: this will also signal rideSelected()
    currentTab->context->ride->ride()->emitReverted();
}

void
MainWindow::splitRide()
{
    if (currentTab->context->ride && currentTab->context->ride->ride() && currentTab->context->ride->ride()->dataPoints().count()) (new SplitActivityWizard(currentTab->context))->exec();
    else {
        if (!currentTab->context->ride || !currentTab->context->ride->ride())
            QMessageBox::critical(this, tr("Split Ride"), tr("No ride selected"));
        else
            QMessageBox::critical(this, tr("Split Ride"), tr("Current ride contains no data to split"));
    }
}

void
MainWindow::mergeRide()
{
    if (currentTab->context->ride && currentTab->context->ride->ride() && currentTab->context->ride->ride()->dataPoints().count()) (new MergeActivityWizard(currentTab->context))->exec();
    else {
        if (!currentTab->context->ride || !currentTab->context->ride->ride())
            QMessageBox::critical(this, tr("Split Ride"), tr("No ride selected"));
        else
            QMessageBox::critical(this, tr("Split Ride"), tr("Current ride contains no data to merge"));
    }
}

void
MainWindow::deleteRide()
{
    RideItem *_item = currentTab->context->ride;

    if (_item==NULL) { 
        QMessageBox::critical(this, tr("Delete Ride"), tr("No ride selected!"));
        return;
    }

    RideItem *item = static_cast<RideItem*>(_item);
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to delete the ride:"));
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
    home.cd(name);

    if (!home.exists()) return;

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
    close();
}

void
MainWindow::openTab(QString name)
{
    QDir home(gcroot);
    home.cd(name);

    if (!home.exists()) return;

    GcUpgrade v3;
    if (!v3.upgradeConfirmedByUser(home)) return;

    setUpdatesEnabled(false);

    // bootstrap
    Context *context = new Context(this);
    context->athlete = new Athlete(context, home);

    // now open up a new tab
    currentTab = new Tab(context);

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
    if (saveRideExitDialog(tab->context) == false) return;

    // lets wipe it
    removeTab(tab);
}

bool
MainWindow::closeTab()
{
    // wipe it down ...
    if (saveRideExitDialog(currentTab->context) == false) return false;

    // if its the last tab we close the window
    if (tabList.count() == 1)
        closeWindow();
    else {
        removeTab(currentTab);
    }

    // we did it
    return true;
}

// no questions asked just wipe away the current tab
void
MainWindow::removeTab(Tab *tab)
{
    setUpdatesEnabled(false);

    if (tabList.count() == 2) showTabbar(false); // don't need it for one!

#ifdef GC_HAVE_LUCENE
    // save the named searches
    tab->context->athlete->namedSearches->write();
#endif

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
MainWindow::saveGCState(Context *context)
{
    // save all the current state to the supplied context
    context->showSidebar = showhideSidebar->isChecked();
    //context->showTabbar = showhideTabbar->isChecked();
    context->showLowbar = showhideLowbar->isChecked();
#ifndef Q_OS_MAC // not on a Mac
    context->showToolbar = showhideToolbar->isChecked();
#endif
#ifdef GC_HAVE_LUCENE
    context->searchText = searchBox->text();
#endif
    context->viewIndex = scopebar->selected();
    context->style = styleAction->isChecked();
    context->viewIndex = scopebar->selected();
}

void
MainWindow::restoreGCState(Context *context)
{
    // restore window state from the supplied context
    showSidebar(context->showSidebar);
#ifndef Q_OS_MAC // not on a Mac
    showToolbar(context->showToolbar);
#endif
    //showTabbar(context->showTabbar);
    showLowbar(context->showLowbar);
    scopebar->setSelected(context->viewIndex);
    scopebar->setContext(context);
    scopebar->setHighlighted(); // to reflect context
#ifdef GC_HAVE_LUCENE
    searchBox->setContext(context);
    searchBox->setText(context->searchText);
#endif
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
    QString fileName = QFileDialog::getSaveFileName( this, tr("Export Metrics"), QDir::homePath(), tr("Comma Separated Variables (*.csv)"));
    if (fileName.length() == 0)
        return;
    currentTab->context->athlete->metricDB->writeAsCSV(fileName);
}

/*----------------------------------------------------------------------
 * Twitter
 *--------------------------------------------------------------------*/
#ifdef GC_HAVE_LIBOAUTH
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
        "The workout directory is not configured, or the directory"
        " selected no longer exists.\n\n"
        "Please check your preference settings.");
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
MainWindow::configChanged()
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
MainWindow::downloadMeasuresFromZeo()
{
    currentTab->context->athlete->zeoDownload->download();
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

