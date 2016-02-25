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
#include "Context.h"


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

class RealtimethbData : public QwtSeriesData<QPointF>
{
    int thbCur_;
    int &thbCur;
    double thbData_[MAXSAMPLES];
    double (&thbData)[MAXSAMPLES];

    public:
    RealtimethbData() : thbCur(thbCur_), thbData(thbData_) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtSeriesData *copy() const ;
    void init() ;
    void addData(double v) ;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

class Realtimesmo2Data : public QwtSeriesData<QPointF>
{
    int smo2Cur_;
    int &smo2Cur;
    double smo2Data_[MAXSAMPLES];
    double (&smo2Data)[MAXSAMPLES];

    public:
    Realtimesmo2Data() : smo2Cur(smo2Cur_), smo2Data(smo2Data_) { init(); }

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
class RealtimehhbData : public QwtSeriesData<QPointF>
{
    int hhbCur_;
    int &hhbCur;
    double hhbData_[MAXSAMPLES];
    double (&hhbData)[MAXSAMPLES];

    public:
    RealtimehhbData() : hhbCur(hhbCur_), hhbData(hhbData_) { init(); }

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
class Realtimeo2hbData : public QwtSeriesData<QPointF>
{
    int o2hbCur_;
    int &o2hbCur;
    double o2hbData_[MAXSAMPLES];
    double (&o2hbData)[MAXSAMPLES];

    public:
    Realtimeo2hbData() : o2hbCur(o2hbCur_), o2hbData(o2hbData_) { init(); }

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
	QwtPlotCurve *thbCurve;
	QwtPlotCurve *hhbCurve;
	QwtPlotCurve *o2hbCurve;
	QwtPlotCurve *smo2Curve;

    int showPowerState;
    int showPow30sState;
    int showHrState;
    int showSpeedState;
    int showCadState;
    int showAltState;
    int showO2HbState;
    int showHHbState;
    int showtHbState;
    int showSmO2State;


    public:
    void setAxisTitle(int axis, QString label);

    Realtime30PwrData *pwr30Data;
    RealtimePwrData *pwrData;
    RealtimePwrData *altPwrData;
    RealtimeSpdData *spdData;
    RealtimeHrData *hrData;
    RealtimeCadData *cadData;
    RealtimethbData *thbData;
    Realtimeo2hbData *o2hbData;
    RealtimehhbData *hhbData;
    Realtimesmo2Data *smo2Data;

    RealtimePlot(Context *context);
    int smooth;

    public slots:
    void configChanged(qint32);
    void showPower(int state);
    void showPow30s(int state);
    void showHr(int state);
    void showSpeed(int state);
    void showCad(int state);
    void showAlt(int state);
    void showO2Hb(int state);
    void showHHb(int state);
    void showtHb(int state);
    void showSmO2(int state);
    void setSmoothing(int value);

    private:
    Context *context;
};



#endif // _GC_RealtimePlot_h

