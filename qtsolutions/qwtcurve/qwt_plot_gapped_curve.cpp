#include "qwt_plot_gapped_curve.h"

////////////////////////////////////////////////////////////////////////////////
QwtPlotGappedCurve::QwtPlotGappedCurve(double gapValue) 
:	QwtPlotCurve(), 
    gapValue_(gapValue), naValue_(0)
{
}

////////////////////////////////////////////////////////////////////////////////
QwtPlotGappedCurve::QwtPlotGappedCurve(const QwtText &title, double gapValue)
:	QwtPlotCurve(title), 
    gapValue_(gapValue), naValue_(0)
{
}

////////////////////////////////////////////////////////////////////////////////
QwtPlotGappedCurve::QwtPlotGappedCurve(const QString &title, double gapValue)
:	QwtPlotCurve(title), 
    gapValue_(gapValue) , naValue_(0)
{
}

////////////////////////////////////////////////////////////////////////////////
void QwtPlotGappedCurve::drawSeries(QPainter *painter, const QwtScaleMap &xMap,
                                                          const QwtScaleMap &yMap, const QRectF &canvRect, int from, int to) const
{
    if ( !painter || dataSize() <= 0 )
        return;

    if (to < 0)
        to = dataSize() - 1;

    int i = from;
    double last = 0;
    while (i < to)
    {
        // First non-missed point will be the start of curve section.
        double x = sample(i).x();
        double y = sample(i).y();
        double yprev = 0;
        if (i>0) yprev = sample(i-1).y();

        if ((y < (naValue_ + -0.001) || y > (naValue_ + 0.001)) && (x - last <= gapValue_) &&
            (yprev < (naValue_ + -0.001) || yprev > (naValue_ + 0.001))) {

            int start = i-1;
            int end = i;

            // Draw the curve section
            QwtPlotCurve::drawSeries(painter, xMap, yMap, canvRect, start, end);
        }

        last = x;
        i++;
    }
}

////////////////////////////////////////////////////////////////////////////////
