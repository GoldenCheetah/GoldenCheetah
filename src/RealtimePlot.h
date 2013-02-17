/*
 * Copyright (c) 2009 Mark Liversedge  (liversedge@gmail.com)
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

#ifndef _GC_RealtimePlot_h
#define _GC_RealtimePlot_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <qwt_series_data.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include "Settings.h"


#define MAXSAMPLES 300

class Realtime30PwrData : public QwtSeriesData<QPointF>
{
    int pwrCur_; 
    int &pwrCur;
    double pwrData_[150];
    double (&pwrData)[150];

    public:
    Realtime30PwrData() : pwrCur(pwrCur_), pwrData(pwrData_) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtSeriesData *copy() const ;
    void init() ;
    void addData(double v) ;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

class RealtimePwrData : public QwtSeriesData<QPointF>
{
    int pwrCur_;
    int &pwrCur;
    double pwrData_[MAXSAMPLES];
    double (&pwrData)[MAXSAMPLES];

    public:
    RealtimePwrData() : pwrCur(pwrCur_), pwrData(pwrData_) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtSeriesData *copy() const ;
    void init() ;
    void addData(double v) ;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

class RealtimeLodData : public QwtSeriesData<QPointF>
{
    int lodCur_;
    int &lodCur;
    double lodData_[MAXSAMPLES];
    double (&lodData)[MAXSAMPLES];

    public:
    RealtimeLodData() : lodCur(lodCur_), lodData(lodData_) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtSeriesData *copy() const ;
    void init() ;
    void addData(double v) ;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

// tedious virtual data interface for QWT
class RealtimeCadData : public QwtSeriesData<QPointF>
{
    int cadCur_;
    int &cadCur;
    double cadData_[MAXSAMPLES];
    double (&cadData)[MAXSAMPLES];

    public:
    RealtimeCadData() : cadCur(cadCur_), cadData(cadData_) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtSeriesData *copy() const ;
    void init() ;
    void addData(double v) ;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

// tedious virtual data interface for QWT
class RealtimeSpdData : public QwtSeriesData<QPointF>
{
    int spdCur_;
    int &spdCur;
    double spdData_[MAXSAMPLES];
    double (&spdData)[MAXSAMPLES];

    public:
    RealtimeSpdData() : spdCur(spdCur_), spdData(spdData_) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtSeriesData *copy() const ;
    void init() ;
    void addData(double v) ;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

// tedious virtual data interface for QWT
class RealtimeHrData : public QwtSeriesData<QPointF>
{
    int hrCur_;
    int &hrCur;
    double hrData_[MAXSAMPLES];
    double (&hrData)[MAXSAMPLES];

    public:
    RealtimeHrData() : hrCur(hrCur_), hrData(hrData_) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtSeriesData *copy() const ;
    void init() ;
    void addData(double v) ;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

class RealtimePlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT

    private:

	QwtPlotGrid *grid;
	QwtPlotCurve *pwrCurve;
	QwtPlotCurve *altPwrCurve;
	QwtPlotCurve *pwr30Curve;
	QwtPlotCurve *cadCurve;
	QwtPlotCurve *spdCurve;
	QwtPlotCurve *hrCurve;
	//QwtPlotCurve *lodCurve;

    int showPowerState;
    int showPow30sState;
    int showHrState;
    int showSpeedState;
    int showCadState;
    int showAltState;

#if 0
    // power stores last 30 seconds for 30 second rolling avg, all else
    // just the last 30 seconds
#endif

    public:
    void setAxisTitle(int axis, QString label);

    Realtime30PwrData *pwr30Data;
    RealtimePwrData *pwrData;
    RealtimePwrData *altPwrData;
    RealtimeSpdData *spdData;
    RealtimeHrData *hrData;
    RealtimeCadData *cadData;
    //RealtimeLodData lodData;

    RealtimePlot();
    int smooth;

    public slots:
    void configChanged();
    void showPower(int state);
    void showPow30s(int state);
    void showHr(int state);
    void showSpeed(int state);
    void showCad(int state);
    void showAlt(int state);
    void setSmoothing(int value);
};



#endif // _GC_RealtimePlot_h

