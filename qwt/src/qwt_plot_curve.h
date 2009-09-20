/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_CURVE_H
#define QWT_PLOT_CURVE_H

#include <qpen.h>
#include <qstring.h>
#include "qwt_global.h"
#include "qwt_plot_item.h"
#include "qwt_text.h"
#include "qwt_polygon.h"
#include "qwt_data.h"

class QPainter;
class QwtScaleMap;
class QwtSymbol;
class QwtCurveFitter;

/*!
  \brief A plot item, that represents a series of points

  A curve is the representation of a series of points in the x-y plane.
  It supports different display styles, interpolation ( f.e. spline ) 
  and symbols.

  \par Usage
  <dl><dt>a) Assign curve properties</dt>
  <dd>When a curve is created, it is configured to draw black solid lines
  with in Lines style and no symbols. You can change this by calling 
  setPen(), setStyle() and setSymbol().</dd>
  <dt>b) Connect/Assign data.</dt>
  <dd>QwtPlotCurve gets its points using a QwtData object offering
  a bridge to the real storage of the points ( like QAbstractItemModel ).
  There are several convenience classes derived from QwtData, that also store
  the points inside ( like QStandardItemModel ). QwtPlotCurve also offers
  a couple of variations of setData(), that build QwtData objects from
  arrays internally.</dd>
  <dt>c) Attach the curve to a plot</dt>
  <dd>See QwtPlotItem::attach()
  </dd></dl>

  \par Example:
  see examples/bode

  \sa QwtPlot, QwtData, QwtSymbol, QwtScaleMap
*/
class QWT_EXPORT QwtPlotCurve: public QwtPlotItem
{
public:
    /*!
       Curve type.

       - Yfx\n
         Draws y as a function of x (the default). The
         baseline is interpreted as a horizontal line
         with y = baseline().
       - Xfy\n
         Draws x as a function of y. The baseline is
         interpreted as a vertical line with x = baseline().

       The baseline is used for aligning the sticks, or
       filling the curve with a brush.

       \sa setCurveType(), curveType(), baseline() brush()
     */
    enum CurveType
    {
        Yfx,
        Xfy
    };

    /*! 
        Curve styles. 

         - NoCurve\n
           Don't draw a curve. Note: This doesn't affect the symbols.
         - Lines\n
           Connect the points with straight lines. The lines might
           be interpolated depending on the 'Fitted' attribute. Curve
           fitting can be configured using setCurveFitter().
         - Sticks\n
           Draw vertical(Yfx) or horizontal(Xfy) sticks from a baseline 
           which is defined by setBaseline().
         - Steps\n
           Connect the points with a step function. The step function
           is drawn from the left to the right or vice versa,
           depending on the 'Inverted' attribute.
         - Dots\n
           Draw dots at the locations of the data points. Note:
           This is different from a dotted line (see setPen()), and faster
           as a curve in NoStyle style and a symbol painting a point.
         - UserCurve\n
           Styles >= UserCurve are reserved for derived
           classes of QwtPlotCurve that overload drawCurve() with
           additional application specific curve types.

        \sa setStyle(), style()
    */
    enum CurveStyle
    {
        NoCurve,

        Lines,
        Sticks,
        Steps,
        Dots,

        UserCurve = 100
    };

    /*! 
      Attribute for drawing the curve

      - Fitted ( in combination with the Lines QwtPlotCurve::CurveStyle only )\n
        A QwtCurveFitter tries to
        interpolate/smooth the curve, before it is painted.
        Note that curve fitting requires temorary memory
        for calculating coefficients and additional points.
        If painting in Fitted mode is slow it might be better
        to fit the points, before they are passed to QwtPlotCurve.
      - Inverted\n
        For Steps only. Draws a step function
        from the right to the left.

        \sa setCurveAttribute(), testCurveAttribute(), curveFitter()
    */
    enum CurveAttribute
    {
        Inverted = 1,
        Fitted = 2
    };

    /*! 
        Attributes to modify the drawing algorithm.

        - PaintFiltered\n
          Tries to reduce the data that has to be painted, by sorting out
          duplicates, or paintings outside the visible area. Might have a
          notable impact on curves with many close points.
          Only a couple of very basic filtering algos are implemented.
        - ClipPolygons\n
          Clip polygons before painting them. In situations, where points
          are far outside the visible area (f.e when zooming deep) this 
          might be a substantial improvement for the painting performance 
          ( especially on Windows ).

        The default is, that no paint attributes are enabled.

        \sa setPaintAttribute(), testPaintAttribute()
    */
    enum PaintAttribute
    {
        PaintFiltered = 1,
        ClipPolygons = 2
    };

    explicit QwtPlotCurve();
    explicit QwtPlotCurve(const QwtText &title);
    explicit QwtPlotCurve(const QString &title);

    virtual ~QwtPlotCurve();

    virtual int rtti() const;

    void setCurveType(CurveType);
    CurveType curveType() const;

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setRawData(const double *x, const double *y, int size);
    void setData(const double *xData, const double *yData, int size);
    void setData(const QwtArray<double> &xData, const QwtArray<double> &yData);
#if QT_VERSION < 0x040000
    void setData(const QwtArray<QwtDoublePoint> &data);
#else
    void setData(const QPolygonF &data);
#endif
    void setData(const QwtData &data);
    
    int closestPoint(const QPoint &pos, double *dist = NULL) const;

    QwtData &data();
    const QwtData &data() const;

    int dataSize() const;
    double x(int i) const;
    double y(int i) const;

    virtual QwtDoubleRect boundingRect() const;

    double minXValue() const;
    double maxXValue() const;
    double minYValue() const;
    double maxYValue() const;

    void setCurveAttribute(CurveAttribute, bool on = true);
    bool testCurveAttribute(CurveAttribute) const;

    void setPen(const QPen &);
    const QPen &pen() const;

    void setBrush(const QBrush &);
    const QBrush &brush() const;

    void setBaseline(double ref);
    double baseline() const;

    void setStyle(CurveStyle style);
    CurveStyle style() const;

    void setSymbol(const QwtSymbol &s);
    const QwtSymbol& symbol() const;

    void setCurveFitter(QwtCurveFitter *);
    QwtCurveFitter *curveFitter() const;

    virtual void draw(QPainter *p, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &) const;

    virtual void draw(QPainter *p, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;

    void draw(int from, int to) const;

    virtual void updateLegend(QwtLegend *) const;

protected:

    void init();

    virtual void drawCurve(QPainter *p, int style,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;

    virtual void drawSymbols(QPainter *p, const QwtSymbol &,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;

    void drawLines(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;
    void drawSticks(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;
    void drawDots(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;
    void drawSteps(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;

    void fillCurve(QPainter *,
        const QwtScaleMap &, const QwtScaleMap &,
        QwtPolygon &) const;
    void closePolyline(const QwtScaleMap &, const QwtScaleMap &,
        QwtPolygon &) const;

private:
    QwtData *d_xy;

    class PrivateData;
    PrivateData *d_data;
};

//! \return the the curve data
inline QwtData &QwtPlotCurve::data()
{
    return *d_xy;
}

//! \return the the curve data
inline const QwtData &QwtPlotCurve::data() const
{
    return *d_xy;
}

/*!
    \param i index
    \return x-value at position i
*/
inline double QwtPlotCurve::x(int i) const 
{ 
    return d_xy->x(i); 
}

/*!
    \param i index
    \return y-value at position i
*/
inline double QwtPlotCurve::y(int i) const 
{ 
    return d_xy->y(i); 
}

//! boundingRect().left()
inline double QwtPlotCurve::minXValue() const 
{ 
    return boundingRect().left(); 
}

//! boundingRect().right()
inline double QwtPlotCurve::maxXValue() const 
{ 
    return boundingRect().right(); 
}

//! boundingRect().top()
inline double QwtPlotCurve::minYValue() const 
{ 
    return boundingRect().top(); 
}

//! boundingRect().bottom()
inline double QwtPlotCurve::maxYValue() const 
{ 
    return boundingRect().bottom(); 
}

#endif
