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

#include "ViewSelection.h"
#include "TrainTool.h"
#include "TrainTabs.h"
#include "MainWindow.h"
#include "Settings.h"
#include "Units.h"
#include <QApplication>
#include <QtGui>
#include <QFont>
#include <QFontMetrics>

ViewSelection::ViewSelection(const MainWindow *parent, const int homeView) :
QComboBox((QWidget *)parent),
main(parent),
homeView(homeView)
{
    // don't make a fat ugly box...
    setFixedHeight(((double)(QFontMetrics(font()).height()) * 1.6));

    // set drop-down list
    addItem(tr("Ride Analysis View"), VIEW_ANALYSIS);
    addItem(tr("Train and Racing View"), VIEW_TRAIN);

    // Default to home view
    if (homeView == VIEW_ANALYSIS) setCurrentIndex(0);
    if (homeView == VIEW_TRAIN) setCurrentIndex(1);

    // set to this view
    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(changeView(int)));
}

void
ViewSelection::changeView(int index)
{
    int view = itemData(index).toInt();

    if (view != homeView) {
        ((MainWindow *)main)->selectView(view);
    }

    // now reset back to the homeView!
    if (homeView == VIEW_ANALYSIS) setCurrentIndex(0);
    if (homeView == VIEW_TRAIN) setCurrentIndex(1);
}
