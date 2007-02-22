/* 
 * $Id: CpintPlot.h,v 1.2 2006/07/12 02:13:57 srhea Exp $
 *
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_CpintPlot_h
#define _GC_CpintPlot_h 1

#include <qwt_plot.h>
#include <QtGui>

class QwtPlotCurve;
class QwtPlotGrid;

class CpintPlot : public QwtPlot
{
    Q_OBJECT

    public:

        CpintPlot(QString path);
        QProgressDialog *progress;
        bool needToScanRides;

        const QwtPlotCurve *getAllCurve() const { return allCurve; }
        const QwtPlotCurve *getThisCurve() const { return thisCurve; }

    public slots:

        void showGrid(int state);
        void calculate(QString fileName, QDateTime dateTime);

    protected:

        QString path;
        QwtPlotCurve *allCurve;
        QwtPlotCurve *thisCurve;
        QwtPlotGrid *grid;
};

#endif // _GC_CpintPlot_h

