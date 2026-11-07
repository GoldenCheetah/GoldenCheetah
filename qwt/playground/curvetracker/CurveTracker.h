/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QwtPlotPicker>

class QwtPlotCurve;
class QLineF;

class CurveTracker : public QwtPlotPicker
{
  public:
    CurveTracker( QWidget* );

  protected:
    QwtText trackerTextF( const QPointF& ) const override;
    QRect trackerRect( const QFont& ) const override;

  private:
    QString curveInfoAt( const QwtPlotCurve*, const QPointF& ) const;
    QLineF curveLineAt( const QwtPlotCurve*, double x ) const;
};
