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

#include "Zones.h"
#include <qwt_plot.h>
#include <qwt_plot_marker.h>   // added djconnel 06Apr2009
#include <QtGui>
#include <QHash>

class QwtPlotCurve;
class QwtPlotGrid;
class RideItem;

#define USE_T0_IN_CP_MODEL 0 // added djconnel 08Apr2009: allow 3-parameter CP model

bool is_ride_filename(const QString filename);
QString ride_filename_to_cpi_filename(const QString filename);
QDate cpi_filename_to_date(const QString filename);

class CpintPlot : public QwtPlot
{
    Q_OBJECT

    public:

        CpintPlot(QString path);
        QProgressDialog *progress;
        bool needToScanRides;

        const QwtPlotCurve *getThisCurve() const { return thisCurve; }

	QVector<QDate> getBestDates() { return bestDates; }
	QVector<double> getBests() { return bests; }
	double cp, tau, t0;                   // CP model parameters
	void deriveCPParameters();            // derive the CP model parameters
	bool deleteCpiFile(QString filename); // delete a CPI file and clean up


    public slots:

        void showGrid(int state);
        void calculate(RideItem *rideItem);
	void plot_CP_curve(
			   CpintPlot *plot,
			   double cp,
			   double tau,
			   double t0n
			   );
	void plot_allCurve(
			   CpintPlot *plot,
			   int n_values,
			   const double *power_values
			   );

    protected:

        QString path;
        QwtPlotCurve *thisCurve;
	QwtPlotCurve *CPCurve;
	QList <QwtPlotCurve *> allCurves;
	QList <QwtPlotMarker *> allZoneLabels;
	void clear_CP_Curves();

        QwtPlotGrid *grid;
        
        QVector<double> bests;
	QVector<QDate> bestDates;

	Zones **zones;                // pointer to power zones added djconnel 24Apr2009

	QHash <QString, bool> cpiDataInBests;  // hash: keys are CPI files contributing to bests (at least originally)
};

#endif // _GC_CpintPlot_h

