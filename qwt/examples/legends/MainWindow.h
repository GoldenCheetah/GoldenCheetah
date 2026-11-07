/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QMainWindow>

class Plot;
class Panel;

class MainWindow : public QMainWindow
{
  public:
    MainWindow( QWidget* parent = 0 );

  private:
    void updatePlot();
    void exportPlot();

    Plot* m_plot;
    Panel* m_panel;
};
