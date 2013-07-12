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

#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "AboutDialog.h"
#include "AddIntervalDialog.h"
#include "BestIntervalDialog.h"
#include "BlankState.h"
#include "ChooseCyclistDialog.h"
#include "Colors.h"
#include "ConfigDialog.h"
#include "PwxRideFile.h"
#include "TcxRideFile.h"
#include "GcRideFile.h"
#include "JsonRideFile.h"
#ifdef GC_HAVE_KML
#include "KmlRideFile.h"
#endif
#include "DownloadRideDialog.h"
#include "ManualRideDialog.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "IntervalSummaryWindow.h"
#ifdef GC_HAVE_ICAL
#include "DiaryWindow.h"
#endif
#include "RideNavigator.h"
#include "RideFile.h"
#include "RideFileCache.h"
#include "RideImportWizard.h"
#include "RideMetadata.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include "Zones.h"

#include "DatePickerDialog.h"
#include "ToolsDialog.h"
#include "ToolsRhoEstimator.h"
#include "MetricAggregator.h"
#include "SplitActivityWizard.h"
#include "BatchExportDialog.h"
#include "RideWithGPSDialog.h"
#include "TtbDialog.h"
#include "WithingsDownload.h"
#include "ZeoDownload.h"
#include "CalendarDownload.h"
#include "WorkoutWizard.h"
#include "ErgDB.h"
#include "ErgDBDownloadDialog.h"
#include "DeviceConfiguration.h"
#include "AddDeviceWizard.h"
#include "TrainTool.h"

#include "GcWindowTool.h"
#include "GcToolBar.h"
#include "GcSideBarItem.h"
#ifdef GC_HAVE_SOAP
#include "TPUploadDialog.h"
#include "TPDownloadDialog.h"
#endif
#ifdef GC_HAVE_ICAL
#include "ICalendar.h"
#include "CalDAV.h"
#endif
#include "HelpWindow.h"
#include "HomeWindow.h"
#include "GcBubble.h"
#include "GcCalendar.h"
#include "GcScopeBar.h"
#include "LTMSidebar.h"

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

#ifdef GC_HAVE_LUCENE
#include "Lucene.h"
#include "NamedSearch.h"
#include "SearchFilterBox.h"
#endif

#include "ChartSettings.h"

#include <assert.h>
#include <QApplication>
#include <QtGui>
#include <QRegExp>
#include <QNetworkProxyQuery>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_grid.h>
#include <qwt_series_data.h>

#include "Library.h"
#include "LibraryParser.h"
#include "TrainDB.h"

#ifdef GC_HAVE_WFAPI
#include "WFApi.h"
#endif

#include "GcUpgrade.h" // upgrade wizard
#include "GcCrashDialog.h" // recovering from a crash?

// handy spacer
class Spacer : public QWidget
{
public:
    Spacer(QWidget *parent) : QWidget(parent) {
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setSizePolicy(sizePolicy);
    }
    QSize sizeHint() const { return QSize(10, 1); }
};

QList<MainWindow *> mainwindows; // keep track of all the MainWindows we have open
QDesktopWidget *desktop = NULL;

MainWindow::MainWindow(const QDir &home) :
    session(0), isclean(false), init(false), isfiltered(false), groupByMapper(NULL)
{
    setAttribute(Qt::WA_DeleteOnClose);

    // Bootstrap context 
    context = new Context(this);
    context->athlete = new Athlete(context, home);
    setInstanceName(context->athlete->cyclist);

    //
    // CRASH PROCESSING
    //

    // Recovering from a crash?
    if(!appsettings->cvalue(context->athlete->cyclist, GC_SAFEEXIT, true).toBool()) {
        GcCrashDialog *crashed = new GcCrashDialog(context->athlete->home);
        crashed->exec();
    }
    appsettings->setCValue(context->athlete->home.dirName(), GC_SAFEEXIT, false); // will be set to true on exit

    //
    // UPGRADE PROCESSING
    //

    // Before we initialise we need to run the upgrade wizard
    GcUpgrade v3;
    if (v3.upgrade(context->athlete->home) != 0) {
        hide();
        QTimer::singleShot(0, this, SLOT(close())); // wait for event loop
    } else { // don't indent since it would change entire file
    init=true;
    // !!! the code below is not indented since it would change the entire
    //     constructor for this small update. Please bear this in mind

    //
    // NORMAL PROCESSING (CONSTRUCTOR)
    //
    #ifdef Q_OS_MAC
    // get an autorelease pool setup
    static CocoaInitializer cocoaInitializer;
    #endif

    #ifdef GC_HAVE_WFAPI
    WFApi *w = WFApi::getInstance(); // ensure created on main thread
    w->apiVersion();//shutup compiler
    #endif

    if (desktop == NULL) desktop = QApplication::desktop();
    static const QIcon hideIcon(":images/toolbar/main/hideside.png");
    static const QIcon rhideIcon(":images/toolbar/main/hiderside.png");
    static const QIcon showIcon(":images/toolbar/main/showside.png");
    static const QIcon rshowIcon(":images/toolbar/main/showrside.png");
    static const QIcon tabIcon(":images/toolbar/main/tab.png");
    static const QIcon tileIcon(":images/toolbar/main/tile.png");
    static const QIcon fullIcon(":images/toolbar/main/togglefull.png");

    /*----------------------------------------------------------------------
     *  Basic State / Config
     *--------------------------------------------------------------------*/
    mainwindows.append(this);  // add us to the list of open windows

    // search paths
    Library::initialise(context->athlete->home);

    // Network proxy
    QNetworkProxyQuery npq(QUrl("http://www.google.com"));
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
    if (listOfProxies.count() > 0) {
        QNetworkProxy::setApplicationProxy(listOfProxies.first());
    }

    /*----------------------------------------------------------------------
     *  Basic GUI setup
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


#ifdef Q_OS_MAC // MAC NATIVE TOOLBAR
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
    searchBox->setFixedWidth(300);
    head->addWidget(searchBox);
#endif

#endif // MAC NATIVE TOOLBAR AND SCOPEBAR

    // COMMON GUI SETUP
    setWindowIcon(QIcon(":images/gc.png"));
    setWindowTitle(context->athlete->home.dirName());
    setContentsMargins(0,0,0,0);
    setAcceptDrops(true);

    GCColor *GCColorSet = new GCColor(context); // get/keep colorset
    GCColorSet->colorSet(); // shut up the compiler

#if (defined Q_OS_MAC) && (defined GC_HAVE_LION)
    fullScreen = new LionFullScreen(this);
#endif
#ifndef Q_OS_MAC
    fullScreen = new QTFullScreen(this);
#endif

    /*----------------------------------------------------------------------
     * The help bubble used everywhere
     *--------------------------------------------------------------------*/
    bubble = new GcBubble(context);
    bubble->hide();

    // if no workout directory is configured, default to the
    // top level GoldenCheetah directory
    if (appsettings->value(NULL, GC_WORKOUTDIR).toString() == "")
        appsettings->setValue(GC_WORKOUTDIR, QFileInfo(context->athlete->home.absolutePath() + "/../").absolutePath());

    /*----------------------------------------------------------------------
     * Non-Mac Toolbar
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
    searchBox->setFixedWidth(250);
    head->addWidget(searchBox);
#endif
    Spacer *spacer = new Spacer(this);
    spacer->setFixedWidth(5);
    head->addWidget(spacer);
#endif

    /*----------------------------------------------------------------------
     * Scope Bar
     *--------------------------------------------------------------------*/
    trainTool = new TrainTool(context, context->athlete->home);
    trainTool->hide();
    trainTool->getToolbarButtons()->hide(); // no show yet

    scopebar = new GcScopeBar(context, trainTool->getToolbarButtons());
    connect(scopebar, SIGNAL(selectDiary()), this, SLOT(selectDiary()));
    connect(scopebar, SIGNAL(selectHome()), this, SLOT(selectHome()));
    connect(scopebar, SIGNAL(selectAnal()), this, SLOT(selectAnalysis()));
    connect(scopebar, SIGNAL(selectTrain()), this, SLOT(selectTrain()));

    // add chart button with a menu
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
     * Sidebar
     *--------------------------------------------------------------------*/

    // RIDES

    // OLD Ride List (retained for legacy)
    treeWidget = new QTreeWidget;
    treeWidget->setColumnCount(3);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    // TODO: Test this on various systems with differing font settings (looks good on Leopard :)
    treeWidget->header()->resizeSection(0,60);
    treeWidget->header()->resizeSection(1,100);
    treeWidget->header()->resizeSection(2,70);
    treeWidget->header()->hide();
    treeWidget->setAlternatingRowColors (false);
    treeWidget->setIndentation(5);
    treeWidget->hide();

    allRides = new QTreeWidgetItem(treeWidget, FOLDER_TYPE);
    allRides->setText(0, tr("All Activities"));
    treeWidget->expandItem(allRides);
    treeWidget->setFirstItemColumnSpanned (allRides, true);

    // UI Ride List (configurable)
    listView = new RideNavigator(context, true);
    listView->setProperty("nomenu", true);

    // sidebar items
    gcCalendar = new GcCalendar(context);
    gcMultiCalendar = new GcMultiCalendar(context);

    // we need to connect the search box on Linux/Windows
#ifdef GC_HAVE_LUCENE

    // link to the sidebars
    connect(searchBox, SIGNAL(searchResults(QStringList)), listView, SLOT(searchStrings(QStringList)));
    connect(searchBox, SIGNAL(searchResults(QStringList)), gcCalendar, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchResults(QStringList)), gcMultiCalendar, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), listView, SLOT(clearSearch()));
    connect(searchBox, SIGNAL(searchClear()), gcCalendar, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchClear()), gcMultiCalendar, SLOT(clearFilter()));

    // and global for charts AFTER sidebars
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(searchResults(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(searchClear()));
#endif
    // retrieve settings (properties are saved when we close the window)
    if (appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGS, "").toString() != "") {
        listView->setSortByIndex(appsettings->cvalue(context->athlete->cyclist, GC_SORTBY).toInt());
        listView->setSortByOrder(appsettings->cvalue(context->athlete->cyclist, GC_SORTBYORDER).toInt());
        listView->setGroupBy(appsettings->cvalue(context->athlete->cyclist, GC_NAVGROUPBY).toInt());
        listView->setColumns(appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGS).toString());
        listView->setWidths(appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGWIDTHS).toString());
    }

    // INTERVALS
    intervalSummaryWindow = new IntervalSummaryWindow(context);
    intervalWidget = new IntervalTreeView(context);
    intervalWidget->setColumnCount(1);
    intervalWidget->setIndentation(5);
    intervalWidget->setSortingEnabled(false);
    intervalWidget->header()->hide();
    intervalWidget->setAlternatingRowColors (false);
    intervalWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    intervalWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    intervalWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    intervalWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    intervalWidget->setFrameStyle(QFrame::NoFrame);

    //allIntervals = new QTreeWidgetItem(intervalWidget, FOLDER_TYPE);
    allIntervals = intervalWidget->invisibleRootItem();
    allIntervals->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);

    allIntervals->setText(0, tr("Intervals"));

    QWidget *activityHistory = new QWidget(this);
    activityHistory->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // not on mac thanks
    activityHistory->setStyleSheet("padding: 0px; border: 0px; margin: 0px;");
#endif
    QVBoxLayout *activityLayout = new QVBoxLayout(activityHistory);
    activityLayout->setSpacing(0);
    activityLayout->setContentsMargins(0,0,0,0);
    activityLayout->addWidget(listView);

    intervalSplitter = new QSplitter(this);
    intervalSplitter->setHandleWidth(1);
    intervalSplitter->setOrientation(Qt::Vertical);
    intervalSplitter->addWidget(intervalWidget);
    intervalSplitter->addWidget(intervalSummaryWindow);
    intervalSplitter->setFrameStyle(QFrame::NoFrame);
    intervalSplitter->setCollapsible(0, false);
    intervalSplitter->setCollapsible(1, false);

    GcSplitterItem *calendarItem = new GcSplitterItem(tr("Calendar"), iconFromPNG(":images/sidebar/calendar.png"), this);
    calendarItem->addWidget(gcMultiCalendar);

    analItem = new GcSplitterItem(tr("Activities"), iconFromPNG(":images/sidebar/folder.png"), this);
    QAction *moreAnalAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    analItem->addAction(moreAnalAct);
    connect(moreAnalAct, SIGNAL(triggered(void)), this, SLOT(analysisPopup()));
    analItem->addWidget(activityHistory);

    intervalItem = new GcSplitterItem(tr("Intervals"), iconFromPNG(":images/mac/stop.png"), this);
    QAction *moreIntervalAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    intervalItem->addAction(moreIntervalAct);
    connect(moreIntervalAct, SIGNAL(triggered(void)), this, SLOT(intervalPopup()));
    intervalItem->addWidget(intervalSplitter);

    analSidebar = new GcSplitter(Qt::Vertical);
    analSidebar->addWidget(calendarItem);
    analSidebar->addWidget(analItem);
    analSidebar->addWidget(intervalItem);
    analSidebar->prepare(context->athlete->cyclist, "analysis");

    QTreeWidgetItem *last = NULL;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(context->athlete->home));
    while (i.hasNext()) {
        QString name = i.next();
        QDateTime dt;
        if (parseRideFileName(name, &dt)) {
            last = new RideItem(RIDE_TYPE, context->athlete->home.path(), name, dt, context->athlete->zones(), context->athlete->hrZones(), context);
            allRides->addChild(last);
        }
    }

    splitter = new QSplitter;

    // TOOLBOX - IS A SPLITTER
    toolBox = new QStackedWidget(this);
    toolBox->setAcceptDrops(true);
    toolBox->setFrameStyle(QFrame::NoFrame);
    toolBox->setContentsMargins(0,0,0,0);
    toolBox->layout()->setSpacing(0);
    splitter->addWidget(toolBox);

    // CONTAINERS FOR TOOLBOX
    masterControls = new QStackedWidget(this);
    masterControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    masterControls->setCurrentIndex(0);          // default to Analysis
    masterControls->setContentsMargins(0,0,0,0);
    analysisControls = new QStackedWidget(this);
    analysisControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    analysisControls->setCurrentIndex(0);          // default to Analysis
    analysisControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(analysisControls);
    trainControls = new QStackedWidget(this);
    trainControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    trainControls->setCurrentIndex(0);          // default to Analysis
    trainControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(trainControls);
    diaryControls = new QStackedWidget(this);
    diaryControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    diaryControls->setCurrentIndex(0);          // default to Analysis
    diaryControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(diaryControls);
    homeControls = new QStackedWidget(this);
    homeControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    homeControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(homeControls);

    // HOME WINDOW & CONTROLS
    homeWindow = new HomeWindow(context, "home", "Home");
    homeControls->addWidget(homeWindow->controls());
    homeControls->setCurrentIndex(0);

    // DIARY WINDOW & CONTROLS
    diaryWindow = new HomeWindow(context, "diary", "Diary");
    diaryControls->addWidget(diaryWindow->controls());

    // TRAIN WINDOW & CONTROLS
    trainWindow = new HomeWindow(context, "train", "Training");
    trainWindow->controls()->hide();
    trainControls->addWidget(trainWindow->controls());

    // ANALYSIS WINDOW & CONTRAOLS
    analWindow = new HomeWindow(context, "analysis", "Analysis");
    analysisControls->addWidget(analWindow->controls());

    currentWindow = NULL;

    // NO RIDE WINDOW - Replace analysis, home and train window when no ride

    // did we say we din't want to see this?
    showBlankTrain = !(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_TRAIN, false).toBool());
    showBlankHome = !(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_HOME, false).toBool());
    showBlankAnal = !(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_ANALYSIS, false).toBool());
    showBlankDiary = !(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_DIARY, false).toBool());

    // setup the blank pages
    blankStateAnalysisPage = new BlankStateAnalysisPage(context);
    blankStateHomePage = new BlankStateHomePage(context);
    blankStateDiaryPage = new BlankStateDiaryPage(context);
    blankStateTrainPage = new BlankStateTrainPage(context);

    connect(blankStateDiaryPage, SIGNAL(closeClicked()), this, SLOT(closeBlankDiary()));
    connect(blankStateHomePage, SIGNAL(closeClicked()), this, SLOT(closeBlankHome()));
    connect(blankStateAnalysisPage, SIGNAL(closeClicked()), this, SLOT(closeBlankAnal()));
    connect(blankStateTrainPage, SIGNAL(closeClicked()), this, SLOT(closeBlankTrain()));


    // POPULATE TOOLBOX

    // do controllers after home windows -- they need their first signals caught
    connect(gcCalendar, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChangedDiary(DateRange)));

    ltmSidebar = new LTMSidebar(context, context->athlete->home);
    connect(ltmSidebar, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChangedLTM(DateRange)));
    ltmSidebar->dateRangeTreeWidgetSelectionChanged(); // force an update to get first date range shown

    toolBox->addWidget(analSidebar);
    toolBox->addWidget(gcCalendar);
    toolBox->addWidget(trainTool->controls());
    toolBox->addWidget(ltmSidebar);

    // Chart Settings now in their own dialog box
    chartSettings = new ChartSettings(this, masterControls);
    chartSettings->setMaximumWidth(450);
    chartSettings->setMaximumHeight(600);

    //toolBox->addItem(masterControls, QIcon(":images/settings.png"), "Chart Settings");
    chartSettings->hide();

    /*----------------------------------------------------------------------
     * Main view
     *--------------------------------------------------------------------*/

    views = new QStackedWidget(this);
    views->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    views->setMinimumWidth(500);

    // add all the layouts
    views->addWidget(analWindow);
    views->addWidget(trainWindow);
    views->addWidget(diaryWindow);
    views->addWidget(homeWindow);

    // add Blank State pages
    views->addWidget(blankStateAnalysisPage);
    views->addWidget(blankStateHomePage);
    views->addWidget(blankStateDiaryPage);
    views->addWidget(blankStateTrainPage);

    //views->setCurrentIndex(0);
    views->setContentsMargins(0,0,0,0);

    QWidget *sviews = new QWidget(this);
    sviews->setContentsMargins(0,0,0,0);
    QVBoxLayout *sviewLayout = new QVBoxLayout(sviews);
    sviewLayout->setContentsMargins(0,0,0,0);
    sviewLayout->setSpacing(0);
    sviewLayout->addWidget(scopebar);
    sviewLayout->addWidget(views);
    splitter->addWidget(sviews);

    splitter->setStretchFactor(0,0);
    splitter->setStretchFactor(1,1);
    splitter->setCollapsible(0, true);
    splitter->setCollapsible(1, false);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(" QSplitter::handle { background-color: rgb(120,120,120); color: darkGray; }");
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0, 0, 0, 0); // attempting to follow some UI guides

    QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, GC_SETTINGS_SPLITTER_SIZES); 
    if (splitterSizes.toByteArray().size() > 1 ) {
        splitter->restoreState(splitterSizes.toByteArray());
        splitter->setOpaqueResize(true); // redraw when released, snappier UI
    }

    // CENTRAL LAYOUT
    QWidget *central = new QWidget(this);
    central->setContentsMargins(0,0,0,0);

    QVBoxLayout *centralLayout = new QVBoxLayout(central);
    centralLayout->setSpacing(0);
    centralLayout->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // nonmac toolbar on main view -- its not 
                 // unified with the title bar.
    centralLayout->addWidget(head);
#endif
    centralLayout->addWidget(splitter);

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

    rideWithGPSAction = new QAction(tr("Upload to RideWithGPS..."), this);
    connect(rideWithGPSAction, SIGNAL(triggered(bool)), this, SLOT(uploadRideWithGPSAction()));
    rideMenu->addAction(rideWithGPSAction);

    ttbAction = new QAction(tr("Upload to Trainingstagebuch..."), this);
    connect(ttbAction, SIGNAL(triggered(bool)), this, SLOT(uploadTtb()));
    rideMenu->addAction(ttbAction);

    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Save activity"), this, SLOT(saveRide()), tr("Ctrl+S"));
    rideMenu->addAction(tr("D&elete activity..."), this, SLOT(deleteRide()));
    rideMenu->addAction(tr("Split &activity..."), this, SLOT(splitRide()));
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
    if (allRides->childCount() != 0)
        treeWidget->setCurrentItem(allRides->child(allRides->childCount()-1));

    // now we're up and runnning lets connect the signals
    connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(rideTreeWidgetSelectionChanged()));
    connect(intervalWidget,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showContextMenuPopup(const QPoint &)));
    connect(intervalWidget,SIGNAL(itemSelectionChanged()), this, SLOT(intervalTreeWidgetSelectionChanged()));
    connect(intervalWidget,SIGNAL(itemChanged(QTreeWidgetItem *,int)), this, SLOT(intervalEdited(QTreeWidgetItem*, int)));
    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(splitterMoved(int,int)));

    // We really do need a mechanism for showing if a ride needs saving...
    //connect(this, SIGNAL(rideDirty()), this, SLOT(enableSaveButton()));
    //connect(this, SIGNAL(rideClean()), this, SLOT(enableSaveButton()));

    // cpx aggregate cache check
    connect(this,SIGNAL(rideAdded(RideItem*)),this,SLOT(checkCPX(RideItem*)));
    connect(this,SIGNAL(rideDeleted(RideItem*)),this,SLOT(checkCPX(RideItem*)));

    // when metricDB updates check if BlankState needs to be closed
    connect(context->athlete->metricDB, SIGNAL(dataChanged()), this, SLOT(checkBlankState()));
    // when config changes see if Train View BlankState needs to be closed
    connect(context, SIGNAL(configChanged()), this, SLOT(checkBlankState()));
    // when trainDB updates check if BlankState needs to be closed
    connect(trainDB, SIGNAL(dataChanged()), this, SLOT(checkBlankState()));

    // Kick off
    rideTreeWidgetSelectionChanged();
    selectAnalysis();
    setStyle();
} // upgrade from first line of constructor
}

/*----------------------------------------------------------------------
 * GUI
 *--------------------------------------------------------------------*/

void
MainWindow::toggleSidebar()
{
    showSidebar(!toolBox->isVisible());
#ifdef Q_OS_MAC
    sidebar->setSelected(toolBox->isVisible());
#endif
}

void
MainWindow::showSidebar(bool want)
{
    if (want) {

        toolBox->show();

        // Restore sizes
        QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, GC_SETTINGS_SPLITTER_SIZES);
        if (splitterSizes.toByteArray().size() > 1 ) {
            splitter->restoreState(splitterSizes.toByteArray());
            splitter->setOpaqueResize(true); // redraw when released, snappier UI
        }

        // if it was collapsed we need set to at least 200
        // unless the mainwindow isn't big enough
        if (toolBox->width()<10) {
            int size = width() - 200;
            if (size>200) size = 200;

            QList<int> sizes;
            sizes.append(size);
            sizes.append(width()-size);
            splitter->setSizes(sizes);
        }

    } else {

        toolBox->hide();
    }
    showhideSidebar->setChecked(toolBox->isVisible());
    setStyle();

}

void
MainWindow::showToolbar(bool want)
{
    if (want) head->show();
    else head->hide();
}

void
MainWindow::setChartMenu()
{
    unsigned int mask=0;
    // called when chart menu about to be shown
    // setup to only show charts that are relevant
    // to this view
    if (currentWindow == analWindow) mask = VIEW_ANALYSIS;
    if (currentWindow == trainWindow) mask = VIEW_TRAIN;
    if (currentWindow == diaryWindow) mask = VIEW_DIARY;
    if (currentWindow == homeWindow) mask = VIEW_HOME;

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
    if (currentWindow == analWindow) mask = VIEW_ANALYSIS;
    if (currentWindow == trainWindow) mask = VIEW_TRAIN;
    if (currentWindow == diaryWindow) mask = VIEW_DIARY;
    if (currentWindow == homeWindow) mask = VIEW_HOME;

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

        QString tripid = context->ride->ride()->getTag("RideWithGPS tripid", "");
        if (tripid == "") rideWithGPSAction->setEnabled(true);
        else rideWithGPSAction->setEnabled(false);
        
        QString activityId = context->ride->ride()->getTag("TtbExercise", "");
        if (activityId == "") ttbAction->setEnabled(true);
        else ttbAction->setEnabled(false);
        
    } else {
        //XXX deprecated stravaAction->setEnabled(false);
        rideWithGPSAction->setEnabled(false);
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
        currentWindow->appendChart(id); // called from MainWindow to inset chart
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
    if (!currentWindow) return;
    currentWindow->setStyle(segment ? 2 : 0);
    styleAction->setChecked(!segment);
}

void
MainWindow::toggleStyle()
{
    if (!currentWindow) return;

    switch (currentWindow->currentStyle) {

    default:
    case 0 :
        currentWindow->setStyle(2);
        styleAction->setChecked(false);
        break;

    case 2 :
        currentWindow->setStyle(0);
        styleAction->setChecked(true);
        break;
    }
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
MainWindow::rideTreeWidgetSelectionChanged()
{
    assert(treeWidget->selectedItems().size() <= 1);
    if (treeWidget->selectedItems().isEmpty())
        context->ride = NULL;
    else {
        QTreeWidgetItem *which = treeWidget->selectedItems().first();
        if (which->type() != RIDE_TYPE) return; // ignore!
        else
            context->ride = (RideItem*) which;
    }

    // update the ride property on all widgets
    // to let them know they need to replot new
    // selected ride
    gcCalendar->setRide(context->ride);
    gcMultiCalendar->setRide(context->ride);
    //context->athlete->rideMetadata()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(context->ride)));
    analWindow->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(context->ride)));
    homeWindow->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(context->ride)));
    diaryWindow->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(context->ride)));
    trainWindow->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(context->ride)));

    if (!context->ride) return;

    // refresh interval list for bottom left
    // first lets wipe away the existing intervals
    QList<QTreeWidgetItem *> intervals = allIntervals->takeChildren();
    for (int i=0; i<intervals.count(); i++) delete intervals.at(i);

    // now add the intervals for the current ride
    if (context->ride) { // only if we have a ride pointer
        RideFile *selected = context->ride->ride();
        if (selected) {
            // get all the intervals in the currently selected RideFile
            QList<RideFileInterval> intervals = selected->intervals();
            for (int i=0; i < intervals.count(); i++) {
                // add as a child to allIntervals
                IntervalItem *add = new IntervalItem(selected,
                                                        intervals.at(i).name,
                                                        intervals.at(i).start,
                                                        intervals.at(i).stop,
                                                        selected->timeToDistance(intervals.at(i).start),
                                                        selected->timeToDistance(intervals.at(i).stop),
                                                        allIntervals->childCount()+1);
                add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
                allIntervals->addChild(add);
            }
        }
    }
}

void
MainWindow::dateRangeChangedDiary(DateRange dr)
{
    // we got signalled date range changed, tell the current view
    // when we have multiple sidebars that change date we need to connect
    // them up individually.... i.e. LTM....
    diaryWindow->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));
    context->_dr = dr;
}

void
MainWindow::dateRangeChangedLTM(DateRange dr)
{
    // we got signalled date range changed, tell the current view
    // when we have multiple sidebars that change date we need to connect
    // them up individually.... i.e. LTM....
    homeWindow->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));
    context->_dr = dr;
}

void
MainWindow::analysisPopup()
{
    // set the point for the menu and call below
    showTreeContextMenuPopup(analSidebar->mapToGlobal(QPoint(analItem->pos().x()+analItem->width()-20, analItem->pos().y())));
}

void
MainWindow::showTreeContextMenuPopup(const QPoint &pos)
{
    if (treeWidget->selectedItems().size() == 0) return; //none selected!

    RideItem *rideItem = (RideItem *)treeWidget->selectedItems().first();
    if (rideItem != NULL && rideItem->text(0) != tr("All Activities")) {
        QMenu menu(treeWidget);


        QAction *actSaveRide = new QAction(tr("Save Changes"), treeWidget);
        connect(actSaveRide, SIGNAL(triggered(void)), this, SLOT(saveRide()));

        QAction *revertRide = new QAction(tr("Revert to Saved version"), treeWidget);
        connect(revertRide, SIGNAL(triggered(void)), this, SLOT(revertRide()));

        QAction *actDeleteRide = new QAction(tr("Delete Activity"), treeWidget);
        connect(actDeleteRide, SIGNAL(triggered(void)), this, SLOT(deleteRide()));

        QAction *actSplitRide = new QAction(tr("Split Activity"), treeWidget);
        connect(actSplitRide, SIGNAL(triggered(void)), this, SLOT(splitRide()));

        if (rideItem->isDirty() == true) {
          menu.addAction(actSaveRide);
          menu.addAction(revertRide);
        }

        menu.addAction(actDeleteRide);
        menu.addAction(actSplitRide);
#ifdef GC_HAVE_ICAL
        QAction *actUploadCalendar = new QAction(tr("Upload Activity to Calendar"), treeWidget);
        connect(actUploadCalendar, SIGNAL(triggered(void)), this, SLOT(uploadCalendar()));
        menu.addAction(actUploadCalendar);
#endif
        menu.addSeparator();

        // ride navigator stuff
        QAction *colChooser = new QAction(tr("Show Column Chooser"), treeWidget);
        connect(colChooser, SIGNAL(triggered(void)), listView, SLOT(showColumnChooser()));
        menu.addAction(colChooser);

        if (listView->groupBy() >= 0) {

            // already grouped lets ungroup
            QAction *nogroups = new QAction(tr("Do Not Show In Groups"), treeWidget);
            connect(nogroups, SIGNAL(triggered(void)), listView, SLOT(noGroups()));
            menu.addAction(nogroups);

        } else {

            QMenu *groupByMenu = new QMenu(tr("Group By"), treeWidget);
            groupByMenu->setEnabled(true);
            menu.addMenu(groupByMenu);

            // add menu options for each column
            if (groupByMapper) delete groupByMapper;
            groupByMapper = new QSignalMapper(this);
            connect(groupByMapper, SIGNAL(mapped(const QString &)), listView, SLOT(setGroupByColumnName(QString)));

            foreach(QString heading, listView->columnNames()) {
                if (heading == "*") continue; // special hidden column

                QAction *groupByAct = new QAction(heading, treeWidget);
                connect(groupByAct, SIGNAL(triggered()), groupByMapper, SLOT(map()));
                groupByMenu->addAction(groupByAct);

                // map action to column heading
                groupByMapper->setMapping(groupByAct, heading);
            }
        }
        menu.exec(pos);
    }
}

void
MainWindow::intervalPopup()
{
    // always show the 'find best' 'find peaks' options
    QMenu menu(intervalItem);

    RideItem *rideItem = (RideItem *)treeWidget->selectedItems().first();

    if (rideItem != NULL && rideItem->ride() && rideItem->ride()->dataPoints().count()) {
        QAction *actFindPeak = new QAction(tr("Find Peak Intervals"), intervalItem);
        QAction *actFindBest = new QAction(tr("Find Best Intervals"), intervalItem);
        connect(actFindPeak, SIGNAL(triggered(void)), this, SLOT(findPowerPeaks(void)));
        connect(actFindBest, SIGNAL(triggered(void)), this, SLOT(addIntervals(void)));
        menu.addAction(actFindPeak);
        menu.addAction(actFindBest);

        // sort but only if 2 or more intervals
        if (allIntervals->childCount() > 1) {
            QAction *actSort = new QAction(tr("Sort Intervals"), intervalItem);
            connect(actSort, SIGNAL(triggered(void)), this, SLOT(sortIntervals(void)));
            menu.addAction(actSort);
        }

        if (intervalWidget->selectedItems().count()) menu.addSeparator();
    }


    if (intervalWidget->selectedItems().count() == 1) {

        // we can zoom, rename etc if only 1 interval is selected
        QAction *actZoomInt = new QAction(tr("Zoom to interval"), intervalWidget);
        QAction *actEditInt = new QAction(tr("Edit interval"), intervalWidget);
        QAction *actDeleteInt = new QAction(tr("Delete interval"), intervalWidget);

        connect(actZoomInt, SIGNAL(triggered(void)), this, SLOT(zoomIntervalSelected(void)));
        connect(actEditInt, SIGNAL(triggered(void)), this, SLOT(editIntervalSelected(void)));
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteIntervalSelected(void)));

        menu.addAction(actZoomInt);
        menu.addAction(actEditInt);
        menu.addAction(actDeleteInt);
    }

    if (intervalWidget->selectedItems().count() > 1) {
        QAction *actRenameInt = new QAction(tr("Rename selected intervals"), intervalWidget);
        connect(actRenameInt, SIGNAL(triggered(void)), this, SLOT(renameIntervalsSelected(void)));
        QAction *actDeleteInt = new QAction(tr("Delete selected intervals"), intervalWidget);
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteIntervalSelected(void)));

        menu.addAction(actRenameInt);
        menu.addAction(actDeleteInt);
    }

    menu.exec(analSidebar->mapToGlobal((QPoint(intervalItem->pos().x()+intervalItem->width()-20, intervalItem->pos().y()))));
}

void
MainWindow::showContextMenuPopup(const QPoint &pos)
{
    QTreeWidgetItem *trItem = intervalWidget->itemAt( pos );
    if (trItem != NULL && trItem->text(0) != tr("Intervals")) {
        QMenu menu(intervalWidget);

        activeInterval = (IntervalItem *)trItem;

        QAction *actEditInt = new QAction(tr("Edit interval"), intervalWidget);
        QAction *actDeleteInt = new QAction(tr("Delete interval"), intervalWidget);
        QAction *actZoomInt = new QAction(tr("Zoom to interval"), intervalWidget);
        QAction *actFrontInt = new QAction(tr("Bring to Front"), intervalWidget);
        QAction *actBackInt = new QAction(tr("Send to back"), intervalWidget);
        connect(actEditInt, SIGNAL(triggered(void)), this, SLOT(editInterval(void)));
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteInterval(void)));
        connect(actZoomInt, SIGNAL(triggered(void)), this, SLOT(zoomInterval(void)));
        connect(actFrontInt, SIGNAL(triggered(void)), this, SLOT(frontInterval(void)));
        connect(actBackInt, SIGNAL(triggered(void)), this, SLOT(backInterval(void)));

        menu.addAction(actZoomInt);
        menu.addAction(actEditInt);
        menu.addAction(actDeleteInt);
        menu.exec(intervalWidget->mapToGlobal( pos ));
    }
}

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
MainWindow::splitterMoved(int pos, int /*index*/)
{
    // show / hide sidebar as dragged..
    if ((pos == 0  && toolBox->isVisible()) || (pos>10 && !toolBox->isVisible())) toggleSidebar();

    listView->setWidth(pos);
    appsettings->setCValue(context->athlete->cyclist, GC_SETTINGS_SPLITTER_SIZES, splitter->saveState());
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
    // do nothing on upgrade exit
    if (init == false) return;

    if (saveRideExitDialog() == false) event->ignore();
    else {

        // stop any active realtime conneection
        trainTool->Stop();

        // save ride list config
        appsettings->setCValue(context->athlete->cyclist, GC_SORTBY, listView->sortByIndex());
        appsettings->setCValue(context->athlete->cyclist, GC_SORTBYORDER, listView->sortByOrder());
        appsettings->setCValue(context->athlete->cyclist, GC_NAVGROUPBY, listView->groupBy());
        appsettings->setCValue(context->athlete->cyclist, GC_NAVHEADINGS, listView->columns());
        appsettings->setCValue(context->athlete->cyclist, GC_NAVHEADINGWIDTHS, listView->widths());

        // save the state of all the pages
        analWindow->saveState();
        homeWindow->saveState();
        trainWindow->saveState();
        diaryWindow->saveState();

#ifdef GC_HAVE_LUCENE
        // save the named searches
        context->athlete->namedSearches->write();
#endif

        // clear the clipboard if neccessary
        QApplication::clipboard()->setText("");

        // blank state settings
        appsettings->setCValue(context->athlete->cyclist, GC_BLANK_ANALYSIS, blankStateAnalysisPage->dontShow->isChecked());
        appsettings->setCValue(context->athlete->cyclist, GC_BLANK_DIARY, blankStateDiaryPage->dontShow->isChecked());
        appsettings->setCValue(context->athlete->cyclist, GC_BLANK_HOME, blankStateHomePage->dontShow->isChecked());
        appsettings->setCValue(context->athlete->cyclist, GC_BLANK_TRAIN, blankStateTrainPage->dontShow->isChecked());

        // set to latest so we don't repeat
        appsettings->setCValue(context->athlete->home.dirName(), GC_VERSION_USED, VERSION_LATEST);
        appsettings->setCValue(context->athlete->home.dirName(), GC_SAFEEXIT, true);

        // now remove from the list
        if(mainwindows.removeOne(this) == false)
            qDebug()<<"closeEvent: mainwindows list error";

    }
}

// global search/data filter
void
MainWindow::searchResults(QStringList f)
{
    filters = f;
    isfiltered = true;
    emit filterChanged(filters);
}

void
MainWindow::searchClear()
{
    filters.clear();
    isfiltered = false;
    emit filterChanged(filters);
}

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
    currentWindow->resetLayout();
}

void MainWindow::dateChanged(const QDate &date)
{
    for (int i = 0; i < allRides->childCount(); i++)
    {
        context->ride = (RideItem*) allRides->child(i);
        if (context->ride->dateTime.date() == date) {
            treeWidget->scrollToItem(allRides->child(i),
                QAbstractItemView::EnsureVisible);
            treeWidget->setCurrentItem(allRides->child(i));
            i = allRides->childCount();
        }
    }
}

void MainWindow::selectRideFile(QString fileName)
{
    for (int i = 0; i < allRides->childCount(); i++)
    {
        context->ride = (RideItem*) allRides->child(i);
        if (context->ride->fileName == fileName) {
            treeWidget->scrollToItem(allRides->child(i),
                QAbstractItemView::EnsureVisible);
            treeWidget->setCurrentItem(allRides->child(i));
            i = allRides->childCount();
        }
    }
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
MainWindow::checkBlankState()
{
    // Home?
    if (views->currentWidget() == blankStateHomePage) {
        // should it be closed?
        if (allRides->childCount() > 0) closeBlankHome();
    }
    // Diary?
    if (views->currentWidget() == blankStateDiaryPage) {
        // should it be closed?
        if (allRides->childCount() > 0) closeBlankDiary();
    }
    // Analysis??
    if (views->currentWidget() == blankStateAnalysisPage) {
        // should it be closed?
        if (allRides->childCount() > 0) closeBlankAnal();
    }
    // Train??
    if (views->currentWidget() == blankStateTrainPage) {
        // should it be closed?
        if (appsettings->value(this, GC_DEV_COUNT).toInt() > 0 && trainDB->getCount() > 2) closeBlankTrain();
    }
}

void
MainWindow::closeBlankTrain()
{
    showBlankTrain = false;
    selectTrain();
}

void 
MainWindow::closeBlankAnal()
{
    showBlankAnal = false;
    selectAnalysis();
}

void 
MainWindow::closeBlankDiary()
{
    showBlankDiary = false;
    selectDiary();
}

void 
MainWindow::closeBlankHome()
{
    showBlankHome = false;
    selectHome();
}

void
MainWindow::selectAnalysis()
{
    // No ride - no analysis view
    if (allRides->childCount() == 0 && showBlankAnal == true) {
        masterControls->setVisible(false);
        toolBox->hide();
        views->setCurrentWidget(blankStateAnalysisPage);
    } else {
        masterControls->setVisible(true);
        masterControls->setCurrentIndex(0);
        views->setCurrentIndex(0);
        analWindow->selected(); // tell it!
        trainTool->getToolbarButtons()->hide();
#ifdef GC_HAVE_ICAL
        scopebar->selected(2);
#else
        scopebar->selected(1);
#endif
        toolBox->setCurrentIndex(0);
    }
    currentWindow = analWindow;
    setStyle();
}

void
MainWindow::selectTrain()
{
    // no devices configured -or- only the manual mode workouts defined
    // we need to get setup properly...
    if ((appsettings->value(this, GC_DEV_COUNT) == 0 || trainDB->getCount() <= 2) && showBlankTrain == true) {
        masterControls->setVisible(false);
        toolBox->hide();
        views->setCurrentWidget(blankStateTrainPage);
    } else {
        masterControls->setVisible(true);
		//this->showSidebar(true);
        masterControls->setCurrentIndex(1);
        views->setCurrentIndex(1);
        trainWindow->selected(); // tell it!
        trainTool->getToolbarButtons()->show();
#ifdef GC_HAVE_ICAL
        scopebar->selected(3);
#else
        scopebar->selected(2);
#endif
        toolBox->setCurrentIndex(2);
    }
    currentWindow = trainWindow;
    setStyle();
}

void
MainWindow::selectDiary()
{
    if (allRides->childCount() == 0 && showBlankDiary == true) {
        masterControls->setVisible(false);
        toolBox->hide();
        views->setCurrentWidget(blankStateDiaryPage);
    } else {
        masterControls->setVisible(true);
		//this->showSidebar(true);
        masterControls->setCurrentIndex(2);
        views->setCurrentIndex(2);
        diaryWindow->selected(); // tell it!
        trainTool->getToolbarButtons()->hide();
        scopebar->selected(1);
        toolBox->setCurrentIndex(1);
        gcCalendar->refresh(); // get that signal with the date range...
    }
    currentWindow = diaryWindow;
    setStyle();
}

void
MainWindow::selectHome()
{
    // No ride - no analysis view
    if (allRides->childCount() == 0 && showBlankHome == true) {
        masterControls->setVisible(false);
        toolBox->hide();
        views->setCurrentWidget(blankStateHomePage);
    } else {
        masterControls->setVisible(true);
        //toolBox->show();
		//this->showSidebar(true);
        masterControls->setCurrentIndex(3);
        views->setCurrentIndex(3);
        homeWindow->selected(); // tell it!
        trainTool->getToolbarButtons()->hide();
        scopebar->selected(0);
        toolBox->setCurrentIndex(3);
    }
    currentWindow = homeWindow;
    setStyle();
}

void
MainWindow::setStyle()
{
    int select = currentWindow->currentStyle == 0 ? 0 : 1;

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

    if (currentWindow != trainWindow) {
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
MainWindow::addRide(QString name, bool dosignal)
{
    QDateTime dt;
    if (!parseRideFileName(name, &dt)) {
        fprintf(stderr, "bad name: %s\n", name.toAscii().constData());
        assert(false);
    }
    RideItem *last = new RideItem(RIDE_TYPE, context->athlete->home.path(), name, dt, context->athlete->zones(), context->athlete->hrZones(), context);

    int index = 0;
    while (index < allRides->childCount()) {
        QTreeWidgetItem *item = allRides->child(index);
        if (item->type() != RIDE_TYPE)
            continue;
        RideItem *other = static_cast<RideItem*>(item);

        if (other->dateTime > dt) break;
        if (other->fileName == name) {
            delete allRides->takeChild(index);
            break;
        }
        ++index;
    }
    if (dosignal) rideAdded(last); // here so emitted BEFORE rideSelected is emitted!
    allRides->insertChild(index, last);

    // if it is the very first ride, we need to select it
    // after we added it
    if (!index) treeWidget->setCurrentItem(last);
}

void
MainWindow::removeCurrentRide()
{
    int x = 0;

    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item->type() != RIDE_TYPE)
        return;
    RideItem *item = static_cast<RideItem*>(_item);


    QTreeWidgetItem *itemToSelect = NULL;
    for (x=0; x<allRides->childCount(); ++x)
    {
        if (item==allRides->child(x))
        {
            break;
        }
    }

    if (x>0) {
        itemToSelect = allRides->child(x-1);
    }
    if ((x+1)<allRides->childCount()) {
        itemToSelect = allRides->child(x+1);
    }

    QString strOldFileName = item->fileName;
    allRides->removeChild(item);


    QFile file(context->athlete->home.absolutePath() + "/" + strOldFileName);
    // purposefully don't remove the old ext so the user wouldn't have to figure out what the old file type was
    QString strNewName = strOldFileName + ".bak";

    // in case there was an existing bak file, delete it
    // ignore errors since it probably isn't there.
    QFile::remove(context->athlete->home.absolutePath() + "/" + strNewName);

    if (!file.rename(context->athlete->home.absolutePath() + "/" + strNewName))
    {
        QMessageBox::critical(
            this, "Rename Error",
            tr("Can't rename %1 to %2")
            .arg(strOldFileName).arg(strNewName));
    }

    // remove any other derived/additional files; notes, cpi etc
    QStringList extras;
    extras << "notes" << "cpi" << "cpx";
    foreach (QString extension, extras) {

        QString deleteMe = QFileInfo(strOldFileName).baseName() + "." + extension;
        QFile::remove(context->athlete->home.absolutePath() + "/" + deleteMe);
    }

    // notify AFTER deleted from DISK..
    rideDeleted(item);

    // ..but before MEMORY cleared
    item->freeMemory();
    delete item;

    // any left?
    if (allRides->childCount() == 0) {
        context->ride = NULL;
        rideTreeWidgetSelectionChanged(); // notifies children
        gcCalendar->setRide(context->ride); // and the pesky calendars
        gcMultiCalendar->setRide(context->ride);
    }

    treeWidget->setCurrentItem(itemToSelect);
    rideTreeWidgetSelectionChanged();
}

void
MainWindow::checkCPX(RideItem*ride)
{
    QList<RideFileCache*> newList;

    foreach(RideFileCache *p, context->athlete->cpxCache) {
        if (ride->dateTime.date() < p->start || ride->dateTime.date() > p->end)
            newList.append(p);
    }
    context->athlete->cpxCache = newList;
}

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

const RideFile *
Context::currentRide()
{
    return mainWindow->currentRide();
}

const RideFile *
MainWindow::currentRide()
{
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        return NULL;
    }
    return ((RideItem*) treeWidget->selectedItems().first())->ride();
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
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
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
MainWindow::deleteRide()
{
    QTreeWidgetItem *_item = treeWidget->currentItem();
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
        removeCurrentRide();
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
        if (!newHome.exists())
            assert(false);
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
        if (!newHome.exists())
            assert(false);
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
* RideWithGPS.com
*--------------------------------------------------------------------*/

void
MainWindow::uploadRideWithGPSAction()
{
    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE) return;

    RideItem *item = dynamic_cast<RideItem*>(_item);

    if (item) { // menu is disabled anyway, but belt and braces
        RideWithGPSDialog d(context, item);
        d.exec();
    }
}

/*----------------------------------------------------------------------
* trainingstagebuch.org
*--------------------------------------------------------------------*/

void
MainWindow::uploadTtb()
{
    QTreeWidgetItem *_item = treeWidget->currentItem();
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
 * Intervals
 *--------------------------------------------------------------------*/

void
MainWindow::addIntervals()
{
    if (context->ride && context->ride->ride() && context->ride->ride()->dataPoints().count()) {

        AddIntervalDialog *p = new AddIntervalDialog(context);
        p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
        p->exec();

    } else {

        if (!context->ride || !context->ride->ride())
            QMessageBox::critical(this, tr("Find Best Intervals"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Find Best Intervals"), tr("Current activity contains no data"));
    }
}

void
MainWindow::addIntervalForPowerPeaksForSecs(RideFile *ride, int windowSizeSecs, QString name)
{
    QList<BestIntervalDialog::BestInterval> results;
    BestIntervalDialog::findBests(ride, windowSizeSecs, 1, results);
    if (results.isEmpty()) return;
    const BestIntervalDialog::BestInterval &i = results.first();
    QTreeWidgetItem *peak =
        new IntervalItem(ride, name+tr(" (%1 watts)").arg((int) round(i.avg)),
                         i.start, i.stop,
                         ride->timeToDistance(i.start),
                         ride->timeToDistance(i.stop),
                         allIntervals->childCount()+1);
    peak->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    allIntervals->addChild(peak);
}

void
MainWindow::findPowerPeaks()
{

    if (!context->ride) return;

    QTreeWidgetItem *which = treeWidget->selectedItems().first();
    if (which->type() != RIDE_TYPE) {
        return;
    }

    if (context->ride && context->ride->ride() && context->ride->ride()->dataPoints().count()) {

        addIntervalForPowerPeaksForSecs(context->ride->ride(), 5, "Peak 5s");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 10, "Peak 10s");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 20, "Peak 20s");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 30, "Peak 30s");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 60, "Peak 1min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 120, "Peak 2min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 300, "Peak 5min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 600, "Peak 10min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 1200, "Peak 20min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 1800, "Peak 30min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 3600, "Peak 60min");

        // now update the RideFileIntervals
        updateRideFileIntervals();

    } else {
        if (!context->ride || !context->ride->ride())
            QMessageBox::critical(this, tr("Find Power Peaks"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Find Power Peaks"), tr("Current activity contains no data"));
    }
}

void
MainWindow::updateRideFileIntervals()
{
    // iterate over allIntervals as they are now defined
    // and update the RideFile->intervals
    RideItem *which = (RideItem *)treeWidget->selectedItems().first();
    RideFile *current = which->ride();
    current->clearIntervals();
    for (int i=0; i < allIntervals->childCount(); i++) {
        // add the intervals as updated
        IntervalItem *it = (IntervalItem *)allIntervals->child(i);
        current->addInterval(it->start, it->stop, it->text(0));
    }

    // emit signal for interval data changed
    intervalsChanged();

    // set dirty
    which->setDirty(true);
}

bool
lessItem(const IntervalItem *s1, const IntervalItem *s2) {
    return s1->start < s2->start;
}

void
MainWindow::sortIntervals()
{
    // sort them chronologically
    QList<IntervalItem*> intervals;

    // set string to first interval selected
    for (int i=0; i<allIntervals->childCount();i++)
        intervals.append((IntervalItem*)(allIntervals->child(i)));

    // now sort them into start time order
    qStableSort(intervals.begin(), intervals.end(), lessItem);

    // empty allIntervals
    allIntervals->takeChildren();

    // and put em back in chronological sequence
    foreach(IntervalItem* item, intervals) {
        allIntervals->addChild(item);
    }

    // now update the ridefile
    updateRideFileIntervals(); // will emit intervalChanged() signal
}

// rename multiple intervals
void
MainWindow::renameIntervalsSelected()
{
    QString string;

    // set string to first interval selected
    for (int i=0; i<allIntervals->childCount();i++) {
        if (allIntervals->child(i)->isSelected()) {
            string = allIntervals->child(i)->text(0);
            break;
        }
    }

    // type in a name and we will renumber all the intervals
    // in the same fashion -- esp if the last characters are
    RenameIntervalDialog dialog(string, this);
    dialog.setFixedWidth(320);

    if (dialog.exec()) {

        int number = 1;

        // does it end in a number?
        // if so we use that to renumber from
        QRegExp ends("^(.*[^0-9])([0-9]+)$");
        if (ends.exactMatch(string)) {

            string = ends.cap(1);
            number = ends.cap(2).toInt();

        } else if (!string.endsWith(" ")) string += " ";

        // now go and renumber from 'number' with prefix 'string'
        for (int i=0; i<allIntervals->childCount();i++) {
            if (allIntervals->child(i)->isSelected())
                allIntervals->child(i)->setText(0, QString("%1%2").arg(string).arg(number++));
        }

        updateRideFileIntervals(); // will emit intervalChanged() signal
    }
}

void
MainWindow::deleteIntervalSelected()
{
    // delete the intervals that are selected (from the menu)
    // the normal delete intervals does that already
    deleteInterval();
}

void
MainWindow::deleteInterval()
{
    // now delete highlighted!
    for (int i=0; i<allIntervals->childCount();) {
        if (allIntervals->child(i)->isSelected()) delete allIntervals->takeChild(i);
        else i++;
    }

    updateRideFileIntervals(); // will emit intervalChanged() signal
}

void
MainWindow::renameIntervalSelected()
{
    // go edit the name
    for (int i=0; i<allIntervals->childCount();) {
        if (allIntervals->child(i)->isSelected()) {
            allIntervals->child(i)->setFlags(allIntervals->child(i)->flags() | Qt::ItemIsEditable);
            intervalWidget->editItem(allIntervals->child(i), 0);
            break;
        } else i++;
    }
    updateRideFileIntervals(); // will emit intervalChanged() signal
}

void
MainWindow::renameInterval() {
    // go edit the name
    activeInterval->setFlags(activeInterval->flags() | Qt::ItemIsEditable);
    intervalWidget->editItem(activeInterval, 0);
}

void
MainWindow::editIntervalSelected()
{
    // go edit the interval
    for (int i=0; i<allIntervals->childCount();) {
        if (allIntervals->child(i)->isSelected()) {
            activeInterval = (IntervalItem*)allIntervals->child(i);
            editInterval();
            break;
        } else i++;
    }
}

void
MainWindow::editInterval()
{
    IntervalItem temp = *activeInterval;
    EditIntervalDialog dialog(this, temp);

    if (dialog.exec()) {
        *activeInterval = temp;
        updateRideFileIntervals(); // will emit intervalChanged() signal
        intervalWidget->update();
    }
}

void
MainWindow::intervalEdited(QTreeWidgetItem *, int) {
    // the user renamed the interval
    updateRideFileIntervals(); // will emit intervalChanged() signal
}

void
MainWindow::zoomIntervalSelected()
{
    // zoom the one interval that is selected via popup menu
    for (int i=0; i<allIntervals->childCount();) {
        if (allIntervals->child(i)->isSelected()) {
            emit intervalZoom((IntervalItem*)(allIntervals->child(i)));
            break;
        } else i++;
    }
}

void
MainWindow::zoomInterval() {
    // zoom into this interval on allPlot
    emit intervalZoom(activeInterval);
}

void
MainWindow::frontInterval()
{
    int oindex = activeInterval->displaySequence;
    for (int i=0; i<allIntervals->childCount(); i++) {
        IntervalItem *it = (IntervalItem *)allIntervals->child(i);
        int ds = it->displaySequence;
        if (ds > oindex)
            it->setDisplaySequence(ds-1);
    }
    activeInterval->setDisplaySequence(allIntervals->childCount());

    // signal!
    intervalsChanged();
}

void
MainWindow::backInterval()
{
    int oindex = activeInterval->displaySequence;
    for (int i=0; i<allIntervals->childCount(); i++) {
        IntervalItem *it = (IntervalItem *)allIntervals->child(i);
        int ds = it->displaySequence;
        if (ds < oindex)
            it->setDisplaySequence(ds+1);
    }
    activeInterval->setDisplaySequence(1);

    // signal!
    intervalsChanged();

}

void
MainWindow::intervalTreeWidgetSelectionChanged()
{
    intervalSelected();
}


/*----------------------------------------------------------------------
 * Utility
 *--------------------------------------------------------------------*/

bool
MainWindow::parseRideFileName(const QString &name, QDateTime *dt)
{
    static char rideFileRegExp[] = "^((\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                                   "_(\\d\\d)_(\\d\\d)_(\\d\\d))\\.(.+)$";
    QRegExp rx(rideFileRegExp);
    if (!rx.exactMatch(name))
            return false;
    assert(rx.numCaptures() == 8);
    QDate date(rx.cap(2).toInt(), rx.cap(3).toInt(),rx.cap(4).toInt());
    QTime time(rx.cap(5).toInt(), rx.cap(6).toInt(),rx.cap(7).toInt());
    if ((! date.isValid()) || (! time.isValid())) {
	QMessageBox::warning(this,
			     tr("Invalid Activity File Name"),
			     tr("Invalid date/time in filename:\n%1\nSkipping file...").arg(name)
			     );
	return false;
    }
    *dt = QDateTime(date, time);
    return true;
}

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
MainWindow::recordMeasure()
{
}

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
MainWindow::exportMeasures()
{
    QDateTime start, end;
    end = QDateTime::currentDateTime();
    start.fromTime_t(0);

    foreach (SummaryMetrics x, context->athlete->metricDB->db()->getAllMeasuresFor(start, end)) {
//qDebug()<<x.getDateTime();
//qDebug()<<x.getText("Weight", "0.0").toDouble();
//qDebug()<<x.getText("Lean Mass", "0.0").toDouble();
//qDebug()<<x.getText("Fat Mass", "0.0").toDouble();
//qDebug()<<x.getText("Fat Ratio", "0.0").toDouble();
    }
}

void
MainWindow::importMeasures() { }

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
MainWindow::importCalendar()
{
    // read an iCalendar format file
}

void
MainWindow::exportCalendar()
{
    // write and iCalendar format file
    // need options to decide wether to
    // include past dates, workout plans only
    // or also workouts themselves
}

void
MainWindow::setBubble(QString text, QPoint pos, Qt::Orientation orientation)
{
    if (text == "") {
        bubble->setVisible(false);
    } else {
        bubble->setText(text);
        bubble->setPos(pos.x(), pos.y(), orientation);
        bubble->setVisible(true);
        bubble->raise();
        bubble->repaint();
    }
}

#ifdef Q_OS_MAC
void
MainWindow::searchTextChanged(QString text)
{
#ifdef GC_HAVE_LUCENE
    // clear or set...
    if (text == "") {
        listView->clearSearch();
        gcCalendar->clearFilter();
        gcMultiCalendar->clearFilter();
    } else {
        context->athlete->lucene->search(text);
        listView->searchStrings(context->athlete->lucene->files());
        gcCalendar->setFilter(context->athlete->lucene->files());
        gcMultiCalendar->setFilter(context->athlete->lucene->files());
    }
#endif
}
#endif
void
MainWindow::actionClicked(int index)
{
    switch(index) {

    default:
    case 0: addIntervals();
            break;

    case 1 : splitRide();
            break;

    case 2 : deleteRide();
            break;

    }
}
