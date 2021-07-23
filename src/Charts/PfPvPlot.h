/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *                    J.T Conklin (jtc@acorntoolworks.com)
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

#ifndef _GC_QaPlot_h
#define _GC_QaPlot_h 1
#include "GoldenCheetah.h"
#include "RideFile.h"

#include <qwt_plot.h>
#include <qwt_point_3d.h>

// forward references
class RideItem;
struct RideFilePoint;
class QwtPlotCurve;
class QwtPlotMarker;
class Context;
class PfPvPlotZoneLabel;

class PfPvPlot : public QwtPlot
{
    Q_OBJECT
    G_OBJECT


    public:

        PfPvPlot(Context *context);
        void refreshZoneItems();
        void setData(RideItem *_rideItem);
        void showIntervals(RideItem *_rideItem);
        void refreshIntervalMarkers();
        void mouseTrack(double cpv, double aepf);

        int getCP();
        void setCP(int cp);
        int getCAD();
        void setCAD(int cadence);
        double getCL();
        void setCL(double cranklen);
        int getPMax();
        void setPMax(int pmax);
        void recalc();
        void recalcCompare();

        // zone shader uses this
        double maxAEPF;
        double maxCPV;
        QVector<double> contour_xvalues;

        RideItem *rideItem;

        bool shadeZones() const { return shade_zones; }
        void setShadeZones(bool value);

        bool mergeIntervals() const { return merge_intervals; }
        void setMergeIntervals(bool value);
        bool frameIntervals() const { return frame_intervals; }
        void setFrameIntervals(bool value);
        bool gearRatioDisplay() const { return gear_ratio_display; }
        void setGearRatioDisplay(bool value);
        void setAxisTitle(int axis, QString label);

        void showCompareIntervals();

    public slots:
        void configChanged(qint32);
        void intervalHover(IntervalItem*);

    signals:
        void changedCP( const QString& );
        void changedCAD( const QString& );
        void changedCL( const QString& );
        void changedPMax( const QString& );

    protected:

        friend class ::PfPvPlotZoneLabel;

        int intervalCount() const;

        Context *context;
        QwtPlotCurve *curve;
        QList <QwtPlotCurve *> gearRatioCurves;
        QwtPlotCurve *hover;
        QList <QwtPlotCurve *> intervalCurves;
        QList <QwtPlotMarker *> intervalMarkers;
        QwtPlotCurve *cpCurve;
        QwtPlotCurve *pmaxCurve;
        QList <QwtPlotCurve *> zoneCurves;
        QList <PfPvPlotZoneLabel *> zoneLabels;
        QwtPlotMarker *mX;
        QwtPlotMarker *mY;

        int cp_;
        int pmax_;
        int cad_;
        double cl_;
        bool shade_zones;    // whether to shade zones, added 27Apr2009 djconnel
        bool merge_intervals, frame_intervals;
        bool gear_ratio_display;

        double timeInQuadrant[4]; // time in seconds spent in each quadrant
        QwtPlotMarker *tiqMarker[4]; // time in seconds spent in each quadrant

    private:
        void mainCurvesSetVisible(bool);
};

#endif // _GC_QaPlot_h
