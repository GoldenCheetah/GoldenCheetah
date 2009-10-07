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
#include "AllPlotWindow.h"
#include "BestIntervalDialog.h"
#include "ChooseCyclistDialog.h"
#include "ConfigDialog.h"
#include "CpintPlot.h"
#include "PfPvWindow.h"
#include "DownloadRideDialog.h"
#include "ManualRideDialog.h"
#include "HistogramWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "QuarqRideFile.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
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

#ifndef GC_VERSION
#define GC_VERSION "(developer build)"
#endif

#define FOLDER_TYPE 0
#define RIDE_TYPE 1

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
    home(home), 
    zones(NULL), currentNotesChanged(false),
    ride(NULL)
{
    settings = GetApplicationSettings();
      
    QVariant unit = settings->value(GC_UNIT);
    useMetricUnits = (unit.toString() == "Metric");

    setWindowTitle(home.dirName());
    settings->setValue(GC_SETTINGS_LAST, home.dirName());

    setWindowIcon(QIcon(":images/gc.png"));
    setAcceptDrops(true);

    QFile zonesFile(home.absolutePath() + "/power.zones");
    if (zonesFile.exists()) {
        zones = new Zones();
        if (!zones->read(zonesFile)) {
            QMessageBox::critical(this, tr("Zones File Error"),
				  zones->errorString());
            delete zones;
            zones = NULL;
        }
	else if (! zones->warningString().isEmpty())
            QMessageBox::warning(this, tr("Reading Zones File"), zones->warningString());
    }

    QVariant geom = settings->value(GC_SETTINGS_MAIN_GEOM);
    if (geom == QVariant())
        resize(640, 480);
    else
        setGeometry(geom.toRect());

    splitter = new QSplitter(this);
    setCentralWidget(splitter);
    splitter->setContentsMargins(10, 20, 10, 10); // attempting to follow some UI guides

    calendar = new RideCalendar;
    calendar->setFirstDayOfWeek(Qt::Monday);
    calendar->setHome(home);
    calendar->addWorkoutCode(QString("race"), QColor(Qt::red));
    calendar->addWorkoutCode(QString("sick"), QColor(Qt::yellow));
    calendar->addWorkoutCode(QString("swim"), QColor(Qt::blue));
    calendar->addWorkoutCode(QString("gym"), QColor(Qt::gray));

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

    allRides = new QTreeWidgetItem(treeWidget, FOLDER_TYPE);
    allRides->setText(0, tr("All Rides"));
    treeWidget->expandItem(allRides);

    leftLayout = new QSplitter;
    leftLayout->setOrientation(Qt::Vertical);
    leftLayout->addWidget(calendar);
    leftLayout->setCollapsible(0, false);
    leftLayout->addWidget(treeWidget);
    leftLayout->setCollapsible(1, false);
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
                                name, dt, &zones, notesFileName);
            allRides->addChild(last);
	    calendar->addRide(reinterpret_cast<RideItem*>(last));
        }
    }

    tabWidget = new QTabWidget;
    tabWidget->setUsesScrollButtons(true);
    rideSummary = new QTextEdit;
    rideSummary->setReadOnly(true);
    tabWidget->addTab(rideSummary, tr("Ride Summary"));

    /////////////////////////// Ride Plot Tab ///////////////////////////
    allPlotWindow = new AllPlotWindow(this);
    tabWidget->addTab(allPlotWindow, "Ride Plot");
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

    QWidget *window = new QWidget;
    QVBoxLayout *vlayout = new QVBoxLayout;
    QHBoxLayout *cpintPickerLayout = new QHBoxLayout;
    QLabel *cpintTimeLabel = new QLabel(tr("Interval Duration:"), window);
    cpintTimeValue = new QLineEdit("0 s");
    QLabel *cpintTodayLabel = new QLabel(tr("Today:"), window);
    cpintTodayValue = new QLineEdit(tr("no data"));
    QLabel *cpintAllLabel = new QLabel(tr("All Rides:"), window);
    cpintAllValue = new QLineEdit(tr("no data"));
    cpintTimeValue->setReadOnly(true);
    cpintTodayValue->setReadOnly(true);
    cpintAllValue->setReadOnly(true);

    cpintSetCPButton = new QPushButton(tr("&Save CP value"), this);
    cpintSetCPButton->setEnabled(false);

    cpintPickerLayout->addWidget(cpintTimeLabel);
    cpintPickerLayout->addWidget(cpintTimeValue);
    cpintPickerLayout->addWidget(cpintTodayLabel);
    cpintPickerLayout->addWidget(cpintTodayValue);
    cpintPickerLayout->addWidget(cpintAllLabel);
    cpintPickerLayout->addWidget(cpintAllValue);
    cpintPickerLayout->addWidget(cpintSetCPButton);
    cpintPlot = new CpintPlot(home.path());
    vlayout->addWidget(cpintPlot);
    vlayout->addLayout(cpintPickerLayout);
    window->setLayout(vlayout);
    tabWidget->addTab(window, "Critical Power Plot");

    picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::PointSelection, 
                               QwtPicker::VLineRubberBand, 
                               QwtPicker::AlwaysOff, cpintPlot->canvas());
    picker->setRubberBandPen(QColor(Qt::blue));

    connect(picker, SIGNAL(moved(const QPoint &)),
            SLOT(pickerMoved(const QPoint &)));

    //////////////////////// Power Histogram Tab ////////////////////////

    histogramWindow = new HistogramWindow(this);
    tabWidget->addTab(histogramWindow, "Histogram Analysis");
    
    //////////////////////// Pedal Force/Velocity Plot ////////////////////////

    pfPvWindow = new PfPvWindow(this);
    tabWidget->addTab(pfPvWindow, tr("PF/PV Plot"));


    //////////////////////// Ride Notes ////////////////////////
    
    rideNotes = new QTextEdit;
    tabWidget->addTab(rideNotes, tr("Notes"));
    
    //////////////////////// Weekly Summary ////////////////////////
    
    // add daily distance / duration graph:
    weeklySummaryWindow = new WeeklySummaryWindow(useMetricUnits, this);
    tabWidget->addTab(weeklySummaryWindow, tr("Weekly Summary"));

    ////////////////////////////// Signals ////////////////////////////// 

    connect(calendar, SIGNAL(clicked(const QDate &)),
            this, SLOT(dateChanged(const QDate &)));
    connect(leftLayout, SIGNAL(splitterMoved(int,int)),
            this, SLOT(leftLayoutMoved()));
    connect(treeWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(rideSelected()));
    connect(splitter, SIGNAL(splitterMoved(int,int)), 
            this, SLOT(splitterMoved()));
    connect(cpintSetCPButton, SIGNAL(clicked()),
	    this, SLOT(cpintSetCPButtonClicked()));
    connect(tabWidget, SIGNAL(currentChanged(int)), 
            this, SLOT(tabChanged(int)));
    connect(rideNotes, SIGNAL(textChanged()),
            this, SLOT(notesChanged()));

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
    rideMenu->addAction(tr("&Export to CSV..."), this, 
                        SLOT(exportCSV()), tr("Ctrl+E")); 
    rideMenu->addAction(tr("&Export to XML..."), this, 
                        SLOT(exportXML()));
    rideMenu->addAction(tr("&Import from File..."), this,
                        SLOT (importFile()), tr ("Ctrl+I"));
    rideMenu->addAction(tr("Find &best intervals..."), this,
                        SLOT(findBestIntervals()), tr ("Ctrl+B"));
    rideMenu->addAction(tr("Split &ride..."), this,
                        SLOT(splitRide()));
    rideMenu->addAction(tr("D&elete ride..."), this,
                        SLOT(deleteRide()));
    rideMenu->addAction(tr("&Manual ride entry..."), this,
          SLOT(manualRide()), tr("Ctrl+M"));

    QMenu *optionsMenu = menuBar()->addMenu(tr("&Tools"));
    optionsMenu->addAction(tr("&Options..."), this, 
                           SLOT(showOptions()), tr("Ctrl+O")); 
    optionsMenu->addAction(tr("&Tools..."), this, 
                           SLOT(showTools()), tr("Ctrl+T")); 
    //optionsMenu->addAction(tr("&Reset Metrics..."), this, 
    //                       SLOT(importRideToDB()), tr("Ctrl+R")); 
    //optionsMenu->addAction(tr("&Update Metrics..."), this, 
    //                       SLOT(scanForMissing()()), tr("Ctrl+U")); 

 
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

    setAttribute(Qt::WA_DeleteOnClose);
}

void
MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction(); // whatever you wanna drop we will try and process!
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
                                  name, dt, &zones, notesFileName);

    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked); // default is ascending sort
    int index = 0;
    while (index < allRides->childCount()) {
        QTreeWidgetItem *item = allRides->child(index);
        if (item->type() != RIDE_TYPE)
            continue;
        RideItem *other = reinterpret_cast<RideItem*>(item);
        
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
    calendar->addRide(last);
    cpintPlot->needToScanRides = true;
    if (bSelect)
    {
        tabWidget->setCurrentIndex(0);
        treeWidget->setCurrentItem(last);
    }
}

void
MainWindow::removeCurrentRide()
{
    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item->type() != RIDE_TYPE)
        return;
    RideItem *item = reinterpret_cast<RideItem*>(_item);

    QTreeWidgetItem *itemToSelect = NULL;
    for (int x=0; x<allRides->childCount(); ++x)
    {
        if (item==allRides->child(x))
        {
            if ((x+1)<allRides->childCount())
                itemToSelect = allRides->child(x+1);
            else if (x>0)
                itemToSelect = allRides->child(x-1);
            break;
        }
    }

    QString strOldFileName = item->fileName;
    allRides->removeChild(item);
    calendar->removeRide(item);
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
    cpintPlot->deleteCpiFile(home.absolutePath() + "/" + ride_filename_to_cpi_filename(strOldFileName));

    treeWidget->setCurrentItem(itemToSelect);
    rideSelected();
    cpintPlot->replot();
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
    return ((RideItem*) treeWidget->selectedItems().first())->ride;
}

void
MainWindow::exportXML()
{
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    RideItem *ride = (RideItem*) treeWidget->selectedItems().first();

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export XML"), QDir::homePath(), tr("XML (*.xml)"));
    if (fileName.length() == 0)
        return;

    QString err;
    QFile file(fileName);
    ride->ride->writeAsXml(file, err);
    if (err.length() > 0) {
        QMessageBox::critical(this, tr("Export XML"),
                              tr("Error writing %1: %2").arg(fileName).arg(err));
        return;
    }
}

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
    items << tr("Metric") << tr("English");
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

    ride->ride->writeAsCsv(file, useMetricUnits);
}

void
MainWindow::importFile()
{
    QVariant lastDirVar = settings->value(GC_SETTINGS_LAST_IMPORT_PATH);
    QString lastDir = (lastDirVar != QVariant()) 
        ? lastDirVar.toString() : QDir::homePath();
   
    QStringList fileNames; 
    if (quarqInterpreterInstalled()) {
        fileNames = QFileDialog::getOpenFileNames(
        this, tr("Import from File"), lastDir,
        tr("All Support Formats (*.raw *.csv *.srm *.tcx *.hrm *.wko *.qla);;Raw Powertap Files (*.raw);;Comma Separated Variable (*.csv);;SRM training files (*.srm);;Garmin Training Centre (*.tcx);;Polar Precision (*.hrm);;WKO+ Files (*.wko);;Quarq ANT+ Files (*.qla);;All files (*.*)"));
    } else {
        fileNames = QFileDialog::getOpenFileNames(
        this, tr("Import from File"), lastDir,
        tr("All Support Formats (*.raw *.csv *.srm *.tcx *.hrm *.wko);;Raw Powertap Files (*.raw);;Comma Separated Variable (*.csv);;SRM training files (*.srm);;Garmin Training Centre (*.tcx);;Polar Precision (*.hrm);;WKO+ Files (*.wko);;All files (*.*)"));

    }

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
    (new BestIntervalDialog(this))->show();
}

void 
MainWindow::rideSelected()
{
    assert(treeWidget->selectedItems().size() <= 1);
    if (treeWidget->selectedItems().isEmpty()) {
	rideSummary->clear();
	return;
    }

    QTreeWidgetItem *which = treeWidget->selectedItems().first();
    if (which->type() != RIDE_TYPE) {
	rideSummary->clear();
	return;
    }

    ride = (RideItem*) which;
    calendar->setSelectedDate(ride->dateTime.date());
    rideSummary->setHtml(ride->htmlSummary());
    rideSummary->setAlignment(Qt::AlignCenter);
    if (ride) {

        allPlotWindow->setData(ride);
        histogramWindow->setData(ride);
        pfPvWindow->setData(ride);

	// turn off tabs that don't make sense for manual file entry
	if (ride->ride && ride->ride->deviceType() == QString("Manual CSV")) {
	    tabWidget->setTabEnabled(1,false); // Ride Plot
	    tabWidget->setTabEnabled(3,false); // Power Histogram
	    tabWidget->setTabEnabled(4,false); // PF/PV Plot
	}
 	else {
	    // enable
	    tabWidget->setTabEnabled(1,true); // Ride Plot
	    tabWidget->setTabEnabled(3,true); // Power Histogram
	    tabWidget->setTabEnabled(4,true); // PF/PV Plot
	}
    }
    if (tabWidget->currentIndex() == 2) {
        cpintPlot->calculate(ride);
	cpintSetCPButton->setEnabled(cpintPlot->cp > 0);
    }

    // generate a weekly summary of the week associated with the current ride
    weeklySummaryWindow->generateWeeklySummary(ride, allRides, zones);

    saveAndOpenNotes();
}


void MainWindow::getBSFactors(float &timeBS, float &distanceBS)
{

    int rides;
    double seconds, distance, bs, convertUnit;
    RideItem * lastRideItem;
    QProgressDialog * progress;
    bool aborted = false;
    seconds = rides = 0;
    distance = bs = 0;
    timeBS = distanceBS = 0.0;

    QVariant BSdays = settings->value(GC_BIKESCOREDAYS);
    if (BSdays.isNull() || BSdays.toInt() == 0)
	BSdays.setValue(30); // by default look back no more than 30 days

    // if there are rides, find most recent ride so we count back from there:
    if (allRides->childCount() > 0)
	lastRideItem =  (RideItem*) allRides->child(allRides->childCount() - 1);
    else
	lastRideItem = ride; // not enough rides, use current ride

    // set up progress bar
    progress = new QProgressDialog(QString(tr("Computing bike score estimating factors.\n")),
	tr("Abort"),0,BSdays.toInt(),this);
    int endingOffset = progress->labelText().size();
    
    for (int i = 0; i < allRides->childCount(); ++i) {
	RideItem *item = (RideItem*) allRides->child(i);
	int days =  item->dateTime.daysTo(lastRideItem->dateTime);
        if (
	    (item->type() == RIDE_TYPE) &&
	    // (item->ride) &&
	    (days  >= 0) && 
	    (days < BSdays.toInt())  
	    ) {

	    RideMetric *m;
	    item->htmlSummary(); // compute metrics

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
	// convert distance from metric:
	if (!useMetricUnits)
	{
	    const double MILES_PER_KM = 0.62137119;
	    convertUnit = MILES_PER_KM;
	}
	else {
	    convertUnit = 1.0;
	}
	distance *= convertUnit;

	timeBS = (bs * 3600) / seconds;  // BS per hour
	distanceBS = bs / distance;  // BS per mile or km
    }
done:
    if (aborted) {
	timeBS = distanceBS = 0;
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
    ConfigDialog *cd = new ConfigDialog(home, &zones);
    cd->exec();

    // update other items in case zones were changed
    if (ride) {
	// daily summary
	rideSummary->setHtml(ride->htmlSummary());
	
	// weekly summary
        weeklySummaryWindow->generateWeeklySummary(ride, allRides, zones);

	// all plot
        allPlotWindow->zonesChanged();

	// histogram
        histogramWindow->zonesChanged();

	// force-versus-pedal velocity plot
        pfPvWindow->zonesChanged();
    }
}

void 
MainWindow::moveEvent(QMoveEvent*)
{
    settings->setValue(GC_SETTINGS_MAIN_GEOM, geometry());
}

void
MainWindow::closeEvent(QCloseEvent*)
{
    saveNotes();
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

// set the rider value of CP to the value derived from the CP model extraction
void
MainWindow::cpintSetCPButtonClicked()
{
  int cp = (int) cpintPlot->cp;
  if (cp <= 0) {
    QMessageBox::critical(
			  this,
			  tr("Set CP value to extracted value"),
			  tr("No non-zero extracted value was identified:\n") +
			  tr("Zones were unchanged.")
			  );
    return;
  }
  
  if (zones == NULL)
    // set up new zones
    zones = new Zones();


  // determine in which range to write the value: use the range associated with the presently selected ride
  int range;
  if (ride)
      range = ride->zoneRange();
  else {
      QDate today = QDate::currentDate();
      range = zones->whichRange(today);
  }

  // add a new range if we failed to find a valid one
  if (range < 0) {
    // create an infinite range
    zones->addZoneRange(cp);
    range = 0;
  }

  zones->setCP(range, cp);        // update the CP value
  zones->setZonesFromCP(range);   // update the zones based on the value of CP
  zones->write(home);             // write the output file

  QDate startDate = zones->getStartDate(range);
  QDate endDate   =  zones->getEndDate(range);
  QMessageBox::information(
			   this,
			   tr("CP saved"),
			   tr("Range from %1 to %2\nRider CP set to %3 watts") .
			   arg(startDate.isNull() ? "BEGIN" : startDate.toString()) .
			   arg(endDate.isNull() ? "END" : endDate.toString()) .
			   arg(cp)
			   );

  // regenerate the ride and weekly summaries associated with the present ride
  if (ride) {
    // daily summary
    rideSummary->setHtml(ride->htmlSummary());

    // weekly summary
    weeklySummaryWindow->generateWeeklySummary(ride, allRides, zones);
  }

}

void
MainWindow::tabChanged(int index)
{
    if (index == 2) {
        if (treeWidget->selectedItems().size() == 1) {
            QTreeWidgetItem *which = treeWidget->selectedItems().first();
            if (which->type() == RIDE_TYPE) {
                RideItem *ride = (RideItem*) which;
                cpintPlot->calculate(ride);
		cpintSetCPButton->setEnabled(cpintPlot->cp > 0);
                return;
            }
        }
    }
}

static unsigned 
curve_to_point(double x, const QwtPlotCurve *curve)
{
    unsigned result = 0;
    if (curve) {
        const QwtData &data = curve->data();
        if (data.size() > 0) {
            unsigned min = 0, mid = 0, max = data.size();
            while (min < max - 1) {
                mid = (max - min) / 2 + min;
                if (x < data.x(mid)) {
                    result = (unsigned) round(data.y(mid));
                    max = mid;
                }
                else {
                    min = mid;
                }
            }
        }
    }
    return result;
}

void 
MainWindow::pickerMoved(const QPoint &pos)
{
    double minutes = cpintPlot->invTransform(QwtPlot::xBottom, pos.x());
    cpintTimeValue->setText(interval_to_str(60.0*minutes));

    // current ride
    {
      unsigned watts = curve_to_point(minutes, cpintPlot->getThisCurve());
      QString label;
      if (watts > 0)
	label = QString("%1 watts").arg(watts);
      else
	label = tr("no data");
      cpintTodayValue->setText(label);
    }

    // global ride
    {
      QString label;
      int index = (int) ceil(minutes * 60);
      if (cpintPlot->getBests().count() > index) {
	  QDate date = cpintPlot->getBestDates()[index];
	  label =
	      QString("%1 watts (%2)").
	      arg(cpintPlot->getBests()[index]).
	      arg(date.isValid() ? date.toString("MM/dd/yyyy") : "no date");
      }
      else
	  label = tr("no data");
      cpintAllValue->setText(label);
    }
}

void
MainWindow::aboutDialog()
{
    QMessageBox::about(this, tr("About GoldenCheetah"), tr(
            "<center>"
            "<h2>GoldenCheetah</h2>"
            "<i>Cycling Power Analysis Software for Linux, Mac, and Windows</i>"
            "<p><i>Build date: "
            "") + QString(__DATE__) + " " + QString(__TIME__) + "</i>"
            "<p><i>Version: " + QString(GC_VERSION) + ("</i>"
            "<p>GoldenCheetah is licensed under the "
            "<a href=\"http://www.gnu.org/copyleft/gpl.html\">GNU General "
            "Public License</a>."
            "<p>Source code can be obtained from "
            "<a href=\"http://goldencheetah.org/\">"
            "http://goldencheetah.org/</a>."
            "</center>"
            ));
}


void MainWindow::importRideToDB()
{
    MetricAggregator aggregator;
    aggregator.aggregateRides(home, zones);
}

void MainWindow::scanForMissing()
{
    MetricAggregator aggregator;
    aggregator.scanForMissing(home, zones);
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
    RideItem *item = reinterpret_cast<RideItem*>(_item);
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


