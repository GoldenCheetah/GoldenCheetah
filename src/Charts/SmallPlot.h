/*
 * Copyright (c) 2011 Damien Grauser
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

#ifndef _GC_SmallPlot_h
#define _GC_SmallPlot_h 1

#include <qwt_plot.h>
#include <qwt_plot_picker.h>
#include <QtGui>

class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class RideItem;
class RideFile;

class SmallPlot : public QwtPlot
{
    Q_OBJECT

    public:

        SmallPlot(QWidget *parent=0);


        void enableTracking();
        bool hasTracking() const;
        int smoothing() const { return smooth; }
        void setData(RideItem *rideItem);
        void setData(RideFile *rideFile);
        void setAxisTitle(QwtAxisId axis, QString label);
        void recalc();
        void setYMax();
        void setXTitle();

    public slots:

        void showPower(int state);
        void showHr(int state);
        void setSmoothing(int value);

    signals:

        void selectedPosX(double dataPosX);
        void mouseLeft();

    protected:

        virtual void leaveEvent(QEvent *event);

        QwtPlotGrid *grid;
        QwtPlotCurve *wattsCurve;
        QwtPlotCurve *hrCurve;
        QwtPlotCurve *altCurve;

        QwtPlotMarker* d_mrk;
        QVector<double> hrArray;
        QVector<double> wattsArray;
        QVector<double> altArray;
        QVector<double> distanceArray;
        QVector<double> timeArray;
        QVector<QwtPlotCurve*> timeCurves;
        int arrayLength;

        QVector<int> interArray;

        int smooth;
        bool tracking;

    protected slots:

        void pointMoved(const QPoint &pos);
};


class SmallPlotPicker : public QwtPlotPicker
{
    Q_OBJECT

    public:
        SmallPlotPicker(QWidget *canvas);

    protected:
        virtual QwtText trackerText(const QPoint &point) const override;
};

#endif // _GC_SmallPlot_h
