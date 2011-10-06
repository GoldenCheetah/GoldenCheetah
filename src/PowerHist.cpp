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

#include "PowerHist.h"
#include "MainWindow.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "Settings.h"
#include "Zones.h"
#include "HrZones.h"
#include "Colors.h"

#include "ZoneScaleDraw.h"

#include <assert.h>
#include <qpainter.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_engine.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_data.h>

#include "LTMCanvasPicker.h" // for tooltip


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
		      const QRect &rect) const
    {
	RideItem *rideItem = parent->rideItem;

	if (! rideItem)
	    return;

	const Zones *zones       = rideItem->zones;
	int zone_range     = rideItem->zoneRange();

	if (parent->shadeZones() && (zone_range >= 0)) {
	    QList <int> zone_lows = zones->getZoneLows(zone_range);
	    int num_zones = zone_lows.size();
	    if (num_zones > 0) {
		for (int z = 0; z < num_zones; z ++) {
		    QRect r = rect;

		    QColor shading_color =
			zoneColor(z, num_zones);
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

	const Zones *zones       = rideItem->zones;
	int zone_range     = rideItem->zoneRange();

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
	      const QRect &rect) const
    {
	if (parent->shadeZones()) {
	    int x = xMap.transform(watts);
	    int y = (rect.bottom() + rect.top()) / 2;

	    // the following code based on source for QwtPlotMarker::draw()
	    QRect tr(QPoint(0, 0), text.textSize(painter->font()));
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
              const QRect &rect) const
    {
    RideItem *rideItem = parent->rideItem;

    if (! rideItem)
        return;

    const HrZones *zones       = parent->mainWindow->hrZones();
    int zone_range     = rideItem->hrZoneRange();

    if (parent->shadeHRZones() && (zone_range >= 0)) {
        QList <int> zone_lows = zones->getZoneLows(zone_range);
        int num_zones = zone_lows.size();
        if (num_zones > 0) {
        for (int z = 0; z < num_zones; z ++) {
            QRect r = rect;

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

    const HrZones *zones       = parent->mainWindow->hrZones();
    int zone_range     = rideItem->hrZoneRange();

    setZ(1.0 + zone_number / 100.0);

    // create new zone labels if we're shading
    if (parent->shadeHRZones() && (zone_range >= 0)) {
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
          const QRect &rect) const
    {
    if (parent->shadeHRZones()) {
        int x = xMap.transform(watts);
        int y = (rect.bottom() + rect.top()) / 2;

        // the following code based on source for QwtPlotMarker::draw()
        QRect tr(QPoint(0, 0), text.textSize(painter->font()));
        tr.moveCenter(QPoint(y, -x));
        painter->rotate(90);             // rotate text to avoid overlap: this needs to be fixed
        text.draw(painter, tr);
    }
    }
};

PowerHist::PowerHist(MainWindow *mainWindow):
    selected(watts),
    shade(false),
    rideItem(NULL),
    mainWindow(mainWindow),
    withz(true),
    unit(0),
    lny(false),
    absolutetime(true)
{

    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    unit = settings->value(GC_UNIT);

    useMetricUnits = (unit.toString() == "Metric");

    binw = settings->value(GC_HIST_BIN_WIDTH, 5).toInt();
    shade = true;

    // create a background object for shading
    bg = new PowerHistBackground(this);
    bg->attach(this);
    hrbg = new HrHistBackground(this);
    hrbg->attach(this);

    setCanvasBackground(Qt::white);

    setParameterAxisTitle();
    setAxisTitle(yLeft, absolutetime ? tr("Time (minutes)") : tr("Time (percent)"));

    curve = new QwtPlotCurve("");
    curve->setStyle(QwtPlotCurve::Steps);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->attach(this);

    curveSelected = new QwtPlotCurve("");
    curveSelected->setStyle(QwtPlotCurve::Steps);
    curveSelected->setRenderHint(QwtPlotItem::RenderAntialiased);
    curveSelected->attach(this);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    grid->attach(this);

    zoneLabels = QList<PowerHistZoneLabel *>();
    hrzoneLabels = QList<HrHistZoneLabel *>();

    zoomer = new penTooltip(this->canvas());
    canvasPicker = new LTMCanvasPicker(this);
    connect(canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), this, SLOT(pointHover(QwtPlotCurve*, int)));

    configChanged();
}

void
PowerHist::configChanged()
{
    // plot background
    setCanvasBackground(GColor(CPLOTBACKGROUND));

    // curve
    QPen pen;
    QColor brush_color;

    switch (selected) {
	case watts:
	case wattsZone:
        pen.setColor(GColor(CPOWER).darker(200));
        brush_color = GColor(CPOWER);
        break;
	case nm:
        pen.setColor(GColor(CTORQUE).darker(200));
        brush_color = GColor(CTORQUE);
        break;
	case hr:
	case hrZone:
        pen.setColor(GColor(CHEARTRATE).darker(200));
        brush_color = GColor(CHEARTRATE);
        break;
	case kph:
        pen.setColor(GColor(CSPEED).darker(200));
        brush_color = GColor(CSPEED);
        break;
	case cad:
        pen.setColor(GColor(CCADENCE).darker(200));
        brush_color = GColor(CCADENCE);
        break;
    }

    double width = 2.0;
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curveSelected->setRenderHint(QwtPlotItem::RenderAntialiased);
    pen.setWidth(width);
    curve->setPen(pen);
    brush_color.setAlpha(64);
    curve->setBrush(brush_color);   // fill below the line

    // intervalselection
    QPen ivl(GColor(CINTERVALHIGHLIGHTER).darker(200));
    ivl.setWidth(width);
    curveSelected->setPen(ivl);
    QColor ivlbrush = GColor(CINTERVALHIGHLIGHTER);
    ivlbrush.setAlpha(64);
    curveSelected->setBrush(ivlbrush);   // fill below the line

    // grid
    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
}

PowerHist::~PowerHist() {
    delete bg;
    delete hrbg;
    delete curve;
    delete curveSelected;
    delete grid;
}

// static const variables from PoweHist.h:
// discritized unit for smoothing
const double PowerHist::wattsDelta;
const double PowerHist::nmDelta;
const double PowerHist::hrDelta;
const double PowerHist::kphDelta;
const double PowerHist::cadDelta;

// digits for text entry validator
const int PowerHist::wattsDigits;
const int PowerHist::nmDigits;
const int PowerHist::hrDigits;
const int PowerHist::kphDigits;
const int PowerHist::cadDigits;

void
PowerHist::refreshZoneLabels()
{
    // delete any existing power zone labels
    if (zoneLabels.size()) {
	QListIterator<PowerHistZoneLabel *> i(zoneLabels);
	while (i.hasNext()) {
	    PowerHistZoneLabel *label = i.next();
	    label->detach();
	    delete label;
	}
    }
    zoneLabels.clear();

    if (! rideItem)
	return;

    if (selected == watts) {
	const Zones *zones = rideItem->zones;
	int zone_range = rideItem->zoneRange();

        // generate labels for existing zones
	if (zone_range >= 0) {
	    int num_zones = zones->numZones(zone_range);
	    for (int z = 0; z < num_zones; z ++) {
		PowerHistZoneLabel *label = new PowerHistZoneLabel(this, z);
		label->attach(this);
		zoneLabels.append(label);
	    }
	}
    }
}

void
PowerHist::refreshHRZoneLabels()
{
    // delete any existing power zone labels
    if (hrzoneLabels.size()) {
    QListIterator<HrHistZoneLabel *> i(hrzoneLabels);
    while (i.hasNext()) {
        HrHistZoneLabel *label = i.next();
        label->detach();
        delete label;
    }
    }
    hrzoneLabels.clear();

    if (! rideItem)
    return;

    if (selected == hr) {
    const HrZones *zones       = mainWindow->hrZones();
    int zone_range     = rideItem->hrZoneRange();

        // generate labels for existing zones
    if (zone_range >= 0) {
        int num_zones = zones->numZones(zone_range);
        for (int z = 0; z < num_zones; z ++) {
        HrHistZoneLabel *label = new HrHistZoneLabel(this, z);
        label->attach(this);
        hrzoneLabels.append(label);
        }
    }
    }
}

void
PowerHist::recalc()
{
    QVector<unsigned int> *array;
    QVector<unsigned int> *selectedArray;
    int arrayLength = 0;
    double delta;

    // make sure the interval length is set
    if (dt <= 0)
	return;

    if (selected == watts) {
	array = &wattsArray;
	delta = wattsDelta;
	arrayLength = wattsArray.size();
	selectedArray = &wattsSelectedArray;
    }
    else if (selected == wattsZone) {
	array = &wattsZoneArray;
	delta = 1;
	arrayLength = wattsZoneArray.size();
	selectedArray = &wattsZoneSelectedArray;
    }
    else if (selected == nm) {
	array = &nmArray;
	delta = nmDelta;
	arrayLength = nmArray.size();
	selectedArray = &nmSelectedArray;
    }
    else if (selected == hr) {
	array = &hrArray;
	delta = hrDelta;
	arrayLength = hrArray.size();
	selectedArray = &hrSelectedArray;
    }
    else if (selected == hrZone) {
	array = &hrZoneArray;
	delta = 1;
	arrayLength = hrZoneArray.size();
	selectedArray = &hrZoneSelectedArray;
    }
    else if (selected == kph) {
	array = &kphArray;
	delta = kphDelta;
	arrayLength = kphArray.size();
	selectedArray = &kphSelectedArray;
    }
    else if (selected == cad) {
	array = &cadArray;
	delta = cadDelta;
	arrayLength = cadArray.size();
	selectedArray = &cadSelectedArray;
    }

    if (!array)
        return;

    // binning of data when not zoned
    if (selected != wattsZone && selected != hrZone) {

        // we add a bin on the end since the last "incomplete" bin
        // will be dropped otherwise
        int count = int(ceil((arrayLength - 1) / binw))+1;

        // allocate space for data, plus beginning and ending point
        QVector<double> parameterValue(count+2, 0.0);
        QVector<double> totalTime(count+2, 0.0);
        QVector<double> totalTimeSelected(count+2, 0.0);
        int i;
        for (i = 1; i <= count; ++i) {
            int high = i * binw;
            int low = high - binw;
            if (low==0 && !withz)
                low++;
            parameterValue[i] = high * delta;
            totalTime[i]  = 1e-9;  // nonzero to accomodate log plot
            totalTimeSelected[i] = 1e-9;  // nonzero to accomodate log plot
            while (low < high && low<arrayLength) {
                if (selectedArray && (*selectedArray).size()>low)
                    totalTimeSelected[i] += dt * (*selectedArray)[low];
                totalTime[i] += dt * (*array)[low++];
            }
        }
        totalTime[i] = 1e-9;       // nonzero to accomodate log plot
        totalTimeSelected[i] = 1e-9;
        parameterValue[i] = i * delta * binw;
        totalTime[0] = 1e-9;
        totalTimeSelected[0] = 1e-9;
        parameterValue[0] = 0;

        // convert vectors from absolute time to percentage
        // if the user has selected that
        if (!absolutetime) {
            percentify(totalTime, 1);
            percentify(totalTimeSelected, 1);
        }

        curve->setData(parameterValue.data(), totalTime.data(), count + 2);
        curveSelected->setData(parameterValue.data(), totalTimeSelected.data(), count + 2);

        // make see through if we're shading zones
        QBrush brush = curve->brush();
        QColor bcol = brush.color();
        bool zoning = (selected == watts && shadeZones()) || (selected == hr && shadeHRZones());
        bcol.setAlpha(zoning ? 165 : 200);
        brush.setColor(bcol);
        curve->setBrush(brush);

        setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw);

        // HR typically starts at 80 or so, rather than zero
        // lets crop the chart so we can focus on the data
        // if we're working with HR data...
        if (selected == hr) {
            double MinX=0;
            for (int i=0; i<hrArray.size(); i++) {
                if (hrArray[i] > 0) {
                    MinX = i;
                    break;
                }
            }
            setAxisScale(xBottom, MinX, parameterValue[count + 1]);

        } else {

            setAxisScale(xBottom, 0.0, parameterValue[count + 1]);
        }

        // we only do zone labels when using absolute values
        refreshZoneLabels();
        refreshHRZoneLabels();

    } else {

        // we're not binning instead we are prettyfing the columnar
        // display in much the same way as the weekly summary workds
        // Each zone column will have 4 points
        QVector<double> xaxis (array->size() * 4);
        QVector<double> yaxis (array->size() * 4);
        QVector<double> selectedxaxis (selectedArray->size() * 4);
        QVector<double> selectedyaxis (selectedArray->size() * 4);

        // samples to time
        for (int i=0, offset=0; i<array->size(); i++) {

            double x = (double) i - 0.5;
            double y = dt * (double)(*array)[i];

            xaxis[offset] = x +0.05;
            yaxis[offset] = 0;
            offset++;
            xaxis[offset] = x+0.05;
            yaxis[offset] = y;
            offset++;
            xaxis[offset] = x+0.95;
            yaxis[offset] = y;
            offset++;
            xaxis[offset] = x +0.95;
            yaxis[offset] = 0;
            offset++;
        }

        for (int i=0, offset=0; i<selectedArray->size(); i++) {
            double x = (double)i - 0.5;
            double y = dt * (double)(*selectedArray)[i];

            selectedxaxis[offset] = x +0.05;
            selectedyaxis[offset] = 0;
            offset++;
            selectedxaxis[offset] = x+0.05;
            selectedyaxis[offset] = y;
            offset++;
            selectedxaxis[offset] = x+0.95;
            selectedyaxis[offset] = y;
            offset++;
            selectedxaxis[offset] = x +0.95;
            selectedyaxis[offset] = 0;
            offset++;
        }

        if (!absolutetime) {
            percentify(yaxis, 2);
            percentify(selectedyaxis, 2);
        }
        // set those curves
        curve->setData(xaxis.data(), yaxis.data(), xaxis.size());

        // Opaque - we don't need to show zone shading
        QBrush brush = curve->brush();
        QColor bcol = brush.color();
        bcol.setAlpha(200);
        brush.setColor(bcol);
        curve->setBrush(brush);

        curveSelected->setData(selectedxaxis.data(), selectedyaxis.data(), selectedxaxis.size());

        // zone scale draw
        if (selected == wattsZone && rideItem && rideItem->zones) {
            setAxisScaleDraw(QwtPlot::xBottom, new ZoneScaleDraw(rideItem->zones, rideItem->zoneRange()));
            setAxisScale(QwtPlot::xBottom, -0.99, rideItem->zones->numZones(rideItem->zoneRange()), 1);
        }

        // hr scale draw
        int hrRange;
        if (selected == hrZone && rideItem && mainWindow->hrZones() &&
            (hrRange=mainWindow->hrZones()->whichRange(rideItem->dateTime.date())) != -1) {
            setAxisScaleDraw(QwtPlot::xBottom, new HrZoneScaleDraw(mainWindow->hrZones(), hrRange));
            setAxisScale(QwtPlot::xBottom, -0.99, mainWindow->hrZones()->numZones(hrRange), 1);
        }

        setAxisMaxMinor(QwtPlot::xBottom, 0);
    }

    setYMax();
    replot();
}

void
PowerHist::setYMax()
{
    double MaxY = curve->maxYValue();
    if (MaxY < curveSelected->maxYValue()) MaxY = curveSelected->maxYValue();
    static const double tmin = 1.0/60;
    setAxisScale(yLeft, (lny ? tmin : 0.0), MaxY * 1.1);
}

void
PowerHist::setData(RideItem *_rideItem)
{
    rideItem = _rideItem;

    RideFile *ride = rideItem->ride();

    if (ride) {
        setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

        static const int maxSize = 4096;

        // recording interval in minutes
        dt = ride->recIntSecs() / 60.0;

        wattsArray.resize(0);
        wattsZoneArray.resize(0);
        nmArray.resize(0);
        hrArray.resize(0);
        hrZoneArray.resize(0);
        kphArray.resize(0);
        cadArray.resize(0);

        wattsSelectedArray.resize(0);
        wattsZoneSelectedArray.resize(0);
        nmSelectedArray.resize(0);
        hrSelectedArray.resize(0);
        hrZoneSelectedArray.resize(0);
        kphSelectedArray.resize(0);
        cadSelectedArray.resize(0);

        // unit conversion factor for imperial units for selected parameters
        double torque_factor = (useMetricUnits ? 1.0 : 0.73756215);
        double speed_factor  = (useMetricUnits ? 1.0 : 0.62137119);

        foreach(const RideFilePoint *p1, ride->dataPoints()) {
            bool selected = isSelected(p1, ride->recIntSecs());

            // watts array
            int wattsIndex = int(floor(p1->watts / wattsDelta));
            if (wattsIndex >= 0 && wattsIndex < maxSize) {
                if (wattsIndex >= wattsArray.size())
                    wattsArray.resize(wattsIndex + 1);
                wattsArray[wattsIndex]++;

                if (selected) {
                    if (wattsIndex >= wattsSelectedArray.size())
                        wattsSelectedArray.resize(wattsIndex + 1);
                    wattsSelectedArray[wattsIndex]++;
                }
            }

            // watts zoned array
            const Zones *zones = rideItem->zones;
            int zoneRange = zones ? zones->whichRange(ride->startTime().date()) : -1;

            // Only calculate zones if we have a valid range and check zeroes
            if (zoneRange > -1 && (withz || (!withz && p1->watts))) {
                wattsIndex = zones->whichZone(zoneRange, p1->watts);

                if (wattsIndex >= 0 && wattsIndex < maxSize) {
                    if (wattsIndex >= wattsZoneArray.size())
                        wattsZoneArray.resize(wattsIndex + 1);
                    wattsZoneArray[wattsIndex]++;

                    if (selected) {
                        if (wattsIndex >= wattsZoneSelectedArray.size())
                            wattsZoneSelectedArray.resize(wattsIndex + 1);
                        wattsZoneSelectedArray[wattsIndex]++;
                    }
                }
            }

            int nmIndex = int(floor(p1->nm * torque_factor / nmDelta));
            if (nmIndex >= 0 && nmIndex < maxSize) {
                if (nmIndex >= nmArray.size())
                    nmArray.resize(nmIndex + 1);
                nmArray[nmIndex]++;

                if (selected) {
                    if (nmIndex >= nmSelectedArray.size())
                        nmSelectedArray.resize(nmIndex + 1);
                    nmSelectedArray[nmIndex]++;
                }
            }

	    int hrIndex = int(floor(p1->hr / hrDelta));
	    if (hrIndex >= 0 && hrIndex < maxSize) {
            if (hrIndex >= hrArray.size())
               hrArray.resize(hrIndex + 1);
            hrArray[hrIndex]++;

            if (selected) {
                if (hrIndex >= hrSelectedArray.size())
                    hrSelectedArray.resize(hrIndex + 1);
                hrSelectedArray[hrIndex]++;
            }
	    }

        // hr zoned array
        int hrZoneRange = mainWindow->hrZones() ? mainWindow->hrZones()->whichRange(ride->startTime().date()) : -1;

        // Only calculate zones if we have a valid range
        if (hrZoneRange > -1 && (withz || (!withz && p1-hr))) {
            hrIndex = mainWindow->hrZones()->whichZone(hrZoneRange, p1->hr);

            if (hrIndex >= 0 && hrIndex < maxSize) {
                if (hrIndex >= hrZoneArray.size())
                    hrZoneArray.resize(hrIndex + 1);
                hrZoneArray[hrIndex]++;

                if (selected) {
                    if (hrIndex >= hrZoneSelectedArray.size())
                        hrZoneSelectedArray.resize(hrIndex + 1);
                    hrZoneSelectedArray[hrIndex]++;
                }
            }
        }

	    int kphIndex = int(floor(p1->kph * speed_factor / kphDelta));
	    if (kphIndex >= 0 && kphIndex < maxSize) {
	        if (kphIndex >= kphArray.size())
	            kphArray.resize(kphIndex + 1);
	        kphArray[kphIndex]++;

            if (selected) {
                if (kphIndex >= kphSelectedArray.size())
                    kphSelectedArray.resize(kphIndex + 1);
                kphSelectedArray[kphIndex]++;
            }
	    }

	    int cadIndex = int(floor(p1->cad / cadDelta));
	    if (cadIndex >= 0 && cadIndex < maxSize) {
		    if (cadIndex >= cadArray.size())
		        cadArray.resize(cadIndex + 1);
		    cadArray[cadIndex]++;

            if (selected) {
                if (cadIndex >= cadSelectedArray.size())
                    cadSelectedArray.resize(cadIndex + 1);
                cadSelectedArray[cadIndex]++;
            }
	    }


	}

	recalc();
    }
    else {
	setTitle("no data");
	replot();
    }
    zoomer->setZoomBase();
}

void
PowerHist::setBinWidth(int value)
{
    binw = value;
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    settings->setValue(GC_HIST_BIN_WIDTH, value);
    recalc();
}

double
PowerHist::getDelta()
{
    switch (selected) {
    case watts:
    case wattsZone:
	return wattsDelta;
    case nm:
	return nmDelta;
    case hr:
    case hrZone:
	return hrDelta;
    case kph:
	return kphDelta;
    case cad:
	return cadDelta;
    }
    return 1;
}

int
PowerHist::getDigits()
{
    switch (selected) {
    case watts:
    case wattsZone:
	return wattsDigits;
    case nm:
	return nmDigits;
    case hr:
    case hrZone:
	return hrDigits;
    case kph:
	return kphDigits;
    case cad:
	return cadDigits;
    }
    return 1;
}

int
PowerHist::setBinWidthRealUnits(double value)
{
    setBinWidth(round(value / getDelta()));
    return binw;
}

double
PowerHist::getBinWidthRealUnits()
{
    return binw * getDelta();
}

void
PowerHist::setWithZeros(bool value)
{
    withz = value;
    setData(rideItem); // for zone recalculating with/without zero
    recalc();
}

void
PowerHist::setlnY(bool value)
{
    // note: setAxisScaleEngine deletes the old ScaleEngine, so specifying
    // "new" in the argument list is not a leak

    lny=value;
    if (lny && selected != wattsZone && selected != hrZone)
    {
        setAxisScaleEngine(yLeft, new QwtLog10ScaleEngine);
	    curve->setBaseline(1e-6);
	    curveSelected->setBaseline(1e-6);
    }
    else
    {
        setAxisScaleEngine(yLeft, new QwtLinearScaleEngine);
	    curve->setBaseline(0);
	    curveSelected->setBaseline(0);
    }
    setYMax();
    replot();
}

void
PowerHist::setSumY(bool value)
{
    absolutetime = value;
    setParameterAxisTitle();
    setData(rideItem); // for zone recalculating with/without zero
    recalc();
}

void
PowerHist::setParameterAxisTitle()
{
    QString axislabel;
    switch (selected) {
        case watts:
            axislabel = tr("Power (watts)");
            break;
        case wattsZone:
            axislabel = tr("Power zone");
            break;
        case hr:
            axislabel = tr("Heartrate (bpm)");
            break;
        case hrZone:
            axislabel = tr("Heartrate zone");
            break;
        case cad:
            axislabel = tr("Cadence (rpm)");
            break;
        case kph:
            axislabel = QString(tr("Speed (%1)")).arg(useMetricUnits ? tr("km/h") : tr("mph"));
            break;
        case nm:
            axislabel = QString(tr("Torque (%1)")).arg(useMetricUnits ? tr("N-m") : tr("ft-lbf"));
            break;
    }
    setAxisTitle(xBottom, axislabel);
    setAxisTitle(yLeft, absolutetime ? tr("Time (minutes)") : tr("Time (percent)"));
}

void
PowerHist::setSelection(Selection selection) {
    selected = selection;
    if (selected == wattsZone || selected == hrZone) setlnY(false);
    configChanged(); // set colors
    setParameterAxisTitle();
    recalc();
}


void
PowerHist::fixSelection() {

    Selection s = selected;
    RideFile *ride = rideItem->ride();

    int powerRange = mainWindow->zones()->whichRange(rideItem->dateTime.date());
    int hrRange = mainWindow->hrZones()->whichRange(rideItem->dateTime.date());

    if (ride) do {

	    if (s == watts) {
		    if (ride->areDataPresent()->watts) setSelection(s);
		    else s = nm;

        } else if (s == wattsZone) {
		    if (ride->areDataPresent()->watts && powerRange != -1) setSelection(s);
		    else s = nm;

        } else if (s == nm) {
		    if (ride->areDataPresent()->nm) setSelection(s);
		    else s = hr;

        } else if (s == hr) {
		    if (ride->areDataPresent()->hr) setSelection(s);
		    else s = kph;

        } else if (s == hrZone) {
		    if (ride->areDataPresent()->hr && hrRange != -1) setSelection(s);
		    else s = kph;

        } else if (s == kph) {
		    if (ride->areDataPresent()->kph) setSelection(s);
		    else s = cad;

        } else if (s == cad) {
		    if (ride->areDataPresent()->cad) setSelection(s);
		    else s = watts;
        }

	} while (s != selected);
}


bool PowerHist::shadeZones() const
{
    return (
	    rideItem &&
	    rideItem->ride() &&
	    selected == watts &&
        shaded() == true
	    );
}

bool PowerHist::shadeHRZones() const
{
    return (
        rideItem &&
        rideItem->ride() &&
        selected == hr &&
        shaded() == true
        );
}

bool PowerHist::isSelected(const RideFilePoint *p, double sample) {
    if (mainWindow->allIntervalItems() != NULL) {
        for (int i=0; i<mainWindow->allIntervalItems()->childCount(); i++) {
            IntervalItem *current = dynamic_cast<IntervalItem*>(mainWindow->allIntervalItems()->child(i));
            if (current != NULL) {
                if (current->isSelected() && p->secs+sample>current->start && p->secs<current->stop) {
                    return true;
                }
            }
        }
    }
    return false;
}

void
PowerHist::pointHover(QwtPlotCurve *curve, int index)
{
    if (index >= 0) {

        double xvalue = curve->x(index);
        double yvalue = curve->y(index);
        QString text;

        if ((selected == hrZone || selected == wattsZone) && yvalue > 0) {
            // output the tooltip
            text = QString("%1 %2").arg(yvalue, 0, 'f', 1).arg(absolutetime ? tr("minutes") : tr("%"));

            // set that text up
            zoomer->setText(text);
            return;

        } else if (yvalue > 0) {

            // output the tooltip
            text = QString("%1 %2\n%3 %4")
                        .arg(xvalue, 0, 'f', getDigits())
                        .arg(this->axisTitle(curve->xAxis()).text())
                        .arg(yvalue, 0, 'f', 1)
                        .arg(absolutetime ? tr("minutes") : tr("%"));

            // set that text up
            zoomer->setText(text);
            return;
        }
    }
    // no point
    zoomer->setText("");
}

// because we need to effectively draw bars when showing
// time in zone (i.e. for every zone there are 2 points for each
// zone - top left and top right) we need to multiply the percentage
// values by 2 to take this into account
void
PowerHist::percentify(QVector<double> &array, double factor)
{
    double total=0;
    foreach (double current, array) total += current;

    if (total > 0)
        for (int i=0; i< array.size(); i++)
            if (array[i] > 0.01) // greater than 0.8s (i.e. not a double storage issue)
                array[i] = factor * (array[i] / total) * (double)100.00;
}
