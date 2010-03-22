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

#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_data.h>
#include "Settings.h"


class Realtime30PwrData : public QwtData
{
    public:
    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    void addData(double v) ;
};

class RealtimePwrData : public QwtData
{
    public:
    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    virtual QwtData *copy() const ;
    void init() ;
    void addData(double v) ;
};

class RealtimeLodData : public QwtData
{
    public:
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
    public:
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
    public:
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
    public:
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

    private:

	QwtPlotGrid *grid;
	QwtPlotCurve *pwrCurve;
	QwtPlotCurve *pwr30Curve;
	QwtPlotCurve *cadCurve;
	QwtPlotCurve *spdCurve;
	QwtPlotCurve *hrCurve;
	//QwtPlotCurve *lodCurve;


    public:
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

