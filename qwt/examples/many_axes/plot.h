#ifndef _PLOT_H_
#define _PLOT_H_

#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>
#include <qwt_data.h>
#include <qwt_text.h>

/*class FunctionData: public QwtSyntheticPointData
{
   public:
      FunctionData(double(*y)(double)):
         QwtSyntheticPointData(100),
         d_y(y)
      {
      }

      virtual QwtSeriesData<QwtDoublePoint> *copy() const
      {
         return new FunctionData(d_y);
      }

      virtual double y(double x) const
      {
         return d_y(x);
      }

   private:
      double(*d_y)(double);
};*/

//-----------------------------------------------------------------
//              simple.cpp
//
//      A simple example which shows how to use QwtPlot and QwtData
//-----------------------------------------------------------------

class SimpleData: public QwtData
{
   // The x values depend on its index and the y values
   // can be calculated from the corresponding x value. 
   // So we don´t need to store the values.
   // Such an implementation is slower because every point 
   // has to be recalculated for every replot, but it demonstrates how
   // QwtData can be used.

public:
    SimpleData(double(*y)(double), size_t size):
       d_size(size),
       d_y(y)
    {
    }

    virtual QwtData *copy() const
    {
       return new SimpleData(d_y, d_size);
    }

    virtual size_t size() const
    {
       return d_size;
    }

    virtual double x(size_t i) const
    {
       return 0.1 * i;
    }

    virtual double y(size_t i) const
    {
       return d_y(x(i));
    }
private:
    size_t d_size;
    double(*d_y)(double);
};

class Plot : public QwtPlot
{
   public:
      Plot(QWidget* widget) :
         QwtPlot(widget)
      {
         setTitle("A Simple QwtPlot Demonstration");
         insertLegend(new QwtLegend(), QwtPlot::RightLegend);

         // Set axes 
         setAxisTitle(xBottom, "xBottom");
         setAxisScale(xBottom, 0.0, 10.0);

         setAxisTitle(yLeft, "yLeft");
         setAxisScale(yLeft, -1.0, 1.0);
         enableAxis(yLeft1);
         setAxisTitle(yLeft1, "yLeft1");
         enableAxis(yLeft2);
         setAxisTitle(yLeft2, "yLeft2");
         enableAxis(yLeft3);
         setAxisTitle(yLeft3, "yLeft3");
         enableAxis(yRight);
         setAxisTitle(yRight, "yRight");
         enableAxis(yRight1);
         setAxisTitle(yRight1, "yRight1");
         enableAxis(yRight2);
         setAxisTitle(yRight2, "yRight2");
         enableAxis(yRight3);
         setAxisTitle(yRight3, "yRight3");

         // Insert new curves
         QwtPlotCurve *cSin = new QwtPlotCurve("y = sin(x)");
#if QT_VERSION >= 0x040000
         cSin->setRenderHint(QwtPlotItem::RenderAntialiased);
#endif
         cSin->setPen(QPen(Qt::red));
         cSin->attach(this);
         //cSin->setAxis(QwtPlot::xBottom, QwtPlot::yLeft);

         QwtPlotCurve *cCos = new QwtPlotCurve("y = cos(x)");
#if QT_VERSION >= 0x040000
         cCos->setRenderHint(QwtPlotItem::RenderAntialiased);
#endif
         cCos->setPen(QPen(Qt::blue));
         cCos->attach(this);
         //cCos->setAxis(QwtPlot::xBottom, QwtPlot::yRight);

         // Create sin and cos data
         const int nPoints = 100;
         cSin->setData(SimpleData(::sin, nPoints));
         cCos->setData(SimpleData(::cos, nPoints));

         // Insert markers

         //  ...a horizontal line at y = 0...
         QwtPlotMarker *mY = new QwtPlotMarker();
         mY->setLabel(QString::fromLatin1("y = 0"));
         mY->setLabelAlignment(Qt::AlignRight|Qt::AlignTop);
         mY->setLineStyle(QwtPlotMarker::HLine);
         mY->setYValue(0.0);
         mY->attach(this);

         //  ...a vertical line at x = 2 * pi
         QwtPlotMarker *mX = new QwtPlotMarker();
         mX->setLabel(QString::fromLatin1("x = 2 pi"));
         mX->setLabelAlignment(Qt::AlignRight|Qt::AlignTop);
         mX->setLineStyle(QwtPlotMarker::VLine);
         mX->setXValue(6.284);
         mX->attach(this);
      }
};

#endif // _PLOT_H_
