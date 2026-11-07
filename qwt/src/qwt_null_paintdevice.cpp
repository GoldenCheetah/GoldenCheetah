/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_null_paintdevice.h"
#include <qpaintengine.h>
#include <qpainterpath.h>

class QwtNullPaintDevice::PrivateData
{
  public:
    Mode mode = NormalMode;
};

class QwtNullPaintDevice::PaintEngine final : public QPaintEngine
{
  public:
    PaintEngine();

    bool begin( QPaintDevice* ) override;
    bool end() override;

    Type type () const override;
    void updateState( const QPaintEngineState& ) override;

    void drawRects( const QRect*, int ) override;
    void drawRects( const QRectF*, int ) override;

    void drawLines( const QLine*, int ) override;
    void drawLines( const QLineF*, int ) override;

    void drawEllipse( const QRectF& ) override;
    void drawEllipse( const QRect& ) override;

    void drawPath( const QPainterPath& ) override;

    void drawPoints( const QPointF*, int ) override;
    void drawPoints( const QPoint*, int ) override;

    void drawPolygon( const QPointF*, int, PolygonDrawMode ) override;
    void drawPolygon( const QPoint*, int, PolygonDrawMode ) override;

    void drawPixmap( const QRectF&, const QPixmap&, const QRectF& ) override;

    void drawTextItem( const QPointF&, const QTextItem& ) override;

    void drawTiledPixmap( const QRectF&,
        const QPixmap&, const QPointF& ) override;

    void drawImage( const QRectF&, const QImage&,
        const QRectF&, Qt::ImageConversionFlags ) override;

  private:
    QwtNullPaintDevice* nullDevice();
};

QwtNullPaintDevice::PaintEngine::PaintEngine()
    : QPaintEngine( QPaintEngine::AllFeatures )
{
}

bool QwtNullPaintDevice::PaintEngine::begin( QPaintDevice* )
{
    setActive( true );
    return true;
}

bool QwtNullPaintDevice::PaintEngine::end()
{
    setActive( false );
    return true;
}

QPaintEngine::Type QwtNullPaintDevice::PaintEngine::type() const
{
    /*
        How to avoid conflicts with other 3rd party pain engines ?
        At least we don't use QPaintEngine::User what is known to
        be the value of some print engines
     */
    return static_cast< QPaintEngine::Type >( QPaintEngine::MaxUser - 2 );
}

void QwtNullPaintDevice::PaintEngine::drawRects(
    const QRect* rects, int rectCount)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawRects( rects, rectCount );
        return;
    }

    device->drawRects( rects, rectCount );
}

void QwtNullPaintDevice::PaintEngine::drawRects(
    const QRectF* rects, int rectCount)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawRects( rects, rectCount );
        return;
    }

    device->drawRects( rects, rectCount );
}

void QwtNullPaintDevice::PaintEngine::drawLines(
    const QLine* lines, int lineCount)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawLines( lines, lineCount );
        return;
    }

    device->drawLines( lines, lineCount );
}

void QwtNullPaintDevice::PaintEngine::drawLines(
    const QLineF* lines, int lineCount)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawLines( lines, lineCount );
        return;
    }

    device->drawLines( lines, lineCount );
}

void QwtNullPaintDevice::PaintEngine::drawEllipse(
    const QRectF& rect)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawEllipse( rect );
        return;
    }

    device->drawEllipse( rect );
}

void QwtNullPaintDevice::PaintEngine::drawEllipse(
    const QRect& rect)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawEllipse( rect );
        return;
    }

    device->drawEllipse( rect );
}


void QwtNullPaintDevice::PaintEngine::drawPath(
    const QPainterPath& path)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    device->drawPath( path );
}

void QwtNullPaintDevice::PaintEngine::drawPoints(
    const QPointF* points, int pointCount)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawPoints( points, pointCount );
        return;
    }

    device->drawPoints( points, pointCount );
}

void QwtNullPaintDevice::PaintEngine::drawPoints(
    const QPoint* points, int pointCount)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawPoints( points, pointCount );
        return;
    }

    device->drawPoints( points, pointCount );
}

void QwtNullPaintDevice::PaintEngine::drawPolygon(
    const QPointF* points, int pointCount, PolygonDrawMode mode)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() == QwtNullPaintDevice::PathMode )
    {
        QPainterPath path;

        if ( pointCount > 0 )
        {
            path.moveTo( points[0] );
            for ( int i = 1; i < pointCount; i++ )
                path.lineTo( points[i] );

            if ( mode != PolylineMode )
                path.closeSubpath();
        }

        device->drawPath( path );
        return;
    }

    device->drawPolygon( points, pointCount, mode );
}

void QwtNullPaintDevice::PaintEngine::drawPolygon(
    const QPoint* points, int pointCount, PolygonDrawMode mode)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() == QwtNullPaintDevice::PathMode )
    {
        QPainterPath path;

        if ( pointCount > 0 )
        {
            path.moveTo( points[0] );
            for ( int i = 1; i < pointCount; i++ )
                path.lineTo( points[i] );

            if ( mode != PolylineMode )
                path.closeSubpath();
        }

        device->drawPath( path );
        return;
    }

    device->drawPolygon( points, pointCount, mode );
}

void QwtNullPaintDevice::PaintEngine::drawPixmap(
    const QRectF& rect, const QPixmap& pm, const QRectF& subRect )
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    device->drawPixmap( rect, pm, subRect );
}

void QwtNullPaintDevice::PaintEngine::drawTextItem(
    const QPointF& pos, const QTextItem& textItem)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawTextItem( pos, textItem );
        return;
    }

    device->drawTextItem( pos, textItem );
}

void QwtNullPaintDevice::PaintEngine::drawTiledPixmap(
    const QRectF& rect, const QPixmap& pixmap,
    const QPointF& subRect)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    if ( device->mode() != QwtNullPaintDevice::NormalMode )
    {
        QPaintEngine::drawTiledPixmap( rect, pixmap, subRect );
        return;
    }

    device->drawTiledPixmap( rect, pixmap, subRect );
}

void QwtNullPaintDevice::PaintEngine::drawImage(
    const QRectF& rect, const QImage& image,
    const QRectF& subRect, Qt::ImageConversionFlags flags)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    device->drawImage( rect, image, subRect, flags );
}

void QwtNullPaintDevice::PaintEngine::updateState(
    const QPaintEngineState& engineState)
{
    QwtNullPaintDevice* device = nullDevice();
    if ( device == nullptr )
        return;

    device->updateState( engineState );
}

inline QwtNullPaintDevice* QwtNullPaintDevice::PaintEngine::nullDevice()
{
    if ( !isActive() )
        return nullptr;

    return static_cast< QwtNullPaintDevice* >( paintDevice() );
}

//! Constructor
QwtNullPaintDevice::QwtNullPaintDevice()
    : m_engine( nullptr )
    , m_data( new PrivateData )
{
}

//! Destructor
QwtNullPaintDevice::~QwtNullPaintDevice()
{
    delete m_engine;
}

/*!
    Set the render mode

    \param mode New mode
    \sa mode()
 */
void QwtNullPaintDevice::setMode( Mode mode )
{
    m_data->mode = mode;
}

/*!
    \return Render mode
    \sa setMode()
 */
QwtNullPaintDevice::Mode QwtNullPaintDevice::mode() const
{
    return m_data->mode;
}

//! See QPaintDevice::paintEngine()
QPaintEngine* QwtNullPaintDevice::paintEngine() const
{
    if ( m_engine == nullptr )
    {
        QwtNullPaintDevice* that =
            const_cast< QwtNullPaintDevice* >( this );

        that->m_engine = new PaintEngine();
    }

    return m_engine;
}

/*!
    See QPaintDevice::metric()

    \param deviceMetric Type of metric
    \return Metric information for the given paint device metric.

    \sa sizeMetrics()
 */
int QwtNullPaintDevice::metric( PaintDeviceMetric deviceMetric ) const
{
    int value;

    switch ( deviceMetric )
    {
        case PdmWidth:
        {
            value = sizeMetrics().width();
            break;
        }
        case PdmHeight:
        {
            value = sizeMetrics().height();
            break;
        }
        case PdmNumColors:
        {
            value = 0xffffffff;
            break;
        }
        case PdmDepth:
        {
            value = 32;
            break;
        }
        case PdmPhysicalDpiX:
        case PdmPhysicalDpiY:
        case PdmDpiY:
        case PdmDpiX:
        {
            value = 72;
            break;
        }
        case PdmWidthMM:
        {
            value = qRound( metric( PdmWidth ) * 25.4 / metric( PdmDpiX ) );
            break;
        }
        case PdmHeightMM:
        {
            value = qRound( metric( PdmHeight ) * 25.4 / metric( PdmDpiY ) );
            break;
        }
        case PdmDevicePixelRatio:
        {
            value = 1.0;
            break;
        }
        default:
        {
            value = QPaintDevice::metric( deviceMetric );
        }
    }

    return value;
}

//! See QPaintEngine::drawRects()
void QwtNullPaintDevice::drawRects(
    const QRect* rects, int rectCount)
{
    Q_UNUSED(rects);
    Q_UNUSED(rectCount);
}

//! See QPaintEngine::drawRects()
void QwtNullPaintDevice::drawRects(
    const QRectF* rects, int rectCount)
{
    Q_UNUSED(rects);
    Q_UNUSED(rectCount);
}

//! See QPaintEngine::drawLines()
void QwtNullPaintDevice::drawLines(
    const QLine* lines, int lineCount)
{
    Q_UNUSED(lines);
    Q_UNUSED(lineCount);
}

//! See QPaintEngine::drawLines()
void QwtNullPaintDevice::drawLines(
    const QLineF* lines, int lineCount)
{
    Q_UNUSED(lines);
    Q_UNUSED(lineCount);
}

//! See QPaintEngine::drawEllipse()
void QwtNullPaintDevice::drawEllipse( const QRectF& rect )
{
    Q_UNUSED(rect);
}

//! See QPaintEngine::drawEllipse()
void QwtNullPaintDevice::drawEllipse( const QRect& rect )
{
    Q_UNUSED(rect);
}

//! See QPaintEngine::drawPath()
void QwtNullPaintDevice::drawPath( const QPainterPath& path )
{
    Q_UNUSED(path);
}

//! See QPaintEngine::drawPoints()
void QwtNullPaintDevice::drawPoints(
    const QPointF* points, int pointCount)
{
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
}

//! See QPaintEngine::drawPoints()
void QwtNullPaintDevice::drawPoints(
    const QPoint* points, int pointCount)
{
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
}

//! See QPaintEngine::drawPolygon()
void QwtNullPaintDevice::drawPolygon(
    const QPointF* points, int pointCount,
    QPaintEngine::PolygonDrawMode mode)
{
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
    Q_UNUSED(mode);
}

//! See QPaintEngine::drawPolygon()
void QwtNullPaintDevice::drawPolygon(
    const QPoint* points, int pointCount,
    QPaintEngine::PolygonDrawMode mode)
{
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
    Q_UNUSED(mode);
}

//! See QPaintEngine::drawPixmap()
void QwtNullPaintDevice::drawPixmap( const QRectF& rect,
    const QPixmap& pm, const QRectF& subRect )
{
    Q_UNUSED(rect);
    Q_UNUSED(pm);
    Q_UNUSED(subRect);
}

//! See QPaintEngine::drawTextItem()
void QwtNullPaintDevice::drawTextItem(
    const QPointF& pos, const QTextItem& textItem)
{
    Q_UNUSED(pos);
    Q_UNUSED(textItem);
}

//! See QPaintEngine::drawTiledPixmap()
void QwtNullPaintDevice::drawTiledPixmap(
    const QRectF& rect, const QPixmap& pixmap,
    const QPointF& subRect)
{
    Q_UNUSED(rect);
    Q_UNUSED(pixmap);
    Q_UNUSED(subRect);
}

//! See QPaintEngine::drawImage()
void QwtNullPaintDevice::drawImage(
    const QRectF& rect, const QImage& image,
    const QRectF& subRect, Qt::ImageConversionFlags flags)
{
    Q_UNUSED(rect);
    Q_UNUSED(image);
    Q_UNUSED(subRect);
    Q_UNUSED(flags);
}

//! See QPaintEngine::updateState()
void QwtNullPaintDevice::updateState(
    const QPaintEngineState& state )
{
    Q_UNUSED(state);
}
