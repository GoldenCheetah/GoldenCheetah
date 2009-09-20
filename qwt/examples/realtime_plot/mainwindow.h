#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_ 1

#include <qmainwindow.h>
#if QT_VERSION < 0x040000
#include <qtoolbutton.h>
#else
#include <qaction.h>
#endif

class QSpinBox;
class QPushButton;
class RandomPlot;
class Counter;

class MainWindow: public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private slots:
    void showRunning(bool);
    void appendPoints(bool);

private:
    QToolBar *toolBar();
    void initWhatsThis();

private:
    Counter *d_randomCount;
    Counter *d_timerCount;
#if QT_VERSION < 0x040000
    QToolButton *d_startBtn;
    QToolButton *d_clearBtn;
#else
    QAction *d_startAction;
    QAction *d_clearAction;
#endif

    RandomPlot *d_plot;
};

#endif
