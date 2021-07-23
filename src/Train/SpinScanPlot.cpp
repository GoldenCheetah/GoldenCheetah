/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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


#include <QDebug>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include "SpinScanPlot.h"
#include <stdint.h>
#include "Colors.h"
#include "Context.h"

// lazy way of mapping x-axis to point value, proly could work it out algorithmically,
// but the irony is this is probably faster anyway since its a straight lookup.
// ok, thats my excuse ;)
static const double xspin[97] = {
    0,0,0,1,1,1.1,1.1,2,2,2.1,2.1,3,3,3.1,3.1,4,4,4.1,4.1,5,5,5.1,5.1,6,6,6.1,6.1,7,7,7.1,7.1,
    8,8,8.1,8.1,9,9,9.1,9.1,10,10,10.1,10.1,11,11,11.1,11.1,12,12,12.1,12.1,13,13,13.1,13.1,14,14,14.1,14.1,
    15,15,15.1,15.1,16,16,16.1,16.1,17,17,17.1,17.1,18,18,18.1,18.1,19,19,19.1,19.1,20,20,20.1,20.1,21,21,21.1,21.1,
    22,22,22.1,22.1,23,23,23.1,23.1,24,24
};

// Power history
double SpinScanData::x(size_t i) const { return xspin[i+(isleft?0:48)]; }
double SpinScanData::y(size_t i) const { return (i%4 == 2 || i%4 == 3) ? spinData[(i+(isleft?0:48))/4] : 0; }
size_t SpinScanData::size() const { return 48; }

QPointF SpinScanData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF SpinScanData::boundingRect() const
{
    return QRectF(0,0,0,0);
}

void SpinScanData::init() { }

SpinScanPlot::SpinScanPlot(QWidget *parent, uint8_t *spinData) : QwtPlot(parent), leftCurve(NULL), rightCurve(NULL), spinData(spinData)
{
    // Setup the axis
    setAxisTitle(YLeft, "SpinScan");
    setAxisMaxMinor(XBottom, 0);
    setAxisMaxMinor(YLeft, 0);

    QPalette pal;
    setAxisScale(YLeft, 0, 90); // max 8 bit plus a little
    setAxisScale(XBottom, 0, 24); // max 8 bit plus a little
    pal.setColor(QPalette::WindowText, GColor(CSPINSCANLEFT));
    pal.setColor(QPalette::Text, GColor(CSPINSCANLEFT));
    axisWidget(QwtAxis::YLeft)->setPalette(pal);
    axisWidget(QwtAxis::YLeft)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    enableAxis(XBottom, false); // very little value and some cpu overhead
    enableAxis(YLeft, true);

    // 30s Power curve
    rightCurve = new QwtPlotCurve("SpinScan Left");
    rightCurve->setRenderHint(QwtPlotItem::RenderAntialiased); // too cpu intensive
    rightCurve->attach(this);
    rightCurve->setYAxis(QwtAxis::YLeft);
    leftCurve = new QwtPlotCurve("SpinScan Right");
    leftCurve->setRenderHint(QwtPlotItem::RenderAntialiased); // too cpu intensive
    leftCurve->attach(this);
    leftCurve->setYAxis(QwtAxis::YLeft);

    leftSpinScanData = new SpinScanData(spinData, true);
    rightSpinScanData = new SpinScanData(spinData, false);

    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);
    configChanged(CONFIG_APPEARANCE); // set colors
}

void
SpinScanPlot::setAxisTitle(int axis, QString label)
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

void
SpinScanPlot::configChanged(qint32)
{
    setCanvasBackground(GColor(CTRAINPLOTBACKGROUND));

    QColor col = GColor(CSPINSCANLEFT);
    col.setAlpha(120);
    QBrush brush = QBrush(col);
    leftCurve->setBrush(brush);
    leftCurve->setPen(QPen(Qt::NoPen));
    //spinCurve->setStyle(QwtPlotCurve::Steps);
    leftCurve->setData(leftSpinScanData);

    QColor col2 = GColor(CSPINSCANRIGHT);
    col2.setAlpha(120);
    QBrush brush2 = QBrush(col2);
    rightCurve->setBrush(brush2);
    rightCurve->setPen(QPen(Qt::NoPen));
    //spinCurve->setStyle(QwtPlotCurve::Steps);
    rightCurve->setData(rightSpinScanData);
}
