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
#include <QNetworkProxyQuery>

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
#include "BatchExportDialog.h"
#include "TwitterDialog.h"
#include "ShareDialog.h"
#include "TtbDialog.h"
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
#ifdef GC_HAVE_LION
#include "LionFullScreen.h" // mac and lion or later
#endif
#include "QtMacButton.h" // mac
#include "QtMacPopUpButton.h" // mac
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
    context = new Context(this);
    context->athlete = new Athlete(context, home);

    setInstanceName(context->athlete->cyclist);
    setWindowIcon(QIcon(":images/gc.png"));
    setWindowTitle(context->athlete->home.dirName());
    setContentsMargins(0,0,0,0);
    setAcceptDrops(true);
    GCColor *GCColorSet = new GCColor(context); // get/keep colorset
    GCColorSet->colorSet(); // shut up the compiler

    #ifdef Q_OS_MAC
    // get an autorelease pool setup
    static CocoaInitializer cocoaInitializer;
    #endif
    #ifdef GC_HAVE_WFAPI
    WFApi *w = WFApi::getInstance(); // ensure created on main thread
    w->apiVersion();//shutup compiler
    #endif
    Library::initialise(context->athlete->home);
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

#if (defined Q_OS_MAC) && (defined GC_HAVE_LION)
    fullScreen = new LionFullScreen(context);
#endif
#ifndef Q_OS_MAC
    fullScreen = new QTFullScreen(context);
#endif

    // if no workout directory is configured, default to the
    // top level GoldenCheetah directory
    if (appsettings->value(NULL, GC_WORKOUTDIR).toString() == "")
        appsettings->setValue(GC_WORKOUTDIR, QFileInfo(context->athlete->home.absolutePath() + "/../").absolutePath());

    /*----------------------------------------------------------------------
     *  GUI setup
     *--------------------------------------------------------------------*/

    // need to restore geometry before setUnifiedToolBar.. on Mac
    appsettings->setValue(GC_SETTINGS_LAST, context->athlete->home.dirName());
    QVariant geom = appsettings->value(this, GC_SETTINGS_MAIN_GEOM);
    if (geom == QVariant()) {

        // first run -- lets set some sensible defaults...
        // lets put it in the middle of screen 1
        QRect size = desktop->availableGeometry();
        struct SizeSettings app = GCColor::defaultSizes(size.height(), size.width());

        // center on the available screen (minus toolbar/sidebar)
        move((size.width()-size.x())/2 - app.width/2,
             (size.height()-size.y())/2 - app.height/2);

        // set to the right default
        resize(app.width, app.height);

        // set all the default font sizes
        appsettings->setValue(GC_FONT_DEFAULT_SIZE, app.defaultFont);
        appsettings->setValue(GC_FONT_TITLES_SIZE, app.titleFont);
        appsettings->setValue(GC_FONT_CHARTMARKERS_SIZE, app.markerFont);
        appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, app.labelFont);
        appsettings->setValue(GC_FONT_CALENDAR_SIZE, app.calendarFont);
        appsettings->setValue(GC_FONT_POPUP_SIZE, app.popupFont);

        // set the default fontsize
        QFont font;
        font.setPointSize(app.defaultFont);
        QApplication::setFont(font);

    } else {

        QRect size = desktop->availableGeometry();

        // ensure saved geometry isn't greater than current screen size
        if ((geom.toRect().height() >= size.height()) || (geom.toRect().width() >= size.width()))
            setGeometry(size.x()+30,size.y()+30,size.width()-60,size.height()-60);
        else
            setGeometry(geom.toRect());
    }


    /*----------------------------------------------------------------------
     *  Mac Toolbar
     *--------------------------------------------------------------------*/
#ifdef Q_OS_MAC 
    setUnifiedTitleAndToolBarOnMac(true);
    head = addToolBar(context->athlete->cyclist);
    head->setContentsMargins(0,0,0,0);

    // widgets
    QWidget *macAnalButtons = new QWidget(this);
    macAnalButtons->setContentsMargins(0,0,20,0);

    // lhs buttons
    QHBoxLayout *lb = new QHBoxLayout(macAnalButtons);
    lb->setContentsMargins(0,0,0,0);
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

    actbuttons = new QtMacSegmentedButton(3, acts);
    actbuttons->setWidth(115);
    actbuttons->setNoSelect();
    actbuttons->setImage(0, new QPixmap(":images/mac/stop.png"));
    actbuttons->setImage(1, new QPixmap(":images/mac/split.png"));
    actbuttons->setImage(2, new QPixmap(":images/mac/trash.png"));
    pp->addWidget(actbuttons);
    lb->addWidget(acts);
    lb->addStretch();
    connect(actbuttons, SIGNAL(clicked(int,bool)), this, SLOT(actionClicked(int)));

    lb->addWidget(new Spacer(this));

    QWidget *viewsel = new QWidget(this);
    viewsel->setContentsMargins(0,0,0,0);
    QHBoxLayout *pq = new QHBoxLayout(viewsel);
    pq->setContentsMargins(0,0,0,0);
    pq->setSpacing(5);
    pq->addWidget(sidebar);
    styleSelector = new QtMacSegmentedButton(2, viewsel);
    styleSelector->setWidth(80); // actually its 80 but we want a 30px space between is and the searchbox
    styleSelector->setImage(0, new QPixmap(":images/mac/tabbed.png"), 24);
    styleSelector->setImage(1, new QPixmap(":images/mac/tiled.png"), 24);
    pq->addWidget(styleSelector);
    connect(sidebar, SIGNAL(clicked(bool)), this, SLOT(toggleSidebar()));
    connect(styleSelector, SIGNAL(clicked(int,bool)), this, SLOT(toggleStyle()));

    // setup Mac thetoolbar
    head->addWidget(macAnalButtons);
    head->addWidget(new Spacer(this));
    head->addWidget(new Spacer(this));
    head->addWidget(viewsel);

#ifdef GC_HAVE_LUCENE
    SearchFilterBox *searchBox = new SearchFilterBox(this,context,false);
    QCleanlooksStyle *toolStyle = new QCleanlooksStyle();
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

    QCleanlooksStyle *toolStyle = new QCleanlooksStyle();
    QPalette metal;
    metal.setColor(QPalette::Button, QColor(215,215,215));

    // get those icons
    importIcon = iconFromPNG(":images/mac/download.png");
    composeIcon = iconFromPNG(":images/mac/compose.png");
    intervalIcon = iconFromPNG(":images/mac/stop.png");
    splitIcon = iconFromPNG(":images/mac/split.png");
    deleteIcon = iconFromPNG(":images/mac/trash.png");
    sidebarIcon = iconFromPNG(":images/mac/sidebar.png");
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
    compose->setToolTip(tr("Create Manual Activity"));
    compose->setPalette(metal);
    connect(compose, SIGNAL(clicked(bool)), this, SLOT(manualRide()));

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
    actbuttons->setSegmentToolTip(0, tr("Find Intervals"));
    actbuttons->setSegmentToolTip(1, tr("Split Activity"));
    actbuttons->setSegmentToolTip(2, tr("Delete Activity"));
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
    head->addWidget(sidebar);
    head->addWidget(styleSelector);

#ifdef GC_HAVE_LUCENE
    // add a search box on far right, but with a little space too
    SearchFilterBox *searchBox = new SearchFilterBox(this,context,false);
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
     * ScopeBar
     *--------------------------------------------------------------------*/
    scopebar = new GcScopeBar(context);
    connect(scopebar, SIGNAL(selectDiary()), this, SLOT(selectDiary()));
    connect(scopebar, SIGNAL(selectHome()), this, SLOT(selectHome()));
    connect(scopebar, SIGNAL(selectAnal()), this, SLOT(selectAnalysis()));
    connect(scopebar, SIGNAL(selectTrain()), this, SLOT(selectTrain()));

    // Add chart is on the scope bar
    chartMenu = new QMenu(this);
    QCleanlooksStyle *styler = new QCleanlooksStyle();
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

    /*----------------------------------------------------------------------
     * Central Widget
     *--------------------------------------------------------------------*/

    tab = new Tab(context);

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
    mainLayout->addWidget(scopebar);
    mainLayout->addWidget(tab);
    setCentralWidget(central);

    /*----------------------------------------------------------------------
     * Application Menus
     *--------------------------------------------------------------------*/
#ifdef WIN32
    menuBar()->setStyleSheet("QMenuBar { background: rgba(225,225,225); }"
		    	     "QMenuBar::item { background: rgba(225,225,225); }");
    menuBar()->setContentsMargins(0,0,0,0);
#endif

    QMenu *fileMenu = menuBar()->addMenu(tr("&Athlete"));
    fileMenu->addAction(tr("&New..."), this, SLOT(newCyclist()), tr("Ctrl+N"));
    fileMenu->addAction(tr("&Open..."), this, SLOT(openCyclist()), tr("Ctrl+O"));
    fileMenu->addAction(tr("&Close Window"), this, SLOT(close()), tr ("Ctrl+W"));
    fileMenu->addAction(tr("&Quit All Windows"), this, SLOT(closeAll()), tr("Ctrl+Q"));

    QMenu *rideMenu = menuBar()->addMenu(tr("A&ctivity"));
    rideMenu->addAction(tr("&Download from device..."), this, SLOT(downloadRide()), tr("Ctrl+D"));
    rideMenu->addAction(tr("&Import from file..."), this, SLOT (importFile()), tr ("Ctrl+I"));
    rideMenu->addAction(tr("&Manual activity entry..."), this, SLOT(manualRide()), tr("Ctrl+M"));
    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Export..."), this, SLOT(exportRide()), tr("Ctrl+E"));
    rideMenu->addAction(tr("&Batch export..."), this, SLOT(exportBatch()), tr("Ctrl+B"));
    rideMenu->addAction(tr("Export Metrics as CSV..."), this, SLOT(exportMetrics()), tr(""));
#ifdef GC_HAVE_SOAP
    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Upload to TrainingPeaks"), this, SLOT(uploadTP()), tr("Ctrl+U"));
    rideMenu->addAction(tr("Down&load from TrainingPeaks..."), this, SLOT(downloadTP()), tr("Ctrl+L"));
#endif

#ifdef GC_HAVE_LIBOAUTH
    tweetAction = new QAction(tr("Tweet Activity"), this);
    connect(tweetAction, SIGNAL(triggered(bool)), this, SLOT(tweetRide()));
    rideMenu->addAction(tweetAction);

    shareAction = new QAction(tr("Share (Strava, RideWithGPS)..."), this);
    connect(shareAction, SIGNAL(triggered(bool)), this, SLOT(share()));
    rideMenu->addAction(shareAction);
#endif

    ttbAction = new QAction(tr("Upload to Trainingstagebuch..."), this);
    connect(ttbAction, SIGNAL(triggered(bool)), this, SLOT(uploadTtb()));
    rideMenu->addAction(ttbAction);

    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Save activity"), this, SLOT(saveRide()), tr("Ctrl+S"));
    rideMenu->addAction(tr("D&elete activity..."), this, SLOT(deleteRide()));
    rideMenu->addAction(tr("Split &activity..."), this, SLOT(splitRide()));
    rideMenu->addAction(tr("Merge activities..."), this, SLOT(mergeRide()));
    rideMenu->addSeparator ();

    QMenu *optionsMenu = menuBar()->addMenu(tr("&Tools"));
    optionsMenu->addAction(tr("&Options..."), this, SLOT(showOptions()));
    optionsMenu->addAction(tr("Critical Power Estimator..."), this, SLOT(showTools()));
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
    optionsMenu->addAction(tr("Upload Activity to Calendar"), this, SLOT(uploadCalendar()), tr (""));
    //optionsMenu->addAction(tr("Import Calendar..."), this, SLOT(importCalendar()), tr ("")); // planned for v3.1
    //optionsMenu->addAction(tr("Export Calendar..."), this, SLOT(exportCalendar()), tr ("")); // planned for v3.1
    optionsMenu->addAction(tr("Refresh Calendar"), this, SLOT(refreshCalendar()), tr (""));
#endif
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Find intervals..."), this, SLOT(addIntervals()), tr (""));

    // Add all the data processors to the tools menu
    const DataProcessorFactory &factory = DataProcessorFactory::instance();
    QMap<QString, DataProcessor*> processors = factory.getProcessors();

    if (processors.count()) {

        optionsMenu->addSeparator();
        toolMapper = new QSignalMapper(this); // maps each option
        QMapIterator<QString, DataProcessor*> i(processors);
        connect(toolMapper, SIGNAL(mapped(const QString &)), this, SLOT(manualProcess(const QString &)));

        i.toFront();
        while (i.hasNext()) {
            i.next();
            // The localized processor name is shown in menu
            QAction *action = new QAction(QString("%1...").arg(i.value()->name()), this);
            optionsMenu->addAction(action);
            connect(action, SIGNAL(triggered()), toolMapper, SLOT(map()));
            toolMapper->setMapping(action, i.key());
        }
    }

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
#ifndef Q_OS_MAC
    viewMenu->addAction(tr("Toggle Full Screen"), this, SLOT(toggleFullScreen()), QKeySequence("F11"));
#endif
    showhideSidebar = viewMenu->addAction(tr("Show Left Sidebar"), this, SLOT(showSidebar(bool)));
    showhideSidebar->setCheckable(true);
    showhideSidebar->setChecked(true);
#ifndef Q_OS_MAC // not on a Mac
    QAction *showhideToolbar = viewMenu->addAction(tr("Show Toolbar"), this, SLOT(showToolbar(bool)));
    showhideToolbar->setCheckable(true);
    showhideToolbar->setChecked(true);
#endif

    styleAction = viewMenu->addAction(tr("Tabbed View"), this, SLOT(toggleStyle()));
    styleAction->setCheckable(true);
    styleAction->setChecked(true);

    //connect(showhideSidebar, SIGNAL(triggered(bool)), this, SLOT(showSidebar(bool)));
    viewMenu->addSeparator();
    viewMenu->addAction(tr("Analysis"), this, SLOT(selectAnalysis()));
    viewMenu->addAction(tr("Home"), this, SLOT(selectHome()));
    viewMenu->addAction(tr("Train"), this, SLOT(selectTrain()));
#ifdef GC_HAVE_ICAL
    viewMenu->addAction(tr("Diary"), this, SLOT(selectDiary()));
#endif
    viewMenu->addSeparator();
    subChartMenu = viewMenu->addMenu(tr("Add Chart"));
    viewMenu->addAction(tr("Reset Layout"), this, SLOT(resetWindowLayout()));

    windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(setWindowMenu()));
    connect(rideMenu, SIGNAL(aboutToShow()), this, SLOT(setActivityMenu()));
    connect(subChartMenu, SIGNAL(aboutToShow()), this, SLOT(setSubChartMenu()));
    connect(subChartMenu, SIGNAL(triggered(QAction*)), this, SLOT(addChart(QAction*)));
    connect(windowMenu, SIGNAL(triggered(QAction*)), this, SLOT(selectWindow(QAction*)));

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&User Guide"), this, SLOT(helpView()));
    helpMenu->addAction(tr("&Log a bug or feature request"), this, SLOT(logBug()));
    helpMenu->addSeparator();
    helpMenu->addAction(tr("&About GoldenCheetah"), this, SLOT(aboutDialog()));

    /*----------------------------------------------------------------------
     * Lets go, choose latest ride and get GUI up and running
     *--------------------------------------------------------------------*/

    // selects the latest ride in the list:
    if (context->athlete->allRides->childCount() != 0)
        context->athlete->treeWidget->setCurrentItem(context->athlete->allRides->child(context->athlete->allRides->childCount()-1));

    //XXX!!! We really do need a mechanism for showing if a ride needs saving...
    //connect(this, SIGNAL(rideDirty()), this, SLOT(enableSaveButton()));
    //connect(this, SIGNAL(rideClean()), this, SLOT(enableSaveButton()));

    // cpx aggregate cache check
    connect(context,SIGNAL(rideSelected(RideItem*)), this, SLOT(rideSelected(RideItem*)));

    // Kick off - select a ride and switch to Analysis View
    context->athlete->rideTreeWidgetSelectionChanged();
    selectAnalysis();
}

/*----------------------------------------------------------------------
 * GUI
 *--------------------------------------------------------------------*/

void
MainWindow::toggleSidebar()
{
    tab->toggleSidebar();
}

void
MainWindow::showSidebar(bool want)
{
    tab->setSidebarEnabled(want);
}

void
MainWindow::showToolbar(bool want)
{
    if (want) {
        head->show();
        scopebar->show();
    }
    else {
        head->hide();
        scopebar->hide();
    }
}

void
MainWindow::setChartMenu()
{
    unsigned int mask=0;

    // called when chart menu about to be shown
    // setup to only show charts that are relevant
    // to this view
    switch(tab->currentView()) {
        case 0 : mask = VIEW_HOME; break;
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
    unsigned int mask=0;
    // called when chart menu about to be shown
    // setup to only show charts that are relevant
    // to this view
    switch(tab->currentView()) {
        case 0 : mask = VIEW_HOME; break;
        default:
        case 1 : mask = VIEW_ANALYSIS; break;
        case 2 : mask = VIEW_DIARY; break;
        case 3 : mask = VIEW_TRAIN; break;
    }

    subChartMenu->clear();
    if (!mask) return;

    for(int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].relevance & mask) 
            subChartMenu->addAction(GcWindows[i].name);
    }
}

void
MainWindow::setActivityMenu()
{
    // enable/disable upload if already uploaded
    if (context->ride && context->ride->ride()) {

        
        QString activityId = context->ride->ride()->getTag("TtbExercise", "");
        if (activityId == "") ttbAction->setEnabled(true);
        else ttbAction->setEnabled(false);
        
    } else {
        ttbAction->setEnabled(false);
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
        tab->addChart(id); // called from MainWindow to inset chart
}

void
MainWindow::setWindowMenu()
{
    windowMenu->clear();
    foreach (MainWindow *m, mainwindows) {
        windowMenu->addAction(m->context->athlete->cyclist);
    }
}

void
MainWindow::selectWindow(QAction *act)
{
    foreach (MainWindow *m, mainwindows) {
        if (m->context->athlete->cyclist == act->text()) {
            m->activateWindow();
            m->raise();
            break;
        }
    }
}

void
MainWindow::setStyleFromSegment(int segment)
{
    tab->setTiled(segment);
    styleAction->setChecked(!segment);
}

void
MainWindow::toggleStyle()
{
    tab->toggleTile();
    styleAction->setChecked(tab->isTiled());
    setStyle();
}

#ifndef Q_OS_MAC
void
MainWindow::toggleFullScreen()
{
    if (fullScreen) fullScreen->toggle();
    else qDebug()<<"no fullscreen support compiled in.";
}
#endif

void
MainWindow::resizeEvent(QResizeEvent*)
{
    appsettings->setValue(GC_SETTINGS_MAIN_GEOM, geometry());
#ifdef Q_OS_MAC
    head->updateGeometry();
    repaint();
#endif
}

void
MainWindow::showOptions()
{
    ConfigDialog *cd = new ConfigDialog(context->athlete->home, context->athlete->zones_, context);
    cd->show();
}

void
MainWindow::moveEvent(QMoveEvent*)
{
    appsettings->setValue(GC_SETTINGS_MAIN_GEOM, geometry());
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    if (saveRideExitDialog() == false) event->ignore();
    else {

#ifdef GC_HAVE_LUCENE
        // save the named searches
        context->athlete->namedSearches->write();
#endif

        // clear the clipboard if neccessary
        QApplication::clipboard()->setText("");

        // close the tab
        tab->close();

        // close athlete 
        context->athlete->close();

        // now remove from the list
        if(mainwindows.removeOne(this) == false)
            qDebug()<<"closeEvent: mainwindows list error";

    }
}

MainWindow::~MainWindow()
{
    delete tab;
    delete context->athlete;
}

// global search/data filter
void MainWindow::setFilter(QStringList f) { context->setFilter(f); }
void MainWindow::clearFilter() { context->clearFilter(); }

void
MainWindow::closeAll()
{
    QList<MainWindow *> windows = mainwindows; // get a copy, since it is updated as closed

    foreach(MainWindow *window, windows) 
        if (window != this) window->close();

    // now close us down!
    close();
}

void
MainWindow::aboutDialog()
{
    AboutDialog *ad = new AboutDialog(context, context->athlete->home);
    ad->exec();
}

void MainWindow::showTools()
{
   ToolsDialog *td = new ToolsDialog();
   td->show();
}

void MainWindow::showRhoEstimator()
{
   ToolsRhoEstimator *tre = new ToolsRhoEstimator(context);
   tre->show();
}

void MainWindow::showWorkoutWizard()
{
   WorkoutWizard *ww = new WorkoutWizard(context);
   ww->show();
}

void MainWindow::resetWindowLayout()
{
    tab->resetLayout();
}

void MainWindow::manualProcess(QString name)
{
    // open a dialog box and let the users
    // configure the options to use
    // and also show the explanation
    // of what this function does
    // then call it!
    RideItem *rideitem = (RideItem*)context->currentRideItem();
    if (rideitem) {
        ManualDataProcessorDialog *p = new ManualDataProcessorDialog(context, name, rideitem);
        p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
        p->exec();
    }
}

void
MainWindow::logBug()
{
    QDesktopServices::openUrl(QUrl("http://www.goldencheetah.org/bug-tracker.html"));
}

void
MainWindow::helpView()
{
    QDesktopServices::openUrl(QUrl("http://www.goldencheetah.org/wiki.html"));
}

void
MainWindow::selectAnalysis()
{
    tab->selectView(1);
    setStyle();
}

void
MainWindow::selectTrain()
{
    tab->selectView(3);
    setStyle();
}

void
MainWindow::selectDiary()
{
    tab->selectView(2);
    setStyle();
}

void
MainWindow::selectHome()
{
    tab->selectView(0);
    setStyle();
}

void
MainWindow::setStyle()
{
    int select = tab->isTiled() ? 1 : 0;

#ifdef Q_OS_MAC
    styleSelector->setSelected(select, true);
#else
    if (styleSelector->isSegmentSelected(select) == false)
        styleSelector->setSegmentSelected(select, true);
#endif
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

    if (accept) event->acceptProposedAction(); // whatever you wanna drop we will try and process!
    else event->ignore();
}

void
MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    if (tab->currentView() != 3) { // we're not on train view
        // We have something to process then
        RideImportWizard *dialog = new RideImportWizard (&urls, context->athlete->home, context);
        dialog->process(); // do it!
    } else {
        QStringList filenames;
        for (int i=0; i<urls.count(); i++)
            filenames.append(QFileInfo(urls.value(i).toLocalFile()).absoluteFilePath());
        Library::importFiles(context, filenames);
    }
    return;
}

/*----------------------------------------------------------------------
 * Ride Library Functions
 *--------------------------------------------------------------------*/


void
MainWindow::downloadRide()
{
    (new DownloadRideDialog(context, context->athlete->home))->show();
}


void
MainWindow::manualRide()
{
    (new ManualRideDialog(context))->show();
}

void
MainWindow::exportBatch()
{
    BatchExportDialog *d = new BatchExportDialog(context);
    d->exec();
}

void
MainWindow::exportRide()
{
    if ((context->athlete->treeWidget->selectedItems().size() != 1)
        || (context->athlete->treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        QMessageBox::critical(this, tr("Select Activity"), tr("No activity selected!"));
        return;
    }

    // what format?
    const RideFileFactory &rff = RideFileFactory::instance();
    QStringList allFormats;
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
    bool result = RideFileFactory::instance().writeRideFile(context, context->currentRide(), file, getSuffix.cap(1));

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
        RideImportWizard *import = new RideImportWizard(fileNamesCopy, context->athlete->home, context);
        import->process();
    }
}

void
MainWindow::saveRide()
{
    if (context->ride)
        saveRideSingleDialog(context->ride); // will signal save to everyone
    else {
        QMessageBox oops(QMessageBox::Critical, tr("No Activity To Save"),
                         tr("There is no currently selected ride to save."));
        oops.exec();
        return;
    }
}

void
MainWindow::revertRide()
{
    context->ride->freeMemory();
    context->ride->ride(); // force re-load

    // in case reverted ride has different starttime
    context->ride->setStartTime(context->ride->ride()->startTime()); // Note: this will also signal rideSelected()
    context->ride->ride()->emitReverted();
}

void
MainWindow::splitRide()
{
    if (context->ride && context->ride->ride() && context->ride->ride()->dataPoints().count()) (new SplitActivityWizard(context))->exec();
    else {
        if (!context->ride || !context->ride->ride())
            QMessageBox::critical(this, tr("Split Activity"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Split Activity"), tr("Current activity contains no data to split"));
    }
}

void
MainWindow::mergeRide()
{
    if (context->ride && context->ride->ride() && context->ride->ride()->dataPoints().count()) (new MergeActivityWizard(context))->exec();
    else {
        if (!context->ride || !context->ride->ride())
            QMessageBox::critical(this, tr("Split Activity"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Split Activity"), tr("Current activity contains no data to merge"));
    }
}

void
MainWindow::deleteRide()
{
    QTreeWidgetItem *_item = context->athlete->treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE) {
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
        context->athlete->removeCurrentRide();
}

/*----------------------------------------------------------------------
 * Realtime Devices and Workouts
 *--------------------------------------------------------------------*/
void
MainWindow::addDevice()
{

    // lets get a new one
    AddDeviceWizard *p = new AddDeviceWizard(context);
    p->show();

}

/*----------------------------------------------------------------------
 * Cyclists
 *--------------------------------------------------------------------*/

void
MainWindow::newCyclist()
{
    QDir newHome = context->athlete->home;
    newHome.cdUp();
    QString name = ChooseCyclistDialog::newCyclistDialog(newHome, this);
    if (!name.isEmpty()) {
        newHome.cd(name);
        if (!newHome.exists()) return;
        MainWindow *main = new MainWindow(newHome);
        main->show();
    }
}

void
MainWindow::openCyclist()
{
    QDir newHome = context->athlete->home;
    newHome.cdUp();
    ChooseCyclistDialog d(newHome, false);
    d.setModal(true);
    if (d.exec() == QDialog::Accepted) {
        newHome.cd(d.choice());
        if (!newHome.exists()) return;
        MainWindow *main = new MainWindow(newHome);
        main->show();
    }
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
    context->athlete->metricDB->writeAsCSV(fileName);
}

/*----------------------------------------------------------------------
 * Twitter
 *--------------------------------------------------------------------*/
#ifdef GC_HAVE_LIBOAUTH
void
MainWindow::tweetRide()
{
    QTreeWidgetItem *_item = context->athlete->treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE) return;

    RideItem *item = dynamic_cast<RideItem*>(_item);

    if (item) { // menu is disabled anyway, but belt and braces
        TwitterDialog *twitterDialog = new TwitterDialog(context, item);
        twitterDialog->setWindowModality(Qt::ApplicationModal);
        twitterDialog->exec();
    }
}

/*----------------------------------------------------------------------
* Share : Twitter, Strava, RideWithGPS
*--------------------------------------------------------------------*/

void
MainWindow::share()
{
    QTreeWidgetItem *_item = context->athlete->treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE) return;

    RideItem *item = dynamic_cast<RideItem*>(_item);

    if (item) { // menu is disabled anyway, but belt and braces
        ShareDialog d(context, item);
        d.exec();
    }
}
#endif

/*----------------------------------------------------------------------
* trainingstagebuch.org
*--------------------------------------------------------------------*/

void
MainWindow::uploadTtb()
{
    QTreeWidgetItem *_item = context->athlete->treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE) return;

    RideItem *item = dynamic_cast<RideItem*>(_item);

    if (item) { // menu is disabled anyway, but belt and braces
        TtbDialog d(context, item);
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
        Library::importFiles(context, fileNamesCopy);
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
        ErgDBDownloadDialog *d = new ErgDBDownloadDialog(context);
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
    LibrarySearchDialog *search = new LibrarySearchDialog(context);
    search->exec();
}

/*----------------------------------------------------------------------
 * TrainingPeaks.com
 *--------------------------------------------------------------------*/

#ifdef GC_HAVE_SOAP
void
MainWindow::uploadTP()
{
    if (context->ride) {
        TPUploadDialog uploader(context->athlete->cyclist, context->currentRide(), context);
        uploader.exec();
    }
}

void
MainWindow::downloadTP()
{
    TPDownloadDialog downloader(context);
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
Context::notifyConfigChanged()
{
    // now tell everyone else
    configChanged();
}

void
Athlete::configChanged()
{
    // re-read Zones in case it changed
    QFile zonesFile(home.absolutePath() + "/power.zones");
    if (zonesFile.exists()) {
        if (!zones_->read(zonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("Zones File Error"),
                                 zones_->errorString());
        }
       else if (! zones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading Zones File"), zones_->warningString());
    }

    // reread HR zones
    QFile hrzonesFile(home.absolutePath() + "/hr.zones");
    if (hrzonesFile.exists()) {
        if (!hrzones_->read(hrzonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("HR Zones File Error"),
                                 hrzones_->errorString());
        }
       else if (! hrzones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading HR Zones File"), hrzones_->warningString());
    }

    QVariant unit = appsettings->cvalue(cyclist, GC_UNIT);
    useMetricUnits = (unit.toString() == GC_UNIT_METRIC);

}

/*----------------------------------------------------------------------
 * Measures
 *--------------------------------------------------------------------*/

void
MainWindow::downloadMeasures()
{
    context->athlete->withingsDownload->download();
}

void
MainWindow::downloadMeasuresFromZeo()
{
    context->athlete->zeoDownload->download();
}

void
MainWindow::refreshCalendar()
{
#ifdef GC_HAVE_ICAL
    context->athlete->davCalendar->download();
    context->athlete->calendarDownload->download();
#endif
}

/*----------------------------------------------------------------------
 * Calendar
 *--------------------------------------------------------------------*/

#ifdef GC_HAVE_ICAL
void
MainWindow::uploadCalendar()
{
    context->athlete->davCalendar->upload((RideItem*)context->currentRideItem()); // remove const coz it updates the ride
                                               // to set GID and upload date
}
#endif

void
MainWindow::actionClicked(int index)
{
    switch(index) {

    default:
    case 0: tab->addIntervals();
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
    tab->addIntervals();
}

void
MainWindow::rideSelected(RideItem*)
{

    // update the ride property on all widgets
    // to let them know they need to replot new
    // selected ride
    tab->setRide(context->ride);

    if (!context->ride) return;

    // refresh interval list for bottom left
    // first lets wipe away the existing intervals
    QList<QTreeWidgetItem *> intervals = context->athlete->allIntervals->takeChildren();
    for (int i=0; i<intervals.count(); i++) delete intervals.at(i);

    // now add the intervals for the current ride
    if (context->ride) { // only if we have a ride pointer
        RideFile *selected = context->ride->ride();
        if (selected) {
            // get all the intervals in the currently selected RideFile
            QList<RideFileInterval> intervals = selected->intervals();
            for (int i=0; i < intervals.count(); i++) {
                // add as a child to context->athlete->allIntervals
                IntervalItem *add = new IntervalItem(selected,
                                                        intervals.at(i).name,
                                                        intervals.at(i).start,
                                                        intervals.at(i).stop,
                                                        selected->timeToDistance(intervals.at(i).start),
                                                        selected->timeToDistance(intervals.at(i).stop),
                                                        context->athlete->allIntervals->childCount()+1);
                add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
                context->athlete->allIntervals->addChild(add);
            }
        }
    }
}

