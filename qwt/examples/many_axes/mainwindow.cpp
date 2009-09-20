#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qsignalmapper.h>
#include "plot.h"
#include "mainwindow.h"

MainWindow::MainWindow()
{
    QFrame *w = new QFrame(this);

    // create the box of checkboxes

    QGroupBox *panel = new QGroupBox("Axes", w);
    QVBoxLayout *vLayout = new QVBoxLayout(panel);

    QStringList list;
    list << "yLeft" << "yLeft1" << "yLeft2" << "yLeft3";
    list << "yRight" << "yRight1" << "yRight2" << "yRight3";

    QSignalMapper* signalMapper = new QSignalMapper(this);

    QCheckBox* checkBox;
    for(int i = 0; i < 8; i++)
    {
       checkBox = new QCheckBox(list[i], panel);
       checkBox->setChecked(true);
       connect(checkBox, SIGNAL(clicked()), signalMapper, SLOT(map()));
       signalMapper->setMapping(checkBox, i);
       vLayout->addWidget(checkBox);
    }

    connect(signalMapper, SIGNAL(mapped(int)),
         this, SLOT(axisCheckBoxToggled(int)));
 
    vLayout->addStretch(10);

    // create the plot widget

    d_plot = new Plot(w);
    d_plot->replot();

    // put the widgets in a horizontal box

    QHBoxLayout *hLayout = new QHBoxLayout(w);
    hLayout->setMargin(0);
    hLayout->addWidget(panel, 10);
    hLayout->addWidget(d_plot, 10);

    setCentralWidget(w);
}

void MainWindow::axisCheckBoxToggled(int axis)
{
    d_plot->enableAxis(axis, !d_plot->axisEnabled(axis));
}
