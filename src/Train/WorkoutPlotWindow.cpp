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
#include "Context.h"

WorkoutPlotWindow::WorkoutPlotWindow(Context *context) :
    GcWindow(context), context(context)
{
    setContentsMargins(0,0,0,0);
    setControls(NULL);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(2,2,2,2);
    ergPlot = new ErgFilePlot(context);
    layout->addWidget(ergPlot);

    connect(context, SIGNAL(setNow(long)), this, SLOT(setNow(long)));
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), ergPlot, SLOT(performancePlot(RealtimeData)));
    connect(context, SIGNAL(start()), ergPlot, SLOT(start()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
}

void
WorkoutPlotWindow::ergFileSelected(ErgFile *f)
{
    // rename window to workout name
    if (f && f->Name != "") setProperty("subtitle", f->Name);
    else setProperty("subtitle", "");

    ergPlot->setData(f);
    ergPlot->replot();
}

void
WorkoutPlotWindow::setNow(long now)
{
    ergPlot->setNow(now);
}

void
WorkoutPlotWindow::configChanged(qint32)
{
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));
    repaint();
}
