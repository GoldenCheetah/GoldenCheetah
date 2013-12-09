/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot.h"
#include "qwt_scale_div.h"
#include "qwt_plot_magnifier.h"
#include "qwt_axes_mask.h"

class QwtPlotMagnifier::PrivateData
{
public:
    QwtAxesMask disabledAxes;
};

/*!
   Constructor
   \param canvas Plot canvas to be magnified
*/
QwtPlotMagnifier::QwtPlotMagnifier( QWidget *canvas ):
    QwtMagnifier( canvas )
{
    d_data = new PrivateData();
}

//! Destructor
QwtPlotMagnifier::~QwtPlotMagnifier()
{
    delete d_data;
}

/*!
   \brief En/Disable an axis

   Only Axes that are enabled will be zoomed.
   All other axes will remain unchanged.

   \param axisId Axis id
   \param on On/Off

   \sa isAxisEnabled()
*/
void QwtPlotMagnifier::setAxisEnabled( QwtAxisId axisId, bool on )
{
    d_data->disabledAxes.setEnabled( axisId, !on );
}

/*!
   Test if an axis is enabled

   \param axisId Axis id
   \return True, if the axis is enabled

   \sa setAxisEnabled()
*/
bool QwtPlotMagnifier::isAxisEnabled( QwtAxisId axisId ) const
{
    return !d_data->disabledAxes.isEnabled( axisId );
}

//! Return observed plot canvas
QWidget *QwtPlotMagnifier::canvas()
{
    return parentWidget();
}

//! Return Observed plot canvas
const QWidget *QwtPlotMagnifier::canvas() const
{
    return parentWidget();
}

//! Return plot widget, containing the observed plot canvas
QwtPlot *QwtPlotMagnifier::plot()
{
    QWidget *w = canvas();
    if ( w )
        w = w->parentWidget();

    return qobject_cast<QwtPlot *>( w );
}

//! Return plot widget, containing the observed plot canvas
const QwtPlot *QwtPlotMagnifier::plot() const
{
    const QWidget *w = canvas();
    if ( w )
        w = w->parentWidget();

    return qobject_cast<const QwtPlot *>( w );
}

/*!
   Zoom in/out the axes scales
   \param factor A value < 1.0 zooms in, a value > 1.0 zooms out.
*/
void QwtPlotMagnifier::rescale( double factor )
{
    QwtPlot* plt = plot();
    if ( plt == NULL )
        return;

    factor = qAbs( factor );
    if ( factor == 1.0 || factor == 0.0 )
        return;

    bool doReplot = false;

    const bool autoReplot = plt->autoReplot();
    plt->setAutoReplot( false );

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        const int axesCount = plt->axesCount( axisPos );

        for ( int i = 0; i < axesCount; i++ )
        {
            const QwtAxisId axisId( axisPos, i );

            const QwtScaleDiv &scaleDiv = plt->axisScaleDiv( axisId );
            if ( isAxisEnabled( axisId ) )
            {
                const double center = scaleDiv.lowerBound() + scaleDiv.range() / 2;
                const double width_2 = scaleDiv.range() / 2 * factor;

                plt->setAxisScale( axisId, center - width_2, center + width_2 );
                doReplot = true;
            }
        }
    }

    plt->setAutoReplot( autoReplot );

    if ( doReplot )
        plt->replot();
}
