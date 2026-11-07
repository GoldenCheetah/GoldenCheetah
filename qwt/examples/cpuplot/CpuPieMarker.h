/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QwtPlotItem>

class CpuPieMarker : public QwtPlotItem
{
  public:
    CpuPieMarker();

    int rtti() const override;

    void draw( QPainter*, const QwtScaleMap&, const QwtScaleMap&,
        const QRectF& ) const override;
};
