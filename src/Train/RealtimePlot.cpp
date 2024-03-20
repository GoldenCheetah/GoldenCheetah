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
#include "Context.h"
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

// tHb history
double RealtimethbData::x(size_t i) const { return (double)MAXSAMPLES-i; }
double RealtimethbData::y(size_t i) const { return thbData[(thbCur+i) < MAXSAMPLES ? (thbCur+i) : (thbCur+i-MAXSAMPLES)]; }
size_t RealtimethbData::size() const { return MAXSAMPLES; }
//QwtSeriesData *RealtimethbData::copy() const { return new RealtimethbData(const_cast<RealtimethbData*>(this)); }
void RealtimethbData::init() { thbCur=0; for (int i=0; i<MAXSAMPLES; i++) thbData[i]=0; }
void RealtimethbData::addData(double v) { thbData[thbCur++] = v; if (thbCur==MAXSAMPLES) thbCur=0; }

QPointF RealtimethbData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF RealtimethbData::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}

// smo2ence history
double Realtimesmo2Data::x(size_t i) const { return (double)MAXSAMPLES-i; }
double Realtimesmo2Data::y(size_t i) const { return smo2Data[(smo2Cur+i) < MAXSAMPLES ? (smo2Cur+i) : (smo2Cur+i-MAXSAMPLES)]; }
size_t Realtimesmo2Data::size() const { return MAXSAMPLES; }
//QwtSeriesData *Realtimesmo2Data::copy() const { return new Realtimesmo2Data(const_cast<Realtimesmo2Data*>(this)); }
void Realtimesmo2Data::init() { smo2Cur=0; for (int i=0; i<MAXSAMPLES; i++) smo2Data[i]=0; }
void Realtimesmo2Data::addData(double v) { smo2Data[smo2Cur++] = v; if (smo2Cur==MAXSAMPLES) smo2Cur=0; }

QPointF Realtimesmo2Data::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF Realtimesmo2Data::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}

// O2Hb history
double Realtimeo2hbData::x(size_t i) const { return (double)MAXSAMPLES-i; }
double Realtimeo2hbData::y(size_t i) const { return o2hbData[(o2hbCur+i) < MAXSAMPLES ? (o2hbCur+i) : (o2hbCur+i-MAXSAMPLES)]; }
size_t Realtimeo2hbData::size() const { return MAXSAMPLES; }
//QwtSeriesData *Realtimeo2hbData::copy() const { return new Realtimeo2hbData(const_cast<Realtimeo2hbData*>(this)); }
void Realtimeo2hbData::init() { o2hbCur=0; for (int i=0; i<MAXSAMPLES; i++) o2hbData[i]=0; }
void Realtimeo2hbData::addData(double v) { o2hbData[o2hbCur++] = v; if (o2hbCur==MAXSAMPLES) o2hbCur=0; }

QPointF Realtimeo2hbData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF Realtimeo2hbData::boundingRect() const
{
    // TODO dgr
    return QRectF(-5000, 5000, 10000, 10000);
}

// HHb history
double RealtimehhbData::x(size_t i) const { return (double)MAXSAMPLES-i; }
double RealtimehhbData::y(size_t i) const { return hhbData[(hhbCur+i) < MAXSAMPLES ? (hhbCur+i) : (hhbCur+i-MAXSAMPLES)]; }
size_t RealtimehhbData::size() const { return MAXSAMPLES; }
//QwtSeriesData *RealtimehhbData::copy() const { return new RealtimehhbData(const_cast<RealtimehhbData*>(this)); }
void RealtimehhbData::init() { hhbCur=0; for (int i=0; i<MAXSAMPLES; i++) hhbData[i]=0; }
void RealtimehhbData::addData(double v) { hhbData[hhbCur++] = v; if (hhbCur==MAXSAMPLES) hhbCur=0; }

QPointF RealtimehhbData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF RealtimehhbData::boundingRect() const
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


RealtimePlot::RealtimePlot(Context *context) : 
    pwrCurve(NULL),
    showPowerState(Qt::Checked),
    showPow30sState(Qt::Checked),
    showHrState(Qt::Checked),
    showSpeedState(Qt::Checked),
    showCadState(Qt::Checked),
    showAltState(Qt::Checked),
    showO2HbState(Qt::Checked),
    showHHbState(Qt::Checked),
    showtHbState(Qt::Checked),
    showSmO2State(Qt::Checked),
    smooth(0),
    context(context)
{
    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    pwr30Data = new Realtime30PwrData;
    pwrData = new RealtimePwrData;
    altPwrData = new RealtimePwrData;
    spdData = new RealtimeSpdData;
    hrData = new RealtimeHrData;
    cadData = new RealtimeCadData;
    thbData = new RealtimethbData;
    o2hbData = new Realtimeo2hbData;
    hhbData = new RealtimehhbData;
    smo2Data = new Realtimesmo2Data;

    // Setup the axis (of evil :-)
    setAxisTitle(QwtAxis::YLeft, "Watts");
    setAxisTitle(QwtAxis::YRight, "Cadence / Hb / HR");
    setAxisTitle(QwtAxisId(QwtAxis::YRight,2).id, "Speed");
    setAxisTitle(QwtAxis::XBottom, "Seconds Ago");
    setAxisMaxMinor(QwtAxis::XBottom, 0);
    setAxisMaxMinor(QwtAxis::YLeft, 0);
    setAxisMaxMinor(QwtAxis::YRight, 0);
    setAxisMaxMinor(QwtAxisId(QwtAxis::YRight,2).id, 0);

    QPalette pal;
    setAxisScale(QwtAxis::YLeft, 0, 500); // watts
    pal.setColor(QPalette::WindowText, GColor(GCol::POWER));
    pal.setColor(QPalette::Text, GColor(GCol::POWER));
    axisWidget(QwtAxis::YLeft)->setPalette(pal);
    axisWidget(QwtAxis::YLeft)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    setAxisScale(QwtAxis::YRight, 0, 230); // cadence / hr
    pal.setColor(QPalette::WindowText, GColor(GCol::HEARTRATE));
    pal.setColor(QPalette::Text, GColor(GCol::HEARTRATE));
    axisWidget(QwtAxis::YRight)->setPalette(pal);
    axisWidget(QwtAxis::YRight)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    setAxisScale(QwtAxis::XBottom, MAXSAMPLES, 0, 15); // time ago
    pal.setColor(QPalette::WindowText, GColor(GCol::PLOTMARKER));
    pal.setColor(QPalette::Text, GColor(GCol::PLOTMARKER));
    axisWidget(QwtAxis::XBottom)->setPalette(pal);
    axisWidget(QwtAxis::XBottom)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

    setAxisScale(QwtAxisId(QwtAxis::YRight,2).id, 0, 60); // speed km/h - 60kmh on a turbo is good going!
    pal.setColor(QPalette::WindowText, GColor(GCol::SPEED));
    pal.setColor(QPalette::Text, GColor(GCol::SPEED));
    axisWidget(QwtAxisId(QwtAxis::YRight,2).id)->setPalette(pal);
    axisWidget(QwtAxisId(QwtAxis::YRight,2).id)->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 3);

	setAxisLabelRotation(QwtAxisId(QwtAxis::YRight,2).id,90);
	setAxisLabelAlignment(QwtAxisId(QwtAxis::YRight,2).id,Qt::AlignVCenter);

    setAxisVisible(QwtAxis::XBottom, false); // very little value and some cpu overhead
    setAxisVisible(QwtAxis::YLeft, true);
    setAxisVisible(QwtAxis::YRight, true);
    setAxisVisible(QwtAxisId(QwtAxis::YRight,2).id, true);

    // 30s Power curve
    pwr30Curve = new QwtPlotCurve("30s Power");
    pwr30Curve->setRenderHint(QwtPlotItem::RenderAntialiased); // too cpu intensive
    pwr30Curve->attach(this);
    pwr30Curve->setYAxis(QwtAxis::YLeft);

    // Power curve
    pwrCurve = new QwtPlotCurve("Power");
    //pwrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    pwrCurve->setData(pwrData);
    pwrCurve->attach(this);
    pwrCurve->setYAxis(QwtAxis::YLeft);

    altPwrCurve = new QwtPlotCurve("Alt Power");
    //pwrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    altPwrCurve->setData(altPwrData);
    altPwrCurve->attach(this);
    altPwrCurve->setYAxis(QwtAxis::YLeft);

    // HR
    hrCurve = new QwtPlotCurve("HeartRate");
    //hrCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hrCurve->setData(hrData);
    hrCurve->attach(this);
    hrCurve->setYAxis(QwtAxis::YRight);

    // Cadence
    cadCurve = new QwtPlotCurve("Cadence");
    //cadCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    cadCurve->setData(cadData);
    cadCurve->attach(this);
    cadCurve->setYAxis(QwtAxis::YRight);

    // Speed
    spdCurve = new QwtPlotCurve("Speed");
    //spdCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    spdCurve->setData(spdData);
    spdCurve->attach(this);
    spdCurve->setYAxis(QwtAxisId(QwtAxis::YRight,2).id);

    // hhb curve
    hhbCurve = new QwtPlotCurve("HHb");
    hhbCurve->setData(hhbData);
    hhbCurve->attach(this);
    hhbCurve->setYAxis(QwtAxis::YRight);

    // o2hb
    o2hbCurve = new QwtPlotCurve("O2Hb");
    o2hbCurve->setData(o2hbData);
    o2hbCurve->attach(this);
    o2hbCurve->setYAxis(QwtAxis::YRight);

    // smo2
    smo2Curve = new QwtPlotCurve("SmO2");
    //cadCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    smo2Curve->setData(smo2Data);
    smo2Curve->attach(this);
    smo2Curve->setYAxis(QwtAxis::YRight);

    // tHb
    thbCurve = new QwtPlotCurve("Speed");
    //spdCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    thbCurve->setData(thbData);
    thbCurve->attach(this);
    thbCurve->setYAxis(QwtAxis::YRight);

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
//    lodCurve->setYAxis(QwtAxis::YLeft);
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // set to current config
    configChanged(CONFIG_APPEARANCE); // set colors
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
RealtimePlot::configChanged(qint32)
{
    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();

    setCanvasBackground(GColor(GCol::TRAINPLOTBACKGROUND));
    QPen pwr30pen = QPen(GColor(GCol::POWER), width, Qt::DashLine);
    pwr30Curve->setPen(pwr30pen);
    pwr30Curve->setData(pwr30Data);

    QPen pwrpen = QPen(GColor(GCol::POWER));
    pwrpen.setWidth(width);
    pwrCurve->setPen(pwrpen);

    QPen apwrpen = QPen(GColor(GCol::ALTPOWER));
    apwrpen.setWidth(width);
    altPwrCurve->setPen(apwrpen);

    QPen hrpen = QPen(GColor(GCol::HEARTRATE));
    hrpen.setWidth(width);
    hrCurve->setPen(hrpen);

    QPen cadpen = QPen(GColor(GCol::CADENCE));
    cadpen.setWidth(width);
    cadCurve->setPen(cadpen);

    QPen spdpen = QPen(GColor(GCol::SPEED));
    spdpen.setWidth(width);
    spdCurve->setPen(spdpen);

    QPen hhbpen = QPen(GColor(GCol::HHB));
    hhbpen.setWidth(width);
    hhbCurve->setPen(hhbpen);

    QPen o2hbpen = QPen(GColor(GCol::O2HB));
    o2hbpen.setWidth(width);
    o2hbCurve->setPen(o2hbpen);

    QPen smo2pen = QPen(GColor(GCol::SMO2));
    smo2pen.setWidth(width);
    smo2Curve->setPen(smo2pen);

    QPen thbpen = QPen(GColor(GCol::THB));
    thbpen.setWidth(width);
    thbCurve->setPen(thbpen);

    replot();
}

void
RealtimePlot::showPower(int state)
{
    showPowerState = state;
    pwrCurve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YLeft, showAltState == Qt::Checked || showPowerState == Qt::Checked || showPow30sState == Qt::Checked);
    replot();
}

void
RealtimePlot::showPow30s(int state)
{
    showPow30sState = state;
    pwr30Curve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YLeft, showAltState == Qt::Checked || showPowerState == Qt::Checked || showPow30sState == Qt::Checked);
    replot();
}

void
RealtimePlot::showHr(int state)
{
    showHrState = state;
    hrCurve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YRight, showCadState == Qt::Checked || showHrState == Qt::Checked ||
                       showO2HbState == Qt::Checked || showHHbState == Qt::Checked ||
                       showtHbState == Qt::Checked || showSmO2State == Qt::Checked);
    replot();
}

void
RealtimePlot::showSpeed(int state)
{
    showSpeedState = state;
    spdCurve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxisId(QwtAxis::YRight,2).id, state == Qt::Checked);
    replot();
}

void
RealtimePlot::showCad(int state)
{
    showCadState = state;
    cadCurve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YRight, showCadState == Qt::Checked || showHrState == Qt::Checked ||
                       showO2HbState == Qt::Checked || showHHbState == Qt::Checked ||
                       showtHbState == Qt::Checked || showSmO2State == Qt::Checked);
    replot();
}

void
RealtimePlot::showAlt(int state)
{
    showAltState = state;
    altPwrCurve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YLeft, showAltState == Qt::Checked || showPowerState == Qt::Checked || showPow30sState == Qt::Checked);
    replot();
}

void
RealtimePlot::showHHb(int state)
{
    showHHbState = state;
    hhbCurve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YRight, showCadState == Qt::Checked || showHrState == Qt::Checked ||
                       showO2HbState == Qt::Checked || showHHbState == Qt::Checked ||
                       showtHbState == Qt::Checked || showSmO2State == Qt::Checked);
    replot();
}

void
RealtimePlot::showO2Hb(int state)
{
    showO2HbState = state;
    o2hbCurve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YRight, showCadState == Qt::Checked || showHrState == Qt::Checked ||
                       showO2HbState == Qt::Checked || showHHbState == Qt::Checked ||
                       showtHbState == Qt::Checked || showSmO2State == Qt::Checked);
    replot();
}

void
RealtimePlot::showtHb(int state)
{
    showtHbState = state;
    thbCurve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YRight, showCadState == Qt::Checked || showHrState == Qt::Checked ||
                       showO2HbState == Qt::Checked || showHHbState == Qt::Checked ||
                       showtHbState == Qt::Checked || showSmO2State == Qt::Checked);
    replot();
}

void
RealtimePlot::showSmO2(int state)
{
    showSmO2State = state;
    smo2Curve->setVisible(state == Qt::Checked);
    setAxisVisible(QwtAxis::YRight, showCadState == Qt::Checked || showHrState == Qt::Checked ||
                       showO2HbState == Qt::Checked || showHHbState == Qt::Checked ||
                       showtHbState == Qt::Checked || showSmO2State == Qt::Checked);
    replot();
}

void
RealtimePlot::setSmoothing(int value)
{
    smooth = value;
}
