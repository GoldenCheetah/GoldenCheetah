/*****************************************************************************
 * Qwt Examples
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#pragma once

#include <QMainWindow>

class QwtPlotZoomer;
class QwtPlotPicker;
class QwtPlotPanner;
class Plot;
class QPolygon;

class MainWindow : public QMainWindow
{
  public:
    MainWindow( QWidget* parent = 0 );

  private:
    void print();
    void exportDocument();
    void enableZoomMode( bool );

    void moved( const QPoint& );
    void showInfo();

    Plot* m_plot;

    QwtPlotZoomer* m_zoomer[2];
    QwtPlotPicker* m_picker;
    QwtPlotPanner* m_panner;
};
