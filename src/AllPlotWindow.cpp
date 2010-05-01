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

AllPlotWindow::AllPlotWindow(MainWindow *mainWindow) :
    QWidget(mainWindow), mainWindow(mainWindow), current(NULL)
{
    QVBoxLayout *vlayout = new QVBoxLayout;

    QHBoxLayout *showLayout = new QHBoxLayout;
    QLabel *showLabel = new QLabel(tr("Show:"), this);
    showLayout->addWidget(showLabel);

    showStack = new QCheckBox(tr("Stacked view"), this);

    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    if (settings->value(GC_RIDE_PLOT_STACK).toInt())
        showStack->setCheckState(Qt::Checked);
    else
        showStack->setCheckState(Qt::Unchecked);
    showLayout->addWidget(showStack);

    stackWidth = 15;

    QLabel *labelspacer = new QLabel(this);
    labelspacer->setFixedWidth(5);
    showLayout->addWidget(labelspacer);

    stackZoomUp = new QwtArrowButton(1, Qt::UpArrow,this);
    stackZoomUp->setFixedHeight(15);
    stackZoomUp->setFixedWidth(15);
    stackZoomUp->setEnabled(false);
    showLayout->addWidget(stackZoomUp);

    stackZoomDown = new QwtArrowButton(1, Qt::DownArrow,this);
    stackZoomDown->setFixedHeight(15);
    stackZoomDown->setFixedWidth(15);
    stackZoomDown->setEnabled(false);
    showLayout->addWidget(stackZoomDown);

    QCheckBox *showGrid = new QCheckBox(tr("Grid"), this);
    showGrid->setCheckState(Qt::Checked);
    showLayout->addWidget(showGrid);

    showHr = new QCheckBox(tr("Heart Rate"), this);
    showHr->setCheckState(Qt::Checked);
    showLayout->addWidget(showHr);

    showSpeed = new QCheckBox(tr("Speed"), this);
    showSpeed->setCheckState(Qt::Checked);
    showLayout->addWidget(showSpeed);

    showCad = new QCheckBox(tr("Cadence"), this);
    showCad->setCheckState(Qt::Checked);
    showLayout->addWidget(showCad);

    showAlt = new QCheckBox(tr("Altitude"), this);
    showAlt->setCheckState(Qt::Checked);
    showLayout->addWidget(showAlt);

    showPower = new QComboBox();
    showPower->addItem(tr("Power + shade"));
    showPower->addItem(tr("Power - shade"));
    showPower->addItem(tr("No Power"));
    showLayout->addWidget(showPower);

    QHBoxLayout *smoothLayout = new QHBoxLayout;
    QComboBox *comboDistance = new QComboBox();
    comboDistance->addItem(tr("X Axis Shows Time"));
    comboDistance->addItem(tr("X Axis Shows Distance"));
    smoothLayout->addWidget(comboDistance);

    QLabel *smoothLabel = new QLabel(tr("Smoothing (secs)"), this);
    smoothLineEdit = new QLineEdit(this);
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
    allPlot = new AllPlot(this, mainWindow);
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


    // Interval selection
    allPicker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                                  QwtPicker::RectSelection | QwtPicker::CornerToCorner|QwtPicker::DragSelection,
                                   QwtPicker::VLineRubberBand,
                                   QwtPicker::ActiveOnly, allPlot->canvas());
    allPicker->setRubberBand(QwtPicker::CrossRubberBand);
    // now select rectangles
    allPicker->setSelectionFlags(QwtPicker::PointSelection | QwtPicker::RectSelection | QwtPicker::DragSelection);
    allPicker->setRubberBand(QwtPicker::VLineRubberBand);
    allPicker->setMousePattern(QwtEventPattern::MouseSelect1,
                                   Qt::LeftButton, Qt::ShiftModifier);

    connect(allPicker, SIGNAL(moved(const QPoint &)),
                SLOT(plotPickerMoved(const QPoint &)));
    connect(allPicker, SIGNAL(appended(const QPoint &)),
                SLOT(plotPickerSelected(const QPoint &)));

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

    QwtPlotMarker* allMarker3 = new QwtPlotMarker();
    allMarker3->setLineStyle(QwtPlotMarker::VLine);
    allMarker3->attach(allPlot);
    allPlot->allMarker3=allMarker3;


    stackFrame = new QScrollArea();
    stackFrame->hide();

    vlayout->addWidget(allPlot);
    vlayout->addWidget(stackFrame);
    vlayout->addLayout(showLayout);
    vlayout->addLayout(smoothLayout);


    setLayout(vlayout);

    connect(showPower, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setShowPower(int)));
    connect(showHr, SIGNAL(stateChanged(int)),
            this, SLOT(setShowHr(int)));
    connect(showSpeed, SIGNAL(stateChanged(int)),
            this, SLOT(setShowSpeed(int)));
    connect(showCad, SIGNAL(stateChanged(int)),
            this, SLOT(setShowCad(int)));
    connect(showAlt, SIGNAL(stateChanged(int)),
            this, SLOT(setShowAlt(int)));
    connect(showGrid, SIGNAL(stateChanged(int)),
            this, SLOT(setShowGrid(int)));
    connect(showStack, SIGNAL(stateChanged(int)),
            this, SLOT(setShowStack(int)));

    connect(stackZoomUp, SIGNAL(clicked()),
            this, SLOT(setStackZoomUp()));
    connect(stackZoomDown, SIGNAL(clicked()),
            this, SLOT(setStackZoomDown()));

    connect(comboDistance, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setByDistance(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(editingFinished()),
            this, SLOT(setSmoothingFromLineEdit()));
    connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(mainWindow, SIGNAL(intervalsChanged()), this, SLOT(intervalsChanged()));
    connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(mainWindow, SIGNAL(configChanged()), allPlot, SLOT(configChanged()));
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(configChanged()));
}

void
AllPlotWindow::configChanged()
{
    allPicker->setRubberBandPen(GColor(CPLOTSELECT));
    allPicker->setTrackerPen(GColor(CPLOTTRACKER));
}

void
AllPlotWindow::rideSelected()
{
    if (mainWindow->activeTab() != this)
        return;
    RideItem *ride = mainWindow->rideItem();
    if (!ride || ride == current)
        return;
    current = ride;

    int showit = showStack->isChecked();
    if (showit) allPlot->hide();
    else allPlot->show();

    // set it up anyway since the stack plots
    // reuse the arrays
    clearSelection(); // clear any ride interval selection data
    setAllPlotWidgets(ride);
    allPlot->setDataI(ride);
    allZoomer->setZoomBase();

    // update stacked view if that is set
    setShowStack(0); // zap whats there
    setShowStack(showit);
}

void
AllPlotWindow::zonesChanged()
{
    if (mainWindow->activeTab() != this)
        return;
    allPlot->refreshZoneLabels();
    allPlot->replot();
}

void
AllPlotWindow::intervalsChanged()
{
    if (mainWindow->activeTab() != this)
        return;
    allPlot->refreshIntervalMarkers();
    allPlot->replot();
    foreach (AllPlot *plot, allPlots) {
        plot->refreshIntervalMarkers();
        plot->replot();
    }
}

void
AllPlotWindow::intervalSelected()
{
    if (mainWindow->activeTab() != this)
        return;
    hideSelection();
}

void
AllPlotWindow::setSmoothingFromSlider()
{
    if (allPlot->smoothing() != smoothSlider->value()) {
        setSmoothing(smoothSlider->value());
        smoothLineEdit->setText(QString("%1").arg(allPlot->smoothing()));
    }
}

void
AllPlotWindow::setSmoothingFromLineEdit()
{
    int value = smoothLineEdit->text().toInt();
    if (value != allPlot->smoothing()) {
        setSmoothing(value);
        smoothSlider->setValue(value);
    }
}

void
AllPlotWindow::setAllPlotWidgets(RideItem *ride)
{
    if (ride->ride() && ride->ride()->deviceType() != QString("Manual CSV")) {
	const RideFileDataPresent *dataPresent = ride->ride()->areDataPresent();
	showPower->setEnabled(dataPresent->watts);
	showHr->setEnabled(dataPresent->hr);
	showSpeed->setEnabled(dataPresent->kph);
	showCad->setEnabled(dataPresent->cad);
	showAlt->setEnabled(dataPresent->alt);
    }
    else {
	showPower->setEnabled(false);
	showHr->setEnabled(false);
	showSpeed->setEnabled(false);
	showCad->setEnabled(false);
	showAlt->setEnabled(false);
    }
}

void
AllPlotWindow::zoomInterval(IntervalItem *which)
{
    QwtDoubleRect rect;

    if (!allPlot->byDistance()) {
        rect.setLeft(which->start/60);
        rect.setRight(which->stop/60);
    } else {
        rect.setLeft(which->startKM);
        rect.setRight(which->stopKM);
    }
    rect.setTop(allPlot->wattsCurve->maxYValue()*1.1);
    rect.setBottom(0);
    allZoomer->zoom(rect);
}

void
AllPlotWindow::plotPickerSelected(const QPoint &pos)
{
    QwtPlotPicker* pick = qobject_cast<QwtPlotPicker *>(sender());
    AllPlot* plot = qobject_cast<AllPlot *>(pick->plot());

    // set start of selection in xunits (minutes assumed for now)
    setStartSelection(plot, pos.x());
}

void
AllPlotWindow::plotPickerMoved(const QPoint &pos)
{
    QString name = QString("Selection #%1 ").arg(selection);
    QwtPlotPicker* pick = qobject_cast<QwtPlotPicker *>(sender());
    AllPlot* plot = qobject_cast<AllPlot *>(pick->plot());

    if (allPlots.length()>0) {
        int posX = pos.x();
        int posY = plot->y()+pos.y()+50;

        foreach (AllPlot *_plot, allPlots) {
            _plot->allMarker1->setValue(plot->allMarker1->value());

            if (_plot->y()<=plot->y() && posY<_plot->y()){
                if (_plot->transform(QwtPlot::xBottom, _plot->allMarker2->xValue())>0){
                    setEndSelection(_plot, 0, false, name);
                    _plot->allMarker2->setLabel(QString(""));
                }
            }
            else if (_plot->y()>=plot->y() && posY>_plot->y()+_plot->height()){
                if (_plot->transform(QwtPlot::xBottom, _plot->allMarker2->xValue())<plot->width()){
                    setEndSelection(_plot, plot->width(), false, name);
                }
            }
            else if (posY>_plot->y() && posY<_plot->y()+_plot->height()){
	            if (pos.x()<6)
	                posX = 6;
	            else if (!_plot->byDistance() && pos.x()>_plot->transform(QwtPlot::xBottom, _plot->timeArray[_plot->timeArray.size()-1]))
	                posX = _plot->transform(QwtPlot::xBottom, _plot->timeArray[_plot->timeArray.size()-1]);
	            else if (plot->byDistance() && pos.x()>_plot->transform(QwtPlot::xBottom, _plot->distanceArray[_plot->distanceArray.size()-1]))
	                posX = _plot->transform(QwtPlot::xBottom, _plot->distanceArray[_plot->distanceArray.size()-1]);

                setEndSelection(_plot, posX, true, name);
                if (plot->y()<_plot->y()) {
                    plot->allMarker1->setLabel(_plot->allMarker1->label());
                    plot->allMarker2->setLabel(_plot->allMarker2->label());
                }
            } else {//if (_plot->y()<posY+3*_plot->height() || _plot->y()<plot->y()+3*plot->height())
                //hide plot for 3 next plots
                _plot->allMarker1->hide();
                _plot->allMarker2->hide();
                _plot->allMarker3->hide();
            }
        }
    }
    else // set end of selection in xunits (minutes assumed for now)
        setEndSelection(plot, pos.x(), true, name);
}

void
AllPlotWindow::setStartSelection(AllPlot* plot, int xPosition)
{
    double xValue = plot->invTransform(QwtPlot::xBottom, xPosition);
    QwtPlotMarker* allMarker1 = plot->allMarker1;

    selection++;

    if (!allMarker1->isVisible() || allMarker1->xValue() != xValue) {

        hideSelection();
        allMarker1->setValue(xValue, plot->byDistance() ? 0 : 100);
        allMarker1->show();
    }
}

void
AllPlotWindow::setEndSelection(AllPlot* plot, int xPosition, bool newInterval, QString name)
{
    double xValue = plot->invTransform(QwtPlot::xBottom, xPosition);

    QwtPlotMarker* allMarker1 = plot->allMarker1;
    QwtPlotMarker* allMarker2 = plot->allMarker2;
    QwtPlotMarker* allMarker3 = plot->allMarker3;

    bool useMetricUnits = plot->useMetricUnits;
    if (!allMarker2->isVisible() || allMarker2->xValue() != xValue) {
        allMarker2->setValue(xValue, plot->byDistance() ? 0 : 100);
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
        double lwidth=plot->transform(QwtPlot::xBottom, x2)-plot->transform(QwtPlot::xBottom, x1);

        allMarker3->setValue((x2-x1)/2+x1, 100);
        QColor marker_color = QColor(GColor(CINTERVALHIGHLIGHTER));
        marker_color.setAlpha(64);
        allMarker3->setLinePen(QPen(marker_color, lwidth, Qt::SolidLine)) ;
        //allMarker3->setZ(-1000.0);
        allMarker3->show();

        RideFile tmpRide = RideFile();

        QTreeWidgetItem *which = mainWindow->rideItem();
        RideItem *ride = (RideItem*)which;

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
        if (plot->byDistance() && useMetricUnits == false) {
            x1 *= KM_PER_MILE;
            x2 *=  KM_PER_MILE;
        }

        foreach (const RideFilePoint *point, ride->ride()->dataPoints()) {
            if ((plot->byDistance()==true && point->km>=x1 && point->km<x2) ||
                (plot->byDistance()==false && point->secs/60>=x1 && point->secs/60<x2)) {

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
        s = s.arg(useMetricUnits ? distance2-distance1 : (distance2-distance1)*MILES_PER_KM, 0, 'f', 1);
        s = s.arg((useMetricUnits? "km":"m"));
        s = s.arg(time_to_string(duration2-duration1));
        if (duration2-duration1-secsMoving>1)
            s = s.arg("("+time_to_string(secsMoving)+")");
        else
            s = s.arg("");
        s = s.arg((useMetricUnits ? 1 : MILES_PER_KM) * (distance2-distance1)/secsMoving*3600, 0, 'f', 1);
        s = s.arg((useMetricUnits? "km/h":"m/h"));
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

        if (allMarker1->xValue()<allMarker2->xValue()) {
            allMarker1->setLabel(s);
            allMarker2->setLabel(QString(""));
        }
        else {
            allMarker2->setLabel(s);
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
            allIntervals->addChild(last);

            // now update the RideFileIntervals and all the plots etc
            mainWindow->updateRideFileIntervals();
        }
    }
}

void
AllPlotWindow::clearSelection()
{
    selection = 0;
    hideSelection();
}

void
AllPlotWindow::hideSelection()
{
    allPlot->allMarker1->setVisible(false);
    allPlot->allMarker2->setVisible(false);
    allPlot->allMarker3->setVisible(false);
    allPlot->replot();

    foreach (AllPlot *plot, allPlots) {
        plot->allMarker1->setVisible(false);
        plot->allMarker2->setVisible(false);
        plot->allMarker3->setVisible(false);
        plot->replot();
    }
}

void
AllPlotWindow::setShowPower(int value)
{
    allPlot->showPower(value);
    if (allPlots.count()>0)
        resetStackedDatas();
}

void
AllPlotWindow::setShowHr(int value)
{
    allPlot->showHr(value);
    foreach (AllPlot *plot, allPlots)
        plot->showHr(value);
}

void
AllPlotWindow::setShowSpeed(int value)
{
    allPlot->showSpeed(value);
    foreach (AllPlot *plot, allPlots)
        plot->showSpeed(value);
}

void
AllPlotWindow::setShowCad(int value)
{
    allPlot->showCad(value);
    foreach (AllPlot *plot, allPlots)
        plot->showCad(value);
}

void
AllPlotWindow::setShowAlt(int value)
{
    allPlot->showAlt(value);
    foreach (AllPlot *plot, allPlots)
        plot->showAlt(value);
}

void
AllPlotWindow::setShowGrid(int value)
{
    allPlot->showGrid(value);
    foreach (AllPlot *plot, allPlots)
        plot->showGrid(value);
}

void
AllPlotWindow::setByDistance(int value)
{
    allPlot->setByDistance(value);
    if (allPlots.count()>0) {
        setShowStack(false);
        setShowStack(true);
    }
}

void
AllPlotWindow::setSmoothing(int value)
{
    allPlot->setSmoothing(value);

    if (allPlots.count()>0)
        resetStackedDatas();
}

void
AllPlotWindow::resetStackedDatas()
{

    int _stackWidth = stackWidth;
    if (allPlot->byDistance())
        _stackWidth = stackWidth/3;

    for(int i = 0 ; i < allPlots.count() ; i++)
    {
        AllPlot *plot = allPlots[i];
        int startIndex, stopIndex;
        if (plot->byDistance()) {
            startIndex = allPlot->rideItem->ride()->distanceIndex((allPlot->useMetricUnits ? 1 : KM_PER_MILE) * _stackWidth*i);
            stopIndex  = allPlot->rideItem->ride()->distanceIndex((allPlot->useMetricUnits ? 1 : KM_PER_MILE) * _stackWidth*(i+1));
        } else {
            startIndex = allPlot->rideItem->ride()->timeIndex(60*_stackWidth*i);
            stopIndex  = allPlot->rideItem->ride()->timeIndex(60*_stackWidth*(i+1));
        }

        // Update AllPlot for stacked view
        plot->setDataP(allPlot, startIndex, stopIndex);
    }
}

void
AllPlotWindow::setStackZoomUp()
{
    if (stackWidth<200 && stackWidth<allPlot->rideItem->ride()->dataPoints().last()->secs/60) {
        stackWidth = ceil(stackWidth * 1.25);
        stackZoomDown->setEnabled(true);
        setShowStack(false);
        setShowStack(true);
        if (stackWidth>=200  || stackWidth>=allPlot->rideItem->ride()->dataPoints().last()->secs/60)
            stackZoomUp->setEnabled(false);
    }
}

void
AllPlotWindow::setStackZoomDown()
{
    if (stackWidth>4) {
        stackWidth = floor(stackWidth / 1.25);
        stackZoomUp->setEnabled(true);
        setShowStack(false);
        setShowStack(true);
        if (stackWidth<=4)
            stackZoomDown->setEnabled(false);
    }
}

void
AllPlotWindow::setShowStack(int value)
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    settings->setValue(GC_RIDE_PLOT_STACK, value);
    if (value) {
        stackZoomUp->setEnabled(true);
        stackZoomDown->setEnabled(true);

        int _stackWidth = stackWidth;
        if (allPlot->byDistance())
            _stackWidth = stackWidth/3;

        allPlot->hide();

        QVBoxLayout *vLayout = new QVBoxLayout();
        RideItem* rideItem = allPlot->rideItem;

        // don't try and plot for null files
        if (!rideItem || !rideItem->ride() || rideItem->ride()->dataPoints().isEmpty()) return;

        double duration = rideItem->ride()->dataPoints().last()->secs;
        double distance =  (allPlot->useMetricUnits ? 1 : MILES_PER_KM) * rideItem->ride()->dataPoints().last()->km;
        int nbplot;

        if (allPlot->byDistance())
            nbplot = (int)floor(distance/_stackWidth)+1;
        else
            nbplot = (int)floor(duration/_stackWidth/60)+1;

        for(int i = 0 ; i < nbplot ; i++)
        {
            AllPlot *_allPlot = new AllPlot(this, mainWindow);
            allPlots.append(_allPlot);
            addPickers(_allPlot);

            int startIndex, stopIndex;
            if (allPlot->byDistance()) {
                startIndex = allPlot->rideItem->ride()->distanceIndex((allPlot->useMetricUnits ? 1 : KM_PER_MILE) * _stackWidth*i);
                stopIndex  = allPlot->rideItem->ride()->distanceIndex((allPlot->useMetricUnits ? 1 : KM_PER_MILE) * _stackWidth*(i+1));
            } else {
                startIndex = allPlot->rideItem->ride()->timeIndex(60*_stackWidth*i);
                stopIndex  = allPlot->rideItem->ride()->timeIndex(60*_stackWidth*(i+1));
            }
            vLayout->addWidget(_allPlot);

            // Update AllPlot for stacked view
            _allPlot->setDataP(allPlot, startIndex, stopIndex);
            _allPlot->setAxisScale(QwtPlot::xBottom, _stackWidth*i, _stackWidth*(i+1), 15/stackWidth);

            if (i==0){
                // First plot view title and legend
                _allPlot->setTitle(allPlot->title());
                _allPlot->plotLayout()->setLegendPosition(QwtPlot::TopLegend);
                _allPlot->setFixedHeight(120+stackWidth*2+50);
            }
            else {
               _allPlot->setTitle("");
               _allPlot->insertLegend(NULL);
               _allPlot->setFixedHeight(120+stackWidth*2);
            }

            // No x axis titles
            _allPlot->setAxisTitle(QwtPlot::xBottom,NULL);
            // Smaller y axis Titles
            QFont axisFont = QFont("Helvetica",10, QFont::Normal);
            QwtText text = _allPlot->axisTitle(QwtPlot::yLeft);
            text.setFont(axisFont);
            _allPlot->setAxisTitle(QwtPlot::yLeft,text);
            text = _allPlot->axisTitle(QwtPlot::yLeft2);
            text.setFont(axisFont);
            _allPlot->setAxisTitle(QwtPlot::yLeft2,text);
            text = _allPlot->axisTitle(QwtPlot::yRight);
            text.setFont(axisFont);
            _allPlot->setAxisTitle(QwtPlot::yRight,text);
            text = _allPlot->axisTitle(QwtPlot::yRight2);
            text.setFont(axisFont);
            _allPlot->setAxisTitle(QwtPlot::yRight2,text);
        }

        QWidget *stack = new QWidget();
        stack->setLayout(vLayout);

        stackFrame->setWidgetResizable(true);
        stackFrame->setWidget(stack);
        stackFrame->show();
    } else {
        stackFrame->hide();
        allPlot->show();

        if (allPlots.count()>0) {
            foreach (AllPlot *plot, allPlots) {
                //layout()->removeWidget(plot);
                delete plot;
                allPlots.removeOne(plot);
            }
        }
    }
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

    QwtPlotMarker* allMarker3 = new QwtPlotMarker();
    allMarker3->setLineStyle(QwtPlotMarker::VLine);
    allMarker3->attach(_allPlot);
    _allPlot->allMarker3 = allMarker3;

    // Interval selection 2
    QwtPlotPicker* allPicker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                                  QwtPicker::RectSelection | QwtPicker::CornerToCorner|QwtPicker::DragSelection,
                                   QwtPicker::NoRubberBand,
                                   QwtPicker::ActiveOnly, _allPlot->canvas());
    allPickers.append(allPicker);
    allPicker->setRubberBand(QwtPicker::NoRubberBand);
    // now select rectangles
    allPicker->setSelectionFlags(QwtPicker::PointSelection | QwtPicker::RectSelection | QwtPicker::DragSelection);
    allPicker->setMousePattern(QwtEventPattern::MouseSelect1,
                                   Qt::LeftButton, Qt::ShiftModifier);


    allPicker->setRubberBandPen(GColor(CPLOTSELECT));
    allPicker->setTrackerPen(GColor(CPLOTTRACKER));

    //void appended(const QwtPlotPicker* pick, const QwtDoublePoint &pos);
    connect(allPicker, SIGNAL(appended(const QPoint &)),
                SLOT(plotPickerSelected(const QPoint &)));
    connect(allPicker, SIGNAL(moved(const QPoint &)),
                SLOT(plotPickerMoved(const QPoint &)));

}

