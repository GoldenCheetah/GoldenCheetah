/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QwtPlot>

class TVPlot : public QwtPlot
{
    Q_OBJECT

  public:
    TVPlot( QWidget* = nullptr );

  public Q_SLOTS:
    void setMode( int );
    void exportPlot();

  private:
    void populate();
    void showItem( const QVariant&, bool on );
};
