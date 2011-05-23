/*
 * Copyright (c) 2009 Andy M. Froncioni (me@andyfroncioni.com)
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

#include "Aerolab.h"
#include "MainWindow.h"
#include "RideFile.h"
#include "RideItem.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"

#include <math.h>
#include <assert.h>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <set>
#include <QDebug>

#define PI M_PI

static inline double
max(double a, double b) { if (a > b) return a; else return b; }
static inline double
min(double a, double b) { if (a < b) return a; else return b; }

Aerolab::Aerolab(QWidget *parent):
  QwtPlot(parent),
  unit(0),
  rideItem(NULL),
  smooth(1), bydist(false) {

  crr       = 0.005;
  cda       = 0.500;
  totalMass = 85.0;
  rho       = 1.236;
  eta       = 1.0;
  eoffset   = 0.0;

  boost::shared_ptr<QSettings> settings = GetApplicationSettings();
  unit = settings->value(GC_UNIT);
  useMetricUnits = true;

  insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
  setCanvasBackground(Qt::white);

  setXTitle();
  setAxisTitle(yLeft, "Elevation (m)");
  setAxisScale(yLeft, -300, 300);
  setAxisTitle(xBottom, "Distance (km)");
  setAxisScale(xBottom, 0, 60);

  veCurve = new QwtPlotCurve(tr("V-Elevation"));

  altCurve = new QwtPlotCurve(tr("Elevation"));

  grid = new QwtPlotGrid();
  grid->enableX(false);
  grid->attach(this);

  configChanged();
}

void
Aerolab::configChanged()
{
  // set colors
  setCanvasBackground(GColor(CPLOTBACKGROUND));
  QPen vePen = QPen(GColor(CAEROVE));
  vePen.setWidth(1);
  veCurve->setPen(vePen);
  QPen altPen = QPen(GColor(CAEROEL));
  altPen.setWidth(1);
  altCurve->setPen(altPen);
  QPen gridPen(GColor(CPLOTGRID));
  gridPen.setStyle(Qt::DotLine);
  grid->setPen(gridPen);
}

void
Aerolab::setData(RideItem *_rideItem, bool new_zoom) {

  // HARD-CODED DATA: p1->kph
  double vfactor = 3.600;
  double m       =  totalMass;
  double small_number = 0.00001;

  rideItem = _rideItem;
  RideFile *ride = rideItem->ride();

  veArray.clear();
  altArray.clear();
  distanceArray.clear();
  timeArray.clear();
  useMetricUnits = true;

  if( ride ) {

    const RideFileDataPresent *dataPresent = ride->areDataPresent();
    setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

    if( dataPresent->watts ) {

      // If watts are present, then we can fill the veArray data:
      const RideFileDataPresent *dataPresent = ride->areDataPresent();
      int npoints = ride->dataPoints().size();
      double dt = ride->recIntSecs();
      veArray.resize(dataPresent->watts ? npoints : 0);
      altArray.resize(dataPresent->alt ? npoints : 0);
      timeArray.resize(dataPresent->watts ? npoints : 0);
      distanceArray.resize(dataPresent->watts ? npoints : 0);

      // quickly erase old data
      veCurve->setVisible(false);
      altCurve->setVisible(false);

      // detach and re-attach the ve curve:
      veCurve->detach();
      if (!veArray.empty()) {
        veCurve->attach(this);
        veCurve->setVisible(dataPresent->watts);
      }

      // detach and re-attach the ve curve:
      bool have_recorded_alt_curve = false;
      altCurve->detach();
      if (!altArray.empty()) {
        have_recorded_alt_curve = true;
        altCurve->attach(this);
        altCurve->setVisible(dataPresent->alt);
      }

      // Fill the virtual elevation profile with data from the ride data:
      double t = 0.0;
      double vlast = 0.0;
      double e     = 0.0;
      double d = 0;
      arrayLength = 0;
      foreach(const RideFilePoint *p1, ride->dataPoints()) {
      if ( arrayLength == 0 )
        e = eoffset;

      timeArray[arrayLength]  = p1->secs;
      if ( have_recorded_alt_curve )
        altArray[arrayLength] = (useMetricUnits
                   ? p1->alt
                   : p1->alt * FEET_PER_METER);

      // Unpack:
      double power = max(0, p1->watts);
      double v     = p1->kph/vfactor;
      double headwind = v;
      if( dataPresent->headwind ) {
        headwind   = p1->headwind/vfactor;
      }
      double f     = 0.0;
      double a     = 0.0;
      d += v * dt;
      distanceArray[arrayLength] = d/1000;


      if( v > small_number ) {
        f  = power/v;
        a  = ( v*v - vlast*vlast ) / ( 2.0 * dt * v );
      } else {
        a = ( v - vlast ) / dt;
      }

      f *= eta; // adjust for drivetrain efficiency if using a crank-based meter
      double s   = slope( f, a, m, crr, cda, rho, headwind );
      double de  = s * v * dt;

      e += de;
      t += dt;
      veArray[arrayLength] = e;

      vlast = v;

      ++arrayLength;
    }

  } else {
      veCurve->setVisible(false);
      altCurve->setVisible(false);
  }

    recalc(new_zoom);
  } else {
    setTitle("no data");

  }
}

struct DataPoint {
  double time, hr, watts, speed, cad, alt;
  DataPoint(double t, double h, double w, double s, double c, double a) :
    time(t), hr(h), watts(w), speed(s), cad(c), alt(a) {}
};

void
Aerolab::recalc( bool new_zoom ) {

  if (timeArray.empty())
    return;
  int rideTimeSecs = (int) ceil(timeArray[arrayLength - 1]);
  int totalRideDistance = (int ) ceil(distanceArray[arrayLength - 1]);

  // If the ride is really long, then avoid it like the plague.
  if (rideTimeSecs > 7*24*60*60) {
    QwtArray<double> data;
    if (!veArray.empty()){
      veCurve->setData(data, data);
    }
    if( !altArray.empty()) {
      altCurve->setData(data, data);
    }
    return;
  }


  QVector<double> &xaxis = distanceArray;
  int startingIndex = 0;
  int totalPoints   = arrayLength - startingIndex;

  // set curves
  if (!veArray.empty())
    veCurve->setData(xaxis.data() + startingIndex,
             veArray.data() + startingIndex, totalPoints);

  if (!altArray.empty()){
    altCurve->setData(xaxis.data() + startingIndex,
              altArray.data() + startingIndex, totalPoints);
  }

  if( new_zoom )
    setAxisScale(xBottom, 0.0, totalRideDistance);

  setYMax();

  replot();
}



void
Aerolab::setYMax() {

  if (veCurve->isVisible()) {
    setAxisTitle(yLeft, "Elevation");
    if ( !altArray.empty() ) {
      setAxisScale(yLeft,
             min( veCurve->minYValue(), altCurve->minYValue() ) - 10,
             10.0 + max( veCurve->maxYValue(), altCurve->maxYValue() ) );
    } else {
      setAxisScale(yLeft,
             veCurve->minYValue() ,
             1.05 * veCurve->maxYValue() );
    }
    setAxisLabelRotation(yLeft,270);
    setAxisLabelAlignment(yLeft,Qt::AlignVCenter);
  }

  enableAxis(yLeft, veCurve->isVisible());
}



void
Aerolab::setXTitle() {

  if (bydist)
    setAxisTitle(xBottom, tr("Distance ")+QString(unit.toString() == "Metric"?"(km)":"(miles)"));
  else
    setAxisTitle(xBottom, tr("Time (minutes)"));
}


void
Aerolab::setByDistance() {
  bydist = true;
  setXTitle();
  recalc(false);
}


double
Aerolab::slope(
           double f,
           double a,
           double m,
           double crr,
           double cda,
           double rho,
           double v
        )  {

  double g = 9.80665;

  // Small angle version of slope calculation:
  double s = f/(m*g) - crr - cda*rho*v*v/(2.0*m*g) - a/g;

  return s;
}

// At slider 1000, we want to get max Crr=0.1000
// At slider 1    , we want to get min Crr=0.0001
void
Aerolab::setIntCrr(
           int value
            ) {

  crr = (double) value / 1000000.0;

  recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
Aerolab::setIntCda(
           int value
            )  {
  cda = (double) value / 10000.0;
  recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
Aerolab::setIntTotalMass(
             int value
              ) {

  totalMass = (double) value / 100.0;
  recalc(false);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
Aerolab::setIntRho(
           int value
            ) {

  rho = (double) value / 10000.0;
  recalc(false);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
Aerolab::setIntEta(
                     int value
                     ) {

  eta = (double) value / 10000.0;
  recalc(false);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
Aerolab::setIntEoffset(
                     int value
                     ) {

  eoffset = (double) value / 100.0;
  recalc(false);
}
