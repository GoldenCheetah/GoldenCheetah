#include <qapplication.h>
#include <qmainwindow.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include "plot.h"

class MainWindow: public QMainWindow
{
public:
    MainWindow(QWidget *parent = NULL):
        QMainWindow(parent)
    {   
        Plot *plot = new Plot(this);
        setCentralWidget(plot);

        QToolBar *toolBar = new QToolBar(this);

        QToolButton *btnLoad = new QToolButton(toolBar);
        
#if QT_VERSION >= 0x040000
        btnLoad->setText("Load SVG");
        btnLoad->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        toolBar->addWidget(btnLoad);
#else
        btnLoad->setTextLabel("Load SVG");
        btnLoad->setUsesTextLabel(true);
#endif

        addToolBar(toolBar);

        connect(btnLoad, SIGNAL(clicked()), plot, SLOT(loadSVG())); 
    }
};

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    MainWindow w;
#if QT_VERSION < 0x040000
    a.setMainWidget(&w);
#endif
    w.resize(600,400);
    w.show();

    int rv = a.exec();
    return rv;
}
