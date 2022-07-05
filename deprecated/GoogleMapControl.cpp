/*
 * Copyright (c) 2009 Greg Lonnon (greg.lonnon@gmail.com)
 *               2011 Mark Liversedge (liversedge@gmail.com)
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

#include "GoogleMapControl.h"

#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"

#ifdef NOWEBKIT
#include <QtWebChannel>
#endif

// overlay helper
#include "TabView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include <QDebug>

GoogleMapControl::GoogleMapControl(Context *context) : GcChartWindow(context), context(context), 
                                                       range(-1), current(NULL), firstShow(true), stale(false)
{
    setControls(NULL);

    setContentsMargins(0,0,0,0);
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setChartLayout(layout);

#ifdef NOWEBKIT
    view = new QWebEngineView(this);
#else
    view = new QWebView();
#endif
    view->setPage(new myWebPage());
    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,0,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    HelpWhatsThis *help = new HelpWhatsThis(view);
    view->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Map));

    webBridge = new WebBridge(context, this);
#ifdef NOWEBKIT
    // file: MyWebEngineView.cpp, MyWebEngineView extends QWebEngineView
    QWebChannel *channel = new QWebChannel(view->page());

    // set the web channel to be used by the page
    // see http://doc.qt.io/qt-5/qwebenginepage.html#setWebChannel
    view->page()->setWebChannel(channel);

    // register QObjects to be exposed to JavaScript
    channel->registerObject(QStringLiteral("webBridge"), webBridge);
#endif

    // put a helper on the screen for mouse over intervals...
    overlayIntervals = new IntervalSummaryWindow(context);
    addHelper(tr("Intervals"), overlayIntervals);

    //
    // connects
    //
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
#ifndef NOWEBKIT
    connect(view->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(updateFrame()));
#endif

    connect(context, SIGNAL(rideChanged(RideItem*)), this, SLOT(forceReplot()));
    connect(context, SIGNAL(intervalsChanged()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalSelected()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalZoom(IntervalItem*)), this, SLOT(zoomInterval(IntervalItem*)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    first = true;
    configChanged(CONFIG_APPEARANCE);
}

GoogleMapControl::~GoogleMapControl()
{
    delete webBridge;
}

void 
GoogleMapControl::configChanged(qint32)
{
    setProperty("color", GColor(CPLOTBACKGROUND));
#ifndef Q_OS_MAC
    overlayIntervals->setStyleSheet(TabView::ourStyleSheet());
#endif
}

void
GoogleMapControl::forceReplot()
{
    stale=true;
    rideSelected();
}

void
GoogleMapControl::rideSelected()
{
    RideItem * ride = myRideItem;

    // set/unset blank then decide what to do next
    if (!ride || !ride->ride() || !ride->ride()->dataPoints().count()) setIsBlank(true);
    else setIsBlank(false);

    // skip display if data already drawn or invalid
    if (myRideItem == NULL || !amVisible()) return;

    // nothing to plot
    if (!ride || !ride->ride()) return;
    else if (!stale && ride == current) return;
    
    // remember what we last plotted
    current = ride;

    // Route metadata ...
    setSubTitle(ride->ride()->getTag("Route", tr("Route")));

    // default to ..
    range = -1;
    rideCP = 300;
    stale = false;

    if (context->athlete->zones(ride->isRun)) {
        range = context->athlete->zones(ride->isRun)->whichRange(ride->dateTime.date());
        if (range >= 0) rideCP = context->athlete->zones(ride->isRun)->getCP(range);
    }

    loadRide();
}

void GoogleMapControl::loadRide()
{
    createHtml();

#ifdef NOWEBKIT
    view->page()->setHtml(currentPage);
#else
    view->page()->mainFrame()->setHtml(currentPage);
#endif
}

void GoogleMapControl::updateFrame()
{
    // deleting the web bridge seems to be the only way to
    // reset state between it and the webpage.
    delete webBridge;
    webBridge = new WebBridge(context, this);
    connect(context, SIGNAL(intervalsChanged()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalSelected()), webBridge, SLOT(intervalsChanged()));

#ifndef NOWEBKIT
    view->page()->mainFrame()->addToJavaScriptWindowObject("webBridge", webBridge);
#endif

}

void GoogleMapControl::createHtml()
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

    // No GPS data, so sorry no map
    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor fgColor = GInvertColor(bgColor);
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
    "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=yes\"/> \n"
    "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n"
    "<title>Golden Cheetah Map</title>\n"
    "<!--<link href=\"http://code.google.com/apis/maps/documentation/javascript/examples/default.css\" rel=\"stylesheet\" type=\"text/css\" />--> \n"
    "<style type=\"text/css\">\n"
    "   html { height: 100% }\n"
    "   body { height: 100%; margin: 0; padding: 0 }\n"
    "   #map-canvas { height: 100% }\n"
    "</style>\n"
    "<script type=\"text/javascript\" src=\"http://maps.googleapis.com/maps/api/js?key=AIzaSyASrk4JoJOzESQguDwjk8aq9nQXsrUUskM\"></script> \n");

#ifdef NOWEBKIT
    currentPage += QString("<script type=\"text/javascript\" src=\"qrc:///qtwebchannel/qwebchannel.js\"></script>\n");
#endif

    currentPage += QString("<script type=\"text/javascript\"> \n"
    "var webBridge; \n"
    "document.addEventListener(\"DOMContentLoaded\", function () { \n"
#ifdef NOWEBKIT
    "<!-- it's a good idea to initialize webchannel after DOM ready, if the code is going to manipulate the DOM -->\n"
    "   new QWebChannel(qt.webChannelTransport, function (channel) { \n"
    "       webBridge = channel.objects.webBridge; \n"
    "       initialize(); \n"
    "   }); \n"
#else
    "   initialize(); \n"
#endif
    "}); \n"
    "</script>");

    // fg/bg
    currentPage += QString("<STYLE>BODY { background-color: %1; color: %2 }</STYLE>")
                                          .arg(bgColor.name()).arg(fgColor.name());

    // local functions
    currentPage += QString("<script type=\"text/javascript\">\n"
    "var map;\n"  // the map object
    "var intervalList;\n"  // array of intervals
    "var markerList;\n"  // array of markers
    "var polyList;\n"  // array of polylines
    "var tmpIntervalHighlighter;\n"  // temp interval

    // Draw the entire route, we use a local webbridge
    // to supply the data to a) reduce bandwidth and
    // b) allow local manipulation. This makes the UI
    // considerably more 'snappy'
    "function drawRoute() {\n"
#ifdef NOWEBKIT
    // load the GPS co-ordinates
    "   webBridge.getLatLons(0, drawRouteForLatLons);\n"
#else
    // load the GPS co-ordinates
    "    var latlons = webBridge.getLatLons(0);\n" // interval "0" is the entire route
    "   drawRouteForLatLons(latlons);\n"
#endif
    "}\n"
    "\n"
    "function drawRouteForLatLons(latlons) {\n"

    // route will be drawn with these options
    "    var routeOptionsYellow = {\n"
    "        strokeColor: '#FFFF00',\n"
    "        strokeOpacity: 0.4,\n"
    "        strokeWeight: 10,\n"
    "        zIndex: -2\n"
    "    };\n"

    // create the route Polyline
    "    var routeYellow = new google.maps.Polyline(routeOptionsYellow);\n"
    "    routeYellow.setMap(map);\n"

    // lastly, populate the route path
    "    var path = routeYellow.getPath();\n"
    "    var j=0;\n"
    "    while (j < latlons.length) { \n"
    "        path.push(new google.maps.LatLng(latlons[j], latlons[j+1]));\n"
    "        j += 2;\n"
    "    }\n"

    // Listen mouse events
    "    google.maps.event.addListener(routeYellow, 'mousedown', function(event) { map.setOptions({draggable: false, zoomControl: false, scrollwheel: false, disableDoubleClickZoom: true}); webBridge.clickPath(event.latLng.lat(), event.latLng.lng()); });\n"
    "    google.maps.event.addListener(routeYellow, 'mouseup',   function(event) { map.setOptions({draggable: true, zoomControl: true, scrollwheel: true, disableDoubleClickZoom: false}); webBridge.mouseup(); });\n"
    "    google.maps.event.addListener(routeYellow, 'mouseover', function(event) { webBridge.hoverPath(event.latLng.lat(), event.latLng.lng()); });\n"
    "}\n"


    "function drawIntervals() { \n"
    // how many to draw?
#ifdef NOWEBKIT
    "   webBridge.intervalCount(drawIntervalsCount);\n"
#else
    "   drawIntervalsCount(webBridge.intervalCount());\n"
#endif
    "}\n"

    "function drawIntervalsCount(intervals) { \n"
    // remove previous intervals highlighted
    "   j= intervalList.length;\n"
    "    while (j) {\n"
    "       var highlighted = intervalList.pop();\n"
    "       highlighted.setMap(null);\n"
    "       j--;\n"
    "    }\n"

    "   while (intervals > 0) {\n"
#ifdef NOWEBKIT
    "       webBridge.getLatLons(intervals, drawInterval);\n"
#else
    "       drawInterval(webBridge.getLatLons(intervals));\n"
#endif
    "       intervals--;\n"
    "   }\n"
    "}\n"

    "function drawInterval(latlons) { \n"
    // intervals will be drawn with these options
    "   var polyOptions = {\n"
    "       strokeColor: '#0000FF',\n"
    "       strokeOpacity: 0.6,\n"
    "       strokeWeight: 10,\n"
    "       zIndex: -1\n"  // put at the bottom
    "   }\n"
    "   var intervalHighlighter = new google.maps.Polyline(polyOptions);\n"
    "   intervalHighlighter.setMap(map);\n"
    "   intervalList.push(intervalHighlighter);\n"
    "   var path = intervalHighlighter.getPath();\n"
    "   var j=0;\n"
    "   while (j<latlons.length) {\n"
    "       path.push(new google.maps.LatLng(latlons[j], latlons[j+1]));\n"
    "       j += 2;\n"
    "   }\n"
    "}\n"

    // initialise function called when map loaded
    "function initialize() {\n"

    // TERRAIN style map please and make it draggable
    // note that because QT webkit offers touch/gesture
    // support the Google API only supports dragging
    // via gestures - this is alrady registered as a bug
    // with the google map team
    "    var controlOptions = {\n"
    "      style: google.maps.MapTypeControlStyle.DEFAULT\n"
    "    };\n"
    "    var myOptions = {\n"
    "      draggable: true,\n"
    "      mapTypeId: google.maps.MapTypeId.TERRAIN,\n"
    "      tilt: 45,\n"
    "      streetViewControl: false,\n"
    "    };\n"

    // setup the map, and fit to contain the limits of the route
    "    map = new google.maps.Map(document.getElementById('map-canvas'), myOptions);\n"
    "    var sw = new google.maps.LatLng(%1,%2);\n"  // .ARG 1, 2
    "    var ne = new google.maps.LatLng(%3,%4);\n"  // .ARG 3, 4
    "    var bounds = new google.maps.LatLngBounds(sw,ne);\n"
    "    map.fitBounds(bounds);\n"

    // add the bike layer, useful in some areas, but coverage
    // is limited, US gets best coverage at this point (Summer 2011)
    "    var bikeLayer = new google.maps.BicyclingLayer();\n"
    "    bikeLayer.setMap(map);\n"

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

    // Liste mouse events
    "    google.maps.event.addListener(map, 'mouseup', function(event) { map.setOptions({draggable: true, zoomControl: true, scrollwheel: true, disableDoubleClickZoom: false}); webBridge.mouseup(); });\n"


    "}\n"
    "</script>\n").arg(minLat,0,'g',GPS_COORD_TO_STRING).
            arg(minLon,0,'g',GPS_COORD_TO_STRING).
            arg(maxLat,0,'g',GPS_COORD_TO_STRING).
            arg(maxLon,0,'g',GPS_COORD_TO_STRING);

    // the main page is rather trivial
    currentPage += QString("</head>\n"

    "<body>\n"
    "<div id=\"map-canvas\"></div>\n"
    "</body>\n"
    "</html>\n");
}


QColor GoogleMapControl::GetColor(int watts)
{
    if (range < 0) return Qt::red;
    else return zoneColor(context->athlete->zones(myRideItem ? myRideItem->isRun : false)->whichZone(range, watts), 7);
}

// create the ride line
void
GoogleMapControl::drawShadedRoute()
{
    int intervalTime = 60;  // 60 seconds
    double rtime=0; // running total for accumulated data
    int count=0;  // how many samples ?
    int rwatts=0; // running total of watts
    double prevtime=0; // time for previous point

    QString code;

    foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints()) {

        if (count == 0) {
            code = QString("{\nvar polyline = new google.maps.Polyline();\n"
                   "   polyline.setMap(map);\n"
                   "   path = polyline.getPath();\n");

            // Listen mouse events
            code += QString("google.maps.event.addListener(polyline, 'mousedown', function(event) { map.setOptions({draggable: false, zoomControl: false, scrollwheel: false, disableDoubleClickZoom: true}); webBridge.clickPath(event.latLng.lat(), event.latLng.lng()); });\n"
                            "google.maps.event.addListener(polyline, 'mouseup',   function(event) { map.setOptions({draggable: true, zoomControl: true, scrollwheel: true, disableDoubleClickZoom: false}); webBridge.mouseup(); });\n"
                            "google.maps.event.addListener(polyline, 'mouseover', function(event) { webBridge.hoverPath(event.latLng.lat(), event.latLng.lng()); });\n");
        } else {
            if (rfp->lat || rfp->lon)
                code += QString("path.push(new google.maps.LatLng(%1,%2));\n").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING);
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
            code += QString("var polyOptions = {\n"
                            "    strokeColor: '%1',\n"
                            "    strokeWeight: 3,\n"
                            "    strokeOpacity: 0.5,\n" // for out and backs, we need both
                            "    zIndex: 0,\n"
                            "}\n"
                            "polyline.setOptions(polyOptions);\n"
                            "}\n").arg(color.name());

        #ifdef NOWEBKIT
            view->page()->runJavaScript(code);
        #else
            view->page()->mainFrame()->evaluateJavaScript(code);
        #endif

        }
    }

}

void
GoogleMapControl::clearTempInterval() {
    QString code = QString( "{ \n"
                            "    tmpIntervalHighlighter.getPath().clear();\n"
                            "}\n" );

#ifdef NOWEBKIT
    view->page()->runJavaScript(code);
#else
    view->page()->mainFrame()->evaluateJavaScript(code);
#endif
}

void
GoogleMapControl::drawTempInterval(IntervalItem *current) {

    QString code = QString( "{ \n"
                    // interval will be drawn with these options
                    "    var polyOptions = {\n"
                    "        strokeColor: '#00FFFF',\n"
                    "        strokeOpacity: 0.6,\n"
                    "        strokeWeight: 10,\n"
                    "        zIndex: -1\n"  // put at the bottom
                    "    }\n"

                    "    if (!tmpIntervalHighlighter) {\n"
                    "       tmpIntervalHighlighter = new google.maps.Polyline(polyOptions);\n"
                    "       tmpIntervalHighlighter.setMap(map);\n"
                    "       google.maps.event.addListener(tmpIntervalHighlighter, 'mouseup',   function(event) { map.setOptions({draggable: true, zoomControl: true, scrollwheel: true, disableDoubleClickZoom: false}); webBridge.mouseup(); });\n"
                    "    } \n"

                    "    var path = tmpIntervalHighlighter.getPath();\n"
                    "    path.clear();\n");

    foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints()) {
        if (rfp->secs+myRideItem->ride()->recIntSecs() > current->start
            && rfp->secs< current->stop) {

            if (rfp->lat || rfp->lon) {
                code += QString("    path.push(new google.maps.LatLng(%1,%2));\n").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING);
            }
        }
    }



    code += QString("}\n" );

#ifdef NOWEBKIT
    view->page()->runJavaScript(code);
#else
    view->page()->mainFrame()->evaluateJavaScript(code);
#endif

    overlayIntervals->intervalSelected();
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
GoogleMapControl::createMarkers()
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

        code = QString("{ var latlng = new google.maps.LatLng(%1,%2);"
                   "var image = new google.maps.MarkerImage('qrc:images/maps/loop.png');"
                   "var marker = new google.maps.Marker({ icon: image, animation: google.maps.Animation.DROP, position: latlng });"
                   "marker.setMap(map); }").arg(points[0]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[0]->lon,0,'g',GPS_COORD_TO_STRING);

    #ifdef NOWEBKIT
        view->page()->runJavaScript(code);
    #else
        view->page()->mainFrame()->evaluateJavaScript(code);
    #endif
    } else {
        // start / finish markers
        code = QString("{ var latlng = new google.maps.LatLng(%1,%2);"
                   "var image = new google.maps.MarkerImage('qrc:images/maps/cycling.png');"
                   "var marker = new google.maps.Marker({ icon: image, animation: google.maps.Animation.DROP, position: latlng });"
                   "marker.setMap(map); }").arg(points[0]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[0]->lon,0,'g',GPS_COORD_TO_STRING);

    #ifdef NOWEBKIT
        view->page()->runJavaScript(code);
    #else
        view->page()->mainFrame()->evaluateJavaScript(code);
    #endif


        code = QString("{ var latlng = new google.maps.LatLng(%1,%2);"
                   "var image = new google.maps.MarkerImage('qrc:images/maps/finish.png');"
                   "var marker = new google.maps.Marker({ icon: image, animation: google.maps.Animation.DROP, position: latlng });"
                   "marker.setMap(map); }").arg(points[points.count()-1]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[points.count()-1]->lon,0,'g',GPS_COORD_TO_STRING);


    #ifdef NOWEBKIT
        view->page()->runJavaScript(code);
    #else
        view->page()->mainFrame()->evaluateJavaScript(code);
    #endif
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
                code = QString(
                    "{ var latlng = new google.maps.LatLng(%1,%2);"
                    "var image = new google.maps.MarkerImage('qrc:images/maps/cycling_feed.png');"
                    "var marker = new google.maps.Marker({ icon: image, animation: google.maps.Animation.DROP, position: latlng });"
                    "marker.setMap(map);"
                "}").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING);

            #ifdef NOWEBKIT
                view->page()->runJavaScript(code);
            #else
                view->page()->mainFrame()->evaluateJavaScript(code);
            #endif

                stoptime=0;
            }
            stoplat=stoplon=stoptime=0;
        }
        laststoptime = rfp->secs;
    }


    //
    // INTERVAL MARKERS
    //
    int interval=0;
    foreach (IntervalItem *x, myRideItem->intervals()) {

        int offset = myRideItem->ride()->intervalBeginSecs(x->start);
        code = QString(
            "{"
            "   var latlng = new google.maps.LatLng(%1,%2);"
            "   var marker = new google.maps.Marker({ title: '%3', animation: google.maps.Animation.DROP, position: latlng });"
            "   marker.setMap(map);"
            "   markerList.push(marker);" // keep track of those suckers
            "   google.maps.event.addListener(marker, 'click', function(event) { webBridge.toggleInterval(%4); });"
            "   google.maps.event.addListener(marker, 'mouseover', function(event) { webBridge.hoverInterval(%4); });"
            "   google.maps.event.addListener(marker, 'mouseout', function(event) { webBridge.clearHover(); });"
            "}")
                                    .arg(myRideItem->ride()->dataPoints()[offset]->lat,0,'g',GPS_COORD_TO_STRING)
                                    .arg(myRideItem->ride()->dataPoints()[offset]->lon,0,'g',GPS_COORD_TO_STRING)
                                    .arg(x->name)
                                    .arg(interval)
                                    ;

    #ifdef NOWEBKIT
        view->page()->runJavaScript(code);
    #else
        view->page()->mainFrame()->evaluateJavaScript(code);
    #endif

        interval++;
    }

    return;
}

void GoogleMapControl::zoomInterval(IntervalItem *which)
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
    QString code = QString("{ var southwest = new google.maps.LatLng(%1, %2);\n"
                           "var northeast = new google.maps.LatLng(%3, %4);\n"
                           "var bounds = new google.maps.LatLngBounds(southwest, northeast);\n"
                           "map.fitBounds(bounds);\n }")
                    .arg(minLat,0,'g',GPS_COORD_TO_STRING)
                    .arg(minLon,0,'g',GPS_COORD_TO_STRING)
                    .arg(maxLat,0,'g',GPS_COORD_TO_STRING)
                    .arg(maxLon,0,'g',GPS_COORD_TO_STRING);
#ifdef NOWEBKIT
    view->page()->runJavaScript(code);
#else
    view->page()->mainFrame()->evaluateJavaScript(code);
#endif
}

// quick diag, used to debug code only
void WebBridge::call(int count) { qDebug()<<"webBridge call:"<<count; }

// how many intervals are highlighted?
int
WebBridge::intervalCount()
{
    RideItem *rideItem = gm->property("ride").value<RideItem*>();
    if (rideItem) return rideItem->intervalsSelected().count();
    return 0;
}

// get a latlon array for the i'th selected interval
QVariantList
WebBridge::getLatLons(int i)
{
    QVariantList latlons;
    RideItem *rideItem = gm->property("ride").value<RideItem*>();

    if (rideItem && i > 0 && rideItem->intervalsSelected().count() >= i) {

        IntervalItem *current = rideItem->intervalsSelected().at(i-1);

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
WebBridge::drawOverlays()
{
    // overlay the markers
    gm->createMarkers();

    // overlay a shaded route
    gm->drawShadedRoute();
}

// interval marker was clicked on the map, toggle its display
void
WebBridge::toggleInterval(int x)
{
    RideItem *rideItem = gm->property("ride").value<RideItem*>();
    if (x < 0 || rideItem->intervals().count() <= x) return;

    IntervalItem *current = rideItem->intervals().at(x);
    if (current) {
        current->selected = !current->selected;
        context->notifyIntervalItemSelectionChanged(current);
    }
}

void 
WebBridge::hoverInterval(int n)
{
    RideItem *rideItem = gm->property("ride").value<RideItem*>();
    if (rideItem && rideItem->ride() && rideItem->intervals().count() > n) {
        context->notifyIntervalHover(rideItem->intervals().at(n));
    }
}

void 
WebBridge::clearHover()
{
}

QList<RideFilePoint*>
WebBridge::searchPoint(double lat, double lng)
{
    QList<RideFilePoint*> list;

    RideItem *rideItem = gm->property("ride").value<RideItem*>();

    RideFilePoint *candidat = NULL;
    foreach (RideFilePoint *p1, rideItem->ride()->dataPoints()) {
        if (p1->lat == 0 && p1->lon == 0)
            continue;

        if (((p1->lat-lat> 0 && p1->lat-lat< 0.0001) || (p1->lat-lat< 0 && p1->lat-lat> -0.0001)) &&
            ((p1->lon-lng> 0 && p1->lon-lng< 0.0001) || (p1->lon-lng< 0 && p1->lon-lng> -0.0001))) {
            // Vérifie distance avec dernier candidat
            candidat = p1;
        } else if (candidat)  {
            list.append(candidat);
            candidat = NULL;
        }
    }

    return list;
}

void
WebBridge::hoverPath(double lat, double lng)
{
    if (point) {

        RideItem *rideItem = gm->property("ride").value<RideItem*>();
        QString name = QString(tr("Selection #%1 ")).arg(selection);

        if (rideItem->intervals(RideFileInterval::USER).count()) {

            IntervalItem *last = rideItem->intervals(RideFileInterval::USER).last();

            if (last->name.startsWith(name) && last->rideInterval) { 

                QList<RideFilePoint*> list = searchPoint(lat, lng);

                if (list.count() > 0)  {

                    RideFilePoint* secondPoint = list.at(0);

                    if (secondPoint->secs>point->secs) {
                        last->rideInterval->start = last->start = point->secs;
                        last->rideInterval->stop = last->stop = secondPoint->secs;
                    } else {
                        last->rideInterval->stop = last->stop = point->secs;
                        last->rideInterval->start = last->start = secondPoint->secs;
                    }
                    last->startKM = last->rideItem()->ride()->timeToDistance(last->start);
                    last->stopKM = last->rideItem()->ride()->timeToDistance(last->stop);

                    // update metrics
                    last->refresh();

                    // mark dirty
                    last->rideItem()->setDirty(true);

                    // overlay a shaded route
                    gm->drawTempInterval(last);

                    // update charts etc
                    context->notifyIntervalsChanged();
                 }
            }
        }

        // add average power to the end of the selection name
        //name += QString("(%1 watts)").arg(round((wattsTotal && arrayLength) ? wattsTotal/arrayLength : 0));


        // now update the RideFileIntervals and all the plots etc
        //context->athlete->updateRideFileIntervals();
    }
}

void
WebBridge::clickPath(double lat, double lng)
{
    selection++;
    RideItem *rideItem = gm->property("ride").value<RideItem*>();
    QString name = QString(tr("Selection #%1 ")).arg(selection);
    QList<RideFilePoint*> list = searchPoint(lat, lng);

    if (list.count() > 0)  {

        point = list.at(0);

        IntervalItem *add = rideItem->newInterval(name, point->secs, point->secs, 0, 0);
        add->selected = true;

        // rebuild list in sidebar
        context->notifyIntervalsUpdate(rideItem);

    } else {
        point = NULL;
    }
}

void
WebBridge::mouseup()
{
    // clear the temorary highlighter
    if (point) {
        gm->clearTempInterval();
        point = NULL;
    }  
}


bool
GoogleMapControl::event(QEvent *event)
{
    // nasty nasty nasty hack to move widgets as soon as the widget geometry
    // is set properly by the layout system, by default the width is 100 and 
    // we wait for it to be set properly then put our helper widget on the RHS
    if (event->type() == QEvent::Resize && geometry().width() != 100) {

        // put somewhere nice on first show
        if (firstShow) {
            firstShow = false;
            helperWidget()->move(mainWidget()->geometry().width()-275, 50);
        }

        // if off the screen move on screen
        if (helperWidget()->geometry().x() > geometry().width()) {
            helperWidget()->move(mainWidget()->geometry().width()-275, 50);
        }
    }
    return QWidget::event(event);
}
