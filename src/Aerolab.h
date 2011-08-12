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

#ifndef _GC_Aerolab_h
#define _GC_Aerolab_h 1

#include <qwt_plot.h>
#include <qwt_data.h>
#include <QtGui>
#include "LTMWindow.h" // for tooltip/canvaspicker

// forward references
class RideItem;
class RideFilePoint;
class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class AerolabWindow;
class MainWindow;
class IntervalAerolabData;
class LTMToolTip;
class LTMCanvasPicker;


class Aerolab : public QwtPlot {

  Q_OBJECT

  public:
  Aerolab( AerolabWindow *, MainWindow * );
  bool byDistance() const { return bydist; }
  bool useMetricUnits;  // whether metric units are used (or imperial)
  void setData(RideItem *_rideItem, bool new_zoom);

  void refreshIntervalMarkers();

  private:
  AerolabWindow *parent;

  LTMToolTip      *tooltip;
  LTMCanvasPicker *_canvasPicker; // allow point selection/hover

  void adjustEoffset();

  public slots:

  void setAutoEoffset(int value);
  void setByDistance(int value);
  void configChanged();

  void pointHover( QwtPlotCurve *, int );

  signals:

  protected:
  friend class ::AerolabWindow;
  friend class ::IntervalAerolabData;


  QVariant unit;
  QwtPlotGrid *grid;
  QVector<QwtPlotMarker*> d_mrk;

  // One curve to plot in the Course Profile:
  QwtPlotCurve *veCurve;   // virtual elevation curve
  QwtPlotCurve *altCurve;    // recorded elevation curve, if available

  QwtPlotCurve *intervalHighlighterCurve;  // highlight selected intervals on the Plot

  RideItem *rideItem;

  QVector<double> hrArray;
  QVector<double> wattsArray;
  QVector<double> speedArray;
  QVector<double> cadArray;

  // We store virtual elevation, time, altitude,and distance:
  QVector<double> veArray;
  QVector<double> altArray;
  QVector<double> timeArray;
  QVector<double> distanceArray;

  int smooth;
  bool bydist;
  bool autoEoffset;
  int arrayLength;
  int iCrr;
  int iCda;
  double crr;
  double cda;
  double totalMass; // Bike + Rider mass
  double rho;
  double eta;
  double eoffset;


  double   slope(double, double, double, double, double, double, double);
  void     recalc(bool);
  void     setYMax(bool);
  void     setXTitle();
  void     setIntCrr(int);
  void     setIntCda(int);
  void     setIntRho(int);
  void     setIntEta(int);
  void     setIntEoffset(int);
  void     setIntTotalMass(int);
  double   getCrr() const { return (double)crr; }
  double   getCda() const { return (double)cda; }
  double   getTotalMass() const { return (double)totalMass; }
  double   getRho() const { return (double)rho; }
  double   getEta() const { return (double)eta; }
  double   getEoffset() const { return (double)eoffset; }
  int      intCrr() const { return (int)( crr * 1000000  ); }
  int      intCda() const { return (int)( cda * 10000); }
  int      intTotalMass() const { return (int)( totalMass * 100); }
  int      intRho() const { return (int)( rho * 10000); }
  int      intEta() const { return (int)( eta * 10000); }
  int      intEoffset() const { return (int)( eoffset * 100); }


};

#endif // _GC_Aerolab_h

