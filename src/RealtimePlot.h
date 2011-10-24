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
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include <qwt_data.h>
#include "Settings.h"

class Realtime30PwrData : public QwtData
{
    int pwrCur_; 
    int &pwrCur;
    double pwr30;
    double pwrData_[150];
    double (&pwrData)[150];

    public:
    Realtime30PwrData() : pwrCur(pwrCur_), pwrData(pwrData_) { init(); }
    Realtime30PwrData(Realtime30PwrData *other) : pwrCur(other->pwrCur_), pwrData(other->pwrData_) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    void addData(double v) ;
};

class RealtimePwrData : public QwtData
{
    int pwrCur_;
    int &pwrCur;
    double pwrData_[150];
    double (&pwrData)[150];

    public:
    RealtimePwrData() : pwrCur(pwrCur_), pwrData(pwrData_) {}
    RealtimePwrData(RealtimePwrData *other) : pwrCur(other->pwrCur_), pwrData(other->pwrData_) {}

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    void addData(double v) ;
};

class RealtimeLodData : public QwtData
{
    int lodCur_;
    int &lodCur;
    double lodData_[50];
    double (&lodData)[50];

    public:
    RealtimeLodData() : lodCur(lodCur_), lodData(lodData_) {}
    RealtimeLodData(RealtimeLodData *other) : lodCur(other->lodCur_), lodData(other->lodData_) {}

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    void addData(double v) ;
};

// tedious virtual data interface for QWT
class RealtimeCadData : public QwtData
{
    int cadCur_;
    int &cadCur;
    double cadData_[50];
    double (&cadData)[50];

    public:
    RealtimeCadData() : cadCur(cadCur_), cadData(cadData_) {}
    RealtimeCadData(RealtimeCadData *other) : cadCur(other->cadCur_), cadData(other->cadData_) {}

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    void addData(double v) ;
};

// tedious virtual data interface for QWT
class RealtimeSpdData : public QwtData
{
    int spdCur_;
    int &spdCur;
    double spdData_[50];
    double (&spdData)[50];

    public:
    RealtimeSpdData() : spdCur(spdCur_), spdData(spdData_) {}
    RealtimeSpdData(RealtimeSpdData *other) : spdCur(other->spdCur_), spdData(other->spdData_) {}

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    void addData(double v) ;
};

// tedious virtual data interface for QWT
class RealtimeHrData : public QwtData
{
    int hrCur_;
    int &hrCur;
    double hrData_[50];
    double (&hrData)[50];

    public:
    RealtimeHrData() : hrCur(hrCur_), hrData(hrData_) {}
    RealtimeHrData(RealtimeHrData *other) : hrCur(other->hrCur_), hrData(other->hrData_) {}

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    void addData(double v) ;
};

class RealtimePlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT

    private:

	QwtPlotGrid *grid;
	QwtPlotCurve *pwrCurve;
	QwtPlotCurve *pwr30Curve;
	QwtPlotCurve *cadCurve;
	QwtPlotCurve *spdCurve;
	QwtPlotCurve *hrCurve;
	//QwtPlotCurve *lodCurve;

#if 0
    // power stores last 30 seconds for 30 second rolling avg, all else
    // just the last 30 seconds
#endif

    public:
    void setAxisTitle(int axis, QString label);
    Realtime30PwrData pwr30Data;
    RealtimePwrData pwrData;
    RealtimeSpdData spdData;
    RealtimeHrData hrData;
    RealtimeCadData cadData;
    //RealtimeLodData lodData;

    RealtimePlot();

    public slots:
    void configChanged();
};



#endif // _GC_RealtimePlot_h

