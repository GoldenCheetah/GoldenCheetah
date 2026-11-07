/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_GRAPHIC_ITEM_H
#define QWT_PLOT_GRAPHIC_ITEM_H

#include "qwt_global.h"
#include "qwt_plot_item.h"

#include <qstring.h>
#include <memory>

/*!
   \brief A plot item, which displays
         a recorded sequence of QPainter commands

   QwtPlotGraphicItem renders a sequence of recorded painter commands
   into a specific plot area. Recording of painter commands can be
   done manually by QPainter or e.g. QSvgRenderer.

   \sa QwtPlotShapeItem, QwtPlotSvgItem
 */

class QWT_EXPORT QwtPlotGraphicItem : public QwtPlotItem
{
  public:
    explicit QwtPlotGraphicItem( const QString& title = QString() );
    explicit QwtPlotGraphicItem( const QwtText& title );

    ~QwtPlotGraphicItem() override;

    void setGraphic( const QRectF& rect, const QwtGraphic& );
    QwtGraphic graphic() const;

    QRectF boundingRect() const override;

    void draw( QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap,
        const QRectF& canvasRect ) const override;

    int rtti() const override;

  private:
    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

#endif
