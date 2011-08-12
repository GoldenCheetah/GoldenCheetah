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
#include "AerolabWindow.h"
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


/*----------------------------------------------------------------------
 * Interval plotting
 *--------------------------------------------------------------------*/

class IntervalAerolabData : public QwtData
{
    public:
        Aerolab *aerolab;
        MainWindow *mainWindow;
        IntervalAerolabData
        (
            Aerolab    *aerolab,
            MainWindow *mainWindow
        ) : aerolab( aerolab ), mainWindow( mainWindow ) { }

        double x( size_t ) const;
        double y( size_t ) const;
        size_t size() const;
        virtual QwtData *copy() const;

        void init();

        IntervalItem *intervalNum( int ) const;

        int intervalCount() const;
};

/*
 * HELPER FUNCTIONS:
 *    intervalNum - returns a pointer to the nth selected interval
 *    intervalCount - returns the number of highlighted intervals
 */
// ------------------------------------------------------------------------------------------------------------
// note this is operating on the children of allIntervals and not the
// intervalWidget (QTreeWidget) -- this is why we do not use the
// selectedItems() member. N starts a one not zero.
// ------------------------------------------------------------------------------------------------------------
IntervalItem *IntervalAerolabData::intervalNum
(
    int number
) const
{
    int                    highlighted  = 0;
    const QTreeWidgetItem *allIntervals = mainWindow->allIntervalItems();
    for ( int ii = 0; ii < allIntervals->childCount(); ii++)

    {
        IntervalItem *current = (IntervalItem *) allIntervals->child( ii );

        if ( current == NULL)
        {
            return NULL;
        }
        if ( current->isSelected() == true )
        {
            ++highlighted;
        }
        if ( highlighted == number )
        {
            return current;
        }
    }

    return NULL;
}

// ------------------------------------------------------------------------------------------------------------
// how many intervals selected?
// ------------------------------------------------------------------------------------------------------------
int IntervalAerolabData::intervalCount() const
{
    int highlighted = 0;

    if ( mainWindow->allIntervalItems() != NULL )
    {
        const QTreeWidgetItem *allIntervals = mainWindow->allIntervalItems();
        for ( int ii = 0; ii < allIntervals->childCount(); ii++)
        {
            IntervalItem *current = (IntervalItem *) allIntervals->child( ii );
            if ( current != NULL )
            {
                if ( current->isSelected() == true )
                {
                    ++highlighted;
                }
            }
        }
    }
    return highlighted;
}
/*
 * INTERVAL HIGHLIGHTING CURVE
 * IntervalAerolabData - implements the qwtdata interface where
 *                    x,y return point co-ordinates and
 *                    size returns the number of points
 */
// The interval curve data is derived from the intervals that have
// been selected in the MainWindow leftlayout for each selected
// interval we return 4 data points; bottomleft, topleft, topright
// and bottom right.
//
// the points correspond to:
// bottom left = interval start, 0 watts
// top left = interval start, maxwatts
// top right = interval stop, maxwatts
// bottom right = interval stop, 0 watts
//
double IntervalAerolabData::x
(
    size_t number
) const
{
    // for each interval there are four points, which interval is this for?
    // interval numbers start at 1 not ZERO in the utility functions

    double result = 0;

    int interval_no = number ? 1 + number / 4 : 1;
    // get the interval
    IntervalItem *current = intervalNum( interval_no );

    if ( current != NULL )
    {
        double multiplier = aerolab->useMetricUnits ? 1 : MILES_PER_KM;
        // which point are we returning?
//qDebug() << "number = " << number << endl;
        switch ( number % 4 )
        {
            case 0 : result = aerolab->bydist ? multiplier * current->startKM : current->start/60; // bottom left
                 break;
            case 1 : result = aerolab->bydist ? multiplier * current->startKM : current->start/60; // top left
                 break;
            case 2 : result = aerolab->bydist ? multiplier * current->stopKM  : current->stop/60;  // bottom right
                 break;
            case 3 : result = aerolab->bydist ? multiplier * current->stopKM  : current->stop/60;  // top right
                 break;
        }
    }
    return result;
}
double IntervalAerolabData::y
(
    size_t number
) const
{
    // which point are we returning?
    double result = 0;
    switch ( number % 4 )
    {
        case 0 : result = -5000;  // bottom left
                 break;
        case 1 : result = 5000;  // top left - set to out of bound value
                 break;
        case 2 : result = 5000;  // top right - set to out of bound value
                 break;
        case 3 : result = -5000;  // bottom right
                 break;
    }
    return result;
}

size_t IntervalAerolabData::size() const
{
    return intervalCount() * 4;
}

QwtData *IntervalAerolabData::copy() const
{
    return new IntervalAerolabData( aerolab, mainWindow );
}


//**********************************************
//**        END IntervalAerolabData           **
//**********************************************


Aerolab::Aerolab(
    AerolabWindow *parent,
    MainWindow    *mainWindow
):
  QwtPlot(parent),
  parent(parent),
  unit(0),
  rideItem(NULL),
  smooth(1), bydist(true), autoEoffset(true) {

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
  setAxisTitle(yLeft, tr("Elevation (m)"));
  setAxisScale(yLeft, -300, 300);
  setAxisTitle(xBottom, tr("Distance (km)"));
  setAxisScale(xBottom, 0, 60);

  veCurve = new QwtPlotCurve(tr("V-Elevation"));
  altCurve = new QwtPlotCurve(tr("Elevation"));

  // get rid of nasty blank space on right of the plot
  veCurve->setYAxis( yLeft );
  altCurve->setYAxis( yLeft );

  intervalHighlighterCurve = new QwtPlotCurve();
  intervalHighlighterCurve->setBaseline(-5000);
  intervalHighlighterCurve->setYAxis( yLeft );
  intervalHighlighterCurve->setData( IntervalAerolabData( this, mainWindow ) );
  intervalHighlighterCurve->attach( this );
  this->legend()->remove( intervalHighlighterCurve ); // don't show in legend

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

  QPen ihlPen = QPen( GColor( CINTERVALHIGHLIGHTER ) );
  ihlPen.setWidth(1);
  intervalHighlighterCurve->setPen( ihlPen );

  QColor ihlbrush = QColor(GColor(CINTERVALHIGHLIGHTER));
  ihlbrush.setAlpha(40);
  intervalHighlighterCurve->setBrush(ihlbrush);   // fill below the line

  this->legend()->remove( intervalHighlighterCurve ); // don't show in legend
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

      timeArray[arrayLength]  = p1->secs / 60.0;
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

      // Use km data insteed of formula for file with a stop (gap).
      //d += v * dt;
      //distanceArray[arrayLength] = d/1000;

      distanceArray[arrayLength] = p1->km;



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
    adjustEoffset();
  } else {
    setTitle("no data");

  }
}

void
Aerolab::adjustEoffset() {

    if (autoEoffset && !altArray.empty()) {
        double idx = axisScaleDiv( QwtPlot::xBottom )->lowerBound();
        parent->eoffsetSlider->setEnabled(false);

        if (bydist) {
            int v = 100*(altArray.at(rideItem->ride()->distanceIndex(idx))-veArray.at(rideItem->ride()->distanceIndex(idx)));
            parent->eoffsetSlider->setValue(intEoffset()+v);
        }
        else {
            int v = 100*(altArray.at(rideItem->ride()->timeIndex(60*idx))-veArray.at(rideItem->ride()->timeIndex(60*idx)));
            parent->eoffsetSlider->setValue(intEoffset()+v);
        }
    } else
        parent->eoffsetSlider->setEnabled(true);
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

  QVector<double> &xaxis = (bydist?distanceArray:timeArray);
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
      setAxisScale(xBottom, 0.0, (bydist?totalRideDistance:rideTimeSecs));



  setYMax(new_zoom );
  refreshIntervalMarkers();

  replot();
}



void
Aerolab::setYMax(bool new_zoom)
{
          if (veCurve->isVisible())
          {

             if ( useMetricUnits )

             {

                 setAxisTitle( yLeft, "Elevation (m)" );

             }

             else

             {

                 setAxisTitle( yLeft, "Elevation (')" );

             }

            double minY = 0.0;
            double maxY = 0.0;

            //************

  //if (veCurve->isVisible()) {
   // setAxisTitle(yLeft, tr("Elevation"));
    if ( !altArray.empty() ) {
   //   setAxisScale(yLeft,
   //          min( veCurve->minYValue(), altCurve->minYValue() ) - 10,
   //          10.0 + max( veCurve->maxYValue(), altCurve->maxYValue() ) );

        minY = min( veCurve->minYValue(), altCurve->minYValue() ) - 10;
        maxY = 10.0 + max( veCurve->maxYValue(), altCurve->maxYValue() );

    } else {
      //setAxisScale(yLeft,
      //       veCurve->minYValue() ,
      //       1.05 * veCurve->maxYValue() );

        if ( new_zoom )

              {

                minY = veCurve->minYValue();

                maxY = veCurve->maxYValue();

              }

              else

              {

                minY = parent->getCanvasTop();

                maxY = parent->getCanvasBottom();

              }

              //adjust eooffset
              // TODO



    }

    setAxisScale( yLeft, minY, maxY );
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
Aerolab::setAutoEoffset(int value)
{
    autoEoffset = value;
    adjustEoffset();
}

void
Aerolab::setByDistance(int value) {
  bydist = value;
  setXTitle();
  recalc(true);
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










void Aerolab::pointHover (QwtPlotCurve *curve, int index)
{
    if ( index >= 0 && curve != intervalHighlighterCurve )
    {
        double x_value = curve->x( index );
        double y_value = curve->y( index );
        // output the tooltip

        QString text = QString( "%1 %2 %3 %4 %5" )
              . arg( this->axisTitle( curve->xAxis() ).text() )
              . arg( x_value, 0, 'f', 3 )
              . arg( "\n" )
              . arg( this->axisTitle( curve->yAxis() ).text() )
              . arg( y_value, 0, 'f', 3 );

        // set that text up
        tooltip->setText( text );
    }
    else
    {
        // no point
        tooltip->setText( "" );
    }
}

void Aerolab::refreshIntervalMarkers()
{
    foreach( QwtPlotMarker *mrk, d_mrk )
    {
        mrk->detach();
        delete mrk;
    }
    d_mrk.clear();

    QRegExp wkoAuto("^(Peak *[0-9]*(s|min)|Entire workout|Find #[0-9]*) *\\([^)]*\\)$");
    if ( rideItem->ride() )
    {
        foreach(const RideFileInterval &interval, rideItem->ride()->intervals()) {
            // skip WKO autogenerated peak intervals
            if (wkoAuto.exactMatch(interval.name))
                continue;
            QwtPlotMarker *mrk = new QwtPlotMarker;
            d_mrk.append(mrk);
            mrk->attach(this);
            mrk->setLineStyle(QwtPlotMarker::VLine);
            mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            mrk->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));
            QwtText text(interval.name);
            text.setFont(QFont("Helvetica", 10, QFont::Bold));
            text.setColor(GColor(CPLOTMARKER));
            if (!bydist)
                mrk->setValue(interval.start / 60.0, 0.0);
            else
                mrk->setValue((useMetricUnits ? 1 : MILES_PER_KM) *
                                rideItem->ride()->timeToDistance(interval.start), 0.0);
            mrk->setLabel(text);
        }
    }
}
