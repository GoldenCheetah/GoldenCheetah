
/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "IndendPlotMarker.h"
#include "qwt_symbol.h"
#include "qwt_plot_marker.h"
#include "qwt_painter.h"
#include "qwt_scale_map.h"

#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

QwtIndPlotMarker::Matrix* QwtIndPlotMarker::m_drawnPixels = new QwtIndPlotMarker::Matrix(1,1);

inline QwtIndPlotMarker::Matrix::Matrix(unsigned long rows, unsigned long cols):
    m_rows(rows), m_cols(cols)
{
    m_data = new bool[rows * cols]();
}

inline QwtIndPlotMarker::Matrix::~Matrix()
{
    reset();
}

/* UNUSED
QwtIndPlotMarker::Matrix& QwtIndPlotMarker::Matrix::operator=(const Matrix& m){
    if((m_rows!=m.rows())||(m_cols!=m.cols())) {
        resize(m.rows(),m.cols());
    }

    for(unsigned long i=0;i<m.rows();i++){
        for (unsigned long j = 0; j < m.cols(); j++) {
        m_data[i*m_cols+j]=m(i,j);
        }
    }
}
*/


 /* bool& QwtIndPlotMarker::Matrix::operator() (unsigned long row, unsigned long col) const */
/* { */
 /*    if ((row >= m_rows || col >= m_cols)||(!m_data)) */
 /*        throw std::out_of_range("operator(): invalid index for m_data"); */
 /*    return m_data[m_cols*row + col]; */
/* } */

bool QwtIndPlotMarker::Matrix::operator() (unsigned long row, unsigned long col) const
{
    if ((row >= m_rows || col >= m_cols)||(!m_data))
        throw std::out_of_range("operator(): invalid index for m_data");
    return m_data[m_cols*row + col];
}

inline void QwtIndPlotMarker::Matrix::init() {
    for (unsigned long i = 0; i < m_cols*m_rows; i++) {
        m_data[i] = false;
    }
}

inline void QwtIndPlotMarker::Matrix::reset() {
    delete[] m_data;
    m_data = 0;
    m_rows = 0;
    m_cols = 0;
}

int QwtIndPlotMarker::Matrix::resize(unsigned long row, unsigned long col) {
    delete[] m_data;

    m_data = new bool[row * col]();
    if(m_data) {
        m_rows = row;
        m_cols = col;
        init();
        return 0;
    }
    else {
        throw std::bad_alloc();
        return -1;
    }
}

unsigned long QwtIndPlotMarker::Matrix::rows() const {
    return m_rows;
}

unsigned long QwtIndPlotMarker::Matrix::cols() const {
    return m_cols;
}
uintptr_t QwtIndPlotMarker::Matrix::canvasId() const {
    return m_canvasId;
}
void QwtIndPlotMarker::Matrix::setCanvasId(uintptr_t vcanvasId){
    m_canvasId = vcanvasId;
}
void QwtIndPlotMarker::Matrix::set(unsigned long row, unsigned long col, bool value) {
    m_data[m_rows*row+col] = value;
}

void QwtIndPlotMarker::Matrix::drawnAt(unsigned long row, unsigned long col, unsigned long ysize, unsigned long xsize){
    if((row>=m_rows)||(col>=m_cols))
        return;
    for (unsigned long y = row; (y < (ysize+row))&&(y<m_rows); y++) {
        for (unsigned long x = col; (x < (xsize+col))&&(x<m_cols); x++) {
            m_data[m_cols*y+x] = true;
        }
    }
}
bool QwtIndPlotMarker::Matrix::isFree(unsigned long row, unsigned long col, unsigned long ysize, unsigned long xsize) const {
    bool ret = true;
    if((row>=m_rows)||(col>=m_cols))
        return ret;
    for (unsigned long y = row; (y < (ysize+row))&&ret&&(y<m_rows);y++) {
        for (unsigned long x = col; (x < (xsize+col))&&ret&&(x<m_cols); x++) {
            ret = !(m_data[m_cols*y+x]);
        }
    }
    return ret;
}

//! Sets alignment to Qt::AlignCenter, and style to QwtIndPlotMarker::NoLine
QwtIndPlotMarker::QwtIndPlotMarker( const QString &title ):
    QwtPlotMarker(QwtText(title))
{
    m_spacingX=0;
    m_spacingY=0;
    if(!this->m_drawnPixels) {
        this->m_drawnPixels = new Matrix(1,1);
    }
}

//! Sets alignment to Qt::AlignCenter, and style to QwtIndPlotMarker::NoLine
QwtIndPlotMarker::QwtIndPlotMarker( const QwtText &title ):
    QwtPlotMarker(title)
{
    m_spacingX=0;
    m_spacingY=0;
    if(!m_drawnPixels) {
        m_drawnPixels = new Matrix(1,1);
    }
}

//! Destructor
QwtIndPlotMarker::~QwtIndPlotMarker()
{
}

/*!
  Draw the marker

  \param painter Painter
  \param xMap x Scale Map
  \param yMap y Scale Map
  \param canvasRect Contents rectangle of the canvas in painter coordinates
  */
void QwtIndPlotMarker::draw( QPainter *painter,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRectF &canvasRect ) const
{
    const QPointF pos( xMap.transform( this->xValue() ),
            yMap.transform( this->yValue() ) );

    // draw lines

    drawLines( painter, canvasRect, pos );

    // draw symbol
    if ( this->symbol() &&
            ( this->symbol()->style() != QwtSymbol::NoSymbol ) )
    {
        const QSizeF sz = this->symbol()->size();

        const QRectF clipRect = canvasRect.adjusted(
                -sz.width(), -sz.height(), sz.width(), sz.height() );

        if ( clipRect.contains( pos ) )
            this->symbol()->drawSymbol( painter, pos );
    }

    drawLabel( painter, canvasRect, pos );
}

/*!
  Align and draw the text label of the marker
  This is a modified version of the normal PlotMarker drawLabel method.
  It checks first if the space where we want to draw a label is free and if not
  it will indend the label accordingly.
  At the moment this indending is indending is only done if the label is written
  horizontally, in the future we may want to expand the funtionality to vertical labels.

  \param painter Painter
  \param canvasRect Contents rectangle of the canvas in painter coordinates
  \param pos Position of the marker, translated into widget coordinates

  \sa drawLabel(), QwtSymbol::drawSymbol()
  */
void QwtIndPlotMarker::drawLabel( QPainter *painter,
        const QRectF &canvasRect, const QPointF &pos ) const
{
    if(reinterpret_cast<uintptr_t>(painter)!=m_drawnPixels->canvasId()){
        m_drawnPixels->setCanvasId(reinterpret_cast<uintptr_t>(painter));
        m_drawnPixels->init();
    }
    //We have a wrong size of the Canvas, it probably was resized, so resize our table too
    if(((*m_drawnPixels).cols()!=canvasRect.width())||((*m_drawnPixels).rows()!=canvasRect.height())){
        m_drawnPixels->resize(canvasRect.height(), canvasRect.width());
    }
    if ( this->label().isEmpty() )
        return;

    Qt::Alignment align = labelAlignment();
    QPointF alignPos = pos;

    QSizeF symbolOff( 0, 0 );

    switch ( this->lineStyle() )
    {
        case QwtPlotMarker::VLine:
            {
                // In VLine-style the y-position is pointless and
                // the alignment flags are relative to the canvas

                if ( this->labelAlignment() & Qt::AlignTop )
                {
                    alignPos.setY( canvasRect.top() );
                    align &= ~Qt::AlignTop;
                    align |= Qt::AlignBottom;
                }
                else if ( this->labelAlignment() & Qt::AlignBottom )
                {
                    // In HLine-style the x-position is pointless and
                    // the alignment flags are relative to the canvas

                    alignPos.setY( canvasRect.bottom() - 1 );
                    align &= ~Qt::AlignBottom;
                    align |= Qt::AlignTop;
                }
                else
                {
                    alignPos.setY( canvasRect.center().y() );
                }
                break;
            }
        case QwtPlotMarker::HLine:
            {
                if ( this->labelAlignment() & Qt::AlignLeft )
                {
                    alignPos.setX( canvasRect.left() );
                    align &= ~Qt::AlignLeft;
                    align |= Qt::AlignRight;
                }
                else if ( this->labelAlignment() & Qt::AlignRight )
                {
                    alignPos.setX( canvasRect.right() - 1 );
                    align &= ~Qt::AlignRight;
                    align |= Qt::AlignLeft;
                }
                else
                {
                    alignPos.setX( canvasRect.center().x() );
                }
                break;
            }
        default:
            {
                if ( this->symbol() &&
                        ( this->symbol()->style() != QwtSymbol::NoSymbol ) )
                {
                    symbolOff = this->symbol()->size() + QSizeF( 1, 1 );
                    symbolOff /= 2;
                }
            }
    }

    qreal pw2 = this->linePen().widthF() / 2.0;
    if ( pw2 == 0.0 )
        pw2 = 0.5;

    /* const int spacing = this->spacing(); */
    const int spacingX = m_spacingX;
    const int spacingY = m_spacingY;

    const qreal xOff = qMax( pw2, symbolOff.width() );
    const qreal yOff = qMax( pw2, symbolOff.height() );

    const QSizeF textSize = this->label().textSize( painter->font() );

    if ( align & Qt::AlignLeft )
    {
        alignPos.rx() -= xOff + spacingX;
        if ( this->labelOrientation() == Qt::Vertical )
            alignPos.rx() -= textSize.height();
        else
            alignPos.rx() -= textSize.width();
    }
    else if ( align & Qt::AlignRight )
    {
        alignPos.rx() += xOff + spacingX;
    }
    else
    {
        if ( this->labelOrientation() == Qt::Vertical )
            alignPos.rx() -= textSize.height() / 2;
        else
            alignPos.rx() -= textSize.width() / 2;
    }

    if ( align & Qt::AlignTop )
    {
        alignPos.ry() -= yOff + spacingY;
        if ( this->labelOrientation() != Qt::Vertical )
            alignPos.ry() -= textSize.height();
    }
    else if ( align & Qt::AlignBottom )
    {
        alignPos.ry() += yOff + spacingY;
        if ( this->labelOrientation() == Qt::Vertical )
            alignPos.ry() += textSize.width();
    }
    else
    {
        if ( this->labelOrientation() == Qt::Vertical )
            alignPos.ry() += textSize.width() / 2;
        else
            alignPos.ry() -= textSize.height() / 2;
    }
    //Check to see if the calculated space is free, else shift
    while(!m_drawnPixels->isFree(alignPos.y(),alignPos.x(),textSize.height(),textSize.width())) {
        alignPos.ry() += textSize.height();
    }
    painter->translate( alignPos.x(), alignPos.y() );
    if ( this->labelOrientation() == Qt::Vertical )
        painter->rotate( -90.0 );

    const QRectF textRect( 0, 0, textSize.width(), textSize.height() );
    this->label().draw( painter, textRect );
    m_drawnPixels->drawnAt(alignPos.y(),alignPos.x(),textSize.height()+spacingY,textSize.width()+spacingX);
}

/*!
  \brief Set the spacing

  When the label is not centered on the marker position, the spacing
  is the distance between the position and the label.

  \param spacing Spacing
  \sa spacing(), setLabelAlignment()
  */
void QwtIndPlotMarker::setSpacing( int spacing )
{
    if ( spacing < 0 )
        spacing = 0;

    if (( spacing == this->spacingX() )&&( spacing == this->spacingY() ))
        return;

    m_spacingX = spacing;
    m_spacingY = spacing;

    itemChanged();
}

/*!
  \brief Set the spacing

  When the label is not centered on the marker position, the spacing
  is the distance between the position and the label.

  \param spacing Spacing
  \sa spacing(), setLabelAlignment()
  */
void QwtIndPlotMarker::setSpacing( int spacingX, int spacingY )
{
    if ( spacingX < 0 )
        spacingX = 0;
    if ( spacingY < 0 )
        spacingY = 0;


    if (( spacingX == this->spacingX() )&&( spacingY == this->spacingY() ))
        return;

    m_spacingX = spacingX;
    m_spacingY = spacingY;

    itemChanged();
}

/*!
  \return the spacing
  \sa setSpacing()
  */
/* int QwtIndPlotMarker::spacing() const */
/* { */
/*     return d_data->spacing; */
/* } */

/*!
  \return the spacing X
  \sa setSpacing()
  */
int QwtIndPlotMarker::spacingX() const
{
    return m_spacingX;
}

/*!
  \return the spacing Y
  \sa setSpacing()
  */
int QwtIndPlotMarker::spacingY() const
{
    return m_spacingY;
}

void QwtIndPlotMarker::resetDrawnLabels(){
    m_drawnPixels->init();
}

