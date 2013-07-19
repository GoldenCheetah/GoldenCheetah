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

#include "GoldenCheetah.h"
#include "GcWindowRegistry.h"
#include "GcWindowTool.h"
#include <QtGui>

GcWindowTool::GcWindowTool(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    setLayout(mainLayout);

    chartTree = new QTreeWidget;
    mainLayout->addWidget(chartTree);

    // setup the chart list
    chartTree->setColumnCount(1);
    chartTree->setSelectionMode(QAbstractItemView::SingleSelection);
    chartTree->header()->hide();
    chartTree->setAlternatingRowColors (false);
    chartTree->setIndentation(5);
    chartTree->setDragDropMode(QAbstractItemView::DragOnly);
    chartTree->setFrameStyle(QFrame::NoFrame);
    allCharts = new QTreeWidgetItem(chartTree, ROOT_TYPE);
    allCharts->setText(0, tr("Chart"));

    // initialise the metrics catalogue and user selector
    for (int i = 0; GcWindows[i].id != GcWindowTypes::None; i++) {

        // add to the tree
        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(allCharts, static_cast<int>(GcWindows[i].id));
        add->setText(0, GcWindows[i].name);
        add->setFlags(add->flags() | Qt::ItemIsDragEnabled);
    }
    chartTree->expandItem(allCharts);

    connect(chartTree,SIGNAL(itemSelectionChanged()), this, SLOT(chartTreeWidgetSelectionChanged()));
}

/*----------------------------------------------------------------------
 * Selections Made
 *----------------------------------------------------------------------*/

void
GcWindowTool::chartTreeWidgetSelectionChanged()
{
    chartSelected();
}
