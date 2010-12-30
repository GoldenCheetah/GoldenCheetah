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
#include "GoldenCheetah.h"

#include <qwt_plot.h>
#include <qwt_plot_zoomer.h>
#include <qsettings.h>
#include <qvariant.h>


class QwtPlotCurve;
class QwtPlotGrid;
class MainWindow;
class RideItem;
class RideFilePoint;
class PowerHistBackground;
class PowerHistZoneLabel;
class HrHistBackground;
class HrHistZoneLabel;
class LTMCanvasPicker;
class ZoneScaleDraw;

class penTooltip: public QwtPlotZoomer
{
    public:
         penTooltip(QwtPlotCanvas *canvas):
             QwtPlotZoomer(canvas), tip("")
         {
                 // With some versions of Qt/Qwt, setting this to AlwaysOn
                 // causes an infinite recursion.
                 //setTrackerMode(AlwaysOn);
                 setTrackerMode(AlwaysOn);
         }

    virtual QwtText trackerText(const QwtDoublePoint &/*pos*/) const
    {
        QColor bg = QColor(255,255, 170); // toolyip yellow
#if QT_VERSION >= 0x040300
        bg.setAlpha(200);
#endif
        QwtText text;
        QFont def;
        //def.setPointSize(8); // too small on low res displays (Mac)
        //double val = ceil(pos.y()*100) / 100; // round to 2 decimal place
        //text.setText(QString("%1 %2").arg(val).arg(format), QwtText::PlainText);
        text.setText(tip);
        text.setFont(def);
        text.setBackgroundBrush( QBrush( bg ));
        text.setRenderFlags(Qt::AlignLeft | Qt::AlignTop);
        return text;
    }
    void setFormat(QString fmt) { format = fmt; }
    void setText(QString txt) { tip = txt; }
    private:
    QString format;
    QString tip;
 };

class PowerHist : public QwtPlot
{
    Q_OBJECT
    G_OBJECT


    public:

        QwtPlotCurve *curve, *curveSelected;
	QList <PowerHistZoneLabel *> zoneLabels;
	QList <HrHistZoneLabel *> hrzoneLabels;

        PowerHist(MainWindow *mainWindow);
	~PowerHist();

        int binWidth() const { return binw; }
        inline bool islnY() const { return lny; }
        inline bool withZeros() const { return withz; }
        bool shadeZones() const;
        bool shadeHRZones() const;

	enum Selection {
	    watts,
	    wattsZone,
	    nm,
	    hr,
        hrZone,
	    kph,
	    cad
	} selected;
	inline Selection selection() { return selected; }

    bool shade;
    inline bool shaded() const { return shade; }


        void setData(RideItem *_rideItem);

	void setSelection(Selection selection);
	void fixSelection();

    void setShading(bool x) { shade=x; }

        void setBinWidth(int value);
	double getDelta();
	int    getDigits();
        double getBinWidthRealUnits();
        int setBinWidthRealUnits(double value);

	void refreshZoneLabels();
	void refreshHRZoneLabels();

	RideItem *rideItem;
    MainWindow *mainWindow;

    public slots:

        void setlnY(bool value);
        void setWithZeros(bool value);
        void setSumY(bool value);
        void pointHover(QwtPlotCurve *curve, int index);
        void configChanged();
        void setAxisTitle(int axis, QString label);

    protected:

        QwtPlotGrid *grid;

        // storage for data counts
        QVector<unsigned int>
        wattsArray,
        wattsZoneArray,
        nmArray,
        hrArray,
        hrZoneArray,
        kphArray,
        cadArray;

        // storage for data counts in interval selected
        QVector<unsigned int>
        wattsSelectedArray,
        wattsZoneSelectedArray,
        nmSelectedArray,
        hrSelectedArray,
        hrZoneSelectedArray,
        kphSelectedArray,
        cadSelectedArray;

        int binw;

        bool withz;        // whether zeros are included in histogram
	double dt;         // length of sample

        void recalc();
        void setYMax();
        penTooltip *zoomer;

    private:
        QVariant unit;

	PowerHistBackground *bg;
	HrHistBackground *hrbg;

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

    bool absolutetime; // do we sum absolute or percentage?
    void percentify(QVector<double> &, double factor); // and a function to convert

    LTMCanvasPicker *canvasPicker;
};

#endif // _GC_PowerHist_h
