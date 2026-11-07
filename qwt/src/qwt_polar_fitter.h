/******************************************************************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_POLAR_FITTER_H
#define QWT_POLAR_FITTER_H

#include "qwt_global.h"
#include "qwt_curve_fitter.h"

#include <memory>

/*!
   \brief A simple curve fitter for polar points

   QwtPolarFitter adds equidistant points between 2 curve points,
   so that the connection gets rounded according to the nature of
   a polar plot.

   \sa QwtPolarCurve::setCurveFitter()
 */
class QWT_EXPORT QwtPolarFitter : public QwtCurveFitter
{
  public:
    QwtPolarFitter( int stepCount = 5 );
    ~QwtPolarFitter() override;

    void setStepCount( int size );
    int stepCount() const;

    QPolygonF fitCurve( const QPolygonF& ) const override;
    QPainterPath fitCurvePath( const QPolygonF& ) const override;

  private:
    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

#endif
