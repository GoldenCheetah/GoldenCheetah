/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#include "AnalysisSidebar.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Units.h"
#include <QtGui>

// metadata support
#include "RideMetadata.h"
#include "SpecialFields.h"

AnalysisSidebar::AnalysisSidebar(Context *context) : QWidget(context->mainWindow), context(context)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);
    
    splitter = new GcSplitter(Qt::Vertical);
    mainLayout->addWidget(splitter);

    // calendar
    calendarWidget = new GcMultiCalendar(context);
    calendarItem = new GcSplitterItem(tr("Calendar"), iconFromPNG(":images/sidebar/calendar.png"), this);
    calendarItem->addWidget(calendarWidget);

    // Activity History
    rideNavigator = new RideNavigator(context, true);
    rideNavigator->setProperty("nomenu", true);

    // retrieve settings (properties are saved when we close the window)
    if (appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGS, "").toString() != "") {
        rideNavigator->setSortByIndex(appsettings->cvalue(context->athlete->cyclist, GC_SORTBY).toInt());
        rideNavigator->setSortByOrder(appsettings->cvalue(context->athlete->cyclist, GC_SORTBYORDER).toInt());
        rideNavigator->setGroupBy(appsettings->cvalue(context->athlete->cyclist, GC_NAVGROUPBY).toInt());
        rideNavigator->setColumns(appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGS).toString());
        rideNavigator->setWidths(appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGWIDTHS).toString());
    }

    QWidget *activityHistory = new QWidget(this);
    activityHistory->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // not on mac thanks
    activityHistory->setStyleSheet("padding: 0px; border: 0px; margin: 0px;");
#endif
    QVBoxLayout *activityLayout = new QVBoxLayout(activityHistory);
    activityLayout->setSpacing(0);
    activityLayout->setContentsMargins(0,0,0,0);
    activityLayout->addWidget(rideNavigator);

    activityItem = new GcSplitterItem(tr("Activities"), iconFromPNG(":images/sidebar/folder.png"), this);
    QAction *moreAnalAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    activityItem->addAction(moreAnalAct);
    connect(moreAnalAct, SIGNAL(triggered(void)), context->mainWindow, SLOT(analysisPopup()));
    activityItem->addWidget(activityHistory);

    // INTERVALS
    intervalSummaryWindow = new IntervalSummaryWindow(context);

    intervalSplitter = new QSplitter(this);
    intervalSplitter->setHandleWidth(1);
    intervalSplitter->setOrientation(Qt::Vertical);
    intervalSplitter->addWidget(context->athlete->intervalWidget);
    intervalSplitter->addWidget(intervalSummaryWindow);
    intervalSplitter->setFrameStyle(QFrame::NoFrame);
    intervalSplitter->setCollapsible(0, false);
    intervalSplitter->setCollapsible(1, false);

    intervalItem = new GcSplitterItem(tr("Intervals"), iconFromPNG(":images/mac/stop.png"), this);
    QAction *moreIntervalAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    intervalItem->addAction(moreIntervalAct);
    connect(moreIntervalAct, SIGNAL(triggered(void)), context->mainWindow, SLOT(intervalPopup()));
    intervalItem->addWidget(intervalSplitter);

    splitter->addWidget(calendarItem);
    splitter->addWidget(activityItem);
    splitter->addWidget(intervalItem);

    splitter->prepare(context->athlete->cyclist, "analysis");

    // GC signal
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
}

void
AnalysisSidebar::setRide(RideItem*ride)
{
    calendarWidget->setRide(ride);
}

void
AnalysisSidebar::close()
{
    appsettings->setCValue(context->athlete->cyclist, GC_SORTBY, rideNavigator->sortByIndex());
    appsettings->setCValue(context->athlete->cyclist, GC_SORTBYORDER, rideNavigator->sortByOrder());
    appsettings->setCValue(context->athlete->cyclist, GC_NAVGROUPBY, rideNavigator->groupBy());
    appsettings->setCValue(context->athlete->cyclist, GC_NAVHEADINGS, rideNavigator->columns());
    appsettings->setCValue(context->athlete->cyclist, GC_NAVHEADINGWIDTHS, rideNavigator->widths());
}

void
AnalysisSidebar::configChanged()
{
}

void
AnalysisSidebar::setFilter(QStringList filter)
{
    rideNavigator->searchStrings(filter);
    calendarWidget->setFilter(filter);
}

void
AnalysisSidebar::clearFilter()
{
    rideNavigator->clearSearch();
    calendarWidget->clearFilter();
}
