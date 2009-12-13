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
#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>

AllPlotWindow::AllPlotWindow(MainWindow *mainWindow) :
    QWidget(mainWindow), mainWindow(mainWindow)
{
    QVBoxLayout *vlayout = new QVBoxLayout;

    QHBoxLayout *showLayout = new QHBoxLayout;
    QLabel *showLabel = new QLabel("Show:", this);
    showLayout->addWidget(showLabel);

    QCheckBox *showGrid = new QCheckBox("Grid", this);
    showGrid->setCheckState(Qt::Checked);
    showLayout->addWidget(showGrid);

    showHr = new QCheckBox("Heart Rate", this);
    showHr->setCheckState(Qt::Checked);
    showLayout->addWidget(showHr);

    showSpeed = new QCheckBox("Speed", this);
    showSpeed->setCheckState(Qt::Checked);
    showLayout->addWidget(showSpeed);

    showCad = new QCheckBox("Cadence", this);
    showCad->setCheckState(Qt::Checked);
    showLayout->addWidget(showCad);

    showAlt = new QCheckBox("Altitude", this);
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
    allPicker->setRubberBandPen(QColor(Qt::blue));
    allPicker->setRubberBand(QwtPicker::CrossRubberBand);
    allPicker->setTrackerPen(QColor(Qt::blue));
    // now select rectangles
    allPicker->setSelectionFlags(QwtPicker::PointSelection | QwtPicker::RectSelection | QwtPicker::DragSelection);
    allPicker->setRubberBand(QwtPicker::VLineRubberBand);
    allPicker->setMousePattern(QwtEventPattern::MouseSelect1,
                                   Qt::LeftButton, Qt::ShiftModifier);

    connect(allPicker, SIGNAL(moved(const QPoint &)),
                SLOT(plotPickerMoved(const QPoint &)));
    connect(allPicker, SIGNAL(appended(const QPoint &)),
                SLOT(plotPickerSelected(const QPoint &)));

    allMarker1 = new QwtPlotMarker();
    allMarker1->setLineStyle(QwtPlotMarker::VLine);
    allMarker1->attach(allPlot);
    allMarker1->setLabelAlignment(Qt::AlignTop|Qt::AlignRight);

    allMarker2 = new QwtPlotMarker();
    allMarker2->setLineStyle(QwtPlotMarker::VLine);
    allMarker2->attach(allPlot);

    allMarker3 = new QwtPlotMarker();
    allMarker3->setLineStyle(QwtPlotMarker::VLine);
    allMarker3->attach(allPlot);

    vlayout->addWidget(allPlot);
    vlayout->addLayout(showLayout);
    vlayout->addLayout(smoothLayout);
    setLayout(vlayout);

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
    connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(mainWindow, SIGNAL(intervalsChanged()), this, SLOT(intervalsChanged()));
    connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
}

void
AllPlotWindow::rideSelected()
{
    RideItem *ride = mainWindow->rideItem();
    if (!ride)
        return;
    clearSelection(); // clear any ride interval selection data
    setAllPlotWidgets(ride);
    allPlot->setData(ride);
    allZoomer->setZoomBase();
}

void
AllPlotWindow::zonesChanged()
{
    allPlot->refreshZoneLabels();
    allPlot->replot();
}

void
AllPlotWindow::intervalsChanged()
{
    allPlot->refreshIntervalMarkers();
    allPlot->replot();
}

void
AllPlotWindow::intervalSelected()
{
    hideSelection();
    allPlot->replot();
}

void
AllPlotWindow::setSmoothingFromSlider()
{
    if (allPlot->smoothing() != smoothSlider->value()) {
        allPlot->setSmoothing(smoothSlider->value());
        smoothLineEdit->setText(QString("%1").arg(allPlot->smoothing()));
    }
}

void
AllPlotWindow::setSmoothingFromLineEdit()
{
    int value = smoothLineEdit->text().toInt();
    if (value != allPlot->smoothing()) {
        allPlot->setSmoothing(value);
        smoothSlider->setValue(value);
    }
}

void
AllPlotWindow::setAllPlotWidgets(RideItem *ride)
{
    if (ride->ride && ride->ride->deviceType() != QString("Manual CSV")) {
	const RideFileDataPresent *dataPresent = ride->ride->areDataPresent();
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
    // set start of selection in xunits (minutes assumed for now)
    setStartSelection(allPlot->invTransform(QwtPlot::xBottom, pos.x()));
}

void
AllPlotWindow::plotPickerMoved(const QPoint &pos)
{
    QString name = QString("Selection #%1").arg(selection);
    // set end of selection in xunits (minutes assumed for now)
    setEndSelection(allPlot->invTransform(QwtPlot::xBottom, pos.x()), true, name);
}

void
AllPlotWindow::setStartSelection(double xValue)
{
    selection++;

    if (!allMarker1->isVisible() || allMarker1->xValue() != xValue) {

        allMarker1->hide();
        allMarker2->hide();
        allMarker3->hide();
        allMarker1->setValue(xValue, allPlot->byDistance() ? 0 : 100);
        allMarker1->show();
    }
}

void
AllPlotWindow::setEndSelection(double xValue, bool newInterval, QString name)
{
    bool useMetricUnits = allPlot->useMetricUnits;
    if (!allMarker2->isVisible() || allMarker2->xValue() != xValue) {
        allMarker2->setValue(xValue, allPlot->byDistance() ? 0 : 100);
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
        double lwidth=allPlot->transform(QwtPlot::xBottom, x2)-allPlot->transform(QwtPlot::xBottom, x1);

        allMarker3->setValue((x2-x1)/2+x1, 100);
        QColor marker_color = QColor(Qt::blue);
        marker_color.setAlpha(64);
        allMarker3->setLinePen(QPen(QBrush(marker_color), lwidth, Qt::SolidLine)) ;
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
        if (allPlot->byDistance() && useMetricUnits == false) {
            x1 *= KM_PER_MILE;
            x2 *=  KM_PER_MILE;
        }

        foreach (const RideFilePoint *point, ride->ride->dataPoints()) {
            if ((allPlot->byDistance()==true && point->km>=x1 && point->km<x2) ||
                (allPlot->byDistance()==false && point->secs/60>=x1 && point->secs/60<x2)) {

                if (distance1 == -1) distance1 = point->km;
                distance2 = point->km;

                if (duration1 == -1) duration1 = point->secs;
                duration2 = point->secs;

                if (point->kph > 0.0)
                   secsMoving += ride->ride->recIntSecs();
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

        allMarker1->setLabel(s);

        if (newInterval) {

            QTreeWidgetItem *allIntervals = mainWindow->mutableIntervalItems();
            int count = allIntervals->childCount();

            // are we adjusting an existing interval? - if so delete it and readd it
            if (count > 0) {
                IntervalItem *bottom = (IntervalItem *) allIntervals->child(count-1);
                if (bottom->text(0) == name) delete allIntervals->takeChild(count-1);
            }

            QTreeWidgetItem *last = new IntervalItem(ride->ride, name, duration1, duration2, distance1, distance2);
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
    allMarker1->setVisible(false);
    allMarker2->setVisible(false);
    allMarker3->setVisible(false);
    allPlot->replot();
}

