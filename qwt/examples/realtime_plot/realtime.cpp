#include <qapplication.h>
#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();
#if QT_VERSION < 0x040000
    a.setMainWidget(&w);
#endif

    return a.exec();
}
