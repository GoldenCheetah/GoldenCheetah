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
#include "AerolabWindow.h"
#include "Aerolab.h"
#include "GoogleMapControl.h"
#include "AllPlotWindow.h"
#include "AllPlot.h"
#include "BestIntervalDialog.h"
#include "ChooseCyclistDialog.h"
#include "Colors.h"
#include "Computrainer.h"
#include "ConfigDialog.h"
#include "CriticalPowerWindow.h"
#include "GcRideFile.h"
#ifdef GC_HAVE_KML
#include "KmlRideFile.h"
#endif
#include "PwxRideFile.h"
#include "LTMWindow.h"
#include "PfPvWindow.h"
#include "DownloadRideDialog.h"
#include "ManualRideDialog.h"
#include "HistogramWindow.h"
#include "ModelWindow.h"
#include "RealtimeWindow.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideEditor.h"
#include "RideFile.h"
#include "RideSummaryWindow.h"
#include "RideImportWizard.h"
#include "QuarqRideFile.h"
#include "RideMetadata.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include "WeeklySummaryWindow.h"
#include "Zones.h"
#include <assert.h>
#include <QApplication>
#include <QtGui>
#include <QRegExp>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_grid.h>
#include <qwt_data.h>
#include <boost/scoped_ptr.hpp>
#include "RideCalendar.h"
#include "DatePickerDialog.h"
#include "ToolsDialog.h"
#include "MetricAggregator.h"
#include "SplitRideDialog.h"
#include "PerformanceManagerWindow.h"
#include "TrainWindow.h"
#include "TwitterDialog.h"

#ifndef GC_VERSION
#define GC_VERSION "(developer build)"
#endif

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
			     tr("Invalid Ride File Name"),
			     tr("Invalid date/time in filename:\n%1\nSkipping file...").arg(name)
			     );
	return false;
    }
    *dt = QDateTime(date, time);
    *notesFileName = rx.cap(1) + ".notes";
    return true;
}

MainWindow::MainWindow(const QDir &home) : 
    home(home), session(0), isclean(false),
    zones_(new Zones), hrZones_(new HrZones), currentNotesChanged(false),
    ride(NULL)
{
    setAttribute(Qt::WA_DeleteOnClose);

    settings = GetApplicationSettings();

    GCColor *GCColorSet = new GCColor(this); // get/keep colorset
    GCColorSet->colorSet(); // shut up the compiler

    QVariant unit = settings->value(GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");

    setWindowTitle(home.dirName());
    settings->setValue(GC_SETTINGS_LAST, home.dirName());

    setWindowIcon(QIcon(":images/gc.png"));
    setAcceptDrops(true);

    QFile zonesFile(home.absolutePath() + "/power.zones");
    if (zonesFile.exists()) {
        if (!zones_->read(zonesFile)) {
            QMessageBox::critical(this, tr("Zones File Error"),
				  zones_->errorString());
        }
	else if (! zones_->warningString().isEmpty())
            QMessageBox::warning(this, tr("Reading Zones File"), zones_->warningString());
    }

    QFile hrZonesFile(home.absolutePath() + "/hr.zones");
    if (hrZonesFile.exists()) {
        if (!hrZones_->read(hrZonesFile)) {
            QMessageBox::critical(this, tr("Hr Zones File Error"),
                                  hrZones_->errorString());
        }
        else if (! hrZones_->warningString().isEmpty())
            QMessageBox::warning(this, tr("Reading Hr Zones File"), hrZones_->warningString());
    }

    QVariant geom = settings->value(GC_SETTINGS_MAIN_GEOM);
    if (geom == QVariant())
        resize(640, 480);
    else
        setGeometry(geom.toRect());

    splitter = new QSplitter;
    splitter->setContentsMargins(10, 20, 10, 10); // attempting to follow some UI guides

    // need to get rideNotes before metadata!
    rideNotes = new QTextEdit;

    // need to get metadata in before calendar!
    _rideMetadata = new RideMetadata(this);

    // Analysis toolbox contents
    calendar = new RideCalendar(this);
    calendar->setHome(home);

    treeWidget = new QTreeWidget;
    treeWidget->setColumnCount(3);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    // TODO: Test this on various systems with differing font settings (looks good on Leopard :)
    treeWidget->header()->resizeSection(0,70);
    treeWidget->header()->resizeSection(1,95);
    treeWidget->header()->resizeSection(2,70);
    //treeWidget->setMaximumWidth(250);
    treeWidget->header()->hide();
    treeWidget->setAlternatingRowColors (true);
    treeWidget->setIndentation(5);
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    allRides = new QTreeWidgetItem(treeWidget, FOLDER_TYPE);
    allRides->setText(0, tr("All Rides"));
    treeWidget->expandItem(allRides);

    intervalWidget = new QTreeWidget(this);
    intervalWidget->setColumnCount(1);
    intervalWidget->setIndentation(5);
    intervalWidget->setSortingEnabled(false);
    intervalWidget->header()->hide();
    intervalWidget->setAlternatingRowColors (true);
    intervalWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    intervalWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    intervalWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    intervalWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    allIntervals = new QTreeWidgetItem(intervalWidget, FOLDER_TYPE);
    allIntervals->setText(0, tr("Intervals"));
    intervalWidget->expandItem(allIntervals);

    intervalsplitter = new QSplitter(this);
    intervalsplitter->setOrientation(Qt::Vertical);
    intervalsplitter->addWidget(treeWidget);
    intervalsplitter->setCollapsible(0, true);
    intervalsplitter->addWidget(intervalWidget);
    intervalsplitter->setCollapsible(1, true);

    leftLayout = new QSplitter;
    viewSelection = new ViewSelection(this, VIEW_ANALYSIS);
    leftLayout->setOrientation(Qt::Vertical);
    leftLayout->addWidget(viewSelection);
    leftLayout->setCollapsible(0, false);
    leftLayout->addWidget(calendar);
    leftLayout->setCollapsible(1, true);
    leftLayout->addWidget(intervalsplitter);
    leftLayout->setCollapsible(2, false);

    splitter->addWidget(leftLayout);
    splitter->setCollapsible(0, true);
    QVariant calendarSizes = settings->value(GC_SETTINGS_CALENDAR_SIZES);
    if (calendarSizes != QVariant()) {
        leftLayout->restoreState(calendarSizes.toByteArray());
    }

    QTreeWidgetItem *last = NULL;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(home));
    while (i.hasNext()) {
        QString name = i.next(), notesFileName;
        QDateTime dt;
        if (parseRideFileName(name, &notesFileName, &dt)) {
            last = new RideItem(RIDE_TYPE, home.path(), 
                                name, dt, zones(), hrZones(), notesFileName, this);
            allRides->addChild(last);
	    calendar->update();
        }
    }

    tabWidget = new QTabWidget;
    tabWidget->setUsesScrollButtons(true);
    tabWidget->setElideMode(Qt::ElideNone);

    // setup trainWindow
    trainWindow = new TrainWindow(this, home);

    // Setup the two views
    // add the two views; Analysis and Train
    views = new QStackedWidget(this);
    setCentralWidget(views);
    views->addWidget(splitter);          // Analysis stuff
    views->addWidget(trainWindow);      // Train Stuff
    views->setCurrentIndex(0);          // default to Analysis

    rideSummaryWindow = new RideSummaryWindow(this);
    QLabel *notesLabel = new QLabel(tr("Notes"));
    notesLabel->setMaximumHeight(30);

    summarySplitter = new QSplitter;
    summarySplitter->setContentsMargins(0, 0, 0, 0);
    summarySplitter->setOrientation(Qt::Vertical);
    summarySplitter->addWidget(rideSummaryWindow);
    summarySplitter->setCollapsible(0, true);
    summarySplitter->addWidget(_rideMetadata);
    summarySplitter->setCollapsible(1, true);


    // Use last remembered size or set to a sensible default
    QVariant summarySizes = settings->value(GC_SETTINGS_SUMMARYSPLITTER_SIZES);
    if (summarySizes != QVariant())
        summarySplitter->restoreState(summarySizes.toByteArray());
    else {
        QList<int> sizes;
        sizes.append(650);
        sizes.append(350);
        summarySplitter->setSizes(sizes);
    }

    tabs.append(TabInfo(summarySplitter, tr("Ride Summary")));

    /////////////////////////// Ride Plot Tab ///////////////////////////
    allPlotWindow = new AllPlotWindow(this);
    tabs.append(TabInfo(allPlotWindow, tr("Ride Plot")));
    splitter->addWidget(tabWidget);
    splitter->setCollapsible(1, true);

    QVariant splitterSizes = settings->value(GC_SETTINGS_SPLITTER_SIZES); 
    if (splitterSizes != QVariant())
        splitter->restoreState(splitterSizes.toByteArray());
    else {
        QList<int> sizes;
        sizes.append(250);
        sizes.append(390);
        splitter->setSizes(sizes);
    }

    ////////////////////// Critical Power Plot Tab //////////////////////

    criticalPowerWindow = new CriticalPowerWindow(home, this);
    tabs.append(TabInfo(criticalPowerWindow, tr("Critical Power")));

    //////////////////////// Power Histogram Tab ////////////////////////

    histogramWindow = new HistogramWindow(this);
    tabs.append(TabInfo(histogramWindow, tr("Histograms")));
    
    //////////////////////// Pedal Force/zones_Velocity Plot ////////////////////////

    pfPvWindow = new PfPvWindow(this);
    tabs.append(TabInfo(pfPvWindow, tr("PF/PV")));

    //////////////////////// 3d Model Window ////////////////////////////

#ifdef GC_HAVE_QWTPLOT3D
    modelWindow = new ModelWindow(this, home);
    tabs.append(TabInfo(modelWindow, tr("3D")));
#endif

    //////////////////////// Weekly Summary ////////////////////////
    
    // add daily distance / duration graph:
    weeklySummaryWindow = new WeeklySummaryWindow(useMetricUnits, this);
    tabs.append(TabInfo(weeklySummaryWindow, tr("Weekly Summary")));

    //////////////////////// LTM ////////////////////////

    // long term metrics window
    metricDB = new MetricAggregator(this, home, zones(), hrZones()); // just to catch config updates!
    ltmWindow = new LTMWindow(this, useMetricUnits, home);
    tabs.append(TabInfo(ltmWindow, tr("Metrics")));

    //////////////////////// Performance Manager  ////////////////////////

    performanceManagerWindow = new PerformanceManagerWindow(this);
    tabs.append(TabInfo(performanceManagerWindow, tr("PM")));


    ///////////////////////////// Aerolab //////////////////////////////////

    aerolabWindow = new AerolabWindow(this);
    tabs.append(TabInfo(aerolabWindow, tr("Aerolab")));

    ///////////////////////////// GoogleMapsb //////////////////////////////////

    googleMap = new GoogleMapControl(this);
    tabs.append(TabInfo(googleMap, tr("Map")));

    ///////////////////////////// Editor  //////////////////////////////////

    rideEdit = new RideEditor(this);
    tabs.append(TabInfo(rideEdit, tr("Editor")));

    ////////////////////////////// Signals ////////////////////////////// 

    connect(calendar, SIGNAL(clicked(const QDate &)),
            this, SLOT(dateChanged(const QDate &)));
    connect(leftLayout, SIGNAL(splitterMoved(int,int)),
            this, SLOT(leftLayoutMoved()));
    connect(treeWidget,SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTreeContextMenuPopup(const QPoint &)));
    connect(treeWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(rideTreeWidgetSelectionChanged()));
    connect(splitter, SIGNAL(splitterMoved(int,int)), 
            this, SLOT(splitterMoved()));
    connect(summarySplitter, SIGNAL(splitterMoved(int,int)),
            this, SLOT(summarySplitterMoved()));
    connect(tabWidget, SIGNAL(currentChanged(int)), 
            this, SLOT(tabChanged(int)));
    connect(rideNotes, SIGNAL(textChanged()),
            this, SLOT(notesChanged()));
    connect(intervalWidget,SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenuPopup(const QPoint &)));
    connect(intervalWidget,SIGNAL(itemSelectionChanged()),
            this, SLOT(intervalTreeWidgetSelectionChanged()));
    connect(intervalWidget,SIGNAL(itemChanged(QTreeWidgetItem *,int)),
            this, SLOT(intervalEdited(QTreeWidgetItem*, int)));

    /////////////////////////////// Menus ///////////////////////////////

    QMenu *fileMenu = menuBar()->addMenu(tr("&Cyclist"));
    fileMenu->addAction(tr("&New..."), this,
                        SLOT(newCyclist()), tr("Ctrl+N"));
    fileMenu->addAction(tr("&Open..."), this,
                        SLOT(openCyclist()), tr("Ctrl+O"));
    fileMenu->addAction(tr("&Quit"), this,
                        SLOT(close()), tr("Ctrl+Q"));

    QMenu *rideMenu = menuBar()->addMenu(tr("&Ride"));
    rideMenu->addAction(tr("&Download from device..."), this,
                        SLOT(downloadRide()), tr("Ctrl+D"));
    rideMenu->addAction(tr("&Import from file..."), this,
                        SLOT (importFile()), tr ("Ctrl+I"));
    rideMenu->addAction(tr("&Manual ride entry..."), this,
                        SLOT(manualRide()), tr("Ctrl+M"));
    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Export to CSV..."), this,
                        SLOT(exportCSV()), tr("Ctrl+E"));
    rideMenu->addAction(tr("Export to GC..."), this,
                        SLOT(exportGC()));
#ifdef GC_HAVE_KML
    rideMenu->addAction(tr("&Export to KML..."), this,
                        SLOT(exportKML()));
#endif
    rideMenu->addAction(tr("Export to PWX..."), this,
                        SLOT(exportPWX()));
    rideMenu->addSeparator ();
    rideMenu->addAction(tr("&Save ride"), this,
                        SLOT(saveRide()), tr("Ctrl+S"));
    rideMenu->addAction(tr("D&elete ride..."), this,
                        SLOT(deleteRide()));
    rideMenu->addAction(tr("Split &ride..."), this,
                        SLOT(splitRide()));
    rideMenu->addSeparator ();
    rideMenu->addAction(tr("Find &best intervals..."), this,
                        SLOT(findBestIntervals()), tr ("Ctrl+B"));
    rideMenu->addAction(tr("Find power &peaks..."), this,
                        SLOT(findPowerPeaks()), tr ("Ctrl+P"));

    QMenu *optionsMenu = menuBar()->addMenu(tr("&Tools"));
    optionsMenu->addAction(tr("&Options..."), this, 
                           SLOT(showOptions()), tr("Ctrl+O")); 
    optionsMenu->addAction(tr("Critical Power Calculator"), this,
                           SLOT(showTools()));
    //optionsMenu->addAction(tr("&Reset Metrics..."), this, 
    //                       SLOT(importRideToDB()), tr("Ctrl+R")); 
    //optionsMenu->addAction(tr("&Update Metrics..."), this, 
    //                       SLOT(scanForMissing()()), tr("Ctrl+U")); 

    // get the available processors
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
            // Changed to show localized processor name in menu
            QAction *action = new QAction(QString("%1...").arg(i.value()->name()), this);
            optionsMenu->addAction(action);
            connect(action, SIGNAL(triggered()), toolMapper, SLOT(map()));
            toolMapper->setMapping(action, i.key());
        }
    }



    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QStringList tabsToHide = settings->value(GC_TABS_TO_HIDE, "").toString().split(",");
    for (int i = 0; i < tabs.size(); ++i) {
        TabInfo &tab = tabs[i]; // QListIterator only returns const references.
        bool hide = tabsToHide.contains(tab.name);
        if (!hide)
            tabWidget->addTab(tab.contents, tab.name);
        if (tab.contents == summarySplitter)
            continue; // Always show the ride summary.
        tab.action = new QAction(tab.name, this);
        tab.action->setCheckable(true);
        tab.action->setChecked(!hide);
        connect(tab.action, SIGNAL(triggered(bool)), this, SLOT(tabViewTriggered(bool)));
        viewMenu->addAction(tab.action);
    }
 
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About GoldenCheetah"), this, SLOT(aboutDialog()));


       
    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked);
    if(isAscending.toInt()>0){
            if (last != NULL)
                treeWidget->setCurrentItem(last);
    } else {
        // selects the first ride in the list:
        if (allRides->child(0) != NULL){
            treeWidget->scrollToItem(allRides->child(0), QAbstractItemView::EnsureVisible);
            treeWidget->setCurrentItem(allRides->child(0));
        }
    }

    treeWidget->setFocus(); // For MacOs QT (4.6.0-4.7.0 bug)

    setAttribute(Qt::WA_DeleteOnClose);
}

void
MainWindow::tabViewTriggered(bool)
{
    disconnect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    QWidget *currentWidget = tabWidget->currentWidget();
    int currentIndex = tabWidget->currentIndex();
    tabWidget->clear();
    QStringList tabsToHide;
    foreach (const TabInfo &tab, tabs) {
        if (!tab.action || tab.action->isChecked())
            tabWidget->addTab(tab.contents, tab.name);
        else
            tabsToHide << tab.name;
    }
    if (tabWidget->indexOf(currentWidget) >= 0)
        tabWidget->setCurrentWidget(currentWidget);
    else if (currentIndex < tabWidget->count())
        tabWidget->setCurrentIndex(currentIndex);
    tabChanged(tabWidget->currentIndex());
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    settings->setValue(GC_TABS_TO_HIDE, tabsToHide.join(","));
}

void
MainWindow::selectView(int view)
{
    if (view == VIEW_ANALYSIS)
        views->setCurrentIndex(0); // set stacked widget to Analysis
    else if (view == VIEW_TRAIN)
        views->setCurrentIndex(1); // set stacked widget to Train

    // notify with a signal
    viewChanged(view);
}

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

void
MainWindow::addRide(QString name, bool bSelect /*=true*/)
{
    QString notesFileName;
    QDateTime dt;
    if (!parseRideFileName(name, &notesFileName, &dt)) {
        fprintf(stderr, "bad name: %s\n", name.toAscii().constData());
        assert(false);
    }
    RideItem *last = new RideItem(RIDE_TYPE, home.path(), 
                                  name, dt, zones(), hrZones(), notesFileName, this);

    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked); // default is ascending sort
    int index = 0;
    while (index < allRides->childCount()) {
        QTreeWidgetItem *item = allRides->child(index);
        if (item->type() != RIDE_TYPE)
            continue;
        RideItem *other = static_cast<RideItem*>(item);
        
        if(isAscending.toInt() > 0 ){
            if (other->dateTime > dt)
                break;
        } else {
            if (other->dateTime < dt)
                break; 
        }
        if (other->fileName == name) {
            delete allRides->takeChild(index);
            break;
        }
        ++index;
    }
    allRides->insertChild(index, last);
    calendar->update();
    criticalPowerWindow->newRideAdded();
    if (bSelect)
    {
        tabWidget->setCurrentWidget(summarySplitter);
        treeWidget->setCurrentItem(last);
    }
    rideAdded(last);
}

void
MainWindow::removeCurrentRide()
{
    int x = 0;

    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item->type() != RIDE_TYPE)
        return;
    RideItem *item = static_cast<RideItem*>(_item);

    rideDeleted(item);

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
    delete item;

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

    // added djconnel: remove old cpi file, then update bests which are associated with the file
    criticalPowerWindow->deleteCpiFile(strOldFileName);

    treeWidget->setCurrentItem(itemToSelect);
    rideTreeWidgetSelectionChanged();
    calendar->update();
}

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
MainWindow::exportPWX()
{
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export PWX"), QDir::homePath(), tr("PWX (*.pwx)"));
    if (fileName.length() == 0)
        return;

    QString err;
    QFile file(fileName);
    PwxFileReader reader;
    reader.writeRideFile(home.dirName() /* cyclist name */, currentRide(), file);
}

void
MainWindow::exportGC()
{
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export GC"), QDir::homePath(), tr("GC (*.gc)"));
    if (fileName.length() == 0)
        return;

    QString err;
    QFile file(fileName);
    GcFileReader reader;
    reader.writeRideFile(currentRide(), file);
}

#ifdef GC_HAVE_KML
void
MainWindow::exportKML()
{
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export KML"), QDir::homePath(), tr("Google Earth KML (*.kml)"));
    if (fileName.length() == 0)
        return;

    QString err;
    QFile file(fileName);
    KmlFileReader reader;
    reader.writeRideFile(currentRide(), file);
}
#endif

void
MainWindow::exportCSV()
{
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    ride = (RideItem*) treeWidget->selectedItems().first();

    // Ask the user if they prefer to export with English or metric units.
    QStringList items;
    items << tr("Metric") << tr("Imperial");
    bool ok;
    QString units = QInputDialog::getItem(
        this, tr("Select Units"), tr("Units:"), items, 0, false, &ok);
    if(!ok) 
        return;
    bool useMetricUnits = (units == items[0]);

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export CSV"), QDir::homePath(),
        tr("Comma-Separated Values (*.csv)"));
    if (fileName.length() == 0)
        return;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        QMessageBox::critical(this, tr("Split Ride"), tr("The file %1 can't be opened for writing").arg(fileName));
        return;
    }

    ride->ride()->writeAsCsv(file, useMetricUnits);
}

void
MainWindow::importFile()
{
    QVariant lastDirVar = settings->value(GC_SETTINGS_LAST_IMPORT_PATH);
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
        settings->setValue(GC_SETTINGS_LAST_IMPORT_PATH, lastDir);
        QStringList fileNamesCopy = fileNames; // QT doc says iterate over a copy
        RideImportWizard *import = new RideImportWizard(fileNamesCopy, home, this);
        import->process();
    }
}

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
    allIntervals->addChild(peak);
}

void
MainWindow::findPowerPeaks()
{
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

//----------------------------------------------------------------------
// User-define Intervals and Interval manipulation on left layout
//----------------------------------------------------------------------

void 
MainWindow::rideTreeWidgetSelectionChanged()
{
    assert(treeWidget->selectedItems().size() <= 1);
    if (treeWidget->selectedItems().isEmpty())
        ride = NULL;
    else {
        QTreeWidgetItem *which = treeWidget->selectedItems().first();
        if (which->type() != RIDE_TYPE)
            ride = NULL;
        else
            ride = (RideItem*) which;
    }
    rideSelected();

    if (!ride)
        return;

    calendar->setSelectedDate(ride->dateTime.date());

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
                allIntervals->addChild(add);
            }
        }
    }

	// turn off tabs that don't make sense for manual file entry
    int histIndex = tabWidget->indexOf(histogramWindow);
    int pfpvIndex = tabWidget->indexOf(pfPvWindow);
    int plotIndex = tabWidget->indexOf(allPlotWindow);
    int modelIndex = tabWidget->indexOf(modelWindow);
    int mapIndex   = tabWidget->indexOf(googleMap);
    int editorIndex   = tabWidget->indexOf(rideEdit);

    bool enabled = (ride->ride() && ride->ride()->deviceType() != QString("Manual CSV") &&
                     !ride->ride()->dataPoints().isEmpty());

    if (histIndex >= 0) tabWidget->setTabEnabled(histIndex, enabled);
    if (pfpvIndex >= 0) tabWidget->setTabEnabled(pfpvIndex, enabled);
    if (plotIndex >= 0) tabWidget->setTabEnabled(plotIndex, enabled);
    if (modelIndex >= 0) tabWidget->setTabEnabled(modelIndex, enabled);
    if (mapIndex >= 0) tabWidget->setTabEnabled(mapIndex, enabled);
    if (editorIndex >= 0) tabWidget->setTabEnabled(editorIndex, enabled);
    saveAndOpenNotes();
}
void
MainWindow::showTreeContextMenuPopup(const QPoint &pos)
{
    QTreeWidgetItem *trItem = treeWidget->itemAt( pos );
    if (trItem != NULL && trItem->text(0) != tr("All Rides")) {
        QMenu menu(treeWidget);

        RideItem *rideItem = (RideItem *)treeWidget->selectedItems().first();

        activeRide = (RideItem *)trItem;

        QAction *actSaveRide = new QAction(tr("Save Changes to Ride"), treeWidget);
        connect(actSaveRide, SIGNAL(triggered(void)), this, SLOT(saveRide()));

        QAction *revertRide = new QAction(tr("Revert to Saved Ride"), treeWidget);
        connect(revertRide, SIGNAL(triggered(void)), this, SLOT(revertRide()));

        QAction *actDeleteRide = new QAction(tr("Delete Ride"), treeWidget);
        connect(actDeleteRide, SIGNAL(triggered(void)), this, SLOT(deleteRide()));

        QAction *actBestInt = new QAction(tr("Find Best Intervals"), treeWidget);
        connect(actBestInt, SIGNAL(triggered(void)), this, SLOT(findBestIntervals()));

        QAction *actPowerPeaks = new QAction(tr("Find Power Peaks"), treeWidget);
        connect(actPowerPeaks, SIGNAL(triggered(void)), this, SLOT(findPowerPeaks()));

        QAction *actSplitRide = new QAction(tr("Split Ride"), treeWidget);
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
        QAction *actTweetRide = new QAction(tr("Tweet Ride"), treeWidget);
        connect(actTweetRide, SIGNAL(triggered(void)), this, SLOT(tweetRide()));
        menu.addAction(actTweetRide);
#endif

        menu.exec(treeWidget->mapToGlobal( pos ));
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

        if (tabWidget->currentWidget() == allPlotWindow || tabWidget->currentWidget() == aerolabWindow)
            menu.addAction(actZoomInt);
        menu.addAction(actRenameInt);
        menu.addAction(actDeleteInt);
        if (tabWidget->currentWidget() == pfPvWindow && activeInterval->isSelected()) {
            menu.addAction(actFrontInt);
            menu.addAction(actBackInt);
        }
        menu.exec(intervalWidget->mapToGlobal( pos ));
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
    allPlotWindow->zoomInterval(activeInterval);
    aerolabWindow->zoomInterval(activeInterval);
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

void MainWindow::getBSFactors(double &timeBS, double &distanceBS,
                              double &timeDP, double &distanceDP)
{
    int rides;
    double seconds, distance, bs, dp;
    QProgressDialog * progress;
    bool aborted = false;
    seconds = rides = 0;
    distance = bs = dp = 0;
    timeBS = distanceBS = timeDP = distanceDP = 0.0;

    QVariant BSdays = settings->value(GC_BIKESCOREDAYS);
    if (BSdays.isNull() || BSdays.toInt() == 0)
        BSdays.setValue(30); // by default look back no more than 30 days

    // if there are rides, find most recent ride so we count back from there:
    if (allRides->childCount() == 0)
        return;

    RideItem *lastRideItem = (RideItem*) allRides->child(allRides->childCount() - 1);

    // set up progress bar
    progress = new QProgressDialog(QString(tr("Computing bike score estimating factors.\n")),
                                   tr("Abort"),0,BSdays.toInt(),this);
    int endingOffset = progress->labelText().size();

    for (int i = 0; i < allRides->childCount(); ++i) {
        RideItem *item = (RideItem*) allRides->child(i);
        int days =  item->dateTime.daysTo(lastRideItem->dateTime);
        if ((item->type() == RIDE_TYPE)
            && (days >= 0) && (days < BSdays.toInt())
            && (item->ride())
            && (item->ride()->deviceType() != QString("Manual CSV"))) {

            RideMetricPtr m;
            item->computeMetrics();

            QString existing = progress->labelText();
            existing.chop(progress->labelText().size() - endingOffset);
            progress->setLabelText(
                existing + QString(tr("Processing %1...")).arg(item->fileName));

            // only count rides with BS > 0
            if ((m = item->metrics.value("skiba_bike_score")) &&
                m->value(true)) {
                bs += m->value(true);

                if ((m = item->metrics.value("time_riding"))) {
                    seconds += m->value(true);
                }

                if ((m = item->metrics.value("total_distance"))) {
                    distance += m->value(true);
                }

                if ((m = item->metrics.value("daniels_points"))) {
                    dp += m->value(true);
                }

                rides++;
            }
            // check progress
            QCoreApplication::processEvents();
            if (progress->wasCanceled()) {
                aborted = true;
                goto done;
            }
            // set progress from 0 to BSdays
            progress->setValue(BSdays.toInt() - days);
        }
    }
    if (rides) {
        if (!useMetricUnits)
            distance *= MILES_PER_KM;
        timeBS = (bs * 3600) / seconds;  // BS per hour
        distanceBS = bs / distance;  // BS per mile or km
        timeDP = (dp * 3600) / seconds;  // DP per hour
        distanceDP = dp / distance;  // DP per mile or km
    }
done:
    if (aborted) {
        timeBS = distanceBS = timeDP = distanceDP = 0;
    }

    delete progress;
}

void
MainWindow::saveAndOpenNotes()
{
    // First save the contents of the notes window.
    saveNotes();

    // Now open any notes associated with the new ride.
    rideNotes->setPlainText("");
    QString notesPath = home.absolutePath() + "/" + ride->notesFileName;
    QFile notesFile(notesPath);

    if (notesFile.exists()) {
        if (notesFile.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&notesFile);
            rideNotes->setPlainText(in.readAll());
            notesFile.close();
        }
        else {
            QMessageBox::critical(
                this, tr("Read Error"),
                tr("Can't read notes file %1").arg(notesPath));
        }
    }

    currentNotesFile = ride->notesFileName;
    currentNotesChanged = false;
}

void MainWindow::saveNotes() 
{
    if ((currentNotesFile != "") && currentNotesChanged) {
        QString notesPath = 
            home.absolutePath() + "/" + currentNotesFile;
        QString tmpPath = notesPath + ".tmp";
        QFile tmp(tmpPath);
        if (tmp.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&tmp);
            out << rideNotes->toPlainText();
            tmp.close();
            QFile::remove(notesPath);
            if (rename(tmpPath.toAscii().constData(),
                       notesPath.toAscii().constData()) == -1) {
                QMessageBox::critical(
                    this, tr("Write Error"),
                    tr("Can't rename %1 to %2")
                    .arg(tmpPath).arg(notesPath));
            }
        }
        else {
            QMessageBox::critical(
                this, tr("Write Error"),
                tr("Can't write notes file %1").arg(tmpPath));
        }
    }
}

void 
MainWindow::resizeEvent(QResizeEvent*)
{
    settings->setValue(GC_SETTINGS_MAIN_GEOM, geometry());
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
    settings->setValue(GC_SETTINGS_MAIN_GEOM, geometry());
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    if (saveRideExitDialog() == false) event->ignore();
    else {
        saveNotes();
        QApplication::clipboard()->setText("");
    }
}

void
MainWindow::leftLayoutMoved()
{
    settings->setValue(GC_SETTINGS_CALENDAR_SIZES, leftLayout->saveState());
}

void
MainWindow::splitterMoved()
{
    settings->setValue(GC_SETTINGS_SPLITTER_SIZES, splitter->saveState());
}

void
MainWindow::summarySplitterMoved()
{
    settings->setValue(GC_SETTINGS_SUMMARYSPLITTER_SIZES, summarySplitter->saveState());
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
			   tr("Range from %1 to %2\nRider CP set to %3 watts") .
			   arg(startDate.isNull() ? "BEGIN" : startDate.toString()) .
			   arg(endDate.isNull() ? "END" : endDate.toString()) .
			   arg(cp)
			   );
  zonesChanged();
}

void
MainWindow::tabChanged(int)
{
    rideSelected();
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
            "<p>Ride files and other data are stored in<br>"
            "<a href=\"%4\">%5</a>"
            "</center>"
            )
            .arg(__DATE__)
            .arg(__TIME__)
            .arg(GC_VERSION)
            .arg(QString(QUrl::fromLocalFile(home.absolutePath()).toEncoded()))
            .arg(home.absolutePath().replace(" ", "&nbsp;")));
}


void MainWindow::importRideToDB()
{
    metricDB->refreshMetrics();
}

void MainWindow::scanForMissing()
{
    metricDB->refreshMetrics();
}



void
MainWindow::notesChanged()
{
    currentNotesChanged = true;
}

void MainWindow::showTools()
{
   ToolsDialog *td = new ToolsDialog();
   td->show();
}

void
MainWindow::saveRide()
{
    saveRideSingleDialog(ride); // will update Dirty flag if saved
    ride->computeMetricsTime = QDateTime();
    ride->computeMetrics();
    rideSelected();
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
    (new SplitRideDialog(this))->exec();
}

void
MainWindow::deleteRide()
{
    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE)
        return;
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
        removeCurrentRide();
}

/*
 *  This slot gets called when the user picks a new date, using the mouse,
 *  in the calendar.  We have to adjust TreeView to match.
 */
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

// notify children that config has changed
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

    // re-read Hr Zones in case it changed
    QFile hrZonesFile(home.absolutePath() + "/hr.zones");
    if (hrZonesFile.exists()) {
        if (!hrZones_->read(hrZonesFile)) {
            QMessageBox::critical(this, tr("Hr Zones File Error"),
                                  hrZones_->errorString());
        }
        else if (! hrZones_->warningString().isEmpty())
            QMessageBox::warning(this, tr("Reading Hr Zones File"), hrZones_->warningString());
    }

    // now tell everyone else
    configChanged();
}

// notify children that rideSelected
// called by RideItem when its date/time changes
void
MainWindow::notifyRideSelected()
{
    rideSelected();
}

void
MainWindow::manualProcess(QString name)
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

#ifdef GC_HAVE_LIBOAUTH
void
MainWindow::tweetRide()
{
    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item==NULL || _item->type() != RIDE_TYPE)
        return;

    RideItem *item = dynamic_cast<RideItem*>(_item);
    item->computeMetrics();

    TwitterDialog *twitterDialog = new TwitterDialog(this, item);
    twitterDialog->setWindowModality(Qt::ApplicationModal);
    twitterDialog->exec();
}
#endif

