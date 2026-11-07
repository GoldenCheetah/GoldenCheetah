/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QMainWindow>

class Plot;
class QLabel;
class Settings;

class MainWindow : public QMainWindow
{
  public:
    MainWindow( QWidget* parent = nullptr );
    bool eventFilter( QObject*, QEvent* ) override;

  private:
    void applySettings( const Settings& );

    Plot* m_plot;
    QLabel* m_frameCount;
};
