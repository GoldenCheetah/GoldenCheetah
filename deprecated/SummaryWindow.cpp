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

#include "SummaryWindow.h"

SummaryWindow::SummaryWindow(Context *context) :
    GcChartWindow(context), context(context)
{
    setControls(NULL);
    setRideItem(NULL);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->setHandleWidth(1);
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setSpacing(0);
    vlayout->setContentsMargins(1,1,1,1);
    vlayout->addWidget(splitter);
    setChartLayout(vlayout);

    rideSummary = new RideSummaryWindow(context);
    rideMetadata = new RideMetadata(context);

    splitter->addWidget(rideSummary);
    splitter->addWidget(rideMetadata);

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideItemChanged()));
}

void
SummaryWindow::rideItemChanged()
{
    rideSummary->setProperty("ride", property("ride"));
    rideMetadata->setProperty("ride", property("ride"));

    RideItem *rideItem = myRideItem;
    if (rideItem) setSubTitle(rideItem->dateTime.toString(tr("dddd MMMM d, yyyy, hh:mm")));
}
