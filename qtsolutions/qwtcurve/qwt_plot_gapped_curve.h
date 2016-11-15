#ifndef QWT_PLOT_GAPPED_CURVE_H
#define QWT_PLOT_GAPPED_CURVE_H

#include <QString>

#include "qwt_plot_curve.h"
#include "qwt_text.h"
#include "qwt_plot_curve.h"
#include "qwt_scale_map.h"

////////////////////////////////////////////////////////////////////////////////
/// This class is a plot curve that can have gaps at points
/// with specific Y value.
/// This specific "gap" value can be passed to constructors, 
/// it's zero by default.  
class QwtPlotGappedCurve: public QwtPlotCurve
{
public:
	/// gapValue is an Y value which denotes missed data
    QwtPlotGappedCurve(double gapValue = 0);
    
	/// gapValue is an Y value which denotes missed data
    QwtPlotGappedCurve(const QwtText &title, double gapValue = 0);

	/// gapValue is an Y value which denotes missed data
    QwtPlotGappedCurve(const QString &title, double gapValue = 0);

    /// Override draw method to create gaps for missed Y values 
    virtual void drawSeries(QPainter *painter, const QwtScaleMap &xMap,
                                                const QwtScaleMap &yMap, const QRectF &canvRect, int from, int to) const;

    void setNAValue(double x) { naValue_=x; }

private:
	/// Value that denotes missed Y data at point
    double gapValue_;
    double naValue_;
};

////////////////////////////////////////////////////////////////////////////////

#endif
