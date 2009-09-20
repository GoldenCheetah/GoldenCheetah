#include <qapplication.h>
#include <qmainwindow.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include "plot.h"

class MainWindow: public QMainWindow
{
public:
    MainWindow(QWidget * = NULL);

private:
    Plot *d_plot;
};

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent)
{
    d_plot = new Plot(this);

    setCentralWidget(d_plot);

    QToolBar *toolBar = new QToolBar(this);

    QToolButton *btnSpectrogram = new QToolButton(toolBar);
    QToolButton *btnContour = new QToolButton(toolBar);
    QToolButton *btnPrint = new QToolButton(toolBar);

#if QT_VERSION >= 0x040000
    btnSpectrogram->setText("Spectrogram");
    //btnSpectrogram->setIcon(QIcon());
    btnSpectrogram->setCheckable(true);
    btnSpectrogram->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->addWidget(btnSpectrogram);

    btnContour->setText("Contour");
    //btnContour->setIcon(QIcon());
    btnContour->setCheckable(true);
    btnContour->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->addWidget(btnContour);

	btnPrint->setText("Print");
    btnPrint->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->addWidget(btnPrint);
#else
    btnSpectrogram->setTextLabel("Spectrogram");
    //btnSpectrogram->setPixmap(zoom_xpm);
    btnSpectrogram->setToggleButton(true);
    btnSpectrogram->setUsesTextLabel(true);

    btnContour->setTextLabel("Contour");
    //btnContour->setPixmap(zoom_xpm);
    btnContour->setToggleButton(true);
    btnContour->setUsesTextLabel(true);

    btnPrint->setTextLabel("Print");
    btnPrint->setUsesTextLabel(true);
#endif

    addToolBar(toolBar);

    connect(btnSpectrogram, SIGNAL(toggled(bool)), 
        d_plot, SLOT(showSpectrogram(bool)));
    connect(btnContour, SIGNAL(toggled(bool)), 
        d_plot, SLOT(showContour(bool)));
    connect(btnPrint, SIGNAL(clicked()), 
        d_plot, SLOT(printPlot()) );

#if QT_VERSION >= 0x040000
    btnSpectrogram->setChecked(true);
    btnContour->setChecked(false);
#else
    btnSpectrogram->setOn(true);
    btnContour->setOn(false);
#endif
}

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    MainWindow mainWindow;
#if QT_VERSION < 0x040000
    a.setMainWidget(&mainWindow);
#endif

    mainWindow.resize(600,400);
    mainWindow.show();

    return a.exec(); 
}
