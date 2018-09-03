/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot_layout.h"
#include "qwt_text.h"
#include "qwt_text_label.h"
#include "qwt_scale_widget.h"
#include "qwt_abstract_legend.h"
#include <qscrollbar.h>
#include <qmath.h>

class QwtPlotLayoutData
{
public:
    struct LegendData
    {
        void init( const QwtAbstractLegend *legend )
        {
            if ( legend )
            {
                frameWidth = legend->frameWidth();
                hScrollExtent = legend->scrollExtent( Qt::Horizontal );
                vScrollExtent = legend->scrollExtent( Qt::Vertical );

                hint = legend->sizeHint();
            }
        }

        QSize legendHint( const QwtAbstractLegend *legend, const QRectF &rect ) const
        {
            int w = qMin( hint.width(), qFloor( rect.width() ) );

            int h = legend->heightForWidth( w );
            if ( h <= 0 )
                h = hint.height();

            if ( h > rect.height() )
                w += hScrollExtent;

            return QSize( w, h );
        }

        int frameWidth;
        int hScrollExtent;
        int vScrollExtent;
        QSize hint;
    };

    struct LabelData
    {
        void init( const QwtTextLabel *label )
        {
            frameWidth = 0;
            text = QwtText();

            if ( label )
            {
                text = label->text();
                if ( !( text.testPaintAttribute( QwtText::PaintUsingTextFont ) ) )
                    text.setFont( label->font() );
        
                frameWidth = label->frameWidth();
            }
        }

        QwtText text;
        int frameWidth;
    };

    struct ScaleData
    {
        void init( const QwtScaleWidget *axisWidget )
        {
            if ( axisWidget )
            {
                isVisible = true;

                scaleWidget = axisWidget;
                scaleFont = axisWidget->font();

                start = axisWidget->startBorderDist();
                end = axisWidget->endBorderDist();

                baseLineOffset = axisWidget->margin();

                dimWithoutTitle = axisWidget->dimForLength(
                    QWIDGETSIZE_MAX, scaleFont );

                if ( !axisWidget->title().isEmpty() )
                {
                    dimWithoutTitle -=
                        axisWidget->titleHeightForWidth( QWIDGETSIZE_MAX );
                }
            }
            else
            {
                isVisible = false;
                axisWidget = NULL;
                start = 0;
                end = 0;
                baseLineOffset = 0;
                dimWithoutTitle = 0;
            }

        }

        bool isVisible;
        const QwtScaleWidget *scaleWidget;
        QFont scaleFont;
        int start;
        int end;
        int baseLineOffset;
        int dimWithoutTitle;
    };

    struct CanvasData
    {
        void init( const QWidget *canvas )
        {
            canvas->getContentsMargins(
                &contentsMargins[ QwtAxis::yLeft ],
                &contentsMargins[ QwtAxis::xTop ],
                &contentsMargins[ QwtAxis::yRight ],
                &contentsMargins[ QwtAxis::xBottom ] );
        }

        int contentsMargins[ QwtAxis::PosCount ];
    };

public:
    enum Label
    {
        Title,
        Footer,

        NumLabels
    };

    QwtPlotLayoutData( const QwtPlot * );
    bool hasSymmetricYAxes() const;

    int numAxes( int axisPos ) const
    {
        return scaleData[ axisPos ].size();
    }

    ScaleData &axisData( QwtAxisId axisId )
    {
        return scaleData[ axisId.pos ][ axisId.id ];
    }

    const ScaleData &axisData( QwtAxisId axisId ) const
    {
        return scaleData[ axisId.pos ][ axisId.id ];
    }

    LegendData legendData;
    LabelData labelData[ NumLabels ];
    QVector<ScaleData> scaleData[ QwtAxis::PosCount ];
    CanvasData canvasData;

    double tickOffset[ QwtAxis::PosCount ];
    int numVisibleScales[ QwtAxis::PosCount ];
};

/*
  Extract all layout relevant data from the plot components
*/
QwtPlotLayoutData::QwtPlotLayoutData( const QwtPlot *plot )
{
    legendData.init( plot->legend() );
    labelData[ Title ].init( plot->titleLabel() );
    labelData[ Footer ].init( plot->footerLabel() );

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        numVisibleScales[ axisPos ] = 0;

        const int axesCount = plot->axesCount( axisPos );
        scaleData[ axisPos ].resize( axesCount );
        tickOffset[ axisPos ] = 0;

        for ( int i = 0; i < axesCount; i++ )
        {
            const QwtAxisId axisId( axisPos, i );

            ScaleData &sclData = axisData( axisId );

            if ( plot->isAxisVisible( axisId ) )
            {
                const QwtScaleWidget *axisWidget = plot->axisWidget( axisId );

                if ( numVisibleScales[ axisPos ] == 0 )
                {
                    tickOffset[ axisPos ] = axisWidget->margin();

                    const QwtScaleDraw *scaleDraw = axisWidget->scaleDraw();
                    if ( scaleDraw->hasComponent( QwtAbstractScaleDraw::Ticks ) )
                    {
                        tickOffset[ axisPos ] += scaleDraw->maxTickLength();
                    }
                }

                numVisibleScales[ axisPos ]++;
                sclData.init( axisWidget );
            }
            else
            {
                sclData.init( NULL );
            }
        }
    }

    canvasData.init( plot->canvas() );
}

bool QwtPlotLayoutData::hasSymmetricYAxes() const
{
    return numVisibleScales[ QwtAxis::yLeft ] == 
        numVisibleScales[ QwtAxis::yRight ];
}

class QwtPlotLayoutHintData
{
public:
    QwtPlotLayoutHintData( const QwtPlot *plot );

    bool hasSymmetricYAxes() const
    {
        return numVisibleScales[ QwtAxis::yLeft ] ==
            numVisibleScales[ QwtAxis::yRight ];
    }

    int yAxesWidth() const 
    { 
        return m_axesSize[ QwtAxis::yLeft ].width() +
            m_axesSize[ QwtAxis::yRight ].width();
    }

    int yAxesHeight() const 
    { 
        return qMax( m_axesSize[ QwtAxis::yLeft ].height(),
            m_axesSize[ QwtAxis::yRight ].height() );
    }

    int xAxesHeight() const 
    { 
        return m_axesSize[ QwtAxis::xTop ].height() +
            m_axesSize[ QwtAxis::xBottom ].height();
    }

    int xAxesWidth() const 
    { 
        return qMax( m_axesSize[ QwtAxis::xTop ].width(),
            m_axesSize[ QwtAxis::xBottom ].width() );
    }

    int alignedSize( const QwtAxisId ) const;

    struct ScaleData
    {
        ScaleData()
        {
            w = h = minLeft = minRight = 0;
        }

        int w;
        int h;
        int minLeft;
        int minRight;
    };

    int canvasBorder[QwtAxis::PosCount];
    int tickOffset[QwtAxis::PosCount];
    int numVisibleScales[ QwtAxis::PosCount ];

private:
    const ScaleData &axisData( QwtAxisId axisId ) const
    {
        return scaleData[ axisId.pos ][ axisId.id ];
    }

    ScaleData &axisData( QwtAxisId axisId ) 
    {
        return scaleData[ axisId.pos ][ axisId.id ];
    }

    int axesWidth( int axisPos ) const
    {
        return m_axesSize[axisPos].width();
    }

    int axesHeight( int axisPos ) const
    {
        return m_axesSize[axisPos].height();
    }

    QSize m_axesSize[ QwtAxis::PosCount ];

    QVector<ScaleData> scaleData[ QwtAxis::PosCount ];
};

QwtPlotLayoutHintData::QwtPlotLayoutHintData( const QwtPlot *plot )
{
    int contentsMargins[ 4 ];

    plot->canvas()->getContentsMargins(
        &contentsMargins[ QwtAxis::yLeft ],
        &contentsMargins[ QwtAxis::xTop ],
        &contentsMargins[ QwtAxis::yRight ],
        &contentsMargins[ QwtAxis::xBottom ] );

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        QSize &axesSize = m_axesSize[ axisPos ];
        axesSize = QSize( 0, 0 );

        canvasBorder[ axisPos ] = contentsMargins[axisPos] + 
            plot->plotLayout()->canvasMargin( axisPos ) + 1;

        numVisibleScales[ axisPos ] = 0;
        tickOffset[ axisPos ] = 0;

        const int axesCount = plot->axesCount( axisPos );
        scaleData[ axisPos ].resize( axesCount );

        for ( int i = 0; i < axesCount; i++ )
        {
            const QwtAxisId axisId( axisPos, i );

            if ( plot->isAxisVisible( axisId ) )
            {
                const QwtScaleWidget *scl = plot->axisWidget( axisId );

                const QSize hint = scl->minimumSizeHint();

                ScaleData &sd = axisData( axisId );
                sd.w = hint.width();
                sd.h = hint.height();
                scl->getBorderDistHint( sd.minLeft, sd.minRight );

                if ( numVisibleScales[axisPos] == 0 )
                {
                    tickOffset[axisPos] = scl->margin();
                    if ( scl->scaleDraw()->hasComponent( QwtAbstractScaleDraw::Ticks ) )
                        tickOffset[axisPos] += qCeil( scl->scaleDraw()->maxTickLength() );
                }

                numVisibleScales[axisPos]++;

                if ( QwtAxis::isYAxis( axisId.pos ) )
                {
                    axesSize.setWidth( axesSize.width() + sd.w );
                    axesSize.setHeight( qMax( axesSize.height(), sd.h ) );
                }
                else
                {
                    axesSize.setHeight( axesSize.height() + sd.h );
                    axesSize.setWidth( qMax( axesSize.width(), sd.w ) );
                }
            }
        }

        if ( axesCount > 1 )
        {
            // The width of the y axes and the height of the x axes depends
            // on the line breaks in the scale title. So after knowning the 
            // bounding height/width we might have some to subtract some line
            // breaks, we don't have anymore.

            for ( int i = 0; i < axesCount; i++ )
            {
                QSize &axesSize = m_axesSize[ axisPos ];

                const QwtAxisId axisId( axisPos, i );

                if ( plot->isAxisVisible( axisId ) )
                {
                    const QwtScaleWidget *scl = plot->axisWidget( axisId );
                    ScaleData &sd = axisData( axisId );

                    if ( QwtAxis::isYAxis( axisId.pos ) )
                    {
                        int off = scl->titleHeightForWidth( sd.h ) -
                            scl->titleHeightForWidth( yAxesHeight() );

                        axesSize.setWidth( axesSize.width() - off );
                    }
                    else
                    {
                        int off = scl->titleHeightForWidth( sd.w ) -
                            scl->titleHeightForWidth( xAxesWidth() );

                        axesSize.setHeight( axesSize.height() - off );
                    }
                }
            }
        }
    }
}

int QwtPlotLayoutHintData::alignedSize( const QwtAxisId axisId ) const
{
    const QwtPlotLayoutHintData::ScaleData &sd = axisData( axisId );
    if ( QwtAxis::isXAxis( axisId.pos ) && sd.w )
    {
        int w = sd.w;
        const int leftW = m_axesSize[QwtAxis::yLeft].width(); 
        const int rightW = m_axesSize[QwtAxis::yRight].width(); 

        const int shiftLeft = sd.minLeft - canvasBorder[QwtAxis::yLeft];
        if ( shiftLeft > 0 && leftW )
            w -= qMin( shiftLeft, leftW );

        const int shiftRight = sd.minRight - canvasBorder[QwtAxis::yRight];
        if ( shiftRight > 0 && rightW )
            w -= qMin( shiftRight, rightW );

        return w;
    }
    if ( QwtAxis::isYAxis( axisId.pos ) && sd.h )
    {
        int h = sd.h;

        const int topH = m_axesSize[QwtAxis::xTop].height(); 
        const int bottomH = m_axesSize[QwtAxis::xBottom].height(); 

        const int shiftBottom = sd.minLeft - canvasBorder[QwtAxis::xBottom];
        if ( shiftBottom > 0 && bottomH )
            h -= qMin( shiftBottom, tickOffset[QwtAxis::xBottom] );

        const int shiftTop = sd.minRight - canvasBorder[QwtAxis::xTop];
        if ( shiftTop > 0 && topH )
            h -= qMin( shiftTop, tickOffset[QwtAxis::xTop] );

        return h;
    }

    return 0;
}

class QwtPlotLayoutEngine
{
public:
    class Dimensions
    {
    public:
        Dimensions( const QwtPlotLayoutData& layoutData )
        {
            dimTitle = dimFooter = 0;
            for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
            {
                dimAxisVector[axisPos] = 
                    QVector<int>( layoutData.numAxes( axisPos ), 0 );
            }
        }

        int dimAxis( QwtAxisId axisId ) const
        {
            return dimAxisVector[ axisId.pos ].at( axisId.id );
        }

        void setDimAxis( QwtAxisId axisId, int dim )
        {
            dimAxisVector[ axisId.pos ][ axisId.id ] = dim;
        }

        inline int dimAxes( int axisPos ) const 
        {
            const QVector<int> &dims = dimAxisVector[ axisPos ];

            int dim = 0;
            for ( int i = 0; i < dims.size(); i++ )
                dim += dims[i];

            return dim;
        }

        inline int dimYAxes() const 
        {
            return dimAxes( QwtAxis::yLeft ) + dimAxes( QwtAxis::yRight );
        }
        
        inline int dimXAxes() const 
        {
            return dimAxes( QwtAxis::xTop ) + dimAxes( QwtAxis::xBottom );
        }

        inline QRectF centered( const QRectF &rect, const QRectF &labelRect ) const
        {
            QRectF r = labelRect;
            r.setX( rect.left() + dimAxes( QwtAxis::yLeft ) );
            r.setWidth( rect.width() - dimYAxes() );

            return r;
        }

        inline QRectF innerRect( const QRectF &rect ) const
        {
            QRectF r( 
                rect.x() + dimAxes( QwtAxis::yLeft ),
                rect.y() + dimAxes( QwtAxis::xTop ), 
                rect.width() - dimYAxes(),
                rect.height() - dimXAxes() );

            if ( r.width() < 0 )
            {
                r.setX( rect.center().x() );
                r.setWidth( 0 );
            }
            if ( r.height() < 0 )
            {
                r.setY( rect.center().x() );
                r.setHeight( 0 );
            }

            return r;
        }

        int dimTitle;
        int dimFooter;

    private:
        QVector<int> dimAxisVector[QwtAxis::PosCount];
    };

    QwtPlotLayoutEngine():
        d_spacing( 5 )
    {
    }

    QRectF layoutLegend( QwtPlotLayout::Options, 
        const QwtPlotLayoutData::LegendData &, const QRectF &, const QSize & ) const;

    QRectF alignLegend( const QSize &legendHint,
        const QRectF &canvasRect, const QRectF &legendRect ) const;

    void alignScales( QwtPlotLayout::Options, const QwtPlotLayoutData &,
        QRectF &canvasRect, 
        QVector<QRectF> scaleRect[QwtAxis::PosCount] ) const;

    Dimensions layoutDimensions( QwtPlotLayout::Options, 
        const QwtPlotLayoutData &, const QRectF &rect ) const;

    inline void setSpacing( int spacing ) { d_spacing = spacing; }
    inline int spacing() const { return d_spacing; }

    inline void setAlignCanvas( int axisPos, bool on ) { d_alignCanvas[ axisPos ] = on; }
    inline bool alignCanvas( int axisPos ) const { return d_alignCanvas[ axisPos ]; }

    inline void setCanvasMargin( int axisPos, int margin ) { d_canvasMargin[ axisPos ] = margin; }
    inline int canvasMargin( int axisPos ) const { return d_canvasMargin[ axisPos ]; }

    inline void setLegendPos( QwtPlot::LegendPosition pos ) { d_legendPos = pos; }
    inline QwtPlot::LegendPosition legendPos() const { return d_legendPos; }

    inline void setLegendRatio( double ratio ) { d_legendRatio = ratio; }
    inline double legendRatio() const { return d_legendRatio; }

private:
    int heightForWidth( QwtPlotLayoutData::Label, const QwtPlotLayoutData &, 
        QwtPlotLayout::Options, double width, int axesWidth ) const;
    
    QwtPlot::LegendPosition d_legendPos;
    double d_legendRatio;

    int d_canvasMargin[QwtAxis::PosCount];
    bool d_alignCanvas[QwtAxis::PosCount];

    int d_spacing;
};

QRectF QwtPlotLayoutEngine::layoutLegend( QwtPlotLayout::Options options,
    const QwtPlotLayoutData::LegendData &legendData, 
    const QRectF &rect, const QSize &legendHint ) const
{
    int dim;
    if ( d_legendPos == QwtPlot::LeftLegend
        || d_legendPos == QwtPlot::RightLegend )
    {
        // We don't allow vertical legends to take more than
        // half of the available space.

        dim = qMin( legendHint.width(), int( rect.width() * d_legendRatio ) );

        if ( !( options & QwtPlotLayout::IgnoreScrollbars ) )
        {
            if ( legendHint.height() > rect.height() )
            {
                // The legend will need additional
                // space for the vertical scrollbar.

                dim += legendData.hScrollExtent;
            }
        }
    }
    else
    {
        dim = qMin( legendHint.height(), int( rect.height() * d_legendRatio ) );
        dim = qMax( dim, legendData.vScrollExtent );
    }

    QRectF legendRect = rect;
    switch ( d_legendPos )
    {
        case QwtPlot::LeftLegend:
        {
            legendRect.setWidth( dim );
            break;
        }
        case QwtPlot::RightLegend:
        {
            legendRect.setX( rect.right() - dim );
            legendRect.setWidth( dim );
            break;
        }
        case QwtPlot::TopLegend:
        {
            legendRect.setHeight( dim );
            break;
        }
        case QwtPlot::BottomLegend:
        {
            legendRect.setY( rect.bottom() - dim );
            legendRect.setHeight( dim );
            break;
        }
    }

    return legendRect;
}

QRectF QwtPlotLayoutEngine::alignLegend( const QSize &legendHint,
    const QRectF &canvasRect, const QRectF &legendRect ) const
{
    QRectF alignedRect = legendRect;

    if ( d_legendPos == QwtPlot::BottomLegend
        || d_legendPos == QwtPlot::TopLegend )
    {
        if ( legendHint.width() < canvasRect.width() )
        {
            alignedRect.setX( canvasRect.x() );
            alignedRect.setWidth( canvasRect.width() );
        }
    }
    else
    {
        if ( legendHint.height() < canvasRect.height() )
        {
            alignedRect.setY( canvasRect.y() );
            alignedRect.setHeight( canvasRect.height() );
        }
    }

    return alignedRect;
}

int QwtPlotLayoutEngine::heightForWidth( 
    QwtPlotLayoutData::Label labelType, const QwtPlotLayoutData &layoutData, 
    QwtPlotLayout::Options options,
    double width, int axesWidth ) const
{
    const QwtPlotLayoutData::LabelData &labelData = layoutData.labelData[ labelType ];

    if ( labelData.text.isEmpty() )
        return 0;

    double w = width;

    if ( !layoutData.hasSymmetricYAxes() )
    {
        // center to the canvas
        w -= axesWidth;
    }

    int d = qCeil( labelData.text.heightForWidth( w ) );
    if ( !( options & QwtPlotLayout::IgnoreFrames ) )
        d += 2 * labelData.frameWidth;

    return d;
}

QwtPlotLayoutEngine::Dimensions
QwtPlotLayoutEngine::layoutDimensions( QwtPlotLayout::Options options, 
    const QwtPlotLayoutData &layoutData, const QRectF &rect ) const
{
    Dimensions dimensions( layoutData );

    int backboneOffset[QwtAxis::PosCount];
    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        backboneOffset[ axisPos ] = 0;
        if ( !( options & QwtPlotLayout::IgnoreFrames ) )
            backboneOffset[ axisPos ] += layoutData.canvasData.contentsMargins[ axisPos ];

        if ( !d_alignCanvas[ axisPos ] )
            backboneOffset[ axisPos ] += d_canvasMargin[ axisPos ];
    }

    bool done = false;
    while ( !done )
    {
        done = true;

        // the size for the 4 axis depend on each other. Expanding
        // the height of a horizontal axis will shrink the height
        // for the vertical axis, shrinking the height of a vertical
        // axis will result in a line break what will expand the
        // width and results in shrinking the width of a horizontal
        // axis what might result in a line break of a horizontal
        // axis ... . So we loop as long until no size changes.

        if ( !( options & QwtPlotLayout::IgnoreTitle ) )
        {
            const int d = heightForWidth( QwtPlotLayoutData::Title, layoutData, options,
                rect.width(), dimensions.dimYAxes() );

            if ( d > dimensions.dimTitle )
            {
                dimensions.dimTitle = d;
                done = false;
            }
        }

        if ( !( options & QwtPlotLayout::IgnoreFooter ) ) 
        {
            const int d = heightForWidth( QwtPlotLayoutData::Footer, layoutData, options,
                rect.width(), dimensions.dimYAxes() );

            if ( d > dimensions.dimFooter )
            {
                dimensions.dimFooter = d;
                done = false;
            }
        }

        for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
        {
            for ( int i = 0; i < layoutData.numAxes( axisPos ); i++ )
            {
                const QwtAxisId axisId( axisPos, i );

                const struct QwtPlotLayoutData::ScaleData &scaleData = layoutData.axisData( axisId );

                if ( scaleData.isVisible )
                {
                    double length;
                    if ( QwtAxis::isXAxis( axisPos ) )
                    {
                        length = rect.width() - dimensions.dimYAxes();
                        length -= scaleData.start + scaleData.end;

                        if ( dimensions.dimAxes( QwtAxis::yRight ) > 0 )
                            length -= 1;

                        length += qMin( dimensions.dimAxes( QwtAxis::yLeft ),
                            scaleData.start - backboneOffset[QwtAxis::yLeft] );

                        length += qMin( dimensions.dimAxes( QwtAxis::yRight ),
                            scaleData.end - backboneOffset[QwtAxis::yRight] );
                    }
                    else // y axis
                    {
                        length = rect.height() - dimensions.dimXAxes();
                        length -= scaleData.start + scaleData.end;
                        length -= 1;

                        if ( dimensions.dimAxes( QwtAxis::xBottom ) <= 0 )
                            length -= 1;

                        if ( dimensions.dimAxes( QwtAxis::xTop ) <= 0 )
                            length -= 1;

                        /*
                          The tick labels of the y axes are always left/right from the 
                          backbone/ticks of the x axes - but we have to take care,
                          that te labels don't overlap.
                         */
                        if ( dimensions.dimAxes( QwtAxis::xBottom ) > 0 )
                        {
                            length += qMin(
                                layoutData.tickOffset[QwtAxis::xBottom],
                                double( scaleData.start - backboneOffset[QwtAxis::xBottom] ) );
                        }
                        if ( dimensions.dimAxes( QwtAxis::xTop ) > 0 )
                        {
                            length += qMin(
                                layoutData.tickOffset[QwtAxis::xTop],
                                double( scaleData.end - backboneOffset[QwtAxis::xTop] ) );
                        }

                        if ( dimensions.dimTitle > 0 )
                            length -= dimensions.dimTitle + d_spacing;
                    }

                    int d = scaleData.dimWithoutTitle;
                    if ( !scaleData.scaleWidget->title().isEmpty() )
                    {
                        d += scaleData.scaleWidget->titleHeightForWidth( qFloor( length ) );
                    }


                    if ( d > dimensions.dimAxis( axisId ) )
                    {
                        dimensions.setDimAxis( axisId, d );
                        done = false;
                    }
                }
            }
        }
    }

    return dimensions;
}

void QwtPlotLayoutEngine::alignScales( QwtPlotLayout::Options options,
    const QwtPlotLayoutData &layoutData, QRectF &canvasRect, 
    QVector<QRectF> scaleRect[QwtAxis::PosCount] ) const
{
    int backboneOffset[QwtAxis::PosCount];
    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        backboneOffset[ axisPos ] = 0;

        if ( !d_alignCanvas[ axisPos ] )
        {
            backboneOffset[ axisPos ] += d_canvasMargin[ axisPos ];
        }

        if ( !( options & QwtPlotLayout::IgnoreFrames ) )
        {
            backboneOffset[ axisPos ] += 
                layoutData.canvasData.contentsMargins[ axisPos ];
        }
    }

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        for ( int i = 0; i < layoutData.numAxes( axisPos ); i++ )
        {
            QRectF &axisRect = scaleRect[ axisPos ][ i ];
            if ( !axisRect.isValid() )
                continue;

            const QwtAxisId axisId( axisPos, i );

            const int startDist = layoutData.axisData( axisId ).start;
            const int endDist = layoutData.axisData( axisId ).end;

            if ( QwtAxis::isXAxis( axisPos ) )
            {
                const QRectF &leftScaleRect = scaleRect[QwtAxis::yLeft][ QWT_DUMMY_ID ];
                const int leftOffset = backboneOffset[QwtAxis::yLeft] - startDist;

                if ( leftScaleRect.isValid() )
                {
                    const double dx = leftOffset + leftScaleRect.width();
                    if ( d_alignCanvas[QwtAxis::yLeft] && dx < 0.0 )
                    {
                        /*
                          The axis needs more space than the width
                          of the left scale.
                         */
                        const double cLeft = canvasRect.left(); // qreal -> double
                        canvasRect.setLeft( qMax( cLeft, axisRect.left() - dx ) );
                    }
                    else
                    {
                        const double minLeft = leftScaleRect.left();
                        const double left = axisRect.left() + leftOffset;
                        axisRect.setLeft( qMax( left, minLeft ) );
                    }
                }
                else
                {
                    if ( d_alignCanvas[QwtAxis::yLeft] && leftOffset < 0 )
                    {
                        canvasRect.setLeft( qMax( canvasRect.left(),
                            axisRect.left() - leftOffset ) );
                    }
                    else
                    {
                        if ( leftOffset > 0 )
                            axisRect.setLeft( axisRect.left() + leftOffset );
                    }
                }

                const QRectF &rightScaleRect = scaleRect[QwtAxis::yRight][ QWT_DUMMY_ID ];
                const int rightOffset =
                    backboneOffset[QwtAxis::yRight] - endDist + 1;

                if ( rightScaleRect.isValid() )
                {
                    const double dx = rightOffset + rightScaleRect.width();
                    if ( d_alignCanvas[QwtAxis::yRight] && dx < 0 )
                    {
                        /*
                          The axis needs more space than the width
                          of the right scale.
                         */
                        const double cRight = canvasRect.right(); // qreal -> double
                        canvasRect.setRight( qMin( cRight, axisRect.right() + dx ) );
                    }   

                    const double maxRight = rightScaleRect.right();
                    const double right = axisRect.right() - rightOffset;
                    axisRect.setRight( qMin( right, maxRight ) );
                }
                else
                {
                    if ( d_alignCanvas[QwtAxis::yRight] && rightOffset < 0 )
                    {
                        canvasRect.setRight( qMin( canvasRect.right(),
                            axisRect.right() + rightOffset ) );
                    }
                    else
                    {
                        if ( rightOffset > 0 )
                            axisRect.setRight( axisRect.right() - rightOffset );
                    }
                }
            }
            else // y axes
            {
                const QRectF &bottomScaleRect = scaleRect[QwtAxis::xBottom][ QWT_DUMMY_ID ];
                const int bottomOffset =
                    backboneOffset[QwtAxis::xBottom] - endDist + 1;

                if ( bottomScaleRect.isValid() )
                {
                    const double dy = bottomOffset + bottomScaleRect.height();
                    if ( d_alignCanvas[QwtAxis::xBottom] && dy < 0 )
                    {
                        /*
                          The axis needs more space than the height
                          of the bottom scale.
                         */
                        const double cBottom = canvasRect.bottom(); // qreal -> double
                        canvasRect.setBottom( qMin( cBottom, axisRect.bottom() + dy ) );
                    }
                    else
                    {
                        const double maxBottom = bottomScaleRect.top() +
                            layoutData.tickOffset[QwtAxis::xBottom];
                        const double bottom = axisRect.bottom() - bottomOffset;
                        axisRect.setBottom( qMin( bottom, maxBottom ) );
                    }
                }
                else
                {
                    if ( d_alignCanvas[QwtAxis::xBottom] && bottomOffset < 0 )
                    {
                        canvasRect.setBottom( qMin( canvasRect.bottom(),
                            axisRect.bottom() + bottomOffset ) );
                    }
                    else
                    {
                        if ( bottomOffset > 0 )
                            axisRect.setBottom( axisRect.bottom() - bottomOffset );
                    }
                }

                const QRectF &topScaleRect = scaleRect[QwtAxis::xTop][ QWT_DUMMY_ID ];
                const int topOffset = backboneOffset[QwtAxis::xTop] - startDist;

                if ( topScaleRect.isValid() )
                {
                    const double dy = topOffset + topScaleRect.height();
                    if ( d_alignCanvas[QwtAxis::xTop] && dy < 0 )
                    {
                        /*
                          The axis needs more space than the height
                          of the top scale.
                         */
                        const double cTop = canvasRect.top(); // qreal -> double
                        canvasRect.setTop( qMax( cTop, axisRect.top() - dy ) );
                    }
                    else
                    {
                        const double minTop = topScaleRect.bottom() -
                            layoutData.tickOffset[QwtAxis::xTop];
                        const double top = axisRect.top() + topOffset;
                        axisRect.setTop( qMax( top, minTop ) );
                    }
                }
                else
                {
                    if ( d_alignCanvas[QwtAxis::xTop] && topOffset < 0 )
                    {
                        canvasRect.setTop( qMax( canvasRect.top(),
                            axisRect.top() - topOffset ) );
                    }
                    else
                    {
                        if ( topOffset > 0 )
                            axisRect.setTop( axisRect.top() + topOffset );
                    }
                }
            }
        }
    }

    /*
      The canvas has been aligned to the scale with largest
      border distances. Now we have to realign the other scale.
     */

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        for ( int i = 0; i < layoutData.numAxes( axisPos ); i++ )
        {
            const QwtAxisId axisId( axisPos, i );

            QRectF &sRect = scaleRect[ axisId.pos ][ axisId.id ];
            const QwtPlotLayoutData::ScaleData &axisData = layoutData.axisData( axisId );

            if ( !sRect.isValid() )
                continue;

            if ( QwtAxis::isXAxis( axisId.pos ) )
            {
                if ( d_alignCanvas[QwtAxis::yLeft] )
                {
                    double y = canvasRect.left() - axisData.start;
                    if ( !( options & QwtPlotLayout::IgnoreFrames ) )
                        y += layoutData.canvasData.contentsMargins[ QwtAxis::yLeft ];

                    sRect.setLeft( y );
                }
                if ( d_alignCanvas[QwtAxis::yRight] )
                {
                    double y = canvasRect.right() - 1 + axisData.end;
                    if ( !( options & QwtPlotLayout::IgnoreFrames ) )
                        y -= layoutData.canvasData.contentsMargins[ QwtAxis::yRight ];

                    sRect.setRight( y );
                }

                if ( d_alignCanvas[ axisId.pos ] )
                {
                    if ( axisId.pos == QwtAxis::xTop )
                        sRect.setBottom( canvasRect.top() );
                    else
                        sRect.setTop( canvasRect.bottom() );
                }
            }
            else
            {
                if ( d_alignCanvas[QwtAxis::xTop] )
                {
                    double x = canvasRect.top() - axisData.start;
                    if ( !( options & QwtPlotLayout::IgnoreFrames ) )
                        x += layoutData.canvasData.contentsMargins[ QwtAxis::xTop ];

                    sRect.setTop( x );
                }
                if ( d_alignCanvas[QwtAxis::xBottom] )
                {
                    double x = canvasRect.bottom() - 1 + axisData.end;
                    if ( !( options & QwtPlotLayout::IgnoreFrames ) )
                        x -= layoutData.canvasData.contentsMargins[ QwtAxis::xBottom ];

                    sRect.setBottom( x );
                }

                if ( d_alignCanvas[ axisId.pos ] )
                {
                    if ( axisId.pos == QwtAxis::yLeft )
                        sRect.setRight( canvasRect.left() );
                    else
                        sRect.setLeft( canvasRect.right() );
                }
            }
        }
    }
}

class QwtPlotLayout::PrivateData
{
public:
    QRectF titleRect;
    QRectF footerRect;
    QRectF legendRect;
    QVector<QRectF> scaleRects[QwtAxis::PosCount];
    QRectF canvasRect;

    QwtPlotLayoutEngine layoutEngine;
};

/*!
  \brief Constructor
 */

QwtPlotLayout::QwtPlotLayout()
{
    d_data = new PrivateData;

    setLegendPosition( QwtPlot::BottomLegend );
    setCanvasMargin( 4 );
    setAlignCanvasToScales( false );

    invalidate();
}

//! Destructor
QwtPlotLayout::~QwtPlotLayout()
{
    delete d_data;
}

/*!
  Change a margin of the canvas. The margin is the space
  above/below the scale ticks. A negative margin will
  be set to -1, excluding the borders of the scales.

  \param margin New margin
  \param axisPos One of QwtAxis::Position. Specifies where the position of the margin.
              -1 means margin at all borders.
  \sa canvasMargin()

  \warning The margin will have no effect when alignCanvasToScale() is true
*/

void QwtPlotLayout::setCanvasMargin( int margin, int axisPos )
{
    if ( margin < -1 )
        margin = -1;

    if ( axisPos == -1 )
    {
        for ( axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
            d_data->layoutEngine.setCanvasMargin( axisPos, margin );
    }
    else if ( QwtAxis::isValid( axisPos ) )
    {
        d_data->layoutEngine.setCanvasMargin( axisPos, margin );
    }
}

/*!
    \param axisPos Axis position
    \return Margin around the scale tick borders
    \sa setCanvasMargin()
*/
int QwtPlotLayout::canvasMargin( int axisPos ) const
{
    if ( !QwtAxis::isValid( axisPos ) )
        return 0;

    return d_data->layoutEngine.canvasMargin( axisPos );
}

/*!
  \brief Set the align-canvas-to-axis-scales flag for all axes

  \param on True/False
  \sa setAlignCanvasToScale(), alignCanvasToScale()
*/
void QwtPlotLayout::setAlignCanvasToScales( bool on )
{
    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
        d_data->layoutEngine.setAlignCanvas( axisPos, on );
}

/*!
  Change the align-canvas-to-axis-scales setting. The canvas may:

  - extend beyond the axis scale ends to maximize its size,
  - align with the axis scale ends to control its size.

  The axisId parameter is somehow confusing as it identifies a border
  of the plot and not the axes, that are aligned. F.e when QwtAxis::yLeft
  is set, the left end of the x-axes ( QwtAxis::xTop, QwtAxis::xBottom )
  is aligned.

  \param axisId Axis index
  \param on New align-canvas-to-axis-scales setting

  \sa setCanvasMargin(), alignCanvasToScale(), setAlignCanvasToScales()
  \warning In case of on == true canvasMargin() will have no effect
*/
void QwtPlotLayout::setAlignCanvasToScale( int axisPos, bool on )
{
    if ( QwtAxis::isValid( axisPos ) )
        d_data->layoutEngine.setAlignCanvas( axisPos, on );
}

/*!
  Return the align-canvas-to-axis-scales setting. The canvas may:
  - extend beyond the axis scale ends to maximize its size
  - align with the axis scale ends to control its size.

  \param axisId Axis index
  \return align-canvas-to-axis-scales setting
  \sa setAlignCanvasToScale(), setAlignCanvasToScale(), setCanvasMargin()
*/
bool QwtPlotLayout::alignCanvasToScale( int axisPos ) const
{
    if ( !QwtAxis::isValid( axisPos ) )
        return false;

    return d_data->layoutEngine.alignCanvas( axisPos );
}

/*!
  Change the spacing of the plot. The spacing is the distance
  between the plot components.

  \param spacing New spacing
  \sa setCanvasMargin(), spacing()
*/
void QwtPlotLayout::setSpacing( int spacing )
{
    d_data->layoutEngine.setSpacing( qMax( 0, spacing ) );
}

/*!
  \return Spacing
  \sa margin(), setSpacing()
*/
int QwtPlotLayout::spacing() const
{
    return d_data->layoutEngine.spacing();
}

/*!
  \brief Specify the position of the legend
  \param pos The legend's position.
  \param ratio Ratio between legend and the bounding rectangle
               of title, footer, canvas and axes. The legend will be shrunk
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.

  \sa QwtPlot::setLegendPosition()
*/

void QwtPlotLayout::setLegendPosition( QwtPlot::LegendPosition pos, double ratio )
{
    if ( ratio > 1.0 )
        ratio = 1.0;

    switch ( pos )
    {
        case QwtPlot::TopLegend:
        case QwtPlot::BottomLegend:
        {
            if ( ratio <= 0.0 )
                ratio = 0.33;

            d_data->layoutEngine.setLegendRatio( ratio );
            d_data->layoutEngine.setLegendPos( pos );
            break;
        }
        case QwtPlot::LeftLegend:
        case QwtPlot::RightLegend:
        {
            if ( ratio <= 0.0 )
                ratio = 0.5;

            d_data->layoutEngine.setLegendRatio( ratio );
            d_data->layoutEngine.setLegendPos( pos );
            break;
        }
        default:
            break;
    }
}

/*!
  \brief Specify the position of the legend
  \param pos The legend's position. Valid values are
      \c QwtPlot::LeftLegend, \c QwtPlot::RightLegend,
      \c QwtPlot::TopLegend, \c QwtPlot::BottomLegend.

  \sa QwtPlot::setLegendPosition()
*/
void QwtPlotLayout::setLegendPosition( QwtPlot::LegendPosition pos )
{
    setLegendPosition( pos, 0.0 );
}

/*!
  \return Position of the legend
  \sa setLegendPosition(), QwtPlot::setLegendPosition(),
      QwtPlot::legendPosition()
*/
QwtPlot::LegendPosition QwtPlotLayout::legendPosition() const
{
    return d_data->layoutEngine.legendPos();
}

/*!
  Specify the relative size of the legend in the plot
  \param ratio Ratio between legend and the bounding rectangle
               of title, footer, canvas and axes. The legend will be shrunk
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.
*/
void QwtPlotLayout::setLegendRatio( double ratio )
{
    setLegendPosition( legendPosition(), ratio );
}

/*!
  \return The relative size of the legend in the plot.
  \sa setLegendPosition()
*/
double QwtPlotLayout::legendRatio() const
{
    return d_data->layoutEngine.legendRatio();
}

/*!
  \brief Set the geometry for the title

  This method is intended to be used from derived layouts
  overloading activate()

  \sa titleRect(), activate()
 */
void QwtPlotLayout::setTitleRect( const QRectF &rect )
{
    d_data->titleRect = rect;
}

/*!
  \return Geometry for the title
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::titleRect() const
{
    return d_data->titleRect;
}

/*!
  \brief Set the geometry for the footer

  This method is intended to be used from derived layouts
  overloading activate()

  \sa footerRect(), activate()
 */
void QwtPlotLayout::setFooterRect( const QRectF &rect )
{
    d_data->footerRect = rect;
}

/*!
  \return Geometry for the footer
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::footerRect() const
{
    return d_data->footerRect;
}

/*!
  \brief Set the geometry for the legend

  This method is intended to be used from derived layouts
  overloading activate()

  \param rect Rectangle for the legend

  \sa legendRect(), activate()
 */
void QwtPlotLayout::setLegendRect( const QRectF &rect )
{
    d_data->legendRect = rect;
}

/*!
  \return Geometry for the legend
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::legendRect() const
{
    return d_data->legendRect;
}

/*!
  \brief Set the geometry for an axis

  This method is intended to be used from derived layouts
  overloading activate()

  \param axis Axis index
  \param rect Rectangle for the scale

  \sa scaleRect(), activate()
 */
void QwtPlotLayout::setScaleRect( QwtAxisId axisId, const QRectF &rect )
{
    if ( QwtAxis::isValid( axisId.pos ) )
    {
        QVector<QRectF> &scaleRects = d_data->scaleRects[ axisId.pos ];

        if ( axisId.id >= 0 && axisId.id < scaleRects.size() )
            scaleRects[axisId.id] = rect;
    }
}

/*!
  \param axis Axis index
  \return Geometry for the scale
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::scaleRect( QwtAxisId axisId ) const
{
    if ( QwtAxis::isValid( axisId.pos ) )
    {
        QVector<QRectF> &scaleRects = d_data->scaleRects[ axisId.pos ];
        if ( axisId.id >= 0 && axisId.id < scaleRects.size() )
            return scaleRects[axisId.id];
    }

    return QRectF();
}

/*!
  \brief Set the geometry for the canvas

  This method is intended to be used from derived layouts
  overloading activate()

  \sa canvasRect(), activate()
 */
void QwtPlotLayout::setCanvasRect( const QRectF &rect )
{
    d_data->canvasRect = rect;
}

/*!
  \return Geometry for the canvas
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::canvasRect() const
{
    return d_data->canvasRect;
}

/*!
  Invalidate the geometry of all components.
  \sa activate()
*/
void QwtPlotLayout::invalidate()
{
    d_data->titleRect = d_data->footerRect = 
        d_data->legendRect = d_data->canvasRect = QRectF();

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        QVector<QRectF> &scaleRects = d_data->scaleRects[axisPos];

        scaleRects.resize( 1 );
        scaleRects[0] = QRectF();
    }
}

/*!
  \return Minimum size hint
  \param plot Plot widget

  \sa QwtPlot::minimumSizeHint()
*/
QSize QwtPlotLayout::minimumSizeHint( const QwtPlot *plot ) const
{
    QwtPlotLayoutHintData hintData( plot );

    /*
      When having x and y axes, we try to use the empty 
      corners for the tick labels that are exceeding the
      scale backbones.
     */
    int xAxesWidth = 0;
    int yAxesHeight = 0;

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        for ( int i = 0; i < plot->axesCount( axisPos ); i++ )
        {
            const int sz = hintData.alignedSize( QwtAxisId( axisPos, i ) );

            if ( QwtAxis::isXAxis( axisPos ) )
                xAxesWidth = qMax( xAxesWidth, sz );
            else
                yAxesHeight = qMax( yAxesHeight, sz );
        }
    }

    const QWidget *canvas = plot->canvas();

    int left, top, right, bottom;
    canvas->getContentsMargins( &left, &top, &right, &bottom );

    const QSize minCanvasSize = canvas->minimumSize();

    int w = hintData.yAxesWidth();
    int cw = xAxesWidth + left + 1 + right + 1;
    w += qMax( cw, minCanvasSize.width() );

    int h = hintData.xAxesHeight();
    int ch = yAxesHeight + top + 1 + bottom + 1;
    h += qMax( ch, minCanvasSize.height() );

    const QwtTextLabel *labels[2];
    labels[0] = plot->titleLabel();
    labels[1] = plot->footerLabel();

    for ( int i = 0; i < 2; i++ )
    {
        const QwtTextLabel *label = labels[i];
        if ( label && !label->text().isEmpty() )
        {
            // we center on the plot canvas.
            const bool centerOnCanvas = hintData.hasSymmetricYAxes();

            int labelW = w;
            if ( centerOnCanvas )
            {
                labelW -= hintData.yAxesWidth();
            }

            int labelH = label->heightForWidth( labelW );
            if ( labelH > labelW ) // Compensate for a long title
            {
                w = labelW = labelH;
                if ( centerOnCanvas )
                    w += hintData.yAxesWidth();

                labelH = label->heightForWidth( labelW );
            }
            h += labelH + spacing();
        }
    }

    // Compute the legend contribution

    const QwtAbstractLegend *legend = plot->legend();
    if ( legend && !legend->isEmpty() )
    {
        if ( d_data->layoutEngine.legendPos() == QwtPlot::LeftLegend
            || d_data->layoutEngine.legendPos() == QwtPlot::RightLegend )
        {
            int legendW = legend->sizeHint().width();
            int legendH = legend->heightForWidth( legendW );

            if ( legend->frameWidth() > 0 )
                w += spacing();

            if ( legendH > h )
                legendW += legend->scrollExtent( Qt::Horizontal );

            if ( d_data->layoutEngine.legendRatio() < 1.0 )
                legendW = qMin( legendW, int( w / ( 1.0 - d_data->layoutEngine.legendRatio() ) ) );

            w += legendW + spacing();
        }
        else 
        {
            int legendW = qMin( legend->sizeHint().width(), w );
            int legendH = legend->heightForWidth( legendW );

            if ( legend->frameWidth() > 0 )
                h += spacing();

            if ( d_data->layoutEngine.legendRatio() < 1.0 )
                legendH = qMin( legendH, int( h / ( 1.0 - d_data->layoutEngine.legendRatio() ) ) );

            h += legendH + spacing();
        }
    }

    return QSize( w, h );
}

void QwtPlotLayout::update( const QwtPlot *plot,
    const QRectF &plotRect, Options options )
{
    invalidate();

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
        d_data->scaleRects[ axisPos ].resize( plot->axesCount( axisPos ) );

    activate( plot, plotRect, options );
}

/*!
  \brief Recalculate the geometry of all components.

  \param plot Plot to be layout
  \param plotRect Rectangle where to place the components
  \param options Layout options

  \sa invalidate(), titleRect(), footerRect()
      legendRect(), scaleRect(), canvasRect()
*/
void QwtPlotLayout::activate( const QwtPlot *plot,
    const QRectF &plotRect, Options options )
{
    QRectF rect( plotRect );  // undistributed rest of the plot rect

    // We extract all layout relevant parameters from the widgets,
    // and save them to d_data->layoutData.

    QwtPlotLayoutData layoutData( plot );

    QSize legendHint;

    if ( !( options & IgnoreLegend )
        && plot->legend() && !plot->legend()->isEmpty() )
    {
        legendHint = layoutData.legendData.legendHint( plot->legend(), rect );

        d_data->legendRect = d_data->layoutEngine.layoutLegend( 
            options, layoutData.legendData, rect, legendHint );

        // subtract d_data->legendRect from rect

        const QRegion region( rect.toRect() );
        rect = region.subtracted( d_data->legendRect.toRect() ).boundingRect();

        switch ( d_data->layoutEngine.legendPos() )
        {
            case QwtPlot::LeftLegend:
            {
                rect.setLeft( rect.left() + spacing() );
                break;
            }
            case QwtPlot::RightLegend:
            {
                rect.setRight( rect.right() - spacing() );
                break;
            }
            case QwtPlot::TopLegend:
            {
                rect.setTop( rect.top() + spacing() );
                break;
            }
            case QwtPlot::BottomLegend:
            {
                rect.setBottom( rect.bottom() - spacing() );
                break;
            }
        }
    }

    /*
     +---+-----------+---+
     |       Title       |
     +---+-----------+---+
     |   |   Axis    |   |
     +---+-----------+---+
     | A |           | A |
     | x |  Canvas   | x |
     | i |           | i |
     | s |           | s |
     +---+-----------+---+
     |   |   Axis    |   |
     +---+-----------+---+
     |      Footer       |
     +---+-----------+---+
    */

    // title, footer and axes include text labels. The height of each
    // label depends on its line breaks, that depend on the width
    // for the label. A line break in a horizontal text will reduce
    // the available width for vertical texts and vice versa.
    // layoutDimensions finds the height/width for title, footer and axes
    // including all line breaks.

    const QwtPlotLayoutEngine::Dimensions dimensions =
        d_data->layoutEngine.layoutDimensions( options, layoutData, rect );

    if ( dimensions.dimTitle > 0 )
    {
        QRectF &labelRect = d_data->titleRect;

        labelRect.setRect( rect.left(), rect.top(), rect.width(), dimensions.dimTitle );

        rect.setTop( labelRect.bottom() + spacing() );

        if ( !layoutData.hasSymmetricYAxes() )
        {
            // if only one of the y axes is missing we align
            // the title centered to the canvas

            labelRect = dimensions.centered( rect, labelRect );
        }
    }

    if ( dimensions.dimFooter > 0 )
    {
        QRectF &labelRect = d_data->footerRect;

        labelRect.setRect( rect.left(), rect.bottom() - dimensions.dimFooter, 
            rect.width(), dimensions.dimFooter );

        rect.setBottom( labelRect.top() - spacing() );

        if ( !layoutData.hasSymmetricYAxes() )
        {
            // if only one of the y axes is missing we align
            // the footer centered to the canvas

            labelRect = dimensions.centered( rect, labelRect );
        }
    }

    d_data->canvasRect = dimensions.innerRect( rect );

    for ( int axisPos = 0; axisPos < QwtAxis::PosCount; axisPos++ )
    {
        // set the rects for the axes

        int pos = 0;
        for ( int i = 0; i < d_data->scaleRects[ axisPos ].size(); i++ )
        {
            const QwtAxisId axisId( axisPos, i );

            if ( dimensions.dimAxis( axisId ) )
            {
                const int dim = dimensions.dimAxis( axisId );

                const QRectF &canvasRect = d_data->canvasRect;

                QRectF &scaleRect = d_data->scaleRects[ axisId.pos ][ axisId.id ];
                scaleRect = canvasRect;

                switch ( axisPos )
                {
                    case QwtAxis::yLeft:
                    {
                        scaleRect.setX( canvasRect.left() - pos - dim );
                        scaleRect.setWidth( dim );
                        break;
                    }
                    case QwtAxis::yRight:
                    {
                        scaleRect.setX( canvasRect.right() + pos );
                        scaleRect.setWidth( dim );
                        break;
                    }
                    case QwtAxis::xBottom:
                    {
                        scaleRect.setY( canvasRect.bottom() + pos );
                        scaleRect.setHeight( dim );
                        break;
                    }
                    case QwtAxis::xTop:
                    {
                        scaleRect.setY( canvasRect.top() - pos - dim );
                        scaleRect.setHeight( dim );
                        break;
                    }
                }
                scaleRect = scaleRect.normalized();
                pos += dim;
            }
        }
    }

    // +---+-----------+---+
    // |  <-   Axis   ->   |
    // +-^-+-----------+-^-+
    // | | |           | | |
    // |   |           |   |
    // | A |           | A |
    // | x |  Canvas   | x |
    // | i |           | i |
    // | s |           | s |
    // |   |           |   |
    // | | |           | | |
    // +-V-+-----------+-V-+
    // |   <-  Axis   ->   |
    // +---+-----------+---+

    // The ticks of the axes - not the labels above - should
    // be aligned to the canvas. So we try to use the empty
    // corners to extend the axes, so that the label texts
    // left/right of the min/max ticks are moved into them.

    d_data->layoutEngine.alignScales( options, layoutData, 
        d_data->canvasRect, d_data->scaleRects );

    if ( !d_data->legendRect.isEmpty() )
    {
        // We prefer to align the legend to the canvas - not to
        // the complete plot - if possible.

        d_data->legendRect = d_data->layoutEngine.alignLegend( 
            legendHint, d_data->canvasRect, d_data->legendRect );
    }
}
