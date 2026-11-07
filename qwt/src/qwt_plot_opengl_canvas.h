/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_OPENGL_CANVAS_H
#define QWT_PLOT_OPENGL_CANVAS_H

#include "qwt_global.h"
#include "qwt_plot_abstract_canvas.h"

#include <qopenglwidget.h>
#include <qframe.h>
#include <memory>

class QwtPlot;

/*!
   \brief An alternative canvas for a QwtPlot derived from QOpenGLWidget

   Even if QwtPlotOpenGLCanvas is not derived from QFrame it imitates
   its API. When using style sheets it supports the box model - beside
   backgrounds with rounded borders.

   \sa QwtPlot::setCanvas(), QwtPlotCanvas, QwtPlotCanvas::OpenGLBuffer

   \note Another way for getting hardware accelerated graphics is using
        an OpenGL offscreen buffer ( QwtPlotCanvas::OpenGLBuffer ) with QwtPlotCanvas.
        Performance is worse, than rendering straight to a QOpenGLWidget, but is usually
        better integrated into a desktop application.
 */
class QWT_EXPORT QwtPlotOpenGLCanvas : public QOpenGLWidget, public QwtPlotAbstractCanvas
{
    Q_OBJECT

    Q_PROPERTY( QFrame::Shadow frameShadow READ frameShadow WRITE setFrameShadow )
    Q_PROPERTY( QFrame::Shape frameShape READ frameShape WRITE setFrameShape )
    Q_PROPERTY( int lineWidth READ lineWidth WRITE setLineWidth )
    Q_PROPERTY( int midLineWidth READ midLineWidth WRITE setMidLineWidth )
    Q_PROPERTY( int frameWidth READ frameWidth )
    Q_PROPERTY( QRect frameRect READ frameRect DESIGNABLE false )

    Q_PROPERTY( double borderRadius READ borderRadius WRITE setBorderRadius )

  public:
    /*!
       \brief Paint attributes

       The default setting enables BackingStore and Opaque.

       \sa setPaintAttribute(), testPaintAttribute()
     */
    enum PaintAttribute
    {
        /*!
           \brief Paint double buffered ( using QOpenGLFramebufferObject )
           \sa backingStore(), invalidateBackingStore()
         */
        BackingStore = 1,

        /*!
           When ImmediatePaint is set replot() calls repaint()
           instead of update().

           \sa replot(), QWidget::repaint(), QWidget::update()
         */
        ImmediatePaint = 8,
    };

    //! Paint attributes
    Q_DECLARE_FLAGS( PaintAttributes, PaintAttribute )

    explicit QwtPlotOpenGLCanvas( QwtPlot* = nullptr );
    explicit QwtPlotOpenGLCanvas( int samples, QwtPlot* = nullptr);

    ~QwtPlotOpenGLCanvas() override;

    void setPaintAttribute( PaintAttribute, bool on = true );
    bool testPaintAttribute( PaintAttribute ) const;

    void setFrameStyle( int style );
    int frameStyle() const;

    void setFrameShadow( QFrame::Shadow );
    QFrame::Shadow frameShadow() const;

    void setFrameShape( QFrame::Shape );
    QFrame::Shape frameShape() const;

    void setLineWidth( int );
    int lineWidth() const;

    void setMidLineWidth( int );
    int midLineWidth() const;

    int frameWidth() const;
    QRect frameRect() const;

    Q_INVOKABLE void invalidateBackingStore();
    Q_INVOKABLE QPainterPath borderPath( const QRect& ) const;

    bool event( QEvent* ) override;

  public Q_SLOTS:
    void replot();

  protected:
    void paintEvent( QPaintEvent* ) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL( int width, int height ) override;

    void draw( QPainter* );

  private:
    void clearBackingStore();

    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( QwtPlotOpenGLCanvas::PaintAttributes )

#endif
