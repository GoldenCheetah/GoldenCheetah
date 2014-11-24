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
#include "Context.h"
#include "Athlete.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "RideItem.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"

#include <math.h>
#include <qwt_series_data.h>
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

class IntervalAerolabData : public QwtSeriesData<QPointF>
{
    public:
        Aerolab *aerolab;
        Context *context;
        IntervalAerolabData
        (
            Aerolab    *aerolab,
            Context *context
        ) : aerolab( aerolab ), context(context) { }

        double x( size_t ) const;
        double y( size_t ) const;
        size_t size() const;
        //virtual QwtData *copy() const;

        void init();

        IntervalItem *intervalNum( int ) const;

        int intervalCount() const;

        virtual QPointF sample(size_t i) const;
        virtual QRectF boundingRect() const;
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
    const QTreeWidgetItem *allIntervals = context->athlete->allIntervalItems();
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

    if ( context->athlete->allIntervalItems() != NULL )
    {
        const QTreeWidgetItem *allIntervals = context->athlete->allIntervalItems();
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
// been selected in the Context leftlayout for each selected
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
        double multiplier = context->athlete->useMetricUnits ? 1 : MILES_PER_KM;
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

QPointF IntervalAerolabData::sample(size_t i) const {
    return QPointF(x(i), y(i));
}

QRectF IntervalAerolabData::boundingRect() const
{
    return QRectF(-5000, 5000, 10000, 10000);
}

//**********************************************
//**        END IntervalAerolabData           **
//**********************************************


Aerolab::Aerolab(
    AerolabWindow *parent,
    Context    *context
):
  QwtPlot(parent),
  context(context),
  parent(parent),
  rideItem(NULL),
  smooth(1), bydist(true), autoEoffset(true) {

  crr       = 0.005;
  cda       = 0.500;
  totalMass = 85.0;
  rho       = 1.236;
  eta       = 1.0;
  eoffset   = 0.0;
  constantAlt = false;

  insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
  setCanvasBackground(Qt::white);
  static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

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
  intervalHighlighterCurve->setSamples( new IntervalAerolabData( this, context ) );
  intervalHighlighterCurve->attach( this );
  //XXX broken this->legend()->remove( intervalHighlighterCurve ); // don't show in legend

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
  vePen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
  veCurve->setPen(vePen);
  QPen altPen = QPen(GColor(CAEROEL));
  altPen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
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

  //XXX broken this->legend()->remove( intervalHighlighterCurve ); // don't show in legend
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

  if( ride ) {

    const RideFileDataPresent *dataPresent = ride->areDataPresent();
    //setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

    if( dataPresent->watts ) {

      // If watts are present, then we can fill the veArray data:
      const RideFileDataPresent *dataPresent = ride->areDataPresent();
      int npoints = ride->dataPoints().size();
      double dt = ride->recIntSecs();
      veArray.resize(dataPresent->watts ? npoints : 0);
      altArray.resize(dataPresent->alt || constantAlt ? npoints : 0);
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
        altCurve->setVisible(dataPresent->alt || constantAlt );
      }

      // Fill the virtual elevation profile with data from the ride data:
      double t = 0.0;
      double vlast = 0.0;
      double e     = 0.0;
      arrayLength = 0;
      foreach(const RideFilePoint *p1, ride->dataPoints()) {
      if ( arrayLength == 0 )
        e = eoffset;

      timeArray[arrayLength]  = p1->secs / 60.0;
      if ( have_recorded_alt_curve ) {
          if ( constantAlt && arrayLength > 0) {
              altArray[arrayLength] = altArray[arrayLength-1];
          }
          else  {
              if ( constantAlt && !dataPresent->alt)
                  altArray[arrayLength] = 0;
              else
                altArray[arrayLength] = (context->athlete->useMetricUnits
                   ? p1->alt
                   : p1->alt * FEET_PER_METER);
          }
      }

      // Unpack:
      double power = max(0, p1->watts);
      double v     = p1->kph/vfactor;
      double headwind = v;
      if( dataPresent->headwind ) {
        headwind   = p1->headwind/vfactor;
      }
      double f     = 0.0;
      double a     = 0.0;

      // Use km data instead of formula for file with a stop (gap).
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
      double de  = s * v * dt * (context->athlete->useMetricUnits ? 1 : FEET_PER_METER);

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
    //setTitle("no data");

  }
}

void
Aerolab::setAxisTitle(int axis, QString label)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(label);
    title.setFont(stGiles);
    QwtPlot::setAxisFont(axis, stGiles);
    QwtPlot::setAxisTitle(axis, title);
}

void
Aerolab::adjustEoffset() {

    if (autoEoffset && !altArray.empty()) {
        double idx = axisScaleDiv( QwtPlot::xBottom ).lowerBound();
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
    QVector<double> data;

    if (!veArray.empty()){
      veCurve->setSamples(data, data);
    }
    if( !altArray.empty()) {
      altCurve->setSamples(data, data);
    }
    return;
  }

  QVector<double> &xaxis = (bydist?distanceArray:timeArray);
  int startingIndex = 0;
  int totalPoints   = arrayLength - startingIndex;

  // set curves
  if (!veArray.empty()) {
      veCurve->setSamples(xaxis.data() + startingIndex, veArray.data() + startingIndex, totalPoints);
  }

  if (!altArray.empty()){
      altCurve->setSamples(xaxis.data() + startingIndex, altArray.data() + startingIndex, totalPoints);
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

             if ( context->athlete->useMetricUnits )

             {

                 setAxisTitle( yLeft, tr("Elevation (m)") );

             }

             else

             {

                 setAxisTitle( yLeft, tr("Elevation (')") );

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
    setAxisTitle(xBottom, tr("Distance ")+QString(context->athlete->useMetricUnits?"(km)":"(miles)"));
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
Aerolab::setConstantAlt(int value)
{
    constantAlt = value;
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
        double x_value = curve->sample( index ).x();
        double y_value = curve->sample( index ).y();
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
                mrk->setValue((context->athlete->useMetricUnits ? 1 : MILES_PER_KM) *
                                rideItem->ride()->timeToDistance(interval.start), 0.0);
            mrk->setLabel(text);
        }
    }
}

/*
 * Estimate CdA and Crr usign energy balance in segments defined by
 * non-zero altitude.
 * Returns an explanatory error message ff it fails to do the estimation,
 * otherwise it updates cda and crr and returns an empty error message.
 * Author: Alejandro Martinez
 * Date: 23-aug-2012
 */
QString Aerolab::estimateCdACrr(RideItem *rideItem)
{
    // HARD-CODED DATA: p1->kph
    const double vfactor = 3.600;
    const double g = 9.80665;
    RideFile *ride = rideItem->ride();
    QString errMsg;

    if(ride) {
        const RideFileDataPresent *dataPresent = ride->areDataPresent();
        if(( dataPresent->alt || constantAlt )  && dataPresent->watts) {
            double dt = ride->recIntSecs();
            int npoints = ride->dataPoints().size();
            double X1[npoints], X2[npoints], Egain[npoints];
            int nSeg = -1;
            double altInit = 0, vInit = 0;
            /* For each segment, defined between points with alt != 0,
             * this loop computes X1, X2 and Egain to verify:
             * Aero-Loss + RR-Loss = Egain
             * where
             *      Aero-Loss = X1[nSgeg] * CdA
             *      RR-Loss = X2[nSgeg] * Crr
             * are the aero and rr components of the energy loss with
             *      X1[nSeg] = sum(0.5 * rho * headwind*headwind * distance)
             *      X2[nSeg] = sum(totalMass * g * distance)
             * and the energy gain sums power in the segment with
             * potential and kinetic variations:
             *      Egain = sum(eta * power * dt) +
             *              totalMass * (g * (altInit - alt) +
             *              0.5 * (vInit*vInit - v*v))
             */
            foreach(const RideFilePoint *p1, ride->dataPoints()) {
                // Unpack:
                double power = max(0, p1->watts);
                double v     = p1->kph/vfactor;
                double distance = v * dt;
                double headwind = v;
                if( dataPresent->headwind ) {
                    headwind   = p1->headwind/vfactor;
                }
                double alt = p1->alt;
                // start initial segment
                if (nSeg < 0 && alt != 0) {
                    nSeg = 0;
                    X1[nSeg] = X2[nSeg] = Egain[nSeg] = 0.0;
                    altInit = alt;
                    vInit = v;
                }
                // accumulate segment data
                if (nSeg >= 0) {
                    // X1[nSgeg] * CdA == Aero-Loss
                    X1[nSeg] += 0.5 * rho * headwind*headwind * distance;
                    // X2[nSgeg] * Crr == RR-Loss
                    X2[nSeg] += totalMass * g * distance;
                    // Energy supplied
                    Egain[nSeg] += eta * power * dt;
                }
                // close current segment and start a new one
                if (nSeg >= 0 && alt != 0) {
                    // Add change in potential and kinetic energy
                    Egain[nSeg] += totalMass * (g * (altInit - alt) + 0.5 * (vInit*vInit - v*v));
                    // Start a new segment
                    nSeg++;
                    X1[nSeg] = X2[nSeg] = Egain[nSeg] = 0.0;
                    altInit = alt;
                    vInit = v;
                }
            }
            /* At least two segmentes needed to approximate:
             *     X1 * CdA + X2 * Crr = Egain
             * which, in matrix form, is:
             *    X * [ CdA ; Crr ] = Egain
             * and pre-multiplying by X transpose (X'):
             *    X'* X [ CdA ; Crr ] = X' * Egain
             * which is a system with two equations and two unknowns
             *    A * [ CdA ; Crr ] = B
             */
            if (nSeg >= 2) {
                // compute the normal equation: A * [ CdA ; Crr ] = B
                // A = X'*X
                // B = X'*Egain
                double A11 = 0, A12 = 0, A21 = 0, A22 = 0, B1 = 0, B2 = 0;
                for (int i = 0; i < nSeg; i++) {
                    A11 += X1[i] * X1[i];
                    A12 += X1[i] * X2[i];
                    A21 += X2[i] * X1[i];
                    A22 += X2[i] * X2[i];
                    B1  += X1[i] * Egain[i];
                    B2  += X2[i] * Egain[i];
                }
                // Solve the normal equation
                // A11 * CdA + A12 * Crr = B1
                // A21 * CdA + A22 * Crr = B2
                double det = A11 * A22 - A12 * A21;
                if (fabs(det) > 0.00) {
                    // round and update if the values are in Aerolab's range
                    double cda = floor(10000 * (A22 * B1 - A12 * B2) / det + 0.5) / 10000;
                    double crr = floor(1000000 * (A11 * B2 - A21 * B1) / det + 0.5) / 1000000;
                    if (cda >= 0.001 and cda <= 1.0 and crr >= 0.0001 and crr <= 0.1) {
                        this->cda = cda;
                        this->crr = crr;
                        errMsg = ""; // No error
                    } else {
                        errMsg = tr("Estimates out-of-range");
                    }
                } else {
                    errMsg = tr("At least two segments must be independent");
                }
            } else {
                errMsg = tr("At least two segments must be defined");
            }
        } else {
            errMsg = tr("Altitude and Power data must be present");
        }
    } else {
        errMsg = tr("No ride selected");
    }
    return errMsg;
}
