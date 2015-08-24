#include "qwt_plot_gapped_curve.h"

////////////////////////////////////////////////////////////////////////////////
QwtPlotGappedCurve::QwtPlotGappedCurve(double gapValue) 
:	QwtPlotCurve(), 
	gapValue_(gapValue) 
{
}

////////////////////////////////////////////////////////////////////////////////
QwtPlotGappedCurve::QwtPlotGappedCurve(const QwtText &title, double gapValue)
:	QwtPlotCurve(title), 
	gapValue_(gapValue)  
{
}

////////////////////////////////////////////////////////////////////////////////
QwtPlotGappedCurve::QwtPlotGappedCurve(const QString &title, double gapValue)
:	QwtPlotCurve(title), 
	gapValue_(gapValue) 
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
    while (i < to)
    {
        // If data begins with missed values,
        // we need to find first non-missed point.
            double y = sample(i).y();
            while ((i < to) && (y == gapValue_))
            {
                ++i;
                y = sample(i).y();
            }
        // First non-missed point will be the start of curve section.
            int start = i;
            y = sample(i).y();
        // Find the last non-missed point, it will be the end of curve section.
            while ((i < to) && (y != gapValue_))
            {
                ++i;
                y = sample(i).y();
            }
        // Correct the end of the section if it is at missed point
            int end = (y == gapValue_) ? i - 1 : i;

        // Draw the curve section
            if (start <= end)
                    QwtPlotCurve::drawSeries(painter, xMap, yMap, canvRect, start, end);
    }
}

////////////////////////////////////////////////////////////////////////////////
