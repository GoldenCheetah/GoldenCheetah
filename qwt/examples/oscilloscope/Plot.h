/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QwtPlot>
#include <QwtInterval>
#include <QElapsedTimer>

class QwtPlotCurve;
class QwtPlotMarker;
class QwtPlotDirectPainter;

class Plot : public QwtPlot
{
    Q_OBJECT

  public:
    Plot( QWidget* = nullptr );
    ~Plot() override;

    void start();
    void replot() override;

    bool eventFilter( QObject*, QEvent* ) override;

  public Q_SLOTS:
    void setIntervalLength( double );

  protected:
    void showEvent( QShowEvent* ) override;
    void resizeEvent( QResizeEvent* ) override;
    void timerEvent( QTimerEvent* ) override;

  private:
    void updateCurve();
    void incrementInterval();

    QwtPlotMarker* m_origin;
    QwtPlotCurve* m_curve;
    int m_paintedPoints;

    QwtPlotDirectPainter* m_directPainter;

    QwtInterval m_interval;
    int m_timerId;

    QElapsedTimer m_elapsedTimer;
};
