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
        to = dataSize();

    int i = from;
    double last = 0;
    bool isGap = true;
    int startPoint = to;
    while (i < to)
    {
        // First non-missed point will be the start of curve section.
        double x = sample(i).x();
        double y = sample(i).y();
        double yprev = 0;
        if (i>0) yprev = sample(i-1).y();

        if ((y < (naValue_ + -0.001) || y > (naValue_ + 0.001)) && (x - last <= gapValue_) &&
            (yprev < (naValue_ + -0.001) || yprev > (naValue_ + 0.001))) {

            // check if this is the first point after a gap
            if (isGap) {

                // this is the start point of a new segment
                startPoint = i;
            }

            isGap = false;
        } else {

            // check whether this is the end of a segment
            if (!isGap) {

                // draw the segment
                QwtPlotCurve::drawSeries(painter, xMap, yMap, canvRect, startPoint, i - 1);
            }

            isGap = true;
        }

        last = x;
        i++;
    }

    // if there is a segment still pending to be drawn, draw it
    if (!isGap) {
        QwtPlotCurve::drawSeries(painter, xMap, yMap, canvRect, startPoint, i - 1);
    }
}

void QwtPlotGappedCurve::drawLines(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                                   const QRectF &canvasRect, int from, int to) const
{
    // if this segment consists only of a single point, draw a point instead of line
    if (from == to) {
        QwtPlotCurve::drawDots(p, xMap, yMap, canvasRect, from, to);
    } else {
        QwtPlotCurve::drawLines(p, xMap, yMap, canvasRect, from, to);
    }
}

////////////////////////////////////////////////////////////////////////////////
