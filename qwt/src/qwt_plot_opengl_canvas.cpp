/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot_opengl_canvas.h"
#include "qwt_plot.h"
#include "qwt_painter.h"

#include <qpainter.h>
#include <qpainterpath.h>
#include <qcoreevent.h>
#include <qopenglframebufferobject.h>
#include <qopenglpaintdevice.h>

static void qwtUpdateContentsRect( int fw, QwtPlotOpenGLCanvas* canvas )
{
    canvas->setContentsMargins( fw, fw, fw, fw );
    canvas->update();
}

class QwtPlotOpenGLCanvas::PrivateData
{
  public:
    ~PrivateData()
    {
        delete fbo;
    }

    int numFBOSamples = -1;

    bool isPolished = false;
    bool fboDirty = true;
    QOpenGLFramebufferObject* fbo = nullptr;

    int frameStyle = QFrame::Panel | QFrame::Sunken;
    int lineWidth = 2;
    int midLineWidth = 0;

    PaintAttributes paintAttributes = QwtPlotOpenGLCanvas::BackingStore;
};

/*!
   \brief Constructor

   \param plot Parent plot widget
   \sa QwtPlot::setCanvas()
 */
QwtPlotOpenGLCanvas::QwtPlotOpenGLCanvas( QwtPlot* plot )
    : QwtPlotOpenGLCanvas( -1, plot )
{
}

/*!
   \brief Constructor

   \param numSamples Number of samples, see QSurfaceFormat::samples()
   \param plot Parent plot widget
   \sa QwtPlot::setCanvas()
 */
QwtPlotOpenGLCanvas::QwtPlotOpenGLCanvas( int numSamples, QwtPlot* plot )
    : QOpenGLWidget( plot )
    , QwtPlotAbstractCanvas( this )
    , m_data( new PrivateData )
{
    qwtUpdateContentsRect( frameWidth(), this );

    if ( numSamples < -1 )
        numSamples = -1;

    QSurfaceFormat fmt = format();
    if ( numSamples != fmt.samples() )
    {
        fmt.setSamples( numSamples );
        setFormat( fmt );
    }

#if 1
    setAttribute( Qt::WA_OpaquePaintEvent, true );
#endif

}

//! Destructor
QwtPlotOpenGLCanvas::~QwtPlotOpenGLCanvas()
{
}

/*!
   \brief Changing the paint attributes
    
   \param attribute Paint attribute
   \param on On/Off
    
   \sa testPaintAttribute()
 */ 
void QwtPlotOpenGLCanvas::setPaintAttribute( PaintAttribute attribute, bool on )
{   
    if ( testPaintAttribute( attribute ) == on )
        return;

    if ( on )
    {   
        m_data->paintAttributes |= attribute;
    }
    else
    {
        m_data->paintAttributes &= ~attribute;

        if ( attribute == BackingStore )
            clearBackingStore();
    }
}

/*!
   Test whether a paint attribute is enabled

   \param attribute Paint attribute
   \return true, when attribute is enabled
   \sa setPaintAttribute()
 */
bool QwtPlotOpenGLCanvas::testPaintAttribute( PaintAttribute attribute ) const
{
    return m_data->paintAttributes & attribute;
}

/*!
   Set the frame style

   \param style The bitwise OR between a shape and a shadow.

   \sa frameStyle(), QFrame::setFrameStyle(),
      setFrameShadow(), setFrameShape()
 */
void QwtPlotOpenGLCanvas::setFrameStyle( int style )
{
    if ( style != m_data->frameStyle )
    {
        m_data->frameStyle = style;
        qwtUpdateContentsRect( frameWidth(), this );
    }
}

/*!
   \return The bitwise OR between a frameShape() and a frameShadow()
   \sa setFrameStyle(), QFrame::frameStyle()
 */
int QwtPlotOpenGLCanvas::frameStyle() const
{
    return m_data->frameStyle;
}

/*!
   Set the frame shadow

   \param shadow Frame shadow
   \sa frameShadow(), setFrameShape(), QFrame::setFrameShadow()
 */
void QwtPlotOpenGLCanvas::setFrameShadow( QFrame::Shadow shadow )
{
    setFrameStyle( ( m_data->frameStyle & QFrame::Shape_Mask ) | shadow );
}

/*!
   \return Frame shadow
   \sa setFrameShadow(), QFrame::setFrameShadow()
 */
QFrame::Shadow QwtPlotOpenGLCanvas::frameShadow() const
{
    return (QFrame::Shadow) ( m_data->frameStyle & QFrame::Shadow_Mask );
}

/*!
   Set the frame shape

   \param shape Frame shape
   \sa frameShape(), setFrameShadow(), QFrame::frameShape()
 */
void QwtPlotOpenGLCanvas::setFrameShape( QFrame::Shape shape )
{
    setFrameStyle( ( m_data->frameStyle & QFrame::Shadow_Mask ) | shape );
}

/*!
   \return Frame shape
   \sa setFrameShape(), QFrame::frameShape()
 */
QFrame::Shape QwtPlotOpenGLCanvas::frameShape() const
{
    return (QFrame::Shape) ( m_data->frameStyle & QFrame::Shape_Mask );
}

/*!
   Set the frame line width

   The default line width is 2 pixels.

   \param width Line width of the frame
   \sa lineWidth(), setMidLineWidth()
 */
void QwtPlotOpenGLCanvas::setLineWidth( int width )
{
    width = qMax( width, 0 );
    if ( width != m_data->lineWidth )
    {
        m_data->lineWidth = qMax( width, 0 );
        qwtUpdateContentsRect( frameWidth(), this );
    }
}

/*!
   \return Line width of the frame
   \sa setLineWidth(), midLineWidth()
 */
int QwtPlotOpenGLCanvas::lineWidth() const
{
    return m_data->lineWidth;
}

/*!
   Set the frame mid line width

   The default midline width is 0 pixels.

   \param width Midline width of the frame
   \sa midLineWidth(), setLineWidth()
 */
void QwtPlotOpenGLCanvas::setMidLineWidth( int width )
{
    width = qMax( width, 0 );
    if ( width != m_data->midLineWidth )
    {
        m_data->midLineWidth = width;
        qwtUpdateContentsRect( frameWidth(), this );
    }
}

/*!
   \return Midline width of the frame
   \sa setMidLineWidth(), lineWidth()
 */
int QwtPlotOpenGLCanvas::midLineWidth() const
{
    return m_data->midLineWidth;
}

/*!
   \return Frame width depending on the style, line width and midline width.
 */
int QwtPlotOpenGLCanvas::frameWidth() const
{
    return ( frameStyle() != QFrame::NoFrame ) ? m_data->lineWidth : 0;
}


/*!
   Paint event

   \param event Paint event
   \sa QwtPlot::drawCanvas()
 */
void QwtPlotOpenGLCanvas::paintEvent( QPaintEvent* event )
{
    if ( m_data->isPolished )
        QOpenGLWidget::paintEvent( event );
}

/*!
   Qt event handler for QEvent::PolishRequest and QEvent::StyleChange
   \param event Qt Event
   \return See QGLWidget::event()
 */
bool QwtPlotOpenGLCanvas::event( QEvent* event )
{
    if ( event->type() == QEvent::Resize )
    {
        if ( m_data->numFBOSamples < 0  )
        {
            /*
                QOpenGLWidget always uses a FBO and sets the number of samples for
                the FBO not for the widget itself. So format().samples() does not
                return the correct value after the first QEvent::Resize.
             */
            m_data->numFBOSamples = qMax( format().samples(), 0 );
        }
    }

    const bool ok = QOpenGLWidget::event( event );

    if ( event->type() == QEvent::PolishRequest )
    {
        // In opposite to non OpenGL widgets receive pointless
        // early repaints. As we always have a QEvent::PolishRequest
        // followed by QEvent::Paint, we can ignore all these repaints.

        m_data->isPolished = true;
    }

    if ( event->type() == QEvent::PolishRequest ||
        event->type() == QEvent::StyleChange )
    {
        // assuming, that we always have a styled background
        // when we have a style sheet

        setAttribute( Qt::WA_StyledBackground,
            testAttribute( Qt::WA_StyleSheet ) );
    }

    return ok;
}

//! \return The rectangle where the frame is drawn in.
QRect QwtPlotOpenGLCanvas::frameRect() const
{
    const int fw = frameWidth();
    return contentsRect().adjusted( -fw, -fw, fw, fw );
}

/*!
   Invalidate the paint cache and repaint the canvas
   \sa invalidatePaintCache()
 */
void QwtPlotOpenGLCanvas::replot()
{
    invalidateBackingStore();

    if ( testPaintAttribute( QwtPlotOpenGLCanvas::ImmediatePaint ) )
        repaint( contentsRect() );
    else
        update( contentsRect() );
}

//! Invalidate the internal backing store
void QwtPlotOpenGLCanvas::invalidateBackingStore()
{
    m_data->fboDirty = true;
}

void QwtPlotOpenGLCanvas::clearBackingStore()
{
    delete m_data->fbo;
    m_data->fbo = nullptr;
}

/*!
   Calculate the painter path for a styled or rounded border

   When the canvas has no styled background or rounded borders
   the painter path is empty.

   \param rect Bounding rectangle of the canvas
   \return Painter path, that can be used for clipping
 */
QPainterPath QwtPlotOpenGLCanvas::borderPath( const QRect& rect ) const
{
    return canvasBorderPath( rect );
}

//! Helper function for the derived plot canvas
void QwtPlotOpenGLCanvas::draw( QPainter* painter )
{
#if 0
    painter->translate( 1, 1 );
#endif

    if ( testAttribute( Qt::WA_StyledBackground ) )
        drawStyled( painter, true );
    else
        drawUnstyled( painter );

    if ( frameWidth() > 0 )
        drawBorder( painter );
}

//! No operation - reserved for some potential use in the future
void QwtPlotOpenGLCanvas::initializeGL()
{
}

//! Paint the plot
void QwtPlotOpenGLCanvas::paintGL()
{
    const bool hasFocusIndicator =
        hasFocus() && focusIndicator() == CanvasFocusIndicator;

    QPainter painter;

    if ( testPaintAttribute( QwtPlotOpenGLCanvas::BackingStore ) &&
        QOpenGLFramebufferObject::hasOpenGLFramebufferBlit() )
    {
        const qreal pixelRatio = QwtPainter::devicePixelRatio( this );
        const QSize fboSize = size() * pixelRatio;

        if ( hasFocusIndicator )
            painter.begin( this );

        /*
           QOpenGLWidget has its own internal FBO, that is used to restore
           its content without having to repaint. This works fine when f.e
           a rubberband is moving on top, but there are still situations,
           where we can repaint without an potentially expensive replot:

               - when having the focus the top level window gets activated/deactivated
               - ???
         */

        if ( m_data->fbo )
        {
            if ( m_data->fbo->size() != fboSize )
            {
                delete m_data->fbo;
                m_data->fbo = nullptr;
            }
        }

        if ( m_data->fbo == nullptr )
        {
            QOpenGLFramebufferObjectFormat fboFormat;
            fboFormat.setAttachment( QOpenGLFramebufferObject::CombinedDepthStencil );

            if ( m_data->numFBOSamples > 0 )
                fboFormat.setSamples( m_data->numFBOSamples );

            m_data->fbo = new QOpenGLFramebufferObject( fboSize, fboFormat );
            m_data->fboDirty = true;
        }

        if ( m_data->fboDirty )
        {
            m_data->fbo->bind();

            QOpenGLPaintDevice pd( fboSize );

            QPainter fboPainter( &pd );
            fboPainter.scale( pixelRatio, pixelRatio );
            draw( &fboPainter);
            fboPainter.end();

            m_data->fboDirty = false;
        }

        QOpenGLFramebufferObject::blitFramebuffer( nullptr, m_data->fbo );
    }
    else
    {
        painter.begin( this );
        draw( &painter );
    }

    if ( hasFocusIndicator )
        drawFocusIndicator( &painter );
}

//! No operation - reserved for some potential use in the future
void QwtPlotOpenGLCanvas::resizeGL( int, int )
{
    // nothing to do
}

#include "moc_qwt_plot_opengl_canvas.cpp"
