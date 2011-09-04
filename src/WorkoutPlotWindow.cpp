/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include "WorkoutPlotWindow.h"

WorkoutPlotWindow::WorkoutPlotWindow(MainWindow *mainWindow) :
    GcWindow(mainWindow), mainWindow(mainWindow)
{
    setContentsMargins(0,0,0,0);
    setInstanceName("RT Plot");
    setControls(NULL);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(2,2,2,2);
    ergPlot = new ErgFilePlot(0);
    layout->addWidget(ergPlot);

    connect(mainWindow, SIGNAL(setNow(long)), this, SLOT(setNow(long)));
    connect(mainWindow, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));

}

void
WorkoutPlotWindow::ergFileSelected(ErgFile *f)
{
    ergPlot->setData(f);
    ergPlot->replot();
}

void
WorkoutPlotWindow::setNow(long now)
{
    ergPlot->setNow(now);
}
