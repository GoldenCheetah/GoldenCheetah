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

#ifndef _GC_SpinScanPolarPlot_h
#define _GC_SpinScanPolarPlot_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include <qwt_series_data.h>
#include "Settings.h"

#include <stdint.h> //uint8_t

class SpinScanPolarData : public QwtSeriesData<QPointF>
{
    bool isleft;
    uint8_t *spinData;

    public:
    SpinScanPolarData(uint8_t *spinData, bool isleft) : isleft(isleft), spinData(spinData) { init(); }

    double x(size_t i) const ;
    double y(size_t i) const ;
    size_t size() const ;
    //virtual QwtData *copy() const ;
    void init() ;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

class SpinScanPolarPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT

    private:

	QwtPlotCurve *leftCurve;
	QwtPlotCurve *rightCurve;

    public:
    void setAxisTitle(int axis, QString label);

    SpinScanPolarData *leftSpinScanPolarData;
    SpinScanPolarData *rightSpinScanPolarData;

    SpinScanPolarPlot(QWidget *parent, uint8_t *);

    uint8_t *spinData;

    public slots:
    void configChanged(qint32);
};



#endif // _GC_SpinScanPolarPlot_h

