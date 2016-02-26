
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

#ifndef _GC_AthleteTool_h
#define _GC_AthleteTool_h 1
#include "GoldenCheetah.h"

#include "MainWindow.h"
#include "Season.h"
#include "RideMetric.h"
#include "LTMSettings.h"

#include <QDir>
#include <QtGui>

// tree widget types
#define ROOT_TYPE   1
#define CHART_TYPE 3

// list of athletes to add to a window
class AthleteTool : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        AthleteTool(QString path, QWidget *parent = 0);

        QTreeWidgetItem *selectedAthlete() { return athleteTree->selectedItems().first(); }

    signals:

        void athleteSelected();

    private slots:
        void athleteTreeWidgetSelectionChanged();

    private:

        QTreeWidget *athleteTree;
        QTreeWidgetItem *allAthletes;

};

#endif // _GC_AthleteTool_h

