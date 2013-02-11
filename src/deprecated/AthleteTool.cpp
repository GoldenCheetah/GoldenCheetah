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
#include "AthleteTool.h"
#include <QtGui>

AthleteTool::AthleteTool(QString path, QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    setLayout(mainLayout);

    athleteTree = new QTreeWidget;
    mainLayout->addWidget(athleteTree);

    // setup the athlete list
    athleteTree->setColumnCount(1);
    athleteTree->setSelectionMode(QAbstractItemView::SingleSelection);
    athleteTree->header()->hide();
    athleteTree->setAlternatingRowColors (false);
    athleteTree->setIndentation(5);
    athleteTree->setFrameStyle(QFrame::NoFrame);
    athleteTree->setDragDropMode(QAbstractItemView::DragOnly);
    allAthletes = new QTreeWidgetItem(athleteTree, ROOT_TYPE);
    allAthletes->setText(0, tr("Athletes"));

    // get a dir list of athletes
    // initialise the metrics catalogue and user selector
    foreach (QString name, QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {

        // add to the tree
        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(allAthletes, 0);
        add->setText(0, name);
        add->setFlags(add->flags() | Qt::ItemIsDragEnabled);
    }
    athleteTree->expandItem(allAthletes);

    connect(athleteTree,SIGNAL(itemSelectionChanged()), this, SLOT(athleteTreeWidgetSelectionChanged()));
}

/*----------------------------------------------------------------------
 * Selections Made
 *----------------------------------------------------------------------*/

void
AthleteTool::athleteTreeWidgetSelectionChanged()
{
    athleteSelected();
}
