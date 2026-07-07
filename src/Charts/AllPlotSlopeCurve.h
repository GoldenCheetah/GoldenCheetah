/*
 * Copyright (c) 2014 Joern Rischmueller (joern.rm@gmail.com)
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


#ifndef _GC_AllPlotSlopeCurve
#define _GC_AllPlotSlopeCurve 1

#include "qwt_global.h"
#include "qwt_plot_seriesitem.h"
#include "qwt_series_data.h"
#include "qwt_plot_curve.h"
#include "qwt_text.h"

#include <QPen>
#include <QString>

class QPainter;
class QPolygonF;
class QwtScaleMap;
class QwtSymbol;
class QwtCurveFitter;

class AllPlotSlopeCurve: public QwtPlotCurve
{

public:

    explicit AllPlotSlopeCurve( const QString &title = QString() );
    explicit AllPlotSlopeCurve( const QwtText &title );

    virtual ~AllPlotSlopeCurve();

    virtual int rtti() const;

    enum CurveStyle
    {
        SlopeDist1 = QwtPlotCurve::UserCurve+1,
        SlopeDist2,
        SlopeDist3,
        SlopeTime1,
        SlopeTime2,
        SlopeTime3
    };

    void setStyle( CurveStyle style );
    CurveStyle style() const;


protected:

    void init();

    void drawCurve( QPainter *p, int style,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRectF &canvasRect, int from, int to ) const;


private:
    class PrivateData;
    PrivateData *d_data;

};


#endif
