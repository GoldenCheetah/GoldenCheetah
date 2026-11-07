/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QwtPlot>

class QwtWheel;
class QwtPlotMarker;
class QwtPlotCurve;

class Plot : public QwtPlot
{
    Q_OBJECT

  public:
    Plot( bool parametric, QWidget* parent = nullptr );

    bool eventFilter( QObject*, QEvent* ) override;

  public Q_SLOTS:
    void setOverlaying( bool );
    void setParametric( const QString& );
    void setBoundaryCondition( const QString& );
    void setClosed( bool );

#ifndef QT_NO_PRINTER
    void printPlot();
#endif

  private:
    void showCurve( QwtPlotItem*, bool on );
    void updateMarker( int axis, double base );
    void scrollLeftAxis( double );
    void legendChecked( const QVariant&, bool on );

    QwtPlotMarker* m_marker;
    QwtPlotCurve* m_curve;
    QwtWheel* m_wheel;

    int m_boundaryCondition;
};
