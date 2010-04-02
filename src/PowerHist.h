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

#ifndef _GC_PowerHist_h
#define _GC_PowerHist_h 1

#include <qwt_plot.h>
#include <qsettings.h>
#include <qvariant.h>

class QwtPlotCurve;
class QwtPlotGrid;
class MainWindow;
class RideItem;
class RideFilePoint;
class PowerHistBackground;
class PowerHistZoneLabel;
class QwtPlotZoomer;

class PowerHist : public QwtPlot
{
    Q_OBJECT

    public:

        QwtPlotCurve *curve, *curveSelected;
	QList <PowerHistZoneLabel *> zoneLabels;

        PowerHist(MainWindow *mainWindow);
	~PowerHist();

        int binWidth() const { return binw; }
        inline bool islnY() const { return lny; }
        inline bool withZeros() const { return withz; }
        bool shadeZones() const;

	enum Selection {
	    wattsShaded,
	    wattsUnshaded,
	    nm,
	    hr,
	    kph,
	    cad
	} selected;
	inline Selection selection() { return selected; }

        void setData(RideItem *_rideItem);

	void setSelection(Selection selection);
	void fixSelection();

        void setBinWidth(int value);
	double getDelta();
	int    getDigits();
        double getBinWidthRealUnits();
        int setBinWidthRealUnits(double value);

	void refreshZoneLabels();

	RideItem *rideItem;

    public slots:

        void setlnY(bool value);
        void setWithZeros(bool value);
        void configChanged();

    protected:

        MainWindow *mainWindow;
        QwtPlotGrid *grid;

        // storage for data counts
        QVector<unsigned int>
        wattsArray,
        nmArray,
        hrArray,
        kphArray,
        cadArray;

        // storage for data counts in interval selected
        QVector<unsigned int>
        wattsSelectedArray,
        nmSelectedArray,
        hrSelectedArray,
        kphSelectedArray,
        cadSelectedArray;

        int binw;

        bool withz;        // whether zeros are included in histogram
	double dt;         // length of sample

        void recalc();
        void setYMax();
        QwtPlotZoomer *zoomer;

    private:
        QSettings *settings;
        QVariant unit;

	PowerHistBackground *bg;
	bool lny;

	// discritized unit for smoothing
        static const double wattsDelta = 1.0;
	static const double nmDelta    = 0.1;
	static const double hrDelta    = 1.0;
	static const double kphDelta   = 0.1;
	static const double cadDelta   = 1.0;

	// digits for text entry validator
        static const int wattsDigits = 0;
	static const int nmDigits    = 1;
	static const int hrDigits    = 0;
	static const int kphDigits   = 1;
	static const int cadDigits   = 0;

	void setParameterAxisTitle();
	bool isSelected(const RideFilePoint *p, double);

	bool useMetricUnits;  // whether metric units are used (or imperial)
};

#endif // _GC_PowerHist_h
