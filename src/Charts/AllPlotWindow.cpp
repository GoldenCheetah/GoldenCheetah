/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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


#include "Context.h"
#include "Context.h"
#include "Athlete.h"
#include "TabView.h"
#include "AllPlotInterval.h"
#include "AllPlotWindow.h"
#include "AllPlot.h"
#include "RideFile.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "TimeUtils.h"
#include "Settings.h"
#include "Units.h" // for MILES_PER_KM
#include "Colors.h" // for MILES_PER_KM
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_widget.h>
#include <qwt_arrow_button.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>

// span slider specials
#include <qxtspanslider.h>
#include <QStyleFactory>
#include <QStyle>
#include <QScrollBar>

// tooltip
#include "LTMWindow.h"

// help
#include "HelpWhatsThis.h"

// overlay helper
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"

static const int stackZoomWidth[8] = { 5, 10, 15, 20, 30, 45, 60, 120 };

AllPlotWindow::AllPlotWindow(Context *context) :
    GcChartWindow(context), current(NULL), context(context), active(false), stale(true), setupStack(false), setupSeriesStack(false), compareStale(true), firstShow(true)
{
    // basic setup
    setContentsMargins(0,0,0,0);
    QWidget *c = new QWidget;
    HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartRides_Performance));
    setControls(c);
    QVBoxLayout *clv = new QVBoxLayout(c);

    // all the controls
    QFormLayout *mainControls = new QFormLayout; // basic stuff at top; power, slider etc

    // aside from basic settings, other stuff is now
    // in a tab widget as we have so many data series !
    QTabWidget *st = new QTabWidget(this);
    clv->addWidget(st);

    // gui controls
    QWidget *basic = new QWidget(this); // show stack etc
    QVBoxLayout *basicControls = new QVBoxLayout(basic);
    basicControls->addLayout(mainControls);
    QFormLayout *guiControls = new QFormLayout; // show stack etc BUT ALSO ACCEL etc
    basicControls->addLayout(guiControls);
    basicControls->addStretch();
    st->addTab(basic, tr("Basic"));

    HelpWhatsThis *basicHelp = new HelpWhatsThis(basic);
    basic->setWhatsThis(basicHelp->getWhatsThisText(HelpWhatsThis::ChartRides_Performance_Config_Basic));

    // data series
    QWidget *series = new QWidget(this); // data series selection
    QHBoxLayout *seriesControls = new QHBoxLayout(series);
    QFormLayout *seriesLeft = new QFormLayout(); // ride side series
    QFormLayout *seriesRight = new QFormLayout(); // ride side series
    seriesControls->setSpacing(2 *dpiXFactor);
    seriesLeft->setSpacing(2 *dpiXFactor);
    seriesRight->setSpacing(2 *dpiXFactor);
    seriesControls->addLayout(seriesLeft);
    seriesControls->addLayout(seriesRight); // ack I swapped them around !
    st->addTab(series, tr("Curves"));

    HelpWhatsThis *seriesHelp = new HelpWhatsThis(series);
    series->setWhatsThis(seriesHelp->getWhatsThisText(HelpWhatsThis::ChartRides_Performance_Config_Series));

    // user data widget
    custom = new QWidget(this);
    st->addTab(custom, tr("User Data"));
    custom->setContentsMargins(20 *dpiXFactor,20 *dpiYFactor,20 *dpiXFactor,20 *dpiYFactor);
    //HelpWhatsThis *curvesHelp = new HelpWhatsThis(custom);
    //custom->setWhatsThis(curvesHelp->getWhatsThisText(HelpWhatsThis::ChartTrends_MetricTrends_Config_Curves));
    QVBoxLayout *customLayout = new QVBoxLayout(custom);
    customLayout->setContentsMargins(0,0,0,0);
    customLayout->setSpacing(5 *dpiXFactor);

    // custom table
    customTable = new QTableWidget(this);
#ifdef Q_OS_MAX
    customTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    customTable->setColumnCount(2);
    customTable->horizontalHeader()->setStretchLastSection(true);
    QStringList headings;
    headings << tr("Name");
    headings << tr("Formula");
    customTable->setHorizontalHeaderLabels(headings);
    customTable->setSortingEnabled(false);
    customTable->verticalHeader()->hide();
    customTable->setShowGrid(false);
    customTable->setSelectionMode(QAbstractItemView::SingleSelection);
    customTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    customLayout->addWidget(customTable);
    connect(customTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(doubleClicked(int, int)));

    // custom buttons
    editCustomButton = new QPushButton(tr("Edit"));
    connect(editCustomButton, SIGNAL(clicked()), this, SLOT(editUserData()));

    addCustomButton = new QPushButton("+");
    connect(addCustomButton, SIGNAL(clicked()), this, SLOT(addUserData()));

    deleteCustomButton = new QPushButton("-");
    connect(deleteCustomButton, SIGNAL(clicked()), this, SLOT(deleteUserData()));

#ifndef Q_OS_MAC
    upCustomButton = new QToolButton(this);
    downCustomButton = new QToolButton(this);
    upCustomButton->setArrowType(Qt::UpArrow);
    downCustomButton->setArrowType(Qt::DownArrow);
    upCustomButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    downCustomButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    addCustomButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteCustomButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    upCustomButton = new QPushButton(tr("Up"));
    downCustomButton = new QPushButton(tr("Down"));
#endif
    connect(upCustomButton, SIGNAL(clicked()), this, SLOT(moveUserDataUp()));
    connect(downCustomButton, SIGNAL(clicked()), this, SLOT(moveUserDataDown()));


    QHBoxLayout *customButtons = new QHBoxLayout;
    customButtons->setSpacing(2 *dpiXFactor);
    customButtons->addWidget(upCustomButton);
    customButtons->addWidget(downCustomButton);
    customButtons->addStretch();
    customButtons->addWidget(editCustomButton);
    customButtons->addStretch();
    customButtons->addWidget(addCustomButton);
    customButtons->addWidget(deleteCustomButton);
    customLayout->addLayout(customButtons);

    // Main layout
    //QGridLayout *mainLayout = new QGridLayout();
    //mainLayout->setContentsMargins(2,2,2,2);

    //
    // reveal controls widget
    //

    // reveal controls
    rSmooth = new QLabel(tr("Smooth"));
    rSmoothEdit = new QLineEdit();
    rSmoothEdit->setFixedWidth(30 *dpiXFactor);
    rSmoothSlider = new QSlider(Qt::Horizontal);
    rSmoothSlider->setTickPosition(QSlider::TicksBelow);
    rSmoothSlider->setTickInterval(10);
    rSmoothSlider->setMinimum(1);
    rSmoothSlider->setMaximum(100);
    rStack = new QCheckBox(tr("Stacked"));
    rBySeries = new QCheckBox(tr("by series"));
    rFull = new QCheckBox(tr("Fullplot"));
    rHelp = new QCheckBox(tr("Overlay"));

    // layout reveal controls
    QHBoxLayout *r = new QHBoxLayout;
    r->setContentsMargins(0,0,0,0);
    r->addStretch();
    r->addWidget(rSmooth);
    r->addWidget(rSmoothEdit);
    r->addWidget(rSmoothSlider);
    QVBoxLayout *v = new QVBoxLayout;
    QHBoxLayout *s = new QHBoxLayout;
    QHBoxLayout *t = new QHBoxLayout;
    s->addWidget(rStack);
    s->addWidget(rBySeries);
    v->addLayout(s);
    t->addWidget(rFull);
    t->addWidget(rHelp);
    v->addLayout(t);
    r->addSpacing(20 *dpiYFactor);
    r->addLayout(v);
    r->addStretch();
    setRevealLayout(r);
    //revealControls->setLayout(r);
    
    // hide them initially
    //revealControls->hide();

    // setup the controls
    QLabel *showLabel = new QLabel(tr("View"), c);

    showStack = new QCheckBox(tr("Stack"), this);
    showStack->setCheckState(Qt::Unchecked);
    guiControls->addRow(showLabel, showStack);

    showBySeries = new QCheckBox(tr("By Series"), this);
    showBySeries->setCheckState(Qt::Unchecked);
    guiControls->addRow(new QLabel("", this), showBySeries);
    guiControls->addRow(new QLabel("", this), new QLabel("",this)); // spacer

    stackWidth = 20 *dpiXFactor;
    stackZoomSlider = new QSlider(Qt::Horizontal,this);
    stackZoomSlider->setMinimum(0);
    stackZoomSlider->setMaximum(7);
    stackZoomSlider->setTickInterval(1);
    stackZoomSlider->setValue(3);
    guiControls->addRow(new QLabel(tr("Stack Zoom")), stackZoomSlider);

    showFull = new QCheckBox(tr("Full plot"), this);
    showFull->setCheckState(Qt::Checked);
    guiControls->addRow(new QLabel(""), showFull);

    showInterval = new QCheckBox(tr("Interval Navigator"), this);
    showInterval->setCheckState(Qt::Checked);
    guiControls->addRow(new QLabel(""), showInterval);

    showHover = new QCheckBox(tr("Hover intervals"), this);
    showHover->setCheckState(Qt::Checked);
    guiControls->addRow(new QLabel(""), showHover);

    showHelp = new QCheckBox(tr("Overlay"), this);
    showHelp->setCheckState(Qt::Unchecked);
    guiControls->addRow(new QLabel(""), showHelp);

    paintBrush = new QCheckBox(tr("Fill Curves"), this);
    paintBrush->setCheckState(Qt::Unchecked);
    guiControls->addRow(new QLabel(""), paintBrush);

    showGrid = new QCheckBox(tr("Grid"), this);
    showGrid->setCheckState(Qt::Checked);
    guiControls->addRow(new QLabel(""), showGrid);
    guiControls->addRow(new QLabel(""), new QLabel(""));

    showAccel = new QCheckBox(tr("Acceleration"), this);
    showAccel->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(tr("Delta Series")), showAccel);
    showPowerD = new QCheckBox(QString(tr("Power %1").arg(deltaChar)), this);
    showPowerD->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showPowerD);
    showCadD = new QCheckBox(QString(tr("Cadence %1").arg(deltaChar)), this);
    showCadD->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showCadD);
    showTorqueD = new QCheckBox(QString(tr("Torque %1").arg(deltaChar)), this);
    showTorqueD->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showTorqueD);
    showHrD = new QCheckBox(QString(tr("Heartrate %1").arg(deltaChar)), this);
    showHrD->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showHrD);

    seriesRight->addRow(new QLabel(""), new QLabel(""));

    showBalance = new QCheckBox(tr("Balance"), this);
    showBalance->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(tr("Left/Right")), showBalance);

    showTE = new QCheckBox(tr("Torque Effectiveness"));
    showTE->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showTE);

    showPS = new QCheckBox(tr("Smoothness"), this);
    showPS->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showPS);

    showPCO = new QCheckBox(tr("Pedal Center Offset"), this);
    showPCO->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showPCO);

    showDC = new QCheckBox(tr("Power Phase"), this);
    showDC->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showDC);

    showPPP = new QCheckBox(tr("Peak Power Phase"), this);
    showPPP->setCheckState(Qt::Unchecked);
    seriesRight->addRow(new QLabel(""), showPPP);

    // running !
    seriesRight->addRow(new QLabel(""), new QLabel(""));
    seriesRight->addRow(new QLabel(""), new QLabel(""));

    showRV = new QCheckBox(tr("Vertical Oscillation"), this);
    showRV->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(tr("Running")), showRV);

    showRGCT = new QCheckBox(tr("Ground Contact Time"), this);
    showRGCT->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(""), showRGCT);

    // trailing space to allow different termin for translation
    // of running and bike cadence
    showRCad = new QCheckBox(tr("Cadence "), this);
    showRCad->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(""), showRCad);

    seriesRight->addRow(new QLabel(""), new QLabel(""));

    showSmO2 = new QCheckBox(tr("SmO2"), this);
    showSmO2->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(tr("Moxy")), showSmO2);

    showtHb = new QCheckBox(tr("tHb"), this);
    showtHb->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(""), showtHb);

    showO2Hb = new QCheckBox(tr("O2Hb"), this);
    showO2Hb->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(""), showO2Hb);

    showHHb = new QCheckBox(tr("HHb"), this);
    showHHb->setCheckState(Qt::Checked);
    seriesRight->addRow(new QLabel(""), showHHb);

    // "standard"
    showHr = new QCheckBox(tr("Heart Rate"), this);
    showHr->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(tr("Data series")), showHr);

    showHRV= new QCheckBox(tr("R-R Rate"), this);
    showHRV->setCheckState(Qt::Unchecked);
    seriesLeft->addRow(new QLabel(), showHRV);

    showTcore = new QCheckBox(tr("Core Temperature"), this);
    showTcore->setCheckState(Qt::Unchecked); // don't show unless user insists
    seriesLeft->addRow(new QLabel(tr("")), showTcore);

    showSpeed = new QCheckBox(tr("Speed"), this);
    showSpeed->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(""), showSpeed);

    showCad = new QCheckBox(tr("Cadence"), this);
    showCad->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(""), showCad);

    showAlt = new QCheckBox(tr("Altitude"), this);
    showAlt->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(""), showAlt);

    showTemp = new QCheckBox(tr("Temperature"), this);
    showTemp->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(""), showTemp);

    showWind = new QCheckBox(tr("Headwind"), this);
    showWind->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(""), showWind);

    showTorque = new QCheckBox(tr("Torque"), this);
    showTorque->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(""), showTorque);

    showGear = new QCheckBox(tr("Gear Ratio"), this);
    showGear->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(""), showGear);

    showSlope = new QCheckBox(tr("Slope"), this);
    showSlope->setCheckState(Qt::Checked);
    seriesLeft->addRow(new QLabel(""), showSlope);

    seriesLeft->addRow(new QLabel(""), new QLabel(""));

    showAltSlope = new QComboBox(this);
    showAltSlope->addItem(tr("No Alt/Slope"));
    showAltSlope->addItem(tr("0.1km|mi -  1min"));
    showAltSlope->addItem(tr("0.5km|mi -  5min"));
    showAltSlope->addItem(tr("1.0km|mi - 10min"));
    seriesLeft->addRow(new QLabel(tr("Alt/Slope")), showAltSlope);
    showAltSlope->setCurrentIndex(0);

    seriesLeft->addRow(new QLabel(""), new QLabel(""));

    showANTISS = new QCheckBox(tr("Anaerobic TISS"), this);
    showANTISS->setCheckState(Qt::Unchecked);
    seriesLeft->addRow(new QLabel(tr("Metrics")), showANTISS);

    showATISS = new QCheckBox(tr("Aerobic TISS"), this);
    showATISS->setCheckState(Qt::Unchecked);
    seriesLeft->addRow(new QLabel(""), showATISS);

    showNP = new QCheckBox(tr("Iso Power"), this);
    showNP->setCheckState(Qt::Unchecked);
    seriesLeft->addRow(new QLabel(""), showNP);

    showXP = new QCheckBox(tr("Skiba xPower"), this);
    showXP->setCheckState(Qt::Unchecked);
    seriesLeft->addRow(new QLabel(""), showXP);

    showAP = new QCheckBox(tr("Altitude Power"), this);
    showAP->setCheckState(Qt::Unchecked);
    seriesLeft->addRow(new QLabel(""), showAP);

    showW = new QCheckBox(tr("W' balance"), this);
    showW->setCheckState(Qt::Unchecked);
    seriesLeft->addRow(new QLabel(""), showW);

    showPower = new QComboBox(this);
    showPower->addItem(tr("Power + shade"));
    showPower->addItem(tr("Power - shade"));
    showPower->addItem(tr("No Power"));
    mainControls->addRow(new QLabel(tr("Shading")), showPower);
    showPower->setCurrentIndex(0);

    comboDistance = new QComboBox(this);
    comboDistance->addItem(tr("Time"));
    comboDistance->addItem(tr("Distance"));
    comboDistance->addItem(tr("Time of day"));
    mainControls->addRow(new QLabel(tr("X Axis")), comboDistance);

    QLabel *smoothLabel = new QLabel(tr("Smooth"), this);
    smoothLineEdit = new QLineEdit(this);
    smoothLineEdit->setFixedWidth(40 *dpiXFactor);

    smoothSlider = new QSlider(Qt::Horizontal, this);
    smoothSlider->setTickPosition(QSlider::TicksBelow);
    smoothSlider->setTickInterval(10);
    smoothSlider->setMinimum(1);
    smoothSlider->setMaximum(600);
    smoothLineEdit->setValidator(new QIntValidator(smoothSlider->minimum(),
                                                   smoothSlider->maximum(),
                                                   smoothLineEdit));
    QHBoxLayout *smoothLayout = new QHBoxLayout;
    smoothLayout->addWidget(smoothLineEdit);
    smoothLayout->addWidget(smoothSlider);
    mainControls->addRow(smoothLabel, smoothLayout);

    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(GColor(CRIDEPLOTBACKGROUND)));

    allPlot = new AllPlot(this, this, context);
    allPlot->setContentsMargins(0,0,0,0);
    allPlot->enableAxis(QwtPlot::xBottom, true);
    allPlot->setAxisVisible(QwtPlot::xBottom, true);
    //allPlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //allPlot->axisWidget(QwtPlot::yLeft)->installEventFilter(this);

    allStack = new QStackedWidget(this);
    allStack->addWidget(allPlot);
    allStack->setCurrentIndex(0);

    HelpWhatsThis *help = new HelpWhatsThis(allPlot);
    allPlot->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Performance));

    // sort out default values
    smoothSlider->setValue(allPlot->smooth);
    smoothLineEdit->setText(QString("%1").arg(allPlot->smooth));
    rSmoothSlider->setValue(allPlot->smooth);
    rSmoothEdit->setText(QString("%1").arg(allPlot->smooth));

    allZoomer = new QwtPlotZoomer(allPlot->canvas());
    allZoomer->setRubberBand(QwtPicker::RectRubberBand);
    allZoomer->setRubberBandPen(GColor(CPLOTSELECT));
    allZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    allZoomer->setEnabled(true);

    // TODO: Hack for OS X one-button mouse
    // allZoomer->initMousePattern(1);

    // RightButton: zoom out by 1
    // Ctrl+RightButton: zoom out to full size
    allZoomer->setMousePattern(QwtEventPattern::MouseSelect1,
                               Qt::LeftButton, Qt::ShiftModifier);
    allZoomer->setMousePattern(QwtEventPattern::MouseSelect2,
                               Qt::RightButton, Qt::ControlModifier);
    allZoomer->setMousePattern(QwtEventPattern::MouseSelect3,
                               Qt::RightButton);

    allPanner = new QwtPlotPanner(allPlot->canvas());
    allPanner->setMouseButton(Qt::MidButton);

    // TODO: zoomer doesn't interact well with automatic axis resizing

    // tooltip on hover over point
    allPlot->tooltip = new LTMToolTip(QwtPlot::xBottom, QwtAxisId(QwtAxis::yLeft, 2).id,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOn,
                               allPlot->canvas(),
                               "");
    allPlot->tooltip->setRubberBand(QwtPicker::VLineRubberBand);
    allPlot->tooltip->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    allPlot->tooltip->setTrackerPen(QColor(Qt::black));
    QColor inv(Qt::white);
    inv.setAlpha(0);
    allPlot->tooltip->setRubberBandPen(inv);
    allPlot->tooltip->setEnabled(true);

    allPlot->_canvasPicker = new LTMCanvasPicker(allPlot);

    connect(context, SIGNAL(intervalHover(IntervalItem*)), allPlot, SLOT(intervalHover(IntervalItem*)));
    connect(allPlot->_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), allPlot, SLOT(pointHover(QwtPlotCurve*, int)));
    connect(allPlot->tooltip, SIGNAL(moved(const QPoint &)), this, SLOT(plotPickerMoved(const QPoint &)));
    connect(allPlot->tooltip, SIGNAL(appended(const QPoint &)), this, SLOT(plotPickerSelected(const QPoint &)));

    QwtPlotMarker* allMarker1 = new QwtPlotMarker();
    allMarker1->setLineStyle(QwtPlotMarker::VLine);
    allMarker1->attach(allPlot);
    allMarker1->setLabelAlignment(Qt::AlignTop|Qt::AlignRight);
    allPlot->standard->allMarker1=allMarker1;

    QwtPlotMarker* allMarker2 = new QwtPlotMarker();
    allMarker2->setLineStyle(QwtPlotMarker::VLine);
    allMarker2->attach(allPlot);
    allMarker2->setLabelAlignment(Qt::AlignTop|Qt::AlignRight);
    allPlot->standard->allMarker2=allMarker2;

    //
    // stack view
    //
    stackPlotLayout = new QVBoxLayout();
    stackPlotLayout->setSpacing(0);
    stackPlotLayout->setContentsMargins(0,0,0,0);

    stackWidget = new QWidget(this);
    stackWidget->setAutoFillBackground(false);
    stackWidget->setLayout(stackPlotLayout);
    stackWidget->setPalette(palette);

    stackFrame = new QScrollArea(this);
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    stackFrame->setStyle(cde);
#endif
    stackFrame->hide();
    stackFrame->setAutoFillBackground(false);
    stackFrame->setWidgetResizable(true);
    stackFrame->setWidget(stackWidget);
    stackFrame->setFrameStyle(QFrame::NoFrame);
    stackFrame->setContentsMargins(0,0,0,0);
    stackFrame->setPalette(palette);

    //
    // series stack view
    //
    seriesstackPlotLayout = new QVBoxLayout();
    seriesstackPlotLayout->setSpacing(0);
    seriesstackPlotLayout->setContentsMargins(0,0,0,0);
    seriesstackWidget = new QWidget(this);
    seriesstackWidget->setAutoFillBackground(true);
    seriesstackWidget->setLayout(seriesstackPlotLayout);
    seriesstackWidget->setPalette(palette);

    seriesstackFrame = new QScrollArea(this);
#ifdef Q_OS_WIN
    cde = QStyleFactory::create(OS_STYLE);
    seriesstackFrame->setStyle(cde);
#endif
    seriesstackFrame->hide();
    seriesstackFrame->setAutoFillBackground(false);
    seriesstackFrame->setWidgetResizable(true);
    seriesstackFrame->setWidget(seriesstackWidget);
    seriesstackFrame->setFrameStyle(QFrame::NoFrame);
    seriesstackFrame->setContentsMargins(0,0,0,0);
    seriesstackFrame->setPalette(palette);

    //
    // Compare - AllPlots stack
    //
    comparePlotLayout = new QVBoxLayout();
    comparePlotLayout->setSpacing(0);
    comparePlotLayout->setContentsMargins(0,0,0,0);
    comparePlotWidget = new QWidget(this);
    comparePlotWidget->setAutoFillBackground(false);
    comparePlotWidget->setLayout(comparePlotLayout);
    comparePlotWidget->setPalette(palette);

    comparePlotFrame = new QScrollArea(this);
#ifdef Q_OS_WIN
    cde = QStyleFactory::create(OS_STYLE);
    comparePlotWidget->setStyle(cde);
#endif
    comparePlotFrame->hide();
    comparePlotFrame->setAutoFillBackground(false);
    comparePlotFrame->setWidgetResizable(true);
    comparePlotFrame->setWidget(comparePlotWidget);
    comparePlotFrame->setFrameStyle(QFrame::NoFrame);
    comparePlotFrame->setContentsMargins(0,0,0,0);
    comparePlotFrame->setPalette(palette);

    allStack->addWidget(comparePlotFrame);

    //
    // allPlot view
    //
    allPlotLayout = new QVBoxLayout;
    allPlotLayout->setSpacing(0);
    allPlotLayout->setContentsMargins(0,0,0,0);
    allPlotFrame = new QScrollArea(this);
#ifdef Q_OS_WIN
    cde = QStyleFactory::create(OS_STYLE);
    allPlotFrame->setStyle(cde);
#endif
    allPlotFrame->setFrameStyle(QFrame::NoFrame);
    allPlotFrame->setAutoFillBackground(false);
    allPlotFrame->setContentsMargins(0,0,0,0);

    spanSlider = new QxtSpanSlider(Qt::Horizontal, this);
    spanSlider->setFocusPolicy(Qt::NoFocus);
    spanSlider->setHandleMovementMode(QxtSpanSlider::NoOverlapping);
    spanSlider->setLowerValue(0);
    spanSlider->setUpperValue(15);

    QFont smallFont;
    smallFont.setPointSize(6);

    scrollLeft = new QPushButton("<", this);
    scrollLeft->setFont(smallFont);
    scrollLeft->setAutoRepeat(true);
    scrollLeft->setFixedHeight(16*dpiYFactor);
    scrollLeft->setFixedWidth(16*dpiXFactor);
    scrollLeft->setContentsMargins(0,0,0,0);

    scrollRight = new QPushButton(">", this);
    scrollRight->setFont(smallFont);
    scrollRight->setAutoRepeat(true);
    scrollRight->setFixedHeight(16*dpiYFactor);
    scrollRight->setFixedWidth(16*dpiXFactor);
    scrollRight->setContentsMargins(0,0,0,0);

#ifdef Q_OS_MAC
    // BUG in QMacStyle and painting of spanSlider
    // so we use a plain style to avoid it, but only
    // on a MAC, since win and linux are fine
#if QT_VERSION > 0x5000
    QStyle *style = QStyleFactory::create("fusion");
#else
    QStyle *style = QStyleFactory::create("Cleanlooks");
#endif
    spanSlider->setStyle(style);
    scrollLeft->setStyle(style);
    scrollRight->setStyle(style);
#endif

    fullPlot = new AllPlot(this, this, context);
    fullPlot->standard->grid->enableY(false);
    fullPlot->setFixedHeight(100 *dpiYFactor);
    fullPlot->setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    fullPlot->setHighlightIntervals(false);
    fullPlot->setPaintBrush(0);
    static_cast<QwtPlotCanvas*>(fullPlot->canvas())->setBorderRadius(0);
    fullPlot->setWantAxis(false);
    fullPlot->setContentsMargins(0,0,0,0);

    HelpWhatsThis *helpFull = new HelpWhatsThis(fullPlot);
    fullPlot->setWhatsThis(helpFull->getWhatsThisText(HelpWhatsThis::ChartRides_Performance));

    intervalPlot = new AllPlotInterval(this, context);
    intervalPlot->setFixedHeight(100 *dpiYFactor);
    intervalPlot->setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    static_cast<QwtPlotCanvas*>(intervalPlot->canvas())->setBorderRadius(0);
    intervalPlot->setContentsMargins(0,0,0,0);

    connect(allPlot, SIGNAL(resized()), this, SLOT(allPlotResized()));

    // tooltip on hover over point
    /*intervalPlot->tooltip = new LTMToolTip(QwtPlot::xBottom, QwtAxis::yLeft,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOn,
                               intervalPlot->canvas(),
                               "");
    intervalPlot->tooltip->setRubberBand(QwtPicker::VLineRubberBand);
    intervalPlot->tooltip->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    intervalPlot->tooltip->setTrackerPen(QColor(Qt::black));
    intervalPlot->tooltip->setRubberBandPen(inv);
    intervalPlot->tooltip->setEnabled(true);*/


    // allPlotStack contains the allPlot and the stack by series
    // because both want the optional fullplot at the bottom
    allPlotStack = new QStackedWidget(this);
    allPlotStack->addWidget(allStack);
    allPlotStack->addWidget(seriesstackFrame);
    allPlotStack->setCurrentIndex(0);

    HelpWhatsThis *helpStack = new HelpWhatsThis(allPlotStack);
    allPlotStack->setWhatsThis(helpStack->getWhatsThisText(HelpWhatsThis::ChartRides_Performance));

    allPlotLayout->addWidget(allPlotStack);
    allPlotFrame->setLayout(allPlotLayout);

    allPlotLayout->addWidget(intervalPlot);


    // controls...
    controlsLayout = new QGridLayout;
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(5 *dpiXFactor,5 *dpiYFactor,5 *dpiXFactor,5 *dpiYFactor);
    controlsLayout->addWidget(fullPlot, 0,1);
    controlsLayout->addWidget(spanSlider, 1,1);
    controlsLayout->addWidget(scrollLeft,1,0);
    controlsLayout->addWidget(scrollRight,1,2);
    controlsLayout->setRowStretch(0, 10);
    controlsLayout->setRowStretch(1, 1);
    controlsLayout->setContentsMargins(0,0,0,0);
#ifdef Q_OS_MAC
    // macs  dpscing is weird
    //controlsLayout->setSpacing(5 *dpiXFactor);
#else
    controlsLayout->setSpacing(0);
#endif
    allPlotLayout->addLayout(controlsLayout);
    allPlotLayout->setStretch(0,100);
    allPlotLayout->setStretch(1,20);

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setContentsMargins(2 *dpiXFactor,0,2 *dpiXFactor,2 *dpiXFactor);
    vlayout->setSpacing(0);
    vlayout->addWidget(allPlotFrame);
    vlayout->addWidget(stackFrame);
    vlayout->setSpacing(1 *dpiXFactor);

    // put a helper on the screen for mouse over intervals...
    overlayIntervals = new IntervalSummaryWindow(context);
    addHelper(tr("Intervals"), overlayIntervals);

    //mainLayout->addLayout(vlayout,0,0);
    //mainLayout->addWidget(revealBackground,0,0, Qt::AlignTop);
    //mainLayout->addWidget(revealControls,0,0, Qt::AlignTop);
    //revealBackground->raise();
    //revealControls->raise();
    setChartLayout(vlayout);

    // common controls
    connect(showPower, SIGNAL(currentIndexChanged(int)), this, SLOT(setShowPower(int)));
    connect(showCad, SIGNAL(stateChanged(int)), this, SLOT(setShowCad(int)));
    connect(showTorque, SIGNAL(stateChanged(int)), this, SLOT(setShowTorque(int)));
    connect(showHr, SIGNAL(stateChanged(int)), this, SLOT(setShowHr(int)));
    connect(showHRV, SIGNAL(stateChanged(int)), this, SLOT(setShowHRV(int)));
    connect(showTcore, SIGNAL(stateChanged(int)), this, SLOT(setShowTcore(int)));
    connect(showPowerD, SIGNAL(stateChanged(int)), this, SLOT(setShowPowerD(int)));
    connect(showCadD, SIGNAL(stateChanged(int)), this, SLOT(setShowCadD(int)));
    connect(showTorqueD, SIGNAL(stateChanged(int)), this, SLOT(setShowTorqueD(int)));
    connect(showHrD, SIGNAL(stateChanged(int)), this, SLOT(setShowHrD(int)));
    connect(showNP, SIGNAL(stateChanged(int)), this, SLOT(setShowNP(int)));
    connect(showRV, SIGNAL(stateChanged(int)), this, SLOT(setShowRV(int)));
    connect(showRCad, SIGNAL(stateChanged(int)), this, SLOT(setShowRCad(int)));
    connect(showRGCT, SIGNAL(stateChanged(int)), this, SLOT(setShowRGCT(int)));
    connect(showGear, SIGNAL(stateChanged(int)), this, SLOT(setShowGear(int)));
    connect(showSmO2, SIGNAL(stateChanged(int)), this, SLOT(setShowSmO2(int)));
    connect(showtHb, SIGNAL(stateChanged(int)), this, SLOT(setShowtHb(int)));
    connect(showO2Hb, SIGNAL(stateChanged(int)), this, SLOT(setShowO2Hb(int)));
    connect(showHHb, SIGNAL(stateChanged(int)), this, SLOT(setShowHHb(int)));
    connect(showATISS, SIGNAL(stateChanged(int)), this, SLOT(setShowATISS(int)));
    connect(showANTISS, SIGNAL(stateChanged(int)), this, SLOT(setShowANTISS(int)));
    connect(showXP, SIGNAL(stateChanged(int)), this, SLOT(setShowXP(int)));
    connect(showAP, SIGNAL(stateChanged(int)), this, SLOT(setShowAP(int)));
    connect(showSpeed, SIGNAL(stateChanged(int)), this, SLOT(setShowSpeed(int)));
    connect(showAccel, SIGNAL(stateChanged(int)), this, SLOT(setShowAccel(int)));
    connect(showAlt, SIGNAL(stateChanged(int)), this, SLOT(setShowAlt(int)));
    connect(showSlope, SIGNAL(stateChanged(int)), this, SLOT(setShowSlope(int)));
    connect(showAltSlope, SIGNAL(currentIndexChanged(int)), this, SLOT(setShowAltSlope(int)));
    connect(showTemp, SIGNAL(stateChanged(int)), this, SLOT(setShowTemp(int)));
    connect(showWind, SIGNAL(stateChanged(int)), this, SLOT(setShowWind(int)));
    connect(showW, SIGNAL(stateChanged(int)), this, SLOT(setShowW(int)));
    connect(showBalance, SIGNAL(stateChanged(int)), this, SLOT(setShowBalance(int)));
    connect(showPS, SIGNAL(stateChanged(int)), this, SLOT(setShowPS(int)));
    connect(showTE, SIGNAL(stateChanged(int)), this, SLOT(setShowTE(int)));
    connect(showPCO, SIGNAL(stateChanged(int)), this, SLOT(setShowPCO(int)));
    connect(showDC, SIGNAL(stateChanged(int)), this, SLOT(setShowDC(int)));
    connect(showPPP, SIGNAL(stateChanged(int)), this, SLOT(setShowPPP(int)));

    connect(showGrid, SIGNAL(stateChanged(int)), this, SLOT(setShowGrid(int)));
    connect(showFull, SIGNAL(stateChanged(int)), this, SLOT(setShowFull(int)));
    connect(showInterval, SIGNAL(stateChanged(int)), this, SLOT(setShowInterval(int)));
    connect(showHelp, SIGNAL(stateChanged(int)), this, SLOT(setShowHelp(int)));
    connect(showStack, SIGNAL(stateChanged(int)), this, SLOT(showStackChanged(int)));
    connect(rStack, SIGNAL(stateChanged(int)), this, SLOT(showStackChanged(int)));
    connect(showBySeries, SIGNAL(stateChanged(int)), this, SLOT(showBySeriesChanged(int)));
    connect(rBySeries, SIGNAL(stateChanged(int)), this, SLOT(showBySeriesChanged(int)));
    connect(rFull, SIGNAL(stateChanged(int)), this, SLOT(setShowFull(int)));
    connect(rHelp, SIGNAL(stateChanged(int)), this, SLOT(setShowHelp(int)));
    connect(paintBrush, SIGNAL(stateChanged(int)), this, SLOT(setPaintBrush(int)));
    connect(comboDistance, SIGNAL(currentIndexChanged(int)), this, SLOT(setByDistance(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)), this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(editingFinished()), this, SLOT(setSmoothingFromLineEdit()));
    connect(rSmoothSlider, SIGNAL(valueChanged(int)), this, SLOT(setrSmoothingFromSlider()));
    connect(rSmoothEdit, SIGNAL(editingFinished()), this, SLOT(setrSmoothingFromLineEdit()));

    // normal view
    connect(spanSlider, SIGNAL(lowerPositionChanged(int)), this, SLOT(zoomChanged()));
    connect(spanSlider, SIGNAL(upperPositionChanged(int)), this, SLOT(zoomChanged()));

    // stacked view
    connect(scrollLeft, SIGNAL(clicked()), this, SLOT(moveLeft()));
    connect(scrollRight, SIGNAL(clicked()), this, SLOT(moveRight()));
    connect(stackZoomSlider, SIGNAL(valueChanged(int)), this, SLOT(stackZoomSliderChanged()));

    // GC signals
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(rideDirty(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(rideChanged(RideItem*)), this, SLOT(forceReplot()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context->athlete, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalZoom(IntervalItem*)), this, SLOT(zoomInterval(IntervalItem*)));
    connect(context, SIGNAL(zoomOut()), this, SLOT(zoomOut()));
    connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(rideDeleted(RideItem*)));

    // comparing things
    connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(compareChanged()));
    connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(compareChanged()));

    // set initial colors
    configChanged(CONFIG_APPEARANCE);
}

void
AllPlotWindow::configChanged(qint32 state)
{
    setUpdatesEnabled(false);

    setProperty("color", GColor(CRIDEPLOTBACKGROUND));

    // Container widgets should not paint
    // since they tend to use naff defaults and
    // 'complicate' or 'make busy' the general
    // look and feel
    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(GColor(CRIDEPLOTBACKGROUND)));
    setPalette(palette); // propagates to children

    // set style sheets
#ifndef Q_OS_MAC
    allPlotFrame->setStyleSheet(TabView::ourStyleSheet());
    comparePlotFrame->setStyleSheet(TabView::ourStyleSheet());
    seriesstackFrame->setStyleSheet(TabView::ourStyleSheet());
    stackFrame->setStyleSheet(TabView::ourStyleSheet());
    overlayIntervals->setStyleSheet(TabView::ourStyleSheet());
#endif

    // set palettes
    allStack->setPalette(palette);
    allPlotStack->setPalette(palette);
    allPlotFrame->setPalette(palette);
    allPlotFrame->viewport()->setPalette(palette); // its a scroll area

    comparePlotFrame->setPalette(palette);
    comparePlotWidget->setPalette(palette);
    seriesstackFrame->setPalette(palette);
    seriesstackWidget->setPalette(palette);
    stackFrame->setPalette(palette);
    stackWidget->setPalette(palette);
    stackZoomSlider->setPalette(palette);
    scrollLeft->setPalette(palette);
    scrollRight->setPalette(palette);


    fullPlot->setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    fullPlot->setPalette(palette);
    fullPlot->configChanged(state);
    fullPlot->update();

    intervalPlot->setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    intervalPlot->setPalette(palette);
    intervalPlot->configChanged(state);
    intervalPlot->update();

    // allPlot of course
    allPlot->setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    allPlot->setPalette(palette);
    allPlot->configChanged(state);
    allPlot->update();

    // and then the stacked plot
    foreach (AllPlot *plot, allPlots) {
        plot->setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
        plot->setPalette(palette);
        plot->configChanged(state);
        plot->update();
    }

    // and then the series plots
    foreach (AllPlot *plot, seriesPlots) {
        plot->setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
        plot->setPalette(palette);
        plot->configChanged(state);
        plot->update();
    }

    // and then the compaer plots
    foreach (AllPlot *plot, allComparePlots) {
        plot->setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
        plot->setPalette(palette);
        plot->configChanged(state);
        plot->update();
    }

    setUpdatesEnabled(true);

    // we're going to replot, but only if we're active
    // and all the other guff
    RideItem *ride = myRideItem;
    if (!amVisible()) {
        stale = true;
        return;
    }

    // ignore if null, or manual / empty
    if (!ride || !ride->ride() || !ride->ride()->dataPoints().count()) return;

    // reset the charts etc
    if (isCompare()) {

        compareChanged();

    } else {

        // ok replot with the new config!
        redrawFullPlot();
        redrawAllPlot();
        redrawStackPlot();
    }

    // just force a replot if (a) units changed (e.g. temp, speed etc are now in different units
    // or the wbal formula changed and we are actually plotting wbal !
    if (state & CONFIG_UNITS || (state & CONFIG_WBAL && showW->isChecked())) forceReplot();
}

QString
AllPlotWindow::getUserData() const
{
    QString returning;
    foreach(UserData *x, userDataSeries)
        returning += x->settings();

    return returning;
}

void
AllPlotWindow::setUserData(QString settings)
{
    // wipe whats there
    foreach(UserData *x, userDataSeries) delete x;
    userDataSeries.clear();

    // snip into discrete user data xml snippets
    QRegExp snippet("(\\<userdata .*\\<\\/userdata\\>)");
    snippet.setMinimal(true); // don't match too much
    QStringList snips;
    int pos = 0;

    while ((pos = snippet.indexIn(settings, pos)) != -1) {
        snips << snippet.cap(1);
        pos += snippet.matchedLength();
    }

    // now create and add each series
    foreach(QString set, snips)
        userDataSeries << new UserData(set);

    // fill with the current rideItem
    setRideForUserData();
    
    // tell full plot we have some and get allplot
    // to match too, since it sets from fullplot
    fullPlot->standard->setUserData(userDataSeries);
    allPlot->standard->setUserData(userDataSeries);

    // and update table
    refreshCustomTable();

    // replot
    forceReplot();
}

void 
AllPlotWindow::setRideForUserData()
{
    if (!current) return;

    // user data needs refreshing
    foreach(UserData *x, userDataSeries) x->setRideItem(current);
}

//
// configuring user data series
//
void
AllPlotWindow::refreshCustomTable(int indexSelectedItem)
{
    // clear then repopulate custom table settings to reflect
    // the current LTMSettings.
    customTable->clear();

    // get headers back
    QStringList header;
    header << tr("Name") << tr("Formula"); 
    customTable->setHorizontalHeaderLabels(header);

    QTableWidgetItem *selected = NULL;
    // now lets add a row for each metric
    customTable->setRowCount(userDataSeries.count());
    int i=0;
    foreach (UserData *x, userDataSeries) {

        QTableWidgetItem *t = new QTableWidgetItem();
        t->setText(x->name); // only metrics .. for now ..
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        customTable->setItem(i,0,t);

        t = new QTableWidgetItem();
        t->setText(x->formula);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        customTable->setItem(i,1,t);

        // keep the selected item from previous step (relevant for moving up/down)
        if (indexSelectedItem == i) {
            selected = t;
        }

        i++;
    }

    if (selected) {
      customTable->setCurrentItem(selected);
    }
}

void
AllPlotWindow::editUserData()
{
    QList<QTableWidgetItem*> items = customTable->selectedItems();
    if (items.count() < 1) return;

    int index = customTable->row(items.first());

    UserData edit(userDataSeries[index]->name,
                  userDataSeries[index]->units,
                  userDataSeries[index]->formula,
                  userDataSeries[index]->zstring,
                  userDataSeries[index]->color);

    EditUserDataDialog dialog(context, &edit);

    if (dialog.exec()) {

        // apply!
        userDataSeries[index]->formula = edit.formula;
        userDataSeries[index]->zstring = edit.zstring;
        userDataSeries[index]->name = edit.name;
        userDataSeries[index]->units = edit.units;
        userDataSeries[index]->color = edit.color;

        // update
        refreshCustomTable();

        // tell full plot we have some.
        fullPlot->standard->setUserData(userDataSeries);
        allPlot->standard->setUserData(userDataSeries);

        // replot
        forceReplot();

    }
}

void
AllPlotWindow::doubleClicked( int row, int )
{
    UserData edit(userDataSeries[row]->name,
                  userDataSeries[row]->units,
                  userDataSeries[row]->formula,
                  userDataSeries[row]->zstring,
                  userDataSeries[row]->color);

    EditUserDataDialog dialog(context, &edit);

    if (dialog.exec()) {

        // apply!
        userDataSeries[row]->formula = edit.formula;
        userDataSeries[row]->zstring = edit.zstring;
        userDataSeries[row]->name = edit.name;
        userDataSeries[row]->units = edit.units;
        userDataSeries[row]->color = edit.color;

        // update
        refreshCustomTable();

        // tell full plot we have some.
        fullPlot->standard->setUserData(userDataSeries);
        allPlot->standard->setUserData(userDataSeries);

        // replot
        forceReplot();
    }
}

void
AllPlotWindow::deleteUserData()
{
    QList<QTableWidgetItem*> items = customTable->selectedItems();
    if (items.count() < 1) return;
    
    int index = customTable->row(items.first());
    UserData *deleteme = userDataSeries[index];

    // wipe
    userDataSeries.removeAt(index);
    delete deleteme;

    // refresh
    refreshCustomTable();

    // tell full plot we have some.
    fullPlot->standard->setUserData(userDataSeries);
    allPlot->standard->setUserData(userDataSeries);

    // replot
    forceReplot();
}

void
AllPlotWindow::addUserData()
{
    UserData add;
    EditUserDataDialog dialog(context, &add);

    if (dialog.exec()) {
        // apply
        userDataSeries.append(new UserData(add.name, add.units, add.formula, add.zstring, add.color));

        // refresh
        refreshCustomTable();

        // tell full plot we have some.
        fullPlot->standard->setUserData(userDataSeries);
        allPlot->standard->setUserData(userDataSeries);

        // replot
        forceReplot();
    }
}

void
AllPlotWindow::moveUserDataUp()
{
    QList<QTableWidgetItem*> items = customTable->selectedItems();
    if (items.count() < 1) return;

    int index = customTable->row(items.first());

    if (index > 0) {
        userDataSeries.swap(index, index-1);
         // refresh
        refreshCustomTable(index-1);

        // tell full plot we have some.
        fullPlot->standard->setUserData(userDataSeries);
        allPlot->standard->setUserData(userDataSeries);

        // replot
        forceReplot();
    }
}

void
AllPlotWindow::moveUserDataDown()
{
    QList<QTableWidgetItem*> items = customTable->selectedItems();
    if (items.count() < 1) return;

    int index = customTable->row(items.first());

    if (index+1 <  userDataSeries.size()) {
        userDataSeries.swap(index, index+1);
         // refresh
        refreshCustomTable(index+1);

        // tell full plot we have some.
        fullPlot->standard->setUserData(userDataSeries);
        allPlot->standard->setUserData(userDataSeries);

        // replot
        forceReplot();
    }
}

bool
AllPlotWindow::event(QEvent *event)
{
    // nasty nasty nasty hack to move widgets as soon as the widget geometry
    // is set properly by the layout system, by default the width is 100 and 
    // we wait for it to be set properly then put our helper widget on the RHS
    if (event->type() == QEvent::Resize && geometry().width() != 100) {

        // put somewhere nice on first show
        if (firstShow) {
            firstShow = false;
            helperWidget()->move(mainWidget()->geometry().width()-(275*dpiXFactor), 50*dpiYFactor);
            helperWidget()->raise();

            if (isShowHelp()) helperWidget()->show();
            else helperWidget()->hide();
        }

        // if off the screen move on screen
        if (helperWidget()->geometry().x() > geometry().width()) {
            helperWidget()->move(mainWidget()->geometry().width()-(275*dpiXFactor), 50*dpiYFactor);
        }
    }
    return QWidget::event(event);
}

void
AllPlotWindow::compareChanged()
{
    if (!amVisible()) {
        if (context->isCompareIntervals) compareStale = true; // switched on
        else stale = true; // switched off
        return;
    }

    // we get busy so lets turn off updates till we're done
    setUpdatesEnabled(false);

    // clean up old
    foreach(AllPlotObject *p, compareIntervalCurves) delete p;
    compareIntervalCurves.clear();

    // clean away old compare user data
    foreach(QList<UserData*>user, compareUserDataSeries) {
        foreach(UserData *p, user)
            delete p;
    }
    compareUserDataSeries.clear();

    // new ones ..
    if (context->isCompareIntervals) {

        // generate the user data series for each interval
        foreach(CompareInterval ci, context->compareIntervals) {
            QList<UserData*> list;
            foreach(UserData *u, userDataSeries) {
                UserData *p = new UserData(u->name, u->units, u->formula, u->zstring, ci.color); // use context for interval
                p->setRideItem(ci.rideItem);
                list << p;
            }
            compareUserDataSeries << list;
        }

        // first, lets init fullPlot, just in case its never
        // been set (ie, switched to us before ever plotting a ride
        if (myRideItem) fullPlot->setDataFromRide(myRideItem, QList<UserData*>());

        // and even if the current ride is blank, we're not
        // going to be blank !!
        setIsBlank(false);

        //
        // SETUP INTERVALPLOT FOR COMPARE MODE
        //
        // No interval plot in compare mode yet
        intervalPlot->hide();

        //
        // SETUP FULLPLOT FOR COMPARE MODE
        // 
        double maxKM=0, maxSECS=0;

        fullPlot->standard->setVisible(false);
        if (fullPlot->smooth < 1) fullPlot->smooth = 1;
        int k=0;
        foreach(CompareInterval ci, context->compareIntervals) {

            AllPlotObject *po = new AllPlotObject(fullPlot, compareUserDataSeries[k]);
            if (ci.isChecked()) fullPlot->setDataFromRideFile(ci.data, po, compareUserDataSeries[k]);

            // what was the maximum x value?
            if (po->maxKM > maxKM) maxKM = po->maxKM;
            if (po->maxSECS > maxSECS) maxSECS = po->maxSECS;

            // prettify / hide unnecessary guff
            po->setColor(ci.color);
            po->hideUnwanted();

            // remember
            compareIntervalCurves << po;

            // next
            k++;
        }

        // what is the longest compareInterval?
        if (fullPlot->bydist) fullPlot->setAxisScale(QwtPlot::xBottom, 0, maxKM);
        else fullPlot->setAxisScale(QwtPlot::xBottom, 0, maxSECS/60);

        // now set it it in all the compare objects so they all get set
        // to the same time / duration and all the data is set too
        k=0;
        foreach (AllPlotObject *po, compareIntervalCurves) {
            po->maxKM = maxKM;
            po->maxSECS = maxSECS;
            k++;
        }

        if (fullPlot->bydist == false) {
            spanSlider->setMinimum(0);
            spanSlider->setMaximum(maxSECS);
        } else {
            spanSlider->setMinimum(0);
            spanSlider->setMaximum(maxKM * 1000);
        }
        spanSlider->setLowerValue(spanSlider->minimum());
        spanSlider->setUpperValue(spanSlider->maximum());

        // redraw for red/no-red title
        fullPlot->replot();

        //
        // SETUP ALLPLOT AS A STACK FOR EACH INTERVAL
        //
        allStack->setCurrentIndex(1);

        // first lets wipe the old ones
        foreach (AllPlot *ap, allComparePlots) delete ap;
        allComparePlots.clear();
        if (comparePlotLayout->count() == 1) {
            comparePlotLayout->takeAt(0); // remove the stretch
        }

        // now lets throw up one for each interval that
        // is checked
        int i=0;
        foreach(CompareInterval ci, context->compareIntervals) {

            // create a new one using the interval data object and
            // referencing fullPlot for the user prefs etc
            AllPlot *ap = new AllPlot(this, this, context);
            ap->bydist = fullPlot->bydist;
            ap->setShadeZones(showPower->currentIndex() == 0);

            // user data series needs setting up
            ap->setDataFromObject(compareIntervalCurves[i], fullPlot);

            // simpler to keep the indexes aligned
            if (!ci.isChecked()) ap->hide();

            // tooltip on hover over point -- consider moving this to AllPlot (!)
            ap->tooltip = new LTMToolTip(QwtPlot::xBottom, QwtAxisId(QwtAxis::yLeft, 2).id,
                                    QwtPicker::VLineRubberBand,
                                    QwtPicker::AlwaysOn,
                                    ap->canvas(),
                                    "");
            ap->tooltip->setRubberBand(QwtPicker::VLineRubberBand);
            ap->tooltip->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
            ap->tooltip->setTrackerPen(QColor(Qt::black));
            QColor inv(Qt::white);
            inv.setAlpha(0);
            ap->tooltip->setRubberBandPen(inv);
            ap->tooltip->setEnabled(true);

            ap->_canvasPicker = new LTMCanvasPicker(ap);
            connect(ap->_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), ap, SLOT(pointHover(QwtPlotCurve*, int)));

            // format it for our purposes
            if (fullPlot->bydist) ap->setAxisScale(QwtPlot::xBottom, 0, maxKM);
            else ap->setAxisScale(QwtPlot::xBottom, 0, maxSECS/60.00f);
            ap->setFixedHeight((120*dpiXFactor) + (stackWidth *4));

            // add to layout
            comparePlotLayout->addWidget(ap);
            allComparePlots << ap;

            i++;
        }
        comparePlotLayout->addStretch();

        //
        // SETUP STACK SERIES
        //

        // Wipe away whatever is there
        foreach(AllPlot *plot, seriesPlots) delete plot;
        seriesPlots.clear();
        // and the stretch
        while (seriesstackPlotLayout->count()) {
            delete seriesstackPlotLayout->takeAt(0);
        }

        // work out what we want to see
        QList<SeriesWanted> wanted;
        SeriesWanted s;
        if (showPower->currentIndex() < 2) { s.one = RideFile::watts; s.two = RideFile::none; wanted << s;};
        if (showW->isChecked()) { s.one = RideFile::wprime; s.two = RideFile::none; wanted << s;};
        if (showPowerD->isChecked()) { s.one = RideFile::wattsd; s.two = RideFile::none; wanted << s;};
        if (showHr->isChecked()) { s.one = RideFile::hr; s.two = RideFile::none; wanted << s;};
        if (showHRV->isChecked()) { s.one = RideFile::hrv; s.two = RideFile::none; wanted << s;};
        if (showTcore->isChecked()) { s.one = RideFile::tcore; s.two = RideFile::none; wanted << s;};
        if (showHrD->isChecked()) { s.one = RideFile::hrd; s.two = RideFile::none; wanted << s;};
        if (showSpeed->isChecked()) { s.one = RideFile::kph; s.two = RideFile::none; wanted << s;};
        if (showAccel->isChecked()) { s.one = RideFile::kphd; s.two = RideFile::none; wanted << s;};
        if (showCad->isChecked()) { s.one = RideFile::cad; s.two = RideFile::none; wanted << s;};
        if (showCadD->isChecked()) { s.one = RideFile::cadd; s.two = RideFile::none; wanted << s;};
        if (showTorque->isChecked()) { s.one = RideFile::nm; s.two = RideFile::none; wanted << s;};
        if (showTorqueD->isChecked()) { s.one = RideFile::nmd; s.two = RideFile::none; wanted << s;};
        if (showAlt->isChecked()) { s.one = RideFile::alt; s.two = RideFile::none; wanted << s;};
        if (showSlope->isChecked()) { s.one = RideFile::slope; s.two = RideFile::none; wanted << s;};
        if (showAltSlope->currentIndex() > 0) { s.one = RideFile::alt; s.two = RideFile::slope; wanted << s;}; // ALT/SLOPE !
        if (showTemp->isChecked()) { s.one = RideFile::temp; s.two = RideFile::none; wanted << s;};
        if (showWind->isChecked()) { s.one = RideFile::headwind; s.two = RideFile::none; wanted << s;};
        if (showNP->isChecked()) { s.one = RideFile::IsoPower; s.two = RideFile::none; wanted << s;};
        if (showRV->isChecked()) { s.one = RideFile::rvert; s.two = RideFile::none; wanted << s;};
        if (showRCad->isChecked()) { s.one = RideFile::rcad; s.two = RideFile::none; wanted << s;};
        if (showRGCT->isChecked()) { s.one = RideFile::rcontact; s.two = RideFile::none; wanted << s;};
        if (showGear->isChecked()) { s.one = RideFile::gear; s.two = RideFile::none; wanted << s;};
        if (showSmO2->isChecked()) { s.one = RideFile::smo2; s.two = RideFile::none; wanted << s;};
        if (showtHb->isChecked()) { s.one = RideFile::thb; s.two = RideFile::none; wanted << s;};
        if (showO2Hb->isChecked()) { s.one = RideFile::o2hb; s.two = RideFile::none; wanted << s;};
        if (showHHb->isChecked()) { s.one = RideFile::hhb; s.two = RideFile::none; wanted << s;};
        if (showATISS->isChecked()) { s.one = RideFile::aTISS; s.two = RideFile::none; wanted << s;};
        if (showANTISS->isChecked()) { s.one = RideFile::anTISS; s.two = RideFile::none; wanted << s;};
        if (showXP->isChecked()) { s.one = RideFile::xPower; s.two = RideFile::none; wanted << s;};
        if (showAP->isChecked()) { s.one = RideFile::aPower; s.two = RideFile::none; wanted << s;};
        if (showBalance->isChecked()) { s.one = RideFile::lrbalance; s.two = RideFile::none; wanted << s;};

        // and the user series
        for(int k=0; k<userDataSeries.count(); k++) {
            s.one = static_cast<RideFile::SeriesType>(RideFile::none + 1 + k);
            s.two = RideFile::none;
            wanted << s;
        }

        /*
        if (showTE->isChecked()) {
            s.one = RideFile::lte; s.two = RideFile::none; wanted << s;
            s.one = RideFile::rte; s.two = RideFile::none; wanted << s;
        }
        if (showPS->isChecked()) {
            s.one = RideFile::lps; s.two = RideFile::none; wanted << s;
            s.one = RideFile::rps; s.two = RideFile::none; wanted << s;
        }
        if (showPCO->isChecked()) {
            s.one = RideFile::lpco; s.two = RideFile::rpco; wanted << s;
        }
        if (showDC->isChecked()) {
            s.one = RideFile::lppb; s.two = RideFile::lppe; wanted << s;
            s.one = RideFile::rppb; s.two = RideFile::rppe; wanted << s;
        }
        if (showPPP->isChecked()) {
            s.one = RideFile::lpppb; s.two = RideFile::lpppe; wanted << s;
            s.one = RideFile::rpppb; s.two = RideFile::rpppe; wanted << s;
        }*/

        // create blank and add to gui
        QPalette palette;
        palette.setBrush(QPalette::Background, Qt::NoBrush);

        foreach(SeriesWanted x, wanted) {

            // create and setup with normal gui stuff
            AllPlot *plot = new AllPlot(this, this, context, x.one, x.two, false);
            plot->setPalette(palette);
            plot->setAutoFillBackground(false);
            plot->setFixedHeight((120*dpiYFactor)+(stackWidth*4));

            // tooltip on hover over point -- consider moving this to AllPlot (!)
            plot->tooltip = new LTMToolTip(QwtPlot::xBottom, QwtAxisId(QwtAxis::yLeft, 2).id,
                                    QwtPicker::VLineRubberBand,
                                    QwtPicker::AlwaysOn,
                                    plot->canvas(),
                                    "");
            plot->tooltip->setRubberBand(QwtPicker::VLineRubberBand);
            plot->tooltip->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
            plot->tooltip->setTrackerPen(QColor(Qt::black));
            QColor inv(Qt::white);
            inv.setAlpha(0);
            plot->tooltip->setRubberBandPen(inv);
            plot->tooltip->setEnabled(true);

            plot->_canvasPicker = new LTMCanvasPicker(plot);
            connect(plot->_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), plot, SLOT(pointHover(QwtPlotCurve*, int)));
            // No x axis titles
            plot->bydist = fullPlot->bydist;
            if (x.one == RideFile::watts) plot->setShadeZones(showPower->currentIndex() == 0);
            else plot->setShadeZones(false);
            plot->setAxisVisible(QwtPlot::xBottom, true);
            plot->enableAxis(QwtPlot::xBottom, true);
            plot->setAxisTitle(QwtPlot::xBottom,NULL);

            // get rid of the User Data axis
            for(int k=0; k<userDataSeries.count(); k++) 
                plot->setAxisVisible(QwtAxisId(QwtAxis::yRight,4 + k), false);

            // common y axis
            QwtScaleDraw *sd = new QwtScaleDraw;
            sd->setTickLength(QwtScaleDiv::MajorTick, 3);
            sd->enableComponent(QwtScaleDraw::Ticks, false);
            sd->enableComponent(QwtScaleDraw::Backbone, false);
            plot->setAxisScaleDraw(QwtPlot::yLeft, sd);
   
            // default paletter override below if needed 
            QPalette pal;
            pal.setColor(QPalette::WindowText, RideFile::colorFor(x.one));
            pal.setColor(QPalette::Text, RideFile::colorFor(x.one));

            // y-axis title and colour
            if (x.one == RideFile::alt && x.two == RideFile::slope) {

                // alt/slope special case
                plot->setAxisTitle(QwtPlot::yLeft, tr("Alt/Slope"));
                plot->showAltSlopeState = allPlot->showAltSlopeState;
                plot->setAltSlopePlotStyle(allPlot->standard->altSlopeCurve);

            } else {

                // user defined series
                if (x.one > RideFile::none) {
                    int index = (int)(x.one) - (RideFile::none + 1);
                    plot->setAxisTitle(QwtPlot::yLeft, userDataSeries[index]->name);
                    pal.setColor(QPalette::WindowText, userDataSeries[index]->color);
                    pal.setColor(QPalette::Text, userDataSeries[index]->color);
                } else {
                    // everything else
                    plot->setAxisTitle(QwtPlot::yLeft, RideFile::seriesName(x.one));
                }
            }

            plot->axisWidget(QwtPlot::yLeft)->setPalette(pal);

            // remember them
            seriesstackPlotLayout->addWidget(plot);
            seriesPlots << plot;
        }

        seriesstackPlotLayout->addStretch();

        // now lets get each of them loaded up with data and in the right format!
        foreach(AllPlot *compare, seriesPlots) {

            // update the one already there
            compare->standard->setVisible(false);
            compare->setDataFromPlots(allComparePlots);

            // format it for our purposes
            compare->bydist = fullPlot->bydist;
            if (fullPlot->bydist) compare->setAxisScale(QwtPlot::xBottom, 0, maxKM);
            else compare->setAxisScale(QwtPlot::xBottom, 0, maxSECS/60.00f);
            compare->setXTitle();

            compare->replot();
        }

        // now remove any series plots that are empty
        for(int i=0; i<seriesPlots.count();) {
            if (seriesPlots[i]->compares.count() == 0 || seriesPlots[i]->compares[0]->data()->size() == 0) {
                delete seriesPlots[i];
                seriesPlots.removeAt(i);
            } else {
                i++;
            }
        }

        // ok, we're done

    } else {

        if (showInterval->isChecked())
            intervalPlot->show();

        // reset to normal view?
        fullPlot->standard->setVisible(true);

        // back to allplot
        allStack->setCurrentIndex(0);

        // reset by just calling ride selected
        // and setting stale to true.. simpler this way
        stale = true;
        rideSelected();
    }

    // we're not stale anymore
    compareStale = false;

    // set the widgets straight
    setAllPlotWidgets(NULL);

    // Now we're done lets paint
    setUpdatesEnabled(true);
    repaint();

}

void
AllPlotWindow::redrawAllPlot()
{
    // refresh if shown or used as basis for stacked series (ie zooming off fullPlot)
    if (((showStack->isChecked() && showBySeries->isChecked()) || !showStack->isChecked())  && current) {

        RideItem *ride = current;

        int startidx, stopidx;
        if (fullPlot->bydist == true) {
            startidx =ride->ride()->distanceIndex((double)spanSlider->lowerValue()/(double)1000);
            stopidx = ride->ride()->distanceIndex((double)spanSlider->upperValue()/(double)1000);
        } else {
            startidx = ride->ride()->timeIndex(spanSlider->lowerValue());
            stopidx = ride->ride()->timeIndex(spanSlider->upperValue());
        }

        // need more than 1 sample to plot
        if (stopidx - startidx > 1) {
            allPlot->setDataFromPlot(fullPlot, startidx, stopidx);
            allZoomer->setZoomBase();
            //allPlot->setTitle("");
            allPlot->replot();
        }

    }
}

void
AllPlotWindow::redrawFullPlot()
{
    // always performed since the data is used
    // by both the stack plots and the allplot
    RideItem *ride = current;

    // null rides are possible on new cyclist
    if (!ride) return;

    // hide the usual plot decorations etc
    fullPlot->setShowPower(1);
    //We now use the window background color
    //fullPlot->setCanvasBackground(GColor(CPLOTTHUMBNAIL));
    static_cast<QwtPlotCanvas*>(fullPlot->canvas())->setBorderRadius(0);
    fullPlot->standard->grid->enableY(false);


    // use the ride to decide
    if (fullPlot->bydist)
        fullPlot->setAxisScale(QwtPlot::xBottom,
        ride->ride()->dataPoints().first()->km * (context->athlete->useMetricUnits ? 1 : MILES_PER_KM),
        ride->ride()->dataPoints().last()->km * (context->athlete->useMetricUnits ? 1 : MILES_PER_KM));
    else
        fullPlot->setAxisScale(QwtPlot::xBottom, ride->ride()->dataPoints().first()->secs/60,
                                                 ride->ride()->dataPoints().last()->secs/60);

    fullPlot->replot();
}

void
AllPlotWindow::redrawIntervalPlot()
{
    // always performed since the data is used
    // by both the stack plots and the allplot
    RideItem *ride = current;

    // null rides are possible on new cyclist
    if (!ride) return;

    static_cast<QwtPlotCanvas*>(intervalPlot->canvas())->setBorderRadius(0);

    // use the ride to decide
    if (intervalPlot->bydist)
        intervalPlot->setAxisScale(QwtPlot::xBottom,
        ride->ride()->dataPoints().first()->km * (context->athlete->useMetricUnits ? 1 : MILES_PER_KM),
        ride->ride()->dataPoints().last()->km * (context->athlete->useMetricUnits ? 1 : MILES_PER_KM));
    else
        intervalPlot->setAxisScale(QwtPlot::xBottom, ride->ride()->dataPoints().first()->secs/60,
                                                ride->ride()->dataPoints().last()->secs/60);

    intervalPlot->replot();
}

void
AllPlotWindow::redrawStackPlot()
{
    if (showStack->isChecked()) {

        // turn off display updates whilst we
        // do this, it takes a while and is prone
        // to screen flicker
        stackFrame->setUpdatesEnabled(false);


        // plot stack view ..
        if (!showBySeries->isChecked()) {

            // BY TIME / DISTANCE

            // remove current plots - then recreate
            resetStackedDatas();

            // now they are all set, lets replot
            foreach(AllPlot *plot, allPlots) plot->replot();

        } else {

            // BY SERIES
            resetSeriesStackedDatas();

            // now they are all set, lets replot
            foreach(AllPlot *plot, seriesPlots) plot->replot();
        }

        // we're done, go update the display now
        stackFrame->setUpdatesEnabled(true);
    }
}

void
AllPlotWindow::zoomChanged()
{
    if (isCompare()) {

        // zoom in all the compare plots
        foreach (AllPlot *plot, allComparePlots) {
            if (fullPlot->bydist) plot->setAxisScale(QwtPlot::xBottom, spanSlider->lowerValue()/1000.00f, spanSlider->upperValue()/1000.00f);
            else plot->setAxisScale(QwtPlot::xBottom, spanSlider->lowerValue() / 60.00f, spanSlider->upperValue() / 60.00f);

            plot->replot();
        }

        // and the series plots
        foreach (AllPlot *plot, seriesPlots) {
            if (fullPlot->bydist) plot->setAxisScale(QwtPlot::xBottom, spanSlider->lowerValue()/1000.00f, spanSlider->upperValue()/1000.00f);
            else plot->setAxisScale(QwtPlot::xBottom, spanSlider->lowerValue() / 60.00f, spanSlider->upperValue() / 60.00f);

            plot->replot();
        }


    } else {

        redrawAllPlot();
        redrawStackPlot(); // series stacks!
    }
}

void
AllPlotWindow::moveLeft()
{
    // move across by 5% of the span, or to zero if not much left
    int span = spanSlider->upperValue() - spanSlider->lowerValue();
    int delta = span / 20;
    if (delta > (spanSlider->lowerValue() - spanSlider->minimum()))
        delta = spanSlider->lowerValue() - spanSlider->minimum();

    spanSlider->setLowerValue(spanSlider->lowerValue()-delta);
    spanSlider->setUpperValue(spanSlider->upperValue()-delta);
    zoomChanged();
}

void
AllPlotWindow::moveRight()
{
    // move across by 5% of the span, or to zero if not much left
    int span = spanSlider->upperValue() - spanSlider->lowerValue();
    int delta = span / 20;
    if (delta > (spanSlider->maximum() - spanSlider->upperValue()))
        delta = spanSlider->maximum() - spanSlider->upperValue();

    spanSlider->setLowerValue(spanSlider->lowerValue()+delta);
    spanSlider->setUpperValue(spanSlider->upperValue()+delta);
    zoomChanged();
}

void
AllPlotWindow::forceReplot()
{
    stale=true;
    rideSelected();
}

void
AllPlotWindow::rideSelected()
{
    // compare mode ignores ride selection
    if (isVisible() && isCompare()) {
        if (compareStale) compareChanged();
        return;
    }

    RideItem *ride = myRideItem;

    if (ride == NULL) current = NULL;

    // ignore if not active
    if (!amVisible()) {
        stale = true;
        setupSeriesStack = setupStack = false;
        return;
    }

    // ignore if null, or manual / empty
    if (!ride || !ride->ride() || !ride->ride()->dataPoints().count()) {
        current = NULL;
        setIsBlank(true);
        return;
    }
    else
        setIsBlank(false);

    // we already plotted it!
    //XXX the !ride->isDirty() code below makes GC crash when selecting
    //XXX on the canvas ?!??!? No idea why, but commenting out for now
    //XXX if (!ride->isDirty() && ride == current && stale == false) return;
    if (ride == current && stale == false) return;

    // ok, its now the current ride
    current = ride;

    // setup user data if needed
    setRideForUserData();

    // clear any previous selections
    clearSelection();

    // setup the control widgets, dependant on
    // data present in this ride, needs to happen
    // before we set the plots below...
    setAllPlotWidgets(ride);

    // setup the charts to reflect current ride selection
    fullPlot->setDataFromRide(ride, userDataSeries);
    intervalPlot->setDataFromRide(ride);

    // redraw all the plots, they will check
    // to see if they are currently visible
    // and only redraw if neccessary
    redrawFullPlot();
    redrawAllPlot();
    redrawIntervalPlot();

    // we need to reset the stacks as the ride has changed
    // but it may ignore if not in stacked mode.
    setupSeriesStack = setupStack = false;

    setupStackPlots();
    setupSeriesStackPlots();

    stale = false;
}

void
AllPlotWindow::rideDeleted(RideItem *ride)
{
    if (ride == myRideItem) {
        // we have nothing to show
        setProperty("ride", QVariant::fromValue<RideItem*>(NULL));

        // notify all the plots, because when zones are redrawn
        // they will try and reference AllPlot::rideItem
        if (!isCompare()) {
            setAllPlotWidgets(NULL);
            fullPlot->setDataFromRide(NULL,QList<UserData*>());
        }
    }
}

void
AllPlotWindow::zonesChanged()
{
    if (!amVisible()) {
        stale = true;
        return;
    }

    allPlot->refreshZoneLabels();
    allPlot->replot();
}

void
AllPlotWindow::intervalsChanged()
{
    if (!amVisible()) {
        stale = true;
        return;
    }

    // show selection on fullplot too
    fullPlot->refreshIntervalMarkers();
    fullPlot->replot();

    intervalPlot->refreshIntervals();
    intervalPlot->replot();

    // allPlot of course
    allPlot->refreshIntervalMarkers();
    allPlot->replot();

    // and then the stacked plot
    foreach (AllPlot *plot, allPlots) {
        plot->refreshIntervalMarkers();
        plot->replot();
    }

    // and then the series plots
    foreach (AllPlot *plot, seriesPlots) {
        plot->refreshIntervalMarkers();
        plot->replot();
    }

}

void
AllPlotWindow::intervalSelected()
{
    if (isCompare() || !amVisible()) {
        stale = true;
        return;
    }

    if (active == true) return;

    // the intervals are highlighted
    // in the plot automatically, we just
    // need to replot, depending upon
    // which mode we are in...
    hideSelection();
    if (showStack->isChecked()) {

        if (showBySeries->isChecked()) {
            foreach (AllPlot *plot, seriesPlots)
                plot->replot();
        } else {
            foreach (AllPlot *plot, allPlots)
                plot->replot();
        }
    } else {
        fullPlot->replot();
        allPlot->replot();
    }
}

void
AllPlotWindow::setStacked(int value)
{
    showStack->setChecked(value);
    rStack->setChecked(value);

    // enables / disable as needed
    if (showStack->isChecked()) {
        showBySeries->setEnabled(true);
        rBySeries->setEnabled(true);
    } else {
        showBySeries->setEnabled(false);
        rBySeries->setEnabled(false);
    }
}

void
AllPlotWindow::setBySeries(int value)
{
    showBySeries->setChecked(value);
    rBySeries->setChecked(value);
}

void
AllPlotWindow::setSmoothingFromSlider()
{
    // active tells us we have been triggered by
    // the setSmoothingFromLineEdit which will also
    // recalculates smoothing, lets not double up...
    if (active) return;
    else active = true;

    if (allPlot->smooth != smoothSlider->value()) {
        setSmoothing(smoothSlider->value());
        smoothLineEdit->setText(QString("%1").arg(fullPlot->smooth));
        rSmoothEdit->setText(QString("%1").arg(fullPlot->smooth));
        rSmoothSlider->setValue(rSmoothSlider->value());
    }
    active = false;
}

void
AllPlotWindow::setrSmoothingFromSlider()
{
    // active tells us we have been triggered by
    // the setSmoothingFromLineEdit which will also
    // recalculates smoothing, lets not double up...
    if (active) return;
    else active = true;

    if (allPlot->smooth != rSmoothSlider->value()) {
        setSmoothing(rSmoothSlider->value());
        rSmoothEdit->setText(QString("%1").arg(fullPlot->smooth));
        smoothSlider->setValue(rSmoothSlider->value());
        smoothLineEdit->setText(QString("%1").arg(fullPlot->smooth));
    }
    active = false;
}

void
AllPlotWindow::setSmoothingFromLineEdit()
{
    // active tells us we have been triggered by
    // the setSmoothingFromSlider which will also
    // recalculates smoothing, lets not double up...
    if (active) return;
    else active = true;

    int value = smoothLineEdit->text().toInt();
    if (value != fullPlot->smooth) {
        smoothSlider->setValue(value);
        rSmoothSlider->setValue(value);
        setSmoothing(value);
    }
    active = false;
}

void
AllPlotWindow::setrSmoothingFromLineEdit()
{
    // active tells us we have been triggered by
    // the setSmoothingFromSlider which will also
    // recalculates smoothing, lets not double up...
    if (active) return;
    else active = true;

    int value = rSmoothEdit->text().toInt();
    if (value != fullPlot->smooth) {
        rSmoothSlider->setValue(value);
        smoothSlider->setValue(value);
        setSmoothing(value);
    }
    active = false;
}

void
AllPlotWindow::setAllPlotWidgets(RideItem *ride)
{
    // this routine sets up the display widgets
    // depending upon what data is available in the
    // ride. It also hides/shows widgets depending
    // upon wether we are in 'normal' mode or
    // stacked plot mode
    if (!isCompare()) {

        // set for currently selected ride
        if (!ride) return;

        // checkboxes to show/hide specific data series...
        const RideFileDataPresent *dataPresent = ride->ride()->areDataPresent();
        if (ride->ride() && ride->ride()->deviceType() != QString("Manual CSV")) {

            showPowerD->setEnabled(dataPresent->watts);
            showCadD->setEnabled(dataPresent->cad);
            showTorqueD->setEnabled(dataPresent->nm);
            showHrD->setEnabled(dataPresent->hr);
            showPower->setEnabled(dataPresent->watts);
            showCad->setEnabled(dataPresent->cad);
            showTorque->setEnabled(dataPresent->nm);
            showHr->setEnabled(dataPresent->hr);
            showHRV->setEnabled(dataPresent->hrv);
            showTcore->setEnabled(dataPresent->hr);
            showSpeed->setEnabled(dataPresent->kph);
            showAccel->setEnabled(dataPresent->kph);
            showAlt->setEnabled(dataPresent->alt);
            showAltSlope->setEnabled(dataPresent->alt);
            showSlope->setEnabled(dataPresent->slope);
            showTemp->setEnabled(dataPresent->temp);
            showWind->setEnabled(dataPresent->headwind);
            showBalance->setEnabled(dataPresent->lrbalance);
        } else {
            showAccel->setEnabled(false);
            showPowerD->setEnabled(false);
            showCadD->setEnabled(false);
            showTorqueD->setEnabled(false);
            showHrD->setEnabled(false);
            showPower->setEnabled(false);
            showHr->setEnabled(false);
            showHRV->setEnabled(false);
            showTcore->setEnabled(false);
            showSpeed->setEnabled(false);
            showCad->setEnabled(false);
            showAlt->setEnabled(false);
            showAltSlope->setEnabled(false);
            showSlope->setEnabled(false);
            showTemp->setEnabled(false);
            showWind->setEnabled(false);
            showTorque->setEnabled(false);
            showBalance->setEnabled(false);
        }

        // turn on/off shading, if it's not available
        bool shade;
        if (dataPresent->watts) shade = (showPower->currentIndex() == 0);
        else shade = false;
        allPlot->setShadeZones(shade);
        foreach (AllPlot *plot, allPlots) plot->setShadeZones(shade);
        allPlot->setShowGrid(showGrid->checkState() == Qt::Checked);
        foreach (AllPlot *plot, allPlots) plot->setShowGrid(showGrid->checkState() == Qt::Checked);

        // set the SpanSlider for the ride length, by default
        // show the entire ride (the user can adjust later)
        if (fullPlot->bydist == false) {
            spanSlider->setMinimum(ride->ride()->dataPoints().first()->secs);
            spanSlider->setMaximum(ride->ride()->dataPoints().last()->secs);
            spanSlider->setLowerValue(spanSlider->minimum());
            spanSlider->setUpperValue(spanSlider->maximum());
        } else {
            spanSlider->setMinimum(ride->ride()->dataPoints().first()->km * 1000);
            spanSlider->setMaximum(ride->ride()->dataPoints().last()->km * 1000);
            spanSlider->setLowerValue(spanSlider->minimum());
            spanSlider->setUpperValue(spanSlider->maximum());
        }
    }

    // now set the visible plots, depending upon whether
    // we are in stacked mode or not
    if (!isCompare() && showStack->isChecked() && !showBySeries->isChecked()) {

        // ALL PLOT STACK ORIGINAL AS CODED BY DAMIEN

        // hide normal view
        allPlotFrame->hide();
        allPlot->hide();
        fullPlot->hide();
        controlsLayout->setRowStretch(0, 0);
        controlsLayout->setRowStretch(1, 0);

        // show stacked view
        stackFrame->show();

    } else {

        // hide stack plots (segmented by time/distance)
        stackFrame->hide();

        // show normal view
        allPlotFrame->show();
        allPlot->show();

        if (showStack->isChecked() && (isCompare() || showBySeries->isChecked())) {

            // STACK SERIES LANES SHOWING
            allPlotStack->setCurrentIndex(1);

        } else {

            // ALLPLOT 
            allPlotStack->setCurrentIndex(0);

            if (isCompare()) {

                // COMPARE ALL PLOTS
                allStack->setCurrentIndex(1); // compare mode stack of all plots

            } else { 

                // NORMAL SINGLE ALL PLOT
                allStack->setCurrentIndex(0); // normal single allplot
            }

        }

        // FIXUP FULL PLOT!
        if (showFull->isChecked()) {
            fullPlot->show();
            controlsLayout->setRowStretch(0, 100);
            controlsLayout->setRowStretch(1, 20);
        } else {
            fullPlot->hide();

            if (isCompare()) {
                controlsLayout->setRowStretch(0, 100);
                controlsLayout->setRowStretch(1, 00);
            } else {
                controlsLayout->setRowStretch(0, 100);
                controlsLayout->setRowStretch(1, 00);
            }
        }
    }
}

void
AllPlotWindow::zoomOut()
{
    // we don't do that in compare mode
    if (isCompare()) return;

    // set them to maximums to avoid overlapping
    // when we set them below, daft but works
    spanSlider->setLowerValue(spanSlider->minimum());
    spanSlider->setUpperValue(spanSlider->maximum());
    zoomChanged();
}

void
AllPlotWindow::zoomInterval(IntervalItem *which)
{
    // we don't do that in compare mode
    if (isCompare()) return;

    // use the span slider to highlight
    // when we are in normal mode.

    // set them to maximums to avoid overlapping
    // when we set them below, daft but works
    spanSlider->setLowerValue(spanSlider->minimum());
    spanSlider->setUpperValue(spanSlider->maximum());

    if (!allPlot->bydist) {
        spanSlider->setLowerValue(which->start);
        spanSlider->setUpperValue(which->stop);
    } else {
        spanSlider->setLowerValue(which->startKM * (double)1000);
        spanSlider->setUpperValue(which->stopKM * (double)1000);
    }
    zoomChanged();
}

void
AllPlotWindow::plotPickerSelected(const QPoint &pos)
{
    // we don't do selection in compare mode
    if (isCompare()) return;

    QwtPlotPicker* pick = qobject_cast<QwtPlotPicker *>(sender());
    AllPlot* plot = qobject_cast<AllPlot *>(pick->plot());
    double xValue = plot->invTransform(QwtPlot::xBottom, pos.x());

    setStartSelection(plot, xValue);
}

void
AllPlotWindow::plotPickerMoved(const QPoint &pos)
{
    // we don't do selection in compare mode
    if (isCompare()) return;

    QString name = QString(tr("Selection #%1 ")).arg(selection);

    // which picker and plot send this signal?
    QwtPlotPicker* pick = qobject_cast<QwtPlotPicker *>(sender());
    AllPlot* plot = qobject_cast<AllPlot *>(pick->plot());

    int posX = pos.x();
    int posY = plot->y()+pos.y()+(50*dpiYFactor);

    if (showStack->isChecked()) {

        // need to highlight across stacked plots
        foreach (AllPlot *_plot, (showBySeries->isChecked() ? seriesPlots : allPlots)) {

            // mark the start of selection on every plot
            _plot->standard->allMarker1->setValue(plot->standard->allMarker1->value());

            if (_plot->y()<=plot->y() && posY<_plot->y()){

                if (_plot->transform(QwtPlot::xBottom, _plot->standard->allMarker2->xValue())>0) {
                    setEndSelection(_plot, 0, false, name);
                    _plot->standard->allMarker2->setLabel(QString(""));
                }

            } else if (_plot->y()>=plot->y() && posY>_plot->y()+_plot->height()) {

                if (_plot->transform(QwtPlot::xBottom, _plot->standard->allMarker2->xValue())<plot->width()){
                    setEndSelection(_plot, _plot->transform(QwtPlot::xBottom, plot->width()), false, name);
                }
            }
            else if (posY>_plot->y() && posY<_plot->y()+_plot->height()) {

                if (pos.x()<6) {
                    posX = 6;
                } else if (!_plot->bydist && pos.x()>_plot->transform(QwtPlot::xBottom,
                                        fullPlot->standard->timeArray[fullPlot->standard->timeArray.size()-1])) {
                    posX = _plot->transform(QwtPlot::xBottom, fullPlot->standard->timeArray[fullPlot->standard->timeArray.size()-1]);
                } else if (plot->bydist && pos.x()>_plot->transform(QwtPlot::xBottom,
                                        fullPlot->standard->distanceArray[fullPlot->standard->distanceArray.size()-1])) {
                    posX = fullPlot->transform(QwtPlot::xBottom,
                                        fullPlot->standard->distanceArray[fullPlot->standard->distanceArray.size()-1]);
                }

                setEndSelection(_plot, _plot->invTransform(QwtPlot::xBottom, posX), true, name);

                if (plot->y()<_plot->y()) {
                    plot->standard->allMarker1->setLabel(_plot->standard->allMarker1->label());
                    plot->standard->allMarker2->setLabel(_plot->standard->allMarker2->label());
                }

            } else {

                _plot->standard->allMarker1->hide();
                _plot->standard->allMarker2->hide();

            }
        }

    } else {

        // working on AllPlot
        double xValue = plot->invTransform(QwtPlot::xBottom, pos.x());
        setEndSelection(plot, xValue, true, name);
    }
}

void
AllPlotWindow::setStartSelection(AllPlot* plot, double xValue)
{
    // we don't do selection in compare mode
    if (isCompare()) return;

    QwtPlotMarker* allMarker1 = plot->standard->allMarker1;

    selection++;

    if (!allMarker1->isVisible() || allMarker1->xValue() != xValue) {
        allMarker1->setValue(xValue, plot->bydist ? 0 : 100);
        allMarker1->show();
    }
}

void
AllPlotWindow::setEndSelection(AllPlot* plot, double xValue, bool newInterval, QString name)
{
    // we don't do selection in compare mode
    if (isCompare()) return;

    active = true;

    QwtPlotMarker* allMarker1 = plot->standard->allMarker1;
    QwtPlotMarker* allMarker2 = plot->standard->allMarker2;

    if (!allMarker2->isVisible() || allMarker2->xValue() != xValue) {

        allMarker2->setValue(xValue, plot->bydist ? 0 : 100);
        allMarker2->show();

        double x1, x2;  // time or distance depending upon mode selected

        // swap to low-high if neccessary
        if (allMarker1->xValue()>allMarker2->xValue()){
            x2 = allMarker1->xValue();
            x1 = allMarker2->xValue();
        } else {
            x1 = allMarker1->xValue();
            x2 = allMarker2->xValue();
        }

        RideItem *ride = current;

        double distance1 = -1;
        double distance2 = -1;
        double duration1 = -1;
        double duration2 = -1;

        // if we are in distance mode then x1 and x2 are distances
        // we need to make sure they are in KM for the rest of this
        // code.
        if (plot->bydist) {

            if (context->athlete->useMetricUnits == false) {
                // convert to metric
                x1 *= KM_PER_MILE;
                x2 *=  KM_PER_MILE;
            }

            // distance  to time
            distance1 = x1;
            distance2 = x2;
            duration1 = ride->ride()->distanceToTime(x1);
            duration2 = ride->ride()->distanceToTime(x2);

        } else {

            // convert to seconds from minutes
            x1 *=60;
            x2 *=60;

            // time to distance
            duration1 = x1;
            duration2 = x2;
            distance1 = ride->ride()->timeToDistance(x1);
            distance2 = ride->ride()->timeToDistance(x2);
        }

        if (newInterval) {

            // what we go already ?
            QList<IntervalItem*> users = ride->intervals(RideFileInterval::USER);

            // are we adjusting an existing interval? - if so delete it and readd it
            if (users.count() > 0 && users.last()->name.startsWith(name)) {

                // update interval - could do via a IntervalItem::setXX() function
                IntervalItem *interval = users.last();
                interval->setValues(interval->name, duration1, duration2, distance1, distance2, interval->color, false);
                interval->selected = true;

            } else {

                //create a new one
                IntervalItem *interval = ride->newInterval(name, duration1, duration2, distance1, distance2, Qt::black, false);
                interval->selected = true;
                ride->

                // rebuild list
                context->notifyIntervalsUpdate(ride);
            }

            // charts need to update
            context->notifyIntervalsChanged();
        }
    }
    active = false;
}

void
AllPlotWindow::clearSelection()
{
    // we don't do selection in compare mode
    if (isCompare()) return;

    selection = 0;
    allPlot->standard->allMarker1->setVisible(false);
    allPlot->standard->allMarker2->setVisible(false);

    foreach (AllPlot *plot, allPlots) {
        plot->standard->allMarker1->setVisible(false);
        plot->standard->allMarker2->setVisible(false);
    }
}

void
AllPlotWindow::hideSelection()
{
    // we don't do selection in compare mode
    if (isCompare()) return;

    if (showStack->isChecked()) {
        foreach (AllPlot *plot, allPlots) {
            plot->standard->allMarker1->setVisible(false);
            plot->standard->allMarker2->setVisible(false);
            plot->replot();
        }
    } else {
        fullPlot->replot();
        allPlot->standard->allMarker1->setVisible(false);
        allPlot->standard->allMarker2->setVisible(false);
        allPlot->replot();
    }
}

void
AllPlotWindow::setHovering(int value)
{
    showHover->setChecked(value);
}

void
AllPlotWindow::setShowPower(int value)
{
    showPower->setCurrentIndex(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    // we only show the power shading on the
    // allPlot / stack plots, not on the fullPlot
    allPlot->setShowPower(value);
    if (!showStack->isChecked())
        allPlot->replot();

    // redraw
    foreach (AllPlot *plot, allPlots)
        plot->setShowPower(value);

    // now replot 'em - to avoid flicker
    if (showStack->isChecked()) {
        stackFrame->setUpdatesEnabled(false); // don't repaint whilst we do this...
        foreach (AllPlot *plot, allPlots)
            plot->replot();
        stackFrame->setUpdatesEnabled(true); // don't repaint whilst we do this...
    }

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}


void
AllPlotWindow::setShowHr(int value)
{
    showHr->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showHr->isEnabled()) ? true : false;

    allPlot->setShowHr(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowHr(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowTcore(int value)
{
    showTcore->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showTcore->isEnabled()) ? true : false;
    allPlot->setShowTcore(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowTcore(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowHRV(int value)
{
    showHRV->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showHRV->isEnabled()) ? true : false;

    allPlot->setShowHRV(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowHRV(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}


void
AllPlotWindow::setShowNP(int value)
{
    showNP->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showNP->isEnabled()) ? true : false;

    // recalc only does it if it needs to
    if (value && current && current->ride()) current->ride()->recalculateDerivedSeries();

    allPlot->setShowNP(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowNP(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowANTISS(int value)
{
    showANTISS->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showANTISS->isEnabled()) ? true : false;

    // recalc only does it if it needs to
    if (value && current && current->ride()) current->ride()->recalculateDerivedSeries();

    allPlot->setShowANTISS(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowANTISS(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowATISS(int value)
{
    showATISS->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showATISS->isEnabled()) ? true : false;

    // recalc only does it if it needs to
    if (value && current && current->ride()) current->ride()->recalculateDerivedSeries();

    allPlot->setShowATISS(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowATISS(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowXP(int value)
{
    showXP->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showXP->isEnabled()) ? true : false;

    // recalc only does it if it needs to
    if (value && current && current->ride()) current->ride()->recalculateDerivedSeries();

    allPlot->setShowXP(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowXP(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowAP(int value)
{
    showAP->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showAP->isEnabled()) ? true : false;

    // recalc only does it if it needs to
    if (value && current && current->ride()) current->ride()->recalculateDerivedSeries();

    allPlot->setShowAP(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowAP(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowSpeed(int value)
{
    showSpeed->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showSpeed->isEnabled()) ? true : false;

    allPlot->setShowSpeed(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowSpeed(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowAccel(int value)
{
    showAccel->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showAccel->isEnabled()) ? true : false;

    allPlot->setShowAccel(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowAccel(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowPowerD(int value)
{
    showPowerD->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showPowerD->isEnabled()) ? true : false;

    allPlot->setShowPowerD(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowPowerD(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowCadD(int value)
{
    showCadD->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showCadD->isEnabled()) ? true : false;

    allPlot->setShowCadD(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowCadD(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowTorqueD(int value)
{
    showTorqueD->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showTorqueD->isEnabled()) ? true : false;

    allPlot->setShowTorqueD(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowTorqueD(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowHrD(int value)
{
    showHrD->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showHrD->isEnabled()) ? true : false;

    allPlot->setShowHrD(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowHrD(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowCad(int value)
{
    showCad->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showCad->isEnabled()) ? true : false;

    allPlot->setShowCad(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowCad(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowAlt(int value)
{
    showAlt->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showAlt->isEnabled()) ? true : false;

    allPlot->setShowAlt(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowAlt(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowSlope(int value)
{
    showSlope->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showSlope->isEnabled()) ? true : false;

    allPlot->setShowSlope (checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowSlope(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowAltSlope(int value)
{
    showAltSlope->setCurrentIndex(value);

    // compare mode selfcontained update
    if (isCompare()) {
       allPlot->showAltSlopeState = value;
       compareChanged();
       return;
    }

    allPlot->setShowAltSlope(value);

    foreach (AllPlot *plot, allPlots)
        plot->setShowAltSlope(value);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}



void
AllPlotWindow::setShowTemp(int value)
{
    showTemp->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showTemp->isEnabled()) ? true : false;

    allPlot->setShowTemp(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowTemp(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowRV(int value)
{
    showRV->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showRV->isEnabled()) ? true : false;

    allPlot->setShowRV(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowRV(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowRGCT(int value)
{
    showRGCT->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showRGCT->isEnabled()) ? true : false;

    allPlot->setShowRGCT(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowRGCT(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowRCad(int value)
{
    showRCad->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showRCad->isEnabled()) ? true : false;

    allPlot->setShowRCad(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowRCad(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowSmO2(int value)
{
    showSmO2->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showSmO2->isEnabled()) ? true : false;

    allPlot->setShowSmO2(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowSmO2(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowtHb(int value)
{
    showtHb->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showtHb->isEnabled()) ? true : false;

    allPlot->setShowtHb(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowtHb(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowO2Hb(int value)
{
    showO2Hb->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showO2Hb->isEnabled()) ? true : false;

    allPlot->setShowO2Hb(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowO2Hb(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowHHb(int value)
{
    showHHb->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showHHb->isEnabled()) ? true : false;

    allPlot->setShowHHb(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowHHb(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowGear(int value)
{
    showGear->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showGear->isEnabled()) ? true : false;

    allPlot->setShowGear(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowGear(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowWind(int value)
{
    showWind->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = (( value == Qt::Checked ) && showWind->isEnabled()) ? true : false;

    allPlot->setShowWind(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowWind(checked);

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowW(int value)
{
    showW->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showW->isEnabled()) ? true : false;

    allPlot->setShowW(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowW(checked);

    // refresh W' data if needed
    if (checked && current && current->ride()) {

        // redraw
        redrawFullPlot();
        redrawAllPlot();
        redrawStackPlot();
    }

    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowTorque(int value)
{
    showTorque->setChecked(value);
    bool checked = ( ( value == Qt::Checked ) && showTorque->isEnabled()) ? true : false;

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    allPlot->setShowTorque(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowTorque(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowBalance(int value)
{
    showBalance->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showBalance->isEnabled()) ? true : false;

    allPlot->setShowBalance(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowBalance(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setShowPS(int value)
{
    showPS->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showPS->isEnabled()) ? true : false;

    allPlot->setShowPS(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowPS(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw

}

void
AllPlotWindow::setShowTE(int value)
{
    showTE->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showTE->isEnabled()) ? true : false;

    allPlot->setShowTE(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowTE(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw

}

void
AllPlotWindow::setShowPCO(int value)
{
    showPCO->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showPCO->isEnabled()) ? true : false;

    allPlot->setShowPCO(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowPCO(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw

}

void
AllPlotWindow::setShowDC(int value)
{
    showDC->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showDC->isEnabled()) ? true : false;

    allPlot->setShowDC(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowDC(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw

}

void
AllPlotWindow::setShowPPP(int value)
{
    showPPP->setChecked(value);

    // compare mode selfcontained update
    if (isCompare()) {
        compareChanged();
        return;
    }

    bool checked = ( ( value == Qt::Checked ) && showPPP->isEnabled()) ? true : false;

    allPlot->setShowPPP(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowPPP(checked);
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw

}

void
AllPlotWindow::setShowHelp(int value)
{
    rHelp->setChecked(value);
    showHelp->setChecked(value);
    if (showHelp->isChecked()) helperWidget()->show();
    else helperWidget()->hide();
}

void
AllPlotWindow::setShowFull(int value)
{
    rFull->setChecked(value);
    showFull->setChecked(value);
    if (showFull->isChecked()) {
        fullPlot->show();
        allPlotLayout->setStretch(1,20);
    }
    else {
        fullPlot->hide();
        allPlotLayout->setStretch(1,0);
    }
}

void
AllPlotWindow::setShowInterval(int value)
{
    showInterval->setChecked(value);
    if (showInterval->isChecked()) {
        intervalPlot->show();
        //allPlotLayout->setStretch(1,20);
    }
    else {
        intervalPlot->hide();
        //allPlotLayout->setStretch(1,0);
    }
}

void
AllPlotWindow::setShowGrid(int value)
{
    showGrid->setChecked(value);

    if (isCompare()) {

        // XXX 
        return;

    } else {

        allPlot->setShowGrid(value);
        foreach (AllPlot *plot, allPlots)
            plot->setShowGrid(value);
        // and the series stacks too
        forceSetupSeriesStackPlots(); // scope changed so force redraw
    }
}

void
AllPlotWindow::setPaintBrush(int value)
{
    if (active == true) return;

    if (isCompare()) {
        // XXX 
        return;
    }

    active = true;
    paintBrush->setChecked(value);

    //if (!current) return;

    allPlot->setPaintBrush(value);
    foreach (AllPlot *plot, allPlots)
        plot->setPaintBrush(value);

    active = false;
    // and the series stacks too
    forceSetupSeriesStackPlots(); // scope changed so force redraw
}

void
AllPlotWindow::setByDistance(int value)
{
    if (value <0 || value >2 || active == true) return;

    active = true;
    comboDistance->setCurrentIndex(value);

    // compare mode is self contained
    if (isCompare()) {
        fullPlot->bydist = (value == 1);
        compareChanged();
        active = false;
        return;
    }

    fullPlot->setByDistance(value);
    intervalPlot->setByDistance(value);
    allPlot->setByDistance(value);

    // refresh controls, specifically spanSlider
    setAllPlotWidgets(fullPlot->rideItem);

    // refresh
    setupSeriesStack = setupStack = false;
    redrawFullPlot();
    redrawAllPlot();
    setupStackPlots();
    setupSeriesStackPlots();
    redrawIntervalPlot();

    active = false;
}

void
AllPlotWindow::setSmoothing(int value)
{
    //if (!current) return;
    smoothSlider->setValue(value);

    // Compare has LOTS of rides to smooth...
    if (context->isCompareIntervals) {

        // no zero smoothing when comparing -- we need the
        // arrays to be initialised for all series
        if (value < 1) value = 1;
        fullPlot->setSmoothing(value);

        setUpdatesEnabled(false);

        // smooth each on full plots ...
        foreach(AllPlotObject *po, compareIntervalCurves) {
            fullPlot->recalc(po);
        }

        // .. and all plots too!
        int i=0;
        foreach (AllPlot *plot, allComparePlots) {
            plot->setDataFromObject(compareIntervalCurves[i], allPlot);
            i++;
        }
        // and series plots..
        foreach (AllPlot *plot, seriesPlots) {
            plot->setDataFromPlots(allComparePlots);
            plot->replot(); // straight away this time
        }

        setUpdatesEnabled(true);

    } else {

        // recalculate etc
        fullPlot->setSmoothing(value);

        // redraw
        redrawFullPlot();
        redrawAllPlot();
        redrawStackPlot();
    }
}

void
AllPlotWindow::resetSeriesStackedDatas()
{
    if (!current) return;

    // just reset from AllPlot
    foreach(AllPlot *p, seriesPlots) {
        p->setDataFromPlot(allPlot);
    }
}
//
// Runs through the stacks and updates their contents
//
void
AllPlotWindow::resetStackedDatas()
{
    if (!allPlot->rideItem || !allPlot->rideItem->ride() || !current) return;

    int _stackWidth = stackWidth;
    if (allPlot->bydist)
        _stackWidth = stackWidth/3;

    for(int i = 0 ; i < allPlots.count() ; i++)
    {
        AllPlot *plot = allPlots[i];
        int startIndex, stopIndex;
        if (plot->bydist) {
            startIndex = allPlot->rideItem->ride()->distanceIndex(
                        (context->athlete->useMetricUnits ? 1 : KM_PER_MILE) * _stackWidth*i);
            stopIndex  = allPlot->rideItem->ride()->distanceIndex(
                        (context->athlete->useMetricUnits ? 1 : KM_PER_MILE) * _stackWidth*(i+1));
        } else {
            startIndex = allPlot->rideItem->ride()->timeIndex(60*_stackWidth*i);
            stopIndex  = allPlot->rideItem->ride()->timeIndex(60*_stackWidth*(i+1));
        }

        // Update AllPlot for stacked view
        plot->setDataFromPlot(fullPlot, startIndex, stopIndex);
        if (i != 0) plot->setTitle("");
    }

}

void
AllPlotWindow::stackZoomSliderChanged()
{
    // slider moved!
    setStackWidth(stackZoomWidth[stackZoomSlider->value()]);
}

void
AllPlotWindow::setStackWidth(int width)
{
    // derive width from allowed values -- this is for backward 
    // compatibility to ensure we always use a size that we are
    // expecting and set the slider to the appropriate value
    int i=0;
    for (; i<8; i++) // there are 8 pre-set sizes
        if (width <= stackZoomWidth[i]) break;

    // we never found it
    if (i == 8) i=7;

    // did anything actually change?
    if (stackZoomWidth[i] == stackWidth) return;

    // set the value and the slider
    stackWidth = stackZoomWidth[i];
    stackZoomSlider->setValue(i);

    resizeSeriesPlots(); // its only the size that needs to change
                         // no need to replot

    resizeComparePlots();

    // now lets do the plots...
    setupStack = false; // force resize
    setupStackPlots();
}

void
AllPlotWindow::showStackChanged(int value)
{

    showStack->setCheckState((Qt::CheckState)value);
    rStack->setCheckState((Qt::CheckState)value);

    if (isCompare()) {
        setAllPlotWidgets(NULL); // no need to do anything fancy
        return;
    }

    if (!current) return;

    // enables / disable as needed
    if (showStack->isChecked()) {
        showBySeries->setEnabled(true);
        rBySeries->setEnabled(true);

        // all plot...
        allPlotStack->setCurrentIndex(0);
        if (isCompare()) allStack->setCurrentIndex(1); // compare mode stack of all plots
        else allStack->setCurrentIndex(0); // normal single allplot

    } else {
        showBySeries->setEnabled(false);
        rBySeries->setEnabled(false);
        allPlotStack->setCurrentIndex(1);
    }

    // when we swap from normal to
    // stacked view, save the mode so
    // we can startup with the 'preferred'
    // view next time. And replot for the
    // target mode since it is probably
    // out of date. Then call setAllPlotWidgets
    // to make sure all the controls are setup
    // and the right widgets are hidden/shown.
    if (value) {

        if (!showBySeries->isChecked()) {

            // BY TIME / DISTANCE

            // setup the stack plots if needed
            setupStackPlots();

            // refresh plots
            resetStackedDatas();

            // now they are all set, lets replot them
            foreach(AllPlot *plot, allPlots) plot->replot();

        } else {

            // BY SERIES

            // setup the stack plots if needed
            setupSeriesStackPlots();

            // refresh plots
            resetSeriesStackedDatas();

            // now they are all set, lets replot them
            foreach(AllPlot *plot, seriesPlots) plot->replot();

        }

    } else {
        // refresh plots
        redrawAllPlot();
    }

    // reset the view
    setAllPlotWidgets(current);
}

void
AllPlotWindow::showBySeriesChanged(int value)
{
    if (!current) return;

    showBySeries->setCheckState((Qt::CheckState)value);
    rBySeries->setCheckState((Qt::CheckState)value);
    showStackChanged(showStack->checkState()); // force replot etc
}

void
AllPlotWindow::forceSetupSeriesStackPlots()
{
    setupSeriesStack = false;
    setupSeriesStackPlots();
}

void
AllPlotWindow::resizeSeriesPlots()
{
    // calling setupdates enabled when its disabled
    // causes all paint events to be flushed. We want
    // to avoid that, since it causes a flicker.
    bool update = seriesstackFrame->updatesEnabled();

    if (update) seriesstackFrame->setUpdatesEnabled(false);
    foreach (AllPlot *plot, seriesPlots)
        plot->setFixedHeight((120*dpiYFactor) + (stackWidth *4));
    if (update) seriesstackFrame->setUpdatesEnabled(true);
}

void
AllPlotWindow::resizeComparePlots()
{
    comparePlotFrame->setUpdatesEnabled(false);
    foreach (AllPlot *plot, allComparePlots)
        plot->setFixedHeight((120*dpiYFactor) + (stackWidth *4));
    comparePlotFrame->setUpdatesEnabled(true);
}

void
AllPlotWindow::setupSeriesStackPlots()
{
    if (!isCompare() && (!showStack->isChecked() || !showBySeries->isChecked() || setupSeriesStack)) return;

    QVBoxLayout *newLayout = new QVBoxLayout;

    // this is NOT a memory leak (see ZZZ below)
    seriesPlots.clear();

    bool addHeadwind = false;
    RideItem* rideItem = current;
    if (!rideItem || !rideItem->ride() || rideItem->ride()->dataPoints().isEmpty()) return;

    // the refresh takes a while and is prone
    // to lots of flicker, we turn off updates
    // whilst we switch from the old to the new
    // stackWidget and plots
    seriesstackFrame->setUpdatesEnabled(false);

    QPalette palette;
    palette.setBrush(QPalette::Background, Qt::NoBrush);

    QList<SeriesWanted> serieslist;
    SeriesWanted s;
    // lets get a list of what we need to plot -- plot is same order as options in settings
    if (showPower->currentIndex() < 2 && rideItem->ride()->areDataPresent()->watts) { s.one = RideFile::watts; s.two = RideFile::none; serieslist << s; }
    if (showW->isChecked() && rideItem->ride()->areDataPresent()->watts) { s.one = RideFile::wprime; s.two = RideFile::none; serieslist << s; }
    if (showPowerD->isChecked() && rideItem->ride()->areDataPresent()->watts) { s.one = RideFile::wattsd;s.two = RideFile::none; serieslist << s; }
    if (showHr->isChecked() && rideItem->ride()->areDataPresent()->hr) { s.one = RideFile::hr; s.two = RideFile::none; serieslist << s; }
    if (showHRV->isChecked() && rideItem->ride()->areDataPresent()->hrv) { s.one = RideFile::hrv; s.two = RideFile::none; serieslist << s; }
    if (showTcore->isChecked() && rideItem->ride()->areDataPresent()->hr) { s.one = RideFile::tcore; s.two = RideFile::none; serieslist << s; }
    if (showHrD->isChecked() && rideItem->ride()->areDataPresent()->hr) { s.one = RideFile::hrd; s.two = RideFile::none; serieslist << s; }
    if (showSmO2->isChecked() && rideItem->ride()->areDataPresent()->smo2) { s.one = RideFile::smo2; s.two = RideFile::none; serieslist << s; }
    if (showtHb->isChecked() && rideItem->ride()->areDataPresent()->thb) { s.one = RideFile::thb; s.two = RideFile::none; serieslist << s; }
    if (showO2Hb->isChecked() && rideItem->ride()->areDataPresent()->o2hb) { s.one = RideFile::o2hb; s.two = RideFile::none; serieslist << s; }
    if (showHHb->isChecked() && rideItem->ride()->areDataPresent()->hhb) { s.one = RideFile::hhb; s.two = RideFile::none; serieslist << s; }
    if (showWind->isChecked() && rideItem->ride()->areDataPresent()->headwind) addHeadwind=true; //serieslist << RideFile::headwind;
    if (showSpeed->isChecked() && rideItem->ride()->areDataPresent()->kph) {s.one = RideFile::kph; s.two = (addHeadwind ? RideFile::headwind : RideFile::none); serieslist << s; }
    if (showAccel->isChecked() && rideItem->ride()->areDataPresent()->kph) { s.one = RideFile::kphd; s.two = RideFile::none; serieslist << s; }
    if (showCad->isChecked() && rideItem->ride()->areDataPresent()->cad) { s.one = RideFile::cad; s.two = RideFile::none; serieslist << s; }
    if (showCadD->isChecked() && rideItem->ride()->areDataPresent()->cad) { s.one = RideFile::cadd; s.two = RideFile::none; serieslist << s; }
    if (showTorque->isChecked() && rideItem->ride()->areDataPresent()->nm) { s.one = RideFile::nm; s.two = RideFile::none; serieslist << s; }
    if (showTorqueD->isChecked() && rideItem->ride()->areDataPresent()->nm) { s.one = RideFile::nmd; s.two = RideFile::none; serieslist << s; }
    if (showAlt->isChecked() && rideItem->ride()->areDataPresent()->alt) { s.one = RideFile::alt; s.two = RideFile::none; serieslist << s; }
    if (showAltSlope->currentIndex() > 0 && rideItem->ride()->areDataPresent()->alt) { s.one = RideFile::alt; s.two = RideFile::slope; serieslist << s; }
    if (showSlope->isChecked() && rideItem->ride()->areDataPresent()->slope) { s.one = RideFile::slope; s.two = RideFile::none; serieslist << s; }
    if (showTemp->isChecked() && rideItem->ride()->areDataPresent()->temp) { s.one = RideFile::temp; s.two = RideFile::none; serieslist << s; } 
    if (showNP->isChecked() && rideItem->ride()->areDataPresent()->watts) { s.one = RideFile::IsoPower; s.two = RideFile::none; serieslist << s; }
    if (showRV->isChecked() && rideItem->ride()->areDataPresent()->rvert) { s.one = RideFile::rvert; s.two = RideFile::none; serieslist << s; }
    if (showRCad->isChecked() && rideItem->ride()->areDataPresent()->rcad) { s.one = RideFile::rcad; s.two = RideFile::none; serieslist << s; }
    if (showRGCT->isChecked() && rideItem->ride()->areDataPresent()->rcontact) { s.one = RideFile::rcontact; s.two = RideFile::none; serieslist << s; }
    if (showGear->isChecked() && rideItem->ride()->areDataPresent()->gear) { s.one = RideFile::gear; s.two = RideFile::none; serieslist << s; }
    if (showATISS->isChecked() && rideItem->ride()->areDataPresent()->watts) { s.one = RideFile::aTISS; s.two = RideFile::none; serieslist << s; }
    if (showANTISS->isChecked() && rideItem->ride()->areDataPresent()->watts) { s.one = RideFile::anTISS; s.two = RideFile::none; serieslist << s; }
    if (showXP->isChecked() && rideItem->ride()->areDataPresent()->watts) { s.one = RideFile::xPower; s.two = RideFile::none; serieslist << s; }
    if (showAP->isChecked() && rideItem->ride()->areDataPresent()->watts) { s.one = RideFile::aPower; s.two = RideFile::none; serieslist << s; }
    if (showBalance->isChecked() && rideItem->ride()->areDataPresent()->lrbalance) { s.one = RideFile::lrbalance; s.two = RideFile::none; serieslist << s; }
    if (showTE->isChecked() && rideItem->ride()->areDataPresent()->lte) { s.one = RideFile::lte; s.two = RideFile::none; serieslist << s;
         s.one = RideFile::rte; s.two = RideFile::none; serieslist << s; }
    if (showPS->isChecked() && rideItem->ride()->areDataPresent()->lps) { s.one = RideFile::lps; s.two = RideFile::none; serieslist << s;
         s.one = RideFile::rps; s.two = RideFile::none; serieslist << s; }
    if (showPCO->isChecked() && rideItem->ride()->areDataPresent()->lpco) { s.one = RideFile::lpco; s.two = RideFile::rpco; serieslist << s;}
    if (showDC->isChecked() && rideItem->ride()->areDataPresent()->lppb) { s.one = RideFile::lppb; s.two = RideFile::lppe; serieslist << s;
         s.one = RideFile::rppb; s.two = RideFile::rppe; serieslist << s; }
    if (showPPP->isChecked() && rideItem->ride()->areDataPresent()->lpppb) { s.one = RideFile::lpppb; s.two = RideFile::lpppe; serieslist << s;
         s.one = RideFile::rpppb; s.two = RideFile::rpppe; serieslist << s; }

    // and the user series
    for(int k=0; k<userDataSeries.count(); k++) {
        if (userDataSeries[k]->isEmpty()) continue;
        s.one = static_cast<RideFile::SeriesType>(RideFile::none + 1 + k);
        s.two = RideFile::none;
        serieslist << s;
    }

    bool first = true;
    foreach(SeriesWanted x, serieslist) {

        // create that plot
        AllPlot *_allPlot = new AllPlot(this, this, context, x.one, x.two, first);
        _allPlot->setAutoFillBackground(false);
        _allPlot->setPalette(palette);
        _allPlot->setPaintBrush(paintBrush->checkState());
        _allPlot->setDataFromPlot(allPlot); // will clone all settings and data for the series
                                                   // being plotted, only works for one series plotting

        if (x.one == RideFile::watts) _allPlot->setShadeZones(showPower->currentIndex() == 0);
        else _allPlot->setShadeZones(false);
        first = false;

        // add to the list
        seriesPlots.append(_allPlot);
        addPickers(_allPlot);
        newLayout->addWidget(_allPlot);
        _allPlot->setFixedHeight((120*dpiYFactor)+(stackWidth*4));

        // No x axis titles
        _allPlot->setAxisVisible(QwtPlot::xBottom, true);
        _allPlot->enableAxis(QwtPlot::xBottom, true);
        _allPlot->setAxisTitle(QwtPlot::xBottom,NULL);
        _allPlot->setAxisMaxMinor(QwtPlot::xBottom, 0);
        _allPlot->setAxisMaxMinor(QwtPlot::yLeft, 0);
        _allPlot->setAxisMaxMinor(QwtAxisId(QwtAxis::yLeft,1), 0);
        _allPlot->setAxisMaxMinor(QwtPlot::yRight, 0);
        _allPlot->setAxisMaxMinor(QwtAxisId(QwtAxis::yRight,2).id, 0);
        _allPlot->setAxisMaxMinor(QwtAxisId(QwtAxis::yRight,3).id, 0);

        // controls
        _allPlot->replot();
    }

    newLayout->addStretch();

    // lets make sure the size matches the user preferences
    resizeSeriesPlots(); // << checks if updates enabled and doesn't reenable

    // set new widgets
    QPalette old = seriesstackWidget->palette();
    seriesstackWidget = new QWidget(this);
    seriesstackWidget->setPalette(old);
    seriesstackWidget->setAutoFillBackground(false);
    seriesstackWidget->setLayout(newLayout);
    seriesstackFrame->setWidget(seriesstackWidget);

    // lets remember the layout
    seriesstackPlotLayout = newLayout;

    // ZZZZ zap old widgets - is NOT required
    //                        since setWidget above will destroy
    //                        the ScrollArea's children
    // not neccessary --> foreach (AllPlot *plot, oldPlots) delete plot;
    // not neccessary --> delete stackPlotLayout;
    // not neccessary --> delete stackWidget;

    // we are now happy for a screen refresh to take place
    seriesstackFrame->setUpdatesEnabled(true);

#if 0
    // first lets wipe out the current plots
    // since we cache out above we need to
    // track and manage via below
#endif
    setupSeriesStack = true; // we're clean
}

void
AllPlotWindow::setupStackPlots()
{
    // recreate all the stack plots
    // they are all attached to the
    // stackWidget, which we ultimately
    // destroy in this function and replace
    // with a new one. This is to avoid some
    // nasty flickers and simplifies the code
    // for refreshing.
    if (!showStack->isChecked() || showBySeries->isChecked() || setupStack) return;

    // since we cache out above we need to
    // track and manage via below
    setupStack = true;

    // ************************************
    // EDIT WITH CAUTION -- THIS HAS BEEN
    // OPTIMISED FOR PERFORMANCE AS WELL AS
    // FOR SCREEN FLICKER. IT IS A LITTLE
    // COMPLICATED
    // ************************************
    QVBoxLayout *newLayout = new QVBoxLayout;

    // this is NOT a memory leak (see ZZZ below)
    allPlots.clear();

    int _stackWidth = stackWidth;
    if (fullPlot->bydist) _stackWidth = stackWidth/3;

    RideItem* rideItem = current;

    // don't try and plot for null files
    if (!rideItem || !rideItem->ride() || rideItem->ride()->dataPoints().isEmpty()) return;

    double duration = rideItem->ride()->dataPoints().last()->secs;
    double distance =  (context->athlete->useMetricUnits ? 1 : MILES_PER_KM) * rideItem->ride()->dataPoints().last()->km;
    int nbplot;

    if (fullPlot->bydist)
        nbplot = (int)floor(distance/_stackWidth)+1;
    else
        nbplot = (int)floor(duration/_stackWidth/60)+1;

    QPalette palette;
    palette.setBrush(QPalette::Background, Qt::NoBrush);

    for(int i = 0 ; i < nbplot ; i++) {

        // calculate the segment of ride this stack plot contains
        int startIndex, stopIndex;
        if (fullPlot->bydist) {
            startIndex = fullPlot->rideItem->ride()->distanceIndex((context->athlete->useMetricUnits ?
                            1 : KM_PER_MILE) * _stackWidth*i);
            stopIndex  = fullPlot->rideItem->ride()->distanceIndex((context->athlete->useMetricUnits ?
                            1 : KM_PER_MILE) * _stackWidth*(i+1));
        } else {
            startIndex = fullPlot->rideItem->ride()->timeIndex(60*_stackWidth*i);
            stopIndex  = fullPlot->rideItem->ride()->timeIndex(60*_stackWidth*(i+1));
        }

        // we need at least one point to plot
        if (stopIndex - startIndex < 2) break;

        // create that plot
        AllPlot *_allPlot = new AllPlot(this, this, context);
        _allPlot->setAutoFillBackground(false);
        _allPlot->setPalette(palette);
        _allPlot->setPaintBrush(paintBrush->checkState());

        // add to the list
        allPlots.append(_allPlot);
        addPickers(_allPlot);
        newLayout->addWidget(_allPlot);

        // Update AllPlot for stacked view
        _allPlot->setDataFromPlot(fullPlot, startIndex, stopIndex);
        _allPlot->setAxisScale(QwtPlot::xBottom, _stackWidth*i, _stackWidth*(i+1), 15/stackWidth);

        _allPlot->setFixedHeight((120*dpiYFactor)+stackWidth*4);

        // No x axis titles
        _allPlot->setAxisVisible(QwtPlot::xBottom, true);
        _allPlot->enableAxis(QwtPlot::xBottom, true);
        _allPlot->setAxisTitle(QwtPlot::xBottom,NULL);
        _allPlot->setAxisMaxMinor(QwtPlot::xBottom, 0);
        _allPlot->setAxisMaxMinor(QwtPlot::yLeft, 0);
        _allPlot->setAxisMaxMinor(QwtAxisId(QwtAxis::yLeft,1), 0);
        _allPlot->setAxisMaxMinor(QwtPlot::yRight, 0);
        _allPlot->setAxisMaxMinor(QwtAxisId(QwtAxis::yRight,2).id, 0);
        _allPlot->setAxisMaxMinor(QwtAxisId(QwtAxis::yRight,3).id, 0);

        // controls
        _allPlot->setShadeZones(showPower->currentIndex() == 0);
        _allPlot->setShowPower(showPower->currentIndex());
        _allPlot->setShowHr( (showHr->isEnabled()) ? ( showHr->checkState() == Qt::Checked ) : false );
        _allPlot->setShowHRV( (showHRV->isEnabled()) ? ( showHRV->checkState() == Qt::Checked ) : false );
        _allPlot->setShowTcore( (showTcore->isEnabled()) ? ( showTcore->checkState() == Qt::Checked ) : false );
        _allPlot->setShowSpeed((showSpeed->isEnabled()) ? ( showSpeed->checkState() == Qt::Checked ) : false );
        _allPlot->setShowAccel((showAccel->isEnabled()) ? ( showAccel->checkState() == Qt::Checked ) : false );
        _allPlot->setShowPowerD((showPowerD->isEnabled()) ? ( showPowerD->checkState() == Qt::Checked ) : false );
        _allPlot->setShowCadD((showCadD->isEnabled()) ? ( showCadD->checkState() == Qt::Checked ) : false );
        _allPlot->setShowTorqueD((showTorqueD->isEnabled()) ? ( showTorqueD->checkState() == Qt::Checked ) : false );
        _allPlot->setShowHrD((showHrD->isEnabled()) ? ( showHrD->checkState() == Qt::Checked ) : false );
        _allPlot->setShowCad((showCad->isEnabled()) ? ( showCad->checkState() == Qt::Checked ) : false );
        _allPlot->setShowAlt((showAlt->isEnabled()) ? ( showAlt->checkState() == Qt::Checked ) : false );
        _allPlot->setShowSlope((showSlope->isEnabled()) ? ( showSlope->checkState() == Qt::Checked ) : false );
        _allPlot->setShowAltSlope(showAltSlope->currentIndex());
        _allPlot->setShowTemp((showTemp->isEnabled()) ? ( showTemp->checkState() == Qt::Checked ) : false );
        _allPlot->setShowTorque((showTorque->isEnabled()) ? ( showTorque->checkState() == Qt::Checked ) : false );
        _allPlot->setShowW((showW->isEnabled()) ? ( showW->checkState() == Qt::Checked ) : false );
        _allPlot->setShowATISS((showATISS->isEnabled()) ? ( showATISS->checkState() == Qt::Checked ) : false );
        _allPlot->setShowANTISS((showANTISS->isEnabled()) ? ( showANTISS->checkState() == Qt::Checked ) : false );
        _allPlot->setShowGrid(showGrid->checkState() == Qt::Checked);
        _allPlot->setPaintBrush(paintBrush->checkState());
        _allPlot->setSmoothing(smoothSlider->value());
        _allPlot->setByDistance(comboDistance->currentIndex());
        _allPlot->setShowBalance((showBalance->isEnabled()) ? ( showBalance->checkState() == Qt::Checked ) : false );
        _allPlot->setShowNP((showNP->isEnabled()) ? ( showNP->checkState() == Qt::Checked ) : false );
        _allPlot->setShowRV((showRV->isEnabled()) ? ( showRV->checkState() == Qt::Checked ) : false );
        _allPlot->setShowRCad((showRCad->isEnabled()) ? ( showRCad->checkState() == Qt::Checked ) : false );
        _allPlot->setShowRGCT((showRGCT->isEnabled()) ? ( showRGCT->checkState() == Qt::Checked ) : false );
        _allPlot->setShowGear((showGear->isEnabled()) ? ( showGear->checkState() == Qt::Checked ) : false );
        _allPlot->setShowSmO2((showSmO2->isEnabled()) ? ( showSmO2->checkState() == Qt::Checked ) : false );
        _allPlot->setShowtHb((showtHb->isEnabled()) ? ( showtHb->checkState() == Qt::Checked ) : false );
        _allPlot->setShowO2Hb((showO2Hb->isEnabled()) ? ( showO2Hb->checkState() == Qt::Checked ) : false );
        _allPlot->setShowHHb((showHHb->isEnabled()) ? ( showHHb->checkState() == Qt::Checked ) : false );
        _allPlot->setShowXP((showXP->isEnabled()) ? ( showXP->checkState() == Qt::Checked ) : false );
        _allPlot->setShowAP((showAP->isEnabled()) ? ( showAP->checkState() == Qt::Checked ) : false );
        _allPlot->setShowTE((showTE->isEnabled()) ? ( showTE->checkState() == Qt::Checked ) : false );
        _allPlot->setShowPS((showPS->isEnabled()) ? ( showPS->checkState() == Qt::Checked ) : false );
        _allPlot->setShowPCO((showPCO->isEnabled()) ? ( showPCO->checkState() == Qt::Checked ) : false );
        _allPlot->setShowDC((showDC->isEnabled()) ? ( showDC->checkState() == Qt::Checked ) : false );
        _allPlot->setShowPPP((showPPP->isEnabled()) ? ( showPPP->checkState() == Qt::Checked ) : false );

        _allPlot->replot();
    }
    newLayout->addStretch();

    // the refresh takes a while and is prone
    // to lots of flicker, we turn off updates
    // whilst we switch from the old to the new
    // stackWidget and plots
    stackFrame->setUpdatesEnabled(false);

    // set new widgets
    QPalette old = stackWidget->palette();
    stackWidget = new QWidget(this);
    stackWidget->setPalette(old);
    stackWidget->setAutoFillBackground(false);
    stackWidget->setLayout(newLayout);
    stackFrame->setWidget(stackWidget);

    // ZZZZ zap old widgets - is NOT required
    //                        since setWidget above will destroy
    //                        the ScrollArea's children
    // not neccessary --> foreach (AllPlot *plot, oldPlots) delete plot;
    // not neccessary --> delete stackPlotLayout;
    // not neccessary --> delete stackWidget;

    // we are now happy for a screen refresh to take place
    stackFrame->setUpdatesEnabled(true);

}

void
AllPlotWindow::addPickers(AllPlot *_allPlot)
{
    QwtPlotMarker* allMarker1 = new QwtPlotMarker();
    allMarker1->setLineStyle(QwtPlotMarker::VLine);
    allMarker1->attach(_allPlot);
    allMarker1->setLabelAlignment(Qt::AlignTop|Qt::AlignRight);
    _allPlot->standard->allMarker1 = allMarker1;

    QwtPlotMarker* allMarker2 = new QwtPlotMarker();
    allMarker2->setLineStyle(QwtPlotMarker::VLine);
    allMarker2->attach(_allPlot);
    allMarker2->setLabelAlignment(Qt::AlignTop|Qt::AlignRight);
    _allPlot->standard->allMarker2 = allMarker2;

    // use the tooltip picker rather than a standard picker
    _allPlot->tooltip = new LTMToolTip(QwtPlot::xBottom, QwtAxisId(QwtAxis::yLeft, 2).id,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOn,
                               _allPlot->canvas(),
                               "");
    _allPlot->tooltip->setRubberBand(QwtPicker::VLineRubberBand);
    _allPlot->tooltip->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    _allPlot->tooltip->setTrackerPen(QColor(Qt::black));
    QColor inv(Qt::white);
    inv.setAlpha(0);
    _allPlot->tooltip->setRubberBandPen(inv);
    _allPlot->tooltip->setEnabled(true);

    _allPlot->_canvasPicker = new LTMCanvasPicker(_allPlot);
    connect(context, SIGNAL(intervalHover(IntervalItem*)), _allPlot, SLOT(intervalHover(IntervalItem*)));
    connect(_allPlot->_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), _allPlot, SLOT(pointHover(QwtPlotCurve*, int)));
    connect(_allPlot->tooltip, SIGNAL(moved(const QPoint &)), this, SLOT(plotPickerMoved(const QPoint &)));
    connect(_allPlot->tooltip, SIGNAL(appended(const QPoint &)), this, SLOT(plotPickerSelected(const QPoint &)));
}

void
AllPlotWindow::allPlotResized()
{
    QwtPlotCanvas *allPlotCanvas = static_cast<QwtPlotCanvas *>(allPlot->canvas());
    QwtScaleWidget *xAxis = allPlot->axisWidget(QwtAxisId(QwtPlot::xBottom));
    QwtScaleDraw *xAxisScaleDraw = xAxis->scaleDraw();
    int left = xAxis->x() + xAxisScaleDraw->pos().x() - 3;
    int right = allPlot->width() - allPlotCanvas->width() - allPlotCanvas->x() + 1;
    intervalPlot->setContentsMargins(left, 0, right, 0);
}
