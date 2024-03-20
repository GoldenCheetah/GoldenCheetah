/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "MetadataWindow.h"
#include "Colors.h"
#include "HelpWhatsThis.h"

MetadataWindow::MetadataWindow(Context *context) :
    GcChartWindow(context), context(context)
{
    setControls(NULL);
    setRideItem(NULL);
    setContentsMargins(0,0,0,0);

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setSpacing(0);
    rideMetadata = new RideMetadata(context);
    QFont font;
    font.setPointSize(font.pointSize());
    rideMetadata->setFont(font);
    rideMetadata->setContentsMargins(20,0,20,20);
    vlayout->addWidget(rideMetadata);
    setChartLayout(vlayout);

    HelpWhatsThis *help = new HelpWhatsThis(rideMetadata);
    rideMetadata->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Details));


    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideItemChanged()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // set colors
    configChanged(CONFIG_APPEARANCE);
}

void
MetadataWindow::rideItemChanged()
{
    rideMetadata->setProperty("ride", property("ride"));
    if (myRideItem) setIsBlank(false);
    else setIsBlank(true);
}

void
MetadataWindow::configChanged(qint32)
{
    setProperty("color", GColor(GCol::PLOTBACKGROUND));
}
