// code borrowed from event_filter qwt examples
// and modified for GC LTM purposes

#include <qapplication.h>
#include <qevent.h>
#include <qwhatsthis.h>
#include <qpainter.h>
#include <qwt_plot.h>
#include <qwt_symbol.h>
#include <qwt_scale_map.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include "LTMCanvasPicker.h"

#include <QDebug>

LTMCanvasPicker::LTMCanvasPicker(QwtPlot *plot):
    QObject(plot),
    d_selectedCurve(NULL),
    d_selectedPoint(-1)
{
    canvas = static_cast<QwtPlotCanvas*>(plot->canvas());
    canvas->installEventFilter(this);


    // We want the focus, but no focus rect. The
    canvas->setFocusPolicy(Qt::StrongFocus);
    canvas->setFocusIndicator(QwtPlotCanvas::ItemFocusIndicator);
}

bool LTMCanvasPicker::event(QEvent *e)
{
    if ( e->type() == QEvent::User )
    {
        //showCursor(true);
        return true;
    }
    return QObject::event(e);
}

bool LTMCanvasPicker::eventFilter(QObject *object, QEvent *e)
{
    // for our canvas ?
    if (object != canvas) return false;

    switch(e->type())
    {
        default:
            QApplication::postEvent(this, new QEvent(QEvent::User));
            break;
        case QEvent::MouseButtonPress:
            select(((QMouseEvent *)e)->pos(), true);
            break;
        case QEvent::MouseMove:
            select(((QMouseEvent *)e)->pos(), false);
            break;
    }
    return QObject::eventFilter(object, e);
}

// Select the point at a position. If there is no point
// deselect the selected point

void LTMCanvasPicker::select(const QPoint &pos, bool clicked)
{
    QwtPlotCurve *curve = NULL;
    double dist = 10e10;
    int index = -1;

    const QwtPlotItemList& itmList = plot()->itemList();
    for ( QwtPlotItemIterator it = itmList.begin();
        it != itmList.end(); ++it )
    {
        if ( (*it)->rtti() == QwtPlotItem::Rtti_PlotCurve )
        {
            QwtPlotCurve *c = (QwtPlotCurve*)(*it);

            double d = -1;
            int idx = c->closestPoint(pos, &d);
            if ( d != -1 && d < dist )
            {
                curve = c;
                index = idx;
                dist = d;
            }
        }
    }

    d_selectedCurve = NULL;
    d_selectedPoint = -1;

    if ( curve && dist < 10 ) // 10 pixels tolerance
    {
        // picked one
        d_selectedCurve = curve;
        d_selectedPoint = index;

        if (clicked)
            pointClicked(curve, index); // emit
        else
            pointHover(curve, index);  // emit
    } else {
        // didn't
        if (clicked)
            pointClicked(NULL, -1); // emit
        else
            pointHover(NULL, -1);  // emit

    }
}
