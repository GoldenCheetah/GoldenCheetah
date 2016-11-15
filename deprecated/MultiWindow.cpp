/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "MultiWindow.h"
#include "MainWindow.h"
#include "RealtimeData.h"
#include "math.h" // for round()
#include "Units.h" // for MILES_PER_KM

// Two current realtime device types supported are:
#include "ComputrainerController.h"
#include "ANTplusController.h"

#include "TrainTool.h"

MultiWindow::MultiWindow(MainWindow *parent, TrainTool *trainTool, const QDir &home)  :
QWidget(parent), home(home), main(parent), trainTool(trainTool)
{
}
