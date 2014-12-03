/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 *               2011 Mark Liversedge (liversedge@gmail.com)
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
#include "RideFile.h"
#include "RideItem.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_marker.h>
#include <qwt_point_3d.h>
#include <qwt_compat.h>
#include <qwt_scale_draw.h>
#include <qsettings.h>
#include <qvariant.h>


class QwtPlotCurve;
class QwtPlotGrid;
class Context;
class RideItem;
struct RideFilePoint;
class RideFileCache;
class HistogramWindow;
class PowerHistBackground;
class PowerHistZoneLabel;
class HrHistBackground;
class HrHistZoneLabel;
class PaceHistBackground;
class PaceHistZoneLabel;
class LTMCanvasPicker;
class ZoneScaleDraw;
class SummaryMetrics;

class penTooltip: public QwtPlotZoomer
{
    public:
         penTooltip(QWidget *canvas): QwtPlotZoomer(canvas), tip("") {
             // With some versions of Qt/Qwt, setting this to AlwaysOn
             // causes an infinite recursion.
             //setTrackerMode(AlwaysOn);
             setTrackerMode(AlwaysOn);
         }

        virtual QwtText trackerText(const QPoint &/*pos*/) const {
            QColor bg = QColor(Qt::lightGray);
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

class HistData // each curve needs a lot of data (!? this may need refactoring, it seems all over the place)
{
    public:

        // storage for data counts
        QVector<unsigned int> aPowerArray, wattsArray, wattsZoneArray,
                              wattsCPZoneArray, wattsKgArray, nmArray,
                              hrArray, hrZoneArray, hrCPZoneArray,
                              kphArray, paceZoneArray, paceCPZoneArray,
                              cadArray, gearArray, smo2Array, metricArray;

        // storage for data counts in interval selected
        QVector<unsigned int> aPowerSelectedArray, wattsSelectedArray,
                              wattsZoneSelectedArray, wattsCPZoneSelectedArray,
                              wattsKgSelectedArray, nmSelectedArray,
                              hrSelectedArray, hrZoneSelectedArray, hrCPZoneSelectedArray,
                              kphSelectedArray, paceZoneSelectedArray, paceCPZoneSelectedArray,
                              cadSelectedArray, smo2SelectedArray, gearSelectedArray;
};

class PowerHist : public QwtPlot
{
    Q_OBJECT
    G_OBJECT

    friend class ::PaceHistBackground;
    friend class ::PaceHistZoneLabel;
    friend class ::HrHistBackground;
    friend class ::HrHistZoneLabel;
    friend class ::PowerHistBackground;
    friend class ::PowerHistZoneLabel;
    friend class ::HistogramWindow;

    public:

        PowerHist(Context *context, bool rangemode);
        ~PowerHist();

        double minX;
        double maxX;
        bool rangemode;

    public slots:

        // public setters
        void setShading(bool x) { shade=x; }
        void setSeries(RideFile::SeriesType series);

        // set data from a ride
        void setData(RideItem *_rideItem, bool force=false);

        // used to set and bin ride data
        void setArraysFromRide(RideFile *ride, HistData &standard, const Zones *zones, RideFileInterval hover);
        void binData(HistData &standard, QVector<double>&, QVector<double>&, QVector<double>&, QVector<double>&);

        // set data from the compare intervals -or- dateranges
        void setDataFromCompare(); // cache
        void setDataFromCompare(QString totalMetric, QString distMetric); // metric

        // set data from a ridefile cache
        void setData(RideFileCache *source);

        // set data from metrics
        void setData(QList<SummaryMetrics>&results, QString totalMetric, QString distMetric,
                     bool isFiltered, QStringList files, HistData *data);

        void setlnY(bool value);
        void setWithZeros(bool value);
        void setZoned(bool value);
        void setCPZoned(bool value);
        void setSumY(bool value);
        void configChanged();
        void setAxisTitle(int axis, QString label);
        void setYMax();
        void setBinWidth(double value);
        void setColor(QColor color) { metricColor = color; }

        // public getters
        void setDelta(double delta) { this->delta = delta; }
        void setDigits(int digits) { this->digits = digits; }
        inline bool islnY() const { return lny; }
        inline bool withZeros() const { return withz; }
        inline double binWidth() const { return binw; }

        // react to plot signals
        void pointHover(QwtPlotCurve *curve, int index);
        void intervalHover(RideFileInterval);

        // get told to refresh
        void recalc(bool force=false); // normal mode recalc
        void recalcCompare(); // compare mode recalc
        void refreshZoneLabels();

        // redraw, reset zoom base
        void updatePlot();

        // hide / show curves etc
        void hideStandard(bool);
        void setComparePens();

    protected:

        bool isZoningEnabled();
        void refreshHRZoneLabels();
        void refreshPaceZoneLabels();
        void setParameterAxisTitle();
        bool isSelected(const RideFilePoint *p, double);
        void percentify(QVector<double> &, double factor); // and a function to convert

        bool shadeZones() const; // check if zone shading is both wanted and possible
        bool shadeHRZones() const; // check if zone shading is both wanted and possible
        bool shadePaceZones() const; // check if zone shading is both wanted and possible

        // plot settings
        RideItem *rideItem;
        Context *context;
        RideFile::SeriesType series;
        bool lny;
        bool shade;
        bool zoned;        // show in zones
        bool cpzoned;        // show in cp zones
        double binw;
        bool withz;        // whether zeros are included in histogram
        double dt;         // length of sample
        bool absolutetime; // do we sum absolute or percentage?

        HistData standard;

    private:

        // plot objects
        QwtPlotGrid *grid;
        PowerHistBackground *bg;
        HrHistBackground *hrbg;
        PaceHistBackground *pacebg;
        penTooltip *zoomer;
        LTMCanvasPicker *canvasPicker;

        // curves when NOT in compare mode
        QwtPlotCurve *curve, *curveSelected, *curveHover;

        // curves when ARE in compare mode
        QList<QwtPlotCurve*> compareCurves;

        // background shading
        QList <PowerHistZoneLabel *> zoneLabels;
        QList <HrHistZoneLabel *> hrzoneLabels;
        QList <PaceHistZoneLabel *> pacezoneLabels;

        // zone data labels (actual values)
        QList <QwtPlotMarker *> zoneDataLabels;

        QString metricX, metricY;
        int digits;
        double delta;

        // source cache
        RideFileCache *cache;

        // data arrays -- for one curve, not in compare mode
        QList<HistData> compareData;


        enum Source { Ride, Cache, Metric } source, LASTsource;
        QColor metricColor;

        // last plot settings - to avoid lots of uneeded recalcs
        RideItem *LASTrideItem;
        RideFileCache *LASTcache;
        RideFile::SeriesType LASTseries;
        bool LASTshade;
        bool LASTuseMetricUnits;  // whether metric units are used (or imperial)
        bool LASTlny;
        bool LASTzoned;        // show in zones
        bool LASTcpzoned;        // show in zones
        double LASTbinw;
        bool LASTwithz;        // whether zeros are included in histogram
        double LASTdt;         // length of sample
        bool LASTabsolutetime; // do we sum absolute or percentage?
};

/*----------------------------------------------------------------------
 * From here to the end of source file the routines for zone shading
 *--------------------------------------------------------------------*/

// define a background class to handle shading of power zones
// draws power zone bands IF zones are defined and the option
// to draw bonds has been selected
class PowerHistBackground: public QwtPlotItem
{
private:
    PowerHist *parent;

public:
    PowerHistBackground(PowerHist *_parent)
    {
        setZ(0.0);
	parent = _parent;
    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw(QPainter *painter,
		      const QwtScaleMap &xMap, const QwtScaleMap &,
                      const QRectF &rect) const
    {
	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const Zones *zones = parent->context->athlete->zones();
        int zone_range = -1;
        if (zones) zone_range = zones->whichRange(rideItem->dateTime.date());

	if (parent->shadeZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    int num_zones = zone_lows.size();
	    if (num_zones > 0) {
		for (int z = 0; z < num_zones; z ++) {
                    QRectF r = rect;

		    QColor shading_color =
			zoneColor(z, num_zones);
		    shading_color.setHsv(
					 shading_color.hue(),
					 shading_color.saturation() / 4,
					 shading_color.value()
					 );

            double wattsLeft = zone_lows[z];
            if (parent->series == RideFile::wattsKg) {
                wattsLeft = wattsLeft / rideItem->ride()->getWeight();
            }
            r.setLeft(xMap.transform(wattsLeft));

            if (z + 1 < num_zones)  {
                double wattsRight = zone_lows[z + 1];
                if (parent->series == RideFile::wattsKg) {
                    wattsRight = wattsRight / rideItem->ride()->getWeight();
                }
                r.setRight(xMap.transform(wattsRight));
            }

		    if (r.right() >= r.left())
			painter->fillRect(r, shading_color);
		}
	    }
	}
    }
};


// Zone labels are drawn if power zone bands are enabled, automatically
// at the center of the plot
class PowerHistZoneLabel: public QwtPlotItem
{
private:
    PowerHist *parent;
    int zone_number;
    double watts;
    QwtText text;

public:
    PowerHistZoneLabel(PowerHist *_parent, int _zone_number)
    {
	parent = _parent;
	zone_number = _zone_number;

	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const Zones *zones = parent->context->athlete->zones();
        int zone_range = -1;
        if (zones) zone_range = zones->whichRange(rideItem->dateTime.date());

	setZ(1.0 + zone_number / 100.0);

	// create new zone labels if we're shading
	if (parent->shadeZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    QList <QString> zone_names = zones->getZoneNames(zone_range);
	    int num_zones = zone_lows.size();
	    assert(zone_names.size() == num_zones);
	    if (zone_number < num_zones) {
		watts =
		    (
		     (zone_number + 1 < num_zones) ?
		     0.5 * (zone_lows[zone_number] + zone_lows[zone_number + 1]) :
		     (
		      (zone_number > 0) ?
		      (1.5 * zone_lows[zone_number] - 0.5 * zone_lows[zone_number - 1]) :
		      2.0 * zone_lows[zone_number]
		      )
             );

		text = QwtText(zone_names[zone_number]);
		text.setFont(QFont("Helvetica",24, QFont::Bold));
		QColor text_color = zoneColor(zone_number, num_zones);
		text_color.setAlpha(64);
		text.setColor(text_color);
	    }
	}

    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    void draw(QPainter *painter,
	      const QwtScaleMap &xMap, const QwtScaleMap &,
              const QRectF &rect) const
    {
        RideItem *rideItem = parent->rideItem;

    if (parent->shadeZones()) {
        double position = watts;
        if (parent->series == RideFile::wattsKg) {
            position = watts / rideItem->ride()->getWeight();
        }
        int x = xMap.transform(position);
	    int y = (rect.bottom() + rect.top()) / 2;

	    // the following code based on source for QwtPlotMarker::draw()
            QRect tr(QPoint(0, 0), text.textSize(painter->font()).toSize());
	    tr.moveCenter(QPoint(y, -x));
	    painter->rotate(90);             // rotate text to avoid overlap: this needs to be fixed
	    text.draw(painter, tr);
	}
    }
};

// define a background class to handle shading of HR zones
// draws power zone bands IF zones are defined and the option
// to draw bonds has been selected
class HrHistBackground: public QwtPlotItem
{
private:
    PowerHist *parent;

public:
    HrHistBackground(PowerHist *_parent)
    {
        setZ(0.0);
	parent = _parent;
    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw(QPainter *painter,
		      const QwtScaleMap &xMap, const QwtScaleMap &,
                      const QRectF &rect) const
    {
	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const HrZones *zones = parent->context->athlete->hrZones();
        int zone_range = -1;
        if (zones) zone_range = zones->whichRange(rideItem->dateTime.date());

	if (parent->shadeHRZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    int num_zones = zone_lows.size();
	    if (num_zones > 0) {
		for (int z = 0; z < num_zones; z ++) {
                    QRectF r = rect;

		    QColor shading_color =
			hrZoneColor(z, num_zones);
		    shading_color.setHsv(
					 shading_color.hue(),
					 shading_color.saturation() / 4,
					 shading_color.value()
					 );
		    r.setLeft(xMap.transform(zone_lows[z]));
		    if (z + 1 < num_zones)
			r.setRight(xMap.transform(zone_lows[z + 1]));
		    if (r.right() >= r.left())
			painter->fillRect(r, shading_color);
		}
	    }
	}
    }
};


// Zone labels are drawn if power zone bands are enabled, automatically
// at the center of the plot
class HrHistZoneLabel: public QwtPlotItem
{
private:
    PowerHist *parent;
    int zone_number;
    double watts;
    QwtText text;

public:
    HrHistZoneLabel(PowerHist *_parent, int _zone_number)
    {
	parent = _parent;
	zone_number = _zone_number;

	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const HrZones *zones = parent->context->athlete->hrZones();
        int zone_range = -1;
        if (zones) zone_range = zones->whichRange(rideItem->dateTime.date());

	setZ(1.0 + zone_number / 100.0);

	// create new zone labels if we're shading
	if (parent->shadeHRZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    QList <QString> zone_names = zones->getZoneNames(zone_range);
	    int num_zones = zone_lows.size();
	    assert(zone_names.size() == num_zones);
	    if (zone_number < num_zones) {
                double min = parent->minX;
                if (zone_lows[zone_number]>min)
                    min = zone_lows[zone_number];

                watts =
		    (
		     (zone_number + 1 < num_zones) ?
                     0.5 * (min + zone_lows[zone_number + 1]) :
		     (
		      (zone_number > 0) ?
		      (1.5 * zone_lows[zone_number] - 0.5 * zone_lows[zone_number - 1]) :
		      2.0 * zone_lows[zone_number]
		      )
		     );

		text = QwtText(zone_names[zone_number]);
		text.setFont(QFont("Helvetica",24, QFont::Bold));
		QColor text_color = hrZoneColor(zone_number, num_zones);
		text_color.setAlpha(64);
		text.setColor(text_color);
	    }
	}

    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    void draw(QPainter *painter,
	      const QwtScaleMap &xMap, const QwtScaleMap &,
              const QRectF &rect) const
    {
	if (parent->shadeHRZones()) {
            int x = xMap.transform(watts);
	    int y = (rect.bottom() + rect.top()) / 2;

	    // the following code based on source for QwtPlotMarker::draw()
            QRect tr(QPoint(0, 0), text.textSize(painter->font()).toSize());
	    tr.moveCenter(QPoint(y, -x));
	    painter->rotate(90);             // rotate text to avoid overlap: this needs to be fixed
	    text.draw(painter, tr);
	}
    }
};

// define a background class to handle shading of pace zones
// draws pace zone bands IF zones are defined and the option
// to draw bonds has been selected
class PaceHistBackground: public QwtPlotItem
{
private:
    PowerHist *parent;

public:
    PaceHistBackground(PowerHist *_parent)
    {
        setZ(0.0);
        parent = _parent;
    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw(QPainter *painter,
                      const QwtScaleMap &xMap, const QwtScaleMap &,
                      const QRectF &rect) const
    {
	RideItem *rideItem = parent->rideItem;

    // only for running activities
	if (! rideItem || ! rideItem->isRun())
	    return;

	const PaceZones *zones = parent->context->athlete->paceZones();
	int zone_range = zones ? zones->whichRange(rideItem->dateTime.date()) : -1;

    // unit conversion factor for imperial units
    const double speed_factor  = (parent->context->athlete->useMetricUnits ? 1.0 : 0.62137119);

	if (parent->shadePaceZones() && (zone_range >= 0)) {
	    QList <double> zone_lows = zones->getZoneLows(zone_range);
	    int num_zones = zone_lows.size();
	    if (num_zones > 0) {
		for (int z = 0; z < num_zones; z ++) {
                    QRectF r = rect;

		    QColor shading_color =
			paceZoneColor(z, num_zones);
		    shading_color.setHsv(
					 shading_color.hue(),
					 shading_color.saturation() / 4,
					 shading_color.value()
					 );
		    r.setLeft(xMap.transform(zone_lows[z]*speed_factor));
		    if (z + 1 < num_zones)
			r.setRight(xMap.transform(zone_lows[z + 1]*speed_factor));
		    if (r.right() >= r.left())
			painter->fillRect(r, shading_color);
		}
	    }
	}
    }
};

// Zone labels are drawn if pace zone bands are enabled, automatically
// at the center of the plot
class PaceHistZoneLabel: public QwtPlotItem
{
private:
    PowerHist *parent;
    int zone_number;
    double watts;
    QwtText text;

public:
    PaceHistZoneLabel(PowerHist *_parent, int _zone_number)
    {
	parent = _parent;
	zone_number = _zone_number;

	RideItem *rideItem = parent->rideItem;

    // only for running activities
	if (! rideItem || ! rideItem->isRun())
	    return;

	const PaceZones *zones = parent->context->athlete->paceZones();
	int zone_range = zones ? zones->whichRange(rideItem->dateTime.date()) : -1;

    // unit conversion factor for imperial units
    const double speed_factor  = (parent->context->athlete->useMetricUnits ? 1.0 : 0.62137119);

	setZ(1.0 + zone_number / 100.0);

	// create new zone labels if we're shading
	if (parent->shadePaceZones() && (zone_range >= 0)) {
	    QList <double> zone_lows = zones->getZoneLows(zone_range);
	    QList <QString> zone_names = zones->getZoneNames(zone_range);
	    int num_zones = zone_lows.size();
	    assert(zone_names.size() == num_zones);
	    if (zone_number < num_zones) {
                double min = parent->minX;
                if (zone_lows[zone_number]*speed_factor > min)
                    min = zone_lows[zone_number]*speed_factor;

                watts =
		    (
		     (zone_number + 1 < num_zones) ?
                     0.5 * (min + zone_lows[zone_number + 1]*speed_factor) :
		     (
		      (zone_number > 0) ?
		      (1.5 * zone_lows[zone_number]*speed_factor - 0.5 * zone_lows[zone_number - 1]*speed_factor) :
		      2.0 * zone_lows[zone_number]*speed_factor
		      )
		     );

		text = QwtText(zone_names[zone_number]);
		text.setFont(QFont("Helvetica",24, QFont::Bold));
		QColor text_color = hrZoneColor(zone_number, num_zones);
		text_color.setAlpha(64);
		text.setColor(text_color);
	    }
	}

    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    void draw(QPainter *painter,
	      const QwtScaleMap &xMap, const QwtScaleMap &,
              const QRectF &rect) const
    {
	if (parent->shadePaceZones()) {
            int x = xMap.transform(watts);
	    int y = (rect.bottom() + rect.top()) / 2;

	    // the following code based on source for QwtPlotMarker::draw()
            QRect tr(QPoint(0, 0), text.textSize(painter->font()).toSize());
	    tr.moveCenter(QPoint(y, -x));
	    painter->rotate(90);             // rotate text to avoid overlap: this needs to be fixed
	    text.draw(painter, tr);
	}
    }
};

class HTimeScaleDraw: public QwtScaleDraw
{

    public:

    HTimeScaleDraw() : QwtScaleDraw() {}

    virtual QwtText label(double v) const
    {
        QTime t = QTime().addSecs(v*60.00);
        if (scaleMap().sDist() > 5)
            return t.toString("hh:mm");
        return t.toString("hh:mm:ss");
    }
};
#endif // _GC_PowerHist_h
