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
#include "ManualRideDialog.h"
#include "PowerHist.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "QuarqRideFile.h"
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
#include <qwt_plot_grid.h>
#include <qwt_data.h>
#include <boost/scoped_ptr.hpp>
#include "DaysScaleDraw.h"
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
    QWidget *window = new QWidget;
    QVBoxLayout *vlayout = new QVBoxLayout;

    QHBoxLayout *showLayout = new QHBoxLayout;
    QLabel *showLabel = new QLabel("Show:", window);
    showLayout->addWidget(showLabel);

    QCheckBox *showGrid = new QCheckBox("Grid", window);
    showGrid->setCheckState(Qt::Checked);
    showLayout->addWidget(showGrid);

    showHr = new QCheckBox("Heart Rate", window);
    showHr->setCheckState(Qt::Checked);
    showLayout->addWidget(showHr);

    showSpeed = new QCheckBox("Speed", window);
    showSpeed->setCheckState(Qt::Checked);
    showLayout->addWidget(showSpeed);

    showCad = new QCheckBox("Cadence", window);
    showCad->setCheckState(Qt::Checked);
    showLayout->addWidget(showCad);

    showAlt = new QCheckBox("Altitude", window);
    showAlt->setCheckState(Qt::Checked);
    showLayout->addWidget(showAlt);

    showPower = new QComboBox();
    showPower->addItem(tr("Power + shade"));
    showPower->addItem(tr("Power - shade"));
    showPower->addItem(tr("No Power"));
    showLayout->addWidget(showPower);

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
    allPlot = new AllPlot();
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

    window = new QWidget;
    vlayout = new QVBoxLayout;
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
    QLabel *binWidthLabel = new QLabel(tr("Bin width"), window);
    binWidthLineEdit = new QLineEdit(window);
    binWidthLineEdit->setFixedWidth(30);
    
    binWidthLayout->addWidget(binWidthLabel);
    binWidthLayout->addWidget(binWidthLineEdit);
    binWidthSlider = new QSlider(Qt::Horizontal);
    binWidthSlider->setTickPosition(QSlider::TicksBelow);
    binWidthSlider->setTickInterval(1);
    binWidthSlider->setMinimum(1);
    binWidthSlider->setMaximum(100);
    binWidthLayout->addWidget(binWidthSlider);

    lnYHistCheckBox = new QCheckBox;
    lnYHistCheckBox->setText("Log y");
    binWidthLayout->addWidget(lnYHistCheckBox);

    withZerosCheckBox = new QCheckBox;
    withZerosCheckBox->setText("With zeros");
    binWidthLayout->addWidget(withZerosCheckBox);
    histParameterCombo = new QComboBox();
    binWidthLayout->addWidget(histParameterCombo);

    powerHist = new PowerHist();
    setHistTextValidator();

    lnYHistCheckBox->setChecked(powerHist->islnY());
    withZerosCheckBox->setChecked(powerHist->withZeros());
    binWidthSlider->setValue(powerHist->binWidth());
    setHistBinWidthText();
    vlayout->addWidget(powerHist);
    vlayout->addLayout(binWidthLayout);
    window->setLayout(vlayout);
    window->show();

    tabWidget->addTab(window, "Histogram Analysis");
    
    //////////////////////// Pedal Force/Velocity Plot ////////////////////////

    window = new QWidget;
    vlayout = new QVBoxLayout;
    QHBoxLayout *qaLayout = new QHBoxLayout;

    pfPvPlot = new PfPvPlot();
    QLabel *qaCPLabel = new QLabel(tr("Watts:"), window);
    qaCPValue = new QLineEdit(QString("%1").arg(pfPvPlot->getCP()));
    qaCPValue->setValidator(new QIntValidator(0, 9999, qaCPValue));
    QLabel *qaCadLabel = new QLabel(tr("RPM:"), window);
    qaCadValue = new QLineEdit(QString("%1").arg(pfPvPlot->getCAD()));
    qaCadValue->setValidator(new QIntValidator(0, 999, qaCadValue));
    QLabel *qaClLabel = new QLabel(tr("Crank Length (m):"), window);
    qaClValue = new QLineEdit(QString("%1").arg(1000 * pfPvPlot->getCL()));
    shadeZonesPfPvCheckBox = new QCheckBox;
    shadeZonesPfPvCheckBox->setText("Shade zones");
    shadeZonesPfPvCheckBox->setCheckState(Qt::Checked);

    qaLayout->addWidget(qaCPLabel);
    qaLayout->addWidget(qaCPValue);
    qaLayout->addWidget(qaCadLabel);
    qaLayout->addWidget(qaCadValue);
    qaLayout->addWidget(qaClLabel);
    qaLayout->addWidget(qaClValue);
    qaLayout->addWidget(shadeZonesPfPvCheckBox);

    vlayout->addWidget(pfPvPlot);
    vlayout->addLayout(qaLayout);
    window->setLayout(vlayout);
    window->show();

    connect(pfPvPlot, SIGNAL(changedCP(const QString&)),
            qaCPValue, SLOT(setText(const QString&)) );
    connect(pfPvPlot, SIGNAL(changedCAD(const QString&)),
            qaCadValue, SLOT(setText(const QString&)) );
    connect(pfPvPlot, SIGNAL(changedCL(const QString&)),
            qaClValue, SLOT(setText(const QString&)) );

    tabWidget->addTab(window, tr("PF/PV Plot"));


    //////////////////////// Ride Notes ////////////////////////
    
    rideNotes = new QTextEdit;
    tabWidget->addTab(rideNotes, tr("Notes"));
    
    //////////////////////// Weekly Summary ////////////////////////
    
    // add daily distance / duration graph:
    window = new QWidget;
    QGridLayout *glayout = new QGridLayout;
    
    // set up the weekly distance / duration plot:
    weeklyPlot = new QwtPlot();
    weeklyPlot->enableAxis(QwtPlot::yRight, true);
    weeklyPlot->setAxisMaxMinor(QwtPlot::xBottom,0);
    weeklyPlot->setAxisScaleDraw(QwtPlot::xBottom, new DaysScaleDraw());
    QFont weeklyPlotAxisFont = weeklyPlot->axisFont(QwtPlot::yLeft);
    weeklyPlotAxisFont.setPointSize(weeklyPlotAxisFont.pointSize() * 0.9f);
    weeklyPlot->setAxisFont(QwtPlot::xBottom, weeklyPlotAxisFont);
    weeklyPlot->setAxisFont(QwtPlot::yLeft, weeklyPlotAxisFont);
    weeklyPlot->setAxisFont(QwtPlot::yRight, weeklyPlotAxisFont);
    
    weeklyDistCurve = new QwtPlotCurve();
    weeklyDistCurve->setStyle(QwtPlotCurve::Steps);
    QPen pen(Qt::SolidLine);
    weeklyDistCurve->setPen(pen);
    weeklyDistCurve->setBrush(Qt::red);
    weeklyDistCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    weeklyDistCurve->setCurveAttribute(QwtPlotCurve::Inverted, true); // inverted, right-to-left
    weeklyDistCurve->attach(weeklyPlot); 

    weeklyDurationCurve = new QwtPlotCurve();
    weeklyDurationCurve->setStyle(QwtPlotCurve::Steps);
    weeklyDurationCurve->setPen(pen); 
    weeklyDurationCurve->setBrush(QColor(255,200,0,255));
    weeklyDurationCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    weeklyDurationCurve->setCurveAttribute(QwtPlotCurve::Inverted, true); // inverted, right-to-left
    weeklyDurationCurve->setYAxis(QwtPlot::yRight);
    weeklyDurationCurve->attach(weeklyPlot); 
    
    // set up the weekly bike score plot:
    weeklyBSPlot = new QwtPlot();
    weeklyBSPlot->enableAxis(QwtPlot::yRight, true);
    weeklyBSPlot->setAxisMaxMinor(QwtPlot::xBottom,0);
    weeklyBSPlot->setAxisScaleDraw(QwtPlot::xBottom, new DaysScaleDraw());
    QwtText textLabel = QwtText();
    weeklyBSPlot->setAxisFont(QwtPlot::xBottom, weeklyPlotAxisFont);
    weeklyBSPlot->setAxisFont(QwtPlot::yLeft, weeklyPlotAxisFont);
    weeklyBSPlot->setAxisFont(QwtPlot::yRight, weeklyPlotAxisFont);
    
    weeklyBSCurve = new QwtPlotCurve();
    weeklyBSCurve->setStyle(QwtPlotCurve::Steps);
    weeklyBSCurve->setPen(pen);
    weeklyBSCurve->setBrush(Qt::blue);
    weeklyBSCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    weeklyBSCurve->setCurveAttribute(QwtPlotCurve::Inverted, true); // inverted, right-to-left
    weeklyBSCurve->attach(weeklyBSPlot); 
    
    weeklyRICurve = new QwtPlotCurve();
    weeklyRICurve->setStyle(QwtPlotCurve::Steps);
    weeklyRICurve->setPen(pen);
    weeklyRICurve->setBrush(Qt::green);
    weeklyRICurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    weeklyRICurve->setCurveAttribute(QwtPlotCurve::Inverted, true); // inverted, right-to-left
    weeklyRICurve->setYAxis(QwtPlot::yRight);
    weeklyRICurve->attach(weeklyBSPlot); 

    // set baseline curves to obscure linewidth variations along baseline
    pen.setWidth(2);
    weeklyBaselineCurve   = new QwtPlotCurve();
    weeklyBaselineCurve->setPen(pen);
    weeklyBaselineCurve->attach(weeklyPlot);
    weeklyBSBaselineCurve = new QwtPlotCurve();
    weeklyBSBaselineCurve->setPen(pen);
    weeklyBSBaselineCurve->attach(weeklyBSPlot);
   
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->enableX(false);
    QPen gridPen;
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
    grid->attach(weeklyPlot);
    
    QwtPlotGrid *grid1 = new QwtPlotGrid();
    grid1->enableX(false);
    gridPen.setStyle(Qt::DotLine);
    grid1->setPen(gridPen);
    grid1->attach(weeklyBSPlot);
    
    weeklySummary = new QTextEdit;
    weeklySummary->setReadOnly(true);
    
    glayout->addWidget(weeklySummary,0,0,1,-1); // row, col, rowspan, colspan. -1 == fill to edge
    glayout->addWidget(weeklyPlot,1,0);
    glayout->addWidget(weeklyBSPlot,1,1);
    
    glayout->setRowStretch(0, 3);           // stretch factor of summary
    glayout->setRowStretch(1, 2);           // stretch factor of weekly plots
    glayout->setColumnStretch(0, 1);        // stretch first column
    glayout->setColumnStretch(1, 1);        // stretch second column
    glayout->setRowMinimumHeight(0, 180);   // minimum height of weekly summary
    glayout->setRowMinimumHeight(1, 120);   // minimum height of weekly plots
    
    window->setLayout(glayout);
    window->show();
    tabWidget->addTab(window, tr("Weekly Summary"));

    ////////////////////////////// Signals ////////////////////////////// 

    connect(calendar, SIGNAL(clicked(const QDate &)),
            this, SLOT(dateChanged(const QDate &)));
    connect(leftLayout, SIGNAL(splitterMoved(int,int)),
            this, SLOT(leftLayoutMoved()));
    connect(treeWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(rideSelected()));
    connect(splitter, SIGNAL(splitterMoved(int,int)), 
            this, SLOT(splitterMoved()));
    connect(showPower, SIGNAL(currentIndexChanged(int)),
            allPlot, SLOT(showPower(int)));
    connect(showHr, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showHr(int)));
    connect(showSpeed, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showSpeed(int)));
    connect(showCad, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showCad(int)));
    connect(showAlt, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showAlt(int)));
    connect(showGrid, SIGNAL(stateChanged(int)),
            allPlot, SLOT(showGrid(int)));
    connect(comboDistance, SIGNAL(currentIndexChanged(int)),
            allPlot, SLOT(setByDistance(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(editingFinished()),
            this, SLOT(setSmoothingFromLineEdit()));
    connect(cpintSetCPButton, SIGNAL(clicked()),
	    this, SLOT(cpintSetCPButtonClicked()));
    connect(binWidthSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setBinWidthFromSlider()));
    connect(binWidthLineEdit, SIGNAL(editingFinished()),
            this, SLOT(setBinWidthFromLineEdit()));
    connect(lnYHistCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(setlnYHistFromCheckBox()));
    connect(withZerosCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(setWithZerosFromCheckBox()));
    connect(histParameterCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setHistSelection(int)));
    connect(shadeZonesPfPvCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(setShadeZonesPfPvFromCheckBox()));
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
        tr("Raw Powertap Files (*.raw);;Comma Separated Variable (*.csv);;SRM training files (*.srm);;Garmin Training Centre (*.tcx);;Polar Precision (*.hrm);;WKO+ Files (*.wko);;Quarq ANT+ (*.qla);;All files (*.*)"));
    } else {
        fileNames = QFileDialog::getOpenFileNames(
        this, tr("Import from File"), lastDir,
        tr("Raw Powertap Files (*.raw);;Comma Separated Variable (*.csv);;SRM training files (*.srm);;Garmin Training Centre (*.tcx);;Polar Precision (*.hrm);;WKO+ Files (*.wko);;All files (*.*)"));

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

	setAllPlotWidgets(ride);
	allPlot->setData(ride);

	// set the histogram data
	powerHist->setData(ride);
	// make sure the histogram has a legal selection
	powerHist->fixSelection();
	// update the options in the histogram combobox
	setHistWidgets(ride);

	pfPvPlot->setData(ride);
	// update the QLabel widget with the CP value set in PfPvPlot::setData()
	qaCPValue->setText(QString("%1").arg(pfPvPlot->getCP()));

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
    generateWeeklySummary();
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

void MainWindow::generateWeeklySummary()
{
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
    RideMetric *weeklyBS = factory.newMetric("skiba_bike_score");
    assert(weeklyBS);
    RideMetric *weeklyRelIntensity = factory.newMetric("skiba_relative_intensity");
    assert(weeklyRelIntensity);

    RideMetric *dailySeconds[7];
    RideMetric *dailyDistance[7];
    RideMetric *dailyBS[7];
    RideMetric *dailyRI[7];
    RideMetric *dailyW[7];
    RideMetric *dailyXP[7];

    for (int i = 0; i < 7; i++) {
	dailySeconds[i] = factory.newMetric("time_riding");
	assert(dailySeconds[i]);
	dailyDistance[i] = factory.newMetric("total_distance");
	assert(dailyDistance[i]);
	dailyBS[i] = factory.newMetric("skiba_bike_score");
	assert(dailyBS[i]);
	dailyRI[i] = factory.newMetric("skiba_relative_intensity");
	assert(dailyRI[i]);
	dailyW[i] = factory.newMetric("total_work");
	assert(dailyW[i]);
	dailyXP[i] = factory.newMetric("skiba_xpower");
	assert(dailyXP[i]);
    }
    
    int zone_range = -1;
    double *time_in_zone = NULL;
    int num_zones = -1;
    bool zones_ok = true;

    for (int i = 0; i < allRides->childCount(); ++i) {
	RideItem *item = (RideItem*) allRides->child(i);
	int day;
        if (
	    (item->type() == RIDE_TYPE) &&
	    (item->ride) &&
	    ((day = wstart.daysTo(item->dateTime.date())) >= 0) && 
	    (day < 7)
	    ) {

	    RideMetric *m;
	    item->htmlSummary(); // compute metrics
	    if ((m = item->metrics.value(weeklySeconds->name()))) {
		weeklySeconds->aggregateWith(m);
		dailySeconds[day]->aggregateWith(m);
	    }

	    if ((m = item->metrics.value(weeklyDistance->name()))) {
		weeklyDistance->aggregateWith(m);
		dailyDistance[day]->aggregateWith(m);
	    }

	    if ((m = item->metrics.value(weeklyWork->name()))) {
		weeklyWork->aggregateWith(m);
		dailyW[day]->aggregateWith(m);
	    }

        if ((m = item->metrics.value(weeklyBS->name()))) {
		weeklyBS->aggregateWith(m);
		dailyBS[day]->aggregateWith(m);
	    }

	    if ((m = item->metrics.value(weeklyRelIntensity->name()))) {
		weeklyRelIntensity->aggregateWith(m);
		dailyRI[day]->aggregateWith(m);
	    }

	    if ((m = item->metrics.value("skiba_xpower")))
		dailyXP[day]->aggregateWith(m);

	    // compute time in zones
	    if (zones) {
		if (zone_range == -1) {
		    zone_range = item->zoneRange();
		    num_zones = item->numZones();
		    time_in_zone = new double[num_zones];
		    for (int j = 0; j < num_zones; ++j)
			time_in_zone[j] = 0;
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

    int seconds = ((int) round(weeklySeconds->value(true)));
    int minutes = seconds / 60;
    seconds %= 60;
    int hours = minutes / 60;
    minutes %= 60;

    const char *dateFormat = "MM/dd/yyyy";
    
    QString summary;
    summary =
	tr(
	   "<center>"
	   "<h2>Week of %1 through %2</h2>"
	   "<h2>Summary</h2>"
	   "<p>"
	   "<table align=\"center\" width=\"60%\" border=0>"
	   "<tr><td>Total time riding:</td>"
	   "    <td align=\"right\">%3:%4:%5</td></tr>"
	   "<tr><td>Total distance (%6):</td>"
	   "    <td align=\"right\">%7</td></tr>"
	   "<tr><td>Total work (kJ):</td>"
	   "    <td align=\"right\">%8</td></tr>"
	   "<tr><td>Daily Average work (kJ):</td>"
	   "    <td align=\"right\">%9</td></tr>"
	   )
	.arg(wstart.toString(dateFormat))
	.arg(wstart.addDays(6).toString(dateFormat))
	.arg(hours)
	.arg(minutes, 2, 10, QLatin1Char('0'))
	.arg(seconds, 2, 10, QLatin1Char('0'))
	.arg(useMetricUnits ? "km" : "miles")
	.arg(weeklyDistance->value(useMetricUnits), 0, 'f', 1)
	.arg((unsigned) round(weeklyWork->value(useMetricUnits)))
	.arg((unsigned) round(weeklyWork->value(useMetricUnits) / 7));

    double weeklyBSValue = weeklyBS->value(useMetricUnits);
    bool useBikeScore = (zone_range != -1) && (weeklyBSValue > 0);

    if (zone_range != -1) {
	if (useBikeScore)
	    summary += 
		tr(
		   "<tr><td>Total BikeScore:</td>"
		   "    <td align=\"right\">%1</td></tr>"
		   "<tr><td>Net Relative Intensity:</td>"
		   "    <td align=\"right\">%2</td></tr>"
		   )
		.arg((unsigned) round(weeklyBSValue))
		.arg(weeklyRelIntensity->value(useMetricUnits), 0, 'f', 3);

        summary +=
	    tr(
	       "</table>"
	       "<h2>Power Zones</h2>"
	       );
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
    delete weeklyBS;
    delete weeklyRelIntensity;

    // set the axis labels of the weekly plots
    QwtText textLabel = QwtText(useMetricUnits ? "km" : "miles");
    QFont weeklyPlotAxisTitleFont = textLabel.font();
    weeklyPlotAxisTitleFont.setPointSize(10);
    weeklyPlotAxisTitleFont.setBold(true);
    textLabel.setFont(weeklyPlotAxisTitleFont);
    weeklyPlot->setAxisTitle(QwtPlot::yLeft, textLabel);
    textLabel.setText("Minutes");
    weeklyPlot->setAxisTitle(QwtPlot::yRight, textLabel);
    textLabel.setText(useBikeScore ? "BikeScore" : "kJoules");
    weeklyBSPlot->setAxisTitle(QwtPlot::yLeft, textLabel);
    textLabel.setText(useBikeScore ? "Intensity" : "xPower");
    weeklyBSPlot->setAxisTitle(QwtPlot::yRight, textLabel);

    // for the daily distance/duration and bikescore plots:
    // first point: establish zero position
    // points 2N, 2N+1: Nth day (N from 1 to 7), up then down
    // 16th point: move to draw baseline off right of plot
    
    double xdist[16];
    double xdur[16];
    double xbsorw[16];
    double xriorxp[16];

    double ydist[16];   // daily distance
    double ydur[16];    // daily total time
    double ybsorw[16];     // daily minutes
    double yriorxp[16];     // daily relative intensity

    // data for a "baseline" curve to draw a baseline
    double xbaseline[] = {0, 8};
    double ybaseline[] = {0, 0};
    weeklyBaselineCurve->setData(xbaseline, ybaseline, 2);
    weeklyBSBaselineCurve->setData(xbaseline, ybaseline, 2);

    const double bar_width = 0.3;

    int i = 0;
    xdist[i] =
	xdur[i] =
	xbsorw[i] =
	xriorxp[i] =
	0;

    ydist[i] =
	ydur[i] =
	ybsorw[i] =
	yriorxp[i] =
	0;

    for(int day = 0; day < 7; day++){
	double x;

	i++;
        xdist[i]         = x = day + 1 - bar_width;
        xdist[i + 1]     = x += bar_width;
        xdur[i]          = x;
        xdur[i + 1]      = x += bar_width;

        xbsorw[i]        = x = day + 1 - bar_width;
        xbsorw[i + 1]    = x += bar_width;
        xriorxp[i]       = x;
	xriorxp[i + 1]   = x += bar_width;

	ydist[i]         = dailyDistance[day]->value(useMetricUnits);
	ydur[i]          = dailySeconds[day]->value(useMetricUnits) / 60;
	ybsorw[i]        = useBikeScore ? dailyBS[day]->value(useMetricUnits) : dailyW[day]->value(useMetricUnits) / 1000;
	yriorxp[i]       = useBikeScore ? dailyRI[day]->value(useMetricUnits) : dailyXP[day]->value(useMetricUnits);

	i++;
	ydist[i] = 0;
	ydur[i]  = 0;
	ybsorw[i]   = 0;
	yriorxp[i]   = 0;

	delete dailyDistance[day];
	delete dailySeconds[day];
	delete dailyBS[day];
	delete dailyRI[day];
	delete dailyW[day];
	delete dailyXP[day];
    }

    // sweep a baseline off the right of the plot
    i++;
    xdist[i] =
	xdur[i] =
	xbsorw[i] =
	xriorxp[i] =
	8;

    ydist[i] =
	ydur[i] =
	ybsorw[i] =
	yriorxp[i] =
	0;

    // Distance/Duration plot:
    weeklyDistCurve->setData(xdist, ydist, 16);
    weeklyPlot->setAxisScale(QwtPlot::yLeft, 0, weeklyDistCurve->maxYValue()*1.1, 0);
    weeklyPlot->setAxisScale(QwtPlot::xBottom, 0.5, 7.5, 0);
    weeklyPlot->setAxisTitle(QwtPlot::yLeft, useMetricUnits ? "Kilometers" : "Miles");

    weeklyDurationCurve->setData(xdur, ydur, 16);
    weeklyPlot->setAxisScale(QwtPlot::yRight, 0, weeklyDurationCurve->maxYValue()*1.1, 0);
    weeklyPlot->replot();
    
    // BikeScore/Relative Intensity plot
    weeklyBSCurve->setData(xbsorw, ybsorw, 16);
    weeklyBSPlot->setAxisScale(QwtPlot::yLeft, 0, weeklyBSCurve->maxYValue()*1.1, 0);
    weeklyBSPlot->setAxisScale(QwtPlot::xBottom, 0.5, 7.5, 0);
    
    // set axis minimum for relative intensity
    double RImin = -1;
    for(int i = 1; i < 16; i += 2)
        if (yriorxp[i] > 0 && ((RImin < 0) || (yriorxp[i] < RImin)))
	    RImin = yriorxp[i];
    if (RImin < 0)
	RImin = 0;
    RImin *= 0.8;
    for (int i = 0; i < 16; i ++)
	if (yriorxp[i] < RImin)
	    yriorxp[i] = RImin;
    weeklyRICurve->setBaseline(RImin);
    weeklyRICurve->setData(xriorxp, yriorxp, 16);
    weeklyBSPlot->setAxisScale(QwtPlot::yRight, RImin, weeklyRICurve->maxYValue()*1.1, 0);

    weeklyBSPlot->replot();
    
    
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
	generateWeeklySummary();

	// all plot
	allPlot->refreshZoneLabels();
	allPlot->replot();

	// histogram
	powerHist->refreshZoneLabels();
	powerHist->replot();

	// force-versus-pedal velocity plot
	pfPvPlot->refreshZoneItems();
	pfPvPlot->replot();
        qaCPValue->setText(QString("%1").arg(pfPvPlot->getCP()));
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
    generateWeeklySummary();
  }

}

void
MainWindow::setBinWidthFromSlider()
{
    if (powerHist->binWidth() != binWidthSlider->value()) {
        powerHist->setBinWidth(binWidthSlider->value());
	setHistBinWidthText();
    }
}

void
MainWindow::setlnYHistFromCheckBox()
{
    if (powerHist->islnY() != lnYHistCheckBox->isChecked())
	powerHist->setlnY(! powerHist->islnY());
}

void
MainWindow::setWithZerosFromCheckBox()
{
    if (powerHist->withZeros() != withZerosCheckBox->isChecked()) {
        powerHist->setWithZeros(withZerosCheckBox->isChecked());
    }
}

void
MainWindow::setHistBinWidthText()
{
    binWidthLineEdit->setText(QString("%1").arg(powerHist->getBinWidthRealUnits(), 0, 'g', 3));
}

void
MainWindow::setHistTextValidator()
{
    double delta = powerHist->getDelta();
    int digits = powerHist->getDigits();
    binWidthLineEdit->setValidator(
	    (digits == 0) ?
	    (QValidator *) (
		new QIntValidator(binWidthSlider->minimum() * delta,
		    binWidthSlider->maximum() * delta,
		    binWidthLineEdit
		    )
		) :
	    (QValidator *) (
		new QDoubleValidator(binWidthSlider->minimum() * delta,
		    binWidthSlider->maximum() * delta,
		    digits,
		    binWidthLineEdit
		    )
		)
	    );

}

void
MainWindow::setHistSelection(int id)
{
    if (id == histWattsShadedID)
	powerHist->setSelection(PowerHist::wattsShaded);
    else if (id == histWattsUnshadedID)
	powerHist->setSelection(PowerHist::wattsUnshaded);
    else if (id == histNmID)
	powerHist->setSelection(PowerHist::nm);
    else if (id == histHrID)
	powerHist->setSelection(PowerHist::hr);
    else if (id == histKphID)
	powerHist->setSelection(PowerHist::kph);
    else if (id == histCadID)
	powerHist->setSelection(PowerHist::cad);
    else
	fprintf(stderr, "Illegal id encountered: %d", id);

    setHistBinWidthText();
    setHistTextValidator();
}

void
MainWindow::setShadeZonesPfPvFromCheckBox()
{
    if (pfPvPlot->shadeZones() != shadeZonesPfPvCheckBox->isChecked()) {
        pfPvPlot->setShadeZones(shadeZonesPfPvCheckBox->isChecked());
    }
}

void
MainWindow::setBinWidthFromLineEdit()
{
    double value = binWidthLineEdit->text().toDouble();
    if (value != powerHist->binWidth()) {
	binWidthSlider->setValue(powerHist->setBinWidthRealUnits(value));
	setHistBinWidthText();
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

void MainWindow::setAllPlotWidgets(RideItem *ride)
{
    if (ride->ride) {
	RideFileDataPresent *dataPresent = ride->ride->areDataPresent();
	showPower->setEnabled(dataPresent->watts);
	showHr->setEnabled(dataPresent->hr);
	showSpeed->setEnabled(dataPresent->kph);
	showCad->setEnabled(dataPresent->cad);
	showAlt->setEnabled(dataPresent->alt);
	allPlot->showPower(showPower->currentIndex());
	allPlot->wattsCurve->setVisible(dataPresent->watts && (showPower->currentIndex() < 2));
	allPlot->hrCurve->setVisible(dataPresent->hr && showHr->isChecked());
	allPlot->speedCurve->setVisible(dataPresent->kph && showSpeed->isChecked());
	allPlot->cadCurve->setVisible(dataPresent->cad && showCad->isChecked());
	allPlot->altCurve->setVisible(dataPresent->alt && showAlt->isChecked());
    }
    else {
	showPower->setEnabled(false);
	showHr->setEnabled(false);
	showSpeed->setEnabled(false);
	showCad->setEnabled(false);
	showAlt->setEnabled(false);
	allPlot->showPower(false);
	allPlot->wattsCurve->setVisible(false);
	allPlot->hrCurve->setVisible(false);
	allPlot->speedCurve->setVisible(false);
	allPlot->cadCurve->setVisible(false);
    }
}

void MainWindow::setHistWidgets(RideItem *rideItem)
{
    int count = 0;
    assert(rideItem);
    RideFile *ride = rideItem->ride;

    // prevent selection from changing during reconstruction of options
    disconnect(histParameterCombo, SIGNAL(currentIndexChanged(int)),
	       this, SLOT(setHistSelection(int)));

    if (ride) {
	// we want to retain the present selection
	PowerHist::Selection s = powerHist->selection();
    
	histParameterCombo->clear();


	histWattsShadedID =
	    histWattsUnshadedID =
	    histNmID =
	    histHrID =
	    histKphID =
	    histCadID =
	    -1;

	if (ride->areDataPresent()->watts) {
	    histWattsShadedID = count ++;
	    histParameterCombo->addItem(tr("Watts(shaded)"));
	    histWattsUnshadedID = count ++;
	    histParameterCombo->addItem(tr("Watts(unshaded)"));
	}
	if (ride->areDataPresent()->nm) {
	    histNmID = count ++;
	    histParameterCombo->addItem(tr("Torque"));
	}
	if (ride->areDataPresent()->hr) {
	    histHrID = count ++;
	    histParameterCombo->addItem(tr("Heartrate"));
	}
	if (ride->areDataPresent()->kph) {
	    histKphID = count ++;
	    histParameterCombo->addItem(tr("Speed"));
	}
	if (ride->areDataPresent()->cad) {
	    histCadID = count ++;
	    histParameterCombo->addItem(tr("Cadence"));
	}
	if (count > 0) {
	    histParameterCombo->setEnabled(true);
	    binWidthLineEdit->setEnabled(true);
	    binWidthSlider->setEnabled(true);
	    withZerosCheckBox->setEnabled(true);
	    lnYHistCheckBox->setEnabled(true);    

	    // set widget to proper value
	    if ((s == PowerHist::wattsShaded) && (histWattsShadedID >= 0))
		histParameterCombo->setCurrentIndex(histWattsShadedID);
	    else if ((s == PowerHist::wattsUnshaded) && (histWattsUnshadedID >= 0))
		histParameterCombo->setCurrentIndex(histWattsUnshadedID);
	    else if ((s == PowerHist::nm) && (histNmID >= 0))
		histParameterCombo->setCurrentIndex(histNmID);
	    else if ((s == PowerHist::hr) && (histHrID >= 0))
		histParameterCombo->setCurrentIndex(histHrID);
	    else if ((s == PowerHist::kph) && (histKphID >= 0))
		histParameterCombo->setCurrentIndex(histKphID);
	    else if ((s == PowerHist::cad) && (histCadID >= 0))
		histParameterCombo->setCurrentIndex(histCadID);
	    else
		histParameterCombo->setCurrentIndex(0);

	    // reconnect widget
	    connect(histParameterCombo, SIGNAL(currentIndexChanged(int)),
		    this, SLOT(setHistSelection(int)));

	    return;
	}
    }

    histParameterCombo->addItem(tr("no data"));
    histParameterCombo->setEnabled(false);
    binWidthLineEdit->setEnabled(false);
    binWidthSlider->setEnabled(false);
    withZerosCheckBox->setEnabled(false);
    lnYHistCheckBox->setEnabled(false);    
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


