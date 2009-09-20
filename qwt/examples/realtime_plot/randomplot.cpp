#include <stdlib.h>
#include <qtimer.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include "scrollzoomer.h"
#include "randomplot.h"

const unsigned int c_rangeMax = 1000;

class Zoomer: public ScrollZoomer
{
public:
    Zoomer(QwtPlotCanvas *canvas):
        ScrollZoomer(canvas)
    {
    }

    virtual void rescale()
    {
        QwtScaleWidget *scaleWidget = plot()->axisWidget(yAxis());
        QwtScaleDraw *sd = scaleWidget->scaleDraw();

        int minExtent = 0;
        if ( zoomRectIndex() > 0 )
        {
            // When scrolling in vertical direction
            // the plot is jumping in horizontal direction
            // because of the different widths of the labels
            // So we better use a fixed extent.

            minExtent = sd->spacing() + sd->majTickLength() + 1;
            minExtent += sd->labelSize(
                scaleWidget->font(), c_rangeMax).width();
        }

        sd->setMinimumExtent(minExtent);

        ScrollZoomer::rescale();
    }
};

RandomPlot::RandomPlot(QWidget *parent):
    IncrementalPlot(parent),
    d_timer(0),
    d_timerCount(0)
{
    setFrameStyle(QFrame::NoFrame);
    setLineWidth(0);
    setCanvasLineWidth(2);

    plotLayout()->setAlignCanvasToScales(true);

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(this);

    setCanvasBackground(QColor(29, 100, 141)); // nice blue

    setAxisScale(xBottom, 0, c_rangeMax);
    setAxisScale(yLeft, 0, c_rangeMax);
    
    replot();

    // enable zooming

    Zoomer *zoomer = new Zoomer(canvas());
    zoomer->setRubberBandPen(QPen(Qt::red, 2, Qt::DotLine));
    zoomer->setTrackerPen(QPen(Qt::red));
}

QSize RandomPlot::sizeHint() const
{
    return QSize(540,400);
}

void RandomPlot::appendPoint()
{
    double x = rand() % c_rangeMax;
    x += ( rand() % 100 ) / 100;

    double y = rand() % c_rangeMax;
    y += ( rand() % 100 ) / 100;

    appendData(x, y);

    if ( --d_timerCount <= 0 )
        stop();
}

void RandomPlot::append(int timeout, int count)
{
    if ( !d_timer )
    {
        d_timer = new QTimer(this);
        connect(d_timer, SIGNAL(timeout()), SLOT(appendPoint()));
    }

    d_timerCount = count;

    emit running(true);

    canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, false);
    d_timer->start(timeout);
}

void RandomPlot::stop()
{
    if ( d_timer )
    {
        d_timer->stop();
        emit running(false);
    }

    canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, true);
}

void RandomPlot::clear()
{
    removeData();
    replot();
}
