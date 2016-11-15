/*
 * Copyright (c) 2011 Greg Lonnon (greg.lonnon@gmail.com)
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

#include "RideWindow.h"
#include "RideItem.h"
#include "RealtimeData.h"
#include "JsonRideFile.h"

// this is called on every page load in the web browser
void RideWindow::addJSObjects()
{
    view->page()->mainFrame()->addToJavaScriptWindowObject("GCRider",rider);
}

void RideWindow::rideSelected()
{
    // skip display if data drawn or invalid
    if (myRideItem == NULL || !amVisible()) return;
    RideItem * r = myRideItem;
    if (ride == r || !r || !r->ride()) return;
    else ride = r;

    loadRide();
}

void RideWindow::loadRide()
{
    QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    // uncommit these two lines to get the QWebInspector working.
    //    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, true);
    //    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

    QWebPage *page = view->page();
    QWebFrame *frame = page->mainFrame();

    // Signal is emitted before frame loads any web content:
    QObject::connect(frame, SIGNAL(javaScriptWindowObjectCleared()),
                            this, SLOT(addJSObjects()));
    view->setPage(page);
    // used for testing...
    RiderBridge* tr = new RealtimeRider(context);
    tr->setRide(ride);
    rider = tr;
    addJSObjects();
}

RideWindow::RideWindow(Context *context) :
    GcChartWindow(context),
    rideLoaded(false),
    context(context)
{
    view = new QWebView();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(view);
    setChartLayout(layout);

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
}
