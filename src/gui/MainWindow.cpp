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
#include "CpintPlot.h"
#include "DownloadRideDialog.h"
#include "PowerHist.h"
#include "RawFile.h"
#include "RideItem.h"
#include "Settings.h"
#include <assert.h>
#include <QApplication>
#include <QtGui>
#include <QRegExp>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_data.h>

#define FOLDER_TYPE 0
#define RIDE_TYPE 1

static char *rideFileRegExp = ("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                               "_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.(raw|srm)$");

MainWindow::MainWindow(const QDir &home) : 
    home(home), settings(GC_SETTINGS_CO, GC_SETTINGS_APP)
{
    setWindowTitle(home.dirName());
    settings.setValue(GC_SETTINGS_LAST, home.dirName());

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
    // TODO:
    // treeWidget->header()->resizeSection(0,80);
    treeWidget->header()->hide();

    allRides = new QTreeWidgetItem(treeWidget, FOLDER_TYPE);
    allRides->setText(0, tr("All Rides"));
    treeWidget->expandItem(allRides);
    splitter->addWidget(treeWidget);

    QRegExp rx(rideFileRegExp);
    QStringList filters;
    filters << "*.raw" << "*.srm";
    QTreeWidgetItem *last = NULL;
    QStringListIterator i(home.entryList(filters, QDir::Files));
    while (i.hasNext()) {
        QString name = i.next();
        if (rx.exactMatch(name)) {
            assert(rx.numCaptures() == 7);
            QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt()); 
            QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt()); 
            QDateTime dt(date, time);
            last = new RideItem(allRides, RIDE_TYPE, home.path(), name, dt);
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
    cpintTimeLabel = new QLabel(tr("Interval Duration:"), window);
    cpintTodayLabel = new QLabel(tr("Today:"), window);
    cpintAllLabel = new QLabel(tr("All Rides:"), window);
    cpintPickerLayout->addWidget(cpintTimeLabel);
    cpintPickerLayout->addWidget(cpintTodayLabel);
    cpintPickerLayout->addWidget(cpintAllLabel);
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

    //////////////////////// Power Histogram Tab ////////////////////////
    
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
    RideItem *last = new RideItem(allRides, RIDE_TYPE, home.path(), name, dt);
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

    QTextStream out(&file);
    out << "Time (min),Torq (N-m),Speed (MPH),Power (watts),"
        << "Distance (miles),Cadence (RPM),Heart rate (BPM),"
        << "Interval\n";

    QListIterator<RawFilePoint*> i(ride->raw->points);
    while (i.hasNext()) {
        RawFilePoint *point = i.next();
        out << point->secs/60.0;
        out << ",";
        if (point->nm >= 0) out << point->nm;
        out << ",";
        if (point->mph >= 0) out << point->mph;
        out << ",";
        if (point->watts >= 0) out << point->watts;
        out << ",";
        out << point->miles;
        out << ",";
        out << point->cad;
        out << ",";
        out << point->hr;
        out << ",";
        out << point->interval << "\n";
    }

    file.close();
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
        RawFile *raw = RawFile::readFile(file, errors);
        if (!raw || !errors.empty()) {
            QString all = (raw 
                           ? tr("Non-fatal problem(s) opening %1:")
                           : tr("Fatal problem(s) opening %1:")).arg(fileName);
            QStringListIterator i(errors);
            while (i.hasNext())
                all += "\n" + i.next();
            if (raw)
                QMessageBox::warning(this, tr("Open Warning"), all);
            else {
                QMessageBox::critical(this, tr("Open Error"), all);
                return;
            }
        }

        QChar zero = QLatin1Char('0'); 
        QString name = QString("%1_%2_%3_%4_%5_%6.srm")
            .arg(raw->startTime.date().year(), 4, 10, zero)
            .arg(raw->startTime.date().month(), 2, 10, zero)
            .arg(raw->startTime.date().day(), 2, 10, zero)
            .arg(raw->startTime.time().hour(), 2, 10, zero)
            .arg(raw->startTime.time().minute(), 2, 10, zero)
            .arg(raw->startTime.time().second(), 2, 10, zero);

        if (!file.copy(home.absolutePath() + "/" + name)) {
            QMessageBox::critical(this, tr("Copy Error"), 
                                  tr("Couldn't copy %1").arg(fileName));
            return;
        }

        delete raw;
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
            allPlot->setData(ride->raw);
            if (tabWidget->currentIndex() == 2)
                cpintPlot->calculate(ride->fileName, ride->dateTime);
            powerHist->setData(ride->raw);

            QDate wstart = ride->dateTime.date();
            wstart = wstart.addDays(Qt::Monday - wstart.dayOfWeek());
            assert(wstart.dayOfWeek() == Qt::Monday);
            QDate wend = wstart.addDays(7);
            double weeklySeconds = 0.0;
            double weeklyDistance = 0.0;
            double weeklyWork = 0.0;

            for (int i = 0; i < allRides->childCount(); ++i) {
                if (allRides->child(i)->type() == RIDE_TYPE) {
                    RideItem *item = (RideItem*) allRides->child(i);
                    if ((item->dateTime.date() >= wstart)
                        && (item->dateTime.date() < wend)) {
                        weeklySeconds += item->secsMovingOrPedaling();
                        weeklyDistance += item->totalDistance();
                        weeklyWork += item->totalWork();
                    }
                }
            }

            int minutes = ((int) round(weeklySeconds)) / 60;
            int hours = (int) minutes / 60;
            minutes %= 60;

            const char *dateFormat = "MM/dd/yyyy";
            weeklySummary->setHtml(tr(
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
                "</table>"
                "</center>"
                )
                .arg(wstart.toString(dateFormat))
                .arg(wstart.addDays(6).toString(dateFormat))
                .arg(hours)
                .arg(minutes, 2, 10, QLatin1Char('0'))
                .arg((unsigned) round(weeklyDistance))
                .arg((unsigned) round(weeklyWork))
                );

            return;
        }
    }
    rideSummary->clear();
}

void 
MainWindow::resizeEvent(QResizeEvent*)
{
    settings.setValue(GC_SETTINGS_MAIN_GEOM, geometry());
}

void 
MainWindow::moveEvent(QMoveEvent*)
{
    settings.setValue(GC_SETTINGS_MAIN_GEOM, geometry());
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

static QString
time_to_string(double secs) {
    if (secs < 60.0)
        return QString("%1s").arg(secs, 0, 'f', 2, QLatin1Char('0'));
    QString result;
    unsigned rounded = (unsigned) round(secs);
    bool needs_colon = false;
    if (rounded >= 3600) {
        result += QString("%1h").arg(rounded / 3600);
        rounded %= 3600;
        needs_colon = true;
    }
    if (needs_colon || rounded >= 60) {
        if (needs_colon)
            result += " ";
        result += QString("%1m").arg(rounded / 60, 2, 10, QLatin1Char('0'));
        rounded %= 60;
        needs_colon = true;
    }
    if (needs_colon)
        result += " ";
    result += QString("%1s").arg(rounded, 2, 10, QLatin1Char('0'));
    return result;
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
    cpintTimeLabel->setText(tr("Interval Duration: %1")
                            .arg(time_to_string(60.0*minutes)));
    cpintAllLabel->setText(tr("All Rides: %1 watts").arg(
            curve_to_point(minutes, cpintPlot->getAllCurve())));
    cpintTodayLabel->setText(tr("Today: %1 watts").arg(
            curve_to_point(minutes, cpintPlot->getThisCurve())));
}

