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


#include <assert.h>
#include <QDebug>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include "SpinScanPlot.h"
#include <stdint.h>
#include "Colors.h"

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
double SpinScanData::x(size_t i) const { return xspin[i]; }
double SpinScanData::y(size_t i) const { return (i%4 == 2 || i%4 == 3) ? spinData[i/4] : 0; }
size_t SpinScanData::size() const { return 97; }
QwtData *SpinScanData::copy() const { return new SpinScanData(spinData); }
void SpinScanData::init() { }

SpinScanPlot::SpinScanPlot(uint8_t *spinData) : spinCurve(NULL), spinData(spinData)
{
    setInstanceName("SpinScan Plot");

    // Setup the axis
    setAxisTitle(yLeft, "SpinScan");
    setAxisMaxMinor(xBottom, 0);
    setAxisMaxMinor(yLeft, 0);

    QPalette pal;
    setAxisScale(yLeft, 0, 90); // max 8 bit plus a little
    setAxisScale(xBottom, 0, 24); // max 8 bit plus a little
    pal.setColor(QPalette::WindowText, GColor(CSPINSCAN));
    pal.setColor(QPalette::Text, GColor(CSPINSCAN));
    axisWidget(QwtPlot::yLeft)->setPalette(pal);
    axisWidget(QwtPlot::yLeft)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    enableAxis(xBottom, false); // very little value and some cpu overhead
    enableAxis(yLeft, true);

    // 30s Power curve
    spinCurve = new QwtPlotCurve("SpinScan");
    spinCurve->setRenderHint(QwtPlotItem::RenderAntialiased); // too cpu intensive
    spinCurve->attach(this);
    spinCurve->setYAxis(QwtPlot::yLeft);

    spinScanData = new SpinScanData(spinData);
    canvas()->setFrameStyle(QFrame::NoFrame);
    configChanged(); // set colors
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
SpinScanPlot::configChanged()
{
    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));

    QColor col = GColor(CSPINSCAN);
    spinCurve->setPen(Qt::NoPen);
    col.setAlpha(120);
    QBrush brush = QBrush(col);
    spinCurve->setBrush(brush);
    //spinCurve->setStyle(QwtPlotCurve::Steps);
    spinCurve->setData(*spinScanData);
}
