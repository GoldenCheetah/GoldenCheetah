/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#include "UserChartWindow.h"

#include "Colors.h"
#include "AbstractView.h"
#include "RideFileCommand.h"
#include "Utils.h"
#include "AthleteTab.h"
#include "LTMTool.h"
#include "RideNavigator.h"
#include "ColorButton.h"
#include "MainWindow.h"
#include "UserChartData.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"

#include <limits>
#include <QScrollArea>
#include <QDialog>

UserChartWindow::UserChartWindow(Context *context, bool rangemode) : GcChartWindow(context), context(context), rangemode(rangemode), stale(true), last(NULL)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::Chart_User));

    chart = new UserChart(this, context, rangemode);

    // the config
    settingsTool_ = chart->settingsTool();
    setControls(settingsTool_);

    // layout
    QVBoxLayout *main=new QVBoxLayout();
    setChartLayout(main);
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);
    main->addWidget(chart);

    // when a ride is selected
    if (!rangemode) {
        connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(setRide(RideItem*)));
    } else {
        connect(this, SIGNAL(dateRangeChanged(DateRange)), SLOT(setDateRange(DateRange)));
        connect(context, SIGNAL(homeFilterChanged()), this, SLOT(refresh()));
        connect(context, SIGNAL(filterChanged()), this, SLOT(refresh()));
        connect(this, SIGNAL(perspectiveFilterChanged(QString)), this, SLOT(refresh()));
    }

    // need to refresh when perspective selected
    connect(this, SIGNAL(perspectiveChanged(Perspective*)), this, SLOT(refresh()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged()));

    connect(chart, SIGNAL(userChartConfigChanged()), this, SLOT(configChanged()));
}

void
UserChartWindow::configChanged()
{
    chart->setProperty("perspective", QVariant::fromValue<Perspective*>(myPerspective));

    // tell perspective that we changed, bgcolor might need tabbar updating?
    // but only if we have been placed onto the perspective (after initial create)
    if (myPerspective) myPerspective->userChartConfigChanged(this);
    update();
}

//
// Ride selected
//
void
UserChartWindow::setRide(RideItem *item)
{
    chart->setProperty("perspective", QVariant::fromValue<Perspective*>(myPerspective));
    chart->setRide(item);
}

 void
 UserChartWindow::setDateRange(DateRange d)
 {
    chart->setProperty("perspective", QVariant::fromValue<Perspective*>(myPerspective));
    chart->setDateRange(d);
 }

 void
 UserChartWindow::refresh()
 {
    // we get called when filters and perspectives change so lets set the property
    chart->setProperty("perspective", QVariant::fromValue<Perspective*>(myPerspective));

    if (!amVisible()) { stale=true; return; }

    chart->refresh();
}
