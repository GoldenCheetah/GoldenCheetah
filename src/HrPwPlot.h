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

#ifndef _GC_HrPwPlot_h
#define _GC_HrPwPlot_h 1

#include "GoldenCheetah.h"
#include <qwt_plot.h>
#include <QtGui>

class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class MainWindow;
class HrPwWindow;
class RideItem;
class HrPwPlotBackground;
class HrPwPlotZoneLabel;

// Tooltips
class LTMToolTip;
class LTMCanvasPicker;

class HrPwPlot : public QwtPlot
{
    Q_OBJECT

    public:

        HrPwPlot(MainWindow *mainWindow, HrPwWindow *hrPwWindow);

        RideItem *rideItem;
        QwtPlotMarker *r_mrk1;
        QwtPlotMarker *r_mrk2;
        bool joinLine;
        int shadeZones;

        void setShadeZones(int);
        int isShadeZones() const;
        void refreshZoneLabels();

        int smoothing() const { return smooth; }
        void setDataFromRide(RideItem *ride);
        void setJoinLine(bool value);
        void setAxisTitle(int axis, QString label);

    public slots:
        // for tooltip
        void pointHover(QwtPlotCurve*, int);

    protected:
        friend class ::HrPwPlotBackground;
        friend class ::HrPwPlotZoneLabel;
        friend class ::HrPwWindow;

        HrPwWindow *hrPwWindow;
        MainWindow *mainWindow;

        HrPwPlotBackground *bg;

        QwtPlotGrid *grid;
        QVector<QwtPlotCurve*> hrCurves;
        QwtPlotCurve *regCurve;
        QwtPlotCurve *wattsStepCurve;
        QwtPlotCurve *hrStepCurve;

        QPainter     *painter;

        QVector<double> hrArray;
        QVector<double> wattsArray;
        QVector<double> timeArray;
        QVector<int> interArray;

        int arrayLength;

        double *array;
        int arrayLength2;

        int smooth;
        int hrMin;

        QList <HrPwPlotZoneLabel *> zoneLabels;
        bool shade_zones;     // whether power should be shaded

        void recalc();
        void setYMax();
        void setXTitle();

        void addWattStepCurve(QVector<double> &finalWatts, int nbpoints);
        void addHrStepCurve(QVector<double> &finalHr, int nbpoints);
        void addRegLinCurve(QVector<double> &finalHr, QVector<double> &finalWatts, int nbpoints);

    private:
        QSettings settings;
        QVariant unit;

        LTMToolTip *tooltip;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover
};

#endif // _GC_HrPwPlot_h
