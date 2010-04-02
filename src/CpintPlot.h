/*
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
class QwtPlotMarker;
class RideItem;
class Zones;

QString ride_filename_to_cpi_filename(const QString filename);

class CpintPlot : public QwtPlot
{
    Q_OBJECT

    public:

        CpintPlot(QString path, const Zones *zones);
        bool needToScanRides;

        const QwtPlotCurve *getThisCurve() const { return thisCurve; }
        const QwtPlotCurve *getCPCurve() const { return CPCurve; }

        QVector<QDate> getBestDates() { return bestDates; }
        QVector<double> getBests() { return bests; }
        double cp, tau, t0; // CP model parameters
        void deriveCPParameters();
        bool deleteCpiFile(QString filename);
        void changeSeason(const QDate &start, const QDate &end);
        void setEnergyMode(bool value);
        bool energyMode() const { return energyMode_; }

    public slots:

        void showGrid(int state);
        void calculate(RideItem *rideItem);
        void plot_CP_curve(CpintPlot *plot, double cp, double tau, double t0n);
        void plot_allCurve(CpintPlot *plot, int n_values, const double *power_values);
        void configChanged();

    protected:

        QString path;
        QwtPlotCurve *thisCurve;
        QwtPlotCurve *CPCurve;
        QList<QwtPlotCurve*> allCurves;
        QList<QwtPlotMarker*> allZoneLabels;
        void clear_CP_Curves();
        QStringList filterForSeason(QStringList cpints, QDate startDate, QDate endDate);
        QwtPlotGrid *grid;
        QVector<double> bests;
        QVector<QDate> bestDates;
        QDate startDate;
        QDate endDate;
        const Zones *zones;
        // keys are CPI files contributing to bests (at least originally)
        QHash<QString,bool> cpiDataInBests;
        bool energyMode_;
};

#endif // _GC_CpintPlot_h

