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
#include "RealtimePlot.h"
#include "Colors.h"


// Power history
double RealtimePwrData::x(size_t i) const { return (double)50-i; }
double RealtimePwrData::y(size_t i) const { return pwrData[(pwrCur+i) < 50 ? (pwrCur+i) : (pwrCur+i-50)]; }
size_t RealtimePwrData::size() const { return 50; }
QwtData *RealtimePwrData::copy() const { return new RealtimePwrData(const_cast<RealtimePwrData*>(this)); }
void RealtimePwrData::init() { pwrCur=0; for (int i=0; i<50; i++) pwrData[i]=0; }
void RealtimePwrData::addData(double v) { pwrData[pwrCur++] = v; if (pwrCur==50) pwrCur=0; }

// 30 second Power rolling avg
double Realtime30PwrData::x(size_t i) const { return i ? 0 : 50; }

double Realtime30PwrData::y(size_t i) const { double pwr30=0; for (int x=0; x<150; x++) { pwr30+=pwrData[x]; } pwr30 /= 150; return pwr30; }
size_t Realtime30PwrData::size() const { return 50; }
QwtData *Realtime30PwrData::copy() const { return new Realtime30PwrData(const_cast<Realtime30PwrData*>(this)); }
void Realtime30PwrData::init() { pwrCur=0; for (int i=0; i<150; i++) pwrData[i]=0; }
void Realtime30PwrData::addData(double v) { pwrData[pwrCur++] = v; if (pwrCur==150) pwrCur=0; }

// Cadence history
double RealtimeCadData::x(size_t i) const { return (double)50-i; }
double RealtimeCadData::y(size_t i) const { return cadData[(cadCur+i) < 50 ? (cadCur+i) : (cadCur+i-50)]; }
size_t RealtimeCadData::size() const { return 50; }
QwtData *RealtimeCadData::copy() const { return new RealtimeCadData(const_cast<RealtimeCadData*>(this)); }
void RealtimeCadData::init() { cadCur=0; for (int i=0; i<50; i++) cadData[i]=0; }
void RealtimeCadData::addData(double v) { cadData[cadCur++] = v; if (cadCur==50) cadCur=0; }

// Speed history
double RealtimeSpdData::x(size_t i) const { return (double)50-i; }
double RealtimeSpdData::y(size_t i) const { return spdData[(spdCur+i) < 50 ? (spdCur+i) : (spdCur+i-50)]; }
size_t RealtimeSpdData::size() const { return 50; }
QwtData *RealtimeSpdData::copy() const { return new RealtimeSpdData(const_cast<RealtimeSpdData*>(this)); }
void RealtimeSpdData::init() { spdCur=0; for (int i=0; i<50; i++) spdData[i]=0; }
void RealtimeSpdData::addData(double v) { spdData[spdCur++] = v; if (spdCur==50) spdCur=0; }

// HR history
double RealtimeHrData::x(size_t i) const { return (double)50-i; }
double RealtimeHrData::y(size_t i) const { return hrData[(hrCur+i) < 50 ? (hrCur+i) : (hrCur+i-50)]; }
size_t RealtimeHrData::size() const { return 50; }
QwtData *RealtimeHrData::copy() const { return new RealtimeHrData(const_cast<RealtimeHrData*>(this)); }
void RealtimeHrData::init() { hrCur=0; for (int i=0; i<50; i++) hrData[i]=0; }
void RealtimeHrData::addData(double v) { hrData[hrCur++] = v; if (hrCur==50) hrCur=0; }

// Load history
//double RealtimeLodData::x(size_t i) const { return (double)50-i; }
//double RealtimeLodData::y(size_t i) const { return lodData[(lodCur+i) < 50 ? (lodCur+i) : (lodCur+i-50)]; }
//size_t RealtimeLodData::size() const { return 50; }
//QwtData *RealtimeLodData::copy() const { return new RealtimeLodData(); }
//void RealtimeLodData::init() { lodCur=0; for (int i=0; i<50; i++) lodData[i]=0; }
//void RealtimeLodData::addData(double v) { lodData[lodCur++] = v; if (lodCur==50) lodCur=0; }


RealtimePlot::RealtimePlot() : pwrCurve(NULL)
{
    setInstanceName("Realtime Plot");

    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    pwrData.init();
    cadData.init();
    spdData.init();
    hrData.init();

    // Setup the axis (of evil :-)
    setAxisTitle(yLeft, "Watts");
    setAxisTitle(yRight, "Cadence / HR");
    setAxisTitle(yRight2, "Speed");
    setAxisTitle(xBottom, "Seconds Ago");
    setAxisMaxMinor(xBottom, 0);
    setAxisMaxMinor(yLeft, 0);
    setAxisMaxMinor(yLeft2, 0);
    setAxisMaxMinor(yRight, 0);
    setAxisMaxMinor(yRight2, 0);

    QPalette pal;
    setAxisScale(yLeft, 0, 500); // watts
    pal.setColor(QPalette::WindowText, GColor(CPOWER));
    pal.setColor(QPalette::Text, GColor(CPOWER));
    axisWidget(QwtPlot::yLeft)->setPalette(pal);
    axisWidget(QwtPlot::yLeft)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    setAxisScale(yRight, 0, 230); // cadence / hr
    pal.setColor(QPalette::WindowText, GColor(CHEARTRATE));
    pal.setColor(QPalette::Text, GColor(CHEARTRATE));
    axisWidget(QwtPlot::yRight)->setPalette(pal);
    axisWidget(QwtPlot::yRight)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    setAxisScale(xBottom, 50, 0, 15); // time ago
    pal.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    pal.setColor(QPalette::Text, GColor(CPLOTMARKER));
    axisWidget(QwtPlot::xBottom)->setPalette(pal);
    axisWidget(QwtPlot::xBottom)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    setAxisScale(yRight2, 0, 60); // speed km/h - 60kmh on a turbo is good going!
    pal.setColor(QPalette::WindowText, GColor(CSPEED));
    pal.setColor(QPalette::Text, GColor(CSPEED));
    axisWidget(QwtPlot::yRight2)->setPalette(pal);
    axisWidget(QwtPlot::yRight2)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

	setAxisLabelRotation(yRight2,90);
	setAxisLabelAlignment(yRight2,Qt::AlignVCenter);

    enableAxis(xBottom, false); // very little value and some cpu overhead
    enableAxis(yLeft, true);
    enableAxis(yRight, true);
    enableAxis(yRight2, true);

    // 30s Power curve
    pwr30Curve = new QwtPlotCurve("30s Power");
    pwr30Curve->setRenderHint(QwtPlotItem::RenderAntialiased); // too cpu intensive
    pwr30Curve->attach(this);
    pwr30Curve->setYAxis(QwtPlot::yLeft);

    // Power curve
    pwrCurve = new QwtPlotCurve("Power");
    //pwrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    pwrCurve->setData(pwrData);
    pwrCurve->attach(this);
    pwr30Curve->setYAxis(QwtPlot::yLeft);

    // HR
    hrCurve = new QwtPlotCurve("HeartRate");
    //hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hrCurve->setData(hrData);
    hrCurve->attach(this);
    hrCurve->setYAxis(QwtPlot::yRight);

    // Cadence
    cadCurve = new QwtPlotCurve("Cadence");
    //cadCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    cadCurve->setData(cadData);
    cadCurve->attach(this);
    cadCurve->setYAxis(QwtPlot::yRight);

    // Speed
    spdCurve = new QwtPlotCurve("Speed");
    //spdCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    spdCurve->setData(spdData);
    spdCurve->attach(this);
    spdCurve->setYAxis(QwtPlot::yRight2);

    // Load
//    lodCurve = new QwtPlotCurve("Load");
//    //lodCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
//    QPen lodpen = QPen(QColor(128,128,128));
//    lodpen.setWidth(2.0);
//    lodCurve->setPen(lodpen);
//    QColor brush_color = QColor(124, 91, 31);
//    brush_color.setAlpha(64);
//    lodCurve->setBrush(brush_color);   // fill below the line
//    lodCurve->setData(lodData);
//    lodCurve->attach(this);
//    lodCurve->setYAxis(QwtPlot::yLeft);
    canvas()->setFrameStyle(QFrame::NoFrame);
    configChanged(); // set colors
}

void
RealtimePlot::setAxisTitle(int axis, QString label)
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
RealtimePlot::configChanged()
{
    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    QPen pwr30pen = QPen(GColor(CPOWER), 2.0, Qt::DashLine);
    pwr30Curve->setPen(pwr30pen);
    pwr30Curve->setData(pwr30Data);

    QPen pwrpen = QPen(GColor(CPOWER));
    pwrpen.setWidth(2.0);
    pwrCurve->setPen(pwrpen);

    QPen hrpen = QPen(GColor(CHEARTRATE));
    hrpen.setWidth(2.0);
    hrCurve->setPen(hrpen);

    QPen cadpen = QPen(GColor(CCADENCE));
    cadpen.setWidth(2.0);
    cadCurve->setPen(cadpen);

    QPen spdpen = QPen(GColor(CSPEED));
    spdpen.setWidth(2.0);
    spdCurve->setPen(spdpen);
}
