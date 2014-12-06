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

#include "BingMap.h"

#include "RideItem.h"
#include "RideFile.h"
#include "IntervalItem.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "TimeUtils.h"

#include <QDebug>

BingMap::BingMap(Context *context) : GcChartWindow(context), context(context), range(-1), current(NULL)
{
    setControls(NULL);
    setContentsMargins(0,0,0,0);
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setChartLayout(layout);

    view = new QWebView();
    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,0,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    webBridge = new BWebBridge(context, this);

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(view->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(updateFrame()));
    connect(context, SIGNAL(intervalsChanged()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalSelected()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalZoom(IntervalItem*)), this, SLOT(zoomInterval(IntervalItem*)));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));

    first = true;

    // get the colors setup for first run
    configChanged();
}

void
BingMap::configChanged()
{
    setProperty("color", GColor(CPLOTBACKGROUND));
    rideSelected();
}

void
BingMap::rideSelected()
{
    // skip display if data drawn or invalid
    if (myRideItem == NULL || !amVisible()) return;
    RideItem * ride = myRideItem;
    if (ride == current || !ride || !ride->ride()) return;
    else current = ride;

    // Route metadata ...
    setSubTitle(ride->ride()->getTag("Route", tr("Route")));

    range=-1;
    rideCP=300;

    if (context->athlete->zones()) {

        // get the right range and CP
        range = context->athlete->zones()->whichRange(ride->dateTime.date());
        if (range >= 0) rideCP = context->athlete->zones()->getCP(range);
    }

    loadRide();
}

void BingMap::loadRide()
{
    createHtml();
    view->page()->mainFrame()->setHtml(currentPage);
}

void BingMap::updateFrame()
{
    // deleting the web bridge seems to be the only way to
    // reset state between it and the webpage.
    delete webBridge;
    webBridge = new BWebBridge(context, this);
    connect(context, SIGNAL(intervalsChanged()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalSelected()), webBridge, SLOT(intervalsChanged()));

    view->page()->mainFrame()->addToJavaScriptWindowObject("webBridge", webBridge);
}

void BingMap::createHtml()
{
    RideItem * ride = myRideItem;
    currentPage = "";
    double minLat, minLon, maxLat, maxLon;
    minLat = minLon = 1000;
    maxLat = maxLon = -1000; // larger than 360

    // get bounding co-ordinates for ride
    foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints()) {
        if (rfp->lat || rfp->lon) {
            minLat = std::min(minLat,rfp->lat);
            maxLat = std::max(maxLat,rfp->lat);
            minLon = std::min(minLon,rfp->lon);
            maxLon = std::max(maxLon,rfp->lon);
        }
    }

    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);

    // No GPS data, so sorry no map
    if(!ride || !ride->ride() || ride->ride()->areDataPresent()->lat == false || ride->ride()->areDataPresent()->lon == false) {
        currentPage = QString("<STYLE>BODY { background-color: %1; color: %2 }</STYLE><center>%3</center>").arg(bgColor.name()).arg(fgColor.name()).arg(tr("No GPS Data Present"));
        setIsBlank(true);
        return;
    } else {
        setIsBlank(false);
    }

    // load the Google Map v3 API
    currentPage = QString("<!DOCTYPE html> \n"
    "<html>\n"
    "<head>\n"
    "<title>Golden Cheetah Map</title>\n"

    // Load BING Rest API Services
    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n"
    "<script type=\"text/javascript\" src=\"http://ecn.dev.virtualearth.net/mapcontrol/mapcontrol.ashx?v=7.0\"></script>\n");

    // colors
    currentPage += QString("<STYLE>BODY { background-color: %1; color: %2 }</STYLE>")
                                          .arg(bgColor.name()).arg(fgColor.name());
    // local functions
    currentPage += QString("<script type=\"text/javascript\">\n"

    "var map;\n"  // the map object
    "var intervalList;\n"  // array of intervals
    "var markerList;\n"  // array of markers
    "var polyList;\n"  // array of polylines

    // Draw the entire route, we use a local webbridge
    // to supply the data to a) reduce bandwidth and
    // b) allow local manipulation. This makes the UI
    // considerably more 'snappy'
    "function drawRoute() {\n"

    // route will be drawn with these options
    "    var routeOptionsYellow = {\n"
    "        strokeColor: new Microsoft.Maps.Color(100, 255, 255, 0),\n"
    "        strokeThickness: 7,\n"
    "        strokeDashArray: '5 0',\n"
    "        zIndex: -2\n"
    "    };\n"

    // load the GPS co-ordinates
    "    var latlons = webBridge.getLatLons(0);\n" // interval "0" is the entire route

    // create the route path
    "    var route = new Array();\n"
    "    var j=0;\n"
    "    while (j < latlons.length) { \n"
    "        route.push(new Microsoft.Maps.Location(latlons[j],latlons[j+1]));\n"
    "        j += 2;\n"
    "    }\n"

    // create the route Polyline
    "    var routeYellow = new Microsoft.Maps.Polyline(route, routeOptionsYellow);\n"
    "    map.entities.push(routeYellow);\n"

    "}\n"

    "function drawIntervals() { \n"
    // intervals will be drawn with these options
    "    var polyOptions = {\n"
    "        strokeColor: new Microsoft.Maps.Color(100, 0, 0, 255),\n"
    "        strokeThickness: 5,\n"
    "        strokeDashArray: '5 0',\n"
    "        zIndex: -1\n"
    "    };\n"

    // remove previous intervals highlighted
    "    j= intervalList.length;\n"
    "    while (j) {\n"
    "       var highlighted = intervalList.pop();\n"
    "       highlighted.setOptions( { visible: false });\n"
    "       j--;\n"
    "    }\n"

    // how many to draw?
    "    var intervals = webBridge.intervalCount();\n"
    "    while (intervals > 0) {\n"
    "        var latlons = webBridge.getLatLons(intervals);\n"

            // create the route path
    "        var route = new Array();\n"
    "        var j=0;\n"
    "        while (j < latlons.length) { \n"
    "            route.push(new Microsoft.Maps.Location(latlons[j],latlons[j+1]));\n"
    "            j += 2;\n"
    "        }\n"

             // create the route Polyline
    "        var intervalHighlighter = new Microsoft.Maps.Polyline(route, polyOptions);\n"
    "        map.entities.push(intervalHighlighter);\n"
    "        intervalList.push(intervalHighlighter);\n"

    "        intervals--;\n"
    "    }\n"
    "}\n"



    // initialise function called when map loaded
    "function initialize() {\n"

    // define initial map boundary
    "    var initialViewBounds = Microsoft.Maps.LocationRect.fromCorners("
                                 "new Microsoft.Maps.Location(%1,%2), "
                                 "new Microsoft.Maps.Location(%3,%4));\n"

    // map options - we like AUTO change as you zoom in
    "    var options = {\n"
    "        credentials:\"AtgAt5wRsWGlmft5J_qUbo4rTJg_qZvepiwN_8FQMQwryIlMMCSpwCbvYiIZO5Zr\",\n" // API key, registered to GoldenCheetah
    "        bounds: initialViewBounds,\n"
    "        mapTypeId: Microsoft.Maps.MapTypeId.auto,\n"
    "        animate: true\n"
    "    };\n"

    // setup the map, and fit to contain the limits of the route
    "    map = new Microsoft.Maps.Map(document.getElementById(\"map_canvas\"),options);\n"

    // initialise local variables
    "    markerList = new Array();\n"
    "    intervalList = new Array();\n"
    "    polyList = new Array();\n"

    // draw the main route data, getting the geo
    // data from the webbridge - reduces data sent/received
    // to the map server and makes the UI pretty snappy
    "    drawRoute();\n"

    "    drawIntervals();\n"

    // catch signals to redraw intervals
    "    webBridge.drawIntervals.connect(drawIntervals);\n"

    // we're done now let the C++ side draw its overlays
    "    webBridge.drawOverlays();\n"

    "}\n"
    "</script>\n").arg(minLat,0,'g',GPS_COORD_TO_STRING).arg(minLon,0,'g',GPS_COORD_TO_STRING).arg(maxLat,0,'g',GPS_COORD_TO_STRING).arg(maxLon,0,'g',GPS_COORD_TO_STRING);

    // the main page is rather trivial
    currentPage += QString("</head>\n"
    "<body onload=\"initialize()\">\n"
    "<div id=\"map_canvas\"></div>\n"
    "</body>\n"
    "</html>\n");
}


QColor BingMap::GetColor(int watts)
{
    if (range < 0) return Qt::red;
    else return zoneColor(context->athlete->zones()->whichZone(range, watts), 7);
}

// create the ride line
void
BingMap::drawShadedRoute()
{
    int intervalTime = 60;  // 60 seconds
    double rtime=0; // running total for accumulated data
    int count=0;  // how many samples ?
    int rwatts=0; // running total of watts
    double prevtime=0; // time for previous point

    QString code;

    foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints()) {

        if (count == 0) {
            code = QString("{\nvar route = new Array();\n");
        } else {
            if (rfp->lat || rfp->lon)
                code += QString("route.push(new Microsoft.Maps.Location(%1,%2));\n").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING);
        }

        // running total of time
        rtime += rfp->secs - prevtime;
        rwatts += rfp->watts;
        prevtime = rfp->secs;
        count++;

        // end of segment
        if (rtime >= intervalTime) {

            int avgWatts = rwatts / count;
            QColor color = GetColor(avgWatts);
            // thats this segment done, so finish off and
            // add tooltip junk
            count = rwatts = rtime = 0;

            // color the polyline
            code += QString("    var polyOptions = {\n"
                            "        strokeColor: new Microsoft.Maps.Color(200, %1, %2, %3),\n"
                            "        strokeThickness: 3,\n"
                            "        strokeDashArray: '5 0',\n"
                            "        zIndex: 1\n"
                            "    };\n"
                            "    var polyline = new Microsoft.Maps.Polyline(route, polyOptions);\n"
                            "    polyline.setOptions(polyOptions);\n"
                            "    map.entities.push(polyline);\n"
                            "}\n").arg(color.red())
                                  .arg(color.green())
                                  .arg(color.blue());
            view->page()->mainFrame()->evaluateJavaScript(code);
        }
    }
}

//
// Static helper - havervine formula for calculating the distance
//                 between 2 geo co-ordinates
//
static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
static const double EARTH_RADIUS_IN_METERS = 6372797.560856;
static double ArcInRadians(double fromLat, double fromLon, double toLat, double toLon) {

    double latitudeArc  = (fromLat - toLat) * DEG_TO_RAD;
    double longitudeArc = (fromLon - toLon) * DEG_TO_RAD;
    double latitudeH = sin(latitudeArc * 0.5);
    latitudeH *= latitudeH;
    double lontitudeH = sin(longitudeArc * 0.5);
    lontitudeH *= lontitudeH;
    double tmp = cos(fromLat*DEG_TO_RAD) * cos(toLat*DEG_TO_RAD);
    return 2.0 * asin(sqrt(latitudeH + tmp*lontitudeH));
}

static double distanceBetween(double fromLat, double fromLon, double toLat, double toLon) {
    return EARTH_RADIUS_IN_METERS*ArcInRadians(fromLat, fromLon, toLat, toLon);
}

void
BingMap::createMarkers()
{
    QString code;

    //
    // START / FINISH MARKER
    //
    const QVector<RideFilePoint *> &points = myRideItem->ride()->dataPoints();

    bool loop = distanceBetween(points[0]->lat,
                                points[0]->lon,
                                points[points.count()-1]->lat,
                                points[points.count()-1]->lon) < 100 ? true : false;

    if (loop) {

        code = QString("{ var latlng = new Microsoft.Maps.Location(%1,%2); " 
                   "var pushpinOptions = { icon: 'qrc:images/maps/loop.png', height: 37, width: 32 };"
                   "var pushpin = new Microsoft.Maps.Pushpin(latlng, pushpinOptions);"
                   "map.entities.push(pushpin); }"
                   ).arg(points[0]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[0]->lon,0,'g',GPS_COORD_TO_STRING);

        view->page()->mainFrame()->evaluateJavaScript(code);

    } else {
        // start / finish markers
        code = QString("{ var latlng = new Microsoft.Maps.Location(%1,%2); " 
                   "var pushpinOptions = { icon: 'qrc:images/maps/cycling.png', height: 37, width: 32 };"
                   "var pushpin = new Microsoft.Maps.Pushpin(latlng, pushpinOptions);"
                   "map.entities.push(pushpin); }"
                   ).arg(points[0]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[0]->lon,0,'g',GPS_COORD_TO_STRING);
        view->page()->mainFrame()->evaluateJavaScript(code);

        code = QString("{ var latlng = new Microsoft.Maps.Location(%1,%2); " 
                   "var pushpinOptions = { icon: 'qrc:images/maps/finish.png', height: 37, width: 32 };"
                   "var pushpin = new Microsoft.Maps.Pushpin(latlng, pushpinOptions);"
                   "map.entities.push(pushpin); }"
                   ).arg(points[points.count()-1]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[points.count()-1]->lon,0,'g',GPS_COORD_TO_STRING);
        view->page()->mainFrame()->evaluateJavaScript(code);
    }

    //
    // STOPS - BEER AND BURRITO TIME (> 5 mins in same spot)
    //
    double stoplat=0, stoplon=0;
    double laststoptime=0;
    double lastlat=0, lastlon=0;
    int stoptime=0;
    static const int BEERANDBURRITO = 300; // anything longer than 5 minutes

    foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints())
    {
        if (!rfp->lat || !rfp->lon) continue; // ignore blank values

        if (!stoplat || !stoplon) { // register first gps co-ord
            stoplat = rfp->lat;
            stoplon = rfp->lon;
            laststoptime = rfp->secs;
        }

        if (distanceBetween(rfp->lat, rfp->lon, stoplat, stoplon) < 20) {
            if (rfp->secs - laststoptime > myRideItem->ride()->recIntSecs()) stoptime += rfp->secs - laststoptime;
            else stoptime += myRideItem->ride()->recIntSecs();
        } else if (rfp->secs - laststoptime > myRideItem->ride()->recIntSecs()) {
            stoptime += rfp->secs - laststoptime;
            stoplat = rfp->lat;
            stoplon = rfp->lon;
        } else {
            stoptime = 0;
            stoplat = rfp->lat;
            stoplon = rfp->lon;
        }

        if (stoptime > BEERANDBURRITO) { // 3 minutes is more than a traffic light stop dude.

            if ((!lastlat && !lastlon) || distanceBetween(lastlat, lastlon, stoplat, stoplon)>100) {
                lastlat = stoplat;
                lastlon = stoplon;
                code = QString("{ var latlng = new Microsoft.Maps.Location(%1,%2); " 
                   "var pushpinOptions = { icon: 'qrc:images/maps/cycling_feed.png', height: 37, width: 32 };"
                   "var pushpin = new Microsoft.Maps.Pushpin(latlng, pushpinOptions);"
                   "map.entities.push(pushpin); }"
                    ).arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING);
                view->page()->mainFrame()->evaluateJavaScript(code);
                stoptime=0;
            }
            stoplat=stoplon=stoptime=0;
        }
        laststoptime = rfp->secs;
    }

    //
    // INTERVAL MARKERS
    //
    if (context->athlete->allIntervalItems() == NULL) return; // none to do, we are all done then

    int interval=0;
    foreach (const RideFileInterval x, myRideItem->ride()->intervals()) {

        int offset = myRideItem->ride()->intervalBegin(x);
        code = QString("{ var latlng = new Microsoft.Maps.Location(%1,%2);\n" 
                   "var pushpinOptions = { };\n"
                   "var pushpin = new Microsoft.Maps.Pushpin(latlng, pushpinOptions);\n"
                   "map.entities.push(pushpin);\n"
                   "pushpin.cm1001_er_etr.dom.setAttribute('title', '%3');\n"
                   "pushpinClick= Microsoft.Maps.Events.addHandler(pushpin, 'click', function(event) { webBridge.toggleInterval(%4); });\n"
                   " }")
                   .arg(myRideItem->ride()->dataPoints()[offset]->lat,0,'g',GPS_COORD_TO_STRING)
                   .arg(myRideItem->ride()->dataPoints()[offset]->lon,0,'g',GPS_COORD_TO_STRING)
                   .arg(x.name)
                   .arg(interval);

        view->page()->mainFrame()->evaluateJavaScript(code);
        interval++;
    }

    return;
}

void BingMap::zoomInterval(IntervalItem *which)
{
    RideItem *ride = myRideItem;

    // null ride
    if (!ride || !ride->ride()) return;

    int start = ride->ride()->timeIndex(which->start);
    int end = ride->ride()->timeIndex(which->stop);

    // out of bounds
    if (start < 0 || start > ride->ride()->dataPoints().count()-1 ||
        end < 0 || end > ride->ride()->dataPoints().count()-1) return;

    // Get the bounding rectangle for this interval
    double minLat, minLon, maxLat, maxLon;
    minLat = minLon = 1000;
    maxLat = maxLon = -1000; // larger than 360

    // get bounding co-ordinates for ride
    for(int i=start; i<= end; i++) {
        RideFilePoint *rfp = ride->ride()->dataPoints().at(i);
        if (rfp->lat || rfp->lon) {
            minLat = std::min(minLat,rfp->lat);
            maxLat = std::max(maxLat,rfp->lat);
            minLon = std::min(minLon,rfp->lon);
            maxLon = std::max(maxLon,rfp->lon);
        }
    }

    // now zoom to interval
    QString code = QString("{\n"
                           "    var viewBounds = Microsoft.Maps.LocationRect.fromCorners("
                           "                                  new Microsoft.Maps.Location(%1,%2), "
                           "                                  new Microsoft.Maps.Location(%3,%4));\n"
                           "   map.setView( { bounds: viewBounds });\n }")
                    .arg(minLat,0,'g',GPS_COORD_TO_STRING)
                    .arg(minLon,0,'g',GPS_COORD_TO_STRING)
                    .arg(maxLat,0,'g',GPS_COORD_TO_STRING)
                    .arg(maxLon,0,'g',GPS_COORD_TO_STRING);
    view->page()->mainFrame()->evaluateJavaScript(code);
}

// quick diag, used to debug code only
void BWebBridge::call(int count) { qDebug()<<"webBridge call:"<<count; }

// how many intervals are highlighted?
int
BWebBridge::intervalCount()
{
    int highlighted;
    highlighted = 0;
    RideItem *rideItem = gm->property("ride").value<RideItem*>();

    if (context->athlete->allIntervalItems() == NULL ||
        rideItem == NULL || rideItem->ride() == NULL) return 0; // not inited yet!

    for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {
        IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(i));
        if (current != NULL) {
            if (current->isSelected() == true) {
                ++highlighted;
            }
        }
    }
    return highlighted;
}

// get a latlon array for the i'th selected interval
QVariantList
BWebBridge::getLatLons(int i)
{
    QVariantList latlons;
    int highlighted=0;
    RideItem *rideItem = gm->property("ride").value<RideItem*>();

    if (context->athlete->allIntervalItems() == NULL ||
       rideItem ==NULL || rideItem->ride() == NULL) return latlons; // not inited yet!

    if (i) {

        // get for specific interval
        for (int j=0; j<context->athlete->allIntervalItems()->childCount(); j++) {
            IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(j));
            if (current != NULL) {
                if (current->isSelected() == true) {
                    ++highlighted;

                    // this one!
                    if (highlighted==i) {

                        // so this one is the interval we need.. lets
                        // snaffle up the points in this section
                        foreach (RideFilePoint *p1, rideItem->ride()->dataPoints()) {
                            if (p1->secs+rideItem->ride()->recIntSecs() > current->start
                                && p1->secs< current->stop) {

                                if (p1->lat || p1->lon) {
                                    latlons << p1->lat;
                                    latlons << p1->lon;
                                }
                            }
                        }
                        return latlons;
                    }
                }
            }
        }
    } else {

        // get latlons for entire route
        foreach (RideFilePoint *p1, rideItem->ride()->dataPoints()) {
            if (p1->lat || p1->lon) {
                latlons << p1->lat;
                latlons << p1->lon;
            }
        }
    }
    return latlons;
}

// once the basic map and route have been marked, overlay markers, shaded areas etc
void
BWebBridge::drawOverlays()
{
    // overlay the markers
    gm->createMarkers();

    // overlay a shaded route
    gm->drawShadedRoute();
}

// interval marker was clicked on the map, toggle its display
void
BWebBridge::toggleInterval(int x)
{
    IntervalItem *current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(x));
    if (current) current->setSelected(!current->isSelected());
}
