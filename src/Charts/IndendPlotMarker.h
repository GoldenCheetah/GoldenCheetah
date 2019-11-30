
/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef IND_PLOT_MARKER_H
#define IND_PLOT_MARKER_H
#define maxLines 30

#include <qpen.h>
#include <qfont.h>
#include <qstring.h>
#include <qbrush.h>
#include "qwt_global.h"
#include "qwt_plot_item.h"
#include <qwt_plot_marker.h>
#include <stdint.h>

/*!
  \brief A class for drawing indended markers

  A marker can be a horizontal line, a vertical line,
  a symbol, a label or any combination of them, which can
  be drawn around a center point inside a bounding rectangle.

  The setSymbol() member assigns a symbol to the marker.
  The symbol is drawn at the specified point.

  With setLabel(), a label can be assigned to the marker.
  With this derived class the label will be indended, so they don't overlap
  each other.
  At the moment this indending is only done for labels written horizontally.
  In the future we may want to expand the functionality to incorporate vertical labels.
  The setLabelAlignment() member specifies where the label is
  drawn. All the Align*-constants in Qt::AlignmentFlags (see Qt documentation)
  are valid. The interpretation of the alignment depends on the marker's
  line style. The alignment refers to the center point of
  the marker, which means, for example, that the label would be printed
  left above the center point if the alignment was set to
  Qt::AlignLeft | Qt::AlignTop.

  \note QwtPlotTextLabel is intended to align a text label
        according to the geometry of canvas
        ( unrelated to plot coordinates )
*/

class QwtIndPlotMarker: public QwtPlotMarker
{
public:

    explicit QwtIndPlotMarker( const QString &title = QString::null );
    explicit QwtIndPlotMarker( const QwtText &title );

    virtual ~QwtIndPlotMarker();

    /** Set x and y spacing different */
    void setSpacing( int spacingX, int spacingY);
    /** Set both dimensions of spacing at once */
    void setSpacing( int );
    /** Returns the X spacing of a Marker */
    int spacingX() const;
    /** Returns the Y spacing of a Marker */
    int spacingY() const;
    /** Reset where we already have drawn Labels in the plot*/
    static void resetDrawnLabels();

    virtual void draw( QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRectF & ) const;

protected:
    virtual void drawLabel( QPainter *,
        const QRectF &, const QPointF & ) const;

private:

    int m_spacingX;
    int m_spacingY;

    class Matrix {
        public:
            Matrix(unsigned long rows, unsigned long cols);
            /** Subscript operator */
            bool  operator() (unsigned long row, unsigned long col) const;
            /** Destructor */
            ~Matrix();
            /** C++11 Move constructor */
            /* Matrix(Matrix&&); */
            /**Copy assignment operator */
            //UNUSED Matrix& operator=(const Matrix&);
            /** C++11 Move assignment operator */
            /* Matrix& operator=(Matrix&&); */
            void set(unsigned long row, unsigned long col, bool value);
            /** init the Memory */
            void init();
            /** reset the Matrix (free memory)*/
            void reset();
            /** Resizes the Matrix if needed according to row, col */
            int resize(unsigned long row, unsigned long col);
            /** Returns the number of cols */
            unsigned long cols() const;
            /** Returns the number of rows */
            unsigned long rows() const;
            /** Returns the canvasId, needed to keep tracks if we're on a new plot */
            uintptr_t canvasId() const;
            /** Sets a new canvasId */
            void setCanvasId(uintptr_t canvasId);
            /** Marks which pixels already have been drawn at */
            void drawnAt(unsigned long x, unsigned long y, unsigned long xsize, unsigned long ysize);
            /** Returns if the space is already drawn at */
            bool isFree(unsigned long x, unsigned long y, unsigned long xsize, unsigned long ysize) const;
        private:
            unsigned long m_rows; ///< number of rows in pixels
            unsigned long m_cols; ///< number of cols in pixels
            uintptr_t m_canvasId; ///< internal canvasId, needed check if we're on a new plot
            //holds data of where is drawn dimension 1: rows dimension 2: cols
            //e.g. m_data[y*m_rows+cols]
            // y == rows, x == cols
            bool* m_data;
    };
    static Matrix* m_drawnPixels; ///< Matrix of drawn/free Pixels
};

#endif
