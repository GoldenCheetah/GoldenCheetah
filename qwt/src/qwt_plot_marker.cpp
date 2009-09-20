/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qpainter.h>
#include "qwt_painter.h"
#include "qwt_scale_map.h"
#include "qwt_plot_marker.h"
#include "qwt_symbol.h"
#include "qwt_text.h"
#include "qwt_math.h"

class QwtPlotMarker::PrivateData
{
public:
    PrivateData():
        labelAlignment(Qt::AlignCenter),
        labelOrientation(Qt::Horizontal),
        spacing(2),
        style(NoLine),
        xValue(0.0),
        yValue(0.0)
    {
        symbol = new QwtSymbol();
    }

    ~PrivateData()
    {
        delete symbol;
    }

    QwtText label;
#if QT_VERSION < 0x040000
    int labelAlignment;
#else
    Qt::Alignment labelAlignment;
#endif
    Qt::Orientation labelOrientation;
    int spacing;

    QPen pen;
    QwtSymbol *symbol;
    LineStyle style;

    double xValue;
    double yValue;
};

//! Sets alignment to Qt::AlignCenter, and style to NoLine
QwtPlotMarker::QwtPlotMarker():
    QwtPlotItem(QwtText("Marker"))
{
    d_data = new PrivateData;
    setZ(30.0);
}

//! Destructor
QwtPlotMarker::~QwtPlotMarker()
{
    delete d_data;
}

//! \return QwtPlotItem::Rtti_PlotMarker
int QwtPlotMarker::rtti() const
{
    return QwtPlotItem::Rtti_PlotMarker;
}

//! Return Value
QwtDoublePoint QwtPlotMarker::value() const
{
    return QwtDoublePoint(d_data->xValue, d_data->yValue);
}

//! Return x Value
double QwtPlotMarker::xValue() const 
{ 
    return d_data->xValue; 
}

//! Return y Value
double QwtPlotMarker::yValue() const 
{ 
    return d_data->yValue; 
}

//! Set Value
void QwtPlotMarker::setValue(const QwtDoublePoint& pos)
{
    setValue(pos.x(), pos.y());
}

//! Set Value
void QwtPlotMarker::setValue(double x, double y) 
{
    if ( x != d_data->xValue || y != d_data->yValue )
    {
        d_data->xValue = x; 
        d_data->yValue = y; 
        itemChanged(); 
    }
}

//! Set X Value
void QwtPlotMarker::setXValue(double x) 
{ 
    setValue(x, d_data->yValue);
}

//! Set Y Value
void QwtPlotMarker::setYValue(double y) 
{ 
    setValue(d_data->xValue, y);
}

/*!
  Draw the marker

  \param painter Painter
  \param xMap x Scale Map
  \param yMap y Scale Map
  \param canvasRect Contents rect of the canvas in painter coordinates
*/
void QwtPlotMarker::draw(QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &canvasRect) const
{
    const int x = xMap.transform(d_data->xValue);
    const int y = yMap.transform(d_data->yValue);

    drawAt(painter, canvasRect, QPoint(x, y));
}

/*!
  Draw the marker at a specific position

  \param painter Painter
  \param canvasRect Contents rect of the canvas in painter coordinates
  \param pos Position of the marker in painter coordinates
*/
void QwtPlotMarker::drawAt(QPainter *painter, 
    const QRect &canvasRect, const QPoint &pos) const
{
    // draw lines
    if (d_data->style != NoLine)
    {
        painter->setPen(QwtPainter::scaledPen(d_data->pen));
        if ( d_data->style == QwtPlotMarker::HLine || 
            d_data->style == QwtPlotMarker::Cross )
        {
            QwtPainter::drawLine(painter, canvasRect.left(), 
                pos.y(), canvasRect.right(), pos.y() );
        }
        if (d_data->style == QwtPlotMarker::VLine || 
            d_data->style == QwtPlotMarker::Cross )
        {
            QwtPainter::drawLine(painter, pos.x(), 
                canvasRect.top(), pos.x(), canvasRect.bottom());
        }
    }

    // draw symbol
    if (d_data->symbol->style() != QwtSymbol::NoSymbol)
        d_data->symbol->draw(painter, pos.x(), pos.y());

    drawLabel(painter, canvasRect, pos);
}

void QwtPlotMarker::drawLabel(QPainter *painter, 
    const QRect &canvasRect, const QPoint &pos) const
{
    if (d_data->label.isEmpty())
        return;

    int align = d_data->labelAlignment;
    QPoint alignPos = pos;

    QSize symbolOff(0, 0);

    switch(d_data->style)
    {
        case QwtPlotMarker::VLine:
        {
            // In VLine-style the y-position is pointless and
            // the alignment flags are relative to the canvas

            if (d_data->labelAlignment & (int) Qt::AlignTop)
            {
                alignPos.setY(canvasRect.top());
                align &= ~Qt::AlignTop;
                align |= Qt::AlignBottom;
            }
            else if (d_data->labelAlignment & (int) Qt::AlignBottom)
            {
                // In HLine-style the x-position is pointless and
                // the alignment flags are relative to the canvas

                alignPos.setY(canvasRect.bottom() - 1);
                align &= ~Qt::AlignBottom;
                align |= Qt::AlignTop;
            }
            else
            {
                alignPos.setY(canvasRect.center().y());
            }
            break;
        }
        case QwtPlotMarker::HLine:
        {
            if (d_data->labelAlignment & (int) Qt::AlignLeft)
            {
                alignPos.setX(canvasRect.left());
                align &= ~Qt::AlignLeft;
                align |= Qt::AlignRight;
            }
            else if (d_data->labelAlignment & (int) Qt::AlignRight)
            {
                alignPos.setX(canvasRect.right() - 1);
                align &= ~Qt::AlignRight;
                align |= Qt::AlignLeft;
            }
            else
            {
                alignPos.setX(canvasRect.center().x());
            }
            break;
        }
        default:
        {
            if ( d_data->symbol->style() != QwtSymbol::NoSymbol )
            {
                symbolOff = d_data->symbol->size() + QSize(1, 1);
                symbolOff /= 2;
            }
        }
    }
    
    int pw = d_data->pen.width();
    if ( pw == 0 )
        pw = 1;

    const int xSpacing = 
        QwtPainter::metricsMap().screenToLayoutX(d_data->spacing);
    const int ySpacing = 
        QwtPainter::metricsMap().screenToLayoutY(d_data->spacing);


    int xOff = qwtMax( (pw + 1) / 2, symbolOff.width() );
    int yOff = qwtMax( (pw + 1) / 2, symbolOff.height() );

    const QSize textSize = d_data->label.textSize(painter->font());

    if ( align & Qt::AlignLeft )
    {
        alignPos.rx() -= xOff + xSpacing;
        if ( d_data->labelOrientation == Qt::Vertical )
            alignPos.rx() -= textSize.height();
        else
            alignPos.rx() -= textSize.width();
    }
    else if ( align & Qt::AlignRight )
    {
        alignPos.rx() += xOff + xSpacing;
    }
    else
    {
        if ( d_data->labelOrientation == Qt::Vertical )
            alignPos.rx() -= textSize.height() / 2;
        else
            alignPos.rx() -= textSize.width() / 2;
    }

    if (align & (int) Qt::AlignTop)
    {
        alignPos.ry() -= yOff + ySpacing;
        if ( d_data->labelOrientation != Qt::Vertical )
            alignPos.ry() -= textSize.height();
    }
    else if (align & (int) Qt::AlignBottom)
    {
        alignPos.ry() += yOff + ySpacing;
        if ( d_data->labelOrientation == Qt::Vertical )
            alignPos.ry() += textSize.width();
    }
    else
    {
        if ( d_data->labelOrientation == Qt::Vertical )
            alignPos.ry() += textSize.width() / 2;
        else
            alignPos.ry() -= textSize.height() / 2;
    }

    painter->translate(alignPos.x(), alignPos.y());
    if ( d_data->labelOrientation == Qt::Vertical )
        painter->rotate(-90.0);

    const QRect textRect(0, 0, textSize.width(), textSize.height());
    d_data->label.draw(painter, textRect);
}

/*!
  \brief Set the line style
  \param st Line style. Can be one of QwtPlotMarker::NoLine,
    HLine, VLine or Cross
  \sa lineStyle()
*/
void QwtPlotMarker::setLineStyle(QwtPlotMarker::LineStyle st)
{
    if ( st != d_data->style )
    {
        d_data->style = st;
        itemChanged();
    }
}

/*!
  \return the line style
  \sa For a description of line styles, see QwtPlotMarker::setLineStyle()
*/
QwtPlotMarker::LineStyle QwtPlotMarker::lineStyle() const 
{ 
    return d_data->style; 
}

/*!
  \brief Assign a symbol
  \param s New symbol 
  \sa symbol()
*/
void QwtPlotMarker::setSymbol(const QwtSymbol &s)
{
    delete d_data->symbol;
    d_data->symbol = s.clone();
    itemChanged();
}

/*!
  \return the symbol
  \sa setSymbol(), QwtSymbol
*/
const QwtSymbol &QwtPlotMarker::symbol() const 
{ 
    return *d_data->symbol; 
}

/*!
  \brief Set the label
  \param label label text
  \sa label()
*/
void QwtPlotMarker::setLabel(const QwtText& label)
{
    if ( label != d_data->label )
    {
        d_data->label = label;
        itemChanged();
    }
}

/*!
  \return the label
  \sa setLabel()
*/
QwtText QwtPlotMarker::label() const 
{ 
    return d_data->label; 
}

/*!
  \brief Set the alignment of the label

  In case of QwtPlotMarker::HLine the alignment is relative to the
  y position of the marker, but the horizontal flags correspond to the 
  canvas rectangle. In case of QwtPlotMarker::VLine the alignment is 
  relative to the x position of the marker, but the vertical flags
  correspond to the canvas rectangle.

  In all other styles the alignment is relative to the marker's position.

  \param align Alignment. A combination of AlignTop, AlignBottom,
    AlignLeft, AlignRight, AlignCenter, AlgnHCenter,
    AlignVCenter.  
  \sa labelAlignment(), labelOrientation()
*/
#if QT_VERSION < 0x040000
void QwtPlotMarker::setLabelAlignment(int align)
#else
void QwtPlotMarker::setLabelAlignment(Qt::Alignment align)
#endif
{
    if ( align != d_data->labelAlignment )
    {
        d_data->labelAlignment = align;
        itemChanged();
    }
}

/*!
  \return the label alignment
  \sa setLabelAlignment(), setLabelOrientation()
*/
#if QT_VERSION < 0x040000
int QwtPlotMarker::labelAlignment() const 
#else
Qt::Alignment QwtPlotMarker::labelAlignment() const 
#endif
{ 
    return d_data->labelAlignment; 
}

/*!
  \brief Set the orientation of the label

  When orientation is Qt::Vertical the label is rotated by 90.0 degrees
  ( from bottom to top ).

  \param orientation Orientation of the label

  \sa labelOrientation(), setLabelAlignment()
*/
void QwtPlotMarker::setLabelOrientation(Qt::Orientation orientation)
{
    if ( orientation != d_data->labelOrientation )
    {
        d_data->labelOrientation = orientation;
        itemChanged();
    }
}

/*!
  \return the label orientation
  \sa setLabelOrientation(), labelAlignment()
*/
Qt::Orientation QwtPlotMarker::labelOrientation() const
{
    return d_data->labelOrientation;
}

/*!
  \brief Set the spacing

  When the label is not centered on the marker position, the spacing
  is the distance between the position and the label.

  \param spacing Spacing
  \sa spacing(), setLabelAlignment()
*/
void QwtPlotMarker::setSpacing(int spacing)
{
    if ( spacing < 0 )
        spacing = 0;

    if ( spacing == d_data->spacing )
        return;

    d_data->spacing = spacing;
    itemChanged();
}

/*!
  \return the spacing
  \sa setSpacing()
*/
int QwtPlotMarker::spacing() const
{
    return d_data->spacing;
}

/*!
  Specify a pen for the line.

  The width of non cosmetic pens is scaled according to the resolution
  of the paint device.

  \param pen New pen
  \sa linePen(), QwtPainter::scaledPen()
*/
void QwtPlotMarker::setLinePen(const QPen &pen)
{
    if ( pen != d_data->pen )
    {
        d_data->pen = pen;
        itemChanged();
    }
}

/*!
  \return the line pen
  \sa setLinePen()
*/
const QPen &QwtPlotMarker::linePen() const 
{ 
    return d_data->pen; 
}

QwtDoubleRect QwtPlotMarker::boundingRect() const
{
    return QwtDoubleRect(d_data->xValue, d_data->yValue, 0.0, 0.0);
}
