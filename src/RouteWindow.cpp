/*
 * Copyright (c) 2011, 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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
#include "RouteWindow.h"
#include "Athlete.h"
#include "GoogleMapControl.h"
#include "RideItem.h"
#include "Route.h"
#include "RouteItem.h"

#include <QDebug>
#include <QHeaderView>

using namespace std;

#define GOOGLE_KEY "ABQIAAAAS9Z2oFR8vUfLGYSzz40VwRQ69UCJw2HkJgivzGoninIyL8-QPBTtnR-6pM84ljHLEk3PDql0e2nJmg"

RouteWindow::RouteWindow(Context *context) :
    GcWindow(context), context(context), routeItem(NULL)
{
    this->setTitle("Routes");

    view = new QWebView();

    QVBoxLayout *layoutV = new QVBoxLayout();
    QHBoxLayout *layoutH = new QHBoxLayout();


    layout = new QVBoxLayout();
    //QLabel *label = new QLabel("Route segments");
    //layout->addWidget(label);
    layout->addWidget(view);

    // RIDES
    treeWidget = new QTreeWidget;

    treeWidget->setMinimumWidth(270);
    treeWidget->setColumnCount(3);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    treeWidget->header()->resizeSection(0,130);
    treeWidget->header()->resizeSection(1,70);
    treeWidget->header()->resizeSection(2,70);
    treeWidget->header()->hide();
    treeWidget->setAlternatingRowColors (true);
    treeWidget->setIndentation(10);
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);


    /*QWebView* rideSummary = new QWebView(this);
    rideSummary->setContentsMargins(0,0,0,0);
    rideSummary->page()->view()->setContentsMargins(0,0,0,0);
    rideSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rideSummary->setAcceptDrops(false);
    rideSummary->page()->mainFrame()->setHtml("TEST");
    rideSummary->rect().setHeight(50);*/

    rideTable = new QTableWidget;
    rideTable->setColumnCount(5);
    rideTable->horizontalHeader()->hide();
    rideTable->horizontalHeader()->resizeSection(0,130);
    rideTable->verticalHeader()->setDefaultSectionSize(15);
    rideTable->verticalHeader()->hide();
    rideTable->setSelectionMode(QAbstractItemView::SingleSelection);
    rideTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rideTable->setAlternatingRowColors (true);
    rideTable->setMaximumHeight(300);

    rideTable->setShowGrid(false);
    //rideTable->setHorizontalHeaderItem(0, item);

    layoutH->addLayout(layout);
    layoutH->addWidget(treeWidget);

    layoutV->addLayout(layoutH);
    //layoutV->addWidget(rideTable);

    setLayout(layoutV);




    // now we're up and running lets connect the signals
    connect(view, SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    //connect(view, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));

    connect(context->athlete->routes, SIGNAL(routesChanged()), this, SLOT(resetRoutes()));

    connect(treeWidget,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showTreeContextMenuPopup(const QPoint &)));
    connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(rideTreeWidgetSelectionChanged()));

    connect(treeWidget, SIGNAL(itemChanged()), this, SLOT(routeChanged()));
    connect(treeWidget, SIGNAL(itemPressed()), this, SLOT(routeChanged()));
    connect(treeWidget, SIGNAL(itemActivated()), this, SLOT(routeChanged()));




    webBridge = new WebBridgeForRoute(context, this);

    connect(view->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(updateFrame()));

    loadingPage = false;

    resetRoutes();
}

void RouteWindow::loadStarted()
{
    loadingPage = true;
}

/// called after the load is finished
/*void RouteWindow::loadFinished(bool)
{
    loadingPage = false;
    loadRide();
}*/

void RouteWindow::loadRide()
{
    qDebug() << "loadRide";
    createHtml();
    view->page()->mainFrame()->setHtml(currentPage);

    // Fill ride table
    /*
    rideTable->setRowCount(route->getRides().count());



    int i = 0;
    foreach (const RouteRide &ride, route->getRides()) {
        QDateTime dateTime = ride.startTime.addSecs(ride.start);

        //RouteItem* item = new RouteItem(route, RIDE_TYPE, mainWindow->home.path(), "test", dateTime, mainWindow);
        RouteItem* item = new RouteItem(route, &ride, mainWindow->home.path(), mainWindow);


        QTableWidgetItem* item0 = new QTableWidgetItem(0);
        item0->setText(dateTime.toString("MMM d, yyyy"));
        rideTable->setItem(i, 0, item0);

        QTableWidgetItem* item1 = new QTableWidgetItem(0);
        item1->setText(dateTime.toString("h:mm AP"));
        rideTable->setItem(i, 1, item1);

        QTableWidgetItem* item2 = new QTableWidgetItem(0);
        item2->setText(QString("%1m").arg(item->ride()->dataPoints().at(0)->alt));
        rideTable->setItem(i, 2, item2);

        i++;
    }
    */
    qDebug() << "loadRide END";
}

void RouteWindow::updateFrame()
{
    delete webBridge;
    webBridge = new WebBridgeForRoute(context, this);

    view->page()->mainFrame()->addToJavaScriptWindowObject("webBridge", webBridge);
}

void RouteWindow::createHtml()
{
    qDebug() << "route->getId() " << route->id();

    currentPage = "";
    double minLat, minLon, maxLat, maxLon;
    minLat = minLon = 1000;
    maxLat = maxLon = -1000; // larger than 360

    // get bounding co-ordinates for ride
    foreach(RoutePoint rfp, route->getPoints()) {
        if (rfp.lat || rfp.lon) {
            minLat = std::min(minLat,rfp.lat);
            maxLat = std::max(maxLat,rfp.lat);
            minLon = std::min(minLon,rfp.lon);
            maxLon = std::max(maxLon,rfp.lon);
        }
    }

    // No GPS data, so sorry no map
    if(!route) {
        currentPage = "No Route Data Present";
        return;
    }

    // load the Google Map v3 API
    currentPage = QString("<!DOCTYPE html> \n"
    "<html>\n"
    "<head>\n"
    "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=yes\"/> \n"
    "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n"
    "<title>Golden Cheetah Map</title>\n"
    "<link href=\"http://code.google.com/apis/maps/documentation/javascript/examples/default.css\" rel=\"stylesheet\" type=\"text/css\" /> \n"
    "<script type=\"text/javascript\" src=\"http://maps.googleapis.com/maps/api/js?sensor=false\"></script> \n");

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
    "        strokeColor: '#FFFF00',\n"
    "        strokeOpacity: 0.4,\n"
    "        strokeWeight: 10,\n"
    "        zIndex: -2\n"
    "    };\n"

    // load the GPS co-ordinates
    "    var latlons = webBridge.getLatLons(0);\n" // interval "0" is the entire route

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
    "}\n"

    "function drawIntervals() { \n"
    // intervals will be drawn with these options
    "    var polyOptions = {\n"
    "        strokeColor: '#0000FF',\n"
    "        strokeOpacity: 0.6,\n"
    "        strokeWeight: 10,\n"
    "        zIndex: -1\n"  // put at the bottom
    "    }\n"

    // remove previous intervals highlighted
    "    j= intervalList.length;\n"
    "    while (j) {\n"
    "       var highlighted = intervalList.pop();\n"
    "       highlighted.setMap(null);\n"
    "       j--;\n"
    "    }\n"
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

    "}\n"
    "</script>\n").arg(minLat,0,'g',11).arg(minLon,0,'g',11).arg(maxLat,0,'g',11).arg(maxLon,0,'g',11);

    // the main page is rather trivial
    currentPage += QString("</head>\n"
    "<body onload=\"initialize()\">\n"
    "<div id=\"map-canvas\"></div>\n"
    "</body>\n"
    "</html>\n");
}

string RouteWindow::CreateMapToolTipJavaScript()
{
    qDebug() << "CreateMapToolTipJavaScript()";

    ostringstream oss;
    oss << "<script type=\"text/javascript\">" << endl
        << "var MapTooltip = function(obj, html, options) {"<< endl
        << "this.obj = obj;"<< endl
        << "this.html = html;"<< endl
        << "this.options = options || {};"<< endl
        << "}"<< endl
        << "MapTooltip.prototype = new GOverlay();"<< endl
        << "MapTooltip.prototype.initialize = function(map) {"<< endl
        << "var div = document.getElementById('MapTooltipContainer');"<< endl
        << "var that = this;"<< endl
        << "if (!div) {"<< endl
        << "var div = document.createElement('div');"<< endl
        << "div.setAttribute('id', 'MapTooltipContainer');"<< endl
        << "}"<< endl
        << "if (this.options.maxWidth || this.options.minWidth) {"<< endl
        << "div.style.maxWidth = this.options.maxWidth || '150px';"<< endl
        << "div.style.minWidth = this.options.minWidth || '150px';"<< endl
        << "} else {"<< endl
        << "div.style.width = this.options.width || '150px';"<< endl
        << "}"<< endl
        << "div.style.padding = this.options.padding || '5px';"<< endl
        << "div.style.backgroundColor = this.options.backgroundColor || '#ffffff';"<< endl
        << "div.style.border = this.options.border || '1px solid #000000';"<< endl
        << "div.style.fontSize = this.options.fontSize || '80%';"<< endl
        << "div.style.color = this.options.color || '#000';"<< endl
        << "div.innerHTML = this.html;"<< endl
        << "div.style.position = 'absolute';"<< endl
        << "div.style.zIndex = '1000';"<< endl
        << "var offsetX = this.options.offsetX || 10;"<< endl
        << "var offsetY = this.options.offsetY || 0;"<< endl
        << "var bounds = map.getBounds();"<< endl
        << "rightEdge = map.fromLatLngToDivPixel(bounds.getNorthEast()).x;"<< endl
        << "bottomEdge = map.fromLatLngToDivPixel(bounds.getSouthWest()).y;"<< endl
        << "var mapev = GEvent.addListener(map, 'mousemove', function(latlng) {"<< endl
        << "GEvent.removeListener(mapev);"<< endl
        << "var pixelPosX = (map.fromLatLngToDivPixel(latlng)).x + offsetX;"<< endl
        << "var pixelPosY = (map.fromLatLngToDivPixel(latlng)).y - offsetY;"<< endl
        << "div.style.left = pixelPosX + 'px';"<< endl
        << "div.style.top = pixelPosY + 'px';"<< endl
        << "map.getPane(G_MAP_FLOAT_PANE).appendChild(div);"<< endl
        << "if ( (pixelPosX + div.offsetWidth) > rightEdge ) {"<< endl
        << "div.style.left = (rightEdge - div.offsetWidth - 10) + 'px';"<< endl
        << "}"<< endl
        << "if ( (pixelPosY + div.offsetHeight) > bottomEdge ) {"<< endl
        << "div.style.top = (bottomEdge - div.offsetHeight - 10) + 'px';"<< endl
        << "}"<< endl
        << "});"<< endl
        << "this._map = map;"<< endl
        << "this._div = div;"<< endl
        << "}"<< endl
        << "MapTooltip.prototype.remove = function() {"<< endl
        << "if(this._div != null) {"<< endl
        << "this._div.parentNode.removeChild(this._div);"<< endl
        << "}"<< endl
        << "}"<< endl
        << "MapTooltip.prototype.redraw = function(force) {"<< endl
        << "}"<< endl
        << "</script> "<< endl;
    return oss.str();
}

/// create the ride line
void RouteWindow::drawShadedRoute()
{
    qDebug() << "CreatePolyLine()";


    int count=0;  // how many samples ?

    QString code;

    foreach(RoutePoint rfp, route->getPoints()) {

        if (count == 0) {
            code = QString("{\n\nvar polyline = new google.maps.Polyline();\n"
                   "   polyline.setMap(map);\n"
                   "   path = polyline.getPath();\n");
        } else {
            code += QString("path.push(new google.maps.LatLng(%1,%2));\n").arg(rfp.lat,0,'g',11).arg(rfp.lon,0,'g',11);
        }

        count++;
    }

    QColor color = Qt::red;
    // color the polyline
    code += QString("var polyOptions = {\n"
                    "    strokeColor: '%1',\n"
                    "    strokeWeight: 3,\n"
                    "    strokeOpacity: 0.5,\n" // for out and backs, we need both
                    "    zIndex: 0,\n"
                    "}\n"
                    "polyline.setOptions(polyOptions);\n"
                    "}\n").arg(color.name());

    view->page()->mainFrame()->evaluateJavaScript(code);
}

void
RouteWindow::resetRoutes()
{
    qDebug() << "resetRoutes";
    treeWidget->hide();
    treeWidget->clear();

    routes = context->athlete->routes;
    if (routes->routes.count()>0) {
        for (int n=0;n<routes->routes.count();n++) {
            qDebug() << "route"<<n;
            route = &routes->routes[n];
            RouteItem *allRides = new RouteItem(route, FOLDER_TYPE, treeWidget, context) ; //new QTreeWidgetItem(treeWidget, FOLDER_TYPE);

            allRides->setText(0, route->getName());

            RouteItem *last = NULL;

            for (int j=0;j<route->getRides().count();j++) {
                QDateTime dt = route->getRides()[j].startTime;
                QDateTime dateTime = dt.addSecs(route->getRides()[j].start);

                last = new RouteItem(route, &route->getRides()[j], context->athlete->home->activities().path(), context);

                QString time = QTime(0,0,0,0).addSecs(route->getRides()[j].stop- route->getRides()[j].start).toString("hh:mm:ss");

                last->setText(0, dateTime.toString("MMM d, yyyy"));
                last->setText(1, dateTime.toString("h:mm AP"));
                last->setText(2, time);

                allRides->addChild(last);
            }

            treeWidget->expandItem(allRides);
            treeWidget->setFirstItemColumnSpanned (allRides, true);
        }

        route = &routes->routes[0];
        newRideToLoad = true;
        loadRide();
    }
    treeWidget->show();
    qDebug() << "resetRoutes END";
}

void
RouteWindow::rideTreeWidgetSelectionChanged()
{
    qDebug() << "rideTreeWidgetSelectionChanged";
    //if (routeToSave)
    //    routes->writeRoutes();


    assert(treeWidget->selectedItems().size() <= 1);
    if (treeWidget->selectedItems().isEmpty())
        route = NULL;
    else {
        QTreeWidgetItem *which = treeWidget->selectedItems().first();
        if (which->type() == ROUTE_TYPE) {
            RouteSegment *newroute  = &routes->routes[treeWidget->indexOfTopLevelItem(which)];
            if (newroute != route) {
                route = newroute;
                newRideToLoad = true;
                loadRide();
            }
        } else {
            RouteSegment *newroute = &routes->routes[treeWidget->indexOfTopLevelItem(which->parent())];
            if (newroute != route) { // New RouteSegment
                route = newroute;
                routeItem = (RouteItem *)which;
                newRideToLoad = true;
                loadRide();
            } else if (which != routeItem) { // New Ride for this route
                routeItem = (RouteItem *)which;
                newRideToLoad = true;
                loadRide();
            }

        }
    }
}

void
RouteWindow::showTreeContextMenuPopup(const QPoint &pos)
{
    routeItem = (RouteItem *)treeWidget->selectedItems().first();


    if (routeItem != NULL && routeItem->type() == ROUTE_TYPE) {
        QMenu menu(treeWidget);

        QAction *actRenRoute = new QAction(tr("Rename route"), treeWidget);
        connect(actRenRoute, SIGNAL(triggered(void)), this, SLOT(renameRoute()));

        QAction *actDeleteRoute = new QAction(tr("Delete route"), treeWidget);
        connect(actDeleteRoute, SIGNAL(triggered(void)), this, SLOT(deleteRoute()));

        menu.addAction(actRenRoute);
        menu.addAction(actDeleteRoute);

        menu.exec(treeWidget->mapToGlobal( pos ));
    }
}


void
RouteWindow::deleteRoute()
{
    // renumber remaining
    /*int oindex = activeInterval->displaySequence;
    for (int i=0; i<allIntervals->childCount(); i++) {
        IntervalItem *it = (IntervalItem *)allIntervals->child(i);
        int ds = it->displaySequence;
        if (ds > oindex) it->setDisplaySequence(ds-1);
    }

    // now delete!
    int index = allIntervals->indexOfChild(activeInterval);
    delete allIntervals->takeChild(index);
    this->; // will emit intervalChanged() signal*/
}

void
RouteWindow::renameRoute() {
    // go edit the name
    routeItem->setFlags(routeItem->flags() | Qt::ItemIsEditable);
    treeWidget->editItem(routeItem, 0);

    routeToSave = true;
}

void
RouteWindow::routeChanged(QTreeWidgetItem *item, int column) {
    qDebug() << "routeChanged";
    routes->writeRoutes();
}


// quick diag, used to debug code only
void WebBridgeForRoute::call(int count) { qDebug()<<"webBridge call:"<<count; }

// get a latlon array for the i'th selected interval
QVariantList
WebBridgeForRoute::getLatLons(int i)
{


    QVariantList latlons;
    if (gm->routeItem == NULL ||  gm->routeItem->type() == ROUTE_TYPE)
       return latlons; // not inited yet!

    int highlighted=0;
    RideFile *rideFile = gm->routeItem->ride();

    if (context->athlete->allIntervalItems() == NULL ||
       rideFile == NULL) return latlons; // not inited yet!

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
                        foreach (RideFilePoint *p1, rideFile->dataPoints()) {
                            if (p1->secs+rideFile->recIntSecs() > current->start
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
        if (rideFile && rideFile != NULL)
            foreach (RideFilePoint *p1, rideFile->dataPoints()) {
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
WebBridgeForRoute::drawOverlays()
{
    // overlay the markers
    //gm->createMarkers();

    // overlay a shaded route
    gm->drawShadedRoute();
}


