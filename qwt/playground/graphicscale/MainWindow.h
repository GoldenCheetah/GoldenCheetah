/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QMainWindow>

class Canvas;
class QPainterPath;

class MainWindow : public QMainWindow
{
  public:
    MainWindow();
    ~MainWindow() override;

  private:
    void openFileDialog();
    void loadSVG( const QString& );
    void loadPath( const QPainterPath& );

    Canvas* m_canvas[2];
};
