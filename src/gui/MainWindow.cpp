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
#include "DownloadRideDialog.h"
#include "RawFile.h"
#include "RideItem.h"
#include "Settings.h"
#include <QApplication>
#include <QtGui>
#include <QRegExp>

#define FOLDER_TYPE 0
#define RIDE_TYPE 1

static char *rideFileRegExp = ("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                               "_(\\d\\d)_(\\d\\d)_(\\d\\d)\\.raw$");

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
    filters << "*.raw";
    QTreeWidgetItem *last = NULL;
    QStringListIterator i(home.entryList(filters, QDir::Files));
    while (i.hasNext()) {
        QString name = i.next();
        if (rx.exactMatch(name)) {
            assert(rx.numCaptures() == 6);
            QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt()); 
            QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt()); 
            QDateTime dt(date, time);
            last = new RideItem(allRides, RIDE_TYPE, home.path(), name, dt);
        }
    }

    tabWidget = new QTabWidget;
    rideSummary = new QTextEdit;
    rideSummary->setReadOnly(true);
    tabWidget->addTab(rideSummary, "Ride Summary");

    QWidget *window = new QWidget;
    QVBoxLayout *vlayout = new QVBoxLayout;

    QHBoxLayout *showLayout = new QHBoxLayout;
    QLabel *showLabel = new QLabel("Show:", this);
    showLayout->addWidget(showLabel);

    QCheckBox *showGrid = new QCheckBox("Grid", this);
    showGrid->setCheckState(Qt::Checked);
    showLayout->addWidget(showGrid);

    QCheckBox *showPower = new QCheckBox("Power", this);
    showPower->setCheckState(Qt::Checked);
    showLayout->addWidget(showPower);

    QCheckBox *showHr = new QCheckBox("Heart Rate", this);
    showHr->setCheckState(Qt::Checked);
    showLayout->addWidget(showHr);

    QHBoxLayout *smoothLayout = new QHBoxLayout;
    QLabel *smoothLabel = new QLabel(tr("Smoothing (secs)"), this);
    smoothLineEdit = new QLineEdit(this);
    smoothLineEdit->setFixedWidth(30);
    
    smoothLayout->addWidget(smoothLabel);
    smoothLayout->addWidget(smoothLineEdit);
    smoothSlider = new QSlider(Qt::Horizontal);
    smoothSlider->setTickPosition(QSlider::TicksBelow);
    smoothSlider->setTickInterval(1);
    smoothSlider->setMinimum(2);
    smoothSlider->setMaximum(60);
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

    tabWidget->addTab(window, "All-in-One Graph");
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

    connect(treeWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(rideSelected()));
    connect(splitter, SIGNAL(splitterMoved(int,int)), 
            this, SLOT(splitterMoved()));
    connect(showPower, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showPower(int)));
    connect(showHr, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showHr(int)));
    connect(showGrid, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showGrid(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(returnPressed()),
            this, SLOT(setSmoothingFromLineEdit()));

    QMenu *fileMenu = new QMenu(tr("&File"), this); 
    fileMenu->addAction(tr("&New..."), this, 
                        SLOT(newCyclist()), tr("Ctrl+N")); 
    fileMenu->addAction(tr("&Open..."), this, 
                        SLOT(openCyclist()), tr("Ctrl+O")); 
    fileMenu->addAction(tr("&Download ride..."), this, 
                        SLOT(downloadRide()), tr("Ctrl+D")); 

    QMenuBar *menuBar = new QMenuBar(this); 
    menuBar->addMenu(fileMenu); 

    if (last != NULL) {
        treeWidget->setCurrentItem(last);
        rideSelected();
    }
}

void
MainWindow::addRide(QString name) 
{
    QRegExp rx(rideFileRegExp);
    if (!rx.exactMatch(name))
        assert(false);
    assert(rx.numCaptures() == 6);
    QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(),rx.cap(3).toInt()); 
    QTime time(rx.cap(4).toInt(), rx.cap(5).toInt(),rx.cap(6).toInt()); 
    QDateTime dt(date, time);
    RideItem *last = new RideItem(allRides, RIDE_TYPE, home.path(), name, dt);
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

