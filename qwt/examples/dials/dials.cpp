#include <qapplication.h>
#include <qtabwidget.h>
#include "compass_grid.h"
#include "cockpit_grid.h"

//-----------------------------------------------------------------
//
//      dials.cpp -- A demo program featuring QwtDial and friends
//
//-----------------------------------------------------------------

int main (int argc, char **argv)
{
    QApplication a(argc, argv);

    QTabWidget tabWidget;
#if QT_VERSION < 0x040000
    tabWidget.addTab(new CompassGrid(&tabWidget), "Compass");
    tabWidget.addTab(new CockpitGrid(&tabWidget), "Cockpit");
    a.setMainWidget(&tabWidget);
#else
    tabWidget.addTab(new CompassGrid, "Compass");
    tabWidget.addTab(new CockpitGrid, "Cockpit");
#endif

    tabWidget.show();

    return a.exec();
}

