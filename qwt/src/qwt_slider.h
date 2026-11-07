/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SLIDER_H
#define QWT_SLIDER_H

#include "qwt_global.h"
#include "qwt_abstract_slider.h"

#include <memory>

class QwtScaleDraw;

/*!
   \brief The Slider Widget

   QwtSlider is a slider widget which operates on an interval
   of type double. Its position is related to a scale showing
   the current value.

   The slider can be customized by having a through, a groove - or both.

   \image html sliders.png
 */

class QWT_EXPORT QwtSlider : public QwtAbstractSlider
{
    Q_OBJECT

    Q_PROPERTY( Qt::Orientation orientation
        READ orientation WRITE setOrientation )
    Q_PROPERTY( ScalePosition scalePosition READ scalePosition
        WRITE setScalePosition )

    Q_PROPERTY( bool trough READ hasTrough WRITE setTrough )
    Q_PROPERTY( bool groove READ hasGroove WRITE setGroove )

    Q_PROPERTY( QSize handleSize READ handleSize WRITE setHandleSize )
    Q_PROPERTY( int borderWidth READ borderWidth WRITE setBorderWidth )
    Q_PROPERTY( int spacing READ spacing WRITE setSpacing )

  public:

    /*!
       Position of the scale
       \sa QwtSlider(), setScalePosition(), setOrientation()
     */
    enum ScalePosition
    {
        //! The slider has no scale
        NoScale,

        //! The scale is right of a vertical or below a horizontal slider
        LeadingScale,

        //! The scale is left of a vertical or above a horizontal slider
        TrailingScale
    };

    Q_ENUM( ScalePosition )

    explicit QwtSlider( QWidget* parent = nullptr );
    explicit QwtSlider( Qt::Orientation, QWidget* parent = nullptr );

    ~QwtSlider() override;

    void setOrientation( Qt::Orientation );
    Qt::Orientation orientation() const;

    void setScalePosition( ScalePosition );
    ScalePosition scalePosition() const;

    void setTrough( bool );
    bool hasTrough() const;

    void setGroove( bool );
    bool hasGroove() const;

    void setHandleSize( const QSize& );
    QSize handleSize() const;

    void setBorderWidth( int );
    int borderWidth() const;

    void setSpacing( int );
    int spacing() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setScaleDraw( QwtScaleDraw* );
    const QwtScaleDraw* scaleDraw() const;

    void setUpdateInterval( int );
    int updateInterval() const;

  protected:
    double scrolledTo( const QPoint& ) const override;
    bool isScrollPosition( const QPoint& ) const override;

    virtual void drawSlider ( QPainter*, const QRect& ) const;
    virtual void drawHandle( QPainter*, const QRect&, int pos ) const;

    void mousePressEvent( QMouseEvent* ) override;
    void mouseReleaseEvent( QMouseEvent* ) override;
    void resizeEvent( QResizeEvent* ) override;
    void paintEvent ( QPaintEvent* ) override;
    void changeEvent( QEvent* ) override;
    void timerEvent( QTimerEvent* ) override;

    bool event( QEvent* ) override;

    void scaleChange() override;

    QRect sliderRect() const;
    QRect handleRect() const;

  private:
    QwtScaleDraw* scaleDraw();

    void layoutSlider( bool );

    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

#endif
