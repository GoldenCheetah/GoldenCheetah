/* 
 * $Id: MainWindow.cpp,v 1.7 2006/07/12 02:13:57 srhea Exp $
 *
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
#include "ChooseCyclistDialog.h"
#include "ConfigDialog.h"
#include "CpintPlot.h"
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

#include "DatePickerDialog.h"

#define FOLDER_TYPE 0
#define RIDE_TYPE 1
#define MILES_PER_KM 0.62137119

static char *rideFileRegExp = ("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                               "_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.(raw|srm|csv)$");

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
    QStringListIterator i(CombinedFileReader::instance().listRideFiles(home));
    while (i.hasNext()) {
        QString name = i.next();
        if (rx.exactMatch(name)) {
            assert(rx.numCaptures() == 7);
            QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt()); 
            QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt()); 
            QDateTime dt(date, time);
            last = new RideItem(allRides, RIDE_TYPE, home.path(), 
                                name, dt, zones, notesFileName(name));
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
    powerHist = new PowerHist();
    binWidthSlider->setValue(powerHist->binWidth());
    binWidthLineEdit->setText(QString("%1").arg(powerHist->binWidth()));
    vlayout->addWidget(powerHist);
    vlayout->addLayout(binWidthLayout);
    window->setLayout(vlayout);
    window->show();

    tabWidget->addTab(window, "Power Histogram");
    
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
    connect(smoothSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(returnPressed()),
            this, SLOT(setSmoothingFromLineEdit()));
    connect(binWidthSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setBinWidthFromSlider()));
    connect(binWidthLineEdit, SIGNAL(returnPressed()),
            this, SLOT(setBinWidthFromLineEdit()));
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
    QMenu *optionsMenu = menuBar()->addMenu(tr("&Options"));
    optionsMenu->addAction(tr("&Options..."), this, 
                           SLOT(showOptions()), tr("Ctrl+O")); 
 
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About GoldenCheetah"), this, SLOT(aboutDialog()));

    if (last != NULL)
        treeWidget->setCurrentItem(last);
}

void
MainWindow::addRide(QString name) 
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
    RideItem *last = new RideItem(allRides, RIDE_TYPE, home.path(), 
                                  name, dt, zones, notesFileName(name));
    cpintPlot->needToScanRides = true;
    tabWidget->setCurrentIndex(0);
    treeWidget->setCurrentItem(last);
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
    // QFileDialog::getSaveFileName checks whether the file exists.
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(
            this, tr("Open Error"),
            tr("Can't open %1 for writing").arg(fileName));
        return;
    }

    // Use the column headers that make WKO+ happy.
    double convertUnit;
    QTextStream out(&file);
    if (units=="English"){
        out << "Minutes,Torq (N-m),MPH,Watts,Miles,Cadence,Hrate,ID\n";
        convertUnit = MILES_PER_KM;
    }
    else {
        out << "Minutes,Torq (N-m),Km/h,Watts,Km,Cadence,Hrate,ID\n";
        // TODO: use KM_TO_MI from lib/pt.c instead?
        convertUnit = 1.0;
    }
        
    QListIterator<RideFilePoint*> i(ride->ride->dataPoints());
    while (i.hasNext()) {
        RideFilePoint *point = i.next();
        if (point->secs == 0.0)
            continue;
        out << point->secs/60.0;
        out << ",";
        out << ((point->nm >= 0) ? point->nm : 0.0);
        out << ",";
        out << ((point->kph >= 0) ? (point->kph * convertUnit) : 0.0);
        out << ",";
        out << ((point->watts >= 0) ? point->watts : 0.0);
        out << ",";
        out << point->km * convertUnit;
        out << ",";
        out << point->cad;
        out << ",";
        out << point->hr;
        out << ",";
        out << point->interval << "\n";
    }

    file.close();
}

void MainWindow::importCSV()
{
    // Prompt the user for the ride date
    DatePickerDialog *dpd = new DatePickerDialog(this);
    dpd->exec();

    if(dpd->canceled == true)
        return;

    QFile file ( dpd->fileName );
    QStringList errors;
    RideFile *ride =
        CombinedFileReader::instance().openRideFile(file, errors);

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

    delete ride;
    delete dpd;
    addRide ( name );
}

void
MainWindow::importSRM()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this, tr("Import SRM"), QDir::homePath(),
        tr("SRM Binary Format (*.srm)"));
    QStringListIterator i(fileNames);
    while (i.hasNext()) {
        QString fileName = i.next();
        QFile file(fileName);
        QStringList errors;
        RideFile *ride =
            CombinedFileReader::instance().openRideFile(file, errors);
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

        delete ride;
        addRide(name);
    }
}

void 
MainWindow::rideSelected()
{
    assert(treeWidget->selectedItems().size() <= 1);
    if (treeWidget->selectedItems().size() == 1) {
        QTreeWidgetItem *which = treeWidget->selectedItems().first();
        if (which->type() == RIDE_TYPE) {
            RideItem *ride = (RideItem*) which;
            rideSummary->setHtml(ride->htmlSummary());
            rideSummary->setAlignment(Qt::AlignCenter);
            if (ride->ride)
                allPlot->setData(ride->ride);
            if (tabWidget->currentIndex() == 2)
                cpintPlot->calculate(ride->fileName, ride->dateTime);
            if (ride->ride)
                powerHist->setData(ride->ride);

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

            int minutes = ((int) round(weeklySeconds->value(true))) / 60;
            int hours = (int) minutes / 60;
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
                    "    <td align=\"right\">%3:%4</td></tr>"
                    "<tr><td>Total distance (kilometers):</td>"
                    "    <td align=\"right\">%5</td></tr>"
                    "<tr><td>Total work (kJ):</td>"
                    "    <td align=\"right\">%6</td></tr>"
                    "<tr><td>Daily Average work (kJ):</td>"
                    "    <td align=\"right\">%7</td></tr>"
                    "</table>"
                 )
                .arg(wstart.toString(dateFormat))
                .arg(wstart.addDays(6).toString(dateFormat))
                .arg(hours)
                .arg(minutes, 2, 10, QLatin1Char('0'))
                .arg((unsigned) round(weeklyDistance->value(true)))
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
                    "    <td align=\"right\">%3:%4</td></tr>"
                    "<tr><td>Total distance (miles):</td>"
                    "    <td align=\"right\">%5</td></tr>"
                    "<tr><td>Total work (kJ):</td>"
                    "    <td align=\"right\">%6</td></tr>"
                    "<tr><td>Daily Average work (kJ):</td>"
                    "    <td align=\"right\">%7</td></tr>"
                    "</table>"
                    // TODO: add averages
                    )
                    .arg(wstart.toString(dateFormat))
                    .arg(wstart.addDays(6).toString(dateFormat))
                    .arg(hours)
                    .arg(minutes, 2, 10, QLatin1Char('0'))
                    .arg((unsigned) round(weeklyDistance->value(false)))
                    .arg((unsigned) round(weeklyWork->value(true)))
                    .arg((unsigned) round(weeklyWork->value(true) / 7));
            }    
            if (zone_range != -1) {
                summary += "<h2>Power Zones</h2>";
                if (!zones_ok)
                    summary += "Error: Week spans more than one zone range.";
                else {
                    summary += 
                        zones->summarize(zone_range, time_in_zone, num_zones);
                }
            }

            summary += "</center>";

            delete weeklyDistance;
            delete weeklySeconds;

            // TODO: add daily breakdown

            weeklySummary->setHtml(summary);
            
            // First save the contents of the notes window.
            saveNotes();

            // Now open any notes associated with the new ride.
            rideNotes->setPlainText("");
            QString notesPath = 
                home.absolutePath() + "/" + ride->notesFileName;  
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
            return;
        }
    }
    rideSummary->clear();
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
MainWindow::setBinWidthFromLineEdit()
{
    int value = binWidthLineEdit->text().toInt();
    if (value != powerHist->binWidth()) {
        powerHist->setBinWidth(value);
        binWidthSlider->setValue(value);
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
            curve_to_point(minutes, cpintPlot->getAllCurve())));
}

void
MainWindow::aboutDialog()
{
    QMessageBox::about(this, tr("About GoldenCheetah"), tr(
            "<center>"
            "<h2>GoldenCheetah</h2>"
            "<i>Cycling Power Analysis Software for Linux and Mac OS X</i>"
            "<p><i>Build date: "
            "") + QString(GC_BUILD_DATE).replace("_", " ") + ("</i>"
            "</center>"
            "<p>GoldenCheetah is licensed under the "
            "<a href=\"http://www.gnu.org/copyleft/gpl.html\">GNU General "
            "Public License</a>.  Source code can be obtained from "
            "<a href=\"http://goldencheetah.org/\">"
            "http://goldencheetah.org/</a>."
            ));
}

void
MainWindow::notesChanged()
{
    currentNotesChanged = true;
}

