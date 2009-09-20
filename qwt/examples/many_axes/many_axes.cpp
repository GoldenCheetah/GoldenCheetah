#include <qapplication.h>
#include "mainwindow.h"

//-----------------------------------------------------------------
//              many_axes.cpp
//
//      An example that demonstrates a plot with many axes.
//-----------------------------------------------------------------

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();
    return a.exec(); 
}
