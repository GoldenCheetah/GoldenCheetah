// code stolen from the event_filter qwt example
// and modified for GC LTM

#ifndef GC_LTMCanvasPicker_H
#define GC_LTMCanvasPicker_H 1
#include "GoldenCheetah.h"

#include <qobject.h>

class QPoint;
class QCustomEvent;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotCanvas;

class LTMCanvasPicker: public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    LTMCanvasPicker(QwtPlot *plot);
    virtual bool eventFilter(QObject *, QEvent *);
    virtual bool event(QEvent *);

signals:
    void pointClicked(QwtPlotCurve *, int);
    void pointHover(QwtPlotCurve *, int);

private:
    QwtPlotCanvas *canvas;
    void select(const QPoint &, bool);
    QwtPlot *plot() { return (QwtPlot *)parent(); }
    const QwtPlot *plot() const { return (QwtPlot *)parent(); }
    QwtPlotCurve *d_selectedCurve;
    int d_selectedPoint;
};

#endif
