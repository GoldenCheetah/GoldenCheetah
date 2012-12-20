/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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
#include "AllPlot.h"
#include "MainWindow.h"
#include "RideFile.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "TimeUtils.h"
#include "Settings.h"
#include "Units.h" // for MILES_PER_KM
#include "Colors.h" // for MILES_PER_KM
#include <qwt_plot_layout.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>
#include <qwt_arrow_button.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>

// span slider specials
#include <qxtspanslider.h>
#include <QCleanlooksStyle>

// tooltip
#include "LTMWindow.h"

AllPlotWindow::AllPlotWindow(MainWindow *mainWindow) :
    GcWindow(mainWindow), current(NULL), mainWindow(mainWindow), active(false), stale(true)
{
    setInstanceName("Ride Plot Window");

    QWidget *c = new QWidget;
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    setContentsMargins(0,0,0,0);

    // setup the controls
    QLabel *showLabel = new QLabel(tr("Show"), c);
    cl->addWidget(showLabel);

    showStack = new QCheckBox(tr("Stacked view"), this);
    showStack->setCheckState(Qt::Unchecked);
    cl->addWidget(showStack);

    stackWidth = 15;
    stackZoomUp = new QwtArrowButton(1, Qt::UpArrow,this);
    stackZoomUp->setFixedHeight(15);
    stackZoomUp->setFixedWidth(15);
    stackZoomUp->setEnabled(false);
    stackZoomUp->setContentsMargins(0,0,0,0);
    stackZoomUp->setFlat(true);
    cl->addWidget(stackZoomUp);

    stackZoomDown = new QwtArrowButton(1, Qt::DownArrow,this);
    stackZoomDown->setFixedHeight(15);
    stackZoomDown->setFixedWidth(15);
    stackZoomDown->setEnabled(false);
    stackZoomDown->setContentsMargins(0,0,0,0);
    stackZoomDown->setFlat(true);
    cl->addWidget(stackZoomDown);

    showFull = new QCheckBox(tr("Full plot"), this);
    showFull->setCheckState(Qt::Checked);
    cl->addWidget(showFull);

    paintBrush = new QCheckBox(tr("Fill Curves"), this);
    paintBrush->setCheckState(Qt::Unchecked);
    cl->addWidget(paintBrush);

    showGrid = new QCheckBox(tr("Grid"), this);
    showGrid->setCheckState(Qt::Checked);
    cl->addWidget(showGrid);

    showHr = new QCheckBox(tr("Heart Rate"), this);
    showHr->setCheckState(Qt::Checked);
    cl->addWidget(showHr);

    showSpeed = new QCheckBox(tr("Speed"), this);
    showSpeed->setCheckState(Qt::Checked);
    cl->addWidget(showSpeed);

    showCad = new QCheckBox(tr("Cadence"), this);
    showCad->setCheckState(Qt::Checked);
    cl->addWidget(showCad);

    showAlt = new QCheckBox(tr("Altitude"), this);
    showAlt->setCheckState(Qt::Checked);
    cl->addWidget(showAlt);

    showTemp = new QCheckBox(tr("Temperature"), this);
    showTemp->setCheckState(Qt::Checked);
    cl->addWidget(showTemp);

    showWind = new QCheckBox(tr("Headwind"), this);
    showWind->setCheckState(Qt::Checked);
    cl->addWidget(showWind);

    showTorque = new QCheckBox(tr("Torque"), this);
    showTorque->setCheckState(Qt::Checked);
    cl->addWidget(showTorque);

    showBalance = new QCheckBox(tr("Power balance"), this);
    showBalance->setCheckState(Qt::Checked);
    cl->addWidget(showBalance);

    showPower = new QComboBox();
    showPower->addItem(tr("Power + shade"));
    showPower->addItem(tr("Power - shade"));
    showPower->addItem(tr("No Power"));
    cl->addWidget(showPower);
    showPower->setCurrentIndex(0);

    comboDistance = new QComboBox();
    comboDistance->addItem(tr("X Axis Shows Time"));
    comboDistance->addItem(tr("X Axis Shows Distance"));
    cl->addWidget(comboDistance);

    QLabel *smoothLabel = new QLabel(tr("Smoothing (secs)"), this);
    smoothLineEdit = new QLineEdit(this);
    smoothLineEdit->setFixedWidth(40);

    cl->addWidget(smoothLabel);
    cl->addWidget(smoothLineEdit);
    smoothSlider = new QSlider(Qt::Horizontal);
    smoothSlider->setTickPosition(QSlider::TicksBelow);
    smoothSlider->setTickInterval(10);
    smoothSlider->setMinimum(1);
    smoothSlider->setMaximum(600);
    smoothLineEdit->setValidator(new QIntValidator(smoothSlider->minimum(),
                                                   smoothSlider->maximum(),
                                                   smoothLineEdit));
    cl->addWidget(smoothSlider);
    cl->addStretch();

    allPlot = new AllPlot(this, mainWindow);
    allPlot->setInstanceName("allPlot");
    allPlot->setContentsMargins(0,0,0,0);

    smoothSlider->setValue(allPlot->smooth);
    smoothLineEdit->setText(QString("%1").arg(allPlot->smooth));

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
    allPlot->tooltip = new LTMToolTip(QwtPlot::xBottom, QwtPlot::yLeft,
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
    connect(allPlot->_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), allPlot, SLOT(pointHover(QwtPlotCurve*, int)));
    connect(allPlot->tooltip, SIGNAL(moved(const QPoint &)), this, SLOT(plotPickerMoved(const QPoint &)));
    connect(allPlot->tooltip, SIGNAL(appended(const QPoint &)), this, SLOT(plotPickerSelected(const QPoint &)));

    QwtPlotMarker* allMarker1 = new QwtPlotMarker();
    allMarker1->setLineStyle(QwtPlotMarker::VLine);
    allMarker1->attach(allPlot);
    allMarker1->setLabelAlignment(Qt::AlignTop|Qt::AlignRight);
    allPlot->allMarker1=allMarker1;

    QwtPlotMarker* allMarker2 = new QwtPlotMarker();
    allMarker2->setLineStyle(QwtPlotMarker::VLine);
    allMarker2->attach(allPlot);
    allMarker2->setLabelAlignment(Qt::AlignTop|Qt::AlignRight);
    allPlot->allMarker2=allMarker2;

    // Container widgets should not paint
    // since they tend to use naff defaults and
    // 'complicate' or 'make busy' the general
    // look and feel
    QPalette palette;
    palette.setBrush(QPalette::Background, Qt::NoBrush);

    //
    // stack view
    //
    stackPlotLayout = new QVBoxLayout();
    stackPlotLayout->setSpacing(0);
    stackPlotLayout->setContentsMargins(0,0,0,0);
    stackWidget = new QWidget();
    stackWidget->setAutoFillBackground(false);
    stackWidget->setPalette(palette);
    stackWidget->setLayout(stackPlotLayout);

    stackFrame = new QScrollArea();
    stackFrame->hide();
    stackFrame->setPalette(palette);
    stackFrame->setAutoFillBackground(false);
    stackFrame->setWidgetResizable(true);
    stackFrame->setWidget(stackWidget);
    stackFrame->setFrameStyle(QFrame::NoFrame);
    stackFrame->setContentsMargins(0,0,0,0);

    //
    // allPlot view
    //
    allPlotLayout = new QVBoxLayout;
    allPlotLayout->setSpacing(0);
    allPlotLayout->setContentsMargins(0,0,0,0);
    allPlotFrame = new QScrollArea();
    allPlotFrame->setFrameStyle(QFrame::NoFrame);
    allPlotFrame->setAutoFillBackground(false);
    allPlotFrame->setPalette(palette);
    allPlotFrame->setContentsMargins(0,0,0,0);

    spanSlider = new QxtSpanSlider(Qt::Horizontal);
    spanSlider->setHandleMovementMode(QxtSpanSlider::NoOverlapping);
    spanSlider->setLowerValue(0);
    spanSlider->setUpperValue(15);

    QFont small;
    small.setPointSize(6);

    scrollLeft = new QPushButton("<");
    scrollLeft->setFont(small);
    scrollLeft->setAutoRepeat(true);
    scrollLeft->setFixedHeight(16);
    scrollLeft->setFixedWidth(16);
    scrollLeft->setContentsMargins(0,0,0,0);

    scrollRight = new QPushButton(">");
    scrollRight->setFont(small);
    scrollRight->setAutoRepeat(true);
    scrollRight->setFixedHeight(16);
    scrollRight->setFixedWidth(16);
    scrollRight->setContentsMargins(0,0,0,0);

#ifdef Q_OS_MAC
    // BUG in QMacStyle and painting of spanSlider
    // so we use a plain style to avoid it, but only
    // on a MAC, since win and linux are fine
    QCleanlooksStyle *style = new QCleanlooksStyle();
    spanSlider->setStyle(style);
    scrollLeft->setStyle(style);
    scrollRight->setStyle(style);
#endif

    fullPlot = new AllPlot(this, mainWindow);
    fullPlot->setInstanceName("fullPlot");
    fullPlot->grid->enableY(false);
    QPalette def;
    //fullPlot->setCanvasBackground(def.color(QPalette::Window));
    fullPlot->setCanvasBackground(Qt::white);
    fullPlot->setCanvasLineWidth(0);
    fullPlot->enableAxis(QwtPlot::yLeft, false);
    fullPlot->enableAxis(QwtPlot::yLeft2, false);
    fullPlot->enableAxis(QwtPlot::yRight, false);
    fullPlot->enableAxis(QwtPlot::yRight2, false);
    fullPlot->enableAxis(QwtPlot::xBottom, false);
    //fullPlot->legend()->clear();
    //fullPlot->setTitle("");
    fullPlot->setContentsMargins(0,0,0,0);

    allPlotLayout->addWidget(allPlot);
    allPlotFrame->setLayout(allPlotLayout);

    // controls...
    controlsLayout = new QGridLayout;
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(5,5,5,5);
    controlsLayout->addWidget(fullPlot, 0,1);
    controlsLayout->addWidget(spanSlider, 1,1);
    controlsLayout->addWidget(scrollLeft,1,0);
    controlsLayout->addWidget(scrollRight,1,2);
    controlsLayout->setRowStretch(0, 10);
    controlsLayout->setRowStretch(1, 1);
    controlsLayout->setContentsMargins(0,0,0,0);
#ifdef Q_OS_MAC
    // macs  dpscing is weird
    //controlsLayout->setSpacing(5);
#else
    controlsLayout->setSpacing(0);
#endif
    allPlotLayout->addLayout(controlsLayout);
    allPlotLayout->setStretch(0,100);
    allPlotLayout->setStretch(1,20);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(2,2,2,2);
    vlayout->setSpacing(0);
    vlayout->addWidget(allPlotFrame);
    vlayout->addWidget(stackFrame);
    vlayout->setSpacing(1);
    setLayout(vlayout);

    setContentsMargins(0,0,0,0);

    // common controls
    connect(showPower, SIGNAL(currentIndexChanged(int)), this, SLOT(setShowPower(int)));
    connect(showHr, SIGNAL(stateChanged(int)), this, SLOT(setShowHr(int)));
    connect(showSpeed, SIGNAL(stateChanged(int)), this, SLOT(setShowSpeed(int)));
    connect(showCad, SIGNAL(stateChanged(int)), this, SLOT(setShowCad(int)));
    connect(showAlt, SIGNAL(stateChanged(int)), this, SLOT(setShowAlt(int)));
    connect(showTemp, SIGNAL(stateChanged(int)), this, SLOT(setShowTemp(int)));
    connect(showWind, SIGNAL(stateChanged(int)), this, SLOT(setShowWind(int)));
    connect(showTorque, SIGNAL(stateChanged(int)), this, SLOT(setShowTorque(int)));
    connect(showBalance, SIGNAL(stateChanged(int)), this, SLOT(setShowBalance(int)));
    connect(showGrid, SIGNAL(stateChanged(int)), this, SLOT(setShowGrid(int)));
    connect(showFull, SIGNAL(stateChanged(int)), this, SLOT(setShowFull(int)));
    connect(showStack, SIGNAL(stateChanged(int)), this, SLOT(showStackChanged(int)));
    connect(paintBrush, SIGNAL(stateChanged(int)), this, SLOT(setPaintBrush(int)));
    connect(comboDistance, SIGNAL(currentIndexChanged(int)), this, SLOT(setByDistance(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)), this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(editingFinished()), this, SLOT(setSmoothingFromLineEdit()));

    // normal view
    connect(spanSlider, SIGNAL(lowerPositionChanged(int)), this, SLOT(zoomChanged()));
    connect(spanSlider, SIGNAL(upperPositionChanged(int)), this, SLOT(zoomChanged()));

    // stacked view
    connect(stackZoomUp, SIGNAL(clicked()), this, SLOT(setStackZoomUp()));
    connect(stackZoomDown, SIGNAL(clicked()), this, SLOT(setStackZoomDown()));
    connect(scrollLeft, SIGNAL(clicked()), this, SLOT(moveLeft()));
    connect(scrollRight, SIGNAL(clicked()), this, SLOT(moveRight()));

    // GC signals
    //connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(rideDirty()), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(mainWindow, SIGNAL(intervalsChanged()), this, SLOT(intervalsChanged()));
    connect(mainWindow, SIGNAL(intervalZoom(IntervalItem*)), this, SLOT(zoomInterval(IntervalItem*)));
    connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(mainWindow, SIGNAL(configChanged()), allPlot, SLOT(configChanged()));
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(mainWindow, SIGNAL(rideDeleted(RideItem*)), this, SLOT(rideDeleted(RideItem*)));
}

void
AllPlotWindow::configChanged()
{
    //We now use the window background color
    //fullPlot->setCanvasBackground(GColor(CPLOTTHUMBNAIL));

    // we're going to replot, but only if we're active
    // and all the other guff
    RideItem *ride = myRideItem;
    if (!amVisible()) {
        stale = true;
        return;
    }

    // ignore if null, or manual / empty
    if (!ride || !ride->ride() || !ride->ride()->dataPoints().count()) return;

    // ok replot with the new config!
    redrawFullPlot();
    redrawAllPlot();
    redrawStackPlot();
}

void
AllPlotWindow::redrawAllPlot()
{
    if (!showStack->isChecked() && current) {
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
    // always peformed sincethe data is used
    // by both the stack plots and the allplot
    RideItem *ride = current;

    // null rides are possible on new cyclist
    if (!ride) return;

    // hide the usual plot decorations etc
    fullPlot->setShowPower(1);
    //We now use the window background color
    //fullPlot->setCanvasBackground(GColor(CPLOTTHUMBNAIL));
    fullPlot->setCanvasLineWidth(0);
    fullPlot->grid->enableY(false);
    fullPlot->enableAxis(QwtPlot::yLeft, false);
    fullPlot->enableAxis(QwtPlot::yLeft2, false);
    fullPlot->enableAxis(QwtPlot::yRight, false);
    fullPlot->enableAxis(QwtPlot::yRight2, false);
    fullPlot->enableAxis(QwtPlot::xBottom, false);
    //fullPlot->legend()->clear();
    //fullPlot->setTitle("");

    if (fullPlot->bydist)
        fullPlot->setAxisScale(QwtPlot::xBottom,
        ride->ride()->dataPoints().first()->km * (fullPlot->useMetricUnits ? 1 : MILES_PER_KM),
        ride->ride()->dataPoints().last()->km * (fullPlot->useMetricUnits ? 1 : MILES_PER_KM));
    else
        fullPlot->setAxisScale(QwtPlot::xBottom, ride->ride()->dataPoints().first()->secs/60,
                                                 ride->ride()->dataPoints().last()->secs/60);
    fullPlot->replot();
}

void
AllPlotWindow::redrawStackPlot()
{
    if (showStack->isChecked()) {

        // turn off display updates whilst we
        // do this, it takes a while and is prone
        // to screen flicker
        stackFrame->setUpdatesEnabled(false);

        // remove current plots - then recreate
        resetStackedDatas();

        // now they are all set, lets replot
        foreach(AllPlot *plot, allPlots) plot->replot();

        // we're done, go update the display now
        stackFrame->setUpdatesEnabled(true);
    }
}

void
AllPlotWindow::zoomChanged()
{
    redrawAllPlot();
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
AllPlotWindow::rideSelected()
{
    RideItem *ride = myRideItem;

    if (ride == NULL) current = NULL;

    // ignore if not active
    if (!amVisible()) {
        stale = true;
        return;
    }

    // ignore if null, or manual / empty
    if (!ride || !ride->ride() || !ride->ride()->dataPoints().count()) {
        current = NULL;
        return;
    }

    // we already plotted it!
    if (ride == current && stale == false) return;

    // ok, its now the current ride
    current = ride;

    // clear any previous selections
    clearSelection();

    // setup the control widgets, dependant on
    // data present in this ride, needs to happen
    // before we set the plots below...
    setAllPlotWidgets(ride);

    // setup the charts to reflect current ride selection
    fullPlot->setDataFromRide(ride);


    // Fixup supplied by Josef Gebel
    int startidx, stopidx;
    if ( fullPlot->bydist == true ) {
        startidx = ride->ride()->distanceIndex( ( double ) spanSlider->lowerValue() / 1000.0 );
        stopidx = ride->ride()->distanceIndex( ( double ) spanSlider->upperValue() / 1000.0 );
    } else {
        startidx = ride->ride()->timeIndex( spanSlider->lowerValue() );
        stopidx = ride->ride()->timeIndex( spanSlider->upperValue() );
    }
    allPlot->setDataFromPlot( fullPlot, startidx, stopidx );

    // redraw all the plots, they will check
    // to see if they are currently visible
    // and only redraw if neccessary
    redrawFullPlot();
    redrawAllPlot();
    setupStackPlots();

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
        setAllPlotWidgets(NULL);
        fullPlot->setDataFromRide(NULL);
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

    // allPlot of course
    allPlot->refreshIntervalMarkers();
    allPlot->replot();

    // and then the stacked plot
    foreach (AllPlot *plot, allPlots) {
        plot->refreshIntervalMarkers();
        plot->replot();
    }
}

void
AllPlotWindow::intervalSelected()
{
    if (!amVisible()) {
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
        foreach (AllPlot *plot, allPlots)
            plot->replot();
    } else {
        fullPlot->replot();
        allPlot->replot();
    }
}

void
AllPlotWindow::setStacked(int value)
{
    showStack->setChecked(value);
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
    if (!ride) return;

    // checkboxes to show/hide specific data series...
	const RideFileDataPresent *dataPresent = ride->ride()->areDataPresent();
    if (ride->ride() && ride->ride()->deviceType() != QString("Manual CSV")) {

	    showPower->setEnabled(dataPresent->watts);
	    showHr->setEnabled(dataPresent->hr);
	    showSpeed->setEnabled(dataPresent->kph);
	    showCad->setEnabled(dataPresent->cad);
	    showAlt->setEnabled(dataPresent->alt);
        showTemp->setEnabled(dataPresent->temp);
        showWind->setEnabled(dataPresent->headwind);
        showTorque->setEnabled(dataPresent->nm);
        showBalance->setEnabled(dataPresent->lrbalance);
    } else {
        showPower->setEnabled(false);
        showHr->setEnabled(false);
        showSpeed->setEnabled(false);
        showCad->setEnabled(false);
        showAlt->setEnabled(false);
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

    // now set the visible plots, depending upon whether
    // we are in stacked mode or not
    if (showStack->isChecked()) {

        // hide normal view
        allPlotFrame->hide();
        allPlot->hide();
        fullPlot->hide();
        controlsLayout->setRowStretch(0, 0);
        controlsLayout->setRowStretch(1, 0);
        spanSlider->hide();
        scrollLeft->hide();
        scrollRight->hide();

        stackZoomUp->setEnabled(stackZoomUpShouldEnable(stackWidth));
        stackZoomDown->setEnabled(stackZoomDownShouldEnable(stackWidth));

        // show stacked view
        stackFrame->show();

    } else {

        // hide stack view
        stackZoomDown->setEnabled(false);
        stackZoomUp->setEnabled(false);
        stackFrame->hide();

        // show normal view
        allPlotFrame->show();
        allPlot->show();

        if (showFull->isChecked()) {
            fullPlot->show();
            controlsLayout->setRowStretch(0, 100);
            controlsLayout->setRowStretch(1, 20);
            spanSlider->show();
            scrollLeft->show();
            scrollRight->show();
        } else {
            fullPlot->hide();
            controlsLayout->setRowStretch(0, 100);
            controlsLayout->setRowStretch(1, 00);
            spanSlider->hide();
            scrollLeft->hide();
            scrollRight->hide();
        }

        stackZoomUp->setEnabled(false);
        stackZoomDown->setEnabled(false);
    }
}

void
AllPlotWindow::zoomInterval(IntervalItem *which)
{
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
    QwtPlotPicker* pick = qobject_cast<QwtPlotPicker *>(sender());
    AllPlot* plot = qobject_cast<AllPlot *>(pick->plot());
    double xValue = plot->invTransform(QwtPlot::xBottom, pos.x());

    setStartSelection(plot, xValue);
}

void
AllPlotWindow::plotPickerMoved(const QPoint &pos)
{
    QString name = QString("Selection #%1 ").arg(selection);

    // which picker and plot send this signal?
    QwtPlotPicker* pick = qobject_cast<QwtPlotPicker *>(sender());
    AllPlot* plot = qobject_cast<AllPlot *>(pick->plot());

    int posX = pos.x();
    int posY = plot->y()+pos.y()+50;

    if (showStack->isChecked()) {

        // need to highlight across stacked plots
        foreach (AllPlot *_plot, allPlots) {

            // mark the start of selection on every plot
            _plot->allMarker1->setValue(plot->allMarker1->value());

            if (_plot->y()<=plot->y() && posY<_plot->y()){

                if (_plot->transform(QwtPlot::xBottom, _plot->allMarker2->xValue())>0) {
                    setEndSelection(_plot, 0, false, name);
                    _plot->allMarker2->setLabel(QString(""));
                }

            } else if (_plot->y()>=plot->y() && posY>_plot->y()+_plot->height()) {

                if (_plot->transform(QwtPlot::xBottom, _plot->allMarker2->xValue())<plot->width()){
                    setEndSelection(_plot, _plot->transform(QwtPlot::xBottom, plot->width()), false, name);
                }
            }
            else if (posY>_plot->y() && posY<_plot->y()+_plot->height()) {

                if (pos.x()<6) {
                    posX = 6;
                } else if (!_plot->bydist && pos.x()>_plot->transform(QwtPlot::xBottom,
                                        fullPlot->timeArray[fullPlot->timeArray.size()-1])) {
                    posX = _plot->transform(QwtPlot::xBottom, fullPlot->timeArray[fullPlot->timeArray.size()-1]);
                } else if (plot->bydist && pos.x()>_plot->transform(QwtPlot::xBottom,
                                        fullPlot->distanceArray[fullPlot->distanceArray.size()-1])) {
                    posX = fullPlot->transform(QwtPlot::xBottom,
                                        fullPlot->distanceArray[fullPlot->distanceArray.size()-1]);
                }

                setEndSelection(_plot, _plot->invTransform(QwtPlot::xBottom, posX), true, name);

                if (plot->y()<_plot->y()) {
                    plot->allMarker1->setLabel(_plot->allMarker1->label());
                    plot->allMarker2->setLabel(_plot->allMarker2->label());
                }

            } else {

                _plot->allMarker1->hide();
                _plot->allMarker2->hide();

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
    QwtPlotMarker* allMarker1 = plot->allMarker1;

    selection++;

    if (!allMarker1->isVisible() || allMarker1->xValue() != xValue) {
        allMarker1->setValue(xValue, plot->bydist ? 0 : 100);
        allMarker1->show();
    }
}

void
AllPlotWindow::setEndSelection(AllPlot* plot, double xValue, bool newInterval, QString name)
{
    active = true;

    QwtPlotMarker* allMarker1 = plot->allMarker1;
    QwtPlotMarker* allMarker2 = plot->allMarker2;

    bool useMetricUnits = plot->useMetricUnits;
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
        double secsMoving = 0;
        double wattsTotal = 0;
        double bpmTotal = 0;
        int arrayLength = 0;


        // if we are in distance mode then x1 and x2 are distances
        // we need to make sure they are in KM for the rest of this
        // code.
        if (plot->bydist && useMetricUnits == false) {
            x1 *= KM_PER_MILE;
            x2 *=  KM_PER_MILE;
        }

        foreach (const RideFilePoint *point, ride->ride()->dataPoints()) {
            if ((plot->bydist==true && point->km>=x1 && point->km<x2) ||
                (plot->bydist==false && point->secs/60>=x1 && point->secs/60<x2)) {

                if (distance1 == -1) distance1 = point->km;
                distance2 = point->km;

                if (duration1 == -1) duration1 = point->secs;
                duration2 = point->secs;

                if (point->kph > 0.0)
                   secsMoving += ride->ride()->recIntSecs();
                wattsTotal += point->watts;
                bpmTotal += point->hr;
                ++arrayLength;
            }
        }
        QString s("\n%1%2 %3 %4\n%5%6 %7%8 %9%10");
        s = s.arg(useMetricUnits ? distance2-distance1 : (distance2-distance1)*MILES_PER_KM, 0, 'f', 2);
        s = s.arg((useMetricUnits? "km":"mi"));
        s = s.arg(time_to_string(duration2-duration1));
        if (duration2-duration1-secsMoving>1)
            s = s.arg("("+time_to_string(secsMoving)+")");
        else
            s = s.arg("");
        s = s.arg((useMetricUnits ? 1 : MILES_PER_KM) * (distance2-distance1)/secsMoving*3600, 0, 'f', 1);
        s = s.arg((useMetricUnits? "km/h":"mph"));
        if (wattsTotal>0) {
            s = s.arg(wattsTotal/arrayLength, 0, 'f', 1);
            s = s.arg("W");
        }
        else{
            s = s.arg("");
         s = s.arg("");
        }
        if (bpmTotal>0) {
            s = s.arg(bpmTotal/arrayLength, 0, 'f', 0);
            s = s.arg("bpm");
        }
        else {
            s = s.arg("");
            s = s.arg("");
        }

        QwtText label;
        label.setColor(GColor(CPLOTMARKER));
        label.setFont(QFont());
        label.setText(s);
        if (allMarker1->xValue()<allMarker2->xValue()) {
            allMarker1->setLabel(label);
            allMarker2->setLabel(QString(""));
        }
        else {
            allMarker2->setLabel(label);
            allMarker1->setLabel(QString(""));
        }


        if (newInterval) {

            QTreeWidgetItem *allIntervals = mainWindow->mutableIntervalItems();
            int count = allIntervals->childCount();

            // are we adjusting an existing interval? - if so delete it and readd it
            if (count > 0) {
                IntervalItem *bottom = (IntervalItem *) allIntervals->child(count-1);
                if (bottom->text(0).startsWith(name)) delete allIntervals->takeChild(count-1);
            }

            // add average power to the end of the selection name
            name += QString("(%1 watts)").arg(round((wattsTotal && arrayLength) ? wattsTotal/arrayLength : 0));

            QTreeWidgetItem *last = new IntervalItem(ride->ride(), name, duration1, duration2, distance1, distance2,
                                        allIntervals->childCount()+1);
            last->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
            allIntervals->addChild(last);

            // select this new interval
            mainWindow->intervalTreeWidget()->setItemSelected(last, true);

            // now update the RideFileIntervals and all the plots etc
            mainWindow->updateRideFileIntervals();
        }
    }
    active = false;
}

void
AllPlotWindow::clearSelection()
{
    selection = 0;
    allPlot->allMarker1->setVisible(false);
    allPlot->allMarker2->setVisible(false);

    foreach (AllPlot *plot, allPlots) {
        plot->allMarker1->setVisible(false);
        plot->allMarker2->setVisible(false);
    }
}

void
AllPlotWindow::hideSelection()
{
    if (showStack->isChecked()) {
        foreach (AllPlot *plot, allPlots) {
            plot->allMarker1->setVisible(false);
            plot->allMarker2->setVisible(false);
            plot->replot();
        }
    } else {
        fullPlot->replot();
        allPlot->allMarker1->setVisible(false);
        allPlot->allMarker2->setVisible(false);
        allPlot->replot();
    }
}

void
AllPlotWindow::setShowPower(int value)
{
    showPower->setCurrentIndex(value);

    //if (!current) return;

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
}

void
AllPlotWindow::setShowHr(int value)
{
    showHr->setChecked(value);

    //if (!current) return;
    bool checked = ( ( value == Qt::Checked ) && showHr->isEnabled()) ? true : false;

    allPlot->setShowHr(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowHr(checked);
}

void
AllPlotWindow::setShowSpeed(int value)
{
    showSpeed->setChecked(value);

    //if (!current) return;
    bool checked = ( ( value == Qt::Checked ) && showSpeed->isEnabled()) ? true : false;

    allPlot->setShowSpeed(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowSpeed(checked);
}

void
AllPlotWindow::setShowCad(int value)
{
    showCad->setChecked(value);

    //if (!current) return;
    bool checked = ( ( value == Qt::Checked ) && showCad->isEnabled()) ? true : false;

    allPlot->setShowCad(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowCad(checked);
}

void
AllPlotWindow::setShowAlt(int value)
{
    showAlt->setChecked(value);

    //if (!current) return;
    bool checked = ( ( value == Qt::Checked ) && showAlt->isEnabled()) ? true : false;

    allPlot->setShowAlt(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowAlt(checked);
}

void
AllPlotWindow::setShowTemp(int value)
{
    showTemp->setChecked(value);

    //if (!current) return;
    bool checked = ( ( value == Qt::Checked ) && showTemp->isEnabled()) ? true : false;

    allPlot->setShowTemp(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowTemp(checked);
}

void
AllPlotWindow::setShowWind(int value)
{
    showWind->setChecked(value);

    //if (!current) return;
    bool checked = ( ( value == Qt::Checked ) && showWind->isEnabled()) ? true : false;

    allPlot->setShowWind(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowWind(checked);
}

void
AllPlotWindow::setShowTorque(int value)
{
    showTorque->setChecked(value);
    bool checked = ( ( value == Qt::Checked ) && showTorque->isEnabled()) ? true : false;

    //if (!current) return;

    allPlot->setShowTorque(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowTorque(checked);
}

void
AllPlotWindow::setShowBalance(int value)
{
    showBalance->setChecked(value);

    bool checked = ( ( value == Qt::Checked ) && showBalance->isEnabled()) ? true : false;

    allPlot->setShowBalance(checked);
    foreach (AllPlot *plot, allPlots)
        plot->setShowBalance(checked);
}

void
AllPlotWindow::setShowFull(int value)
{
    showFull->setChecked(value);
    if (showFull->isChecked()) {
        fullPlot->show();
        spanSlider->show();
        scrollLeft->show();
        scrollRight->show();
        allPlotLayout->setStretch(1,20);
    }
    else {
        fullPlot->hide();
        spanSlider->hide();
        scrollLeft->hide();
        scrollRight->hide();
        allPlotLayout->setStretch(1,0);
    }
}

void
AllPlotWindow::setShowGrid(int value)
{
    showGrid->setChecked(value);

    //if (!current) return;

    allPlot->setShowGrid(value);
    foreach (AllPlot *plot, allPlots)
        plot->setShowGrid(value);
}

void
AllPlotWindow::setPaintBrush(int value)
{
    if (active == true) return;

    active = true;
    paintBrush->setChecked(value);

    //if (!current) return;

    allPlot->setPaintBrush(value);
    foreach (AllPlot *plot, allPlots)
        plot->setPaintBrush(value);

    active = false;
}

void
AllPlotWindow::setByDistance(int value)
{
    if (value <0 || value >1 || active == true) return;

    active = true;
    comboDistance->setCurrentIndex(value);

    fullPlot->setByDistance(value);
    allPlot->setByDistance(value);

    // refresh controls, specifically spanSlider
    setAllPlotWidgets(fullPlot->rideItem);

    // refresh
    redrawFullPlot();
    redrawAllPlot();
    setupStackPlots();

    active = false;
}

void
AllPlotWindow::setSmoothing(int value)
{
    //if (!current) return;
    smoothSlider->setValue(value);

    // recalculate etc
    fullPlot->setSmoothing(value);

    // redraw
    redrawFullPlot();
    redrawAllPlot();
    redrawStackPlot();
}

//
// Runs through the stacks and updates their contents
//
void
AllPlotWindow::resetStackedDatas()
{
    if (!current) return;

    int _stackWidth = stackWidth;
    if (allPlot->bydist)
        _stackWidth = stackWidth/3;

    for(int i = 0 ; i < allPlots.count() ; i++)
    {
        AllPlot *plot = allPlots[i];
        int startIndex, stopIndex;
        if (plot->bydist) {
            startIndex = allPlot->rideItem->ride()->distanceIndex(
                        (allPlot->useMetricUnits ? 1 : KM_PER_MILE) * _stackWidth*i);
            stopIndex  = allPlot->rideItem->ride()->distanceIndex(
                        (allPlot->useMetricUnits ? 1 : KM_PER_MILE) * _stackWidth*(i+1));
        } else {
            startIndex = allPlot->rideItem->ride()->timeIndex(60*_stackWidth*i);
            stopIndex  = allPlot->rideItem->ride()->timeIndex(60*_stackWidth*(i+1));
        }

        // Update AllPlot for stacked view
        plot->setDataFromPlot(fullPlot, startIndex, stopIndex);
        if (i != 0) plot->setTitle("");
    }

}

bool
AllPlotWindow::stackZoomUpShouldEnable(int sw)
{
    if (!current) return false;

    if (sw >= 200  || sw >= current->ride()->dataPoints().last()->secs/60) {
        return false;
    }
    else {
        return true;
    }
}

bool
AllPlotWindow::stackZoomDownShouldEnable(int sw)
{
    if (sw <= 4) {
        return false;
    }
    else {
        return true;
    }

}

void
AllPlotWindow::setStackZoomUp()
{
    if (!current) return;

    if (stackWidth<200 && stackWidth<current->ride()->dataPoints().last()->secs/60) {

        stackWidth = ceil(stackWidth * 1.25);
        setupStackPlots();
        stackZoomUp->setEnabled(stackZoomUpShouldEnable(stackWidth));
        stackZoomDown->setEnabled(stackZoomDownShouldEnable(stackWidth));
    }
}

void
AllPlotWindow::setStackZoomDown()
{
    if (!current) return;

    if (stackWidth>4) {

        stackWidth = floor(stackWidth / 1.25);
        setupStackPlots();
        stackZoomUp->setEnabled(stackZoomUpShouldEnable(stackWidth));
        stackZoomDown->setEnabled(stackZoomDownShouldEnable(stackWidth));
    }
}

void
AllPlotWindow::showStackChanged(int value)
{
    if (!current) return;

    // when we swap from normal to
    // stacked view, save the mode so
    // we can startup with the 'preferred'
    // view next time. And replot for the
    // target mode since it is probably
    // out of date. Then call setAllPlotWidgets
    // to make sure all the controls are setup
    // and the right widgets are hidden/shown.
    if (value) {
        // refresh plots
        resetStackedDatas();

        // now they are all set, lets replot them
        foreach(AllPlot *plot, allPlots) plot->replot();

    } else {
        // refresh plots
        redrawAllPlot();
    }

    // reset the view
    setAllPlotWidgets(current);
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
    double distance =  (fullPlot->useMetricUnits ? 1 : MILES_PER_KM) * rideItem->ride()->dataPoints().last()->km;
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
            startIndex = fullPlot->rideItem->ride()->distanceIndex((fullPlot->useMetricUnits ?
                            1 : KM_PER_MILE) * _stackWidth*i);
            stopIndex  = fullPlot->rideItem->ride()->distanceIndex((fullPlot->useMetricUnits ?
                            1 : KM_PER_MILE) * _stackWidth*(i+1));
        } else {
            startIndex = fullPlot->rideItem->ride()->timeIndex(60*_stackWidth*i);
            stopIndex  = fullPlot->rideItem->ride()->timeIndex(60*_stackWidth*(i+1));
        }

        // we need at least one point to plot
        if (stopIndex - startIndex < 2) break;

        // create that plot
        AllPlot *_allPlot = new AllPlot(this, mainWindow);
        _allPlot->setInstanceName("stackPlot");
        _allPlot->setAutoFillBackground(false);
        _allPlot->setPalette(palette);

        // add to the list
        allPlots.append(_allPlot);
        addPickers(_allPlot);
        newLayout->addWidget(_allPlot);

        // Update AllPlot for stacked view
        _allPlot->setDataFromPlot(fullPlot, startIndex, stopIndex);
        _allPlot->setAxisScale(QwtPlot::xBottom, _stackWidth*i, _stackWidth*(i+1), 15/stackWidth);

        _allPlot->setFixedHeight(120+stackWidth*2);

        // No x axis titles
        _allPlot->setAxisTitle(QwtPlot::xBottom,NULL);
        _allPlot->setAxisMaxMinor(QwtPlot::xBottom, 0);
        _allPlot->setAxisMaxMinor(QwtPlot::yLeft, 0);
        _allPlot->setAxisMaxMinor(QwtPlot::yLeft2, 0);
        _allPlot->setAxisMaxMinor(QwtPlot::yRight, 0);
        _allPlot->setAxisMaxMinor(QwtPlot::yRight2, 0);

        // controls
        _allPlot->setShadeZones(showPower->currentIndex() == 0);
        _allPlot->setShowPower(showPower->currentIndex());
        _allPlot->setShowHr( (showHr->isEnabled()) ? ( showHr->checkState() == Qt::Checked ) : false );
        _allPlot->setShowSpeed((showSpeed->isEnabled()) ? ( showSpeed->checkState() == Qt::Checked ) : false );
        _allPlot->setShowCad((showCad->isEnabled()) ? ( showCad->checkState() == Qt::Checked ) : false );
        _allPlot->setShowAlt((showAlt->isEnabled()) ? ( showAlt->checkState() == Qt::Checked ) : false );
        _allPlot->setShowTemp((showTemp->isEnabled()) ? ( showTemp->checkState() == Qt::Checked ) : false );
        _allPlot->setShowTorque((showTorque->isEnabled()) ? ( showTorque->checkState() == Qt::Checked ) : false );
        _allPlot->setShowGrid(showGrid->checkState() == Qt::Checked);
        _allPlot->setPaintBrush(paintBrush->checkState());
        _allPlot->setSmoothing(smoothSlider->value());
        _allPlot->setByDistance(comboDistance->currentIndex());

	    _allPlot->replot();
    }
    newLayout->addStretch();

    // the refresh takes a while and is prone
    // to lots of flicker, we turn off updates
    // whilst we switch from the old to the new
    // stackWidget and plots
    stackFrame->setUpdatesEnabled(false);

    // set new widgets
    QWidget *stackWidget = new QWidget;
    stackWidget->setPalette(palette);
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
    _allPlot->allMarker1 = allMarker1;

    QwtPlotMarker* allMarker2 = new QwtPlotMarker();
    allMarker2->setLineStyle(QwtPlotMarker::VLine);
    allMarker2->attach(_allPlot);
    allMarker2->setLabelAlignment(Qt::AlignTop|Qt::AlignRight);
    _allPlot->allMarker2 = allMarker2;

    // use the tooltip picker rather than a standard picker
    _allPlot->tooltip = new LTMToolTip(QwtPlot::xBottom, QwtPlot::yLeft,
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
    connect(_allPlot->_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), _allPlot, SLOT(pointHover(QwtPlotCurve*, int)));
    connect(_allPlot->tooltip, SIGNAL(moved(const QPoint &)), this, SLOT(plotPickerMoved(const QPoint &)));
    connect(_allPlot->tooltip, SIGNAL(appended(const QPoint &)), this, SLOT(plotPickerSelected(const QPoint &)));
}
