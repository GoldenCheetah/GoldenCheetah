/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_map.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"
#include "qwt_interval.h"

namespace
{
    class AxisData
    {
      public:
        AxisData()
            : isVisible( true )
            , doAutoScale( true )
            , minValue( 0.0 )
            , maxValue( 1000.0 )
            , stepSize( 0.0 )
            , maxMajor( 8 )
            , maxMinor( 5 )
            , isValid( false )
            , scaleEngine( new QwtLinearScaleEngine() )
            , scaleWidget( NULL )
        {
        }

        ~AxisData()
        {
            delete scaleEngine;
        }

        void initWidget( QwtScaleDraw::Alignment align, const QString& name, QwtPlot* plot )
        {
            scaleWidget = new QwtScaleWidget( align, plot );
            scaleWidget->setObjectName( name );

    #if 1
            // better find the font sizes from the application font
            const QFont fscl( plot->fontInfo().family(), 10 );
            const QFont fttl( plot->fontInfo().family(), 12, QFont::Bold );
    #endif

            scaleWidget->setTransformation( scaleEngine->transformation() );

            scaleWidget->setFont( fscl );
            scaleWidget->setMargin( 2 );

            QwtText text = scaleWidget->title();
            text.setFont( fttl );
            scaleWidget->setTitle( text );
        }

        bool isVisible;
        bool doAutoScale;

        double minValue;
        double maxValue;
        double stepSize;

        int maxMajor;
        int maxMinor;

        bool isValid;

        QwtScaleDiv scaleDiv;
        QwtScaleEngine* scaleEngine;
        QwtScaleWidget* scaleWidget;
    };
}

class QwtPlot::ScaleData
{
  public:
    ScaleData( QwtPlot* plot )
    {
        for ( int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++ )
            setAxesCount( plot, axisPos, 1 );
    }

    void setAxesCount( QwtPlot* plot, int axisPos, int count )
    {
        QVector< AxisData >& axisData = d[axisPos].axisData;

        for ( int i = count; i < axisData.size(); i++ )
        {
            delete axisData[i].scaleEngine;
            delete axisData[i].scaleWidget;
        }

        const int numAxis = axisData.size();
        axisData.resize( count );

        for ( int i = numAxis; i < count; i++ )
        {
            QString name;
            QwtScaleDraw::Alignment align = QwtScaleDraw::LeftScale;

            switch( axisPos )
            {
                case QwtAxis::YLeft:
                    align = QwtScaleDraw::LeftScale;
                    name = "QwtPlotAxisYLeft";
                    break;
                case QwtAxis::YRight:
                    align = QwtScaleDraw::RightScale;
                    name = "QwtPlotAxisYRight";
                    break;
                case QwtAxis::XBottom:
                    align = QwtScaleDraw::BottomScale;
                    name = "QwtPlotAxisXBottom";
                    break;
                case QwtAxis::XTop:
                    align = QwtScaleDraw::TopScale;
                    name = "QwtPlotAxisXTop";
                    break;
            }

            if ( i > 0 )
                name += QString().setNum( i );


            axisData[ i ].initWidget( align, name, plot );
        }
    }

    inline int axesCount( int pos ) const
    {
        if ( !QwtAxis::isValid( pos ) )
            return -1;

        return d[pos].axisData.count();
    }

    inline AxisData& axisData( QwtAxisId axisId )
    {
        return d[ axisId.pos ].axisData[ axisId.id ];
    }

    inline const AxisData& axisData( QwtAxisId axisId ) const
    {
        return d[ axisId.pos ].axisData[ axisId.id ];
    }

    struct
    {
        QVector< AxisData > axisData;

    } d[ QwtAxis::AxisPositions ];
};

void QwtPlot::initAxesData()
{
    m_scaleData = new ScaleData( this );

    m_scaleData->axisData( QwtAxis::YRight ).isVisible = false;
    m_scaleData->axisData( QwtAxis::XTop ).isVisible = false;
}

void QwtPlot::deleteAxesData()
{
    delete m_scaleData;
    m_scaleData = NULL;
}

void QwtPlot::setAxesCount( int axisPos, int count )
{
    count = qMax( count, 1 ); // we need at least one axis

    if ( count != axesCount( axisPos ) )
    {
        m_scaleData->setAxesCount( this, axisPos, count );
        autoRefresh();
    }
}

int QwtPlot::axesCount( int axisPos, bool onlyVisible ) const
{
    int count = 0;

    if ( onlyVisible )
    {
        for ( int i = 0; i < m_scaleData->axesCount( axisPos ); i++ )
        {
            const QwtAxisId axisId( axisPos, i );
            if ( m_scaleData->axisData( axisId ).isVisible )
                count++;
        }
    }
    else
    {
        count = m_scaleData->axesCount( axisPos );
    }

    return count;
}

/*!
   Checks if an axis is valid

   \param axisId axis
   \return \c true if the specified axis exists, otherwise \c false
 */
bool QwtPlot::isAxisValid( QwtAxisId axisId ) const
{
    return m_scaleData->axesCount( axisId.pos ) > axisId.id;
}

/*!
   \return Scale widget of the specified axis, or NULL if axisId is invalid.
   \param axisId Axis
 */
const QwtScaleWidget* QwtPlot::axisWidget( QwtAxisId axisId ) const
{
    if ( isAxisValid( axisId ) )
        return m_scaleData->axisData( axisId ).scaleWidget;

    return NULL;
}

/*!
   \return Scale widget of the specified axis, or NULL if axisId is invalid.
   \param axisId Axis
 */
QwtScaleWidget* QwtPlot::axisWidget( QwtAxisId axisId )
{
    if ( isAxisValid( axisId ) )
        return m_scaleData->axisData( axisId ).scaleWidget;

    return NULL;
}

/*!
   Change the scale engine for an axis

   \param axisId Axis
   \param scaleEngine Scale engine

   \sa axisScaleEngine()
 */
void QwtPlot::setAxisScaleEngine( QwtAxisId axisId, QwtScaleEngine* scaleEngine )
{
    if ( isAxisValid( axisId ) && scaleEngine != NULL )
    {
        AxisData& d = m_scaleData->axisData( axisId );

        delete d.scaleEngine;
        d.scaleEngine = scaleEngine;

        d.scaleWidget->setTransformation( scaleEngine->transformation() );

        d.isValid = false;

        autoRefresh();
    }
}

/*!
   \param axisId Axis
   \return Scale engine for a specific axis
 */
QwtScaleEngine* QwtPlot::axisScaleEngine( QwtAxisId axisId )
{
    if ( isAxisValid( axisId ) )
        return m_scaleData->axisData( axisId ).scaleEngine;
    else
        return NULL;
}

/*!
   \param axisId Axis
   \return Scale engine for a specific axis
 */
const QwtScaleEngine* QwtPlot::axisScaleEngine( QwtAxisId axisId ) const
{
    if ( isAxisValid( axisId ) )
        return m_scaleData->axisData( axisId ).scaleEngine;
    else
        return NULL;
}
/*!
   \return \c True, if autoscaling is enabled
   \param axisId Axis
 */
bool QwtPlot::axisAutoScale( QwtAxisId axisId ) const
{
    if ( isAxisValid( axisId ) )
        return m_scaleData->axisData( axisId ).doAutoScale;
    else
        return false;
}

/*!
   \return \c True, if a specified axis is visible
   \param axisId Axis
 */
bool QwtPlot::isAxisVisible( QwtAxisId axisId ) const
{
    if ( isAxisValid( axisId ) )
        return m_scaleData->axisData( axisId ).isVisible;
    else
        return false;
}

/*!
   \return The font of the scale labels for a specified axis
   \param axisId Axis
 */
QFont QwtPlot::axisFont( QwtAxisId axisId ) const
{
    if ( isAxisValid( axisId ) )
        return axisWidget( axisId )->font();
    else
        return QFont();

}

/*!
   \return The maximum number of major ticks for a specified axis
   \param axisId Axis
   \sa setAxisMaxMajor(), QwtScaleEngine::divideScale()
 */
int QwtPlot::axisMaxMajor( QwtAxisId axisId ) const
{
    if ( isAxisValid( axisId ) )
        return m_scaleData->axisData( axisId ).maxMajor;
    else
        return 0;
}

/*!
   \return the maximum number of minor ticks for a specified axis
   \param axisId Axis
   \sa setAxisMaxMinor(), QwtScaleEngine::divideScale()
 */
int QwtPlot::axisMaxMinor( QwtAxisId axisId ) const
{
    if ( isAxisValid( axisId ) )
        return m_scaleData->axisData( axisId ).maxMinor;
    else
        return 0;
}

/*!
   \brief Return the scale division of a specified axis

   axisScaleDiv(axisId).lowerBound(), axisScaleDiv(axisId).upperBound()
   are the current limits of the axis scale.

   \param axisId Axis
   \return Scale division

   \sa QwtScaleDiv, setAxisScaleDiv(), QwtScaleEngine::divideScale()
 */
const QwtScaleDiv& QwtPlot::axisScaleDiv( QwtAxisId axisId ) const
{
    return m_scaleData->axisData( axisId ).scaleDiv;
}

/*!
   \brief Return the scale draw of a specified axis

   \param axisId Axis
   \return Specified scaleDraw for axis, or NULL if axis is invalid.
 */
const QwtScaleDraw* QwtPlot::axisScaleDraw( QwtAxisId axisId ) const
{
    if ( !isAxisValid( axisId ) )
        return NULL;

    return axisWidget( axisId )->scaleDraw();
}

/*!
   \brief Return the scale draw of a specified axis

   \param axisId Axis
   \return Specified scaleDraw for axis, or NULL if axis is invalid.
 */
QwtScaleDraw* QwtPlot::axisScaleDraw( QwtAxisId axisId )
{
    if ( !isAxisValid( axisId ) )
        return NULL;

    return axisWidget( axisId )->scaleDraw();
}

/*!
   \brief Return the step size parameter that has been set in setAxisScale.

   This doesn't need to be the step size of the current scale.

   \param axisId Axis
   \return step size parameter value

   \sa setAxisScale(), QwtScaleEngine::divideScale()
 */
double QwtPlot::axisStepSize( QwtAxisId axisId ) const
{
    if ( !isAxisValid( axisId ) )
        return 0;

    return m_scaleData->axisData( axisId ).stepSize;
}

/*!
   \brief Return the current interval of the specified axis

   This is only a convenience function for axisScaleDiv( axisId )->interval();

   \param axisId Axis
   \return Scale interval

   \sa QwtScaleDiv, axisScaleDiv()
 */
QwtInterval QwtPlot::axisInterval( QwtAxisId axisId ) const
{
    if ( !isAxisValid( axisId ) )
        return QwtInterval();

    return m_scaleData->axisData( axisId ).scaleDiv.interval();
}

/*!
   \return Title of a specified axis
   \param axisId Axis
 */
QwtText QwtPlot::axisTitle( QwtAxisId axisId ) const
{
    if ( isAxisValid( axisId ) )
        return axisWidget( axisId )->title();
    else
        return QwtText();
}

/*!
   \brief Hide or show a specified axis

   Curves, markers and other items can be attached
   to hidden axes, and transformation of screen coordinates
   into values works as normal.

   Only QwtAxis::XBottom and QwtAxis::YLeft are enabled by default.

   \param axisId Axis
   \param on \c true (visible) or \c false (hidden)
 */
void QwtPlot::setAxisVisible( QwtAxisId axisId, bool on )
{
    if ( isAxisValid( axisId ) && on != m_scaleData->axisData( axisId ).isVisible )
    {
        m_scaleData->axisData( axisId ).isVisible = on;
        updateLayout();
    }
}

/*!
   Transform the x or y coordinate of a position in the
   drawing region into a value.

   \param axisId Axis
   \param pos position

   \return Position as axis coordinate

   \warning The position can be an x or a y coordinate,
           depending on the specified axis.
 */
double QwtPlot::invTransform( QwtAxisId axisId, double pos ) const
{
    if ( isAxisValid( axisId ) )
        return( canvasMap( axisId ).invTransform( pos ) );
    else
        return 0.0;
}


/*!
   \brief Transform a value into a coordinate in the plotting region

   \param axisId Axis
   \param value value
   \return X or Y coordinate in the plotting region corresponding
          to the value.
 */
double QwtPlot::transform( QwtAxisId axisId, double value ) const
{
    if ( isAxisValid( axisId ) )
        return( canvasMap( axisId ).transform( value ) );
    else
        return 0.0;
}

/*!
   \brief Change the font of an axis

   \param axisId Axis
   \param font Font
   \warning This function changes the font of the tick labels,
           not of the axis title.
 */
void QwtPlot::setAxisFont( QwtAxisId axisId, const QFont& font )
{
    if ( isAxisValid( axisId ) )
        axisWidget( axisId )->setFont( font );
}

/*!
   \brief Enable autoscaling for a specified axis

   This member function is used to switch back to autoscaling mode
   after a fixed scale has been set. Autoscaling is enabled by default.

   \param axisId Axis
   \param on On/Off
   \sa setAxisScale(), setAxisScaleDiv(), updateAxes()

   \note The autoscaling flag has no effect until updateAxes() is executed
        ( called by replot() ).
 */
void QwtPlot::setAxisAutoScale( QwtAxisId axisId, bool on )
{
    if ( isAxisValid( axisId ) && ( m_scaleData->axisData( axisId ).doAutoScale != on ) )
    {
        m_scaleData->axisData( axisId ).doAutoScale = on;
        autoRefresh();
    }
}

/*!
   \brief Disable autoscaling and specify a fixed scale for a selected axis.

   In updateAxes() the scale engine calculates a scale division from the
   specified parameters, that will be assigned to the scale widget. So
   updates of the scale widget usually happen delayed with the next replot.

   \param axisId Axis
   \param min Minimum of the scale
   \param max Maximum of the scale
   \param stepSize Major step size. If <code>step == 0</code>, the step size is
                  calculated automatically using the maxMajor setting.

   \sa setAxisMaxMajor(), setAxisAutoScale(), axisStepSize(), QwtScaleEngine::divideScale()
 */
void QwtPlot::setAxisScale( QwtAxisId axisId, double min, double max, double stepSize )
{
    if ( isAxisValid( axisId ) )
    {
        AxisData& d = m_scaleData->axisData( axisId );

        d.doAutoScale = false;
        d.isValid = false;

        d.minValue = min;
        d.maxValue = max;
        d.stepSize = stepSize;

        autoRefresh();
    }
}

/*!
   \brief Disable autoscaling and specify a fixed scale for a selected axis.

   The scale division will be stored locally only until the next call
   of updateAxes(). So updates of the scale widget usually happen delayed with
   the next replot.

   \param axisId Axis
   \param scaleDiv Scale division

   \sa setAxisScale(), setAxisAutoScale()
 */
void QwtPlot::setAxisScaleDiv( QwtAxisId axisId, const QwtScaleDiv& scaleDiv )
{
    if ( isAxisValid( axisId ) )
    {
        AxisData& d = m_scaleData->axisData( axisId );

        d.doAutoScale = false;
        d.scaleDiv = scaleDiv;
        d.isValid = true;

        autoRefresh();
    }
}

/*!
   \brief Set a scale draw

   \param axisId Axis
   \param scaleDraw Object responsible for drawing scales.

   By passing scaleDraw it is possible to extend QwtScaleDraw
   functionality and let it take place in QwtPlot. Please note
   that scaleDraw has to be created with new and will be deleted
   by the corresponding QwtScale member ( like a child object ).

   \sa QwtScaleDraw, QwtScaleWidget
   \warning The attributes of scaleDraw will be overwritten by those of the
           previous QwtScaleDraw.
 */

void QwtPlot::setAxisScaleDraw( QwtAxisId axisId, QwtScaleDraw* scaleDraw )
{
    if ( isAxisValid( axisId ) )
    {
        axisWidget( axisId )->setScaleDraw( scaleDraw );
        autoRefresh();
    }
}

/*!
   Change the alignment of the tick labels

   \param axisId Axis
   \param alignment Or'd Qt::AlignmentFlags see <qnamespace.h>

   \sa QwtScaleDraw::setLabelAlignment()
 */
void QwtPlot::setAxisLabelAlignment( QwtAxisId axisId, Qt::Alignment alignment )
{
    if ( isAxisValid( axisId ) )
        axisWidget( axisId )->setLabelAlignment( alignment );
}

/*!
   Rotate all tick labels

   \param axisId Axis
   \param rotation Angle in degrees. When changing the label rotation,
                  the label alignment might be adjusted too.

   \sa QwtScaleDraw::setLabelRotation(), setAxisLabelAlignment()
 */
void QwtPlot::setAxisLabelRotation( QwtAxisId axisId, double rotation )
{
    if ( isAxisValid( axisId ) )
        axisWidget( axisId )->setLabelRotation( rotation );
}

/*!
   Set the maximum number of minor scale intervals for a specified axis

   \param axisId Axis
   \param maxMinor Maximum number of minor steps

   \sa axisMaxMinor()
 */
void QwtPlot::setAxisMaxMinor( QwtAxisId axisId, int maxMinor )
{
    if ( isAxisValid( axisId ) )
    {
        maxMinor = qBound( 0, maxMinor, 100 );

        AxisData& d = m_scaleData->axisData( axisId );
        if ( maxMinor != d.maxMinor )
        {
            d.maxMinor = maxMinor;
            d.isValid = false;
            autoRefresh();
        }
    }
}

/*!
   Set the maximum number of major scale intervals for a specified axis

   \param axisId Axis
   \param maxMajor Maximum number of major steps

   \sa axisMaxMajor()
 */
void QwtPlot::setAxisMaxMajor( QwtAxisId axisId, int maxMajor )
{
    if ( isAxisValid( axisId ) )
    {
        maxMajor = qBound( 1, maxMajor, 10000 );

        AxisData& d = m_scaleData->axisData( axisId );
        if ( maxMajor != d.maxMajor )
        {
            d.maxMajor = maxMajor;
            d.isValid = false;
            autoRefresh();
        }
    }
}

/*!
   \brief Change the title of a specified axis

   \param axisId Axis
   \param title axis title
 */
void QwtPlot::setAxisTitle( QwtAxisId axisId, const QString& title )
{
    if ( isAxisValid( axisId ) )
        axisWidget( axisId )->setTitle( title );
}

/*!
   \brief Change the title of a specified axis

   \param axisId Axis
   \param title Axis title
 */
void QwtPlot::setAxisTitle( QwtAxisId axisId, const QwtText& title )
{
    if ( isAxisValid( axisId ) )
        axisWidget( axisId )->setTitle( title );
}

/*!
   \brief Rebuild the axes scales

   In case of autoscaling the boundaries of a scale are calculated
   from the bounding rectangles of all plot items, having the
   QwtPlotItem::AutoScale flag enabled ( QwtScaleEngine::autoScale() ).
   Then a scale division is calculated ( QwtScaleEngine::didvideScale() )
   and assigned to scale widget.

   When the scale boundaries have been assigned with setAxisScale() a
   scale division is calculated ( QwtScaleEngine::didvideScale() )
   for this interval and assigned to the scale widget.

   When the scale has been set explicitly by setAxisScaleDiv() the
   locally stored scale division gets assigned to the scale widget.

   The scale widget indicates modifications by emitting a
   QwtScaleWidget::scaleDivChanged() signal.

   updateAxes() is usually called by replot().

   \sa setAxisAutoScale(), setAxisScale(), setAxisScaleDiv(), replot()
      QwtPlotItem::boundingRect()
 */
void QwtPlot::updateAxes()
{
    // Find bounding interval of the item data
    // for all axes, where autoscaling is enabled

    QVector< QwtInterval > boundingIntervals[QwtAxis::AxisPositions];
    for ( int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++ )
        boundingIntervals[axisPos].resize( axesCount( axisPos ) );

    const QwtPlotItemList& itmList = itemList();

    QwtPlotItemIterator it;
    for ( it = itmList.begin(); it != itmList.end(); ++it )
    {
        const QwtPlotItem* item = *it;

        if ( !item->testItemAttribute( QwtPlotItem::AutoScale ) )
            continue;

        if ( !item->isVisible() )
            continue;

        if ( axisAutoScale( item->xAxis() ) || axisAutoScale( item->yAxis() ) )
        {
            const QRectF rect = item->boundingRect();

            if ( rect.width() >= 0.0 )
            {
                const QwtAxisId xAxis = item->xAxis();
                boundingIntervals[ xAxis.pos ][ xAxis.id ] |=
                    QwtInterval( rect.left(), rect.right() );
            }

            if ( rect.height() >= 0.0 )
            {
                const QwtAxisId yAxis = item->yAxis();
                boundingIntervals[ yAxis.pos ][ yAxis.id ] |=
                    QwtInterval( rect.top(), rect.bottom() );
            }
        }
    }

    // Adjust scales

    for ( int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++ )
    {
        for ( int i = 0; i < m_scaleData->axesCount( axisPos ); i++ )
        {
            const QwtAxisId axisId( axisPos, i );

            AxisData& d = m_scaleData->axisData( axisId );

            double minValue = d.minValue;
            double maxValue = d.maxValue;
            double stepSize = d.stepSize;

            const QwtInterval& interval = boundingIntervals[axisPos][i];

            if ( d.doAutoScale && interval.isValid() )
            {
                d.isValid = false;

                minValue = interval.minValue();
                maxValue = interval.maxValue();

                d.scaleEngine->autoScale( d.maxMajor,
                    minValue, maxValue, stepSize );
            }
            if ( !d.isValid )
            {
                d.scaleDiv = d.scaleEngine->divideScale(
                    minValue, maxValue, d.maxMajor, d.maxMinor, stepSize );
                d.isValid = true;
            }

            QwtScaleWidget* scaleWidget = axisWidget( axisId );
            scaleWidget->setScaleDiv( d.scaleDiv );

            int startDist, endDist;
            scaleWidget->getBorderDistHint( startDist, endDist );
            scaleWidget->setBorderDist( startDist, endDist );
        }
    }

    for ( it = itmList.begin(); it != itmList.end(); ++it )
    {
        QwtPlotItem* item = *it;
        if ( item->testItemInterest( QwtPlotItem::ScaleInterest ) )
        {
            item->updateScaleDiv( axisScaleDiv( item->xAxis() ),
                axisScaleDiv( item->yAxis() ) );
        }
    }
}

