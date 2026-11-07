/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QFrame>
#include <memory>

class QwtPlot;

class PlotMatrix : public QFrame
{
  public:
    PlotMatrix( int rows, int columns, QWidget* parent = nullptr );
    ~PlotMatrix() override;

    int numRows() const;
    int numColumns() const;

    QwtPlot* plotAt( int row, int column );
    const QwtPlot* plotAt( int row, int column ) const;

    void setAxisVisible( int axisId, bool tf = true );
    bool isAxisVisible( int axisId ) const;

    void setAxisScale( int axisId, int rowOrColumn,
        double min, double max, double step = 0 );

  protected:
    void updateLayout();

  private:
    void syncScales();
    void alignAxes( int rowOrColumn, int axis );
    void alignScaleBorder( int rowOrColumn, int axis );

    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};
