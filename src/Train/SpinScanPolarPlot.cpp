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
#include "SpinScanPolarPlot.h"
#include <stdint.h>
#include "Colors.h"
#include "Context.h"

// to avoid excessive floating point operations when updating the display
// this is a precomputed value for cos(i*15) * (PI / 180)
static const double cosx[24] = {
    1.000000, 0.965926, 0.866026, 0.707107, 0.500001, 0.258820,
    0.000000, -0.258818, -0.499999, -0.707106, -0.866024, -0.965925,
    -1.000000, -0.965926, -0.866027, -0.707109, -0.500003, -0.258822,
    -0.000004, 0.258815, 0.499997, 0.707104, 0.866023, 0.965925
};

// this is a precomputed value for sin(i*15) * (PI / 180)
static const double siny[24] = {
    0.000000, 0.258819, 0.500000, 0.707106, 0.866025, 0.965926,
    1.000000, 0.965926, 0.866026, 0.707108, 0.500002, 0.258821,
    0.000000, -0.258817, -0.499998, -0.707105, -0.866024, -0.965925,
    -1.000000, -0.965927, -0.866027, -0.707110, -0.500004, -0.258823
};

// SpinScan force/direction to cartesian for plotting
size_t SpinScanPolarData::size() const { return 13; }
//QwtData *SpinScanPolarData::copy() const { return new SpinScanPolarData(spinData, isleft); }
void SpinScanPolarData::init() { }
double SpinScanPolarData::x(size_t i) const { i += isleft?0:12; if (i>=24) i=0; return cosx[i] * (double)spinData[i]; }
double SpinScanPolarData::y(size_t i) const { i += isleft?0:12; if (i>=24) i=0; return siny[i] * (double)spinData[i]; }

QPointF SpinScanPolarData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF SpinScanPolarData::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}

// Original code before moving to precomputed constants
//double SpinScanPolarData::x(size_t i) const { if (i==24) i=0; return cos((i*15)*3.14159f/180.00f) * (double)spinData[i]; }
//double SpinScanPolarData::y(size_t i) const { if (i==24) i=0; return sin((i*15)*3.14159f/180.00f) * (double)spinData[i]; }

SpinScanPolarPlot::SpinScanPolarPlot(QWidget *parent, uint8_t *spinData) : QwtPlot(parent), leftCurve(NULL), rightCurve(NULL), spinData(spinData)
{
    // Setup the axis
    setAxisTitle(QwtAxis::YLeft, "SpinScan");
    setAxisMaxMinor(QwtAxis::XBottom, 0);
    setAxisMaxMinor(QwtAxis::YLeft, 0);

    QPalette pal;
    setAxisScale(QwtAxis::YLeft, -90, 90); // max 8 bit plus a little
    setAxisScale(QwtAxis::XBottom, -90, 90); // max 8 bit plus a little
    pal.setColor(QPalette::WindowText, GColor(GCol::SPINSCANLEFT));
    pal.setColor(QPalette::Text, GColor(GCol::SPINSCANLEFT));
    axisWidget(QwtAxis::YLeft)->setPalette(pal);
    axisWidget(QwtAxis::YLeft)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    setAxisVisible(QwtAxis::XBottom, false); // very little value and some cpu overhead
    setAxisVisible(QwtAxis::YLeft, false);

    rightCurve = new QwtPlotCurve("SpinScan Right");
    rightCurve->setRenderHint(QwtPlotItem::RenderAntialiased); // too cpu intensive
    rightCurve->attach(this);
    rightCurve->setYAxis(QwtAxis::YLeft);
    rightSpinScanPolarData = new SpinScanPolarData(spinData, false);

    leftCurve = new QwtPlotCurve("SpinScan Left");
    leftCurve->setRenderHint(QwtPlotItem::RenderAntialiased); // too cpu intensive
    leftCurve->attach(this);
    leftCurve->setYAxis(QwtAxis::YLeft);
    leftSpinScanPolarData = new SpinScanPolarData(spinData, true);

    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);
    configChanged(CONFIG_APPEARANCE); // set colors
}

void
SpinScanPolarPlot::setAxisTitle(int axis, QString label)
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
SpinScanPolarPlot::configChanged(qint32)
{
    setCanvasBackground(GColor(GCol::TRAINPLOTBACKGROUND));

    QColor col = GColor(GCol::SPINSCANRIGHT);
    col.setAlpha(120);
    QBrush brush = QBrush(col);
    QPen pen(col);
    pen.setWidth(4); // thicker pen
    rightCurve->setBrush(Qt::NoBrush);
    rightCurve->setPen(pen);
    rightCurve->setStyle(QwtPlotCurve::Lines);
    rightCurve->setData(rightSpinScanPolarData);

    QColor col2 = GColor(GCol::SPINSCANLEFT);
    col2.setAlpha(120);
    QBrush brush2 = QBrush(col2);
    QPen pen2(col2);
    pen2.setWidth(4); // thicker pen
    leftCurve->setBrush(Qt::NoBrush);
    leftCurve->setPen(pen2);
    leftCurve->setStyle(QwtPlotCurve::Lines);
    leftCurve->setData(leftSpinScanPolarData);
}
