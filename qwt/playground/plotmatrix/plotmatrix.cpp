/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qlayout.h>
#include <qpen.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include "plotmatrix.h"

static void enablePlotAxis( QwtPlot *plot, int axis, bool on )
{
    // when false we still enable the axis to have an effect
    // of the minimal extent active. Instead we hide all visible
    // parts and margins/spacings.

    plot->setAxisVisible( axis, true );

    QwtScaleDraw *sd = plot->axisScaleDraw( axis );
    sd->enableComponent( QwtScaleDraw::Backbone, on );
    sd->enableComponent( QwtScaleDraw::Ticks, on );
    sd->enableComponent( QwtScaleDraw::Labels, on );

    QwtScaleWidget* sw = plot->axisWidget( axis );
    sw->setMargin( on ? 4 : 0 );
    sw->setSpacing( on ? 20 : 0 );
}

class Plot: public QwtPlot
{
public:
    Plot( QWidget *parent = NULL ):
        QwtPlot( parent )
    {
        QwtPlotCanvas *canvas = new QwtPlotCanvas();
        canvas->setLineWidth( 1 );
        canvas->setFrameStyle( QFrame::Box | QFrame::Plain );

        setCanvas( canvas );
    }

    virtual QSize sizeHint() const
    {
        return minimumSizeHint();
    }
};

class PlotMatrix::PrivateData
{
public:
    PrivateData():
        inScaleSync( false )
    {
        isAxisEnabled[QwtAxis::xBottom] = true;
        isAxisEnabled[QwtAxis::xTop] = false;
        isAxisEnabled[QwtAxis::yLeft] = true;
        isAxisEnabled[QwtAxis::yRight] = false;
    }

    bool isAxisEnabled[QwtAxis::PosCount];
    QVector<QwtPlot *> plotWidgets;
    mutable bool inScaleSync;
};

PlotMatrix::PlotMatrix( int numRows, int numColumns, QWidget *parent ):
    QFrame( parent )
{
    d_data = new PrivateData();
    d_data->plotWidgets.resize( numRows * numColumns );

    QGridLayout *layout = new QGridLayout( this );
    layout->setSpacing( 5 );

    for ( int row = 0; row < numRows; row++ )
    {
        for ( int col = 0; col < numColumns; col++ )
        {
            QwtPlot *plot = new Plot( this );
            layout->addWidget( plot, row, col );

            for ( int axis = 0; axis < QwtAxis::PosCount; axis++ )
            {
                connect( plot->axisWidget( axis ),
                    SIGNAL( scaleDivChanged() ), SLOT( scaleDivChanged() ) );
            }
            d_data->plotWidgets[row * numColumns + col] = plot;
        }
    }

    updateLayout();
}

PlotMatrix::~PlotMatrix()
{
    delete d_data;
}

int PlotMatrix::numRows() const
{
    const QGridLayout *l = qobject_cast<const QGridLayout *>( layout() );
    if ( l )
        return l->rowCount();

    return 0;
}

int PlotMatrix::numColumns() const
{
    const QGridLayout *l = qobject_cast<const QGridLayout *>( layout() );
    if ( l )
        return l->columnCount();
    return 0;
}

QwtPlot* PlotMatrix::plotAt( int row, int column )
{
    const int index = row * numColumns() + column;
    if ( index < d_data->plotWidgets.size() )
        return d_data->plotWidgets[index];

    return NULL;
}

const QwtPlot* PlotMatrix::plotAt( int row, int column ) const
{
    const int index = row * numColumns() + column;
    if ( index < d_data->plotWidgets.size() )
        return d_data->plotWidgets[index];

    return NULL;
}

void PlotMatrix::enableAxis( int axis, bool tf )
{
    if ( QwtAxis::isValid( axis ) )
    {
        if ( tf != d_data->isAxisEnabled[axis] )
        {
            d_data->isAxisEnabled[axis] = tf;
            updateLayout();
        }
    }
}

bool PlotMatrix::axisEnabled( int axis ) const
{
    if ( QwtAxis::isValid( axis ) )
        return d_data->isAxisEnabled[axis];

    return false;
}

void PlotMatrix::setAxisScale( int axis, int rowOrColumn,
    double min, double max, double step )
{
    int row = 0;
    int col = 0;

    if ( axis == QwtAxis::xBottom || axis == QwtAxis::xTop )
        col = rowOrColumn;
    else
        row = rowOrColumn;

    QwtPlot *plt = plotAt( row, col );
    if ( plt )
    {
        plt->setAxisScale( axis, min, max, step );
        plt->updateAxes();
    }
}

void PlotMatrix::scaleDivChanged()
{
    if ( d_data->inScaleSync )
        return;

    d_data->inScaleSync = true;

    QwtPlot *plt = NULL;
    int axisId = -1;
    int rowOrColumn = -1;

    // find the changed axis
    for ( int row = 0; row < numRows(); row++ )
    {
        for ( int col = 0; col < numColumns(); col++ )
        {
            QwtPlot *p = plotAt( row, col );
            if ( p )
            {
                for ( int axis = 0; axis < QwtAxis::PosCount; axis++ )
                {
                    if ( p->axisWidget( axis ) == sender() )
                    {
                        plt = p;
                        axisId = axis;
                        if ( axisId == QwtAxis::xBottom || axisId == QwtAxis::xTop )
                            rowOrColumn = col;
                        else
                            rowOrColumn = row;

                    }
                }
            }
        }
    }

    if ( plt )
    {
        const QwtScaleDiv scaleDiv = plt->axisScaleDiv( axisId );

        // synchronize the axes
        if ( axisId == QwtAxis::xBottom || axisId == QwtAxis::xTop )
        {
            for ( int row = 0; row < numRows(); row++ )
            {
                QwtPlot *p = plotAt( row, rowOrColumn );
                if ( p != plt )
                {
                    p->setAxisScaleDiv( axisId, scaleDiv );
                }
            }
        }
        else
        {
            for ( int col = 0; col < numColumns(); col++ )
            {
                QwtPlot *p = plotAt( rowOrColumn, col );
                if ( p != plt )
                {
                    p->setAxisScaleDiv( axisId, scaleDiv );
                }
            }
        }

        updateLayout();
    }

    d_data->inScaleSync = false;
}

void PlotMatrix::updateLayout()
{
    for ( int row = 0; row < numRows(); row++ )
    {
        for ( int col = 0; col < numColumns(); col++ )
        {
            QwtPlot *p = plotAt( row, col );
            if ( p )
            {
                bool showAxis[QwtAxis::PosCount];
                showAxis[QwtAxis::xBottom] =
                    axisEnabled( QwtAxis::xBottom ) && row == numRows() - 1;
                showAxis[QwtAxis::xTop] =
                    axisEnabled( QwtAxis::xTop ) && row == 0;
                showAxis[QwtAxis::yLeft] =
                    axisEnabled( QwtAxis::yLeft ) && col == 0;
                showAxis[QwtAxis::yRight] =
                    axisEnabled( QwtAxis::yRight ) && col == numColumns() - 1;

                for ( int axis = 0; axis < QwtAxis::PosCount; axis++ )
                {
                    enablePlotAxis( p, axis, showAxis[axis] );
                }
            }
        }
    }

    for ( int row = 0; row < numRows(); row++ )
    {
        alignAxes( row, QwtAxis::xTop );
        alignAxes( row, QwtAxis::xBottom );

        alignScaleBorder( row, QwtAxis::yLeft );
        alignScaleBorder( row, QwtAxis::yRight );
    }

    for ( int col = 0; col < numColumns(); col++ )
    {
        alignAxes( col, QwtAxis::yLeft );
        alignAxes( col, QwtAxis::yRight );

        alignScaleBorder( col, QwtAxis::xBottom );
        alignScaleBorder( col, QwtAxis::xTop );
    }

    for ( int row = 0; row < numRows(); row++ )
    {
        for ( int col = 0; col < numColumns(); col++ )
        {
            QwtPlot *p = plotAt( row, col );
            if ( p )
                p->replot();
        }
    }
}

void PlotMatrix::alignAxes( int rowOrColumn, int axis )
{
    if ( QwtAxis::isYAxis( axis ) )
    {
        double maxExtent = 0;

        for ( int row = 0; row < numRows(); row++ )
        {
            QwtPlot *p = plotAt( row, rowOrColumn );
            if ( p )
            {
                QwtScaleWidget *scaleWidget = p->axisWidget( axis );

                QwtScaleDraw *sd = scaleWidget->scaleDraw();
                sd->setMinimumExtent( 0.0 );

                const double extent = sd->extent( scaleWidget->font() );
                if ( extent > maxExtent )
                    maxExtent = extent;
            }
        }

        for ( int row = 0; row < numRows(); row++ )
        {
            QwtPlot *p = plotAt( row, rowOrColumn );
            if ( p )
            {
                QwtScaleWidget *scaleWidget = p->axisWidget( axis );
                scaleWidget->scaleDraw()->setMinimumExtent( maxExtent );
            }
        }
    }
    else
    {
        double maxExtent = 0;

        for ( int col = 0; col < numColumns(); col++ )
        {
            QwtPlot *p = plotAt( rowOrColumn, col );
            if ( p )
            {
                QwtScaleWidget *scaleWidget = p->axisWidget( axis );

                QwtScaleDraw *sd = scaleWidget->scaleDraw();
                sd->setMinimumExtent( 0.0 );

                const double extent = sd->extent( scaleWidget->font() );
                if ( extent > maxExtent )
                    maxExtent = extent;
            }
        }

        for ( int col = 0; col < numColumns(); col++ )
        {
            QwtPlot *p = plotAt( rowOrColumn, col );
            if ( p )
            {
                QwtScaleWidget *scaleWidget = p->axisWidget( axis );
                scaleWidget->scaleDraw()->setMinimumExtent( maxExtent );
            }
        }
    }
}

void PlotMatrix::alignScaleBorder( int rowOrColumn, int axis )
{
    int startDist = 0;
    int endDist = 0;

    if ( axis == QwtAxis::yLeft )
    {
        QwtPlot *p = plotAt( rowOrColumn, 0 );
        if ( p )
            p->axisWidget( axis )->getBorderDistHint( startDist, endDist );

        for ( int col = 1; col < numColumns(); col++ )
        {
            QwtPlot *p = plotAt( rowOrColumn, col );
            if ( p )
                p->axisWidget( axis )->setMinBorderDist( startDist, endDist );
        }
    }
    else if ( axis == QwtAxis::yRight )
    {
        QwtPlot *p = plotAt( rowOrColumn, numColumns() - 1 );
        if ( p )
            p->axisWidget( axis )->getBorderDistHint( startDist, endDist );

        for ( int col = 0; col < numColumns() - 1; col++ )
        {
            QwtPlot *p = plotAt( rowOrColumn, col );
            if ( p )
                p->axisWidget( axis )->setMinBorderDist( startDist, endDist );
        }       
    }
    if ( axis == QwtAxis::xTop )
    {
        QwtPlot *p = plotAt( rowOrColumn, 0 );
        if ( p )
            p->axisWidget( axis )->getBorderDistHint( startDist, endDist );
    
        for ( int row = 1; row < numRows(); row++ )
        {
            QwtPlot *p = plotAt( row, rowOrColumn );
            if ( p )
                p->axisWidget( axis )->setMinBorderDist( startDist, endDist );
        }   
    }   
    else if ( axis == QwtAxis::xBottom )
    {       
        QwtPlot *p = plotAt( numRows() - 1, rowOrColumn );
        if ( p )
            p->axisWidget( axis )->getBorderDistHint( startDist, endDist );
    
        for ( int row = 0; row < numRows() - 1; row++ )
        {
            QwtPlot *p = plotAt( row, rowOrColumn );
            if ( p )
                p->axisWidget( axis )->setMinBorderDist( startDist, endDist );
        }
    }
}
