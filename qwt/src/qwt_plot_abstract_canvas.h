/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_ABSTRACT_CANVAS_H
#define QWT_PLOT_ABSTRACT_CANVAS_H

#include "qwt_global.h"
#include <qwidget.h>
#include <memory>

class QwtPlot;

/*!
   \brief Base class for all type of plot canvases
 */
class QWT_EXPORT QwtPlotAbstractCanvas
{
  public:
    /*!
       \brief Focus indicator
       The default setting is NoFocusIndicator
       \sa setFocusIndicator(), focusIndicator(), drawFocusIndicator()
     */

    enum FocusIndicator
    {
        //! Don't paint a focus indicator
        NoFocusIndicator,

        /*!
           The focus is related to the complete canvas.
           Paint the focus indicator using drawFocusIndicator()
         */
        CanvasFocusIndicator,

        /*!
           The focus is related to an item (curve, point, ...) on
           the canvas. It is up to the application to display a
           focus indication using f.e. highlighting.
         */
        ItemFocusIndicator
    };

    virtual ~QwtPlotAbstractCanvas();

    QwtPlot* plot();
    const QwtPlot* plot() const;

    void setFocusIndicator( FocusIndicator );
    FocusIndicator focusIndicator() const;

    void setBorderRadius( double );
    double borderRadius() const;

  protected:
    explicit QwtPlotAbstractCanvas( QWidget* );

    virtual void drawFocusIndicator( QPainter* );
    virtual void drawBorder( QPainter* );
    virtual void drawBackground( QPainter* );

    void fillBackground( QPainter* );
    void drawCanvas( QPainter* );
    void drawStyled( QPainter*, bool );
    void drawUnstyled( QPainter* );

    QPainterPath canvasBorderPath( const QRect& rect ) const;
    void updateStyleSheetInfo();

  private:
    Q_DISABLE_COPY(QwtPlotAbstractCanvas)

    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

#endif
