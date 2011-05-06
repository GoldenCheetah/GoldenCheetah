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

#include "PfPvPlot.h"
#include "MainWindow.h"
#include "RideFile.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "Settings.h"
#include "Zones.h"
#include "Colors.h"

#include <math.h>
#include <assert.h>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <set>

#define PI M_PI


// Zone labels are drawn if power zone bands are enabled, automatically
// at the center of the plot
class PfPvPlotZoneLabel: public QwtPlotItem
{
    private:
        PfPvPlot *parent;
        int zone_number;
        double watts;
        QwtText text;

    public:
        PfPvPlotZoneLabel(PfPvPlot *_parent, int _zone_number)
        {
	    parent = _parent;
	    zone_number = _zone_number;

	    RideItem *rideItem = parent->rideItem;
	    const Zones *zones = rideItem->zones;
	    int zone_range = rideItem->zoneRange();

	    setZ(1.0 + zone_number / 100.0);

	    // create new zone labels if we're shading
	    if (zone_range >= 0) {
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
	           const QwtScaleMap &xMap, const QwtScaleMap &yMap,
	           const QRect &rect) const
        {
	    if (parent->shadeZones() &&
		(rect.width() > 0) &&
		(rect.height() > 0)
		) {
		// draw the label along a plot diagonal:
		// 1. x*y = watts * dx/dv * dy/df
		// 2. x/width = y/height
		// =>
		// 1. x^2 = width/height * watts
		// 2. y^2 = height/width * watts

		double xscale = fabs(xMap.transform(3) - xMap.transform(0)) / 3;
		double yscale = fabs(yMap.transform(600) - yMap.transform(0)) / 600;
		if ((xscale > 0) && (yscale > 0)) {
		    double w = watts * xscale * yscale;
		    int x = xMap.transform(sqrt(w * rect.width() / rect.height()) / xscale);
		    int y = yMap.transform(sqrt(w * rect.height() / rect.width()) / yscale);

		    // the following code based on source for QwtPlotMarker::draw()
		    QRect tr(QPoint(0, 0), text.textSize(painter->font()));
		    tr.moveCenter(QPoint(x, y));
		    text.draw(painter, tr);
		}
	    }
	}
};


QwtArray<double> PfPvPlot::contour_xvalues;

PfPvPlot::PfPvPlot(MainWindow *mainWindow)
    : rideItem (NULL),
      mainWindow(mainWindow),
      cp_ (0),
      cad_ (85),
      cl_ (0.175),
      shade_zones(true)
{
    setCanvasBackground(Qt::white);

    setAxisTitle(yLeft, tr("Average Effective Pedal Force (N)"));
    setAxisScale(yLeft, 0, 600);
    setAxisTitle(xBottom, tr("Circumferential Pedal Velocity (m/s)"));
    setAxisScale(xBottom, 0, 3);

    mX = new QwtPlotMarker();
    mX->setLineStyle(QwtPlotMarker::VLine);
    mX->attach(this);

    mY = new QwtPlotMarker();
    mY->setLineStyle(QwtPlotMarker::HLine);
    mY->attach(this);

    cpCurve = new QwtPlotCurve();
    cpCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    cpCurve->attach(this);

    curve = new QwtPlotCurve();
    curve->attach(this);
    
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
 
    cl_ = settings->value(GC_CRANKLENGTH).toDouble() / 1000.0;

    merge_intervals = false;
    frame_intervals = true;

    configChanged();

    recalc();
}

void
PfPvPlot::configChanged()
{
    setCanvasBackground(GColor(CPLOTBACKGROUND));

    // frame with inverse of background
    QwtSymbol sym;
    sym.setStyle(QwtSymbol::Ellipse);
    sym.setSize(6);
    sym.setPen(GCColor::invert(GColor(CPLOTBACKGROUND)));
    sym.setBrush(QBrush(Qt::NoBrush));
    curve->setSymbol(sym);
    curve->setStyle(QwtPlotCurve::Dots);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // use grid line color for mX, mY and CPcurve
    QPen marker = GColor(CPLOTMARKER);
    QPen cp = GColor(CCP);
    mX->setLinePen(marker);
    mY->setLinePen(marker);
    cpCurve->setPen(cp);
}

void
PfPvPlot::refreshZoneItems()
{
    // clear out any zone curves which are presently defined
    if (zoneCurves.size()) {
	QListIterator<QwtPlotCurve *> i(zoneCurves); 
	while (i.hasNext()) {
	    QwtPlotCurve *curve = i.next();
	    curve->detach();
	    delete curve;
	}
    }
    zoneCurves.clear();

    
    // delete any existing power zone labels
    if (zoneLabels.size()) {
	QListIterator<PfPvPlotZoneLabel *> i(zoneLabels); 
	while (i.hasNext()) {
	    PfPvPlotZoneLabel *label = i.next();
	    label->detach();
	    delete label;
	}
    }
    zoneLabels.clear();

    if (! rideItem)
	return;

    const Zones *zones = rideItem->zones;
    int zone_range = rideItem->zoneRange();
    
    if (zone_range >= 0) {
	setCP(zones->getCP(zone_range));

	// populate the zone curves
	QList <int> zone_power = zones->getZoneLows(zone_range);
	QList <QString> zone_name = zones->getZoneNames(zone_range);
	int num_zones = zone_power.size();
	assert(zone_name.size() == num_zones);
	if (num_zones > 0) {	
	    QPen *pen = new QPen();
	    pen->setStyle(Qt::NoPen);

	
	    QwtArray<double> yvalues;

	    // generate x values
	    for (int z = 0; z < num_zones; z ++) {
		QwtPlotCurve *curve;
		curve = new QwtPlotCurve(zone_name[z]);
		curve->setPen(*pen);
		QColor brush_color = zoneColor(z, num_zones);
		brush_color.setHsv(
				   brush_color.hue(),
				   brush_color.saturation() / 4,
				   brush_color.value()
				   );
		curve->setBrush(brush_color);   // fill below the line
		curve->setZ(1 - 1e-6 * zone_power[z]);

		// generate data for curve
		if (z < num_zones - 1) {
		    QwtArray <double> contour_yvalues;
		    int watts = zone_power[z + 1];
		    int dwatts = (double) watts;
		    for (int i = 0; i < contour_xvalues.size(); i ++)
			contour_yvalues.append(
					       (1e6 * contour_xvalues[i] < watts) ?
					       1e6 :
					       dwatts / contour_xvalues[i]
					       );
		    curve->setData(contour_xvalues, contour_yvalues);
		}
		else {
		    // top zone has a curve at "infinite" power
		    QwtArray <double> contour_x;
		    QwtArray <double> contour_y;
		    contour_x.append(contour_xvalues[0]);
		    contour_x.append(contour_xvalues[contour_xvalues.size() - 1]);
		    contour_y.append(1e6);
		    contour_y.append(1e6);
		    curve->setData(contour_x, contour_y);
		}
		curve->setVisible(shade_zones);
		curve->attach(this);
		zoneCurves.append(curve);
	    }
	    
	    delete pen;


	    // generate labels for existing zones
	    for (int z = 0; z < num_zones; z ++) {
		PfPvPlotZoneLabel *label = new PfPvPlotZoneLabel(this, z);
		label->setVisible(shade_zones);
		label->attach(this);
		zoneLabels.append(label);
	    }
	    // get the zones visible, even if data may take awhile
	    //replot();

	}
    }
}


// how many intervals selected?
int PfPvPlot::intervalCount() const
{
    int highlighted;
    highlighted = 0;
    if (mainWindow->allIntervalItems() == NULL) return 0; // not inited yet!

    for (int i=0; i<mainWindow->allIntervalItems()->childCount(); i++) {
        IntervalItem *current = dynamic_cast<IntervalItem *>(mainWindow->allIntervalItems()->child(i));
        if (current != NULL) {
            if (current->isSelected() == true) {
                ++highlighted;
            }
        }
    }
    return highlighted;
}

void
PfPvPlot::setData(RideItem *_rideItem)
{
    // clear out any interval curves which are presently defined
    if (intervalCurves.size()) {
       QListIterator<QwtPlotCurve *> i(intervalCurves);
       while (i.hasNext()) {
           QwtPlotCurve *curve = i.next();
           curve->detach();
           delete curve;
       }
    }
    intervalCurves.clear();

    rideItem = _rideItem;

    RideFile *ride = rideItem->ride();

    if (ride) {
	setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

	// quickly erase old data
	curve->setVisible(false);

	// handle zone stuff
	refreshZoneItems();

	// due to the discrete power and cadence values returned by the
	// power meter, there will very likely be many duplicate values.
	// Rather than pass them all to the curve, use a set to strip
	// out duplicates.
	std::set<std::pair<double, double> > dataSet;
	std::set<std::pair<double, double> > dataSetSelected;

	long tot_cad = 0;
	long tot_cad_points = 0;


        foreach(const RideFilePoint *p1, ride->dataPoints()) {

	    if (p1->watts != 0 && p1->cad != 0) {
		double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
		double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

       dataSet.insert(std::make_pair<double, double>(aepf, cpv));
		tot_cad += p1->cad;
		tot_cad_points++;
        
	    }
	}

	if (tot_cad_points == 0) {
	    setTitle(tr("no cadence"));
	    refreshZoneItems();
	    curve->setVisible(false);
	}
	    
	else {
	    // Now that we have the set of points, transform them into the
	    // QwtArrays needed to set the curve's data.
	    QwtArray<double> aepfArray;
	    QwtArray<double> cpvArray;

	    std::set<std::pair<double, double> >::const_iterator j(dataSet.begin());
	    while (j != dataSet.end()) {
		const std::pair<double, double>& dataPoint = *j;

		aepfArray.push_back(dataPoint.first);
		cpvArray.push_back(dataPoint.second);

		++j;
	    }

	    setCAD(tot_cad / tot_cad_points);
	    curve->setData(cpvArray, aepfArray);
	    QwtSymbol sym;
	    sym.setStyle(QwtSymbol::Ellipse);
	    sym.setSize(6);
	    sym.setBrush(QBrush(Qt::NoBrush));

	    // now show the data (zone shading would already be visible)
	    curve->setVisible(true);
	}
    }
    else {
	setTitle("no data");
	refreshZoneItems();
	curve->setVisible(false);
    }

    replot();

    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    setCL(settings->value(GC_CRANKLENGTH).toDouble() / 1000.0);
}

void
PfPvPlot::showIntervals(RideItem *_rideItem)
{
    // clear out any interval curves which are presently defined
    if (intervalCurves.size()) {
       QListIterator<QwtPlotCurve *> i(intervalCurves);
       while (i.hasNext()) {
           QwtPlotCurve *curve = i.next();
           curve->detach();
           delete curve;
       }
    }
    intervalCurves.clear();

    rideItem = _rideItem;

    RideFile *ride = rideItem->ride();

    if (ride) {
       int num_intervals=intervalCount();

       if (mergeIntervals()) num_intervals = 1;
	   if (frameIntervals() || num_intervals==0) curve->setVisible(true);
	   if (frameIntervals()==false && num_intervals) curve->setVisible(false);
       QVector<std::set<std::pair<double, double> > > dataSetInterval(num_intervals);

       long tot_cad = 0;
       long tot_cad_points = 0;

        foreach(const RideFilePoint *p1, ride->dataPoints()) {

            if (p1->watts != 0 && p1->cad != 0) {
                double aepf = (p1->watts * 60.0) / (p1->cad * cl_ * 2.0 * PI);
                double cpv = (p1->cad * cl_ * 2.0 * PI) / 60.0;

                for (int high=-1, t=0; t<mainWindow->allIntervalItems()->childCount(); t++) {
                    IntervalItem *current = dynamic_cast<IntervalItem *>(mainWindow->allIntervalItems()->child(t));
                    if ((current != NULL) && current->isSelected()) {
                        ++high;
                        if (p1->secs+ride->recIntSecs() > current->start
                            && p1->secs< current->stop) {
                            if (mergeIntervals())
                                dataSetInterval[0].insert(std::make_pair<double, double>(aepf, cpv));
                            else
                                dataSetInterval[high].insert(std::make_pair<double, double>(aepf, cpv));
                        }
                    }
                }
                tot_cad += p1->cad;
                tot_cad_points++;
            }
        }

        if (tot_cad_points > 0) {

           // Now that we have the set of points, transform them into the
           // QwtArrays needed to set the curve's data.
           QVector<QwtArray<double> > aepfArrayInterval(num_intervals);
           QVector<QwtArray<double> > cpvArrayInterval(num_intervals);

           for (int i=0;i<num_intervals;i++) {
               std::set<std::pair<double, double> >::const_iterator l(dataSetInterval[i].begin());
               while (l != dataSetInterval[i].end()) {
                   const std::pair<double, double>& dataPoint = *l;

                   aepfArrayInterval[i].push_back(dataPoint.first);
                   cpvArrayInterval[i].push_back(dataPoint.second);

                   ++l;
               }
           }

           QwtSymbol sym;
           sym.setStyle(QwtSymbol::Ellipse);
           sym.setSize(6);
           sym.setBrush(QBrush(Qt::NoBrush));

           // ensure same colors are used for each interval selected
           int num_intervals_defined=0;
           QVector<int> intervalmap;
           if (mainWindow->allIntervalItems() != NULL) {
                num_intervals_defined = mainWindow->allIntervalItems()->childCount();
                for (int g=0; g<mainWindow->allIntervalItems()->childCount(); g++) {
                    IntervalItem *curr = dynamic_cast<IntervalItem *>(mainWindow->allIntervalItems()->child(g));
                    if (curr->isSelected()) intervalmap.append(g);
                }
           }

            // honor display sequencing
            QMap<int, int> intervalOrder;
            int count=0;

            if (mergeIntervals()) intervalOrder.insert(1,0);
            else {
                for (int i=0; i<mainWindow->allIntervalItems()->childCount(); i++) {
                    IntervalItem *current = dynamic_cast<IntervalItem *>(mainWindow->allIntervalItems()->child(i));
                    if (current != NULL && current->isSelected() == true) {
                            intervalOrder.insert(current->displaySequence, count++);
                    }
                }
            }

            QMapIterator<int, int> order(intervalOrder);
            while (order.hasNext()) {
                order.next();
                int z = order.value();

                QwtPlotCurve *curve;
                curve = new QwtPlotCurve();

                QColor intervalColor;
                if (mergeIntervals())
                    intervalColor = Qt::red;
                else
                    intervalColor.setHsv((intervalmap.count() > 0 ? intervalmap.at(z) : 1) * 255/num_intervals_defined, 255,255);

                QPen pen;
                pen.setColor(intervalColor);
                sym.setPen(pen);

                curve->setSymbol(sym);
                curve->setStyle(QwtPlotCurve::Dots);
                curve->setRenderHint(QwtPlotItem::RenderAntialiased);
                curve->setData(cpvArrayInterval[z],aepfArrayInterval[z]);
                curve->attach(this);

                intervalCurves.append(curve);
            }
       }
    }
    replot();
}

void
PfPvPlot::recalc()
{
    // initialize x values used for contours
    if (contour_xvalues.isEmpty()) {
	for (double x = 0; x <= 3.0; x += x / 20 + 0.02)
	    contour_xvalues.append(x);
	contour_xvalues.append(3.0);
    }

    double cpv = (cad_ * cl_ * 2.0 * PI) / 60.0;
    mX->setXValue(cpv);
    
    double aepf = (cp_ * 60.0) / (cad_ * cl_ * 2.0 * PI);
    mY->setYValue(aepf);
    
    QwtArray<double> yvalues(contour_xvalues.size());
    if (cp_) {
	for (int i = 0; i < contour_xvalues.size(); i ++)
	    yvalues[i] =
		(cpv < cp_ / 1e6) ?
		1e6 :
		cp_ / contour_xvalues[i];

	// generate curve at a given power
	cpCurve->setData(contour_xvalues, yvalues);
    }
    else
	// an empty curve if no power (or zero power) is specified
	cpCurve->setData(QwtArray <double>(), QwtArray <double>());
    
    //replot();
}

int
PfPvPlot::getCP()
{
    return cp_;
}

void
PfPvPlot::setCP(int cp)
{
    cp_ = cp;
    recalc();
    emit changedCP( QString("%1").arg(cp) );
}

int
PfPvPlot::getCAD()
{
    return cad_;
}

void
PfPvPlot::setCAD(int cadence)
{
    cad_ = cadence;
    recalc();
    emit changedCAD( QString("%1").arg(cadence) );
}

double
PfPvPlot::getCL()
{
    return cl_;
}

void
PfPvPlot::setCL(double cranklen)
{
    cl_ = cranklen;
    recalc();
    emit changedCL( QString("%1").arg(cranklen) );
}
// process checkbox for zone shading
void
PfPvPlot::setShadeZones(bool value)
{
    shade_zones = value;

    // if there are defined zones and labels, set their visibility
    for (int i = 0; i < zoneCurves.size(); i ++)
	zoneCurves[i]->setVisible(shade_zones);
    for (int i = 0; i < zoneLabels.size(); i ++)
	zoneLabels[i]->setVisible(shade_zones);

    //replot();
}

void
PfPvPlot::setMergeIntervals(bool value)
{
    merge_intervals = value;
    showIntervals(rideItem);
}

void
PfPvPlot::setFrameIntervals(bool value)
{
    frame_intervals = value;
    showIntervals(rideItem);
}
