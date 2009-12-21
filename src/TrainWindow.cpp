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

#include "TrainWindow.h"
#include "TrainTool.h"
#include "TrainTabs.h"
#include "ViewSelection.h"
#include "MainWindow.h"
#include "Settings.h"
#include "Units.h"
#include <QApplication>
#include <QtGui>

TrainWindow::TrainWindow(MainWindow *parent, const QDir &home) : QWidget(parent), home(home), main(parent)
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    // LeftSide
    trainTool = new TrainTool(parent, home);

    // Right side
    trainTabs = new TrainTabs(parent, trainTool, home);

    // setup splitter
    splitter = new QSplitter;
    splitter->addWidget(trainTool);
    splitter->setCollapsible(0, true);
    splitter->addWidget(trainTabs);
    splitter->setCollapsible(1, true);

    // splitter sizing
    QVariant splitterSizes = settings->value(GC_TRAIN_SPLITTER_SIZES);
    if (splitterSizes != QVariant())
        splitter->restoreState(splitterSizes.toByteArray());
    else {
        QList<int> sizes;
        sizes.append(250);
        sizes.append(390);
        splitter->setSizes(sizes);
    }

    // add to the layout
    mainLayout->addWidget(splitter);

    // watch resize of splitter and save away
    connect(splitter, SIGNAL(splitterMoved(int,int)),
            this, SLOT(splitterMoved()));
}

void
TrainWindow::splitterMoved()
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    settings->setValue(GC_TRAIN_SPLITTER_SIZES, splitter->saveState());
}
