/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_MATRIX_RASTER_DATA_H
#define QWT_MATRIX_RASTER_DATA_H

#include "qwt_global.h"
#include "qwt_raster_data.h"
#include <memory>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template< typename T > class QVector;
#endif

/*!
   \brief A class representing a matrix of values as raster data

   QwtMatrixRasterData implements an interface for a matrix of
   equidistant values, that can be used by a QwtPlotRasterItem.
   It implements a couple of resampling algorithms, to provide
   values for positions, that or not on the value matrix.
 */
class QWT_EXPORT QwtMatrixRasterData : public QwtRasterData
{
  public:
    /*!
       \brief Resampling algorithm
       The default setting is NearestNeighbour;
     */
    enum ResampleMode
    {
        /*!
           Return the value from the matrix, that is nearest to the
           the requested position.
         */
        NearestNeighbour,

        /*!
           Interpolate the value from the distances and values of the
           4 surrounding values in the matrix,
         */
        BilinearInterpolation,

        /*!
           Interpolate the value from the 16 surrounding values in the
           matrix using hermite bicubic interpolation
         */
        BicubicInterpolation
    };

    QwtMatrixRasterData();
    ~QwtMatrixRasterData() override;

    void setResampleMode(ResampleMode mode);
    ResampleMode resampleMode() const;

    void setInterval( Qt::Axis, const QwtInterval& );
    QwtInterval interval( Qt::Axis axis) const override final;

    void setValueMatrix( const QVector< double >& values, int numColumns );
    const QVector< double > valueMatrix() const;

    void setValue( int row, int col, double value );

    int numColumns() const;
    int numRows() const;

    QRectF pixelHint( const QRectF& ) const override;

    double value( double x, double y ) const override;

  private:
    void update();

    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

#endif
