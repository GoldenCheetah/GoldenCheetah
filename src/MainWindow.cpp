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
#include "AllPlot.h"
#include "BestIntervalDialog.h"
#include "ChooseCyclistDialog.h"
#include "ConfigDialog.h"
#include "CpintPlot.h"
#include "PfPvPlot.h"
#include "DownloadRideDialog.h"
#include "PowerHist.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Zones.h"
#include <assert.h>
#include <QApplication>
#include <QtGui>
#include <QRegExp>
#include <qwt_plot_curve.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_data.h>
#include <boost/scoped_ptr.hpp>

#include "DatePickerDialog.h"
#include "ToolsDialog.h"
#include "MetricAggregator.h"
#include "SplitRideDialog.h"

/* temp for the qmake/QMAKE_CXXFLAGS bug with xcode */
#ifndef GC_SVN_VERSION
#define GC_SVN_VERSION "0"
#endif
#ifndef GC_BUILD_DATE
#define GC_BUILD_DATE GC_SVN_VERSION
#endif

#define FOLDER_TYPE 0
#define RIDE_TYPE 1

static char *rideFileRegExp = ("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                               "_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.(raw|srm|csv|tcx)$");

QString
MainWindow::notesFileName(QString rideFileName) {
    int i = rideFileName.lastIndexOf(".");
    assert(i >= 0);
    return rideFileName.left(i) + ".notes";
}

MainWindow::MainWindow(const QDir &home) : 
    home(home), settings(GC_SETTINGS_CO, GC_SETTINGS_APP), 
    zones(NULL), currentNotesChanged(false)
{
    setWindowTitle(home.dirName());
    settings.setValue(GC_SETTINGS_LAST, home.dirName());

    setWindowIcon(QIcon(":images/gc.png"));

    QFile zonesFile(home.absolutePath() + "/power.zones");
    if (zonesFile.exists()) {
        zones = new Zones();
        if (!zones->read(zonesFile)) {
            QMessageBox::warning(this, tr("Zones File Error"),
                                 zones->errorString());
            delete zones;
            zones = NULL;
        }
    }

    QVariant geom = settings.value(GC_SETTINGS_MAIN_GEOM);
    if (geom == QVariant())
        resize(640, 480);
    else
        setGeometry(geom.toRect());

    splitter = new QSplitter(this);
    setCentralWidget(splitter);
    splitter->setContentsMargins(10, 20, 10, 10); // attempting to follow some UI guides

    treeWidget = new QTreeWidget;
    treeWidget->setColumnCount(3);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    // TODO: Test this on various systems with differing font settings (looks good on Leopard :)
    treeWidget->header()->resizeSection(0,70);
    treeWidget->header()->resizeSection(1,95);
    treeWidget->header()->resizeSection(2,70);
    treeWidget->setMaximumWidth(250);
    treeWidget->header()->hide();
    treeWidget->setAlternatingRowColors (true);
    treeWidget->setIndentation(5);

    allRides = new QTreeWidgetItem(treeWidget, FOLDER_TYPE);
    allRides->setText(0, tr("All Rides"));
    treeWidget->expandItem(allRides);
    splitter->addWidget(treeWidget);

    QRegExp rx(rideFileRegExp);
    QTreeWidgetItem *last = NULL;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(home));
    while (i.hasNext()) {
        QString name = i.next();
        if (rx.exactMatch(name)) {
            assert(rx.numCaptures() == 7);
            QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt()); 
            QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt()); 
            QDateTime dt(date, time);
            last = new RideItem(RIDE_TYPE, home.path(), 
                                name, dt, zones, notesFileName(name));
            allRides->addChild(last);
        }
    }

    tabWidget = new QTabWidget;
    rideSummary = new QTextEdit;
    rideSummary->setReadOnly(true);
    tabWidget->addTab(rideSummary, tr("Ride Summary"));

    /////////////////////////// Ride Plot Tab ///////////////////////////

    QWidget *window = new QWidget;
    QVBoxLayout *vlayout = new QVBoxLayout;

    QHBoxLayout *showLayout = new QHBoxLayout;
    QLabel *showLabel = new QLabel("Show:", window);
    showLayout->addWidget(showLabel);

    QCheckBox *showGrid = new QCheckBox("Grid", window);
    showGrid->setCheckState(Qt::Checked);
    showLayout->addWidget(showGrid);

    QCheckBox *showPower = new QCheckBox("Power", window);
    showPower->setCheckState(Qt::Checked);
    showLayout->addWidget(showPower);

    QCheckBox *showHr = new QCheckBox("Heart Rate", window);
    showHr->setCheckState(Qt::Checked);
    showLayout->addWidget(showHr);

    QCheckBox *showSpeed = new QCheckBox("Speed", window);
    showSpeed->setCheckState(Qt::Checked);
    showLayout->addWidget(showSpeed);

    QCheckBox *showCad = new QCheckBox("Cadence", window);
    showCad->setCheckState(Qt::Checked);
    showLayout->addWidget(showCad);

    QComboBox *comboDistance = new QComboBox();
    comboDistance->addItem(tr("X Axis Shows Time"));
    comboDistance->addItem(tr("X Axis Shows Distance"));
    showLayout->addWidget(comboDistance);

    QHBoxLayout *smoothLayout = new QHBoxLayout;
    QLabel *smoothLabel = new QLabel(tr("Smoothing (secs)"), window);
    smoothLineEdit = new QLineEdit(window);
    smoothLineEdit->setFixedWidth(40);
    
    smoothLayout->addWidget(smoothLabel);
    smoothLayout->addWidget(smoothLineEdit);
    smoothSlider = new QSlider(Qt::Horizontal);
    smoothSlider->setTickPosition(QSlider::TicksBelow);
    smoothSlider->setTickInterval(10);
    smoothSlider->setMinimum(2);
    smoothSlider->setMaximum(600);
    smoothLineEdit->setValidator(new QIntValidator(smoothSlider->minimum(),
                                                   smoothSlider->maximum(), 
                                                   smoothLineEdit));
    smoothLayout->addWidget(smoothSlider);
    allPlot = new AllPlot;
    smoothSlider->setValue(allPlot->smoothing());
    smoothLineEdit->setText(QString("%1").arg(allPlot->smoothing()));

    allZoomer = new QwtPlotZoomer(allPlot->canvas());
    allZoomer->setRubberBand(QwtPicker::RectRubberBand);
    allZoomer->setRubberBandPen(QColor(Qt::black));
    allZoomer->setSelectionFlags(QwtPicker::DragSelection 
                                 | QwtPicker::CornerToCorner);
    allZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    allZoomer->setEnabled(true);
    
    // TODO: Hack for OS X one-button mouse
    // allZoomer->initMousePattern(1);
    
    // RightButton: zoom out by 1
    // Ctrl+RightButton: zoom out to full size                          
    allZoomer->setMousePattern(QwtEventPattern::MouseSelect2,
                               Qt::RightButton, Qt::ControlModifier);
    allZoomer->setMousePattern(QwtEventPattern::MouseSelect3,
                               Qt::RightButton);

    allPanner = new QwtPlotPanner(allPlot->canvas());
    allPanner->setMouseButton(Qt::MidButton);

    // TODO: zoomer doesn't interact well with automatic axis resizing

    vlayout->addWidget(allPlot);
    vlayout->addLayout(showLayout);
    vlayout->addLayout(smoothLayout);
    window->setLayout(vlayout);
    window->show();

    tabWidget->addTab(window, "Ride Plot");
    splitter->addWidget(tabWidget);

    QVariant splitterSizes = settings.value(GC_SETTINGS_SPLITTER_SIZES); 
    if (splitterSizes != QVariant())
        splitter->restoreState(splitterSizes.toByteArray());
    else {
        QList<int> sizes;
        sizes.append(250);
        sizes.append(390);
        splitter->setSizes(sizes);
    }

    ////////////////////// Critical Power Plot Tab //////////////////////

    window = new QWidget;
    vlayout = new QVBoxLayout;
    QHBoxLayout *cpintPickerLayout = new QHBoxLayout;
    QLabel *cpintTimeLabel = new QLabel(tr("Interval Duration:"), window);
    cpintTimeValue = new QLineEdit("0 s");
    QLabel *cpintTodayLabel = new QLabel(tr("Today:"), window);
    cpintTodayValue = new QLineEdit("0 watts");
    QLabel *cpintAllLabel = new QLabel(tr("All Rides:"), window);
    cpintAllValue = new QLineEdit("0 watts");
    cpintTimeValue->setReadOnly(true);
    cpintTodayValue->setReadOnly(true);
    cpintAllValue->setReadOnly(true);
    cpintPickerLayout->addWidget(cpintTimeLabel);
    cpintPickerLayout->addWidget(cpintTimeValue);
    cpintPickerLayout->addWidget(cpintTodayLabel);
    cpintPickerLayout->addWidget(cpintTodayValue);
    cpintPickerLayout->addWidget(cpintAllLabel);
    cpintPickerLayout->addWidget(cpintAllValue);
    cpintPlot = new CpintPlot(home.path());
    vlayout->addWidget(cpintPlot);
    vlayout->addLayout(cpintPickerLayout);
    window->setLayout(vlayout);
    window->show();
    tabWidget->addTab(window, "Critical Power Plot");

    picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::PointSelection, 
                               QwtPicker::VLineRubberBand, 
                               QwtPicker::AlwaysOff, cpintPlot->canvas());
    picker->setRubberBandPen(QColor(Qt::blue));

    connect(picker, SIGNAL(moved(const QPoint &)),
            SLOT(pickerMoved(const QPoint &)));

    //////////////////////// Power Histogram Tab ////////////////////////

    window = new QWidget;
    vlayout = new QVBoxLayout;
    QHBoxLayout *binWidthLayout = new QHBoxLayout;
    QLabel *binWidthLabel = new QLabel(tr("Bin width (watts)"), window);
    binWidthLineEdit = new QLineEdit(window);
    binWidthLineEdit->setFixedWidth(30);
    
    binWidthLayout->addWidget(binWidthLabel);
    binWidthLayout->addWidget(binWidthLineEdit);
    binWidthSlider = new QSlider(Qt::Horizontal);
    binWidthSlider->setTickPosition(QSlider::TicksBelow);
    binWidthSlider->setTickInterval(1);
    binWidthSlider->setMinimum(1);
    binWidthSlider->setMaximum(100);
    binWidthLineEdit->setValidator(new QIntValidator(binWidthSlider->minimum(),
                                                     binWidthSlider->maximum(), 
                                                     binWidthLineEdit));
    binWidthLayout->addWidget(binWidthSlider);

    withZerosCheckBox = new QCheckBox;
    withZerosCheckBox->setText("With zeros");
    binWidthLayout->addWidget(withZerosCheckBox);

    powerHist = new PowerHist();
    withZerosCheckBox->setChecked(powerHist->withZeros());
    binWidthSlider->setValue(powerHist->binWidth());
    binWidthLineEdit->setText(QString("%1").arg(powerHist->binWidth()));
    vlayout->addWidget(powerHist);
    vlayout->addLayout(binWidthLayout);
    window->setLayout(vlayout);
    window->show();

    tabWidget->addTab(window, "Power Histogram");
    
    //////////////////////// Pedal Force/Velocity Plot ////////////////////////

    window = new QWidget;
    vlayout = new QVBoxLayout;
    QHBoxLayout *qaLayout = new QHBoxLayout;

    pfPvPlot = new PfPvPlot();
    QLabel *qaCPLabel = new QLabel(tr("CP:"), window);
    qaCPValue = new QLineEdit(QString("%1").arg(pfPvPlot->getCP()));
    QLabel *qaCadLabel = new QLabel(tr("Cadence:"), window);
    qaCadValue = new QLineEdit(QString("%1").arg(pfPvPlot->getCAD()));
    QLabel *qaClLabel = new QLabel(tr("Crank Length:"), window);
    qaClValue = new QLineEdit(QString("%1").arg(pfPvPlot->getCL()));
    qaLayout->addWidget(qaCPLabel);
    qaLayout->addWidget(qaCPValue);
    qaLayout->addWidget(qaCadLabel);
    qaLayout->addWidget(qaCadValue);
    qaLayout->addWidget(qaClLabel);
    qaLayout->addWidget(qaClValue);

    vlayout->addWidget(pfPvPlot);
    vlayout->addLayout(qaLayout);
    window->setLayout(vlayout);
    window->show();

    tabWidget->addTab(window, tr("PF/PV Plot"));

    //////////////////////// Ride Notes ////////////////////////
    
    rideNotes = new QTextEdit;
    tabWidget->addTab(rideNotes, tr("Notes"));
    
    //////////////////////// Weekly Summary ////////////////////////
    
    weeklySummary = new QTextEdit;
    weeklySummary->setReadOnly(true);
    tabWidget->addTab(weeklySummary, tr("Weekly Summary"));

    ////////////////////////////// Signals ////////////////////////////// 

    connect(treeWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(rideSelected()));
    connect(splitter, SIGNAL(splitterMoved(int,int)), 
            this, SLOT(splitterMoved()));
    connect(showPower, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showPower(int)));
    connect(showHr, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showHr(int)));
    connect(showSpeed, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showSpeed(int)));
    connect(showCad, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showCad(int)));
    connect(showGrid, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showGrid(int)));
    connect(comboDistance, SIGNAL(activated(int)),
            allPlot, SLOT(setByDistance(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(editingFinished()),
            this, SLOT(setSmoothingFromLineEdit()));
    connect(binWidthSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setBinWidthFromSlider()));
    connect(binWidthLineEdit, SIGNAL(editingFinished()),
            this, SLOT(setBinWidthFromLineEdit()));
    connect(withZerosCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(setWithZerosFromCheckBox()));
    connect(qaCPValue, SIGNAL(editingFinished()),
	    this, SLOT(setQaCPFromLineEdit()));
    connect(qaCadValue, SIGNAL(editingFinished()),
	    this, SLOT(setQaCADFromLineEdit()));
    connect(qaClValue, SIGNAL(editingFinished()),
	    this, SLOT(setQaCLFromLineEdit()));
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
    rideMenu->addAction(tr("&Import from SRM..."), this, 
                        SLOT(importSRM()), tr("Ctrl+I")); 
    rideMenu->addAction(tr("&Import from CSV..."), this,
                        SLOT (importCSV()), tr ("Ctrl+S"));
    rideMenu->addAction(tr("&Import from TCX..."), this,
                        SLOT (importTCX()));
    rideMenu->addAction(tr("Find &best intervals..."), this,
                        SLOT(findBestIntervals()), tr ("Ctrl+B"));
    rideMenu->addAction(tr("Split &ride..."), this,
                        SLOT(splitRide()));
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

    // This will read the user preferences and change the file list order as necessary:
    QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
    QVariant isAscending = settings.value(GC_ALLRIDES_ASCENDING,Qt::Checked);
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
}

void
MainWindow::addRide(QString name, bool bSelect /*=true*/)
{
    QRegExp rx(rideFileRegExp);
    if (!rx.exactMatch(name)) {
        fprintf(stderr, "bad name: %s\n", name.toAscii().constData());
        assert(false);
    }
    assert(rx.numCaptures() == 7);
    QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt()); 
    QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt()); 
    QDateTime dt(date, time);
    RideItem *last = new RideItem(RIDE_TYPE, home.path(), 
                                  name, dt, zones, notesFileName(name));
    
    QVariant isAscending = settings.value(GC_ALLRIDES_ASCENDING,Qt::Checked); // default is ascending sort
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
    delete item;

    QFile file(home.absolutePath() + "/" + strOldFileName);
    // purposefully don't remove the old ext so the user wouldn't have to figure out what the old file type was
    QString strNewName = strOldFileName + ".bak";
    if (!file.rename(home.absolutePath() + "/" + strNewName))
    {
        QMessageBox::critical(
            this, "Rename Error",
            tr("Can't rename %1 to %2")
            .arg(strOldFileName).arg(strNewName));
    }

    tabWidget->setCurrentIndex(0);
    treeWidget->setCurrentItem(itemToSelect);
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
MainWindow::exportCSV()
{
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        QMessageBox::critical(this, tr("Select Ride"), tr("No ride selected!"));
        return;
    }

    RideItem *ride = (RideItem*) treeWidget->selectedItems().first();

    // Ask the user if they prefer to export with English or metric units.
    QStringList items;
    items << tr("Metric") << tr("English");
    bool ok;
    QString units = QInputDialog::getItem(
        this, tr("Select Units"), tr("Units:"), items, 0, false, &ok);
    if(!ok) 
        return;

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

    ride->ride->writeAsCsv(file, units!="English");
}

void MainWindow::importCSV()
{
    // Prompt the user for the ride date
    boost::scoped_ptr<DatePickerDialog> dpd(new DatePickerDialog(this));

    dpd->exec();

    if(dpd->canceled == true)
        return;

    QFile file ( dpd->fileName );
    QStringList errors;
    boost::scoped_ptr<RideFile> ride(
        RideFileFactory::instance().openRideFile(file, errors));

    if (!ride || !errors.empty())
    {
        QString all = 
            ( ride
              ? tr ( "Non-fatal problem(s) opening %1:" )
              : tr ( "Fatal problem(s) opening %1:" ) ).arg ( dpd->fileName );
        QStringListIterator i ( errors );
        while ( i.hasNext() )
            all += "\n" + i.next();
        if (ride)
            QMessageBox::warning ( this, tr ( "Open Warning" ), all );
        else {
            QMessageBox::critical ( this, tr ( "Open Error" ), all );
            return;
        }
    }
    ride->setStartTime(dpd->date); 

    QChar zero = QLatin1Char ( '0' );

    QString name = QString ( "%1_%2_%3_%4_%5_%6.csv" )
        .arg ( ride->startTime().date().year(), 4, 10, zero )
        .arg ( ride->startTime().date().month(), 2, 10, zero )
        .arg ( ride->startTime().date().day(), 2, 10, zero )
        .arg ( ride->startTime().time().hour(), 2, 10, zero )
        .arg ( ride->startTime().time().minute(), 2, 10, zero )
        .arg ( ride->startTime().time().second(), 2, 10, zero );

    if ( !file.copy ( home.absolutePath() + "/" + name ) )
    {
        QMessageBox::critical ( this, tr ( "Copy Error" ),
                                tr ( "Couldn't copy %1" )
                                .arg ( dpd->fileName ) );
        return;
    }

    addRide ( name );
}

void
MainWindow::importSRM()
{
    QVariant lastDirVar = settings.value(GC_SETTINGS_LAST_IMPORT_PATH);
    QString lastDir = (lastDirVar != QVariant()) 
        ? lastDirVar.toString() : QDir::homePath();
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this, tr("Import SRM"), lastDir,
        tr("SRM Binary Format (*.srm)"));
    if (!fileNames.isEmpty()) {
        lastDir = QFileInfo(fileNames.front()).absolutePath();
        settings.setValue(GC_SETTINGS_LAST_IMPORT_PATH, lastDir);
    }
    QStringList fileNamesCopy = fileNames; // QT doc says iterate over a copy
    QStringListIterator i(fileNamesCopy);
    while (i.hasNext()) {
        QString fileName = i.next();
        QFile file(fileName);
        QStringList errors;

        boost::scoped_ptr<RideFile> ride(
            RideFileFactory::instance().openRideFile(file, errors));

        if (!ride || !errors.empty()) {
            QString all = (ride 
                           ? tr("Non-fatal problem(s) opening %1:")
                           : tr("Fatal problem(s) opening %1:")).arg(fileName);
            QStringListIterator i(errors);
            while (i.hasNext())
                all += "\n" + i.next();
            if (ride)
                QMessageBox::warning(this, tr("Open Warning"), all);
            else {
                QMessageBox::critical(this, tr("Open Error"), all);
                return;
            }
        }

        QChar zero = QLatin1Char('0'); 
        QString name = QString("%1_%2_%3_%4_%5_%6.srm")
            .arg(ride->startTime().date().year(), 4, 10, zero)
            .arg(ride->startTime().date().month(), 2, 10, zero)
            .arg(ride->startTime().date().day(), 2, 10, zero)
            .arg(ride->startTime().time().hour(), 2, 10, zero)
            .arg(ride->startTime().time().minute(), 2, 10, zero)
            .arg(ride->startTime().time().second(), 2, 10, zero);

        if (!file.copy(home.absolutePath() + "/" + name)) {
            QMessageBox::critical(this, tr("Copy Error"), 
                                  tr("Couldn't copy %1").arg(fileName));
            return;
        }

        addRide(name);
    }
}

void
MainWindow::importTCX()
{
    QVariant lastDirVar = settings.value(GC_SETTINGS_LAST_IMPORT_PATH);
    QString lastDir = (lastDirVar != QVariant()) 
        ? lastDirVar.toString() : QDir::homePath();
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this, tr("Import TCX"), lastDir,
        tr("TCX Format (*.tcx)"));
    if (!fileNames.isEmpty()) {
        lastDir = QFileInfo(fileNames.front()).absolutePath();
        settings.setValue(GC_SETTINGS_LAST_IMPORT_PATH, lastDir);
    }
    QStringList fileNamesCopy = fileNames; // QT doc says iterate over a copy
    QStringListIterator i(fileNames);
    while (i.hasNext()) {
        QString fileName = i.next();
        QFile file(fileName);
        QStringList errors;

        boost::scoped_ptr<RideFile> ride(
            RideFileFactory::instance().openRideFile(file, errors));

        if (!ride || !errors.empty()) {
            QString all = (ride 
                           ? tr("Non-fatal problem(s) opening %1:")
                           : tr("Fatal problem(s) opening %1:")).arg(fileName);
            QStringListIterator i(errors);
            while (i.hasNext())
                all += "\n" + i.next();
            if (ride)
                QMessageBox::warning(this, tr("Open Warning"), all);
            else {
                QMessageBox::critical(this, tr("Open Error"), all);
                return;
            }
        }

        QChar zero = QLatin1Char('0'); 
        QString name = QString("%1_%2_%3_%4_%5_%6.tcx")
            .arg(ride->startTime().date().year(), 4, 10, zero)
            .arg(ride->startTime().date().month(), 2, 10, zero)
            .arg(ride->startTime().date().day(), 2, 10, zero)
            .arg(ride->startTime().time().hour(), 2, 10, zero)
            .arg(ride->startTime().time().minute(), 2, 10, zero)
            .arg(ride->startTime().time().second(), 2, 10, zero);

        if (!file.copy(home.absolutePath() + "/" + name)) {
            QMessageBox::critical(this, tr("Copy Error"), 
                                  tr("Couldn't copy %1").arg(fileName));
            return;
        }

        addRide(name);
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
    if (treeWidget->selectedItems().size() == 0) {
        rideSummary->clear();
        return;
    }

    QTreeWidgetItem *which = treeWidget->selectedItems().first();
    if (which->type() != RIDE_TYPE) {
        rideSummary->clear();
        return;
    }

    RideItem *ride = (RideItem*) which;
    rideSummary->setHtml(ride->htmlSummary());
    rideSummary->setAlignment(Qt::AlignCenter);
    if (ride->ride)
        allPlot->setData(ride->ride);
    if (tabWidget->currentIndex() == 2)
        cpintPlot->calculate(ride->fileName, ride->dateTime);
    if (ride->ride)
        powerHist->setData(ride->ride);
    if (ride){
        // using a RideItem rather than RideFile to provide access to zones information
        pfPvPlot->setData(ride);
        // update the QLabel widget with the CP value set in PfPvPlot::setData()
        qaCPValue->setText(QString("%1").arg(pfPvPlot->getCP()));
    }

    QDate wstart = ride->dateTime.date();
    wstart = wstart.addDays(Qt::Monday - wstart.dayOfWeek());
    assert(wstart.dayOfWeek() == Qt::Monday);
    QDate wend = wstart.addDays(7);
    const RideMetricFactory &factory = RideMetricFactory::instance();
    RideMetric *weeklySeconds = factory.newMetric("time_riding");
    assert(weeklySeconds);
    RideMetric *weeklyDistance = factory.newMetric("total_distance");
    assert(weeklyDistance);
    RideMetric *weeklyWork = factory.newMetric("total_work");
    assert(weeklyWork);
    
    int zone_range = -1;
    double *time_in_zone = NULL;
    int num_zones = -1;
    bool zones_ok = true;

    for (int i = 0; i < allRides->childCount(); ++i) {
        if (allRides->child(i)->type() == RIDE_TYPE) {
            RideItem *item = (RideItem*) allRides->child(i);
            if ((item->dateTime.date() >= wstart)
                && (item->dateTime.date() < wend)) {
                RideMetric *m;
                item->htmlSummary(); // compute metrics
                m = item->metrics.value(weeklySeconds->name());
                assert(m);
                weeklySeconds->aggregateWith(m);
                m = item->metrics.value(weeklyDistance->name());
                assert(m);
                weeklyDistance->aggregateWith(m);
                m = item->metrics.value(weeklyWork->name());
                assert(m);
                weeklyWork->aggregateWith(m);
                if (zones) {
                    if (zone_range == -1) {
                        zone_range = item->zoneRange();
                        num_zones = item->numZones();
                        time_in_zone = new double[num_zones];
                    }
                    else if (item->zoneRange() != zone_range) {
                        zones_ok = false;
                    }
                    if (zone_range != -1) {
                        for (int j = 0; j < num_zones; ++j)
                            time_in_zone[j] += item->timeInZone(j);
                    }
                }
            }
        }
    }

    int seconds = ((int) round(weeklySeconds->value(true)));
    int minutes = seconds / 60;
    seconds %= 60;
    int hours = minutes / 60;
    minutes %= 60;

    const char *dateFormat = "MM/dd/yyyy";

    QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
    QVariant unit = settings.value(GC_UNIT);

    QString summary;
    if (unit.toString() == "Metric") {
        summary = tr(
            "<center>"
            "<h2>Week of %1 through %2</h2>"
            "<h2>Summary</h2>"
            "<p>"
            "<table align=\"center\" width=\"60%\" border=0>"
            "<tr><td>Total time riding:</td>"
            "    <td align=\"right\">%3:%4:%5</td></tr>"
            "<tr><td>Total distance (km):</td>"
            "    <td align=\"right\">%6</td></tr>"
            "<tr><td>Total work (kJ):</td>"
            "    <td align=\"right\">%7</td></tr>"
            "<tr><td>Daily Average work (kJ):</td>"
            "    <td align=\"right\">%8</td></tr>"
            "</table>"
            )
            .arg(wstart.toString(dateFormat))
            .arg(wstart.addDays(6).toString(dateFormat))
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(weeklyDistance->value(true), 0, 'f', 1)
            .arg((unsigned) round(weeklyWork->value(true)))
            .arg((unsigned) round(weeklyWork->value(true) / 7));
    } 
    else {
        summary = tr(
            "<center>"
            "<h2>Week of %1 through %2</h2>"
            "<h2>Summary</h2>"
            "<p>"
            "<table align=\"center\" width=\"60%\" border=0>"
            "<tr><td>Total time riding:</td>"
            "    <td align=\"right\">%3:%4:%5</td></tr>"
            "<tr><td>Total distance (miles):</td>"
            "    <td align=\"right\">%6</td></tr>"
            "<tr><td>Total work (kJ):</td>"
            "    <td align=\"right\">%7</td></tr>"
            "<tr><td>Daily Average work (kJ):</td>"
            "    <td align=\"right\">%8</td></tr>"
            "</table>"
            // TODO: add averages
            )
            .arg(wstart.toString(dateFormat))
            .arg(wstart.addDays(6).toString(dateFormat))
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(weeklyDistance->value(false), 0, 'f', 1)
            .arg((unsigned) round(weeklyWork->value(true)))
            .arg((unsigned) round(weeklyWork->value(true) / 7));
    }    
    if (zone_range != -1) {
        summary += "<h2>Power Zones</h2>";
        if (!zones_ok)
            summary += "Error: Week spans more than one zone range.";
        else {
            summary += zones->summarize(zone_range, time_in_zone, num_zones);
        }
    }

    if (time_in_zone)
        delete [] time_in_zone;

    summary += "</center>";

    delete weeklyDistance;
    delete weeklySeconds;
    delete weeklyWork;

    // TODO: add daily breakdown

    weeklySummary->setHtml(summary);

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
    settings.setValue(GC_SETTINGS_MAIN_GEOM, geometry());
}

void 
MainWindow::showOptions()
{
   ConfigDialog *cd = new ConfigDialog(home);
   cd->exec();
}


void 
MainWindow::moveEvent(QMoveEvent*)
{
    settings.setValue(GC_SETTINGS_MAIN_GEOM, geometry());
}

void
MainWindow::closeEvent(QCloseEvent*)
{
    saveNotes();
}

void 
MainWindow::splitterMoved()
{
    settings.setValue(GC_SETTINGS_SPLITTER_SIZES, splitter->saveState());
}

void
MainWindow::setSmoothingFromSlider()
{
    if (allPlot->smoothing() != smoothSlider->value()) {
        allPlot->setSmoothing(smoothSlider->value());
        smoothLineEdit->setText(QString("%1").arg(allPlot->smoothing()));
    }
}

void
MainWindow::setSmoothingFromLineEdit()
{
    int value = smoothLineEdit->text().toInt();
    if (value != allPlot->smoothing()) {
        allPlot->setSmoothing(value);
        smoothSlider->setValue(value);
    }
}

void
MainWindow::setBinWidthFromSlider()
{
    if (powerHist->binWidth() != binWidthSlider->value()) {
        powerHist->setBinWidth(binWidthSlider->value());
        binWidthLineEdit->setText(QString("%1").arg(powerHist->binWidth()));
    }
}

void
MainWindow::setWithZerosFromCheckBox()
{
    if (powerHist->withZeros() != withZerosCheckBox->isChecked()) {
        powerHist->setWithZeros(withZerosCheckBox->isChecked());
    }
}

void
MainWindow::setBinWidthFromLineEdit()
{
    int value = binWidthLineEdit->text().toInt();
    if (value != powerHist->binWidth()) {
        powerHist->setBinWidth(value);
        binWidthSlider->setValue(value);
    }
}

void
MainWindow::setQaCPFromLineEdit()
{
    int value = qaCPValue->text().toInt();
    pfPvPlot->setCP(value);
}

void
MainWindow::setQaCADFromLineEdit()
{
    int value = qaCadValue->text().toInt();
    pfPvPlot->setCAD(value);
}

void
MainWindow::setQaCLFromLineEdit()
{
    double value = qaClValue->text().toDouble();
    pfPvPlot->setCL(value);
}

void
MainWindow::tabChanged(int index)
{
    if (index == 2) {
        if (treeWidget->selectedItems().size() == 1) {
            QTreeWidgetItem *which = treeWidget->selectedItems().first();
            if (which->type() == RIDE_TYPE) {
                RideItem *ride = (RideItem*) which;
                cpintPlot->calculate(ride->fileName, ride->dateTime);
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
    cpintTodayValue->setText(tr("%1 watts").arg(
            curve_to_point(minutes, cpintPlot->getThisCurve())));
    cpintAllValue->setText(tr("%1 watts").arg(
            curve_to_point(minutes, cpintPlot->getAllCurve()))+(cpintPlot->getBestDates().count()>minutes*60?" ("+cpintPlot->getBestDates()[(int) ceil(minutes*60)].toString("MM/dd/yyyy")+")":""));
}

void
MainWindow::aboutDialog()
{
    QMessageBox::about(this, tr("About GoldenCheetah"), tr(
            "<center>"
            "<h2>GoldenCheetah</h2>"
            "<i>Cycling Power Analysis Software for Linux, Mac, and Windows</i>"
            "<p><i>Build date: "
            "") + QString(GC_BUILD_DATE).replace("_", " ") + ("</i>"
            "<p><i>Version: "
            "")+QString::number(GC_MAJOR_VER)+(".") +QString::number(GC_MINOR_VER)+(".")+QString(GC_SVN_VERSION) + ("</i>"
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
	MetricAggregator *aggregator = new MetricAggregator();
	aggregator->aggregateRides(home, zones);
    delete aggregator;
}

void MainWindow::scanForMissing()
{
    MetricAggregator *aggregator = new MetricAggregator();
    aggregator->scanForMissing(home, zones);
    delete aggregator;
}



void
MainWindow::notesChanged()
{
    currentNotesChanged = true;
}

void MainWindow::showTools()
{
   ToolsDialog *td = new ToolsDialog();
   td->exec();

}
void
MainWindow::splitRide()
{
    (new SplitRideDialog(this))->exec();
}

