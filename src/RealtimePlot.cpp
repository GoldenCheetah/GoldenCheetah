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
#include "RealtimePlot.h"
#include "Colors.h"


// Power history
double RealtimePwrData::x(size_t i) const { return (double)MAXSAMPLES-i; }
double RealtimePwrData::y(size_t i) const { return pwrData[(pwrCur+i) < MAXSAMPLES ? (pwrCur+i) : (pwrCur+i-MAXSAMPLES)]; }
size_t RealtimePwrData::size() const { return MAXSAMPLES; }
//QwtSeriesData *RealtimePwrData::copy() const { return new RealtimePwrData(const_cast<RealtimePwrData*>(this)); }
void RealtimePwrData::init() { pwrCur=0; for (int i=0; i<MAXSAMPLES; i++) pwrData[i]=0; }
void RealtimePwrData::addData(double v) { pwrData[pwrCur++] = v; if (pwrCur==MAXSAMPLES) pwrCur=0; }

QPointF RealtimePwrData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF RealtimePwrData::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}

// 30 second Power rolling avg
double Realtime30PwrData::x(size_t i) const { return i ? 0 : MAXSAMPLES; }

double Realtime30PwrData::y(size_t /*i*/) const { double pwr30=0; for (int x=0; x<150; x++) { pwr30+=pwrData[x]; } pwr30 /= 150; return pwr30; }
size_t Realtime30PwrData::size() const { return 150; }
//QwtSeriesData *Realtime30PwrData::copy() const { return new Realtime30PwrData(const_cast<Realtime30PwrData*>(this)); }
void Realtime30PwrData::init() { pwrCur=0; for (int i=0; i<150; i++) pwrData[i]=0; }
void Realtime30PwrData::addData(double v) { pwrData[pwrCur++] = v; if (pwrCur==150) pwrCur=0; }

QPointF Realtime30PwrData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF Realtime30PwrData::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}




// Cadence history
double RealtimeCadData::x(size_t i) const { return (double)MAXSAMPLES-i; }
double RealtimeCadData::y(size_t i) const { return cadData[(cadCur+i) < MAXSAMPLES ? (cadCur+i) : (cadCur+i-MAXSAMPLES)]; }
size_t RealtimeCadData::size() const { return MAXSAMPLES; }
//QwtSeriesData *RealtimeCadData::copy() const { return new RealtimeCadData(const_cast<RealtimeCadData*>(this)); }
void RealtimeCadData::init() { cadCur=0; for (int i=0; i<MAXSAMPLES; i++) cadData[i]=0; }
void RealtimeCadData::addData(double v) { cadData[cadCur++] = v; if (cadCur==MAXSAMPLES) cadCur=0; }

QPointF RealtimeCadData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF RealtimeCadData::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}

// Speed history
double RealtimeSpdData::x(size_t i) const { return (double)MAXSAMPLES-i; }
double RealtimeSpdData::y(size_t i) const { return spdData[(spdCur+i) < MAXSAMPLES ? (spdCur+i) : (spdCur+i-MAXSAMPLES)]; }
size_t RealtimeSpdData::size() const { return MAXSAMPLES; }
//QwtSeriesData *RealtimeSpdData::copy() const { return new RealtimeSpdData(const_cast<RealtimeSpdData*>(this)); }
void RealtimeSpdData::init() { spdCur=0; for (int i=0; i<MAXSAMPLES; i++) spdData[i]=0; }
void RealtimeSpdData::addData(double v) { spdData[spdCur++] = v; if (spdCur==MAXSAMPLES) spdCur=0; }

QPointF RealtimeSpdData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF RealtimeSpdData::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}

// HR history
double RealtimeHrData::x(size_t i) const { return (double)MAXSAMPLES-i; }
double RealtimeHrData::y(size_t i) const { return hrData[(hrCur+i) < MAXSAMPLES ? (hrCur+i) : (hrCur+i-MAXSAMPLES)]; }
size_t RealtimeHrData::size() const { return MAXSAMPLES; }
//QwtSeriesData *RealtimeHrData::copy() const { return new RealtimeHrData(const_cast<RealtimeHrData*>(this)); }
void RealtimeHrData::init() { hrCur=0; for (int i=0; i<MAXSAMPLES; i++) hrData[i]=0; }
void RealtimeHrData::addData(double v) { hrData[hrCur++] = v; if (hrCur==MAXSAMPLES) hrCur=0; }

QPointF RealtimeHrData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF RealtimeHrData::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}

// Load history
//double RealtimeLodData::x(size_t i) const { return (double)50-i; }
//double RealtimeLodData::y(size_t i) const { return lodData[(lodCur+i) < 50 ? (lodCur+i) : (lodCur+i-50)]; }
//size_t RealtimeLodData::size() const { return 50; }
//QwtData *RealtimeLodData::copy() const { return new RealtimeLodData(); }
//void RealtimeLodData::init() { lodCur=0; for (int i=0; i<50; i++) lodData[i]=0; }
//void RealtimeLodData::addData(double v) { lodData[lodCur++] = v; if (lodCur==50) lodCur=0; }


RealtimePlot::RealtimePlot() : 
    pwrCurve(NULL),
    showPowerState(Qt::Checked),
    showPow30sState(Qt::Checked),
    showHrState(Qt::Checked),
    showSpeedState(Qt::Checked),
    showCadState(Qt::Checked),
    showAltState(Qt::Checked),
    smooth(0)
{
    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    pwr30Data = new Realtime30PwrData;
    pwrData = new RealtimePwrData;
    altPwrData = new RealtimePwrData;
    spdData = new RealtimeSpdData;
    hrData = new RealtimeHrData;
    cadData = new RealtimeCadData;

    // Setup the axis (of evil :-)
    setAxisTitle(yLeft, "Watts");
    setAxisTitle(yRight, "Cadence / HR");
    setAxisTitle(QwtAxisId(QwtAxis::yRight,2).id, "Speed");
    setAxisTitle(xBottom, "Seconds Ago");
    setAxisMaxMinor(xBottom, 0);
    setAxisMaxMinor(yLeft, 0);
    setAxisMaxMinor(yRight, 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::yRight,2).id, 0);

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

    setAxisScale(xBottom, MAXSAMPLES, 0, 15); // time ago
    pal.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    pal.setColor(QPalette::Text, GColor(CPLOTMARKER));
    axisWidget(QwtPlot::xBottom)->setPalette(pal);
    axisWidget(QwtPlot::xBottom)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    setAxisScale(QwtAxisId(QwtAxis::yRight,2).id, 0, 60); // speed km/h - 60kmh on a turbo is good going!
    pal.setColor(QPalette::WindowText, GColor(CSPEED));
    pal.setColor(QPalette::Text, GColor(CSPEED));
    axisWidget(QwtAxisId(QwtAxis::yRight,2).id)->setPalette(pal);
    axisWidget(QwtAxisId(QwtAxis::yRight,2).id)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

	setAxisLabelRotation(QwtAxisId(QwtAxis::yRight,2).id,90);
	setAxisLabelAlignment(QwtAxisId(QwtAxis::yRight,2).id,Qt::AlignVCenter);

    enableAxis(xBottom, false); // very little value and some cpu overhead
    enableAxis(yLeft, true);
    enableAxis(yRight, true);
    enableAxis(QwtAxisId(QwtAxis::yRight,2).id, true);

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
    pwrCurve->setYAxis(QwtPlot::yLeft);

    altPwrCurve = new QwtPlotCurve("Alt Power");
    //pwrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    altPwrCurve->setData(altPwrData);
    altPwrCurve->attach(this);
    altPwrCurve->setYAxis(QwtPlot::yLeft);

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
    spdCurve->setYAxis(QwtAxisId(QwtAxis::yRight,2).id);

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
    double width = appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble();

    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    QPen pwr30pen = QPen(GColor(CPOWER), width, Qt::DashLine);
    pwr30Curve->setPen(pwr30pen);
    pwr30Curve->setData(pwr30Data);

    QPen pwrpen = QPen(GColor(CPOWER));
    pwrpen.setWidth(width);
    pwrCurve->setPen(pwrpen);

    QPen apwrpen = QPen(GColor(CALTPOWER));
    apwrpen.setWidth(width);
    altPwrCurve->setPen(apwrpen);

    QPen hrpen = QPen(GColor(CHEARTRATE));
    hrpen.setWidth(width);
    hrCurve->setPen(hrpen);

    QPen cadpen = QPen(GColor(CCADENCE));
    cadpen.setWidth(width);
    cadCurve->setPen(cadpen);

    QPen spdpen = QPen(GColor(CSPEED));
    spdpen.setWidth(width);
    spdCurve->setPen(spdpen);
}

void
RealtimePlot::showPower(int state)
{
    showPowerState = state;
    pwrCurve->setVisible(state == Qt::Checked);
    enableAxis(yLeft, showAltState == Qt::Checked || showPowerState == Qt::Checked || showPow30sState == Qt::Checked);
    replot();
}

void
RealtimePlot::showPow30s(int state)
{
    showPow30sState = state;
    pwr30Curve->setVisible(state == Qt::Checked);
    enableAxis(yLeft, showAltState == Qt::Checked || showPowerState == Qt::Checked || showPow30sState == Qt::Checked);
    replot();
}

void
RealtimePlot::showHr(int state)
{
    showHrState = state;
    hrCurve->setVisible(state == Qt::Checked);
    enableAxis(yRight, showCadState == Qt::Checked || showHrState == Qt::Checked);
    replot();
}

void
RealtimePlot::showSpeed(int state)
{
    showSpeedState = state;
    spdCurve->setVisible(state == Qt::Checked);
    enableAxis(QwtAxisId(QwtAxis::yRight,2).id, state == Qt::Checked);
    replot();
}

void
RealtimePlot::showCad(int state)
{
    showCadState = state;
    cadCurve->setVisible(state == Qt::Checked);
    enableAxis(yRight, showCadState == Qt::Checked || showHrState == Qt::Checked);
    replot();
}

void
RealtimePlot::showAlt(int state)
{
    showAltState = state;
    altPwrCurve->setVisible(state == Qt::Checked);
    enableAxis(yLeft, showAltState == Qt::Checked || showPowerState == Qt::Checked || showPow30sState == Qt::Checked);
    replot();
}

void
RealtimePlot::setSmoothing(int value)
{
    smooth = value;
}
