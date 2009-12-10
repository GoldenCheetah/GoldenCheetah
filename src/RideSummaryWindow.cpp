/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#include "RideSummaryWindow.h"
#include "MainWindow.h"
#include "RideItem.h"
#include "Zones.h"
#include <QtGui>
#include <assert.h>

RideSummaryWindow::RideSummaryWindow(MainWindow *mainWindow) :
    QWidget(mainWindow), mainWindow(mainWindow), ride(NULL)
{
    QVBoxLayout *vlayout = new QVBoxLayout;
    rideSummary = new QTextEdit(this);
    rideSummary->setReadOnly(true);
    vlayout->addWidget(rideSummary);
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(refresh()));
    setLayout(vlayout);
}

void
RideSummaryWindow::setData(RideItem *ride)
{
    this->ride = ride;
    refresh();
}

void
RideSummaryWindow::refresh()
{
    if (!ride) {
	rideSummary->clear();
        return;
    }
    rideSummary->setHtml(ride->htmlSummary());
    rideSummary->setAlignment(Qt::AlignCenter);
}

