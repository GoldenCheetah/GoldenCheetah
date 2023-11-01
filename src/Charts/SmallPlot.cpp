/*
 * Copyright (c) 2011 Damien Grauser
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

#include "SmallPlot.h"
#include "RideFile.h"
#include "RideItem.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"

#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>
#include <qwt_compat.h>


SmallPlotPicker::SmallPlotPicker(QWidget *canvas) : QwtPlotPicker(canvas)
{
}


QwtText
SmallPlotPicker::trackerText(const QPoint &point) const
{
    QPointF pointF = invTransform(point);
    if (pointF.x() < 0) {
        return QwtText("");
    }
    int intNormX = pointF.x();
    int hours = intNormX / 60;
    int mins = intNormX % 60;
    int secs = (pointF.x() - intNormX) * 60;
    bool firstLine = true;

    QString text;
    QwtPlotItemList plotItems = plot()->itemList(QwtPlotItem::Rtti_PlotCurve);
    foreach (QwtPlotItem *plotItem, plotItems) {
        QwtPlotCurve *plotCurve = static_cast<QwtPlotCurve *>(plotItem);
        int idx = 0;
        size_t size = plotCurve->data()->size();
        double dist = 100000;
        for (size_t i = 0; i < size; ++i) {
            QPointF nextPoint = plotCurve->data()->sample(i);
            double newDist = fabs(nextPoint.x() - pointF.x());
            if (newDist <= dist) {
                idx = i;
                dist = newDist;
            } else {
                QPointF thisPoint = plotCurve->data()->sample(idx);
                if (thisPoint.y() > 0) {
                    if (! firstLine) {
                        text.append("\n");
                    }
                    text.append(QString("%1: %2").arg(plotCurve->title().text())
                                                 .arg(int(thisPoint.y())));
                    firstLine = false;
                }
                break;
            }
        }
    }
    if (! firstLine) {
        text.append("\n");
    }
    text.append(QString("%1:%2:%3").arg(hours)
                                   .arg(mins, 2, 10, QChar('0'))
                                   .arg(secs, 2, 10, QChar('0')));

    QwtText tooltip(text);
    QFont stGiles;
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());
    stGiles.setWeight(QFont::Bold);
    tooltip.setFont(stGiles);

    tooltip.setBackgroundBrush(QBrush(GColor(CPLOTMARKER)));
    tooltip.setColor(GColor(CRIDEPLOTBACKGROUND));
    tooltip.setBorderRadius(6);
    tooltip.setRenderFlags(Qt::AlignCenter | Qt::AlignVCenter);

    return tooltip;
}


SmallPlot::SmallPlot(QWidget *parent) : QwtPlot(parent), d_mrk(NULL), smooth(30), tracking(false)
{
    setCanvasBackground(GColor(CPLOTBACKGROUND));
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    setXTitle();
    setAxesCount(QwtAxis::yLeft, 2);

    altCurve = new QwtPlotCurve(tr("Altitude"));
    altCurve->setPen(QPen(GColor(CALTITUDE)));
    QColor brush_color = GColor(CALTITUDEBRUSH);
    brush_color.setAlpha(180);
    altCurve->setBrush(brush_color);
    altCurve->setYAxis(QwtAxisId(QwtAxis::yLeft,1));
    altCurve->attach(this);

    wattsCurve = new QwtPlotCurve("Power");
    //timeCurves.resize(36);// wattsCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    wattsCurve->setYAxis(QwtAxisId(QwtAxis::yLeft,0));
    wattsCurve->setPen(QPen(GColor(CPOWER)));
    wattsCurve->attach(this);

    hrCurve = new QwtPlotCurve("Heart Rate");
    // hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hrCurve->setYAxis(QwtAxisId(QwtAxis::yLeft,0));
    hrCurve->setPen(QPen(GColor(CHEARTRATE)));
    hrCurve->attach(this);

    // grid lines on such a small plot look AWFUL
    //grid = new QwtPlotGrid();
    //grid->enableX(false);
    //QPen gridPen;
    //gridPen.setStyle(Qt::DotLine);
    //grid->setPen(QPen(GColor(CPLOTGRID)));
    //grid->attach(this);

    //timeCurves.resize(36);
    //for (int i = 0; i < 36; ++i) {
        //QColor color = QColor(255,255,255);
        //color.setHsv(60+i*(360/36), 255,255,255);

        //QPen pen = QPen(color);
        //pen.setWidth(3);

        //timeCurves[i] = new QwtPlotCurve();
        //timeCurves[i]->setPen(pen);
        //timeCurves[i]->setStyle(QwtPlotCurve::Lines);
        //timeCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased);
        //timeCurves[i]->attach(this);
        //QwtLegend *legend = new QwtLegend;
        //legend->setVisible(false);
        //legend->setDisabled(true);
        //timeCurves[i]->updateLegend(legend);
     //}
}

struct DataPoint {
    double time, hr, alt, watts;
    int inter;
    DataPoint(double t, double h, double w, double a, int i) :
             time(t), hr(h), alt(a), watts(w), inter(i) {}
};

void
SmallPlot::enableTracking()
{
    tracking = true;
    QwtPlotPicker *picker = new SmallPlotPicker(canvas());
    picker->setTrackerMode(QwtPlotPicker::ActiveOnly);
    picker->setStateMachine(new QwtPickerTrackerMachine());
    picker->setRubberBand(QwtPicker::VLineRubberBand);
    picker->setRubberBandPen(QPen(GColor(CPLOTMARKER)));
    connect(picker, SIGNAL(moved(const QPoint&)), this, SLOT(pointMoved(const QPoint&)));
}

void
SmallPlot::recalc()
{
    if (!timeArray.size()) return;

    long rideTimeSecs = (long) ceil(timeArray[arrayLength - 1]);
    if (rideTimeSecs < 0 || rideTimeSecs > SECONDS_IN_A_WEEK) {
        QwtArray<double> data;
        wattsCurve->setSamples(data, data);
        hrCurve->setSamples(data, data);
        altCurve->setSamples(data, data);
        return;
    }

    double totalWatts = 0.0;
    double totalHr = 0.0;
    double totalAlt = 0.0;
    QList<DataPoint*> list;
    int i = 0;

    QVector<double> smoothWatts(rideTimeSecs + 1);
    QVector<double> smoothHr(rideTimeSecs + 1);
    QVector<double> smoothAlt(rideTimeSecs + 1);
    QVector<double> smoothTime(rideTimeSecs + 1);

    QList<double> interList; //Just store the time that it happened.
                             //Intervals are sequential.

    int lastInterval = 0; //Detect if we hit a new interval

    for (int secs = 0; ((secs < smooth) && (secs < rideTimeSecs)); ++secs) {
        smoothWatts[secs] = 0.0;
        smoothHr[secs]    = 0.0;
        smoothAlt[secs]    = 0.0;
    }
    for (int secs = smooth; secs <= rideTimeSecs; ++secs) {
        while ((i < arrayLength) && (timeArray[i] <= secs)) {
            DataPoint *dp =
                new DataPoint(timeArray[i], hrArray[i], wattsArray[i], altArray[i], interArray[i]);
            totalWatts += wattsArray[i];
            totalHr    += hrArray[i];
            totalAlt    += altArray[i];
            list.append(dp);
            //Figure out when and if we have a new interval..
            if(lastInterval != interArray[i]) {
                lastInterval = interArray[i];
                interList.append(secs/60.0);
            }
            ++i;
        }
        while (!list.empty() && (list.front()->time < secs - smooth)) {
            DataPoint *dp = list.front();
            list.removeFirst();
            totalWatts -= dp->watts;
            totalHr    -= dp->hr;
            totalAlt    -= dp->alt;
            delete dp;
        }
        // TODO: this is wrong.  We should do a weighted average over the
        // seconds represented by each point...
        if (list.empty()) {
            smoothWatts[secs] = 0.0;
            smoothHr[secs]    = 0.0;
            smoothAlt[secs]    = 0.0;
        }
        else {
            smoothWatts[secs]    = totalWatts / list.size();
            smoothHr[secs]       = totalHr / list.size();
            smoothAlt[secs]       = totalAlt / list.size();
        }
        smoothTime[secs]  = secs / 60.0;
    }
    wattsCurve->setSamples(smoothTime.constData(), smoothWatts.constData(), rideTimeSecs + 1);
    hrCurve->setSamples(smoothTime.constData(), smoothHr.constData(), rideTimeSecs + 1);
    altCurve->setSamples(smoothTime.constData(), smoothAlt.constData(), rideTimeSecs + 1);
    setAxisScale(xBottom, 0.0, smoothTime[rideTimeSecs]);

    setYMax();
    replot();
}

void
SmallPlot::setYMax()
{
    double ymax = 0;
    double y1max = 500;
    double y1min = 500;
    QString ylabel = "";
    QString y1label = "";
    if (wattsCurve->isVisible()) {
        ymax = std::max(ymax, wattsCurve->maxYValue());
        ylabel += QString((ylabel == "") ? "" : " / ") + tr("Watts");
    }
    if (hrCurve->isVisible()) {
        ymax = std::max(ymax, hrCurve->maxYValue());
        ylabel += QString((ylabel == "") ? "" : " / ") + tr("BPM");
    }
    if (altCurve->isVisible()) {
        size_t size = altCurve->data()->size();
        double curMinY = 10000;
        double curMaxY = 0;
        for (size_t i = 0; i < size; ++i) {
            double nextY = altCurve->data()->sample(i).y();
            if (nextY > 0.0) {
                curMinY = std::min(curMinY, nextY);
                curMaxY = std::max(curMaxY, nextY);
            }
        }
        y1min = curMinY;
        y1max = curMaxY;
        y1label = "m";
    }
    setAxisScale(QwtAxisId(QwtAxis::yLeft,0), 0.0, ymax * 1.1);
    setAxisTitle(QwtAxisId(QwtAxis::yLeft,0), ylabel);
    setAxisScale(QwtAxisId(QwtAxis::yLeft,1), y1min * 0.9, y1max * 1.1);
    setAxisTitle(QwtAxisId(QwtAxis::yLeft,1), y1label);
    setAxisVisible(QwtAxisId(QwtAxis::yLeft,0), false); // hide for a small plot
    setAxisVisible(QwtAxisId(QwtAxis::yLeft,1), false); // hide for a small plot
}

void
SmallPlot::setXTitle()
{
    setAxisTitle(xBottom, tr("Time (minutes)"));
    enableAxis(xBottom, true);
}

void
SmallPlot::setAxisTitle(QwtAxisId axis, QString label)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(label);
    title.setFont(stGiles);
    QwtPlot::setAxisFont(axis, stGiles);
    QwtPlot::setAxisTitle(axis, title);
}


bool
SmallPlot::hasTracking() const
{
    return tracking;
}

void
SmallPlot::setData(RideItem *rideItem)
{
    RideFile *ride = rideItem->ride();
    setData(ride);
}

void
SmallPlot::setData(RideFile *ride)
{

    wattsArray.resize(ride->dataPoints().size());
    hrArray.resize(ride->dataPoints().size());
    altArray.resize(ride->dataPoints().size());
    timeArray.resize(ride->dataPoints().size());
    interArray.resize(ride->dataPoints().size());

    arrayLength = 0;
    foreach (const RideFilePoint *point, ride->dataPoints()) {
        timeArray[arrayLength]  = point->secs;
        wattsArray[arrayLength] = std::max(0., point->watts);
        hrArray[arrayLength]    = std::max(0., point->hr);
        altArray[arrayLength]   = std::max(0., point->alt);
        interArray[arrayLength] = point->interval;
        ++arrayLength;
    }
    recalc();
}

void
SmallPlot::showPower(int state)
{
    wattsCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
SmallPlot::showHr(int state)
{
    hrCurve->setVisible(state == Qt::Checked);
    setYMax();
    replot();
}

void
SmallPlot::setSmoothing(int value)
{
    smooth = value;
    recalc();
}

void
SmallPlot::pointMoved(const QPoint &pos)
{
    double dataPosX = invTransform(QwtAxis::xBottom, pos.x());
    emit selectedPosX(dataPosX);
}


void
SmallPlot::leaveEvent(QEvent *event)
{
    emit mouseLeft();
    QWidget::leaveEvent(event);
}
