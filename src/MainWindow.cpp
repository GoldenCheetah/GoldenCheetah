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

#include "MainWindow.h"
#include "AthleteTool.h"
#include "BestIntervalDialog.h"
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
#include "RideImportWizard.h"
#include "RideMetadata.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include "Zones.h"

#include "RideCalendar.h"
#include "DatePickerDialog.h"
#include "ToolsDialog.h"
#include "ToolsRhoEstimator.h"
#include "MetricAggregator.h"
#include "SplitActivityWizard.h"
#include "BatchExportDialog.h"
#include "StravaDialog.h"
#include "TwitterDialog.h"
#include "WithingsDownload.h"
#include "CalendarDownload.h"
#include "WorkoutWizard.h"
#include "ErgDB.h"
#include "ErgDBDownloadDialog.h"
#include "TrainTool.h"

#include "GcWindowTool.h"
#include "GcToolBar.h"
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

#if (defined Q_OS_MAC) && (defined GC_HAVE_LION)
#include "LionFullScreen.h"
#endif
#ifndef Q_OS_MAC
#include "QTFullScreen.h"
#endif

#include <assert.h>
#include <QApplication>
#include <QtGui>
#include <QRegExp>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_grid.h>
#include <qwt_data.h>
#include <boost/scoped_ptr.hpp>

#ifndef GC_VERSION
#define GC_VERSION "(developer build)"
#endif

QList<MainWindow *> mainwindows; // keep track of all the MainWindows we have open


MainWindow::MainWindow(const QDir &home) :
    home(home), session(0), isclean(false),
    zones_(new Zones), hrzones_(new HrZones),
    ride(NULL), workout(NULL)
{

    static const QIcon hideIcon(":images/toolbar/main/hideside.png");
    static const QIcon showIcon(":images/toolbar/main/showside.png");
    static const QIcon tabIcon(":images/toolbar/main/tab.png");
    static const QIcon tileIcon(":images/toolbar/main/tile.png");
    static const QIcon fullIcon(":images/toolbar/main/togglefull.png");

    mainwindows.append(this); // add us to the list of open windows

    /*----------------------------------------------------------------------
     *  Basic GUI setup
     *--------------------------------------------------------------------*/

    setAttribute(Qt::WA_DeleteOnClose);
    setUnifiedTitleAndToolBarOnMac(true);
    setWindowIcon(QIcon(":images/gc.png"));
    setWindowTitle(home.dirName());
    setContentsMargins(0,0,0,0);
    setAcceptDrops(true);

    appsettings->setValue(GC_SETTINGS_LAST, home.dirName());
    QVariant geom = appsettings->value(this, GC_SETTINGS_MAIN_GEOM);
    if (geom == QVariant()) resize(640, 480);
    else setGeometry(geom.toRect());

    GCColor *GCColorSet = new GCColor(this); // get/keep colorset
    GCColorSet->colorSet(); // shut up the compiler
    setStyleSheet("QFrame { FrameStyle = QFrame::NoFrame };"
                  "QWidget { background = Qt::white; border:0 px; margin: 2px; };"
                  "QTabWidget { background = Qt::white; };"
                  "::pane { FrameStyle = QFrame::NoFrame; border: 0px; };");

    QVariant unit = appsettings->value(this, GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");

#if (defined Q_OS_MAC) && (defined GC_HAVE_LION)
    fullScreen = new LionFullScreen(this);
#endif
#ifndef Q_OS_MAC
    fullScreen = new QTFullScreen(this);
#endif

    /*----------------------------------------------------------------------
     * The help bubble used everywhere
     *--------------------------------------------------------------------*/
    bubble = new GcBubble(this);
    bubble->hide();

    /*----------------------------------------------------------------------
     *  Athlete details
     *--------------------------------------------------------------------*/

    cyclist = home.dirName();
    setInstanceName(cyclist);
    seasons = new Seasons(home);

    // read power zones...
    QFile zonesFile(home.absolutePath() + "/power.zones");
    if (zonesFile.exists()) {
        if (!zones_->read(zonesFile)) {
            QMessageBox::critical(this, tr("Zones File Error"),
				  zones_->errorString());
        } else if (! zones_->warningString().isEmpty())
            QMessageBox::warning(this, tr("Reading Zones File"), zones_->warningString());
    }

    // read hr zones...
    QFile hrzonesFile(home.absolutePath() + "/hr.zones");
    if (hrzonesFile.exists()) {
        if (!hrzones_->read(hrzonesFile)) {
            QMessageBox::critical(this, tr("HR Zones File Error"),
				  hrzones_->errorString());
        } else if (! hrzones_->warningString().isEmpty())
            QMessageBox::warning(this, tr("Reading HR Zones File"), hrzones_->warningString());
    }

    /*----------------------------------------------------------------------
     * Central instances of shared data
     *--------------------------------------------------------------------*/

    // Metadata fields
    _rideMetadata = new RideMetadata(this,true);
    _rideMetadata->hide();
    metricDB = new MetricAggregator(this, home, zones(), hrZones()); // just to catch config updates!
    metricDB->refreshMetrics();

    // Downloaders
    withingsDownload = new WithingsDownload(this);
    calendarDownload = new CalendarDownload(this);

    // Calendar
#ifdef GC_HAVE_ICAL
    rideCalendar = new ICalendar(this); // my local/remote calendar entries
    davCalendar = new CalDAV(this); // remote caldav
    davCalendar->download(); // refresh the diary window
#endif

    // if no workout directory is configured, default to the
    // top level GoldenCheetah directory
    if (appsettings->value(NULL, GC_WORKOUTDIR).toString() == "")
        appsettings->setValue(GC_WORKOUTDIR, QFileInfo(home.absolutePath() + "/../").absolutePath());

    /*----------------------------------------------------------------------
     * Toolbar
     *--------------------------------------------------------------------*/

    toolbar = new GcToolBar(this);

    QWidget *lspacer = new QWidget(this);
    QHBoxLayout *lspacerLayout = new QHBoxLayout(lspacer);
    lspacerLayout->setSpacing(0);
    lspacerLayout->setContentsMargins(0,0,0,0);
    lspacerLayout->addStretch();
    lspacer->setFixedWidth(100);
    lspacer->setContentsMargins(0,0,0,0);
    lspacer->setFocusPolicy(Qt::NoFocus);
    lspacer->setAutoFillBackground(false);
    toolbar->addWidget(lspacer);

    // show hide sidebar
    side = new QPushButton(hideIcon, "", this);
    side->setFocusPolicy(Qt::NoFocus);
    side->setIconSize(QSize(15,15));
    side->setFixedWidth(20);
    side->setToolTip("Show/Hide Sidebar");
    side->setAutoFillBackground(false);
    side->setAutoDefault(false);
    side->setFlat(true);
    lspacerLayout->addWidget(side);
    connect(side, SIGNAL(clicked()), this, SLOT(toggleSidebar()));

    // switch tab/tile
    style = new QPushButton(tabIcon, "", this);
    style->setFocusPolicy(Qt::NoFocus);
    style->setToolTip("Toggle Tabbed Mode");
    style->setIconSize(QSize(15,15));
    style->setAutoFillBackground(false);
    style->setAutoDefault(false);
    style->setFixedWidth(20);
    style->setFlat(true);
    lspacerLayout->addWidget(style);
    connect(style, SIGNAL(clicked()), this, SLOT(toggleStyle()));

#ifndef Q_OS_MAC // full screen is in title bar on a Mac
    full = new QPushButton(fullIcon, "", this);
    full->setFocusPolicy(Qt::NoFocus);
    full->setToolTip("Toggle Full Screen");
    full->setIconSize(QSize(15,15));
    full->setAutoFillBackground(false);
    full->setAutoDefault(false);
    full->setFixedWidth(20);
    full->setFlat(true);
    lspacerLayout->addWidget(full);
    connect(full, SIGNAL(clicked()), this, SLOT(toggleFullScreen()));
#endif
    lspacerLayout->addStretch();

    trainTool = new TrainTool(this, home);
    trainTool->hide();
    toolbar->addWidget(trainTool->getToolbarButtons());

    // Analysis view buttons too.
    QHBoxLayout *toolbuttons=new QHBoxLayout;
    toolbuttons->setSpacing(0);
    toolbuttons->setContentsMargins(0,0,0,0);

    QIcon saveIcon(":images/oxygen/save.png");
    saveButton = new QPushButton(saveIcon, "", this);
    saveButton->setContentsMargins(0,0,0,0);
    saveButton->setFocusPolicy(Qt::NoFocus);
    saveButton->setIconSize(QSize(20,20));
    saveButton->setFixedWidth(24);
    saveButton->setAutoFillBackground(false);
    saveButton->setAutoDefault(false);
    saveButton->setFlat(true);
    saveButton->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    toolbuttons->addWidget(saveButton);
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveRide()));

    QIcon openIcon(":images/oxygen/open.png");
    QPushButton *open = new QPushButton(openIcon, "", this);
    open->setContentsMargins(0,0,0,0);
    open->setFocusPolicy(Qt::NoFocus);
    open->setIconSize(QSize(20,20));
    open->setFixedWidth(24);
    open->setAutoFillBackground(false);
    open->setAutoDefault(false);
    open->setFlat(true);
    open->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    toolbuttons->addWidget(open);
    QMenu *openMenu = new QMenu(this);
    open->setMenu(openMenu);
    openMenu->addAction(tr("Device Download"), this, SLOT(downloadRide()));
    openMenu->addAction(tr("Import file"), this, SLOT (importFile()));
    openMenu->addAction(tr("Manual activity"), this, SLOT(manualRide()));

#ifdef Q_OS_MAC
    QWindowsStyle *macstyler = new QWindowsStyle();
    saveButton->setStyle(macstyler);
    open->setStyle(macstyler);
#endif

    toolbuttons->addStretch();

    analButtons = new QWidget(this);
    analButtons->setContentsMargins(0,0,0,0);
    analButtons->setFocusPolicy(Qt::NoFocus);
    analButtons->setAutoFillBackground(false);
    analButtons->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    analButtons->setLayout(toolbuttons);
    analButtons->show();

    toolbar->addWidget(analButtons);
    toolbar->addStretch();

    // home
    QIcon homeIcon(":images/toolbar/main/home.png");
    homeAct = new QAction(homeIcon, tr("Home"), this);
    connect(homeAct, SIGNAL(triggered()), this, SLOT(selectHome()));
    toolbar->addAction(homeAct);

#ifdef GC_HAVE_ICAL
    // diary
    QIcon diaryIcon(":images/toolbar/main/diary.png");
    diaryAct = new QAction(diaryIcon, tr("Diary"), this);
    connect(diaryAct, SIGNAL(triggered()), this, SLOT(selectDiary()));
    toolbar->addAction(diaryAct);
#endif

    // analyse
    QIcon analysisIcon(":images/toolbar/main/analysis.png");
    analysisAct = new QAction(analysisIcon, tr("Analysis"), this);
    connect(analysisAct, SIGNAL(triggered()), this, SLOT(selectAnalysis()));
    toolbar->addAction(analysisAct);

#if 0 // XXX NOT YET IMPLEMENTED
    // measures
    QIcon measuresIcon(":images/toolbar/main/measures.png");
    measuresAct = new QAction(measuresIcon, tr("Measures"), this);
    //connect(measuresAct, SIGNAL(triggered()), this, SLOT(selectMeasures()));
    toolbar->addAction(measuresAct);
#endif

    // train
    QIcon trainIcon(":images/toolbar/main/train.png");
    trainAct = new QAction(trainIcon, tr("Train"), this);
    connect(trainAct, SIGNAL(triggered()), this, SLOT(selectTrain()));
    toolbar->addAction(trainAct);

#if 0 // XXX NOT YET IMPLEMENTED
    // athletes
    QIcon athleteIcon(":images/toolbar/main/athlete.png");
    athleteAct = new QAction(athleteIcon, tr("Athletes"), this);
    //connect(athleteAct, SIGNAL(triggered()), this, SLOT(athleteView()));
    toolbar->addAction(athleteAct);
#endif

    QWidget *rspacer = new QWidget(this);
    rspacer->setFixedWidth(100);
    rspacer->setContentsMargins(0,0,0,0);
    rspacer->setFocusPolicy(Qt::NoFocus);
    rspacer->setAutoFillBackground(false);
    QHBoxLayout *rspacerLayout = new QHBoxLayout(rspacer);
    rspacerLayout->addStretch();
    toolbar->addWidget(rspacer);

    // add chart button with a menu
    chartMenu = new QMenu(this);
    
    QIcon chartIcon(":images/addchart.png");
    QPushButton *newchart = new QPushButton(chartIcon, "", this);
    newchart->setFocusPolicy(Qt::NoFocus);
    newchart->setToolTip("Add Chart");
    newchart->setIconSize(QSize(15,15));
    newchart->setAutoFillBackground(false);
    newchart->setAutoDefault(false);
    newchart->setFixedWidth(20);
    newchart->setFlat(true);
    newchart->setMenu(chartMenu);
    rspacerLayout->addStretch();
    rspacerLayout->addWidget(newchart);

    connect(chartMenu, SIGNAL(aboutToShow()), this, SLOT(setChartMenu()));
    connect(chartMenu, SIGNAL(triggered(QAction*)), this, SLOT(addChart(QAction*)));

#ifdef Q_OS_MAC
    side->setStyle(macstyler);
    style->setStyle(macstyler);
    newchart->setStyle(macstyler);
#endif
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
    listView = new RideNavigator(this);
    // retrieve settings (properties are saved when we close the window)
    if (appsettings->cvalue(cyclist, GC_NAVHEADINGS, "").toString() != "") {
        listView->setSortByIndex(appsettings->cvalue(cyclist, GC_SORTBY).toInt());
        listView->setSortByOrder(appsettings->cvalue(cyclist, GC_SORTBYORDER).toInt());
        listView->setGroupBy(appsettings->cvalue(cyclist, GC_NAVGROUPBY).toInt());
        listView->setColumns(appsettings->cvalue(cyclist, GC_NAVHEADINGS).toString());
        listView->setWidths(appsettings->cvalue(cyclist, GC_NAVHEADINGWIDTHS).toString());
    }

    // INTERVALS
    intervalSummaryWindow = new IntervalSummaryWindow(this);
    intervalWidget = new IntervalTreeView(this);
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

    allIntervals = new QTreeWidgetItem(intervalWidget, FOLDER_TYPE);
    allIntervals->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);

    allIntervals->setText(0, tr("Intervals"));
    intervalWidget->expandItem(allIntervals);

    intervalSplitter = new QSplitter(this);
    intervalSplitter->setHandleWidth(1);
    intervalSplitter->setOrientation(Qt::Vertical);
    intervalSplitter->addWidget(intervalWidget);
    intervalSplitter->addWidget(intervalSummaryWindow);
    intervalSplitter->setFrameStyle(QFrame::NoFrame);

    QTreeWidgetItem *last = NULL;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(home));
    while (i.hasNext()) {
        QString name = i.next(), notesFileName;
        QDateTime dt;
        if (parseRideFileName(name, &notesFileName, &dt)) {
            last = new RideItem(RIDE_TYPE, home.path(),
                                name, dt, zones(), hrZones(), notesFileName, this);
            allRides->addChild(last);
        }
    }

    splitter = new QSplitter;

#if 0 // moved to toolbar, keep code in case we change our mind
    // CHARTS
    chartTool = new GcWindowTool(this);
#endif

    // TOOLBOX
    toolBox = new QToolBox(this);
    toolBox->setAcceptDrops(true);
    toolBox->setStyleSheet("QToolBox::tab {"
                           "max-height: 18px; "
                           "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                           //"stop: 0 #82848A, stop: 1.0 #53555A);" // XXX match toolbar
                           "stop: 0 #CFCFCF, stop: 1.0 #A8A8A8);"
                           "border: 1px solid rgba(255, 255, 255, 32);"
                           "color: #101010;"
                           "font-weight: bold; }");

    toolBox->setFrameStyle(QFrame::NoFrame);
    //toolBox->setPalette(toolbar->palette());
    toolBox->setContentsMargins(0,0,0,0);
    toolBox->layout()->setSpacing(0);

    //toolBox->setFixedWidth(350);

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
    homeWindow = new HomeWindow(this, "home", "Home");
    homeControls->addWidget(homeWindow->controls());
    homeControls->setCurrentIndex(0);

    // DIARY WINDOW & CONTROLS
    diaryWindow = new HomeWindow(this, "diary", "Diary");
    diaryControls->addWidget(diaryWindow->controls());

    // TRAIN WINDOW & CONTROLS
    trainWindow = new HomeWindow(this, "train", "Training");
    trainWindow->controls()->hide();
    trainControls->addWidget(trainWindow->controls());

    // ANALYSIS WINDOW & CONTRAOLS
    analWindow = new HomeWindow(this, "analysis", "Analysis");
    analysisControls->addWidget(analWindow->controls());
    currentWindow = analWindow;

    // POPULATE TOOLBOX
    toolBox->addItem(listView, QIcon(":images/activity.png"), "Activity History");
    toolBox->addItem(intervalSplitter, QIcon(":images/stopwatch.png"), "Activity Intervals");
    toolBox->addItem(trainTool->controls(), QIcon(":images/library.png"), "Workout Library");
    toolBox->addItem(masterControls, QIcon(":images/settings.png"), "Chart Settings");
#if 0 // XXX NOT YET IMPLEMENTED
    toolBox->addItem(new AthleteTool(QFileInfo(home.path()).path(), this), QIcon(":images/toolbar/main/athlete.png"), "Athletes");
#endif

#if 0 // moved to toolbar, keep code in case we change our mind
    toolBox->addItem(chartTool, QIcon(":images/addchart.png"), "Charts");
#endif


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

    views->setCurrentIndex(0);          // default to Analysis
    views->setContentsMargins(0,0,0,0);

    // SPLITTER
    splitter->addWidget(toolBox);
    splitter->addWidget(views);
    QVariant splitterSizes = appsettings->cvalue(cyclist, GC_SETTINGS_SPLITTER_SIZES); 
    if (splitterSizes != QVariant()) {
        splitter->restoreState(splitterSizes.toByteArray());
        splitter->setOpaqueResize(true); // redraw when released, snappier UI
    } else {
        QList<int> sizes;
        sizes.append(250);
        sizes.append(390);
        splitter->setSizes(sizes);
    }
    splitter->setStretchFactor(0,0);
    splitter->setStretchFactor(1,1);

    splitter->setChildrenCollapsible(false); // QT BUG crash QTextLayout do not undo this
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(" QSplitter::handle { "
                            "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, "
                            "stop:0 rgba(255, 255, 255, 0), "
                            "stop:0.407273 rgba(200, 200, 200, 255), "
                            "stop:0.4825 rgba(101, 104, 113, 235), "
                            "stop:0.6 rgba(255, 255, 255, 0)); }");

    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0, 0, 0, 0); // attempting to follow some UI guides

    // CENTRAL LAYOUT
    QWidget *central = new QWidget(this);
    central->setContentsMargins(0,0,0,0);

    QVBoxLayout *centralLayout = new QVBoxLayout(central);
    centralLayout->setSpacing(0);
    centralLayout->setContentsMargins(0,0,0,0);
    centralLayout->addWidget(toolbar);
    centralLayout->addWidget(splitter);

    setCentralWidget(central);

    /*----------------------------------------------------------------------
     * Application Menus
     *--------------------------------------------------------------------*/

    QMenu *fileMenu = menuBar()->addMenu(tr("&Athlete"));
    fileMenu->addAction(tr("&New..."), this, SLOT(newCyclist()), tr("Ctrl+N"));
    fileMenu->addAction(tr("&Open..."), this, SLOT(openCyclist()), tr("Ctrl+O"));
    fileMenu->addAction(tr("&Close Window"), this, SLOT(close()), tr(""));
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

    stravaAction = new QAction("Upload to Strava...", this);
    connect(stravaAction, SIGNAL(triggered(bool)), this, SLOT(uploadStrava()));
    rideMenu->addAction(stravaAction);


    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Save activity"), this, SLOT(saveRide()), tr("Ctrl+S"));
    rideMenu->addAction(tr("D&elete activity..."), this, SLOT(deleteRide()));
    rideMenu->addAction(tr("Split &activity..."), this, SLOT(splitRide()));
    rideMenu->addSeparator ();


    // XXX MEASURES ARE NOT IMPLEMENTED YET
#if 0
    QMenu *measureMenu = menuBar()->addMenu(tr("&Measure"));
    measureMenu->addAction(tr("&Record Measures..."), this,
                        SLOT(recordMeasure()), tr("Ctrl+R"));
    measureMenu->addSeparator();
    measureMenu->addAction(tr("&Export Measures..."), this,
                        SLOT(exportMeasures()), tr("Ctrl+E"));
    measureMenu->addAction(tr("&Import Measures..."), this,
                        SLOT(importMeasures()), tr("Ctrl+I"));
    measureMenu->addSeparator();
    measureMenu->addAction(tr("Get &Withings Data..."), this,
                        SLOT (downloadMeasures()), tr ("Ctrl+W"));
#endif

    QMenu *optionsMenu = menuBar()->addMenu(tr("&Tools"));
    optionsMenu->addAction(tr("&Options..."), this, SLOT(showOptions()), tr("Ctrl+O"));
    optionsMenu->addAction(tr("Critical Power Calculator..."), this, SLOT(showTools()));
    optionsMenu->addAction(tr("Air Density (Rho) Estimator..."), this, SLOT(showRhoEstimator()));

    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Workout Wizard"), this, SLOT(showWorkoutWizard()));
    optionsMenu->addAction(tr("Get Workouts from ErgDB"), this, SLOT(downloadErgDB()));

#ifdef GC_HAVE_ICAL
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Upload Activity to Calendar"), this, SLOT(uploadCalendar()), tr (""));
    optionsMenu->addAction(tr("Import Calendar..."), this, SLOT(importCalendar()), tr (""));
    optionsMenu->addAction(tr("Export Calendar..."), this, SLOT(exportCalendar()), tr (""));
    optionsMenu->addAction(tr("Refresh Calendar"), this, SLOT(refreshCalendar()), tr (""));
#endif
    optionsMenu->addSeparator();
    optionsMenu->addAction(tr("Find &best intervals..."), this, SLOT(findBestIntervals()), tr ("Ctrl+B"));
    optionsMenu->addAction(tr("Find power &peaks..."), this, SLOT(findPowerPeaks()), tr ("Ctrl+P"));

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
            QAction *action = new QAction(QString("%1...").arg(i.key()), this);
            optionsMenu->addAction(action);
            connect(action, SIGNAL(triggered()), toolMapper, SLOT(map()));
            toolMapper->setMapping(action, i.key());
        }
    }

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
#ifndef Q_OS_MAC
    viewMenu->addAction(tr("Toggle Full Screen"), this, SLOT(toggleFullScreen()), QKeySequence("F11"));
#endif
    showhideSidebar = viewMenu->addAction(tr("Show Sidebar"), this, SLOT(showSidebar(bool)));
    showhideSidebar->setCheckable(true);
    showhideSidebar->setChecked(true);
    //connect(showhideSidebar, SIGNAL(triggered(bool)), this, SLOT(showSidebar(bool)));

    showhideToolbar = viewMenu->addAction(tr("Show Toolbar"), this, SLOT(showToolbar(bool)));
    showhideToolbar->setCheckable(true);
    showhideToolbar->setChecked(true);

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
    subChartMenu = viewMenu->addMenu("Add Chart");
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

    connect(this, SIGNAL(rideDirty()), this, SLOT(enableSaveButton()));
    connect(this, SIGNAL(rideClean()), this, SLOT(enableSaveButton()));

    // Kick off
    rideTreeWidgetSelectionChanged();
    analWindow->selected();
    setStyle();
}

/*----------------------------------------------------------------------
 * GUI
 *--------------------------------------------------------------------*/

void
MainWindow::showDock()
{
    dock->toggleViewAction()->activate(QAction::Trigger);
}

void
MainWindow::toggleSidebar()
{
    showSidebar(!toolBox->isVisible());
}

void
MainWindow::showSidebar(bool want)
{
    static const QIcon hideIcon(":images/toolbar/main/hideside.png");
    static const QIcon showIcon(":images/toolbar/main/showside.png");

    if (want) {
        toolBox->show();
        side->setIcon(hideIcon);
    } else {
        toolBox->hide();
        side->setIcon(showIcon);
    }
    showhideSidebar->setChecked(toolBox->isVisible());
    setStyle();

}

void
MainWindow::showToolbar(bool want)
{
    if (want) toolbar->show();
    else toolbar->hide();
}

void
MainWindow::setChartMenu()
{
    unsigned int mask;
    // called when chart menu about to be shown
    // setup to only show charts that are relevant
    // to this view
    if (currentWindow == analWindow) mask = VIEW_ANALYSIS;
    if (currentWindow == trainWindow) mask = VIEW_TRAIN;
    if (currentWindow == diaryWindow) mask = VIEW_DIARY;
    if (currentWindow == homeWindow) mask = VIEW_HOME;

    chartMenu->clear();
    for(int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].relevance & mask) 
            chartMenu->addAction(GcWindows[i].name);
    }
}

void
MainWindow::setSubChartMenu()
{
    unsigned int mask;
    // called when chart menu about to be shown
    // setup to only show charts that are relevant
    // to this view
    if (currentWindow == analWindow) mask = VIEW_ANALYSIS;
    if (currentWindow == trainWindow) mask = VIEW_TRAIN;
    if (currentWindow == diaryWindow) mask = VIEW_DIARY;
    if (currentWindow == homeWindow) mask = VIEW_HOME;

    subChartMenu->clear();
    for(int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].relevance & mask) 
            subChartMenu->addAction(GcWindows[i].name);
    }
}

void
MainWindow::setActivityMenu()
{
    // enable/disable upload if already uploaded
    if (ride && ride->ride()) {

        QString activityId = ride->ride()->getTag("Strava uploadId", "");
        if (activityId == "") stravaAction->setEnabled(true);
        else stravaAction->setEnabled(false);
        
    } else {
        stravaAction->setEnabled(false);
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
        windowMenu->addAction(m->cyclist);
    }
}

void
MainWindow::selectWindow(QAction *act)
{
    foreach (MainWindow *m, mainwindows) {
        if (m->cyclist == act->text()) {
            m->activateWindow();
            m->raise();
            break;
        }
    }
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
        ride = NULL;
    else {
        QTreeWidgetItem *which = treeWidget->selectedItems().first();
        if (which->type() != RIDE_TYPE) return; // ignore!
        else
            ride = (RideItem*) which;
    }

    // update the ride property on all widgets
    // to let them know they need to replot new
    // selected ride
    _rideMetadata->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
    analWindow->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
    homeWindow->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
    diaryWindow->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
    trainWindow->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));

    enableSaveButton(); // should it be enabled or not?

    if (!ride) return;

    // refresh interval list for bottom left
    // first lets wipe away the existing intervals
    QList<QTreeWidgetItem *> intervals = allIntervals->takeChildren();
    for (int i=0; i<intervals.count(); i++) delete intervals.at(i);

    // now add the intervals for the current ride
    if (ride) { // only if we have a ride pointer
        RideFile *selected = ride->ride();
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
MainWindow::enableSaveButton()
{
    if (!ride) {
        saveButton->setEnabled(false);
        return;
    }

    if (ride->isDirty()) saveButton->setEnabled(true);
    else saveButton->setEnabled(false);
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

        QAction *actBestInt = new QAction(tr("Find Best Intervals"), treeWidget);
        connect(actBestInt, SIGNAL(triggered(void)), this, SLOT(findBestIntervals()));

        QAction *actPowerPeaks = new QAction(tr("Find Power Peaks"), treeWidget);
        connect(actPowerPeaks, SIGNAL(triggered(void)), this, SLOT(findPowerPeaks()));

        QAction *actSplitRide = new QAction(tr("Split Activity"), treeWidget);
        connect(actSplitRide, SIGNAL(triggered(void)), this, SLOT(splitRide()));

        if (rideItem->isDirty() == true) {
          menu.addAction(actSaveRide);
          menu.addAction(revertRide);
        }

        menu.addAction(actDeleteRide);
        menu.addAction(actBestInt);
        menu.addAction(actPowerPeaks);
        menu.addAction(actSplitRide);
#ifdef GC_HAVE_LIBOAUTH
        QAction *actTweetRide = new QAction(tr("Tweet Activity"), treeWidget);
        connect(actTweetRide, SIGNAL(triggered(void)), this, SLOT(tweetRide()));
        menu.addAction(actTweetRide);
#endif
#ifdef GC_HAVE_ICAL
        QAction *actUploadCalendar = new QAction(tr("Upload Activity to Calendar"), treeWidget);
        connect(actUploadCalendar, SIGNAL(triggered(void)), this, SLOT(uploadCalendar()));
        menu.addAction(actUploadCalendar);
#endif
        menu.exec(pos);
    }
}

void
MainWindow::showContextMenuPopup(const QPoint &pos)
{
    QTreeWidgetItem *trItem = intervalWidget->itemAt( pos );
    if (trItem != NULL && trItem->text(0) != tr("Intervals")) {
        QMenu menu(intervalWidget);

        activeInterval = (IntervalItem *)trItem;

        QAction *actRenameInt = new QAction(tr("Rename interval"), intervalWidget);
        QAction *actDeleteInt = new QAction(tr("Delete interval"), intervalWidget);
        QAction *actZoomInt = new QAction(tr("Zoom to interval"), intervalWidget);
        QAction *actFrontInt = new QAction(tr("Bring to Front"), intervalWidget);
        QAction *actBackInt = new QAction(tr("Send to back"), intervalWidget);
        connect(actRenameInt, SIGNAL(triggered(void)), this, SLOT(renameInterval(void)));
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteInterval(void)));
        connect(actZoomInt, SIGNAL(triggered(void)), this, SLOT(zoomInterval(void)));
        connect(actFrontInt, SIGNAL(triggered(void)), this, SLOT(frontInterval(void)));
        connect(actBackInt, SIGNAL(triggered(void)), this, SLOT(backInterval(void)));

        menu.addAction(actZoomInt);
        menu.addAction(actRenameInt);
        menu.addAction(actDeleteInt);
        menu.exec(intervalWidget->mapToGlobal( pos ));
    }
}

void
MainWindow::resizeEvent(QResizeEvent*)
{
    appsettings->setValue(GC_SETTINGS_MAIN_GEOM, geometry());
}

void
MainWindow::splitterMoved(int pos, int /*index*/)
{
    listView->setWidth(pos);
    appsettings->setCValue(cyclist, GC_SETTINGS_SPLITTER_SIZES, splitter->saveState());
}

void
MainWindow::showOptions()
{
    ConfigDialog *cd = new ConfigDialog(home, zones_, this);
    cd->exec();
    zonesChanged();
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

        // save ride list config
        appsettings->setCValue(cyclist, GC_SORTBY, listView->sortByIndex());
        appsettings->setCValue(cyclist, GC_SORTBYORDER, listView->sortByOrder());
        appsettings->setCValue(cyclist, GC_NAVGROUPBY, listView->groupBy());
        appsettings->setCValue(cyclist, GC_NAVHEADINGS, listView->columns());
        appsettings->setCValue(cyclist, GC_NAVHEADINGWIDTHS, listView->widths());

        // save the state of all the pages
        analWindow->saveState();
        homeWindow->saveState();
        trainWindow->saveState();
        diaryWindow->saveState();

        // clear the clipboard if neccessary
        QApplication::clipboard()->setText("");

        // now remove from the list
        if(mainwindows.removeOne(this) == false)
            qDebug()<<"closeEvent: mainwindows list error";
    }
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
    QMessageBox::about(this, tr("About GoldenCheetah"), tr(
            "<center>"
            "<h2>GoldenCheetah</h2>"
            "Cycling Power Analysis Software<br>for Linux, Mac, and Windows"
            "<p>Build date: %1 %2"
            "<p>Version: %3"
            "<p>GoldenCheetah is licensed under the<br>"
            "<a href=\"http://www.gnu.org/copyleft/gpl.html\">GNU General "
            "Public License</a>."
            "<p>Source code can be obtained from<br>"
            "<a href=\"http://goldencheetah.org/\">"
            "http://goldencheetah.org/</a>."
            "<p>Activity files and other data are stored in<br>"
            "<a href=\"%4\">%5</a>"
            "<p>Trademarks used with permission<br>"
            "TSS, NP, IF courtesy of <a href=\"http://www.peaksware.com\">"
            "Peaksware LLC</a>.<br>"
            "BikeScore, xPower courtesy of <a href=\"http://www.physfarm.com\">"
            "Physfarm Training Systems</a>.<br>"
            "Virtual Speed courtesy of Tom Compton <a href=\"http://www.analyticcycling.com\">"
            "Analytic Cycling</a>."
            "</center>"
            )
            .arg(__DATE__)
            .arg(__TIME__)
            .arg(GC_VERSION)
            .arg(QString(QUrl::fromLocalFile(home.absolutePath()).toEncoded()))
            .arg(home.absolutePath().replace(" ", "&nbsp;")));
}

void MainWindow::showTools()
{
   ToolsDialog *td = new ToolsDialog();
   td->show();
}

void MainWindow::showRhoEstimator()
{
   ToolsRhoEstimator *tre = new ToolsRhoEstimator();
   tre->show();
}

void MainWindow::showWorkoutWizard()
{
   WorkoutWizard *ww = new WorkoutWizard(this);
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
        ride = (RideItem*) allRides->child(i);
        if (ride->dateTime.date() == date) {
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
        ride = (RideItem*) allRides->child(i);
        if (ride->fileName == fileName) {
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
    RideItem *rideitem = (RideItem*)currentRideItem();
    if (rideitem) {
        ManualDataProcessorDialog *p = new ManualDataProcessorDialog(this, name, rideitem);
        p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
        p->exec();
    }
}

void
MainWindow::logBug()
{
    QDesktopServices::openUrl(QUrl("http://bugs.goldencheetah.org/projects/goldencheetah/issues/new"));
}

void
MainWindow::helpView()
{
    QDesktopServices::openUrl(QUrl("http://bugs.goldencheetah.org/projects/goldencheetah/wiki"));
}

void
MainWindow::selectAnalysis()
{
    masterControls->setCurrentIndex(0);
    views->setCurrentIndex(0);
    analWindow->selected(); // tell it!
    currentWindow = analWindow;
    analButtons->show();
    trainTool->getToolbarButtons()->hide();
    setStyle();
}

void
MainWindow::selectTrain()
{
    masterControls->setCurrentIndex(1);
    views->setCurrentIndex(1);
    trainWindow->selected(); // tell it!
    currentWindow = trainWindow;
    analButtons->hide();
    trainTool->getToolbarButtons()->show();
    setStyle();
}

void
MainWindow::selectDiary()
{
    masterControls->setCurrentIndex(2);
    views->setCurrentIndex(2);
    diaryWindow->selected(); // tell it!
    currentWindow = diaryWindow;
    analButtons->hide();
    trainTool->getToolbarButtons()->hide();
    setStyle();
}

void
MainWindow::selectHome()
{
    masterControls->setCurrentIndex(3);
    views->setCurrentIndex(3);
    homeWindow->selected(); // tell it!
    currentWindow = homeWindow;
    analButtons->hide();
    trainTool->getToolbarButtons()->hide();
    setStyle();
}
void
MainWindow::selectAthlete()
{
}

void
MainWindow::setStyle()
{
    static const QIcon tabIcon(":images/toolbar/main/tab.png");
    static const QIcon tileIcon(":images/toolbar/main/tile.png");

    if (!currentWindow) return;

    styleAction->setChecked(currentWindow->currentStyle == 0);
    style->setIcon((currentWindow->currentStyle == 0) ? tileIcon : tabIcon);
}

#ifdef GC_HAVE_LIBOAUTH
void
MainWindow::tweetRide()
{
    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE)
        return;

    RideItem *item = dynamic_cast<RideItem*>(_item);
    TwitterDialog *twitterDialog = new TwitterDialog(this, item);
    twitterDialog->setWindowModality(Qt::ApplicationModal);
    twitterDialog->exec();
}
#endif

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

    // We have something to process then
    RideImportWizard *dialog = new RideImportWizard (&urls, home, this);
    dialog->process(); // do it!
    return;
}

/*----------------------------------------------------------------------
 * Ride Library Functions
 *--------------------------------------------------------------------*/

void
MainWindow::addRide(QString name, bool /* bSelect =true*/)
{
    QString notesFileName;
    QDateTime dt;
    if (!parseRideFileName(name, &notesFileName, &dt)) {
        fprintf(stderr, "bad name: %s\n", name.toAscii().constData());
        assert(false);
    }
    RideItem *last = new RideItem(RIDE_TYPE, home.path(), name, dt, zones(), hrZones(), notesFileName, this);

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
    rideAdded(last); // here so emitted BEFORE rideSelected is emitted!
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


    QFile file(home.absolutePath() + "/" + strOldFileName);
    // purposefully don't remove the old ext so the user wouldn't have to figure out what the old file type was
    QString strNewName = strOldFileName + ".bak";

    // in case there was an existing bak file, delete it
    // ignore errors since it probably isn't there.
    QFile::remove(home.absolutePath() + "/" + strNewName);

    if (!file.rename(home.absolutePath() + "/" + strNewName))
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
        QFile::remove(home.absolutePath() + "/" + deleteMe);
    }

    // notify AFTER deleted from DISK..
    rideDeleted(item);

    // ..but before MEMORY cleared
    item->freeMemory();
    delete item;

    // any left?
    if (allRides->childCount() == 0) {
        ride = NULL;
        rideTreeWidgetSelectionChanged(); // notifies children
    }

    // added djconnel: remove old cpi file, then update bests which are associated with the file
    //XXX need to clean up in metricaggregator criticalPowerWindow->deleteCpiFile(strOldFileName);

    treeWidget->setCurrentItem(itemToSelect);
    rideTreeWidgetSelectionChanged();
}

void
MainWindow::downloadRide()
{
    (new DownloadRideDialog(this, home))->show();
}


void
MainWindow::manualRide()
{
    (new ManualRideDialog(this, home, useMetricUnits))->show();
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
    BatchExportDialog *d = new BatchExportDialog(this);
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
    bool result = RideFileFactory::instance().writeRideFile(this, currentRide(), file, getSuffix.cap(1));

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
    fileNames = QFileDialog::getOpenFileNames(
        this, tr("Import from File"), lastDir,
        allFormats.join(";;"));
    if (!fileNames.isEmpty()) {
        lastDir = QFileInfo(fileNames.front()).absolutePath();
        appsettings->setValue(GC_SETTINGS_LAST_IMPORT_PATH, lastDir);
        QStringList fileNamesCopy = fileNames; // QT doc says iterate over a copy
        RideImportWizard *import = new RideImportWizard(fileNamesCopy, home, this);
        import->process();
    }
}

void
MainWindow::saveRide()
{
    if (ride)
        saveRideSingleDialog(ride); // will signal save to everyone
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
    ride->freeMemory();
    ride->ride(); // force re-load

    // in case reverted ride has different starttime
    ride->setStartTime(ride->ride()->startTime()); // Note: this will also signal rideSelected()
    ride->ride()->emitReverted();
}

void
MainWindow::splitRide()
{
    if (ride) (new SplitActivityWizard(this))->exec();
    else QMessageBox::critical(this, tr("Split Activity"), tr("No activity selected!"));
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
 * Cyclists
 *--------------------------------------------------------------------*/

void
MainWindow::newCyclist()
{
    QDir newHome = home;
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
    QDir newHome = home;
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
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export Metrics"), QDir::homePath(), tr("Comma Separated Variables (*.csv)"));
    if (fileName.length() == 0)
        return;
    metricDB->writeAsCSV(fileName);
}

/*----------------------------------------------------------------------
* Strava.com
*--------------------------------------------------------------------*/

void
MainWindow::uploadStrava()
{
    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE) return;

    RideItem *item = dynamic_cast<RideItem*>(_item);

    if (item) { // menu is disabled anyway, but belt and braces
        StravaDialog d(this, item);
        d.exec();
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
        ErgDBDownloadDialog *d = new ErgDBDownloadDialog(this);
        d->exec();
    } else{
        QMessageBox::critical(this, tr("Workout Directory Invalid"), 
        "The workout directory is not configured, or the directory"
        " selected no longer exists.\n\n"
        "Please check your preference settings.");
    }
}

/*----------------------------------------------------------------------
 * TrainingPeaks.com
 *--------------------------------------------------------------------*/

#ifdef GC_HAVE_SOAP
void
MainWindow::uploadTP()
{
    if (ride) {
        TPUploadDialog uploader(cyclist, currentRide(), this);
        uploader.exec();
    }
}

void
MainWindow::downloadTP()
{
    TPDownloadDialog downloader(this);
    downloader.exec();
}
#endif

/*----------------------------------------------------------------------
 * Intervals
 *--------------------------------------------------------------------*/

void
MainWindow::findBestIntervals()
{
    BestIntervalDialog *p = new BestIntervalDialog(this);
    p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
    p->exec();
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

    if (!ride) return;

    QTreeWidgetItem *which = treeWidget->selectedItems().first();
    if (which->type() != RIDE_TYPE) {
        return;
    }

    addIntervalForPowerPeaksForSecs(ride->ride(), 5, "Peak 5s");
    addIntervalForPowerPeaksForSecs(ride->ride(), 10, "Peak 10s");
    addIntervalForPowerPeaksForSecs(ride->ride(), 20, "Peak 20s");
    addIntervalForPowerPeaksForSecs(ride->ride(), 30, "Peak 30s");
    addIntervalForPowerPeaksForSecs(ride->ride(), 60, "Peak 1min");
    addIntervalForPowerPeaksForSecs(ride->ride(), 120, "Peak 2min");
    addIntervalForPowerPeaksForSecs(ride->ride(), 300, "Peak 5min");
    addIntervalForPowerPeaksForSecs(ride->ride(), 600, "Peak 10min");
    addIntervalForPowerPeaksForSecs(ride->ride(), 1200, "Peak 20min");
    addIntervalForPowerPeaksForSecs(ride->ride(), 1800, "Peak 30min");
    addIntervalForPowerPeaksForSecs(ride->ride(), 3600, "Peak 60min");

    // now update the RideFileIntervals
    updateRideFileIntervals();
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

void
MainWindow::deleteInterval()
{
    // renumber remaining
    int oindex = activeInterval->displaySequence;
    for (int i=0; i<allIntervals->childCount(); i++) {
        IntervalItem *it = (IntervalItem *)allIntervals->child(i);
        int ds = it->displaySequence;
        if (ds > oindex) it->setDisplaySequence(ds-1);
    }

    // now delete!
    int index = allIntervals->indexOfChild(activeInterval);
    delete allIntervals->takeChild(index);
    updateRideFileIntervals(); // will emit intervalChanged() signal
}

void
MainWindow::renameInterval() {
    // go edit the name
    activeInterval->setFlags(activeInterval->flags() | Qt::ItemIsEditable);
    intervalWidget->editItem(activeInterval, 0);
}

void
MainWindow::intervalEdited(QTreeWidgetItem *, int) {
    // the user renamed the interval
    updateRideFileIntervals(); // will emit intervalChanged() signal
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

void MainWindow::getBSFactors(double &timeBS, double &distanceBS,
                              double &timeDP, double &distanceDP)
{
    int rides;
    double seconds, distance, bs, dp;
    seconds = rides = 0;
    distance = bs = dp = 0;
    timeBS = distanceBS = timeDP = distanceDP = 0.0;

    QVariant BSdays = appsettings->value(this, GC_BIKESCOREDAYS);
    if (BSdays.isNull() || BSdays.toInt() == 0)
        BSdays.setValue(30); // by default look back no more than 30 days

    // if there are rides, find most recent ride so we count back from there:
    if (allRides->childCount() == 0)
        return;

    RideItem *lastRideItem = (RideItem*) allRides->child(allRides->childCount() - 1);

    // just use the metricDB versions, nice 'n fast
    foreach (SummaryMetrics metric, metricDB->getAllMetricsFor(QDateTime() , QDateTime())) {

        int days =  metric.getRideDate().daysTo(lastRideItem->dateTime);
        if (days >= 0 && days < BSdays.toInt()) {

            bs += metric.getForSymbol("skiba_bike_score");
            seconds += metric.getForSymbol("time_riding");
            distance += metric.getForSymbol("total_distance");
            dp += metric.getForSymbol("daniels_points");

            rides++;
        }
    }

    // if we got any...
    if (rides) {
        if (!useMetricUnits)
            distance *= MILES_PER_KM;
        timeBS = (bs * 3600) / seconds;  // BS per hour
        distanceBS = bs / distance;  // BS per mile or km
        timeDP = (dp * 3600) / seconds;  // DP per hour
        distanceDP = dp / distance;  // DP per mile or km
    }
}

// set the rider value of CP to the value derived from the CP model extraction
void
MainWindow::setCriticalPower(int cp)
{
  // determine in which range to write the value: use the range associated with the presently selected ride
  int range;
  if (ride)
      range = ride->zoneRange();
  else {
      QDate today = QDate::currentDate();
      range = zones_->whichRange(today);
  }

  // add a new range if we failed to find a valid one
  if (range < 0) {
    // create an infinite range
    zones_->addZoneRange();
    range = 0;
  }

  zones_->setCP(range, cp);        // update the CP value
  zones_->setZonesFromCP(range);   // update the zones based on the value of CP
  zones_->write(home);             // write the output file

  QDate startDate = zones_->getStartDate(range);
  QDate endDate   =  zones_->getEndDate(range);
  QMessageBox::information(
			   this,
			   tr("CP saved"),
			   tr("Range from %1 to %2\nAthlete CP set to %3 watts") .
			   arg(startDate.isNull() ? "BEGIN" : startDate.toString()) .
			   arg(endDate.isNull() ? "END" : endDate.toString()) .
			   arg(cp)
			   );
  zonesChanged();
}

bool
MainWindow::parseRideFileName(const QString &name, QString *notesFileName, QDateTime *dt)
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
    *notesFileName = rx.cap(1) + ".notes";
    return true;
}

/*----------------------------------------------------------------------
 * Notifiers - application level events
 *--------------------------------------------------------------------*/

void
MainWindow::notifyConfigChanged()
{
    // re-read Zones in case it changed
    QFile zonesFile(home.absolutePath() + "/power.zones");
    if (zonesFile.exists()) {
        if (!zones_->read(zonesFile)) {
            QMessageBox::critical(this, tr("Zones File Error"),
                                 zones_->errorString());
        }
       else if (! zones_->warningString().isEmpty())
            QMessageBox::warning(this, tr("Reading Zones File"), zones_->warningString());
    }

    // reread HR zones
    QFile hrzonesFile(home.absolutePath() + "/hr.zones");
    if (hrzonesFile.exists()) {
        if (!hrzones_->read(hrzonesFile)) {
            QMessageBox::critical(this, tr("HR Zones File Error"),
                                 hrzones_->errorString());
        }
       else if (! hrzones_->warningString().isEmpty())
            QMessageBox::warning(this, tr("Reading HR Zones File"), hrzones_->warningString());
    }

    // now tell everyone else
    configChanged();
}

// notify children that rideSelected
// called by RideItem when its date/time changes
void
MainWindow::notifyRideSelected() {}

/*----------------------------------------------------------------------
 * Measures
 *--------------------------------------------------------------------*/

void
MainWindow::recordMeasure()
{
qDebug()<<"manually record measurements...";
}

void
MainWindow::downloadMeasures()
{
    withingsDownload->download();
}

void
MainWindow::exportMeasures()
{
    QDateTime start, end;
    end = QDateTime::currentDateTime();
    start.fromTime_t(0);

    foreach (SummaryMetrics x, metricDB->db()->getAllMeasuresFor(start, end)) {
qDebug()<<x.getDateTime();
qDebug()<<x.getText("Weight", "0.0").toDouble();
qDebug()<<x.getText("Lean Mass", "0.0").toDouble();
qDebug()<<x.getText("Fat Mass", "0.0").toDouble();
qDebug()<<x.getText("Fat Ratio", "0.0").toDouble();
    }
}

void
MainWindow::importMeasures() { }

void
MainWindow::refreshCalendar()
{
#ifdef GC_HAVE_ICAL
    davCalendar->download();
    calendarDownload->download();
#endif
}

/*----------------------------------------------------------------------
 * Calendar
 *--------------------------------------------------------------------*/

#ifdef GC_HAVE_ICAL
void
MainWindow::uploadCalendar()
{
    davCalendar->upload((RideItem*)currentRideItem()); // remove const coz it updates the ride
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
