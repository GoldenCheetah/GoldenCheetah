#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <qmainwindow.h>

class Plot;

class MainWindow: public QMainWindow
{
   Q_OBJECT

   public:
      MainWindow();

   private slots:
      void axisCheckBoxToggled(int axis);

   private:
      Plot *d_plot;
};

#endif // _MAINWINDOW_H_
