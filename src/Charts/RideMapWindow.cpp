/*
 * Copyright (c) 2009 Greg Lonnon (greg.lonnon@gmail.com)
 *               2011 Mark Liversedge (liversedge@gmail.com)
 *               2016,2018 Damien Grauser (Damien.Grauser@gmail.com)
 *               2018 Michael Beaulieu (michael.beaulieu@notiotechnologies.com)
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

#include "RideMapWindow.h"

#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "SmallPlot.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"

#include <QtWebChannel>
#include <QWebEngineView>
#include <QWebEngineSettings>

// overlay helper
#include "AbstractView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include <QDebug>

RideMapWindow::RideMapWindow(Context *context, int mapType) : GcChartWindow(context), context(context),
                                                       range(-1), current(NULL), firstShow(true), stale(false)
{
    //
    // Chart settings
    //

    QWidget *settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0,0,0,0);

    QFormLayout *commonLayout = new QFormLayout(settingsWidget);

    // map choice
    mapCombo= new QComboBox(this);
    mapCombo->addItem(tr("OpenStreetMap"));
    mapCombo->addItem(tr("Google"));

    setMapType(mapType); // validate mapType and set current index in mapCombo

    showMarkersCk = new QCheckBox();
    showFullPlotCk = new QCheckBox();
    hideShadedZonesCk = new QCheckBox();
    hideShadedZonesCk->setChecked(false);
    hideYellowLineCk = new QCheckBox();
    hideYellowLineCk->setChecked(false);
    hideRouteLineOpacityCk = new QCheckBox();
    hideRouteLineOpacityCk->setChecked(false);
    showInt = new QCheckBox();
    showInt->setChecked(true);

    commonLayout->addRow(new QLabel(tr("Map")), mapCombo);
    commonLayout->addRow(new QLabel(tr("Show Markers")), showMarkersCk);
    commonLayout->addRow(new QLabel(tr("Show Full Plot")), showFullPlotCk);
    commonLayout->addRow(new QLabel(tr("Hide Shaded Zones")), hideShadedZonesCk);
    commonLayout->addRow(new QLabel(tr("Hide Yellow Line")), hideYellowLineCk);
    commonLayout->addRow(new QLabel(tr("Hide Out & Back Route Opacity")), hideRouteLineOpacityCk);
    commonLayout->addRow(new QLabel(tr("Show Intervals Overlay")), showInt);
    commonLayout->addRow(new QLabel(""));

    osmTSTitle = new QLabel(tr("Open Street Map - Custom Tile Server settings"));
    osmTSTitle->setStyleSheet("font-weight: bold;");
    osmTSLabel = new QLabel(tr("Tile server"));
    osmTSUrlLabel = new QLabel(tr("Tile server URL"));

    // tile choice
    tileCombo= new QComboBox(this);
    tileCombo->addItem(tr("OpenStreetMap (default)"), QVariant(0));
    tileCombo->addItem(tr("Custom Tile Server A"), QVariant(10));
    tileCombo->addItem(tr("Custom Tile Server B"), QVariant(20));
    tileCombo->addItem(tr("Custom Tile Server C"), QVariant(30));

    osmTSUrl = new QLineEdit("");
    osmTSUrl->setFixedWidth(600);

    osmGrayLabel = new QLabel(tr("Map Grayscale Filter"));
    osmGraySlider = new QSlider();
    osmGraySlider->setOrientation(Qt::Horizontal);
    osmGraySlider->setRange(0, 10);
    osmGraySlider->setTickInterval(1);
    osmGraySlider->setTickPosition(QSlider::TicksBelow);

    gkey = new QLineEdit("");
    gkeylabel = new QLabel(tr("Google API key"));

    // set default Tile Server and URL
    tileCombo->setCurrentIndex(0);
    setTileServerUrlForTileType(0);

    commonLayout->addRow(osmTSTitle);
    commonLayout->addRow(osmTSLabel, tileCombo);
    commonLayout->addRow(osmTSUrlLabel, osmTSUrl);
    commonLayout->addRow(osmGrayLabel, osmGraySlider);
    commonLayout->addRow(gkeylabel, gkey);

    connect(mapCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(mapTypeSelected(int)));
    connect(showMarkersCk, SIGNAL(stateChanged(int)), this, SLOT(showMarkersChanged(int)));
    connect(showInt, SIGNAL(stateChanged(int)), this, SLOT(showIntervalsChanged(int)));
    connect(showFullPlotCk, SIGNAL(stateChanged(int)), this, SLOT(showFullPlotChanged(int)));
    connect(hideShadedZonesCk, SIGNAL(stateChanged(int)), this, SLOT(hideShadedZonesChanged(int)));
    connect(hideYellowLineCk, SIGNAL(stateChanged(int)), this, SLOT(hideYellowLineChanged(int)));
    connect(hideRouteLineOpacityCk, SIGNAL(stateChanged(int)), this, SLOT(hideRouteLineOpacityChanged(int)));
    connect(osmTSUrl, SIGNAL(editingFinished()), this, SLOT(osmCustomTSURLEditingFinished()));
    connect(osmGraySlider, SIGNAL(valueChanged(int)), this, SLOT(osmGrayValueChanged(int)));
    connect(tileCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(tileTypeSelected(int)));

    setControls(settingsWidget);

    setContentsMargins(0,0,0,0);
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setChartLayout(layout);

    view = new QWebEngineView(this);
    // stop stealing focus!
    view->settings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);
    view->setPage(new QWebEnginePage(context->webEngineProfile));
    view->setContentsMargins(0,0,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    view->page()->setHtml("<html></html>");
    layout->addWidget(view);

    HelpWhatsThis *help = new HelpWhatsThis(view);
    view->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Map));

    webBridge = new MapWebBridge(context, this);
    // file: MyWebEngineView.cpp, MyWebEngineView extends QWebEngineView
    QWebChannel *channel = new QWebChannel(view->page());

    // set the web channel to be used by the page
    // see http://doc.qt.io/qt-5/qwebenginepage.html#setWebChannel
    view->page()->setWebChannel(channel);

    // register QObjects to be exposed to JavaScript
    channel->registerObject(QStringLiteral("webBridge"), webBridge);

    // put a helper on the screen for mouse over intervals...
    overlayIntervals = new IntervalSummaryWindow(context);
    addHelper(tr("Intervals"), overlayIntervals);

    //
    // connects
    //
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));

    connect(context, SIGNAL(rideChanged(RideItem*)), this, SLOT(forceReplot()));
    connect(context, SIGNAL(intervalsChanged()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalSelected()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalZoom(IntervalItem*)), this, SLOT(zoomInterval(IntervalItem*)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(compareIntervalsStateChanged(bool)));
    connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(compareIntervalsChanged()));
    connect(context, SIGNAL(compareIntervalsChanged()), webBridge, SLOT(compareIntervalsChanged()));

    // just the hr and power as a plot
    smallPlot = new SmallPlot(this);
    smallPlot->enableTracking();
    connect(smallPlot, SIGNAL(selectedPosX(double)), this, SLOT(showPosition(double)));
    connect(smallPlot, SIGNAL(mouseLeft()), this, SLOT(hidePosition()));
    smallPlot->setMaximumHeight(200);
    smallPlot->setMinimumHeight(100);
    smallPlot->setVisible(false);
    layout->addWidget(smallPlot);
    layout->setStretch(1, 20);

    setCustomTSWidgetVisible(mapCombo->currentIndex() == 0);

    configChanged(CONFIG_APPEARANCE);
}

RideMapWindow::~RideMapWindow()
{
    delete webBridge;
    if (view) delete view->page();
}


void
RideMapWindow::mapTypeSelected(int x)
{
    setMapType(x);
    if (x==0) {
        setCustomTSWidgetVisible(true);
    } else {
        setCustomTSWidgetVisible(false);
    }
    forceReplot();
}


void
RideMapWindow::showPosition(double mins)
{
    long secs = mins * 60;
    int idx = secs / 5;
    idx = std::max(idx, 0);
    idx = std::min(idx, (int)positionItems.length() - 1);
    PositionItem positionItem = positionItems.at(idx);
    view->page()->runJavaScript(QString("setPosMarker(%1, %2);").arg(positionItem.lat).arg(positionItem.lng));
}


void
RideMapWindow::hidePosition()
{
    view->page()->runJavaScript(QString("hidePosMarker();"));
}


void
RideMapWindow::setCustomTSWidgetVisible(bool value)
{
    osmTSTitle->setVisible(value);
    osmTSLabel->setVisible(value);
    osmTSUrlLabel->setVisible(value);
    osmTSUrl->setVisible(value);
    osmGrayLabel->setVisible(value);
    osmGraySlider->setVisible(value);
    tileCombo->setVisible(value);
    gkeylabel->setVisible(!value);
    gkey->setVisible(!value);
}

void
RideMapWindow::setTileServerUrlForTileType(int x)
{
    QString ts = "";
    switch (x)
    {
    case 0:
        ts = appsettings->cvalue(context->athlete->cyclist, GC_OSM_TS_DEFAULT, "").toString();
        // set/save the default if necessary
        if (ts.isEmpty()) {
           ts = "https://{s}.tile.openstreetmap.org";
           appsettings->setCValue(context->athlete->cyclist, GC_OSM_TS_DEFAULT, ts);
        }
        break;
    case 10:
        ts = appsettings->cvalue(context->athlete->cyclist, GC_OSM_TS_A, "").toString();
        // set/save some useful default if empty
        if (ts.isEmpty()) {
           ts = "https://{s}.tile.openstreetmap.de";
           appsettings->setCValue(context->athlete->cyclist, GC_OSM_TS_A, ts);
        }
        break;
    case 20:
        ts = appsettings->cvalue(context->athlete->cyclist, GC_OSM_TS_B, "").toString();
        // set/save some useful default if empty
        if (ts.isEmpty()) {
           ts = "https://{s}.tile.openstreetmap.fr/osmfr";
           appsettings->setCValue(context->athlete->cyclist, GC_OSM_TS_B, ts);
        }
        break;
    case 30:
        ts = appsettings->cvalue(context->athlete->cyclist, GC_OSM_TS_C, "").toString();
        // set/save some useful default if empty
        if (ts.isEmpty()) {
           ts = "https://tile.thunderforest.com/cycle";
           appsettings->setCValue(context->athlete->cyclist, GC_OSM_TS_C, ts);
        }
        break;
    }
    osmTSUrl->setText(ts);
}


void
RideMapWindow::setGrayscaleForTileType
(int x)
{
    int gray = 0;
    switch (x) {
    case 0:
        gray = appsettings->cvalue(context->athlete->cyclist, GC_OSM_DEFAULT_GRAY, "-1").toInt();
        // set/save the default if necessary
        if (gray < 0) {
           gray = 0;
           appsettings->setCValue(context->athlete->cyclist, GC_OSM_DEFAULT_GRAY, gray);
        }
        break;
    case 10:
        gray = appsettings->cvalue(context->athlete->cyclist, GC_OSM_A_GRAY, "-1").toInt();
        // set/save the default if necessary
        if (gray < 0) {
           gray = 0;
           appsettings->setCValue(context->athlete->cyclist, GC_OSM_A_GRAY, gray);
        }
        break;
    case 20:
        gray = appsettings->cvalue(context->athlete->cyclist, GC_OSM_B_GRAY, "-1").toInt();
        // set/save the default if necessary
        if (gray < 0) {
           gray = 0;
           appsettings->setCValue(context->athlete->cyclist, GC_OSM_B_GRAY, gray);
        }
        break;
    case 30:
        gray = appsettings->cvalue(context->athlete->cyclist, GC_OSM_C_GRAY, "-1").toInt();
        // set/save the default if necessary
        if (gray < 0) {
           gray = 0;
           appsettings->setCValue(context->athlete->cyclist, GC_OSM_C_GRAY, gray);
        }
        break;
    }
    osmGraySlider->setValue(gray);
}


void
RideMapWindow::tileTypeSelected(int x)
{
    setTileServerUrlForTileType(tileCombo->itemData(x).toInt());
    setGrayscaleForTileType(tileCombo->itemData(x).toInt());
    forceReplot();
}


void
RideMapWindow::showMarkersChanged(int value)
{
    Q_UNUSED(value);
    if (context->isCompareIntervals) {
        return;
    }
    forceReplot();
}

void
RideMapWindow::showIntervalsChanged(int value)
{
    if (context->isCompareIntervals) {
        return;
    }
    // show or hide the helper
    overlayWidget->setVisible(value);
}

void
RideMapWindow::showFullPlotChanged(int value)
{
    if (context->isCompareIntervals) {
        return;
    }
    smallPlot->setVisible(value != 0);
}

void
RideMapWindow::hideShadedZonesChanged(int value)
{
    Q_UNUSED(value);
    if (context->isCompareIntervals) {
        return;
    }
    forceReplot();
}

void
RideMapWindow::hideYellowLineChanged(int value)
{
    Q_UNUSED(value);
    if (context->isCompareIntervals) {
        return;
    }
    forceReplot();
}

void
RideMapWindow::hideRouteLineOpacityChanged(int value)
{
    Q_UNUSED(value);
    if (context->isCompareIntervals) {
        return;
    }
    forceReplot();
}

void
RideMapWindow::osmCustomTSURLEditingFinished()
{

    // just store the text - even if not changed
    switch (osmTS())
    {
    case 0:
        appsettings->setCValue(context->athlete->cyclist, GC_OSM_TS_DEFAULT, osmTSUrl->text());
        break;
    case 10:
        appsettings->setCValue(context->athlete->cyclist, GC_OSM_TS_A, osmTSUrl->text());
        break;
    case 20:
        appsettings->setCValue(context->athlete->cyclist, GC_OSM_TS_B, osmTSUrl->text());
        break;
    case 30:
        appsettings->setCValue(context->athlete->cyclist, GC_OSM_TS_C, osmTSUrl->text());
        break;
    }

    forceReplot();
}


void
RideMapWindow::osmGrayValueChanged
(int value)
{
    switch (osmTS()) {
    case 0:
        appsettings->setCValue(context->athlete->cyclist, GC_OSM_DEFAULT_GRAY, value);
        break;
    case 10:
        appsettings->setCValue(context->athlete->cyclist, GC_OSM_A_GRAY, value);
        break;
    case 20:
        appsettings->setCValue(context->athlete->cyclist, GC_OSM_B_GRAY, value);
        break;
    case 30:
        appsettings->setCValue(context->athlete->cyclist, GC_OSM_C_GRAY, value);
        break;
    }

    // Only update the filter if the tile pane is already available
    view->page()->runJavaScript(QString("{\n"
                                        "    const elems = document.getElementsByClassName('leaflet-tile-pane');\n"
                                        "    if (elems.length > 0 && elems[0] !== null && elems[0].style !== undefined) {\n"
                                        "        elems[0].style.filter = 'grayscale(%1)';\n"
                                        "    }\n"
                                        "}\n").arg(value / 10.0));
}


void
RideMapWindow::configChanged(qint32 value)
{
    setProperty("color", GColor(CPLOTBACKGROUND));
#ifndef Q_OS_MAC
    overlayIntervals->setStyleSheet(AbstractView::ourStyleSheet());
#endif

    if (value & CONFIG_APPEARANCE) forceReplot();
}

void
RideMapWindow::forceReplot()
{
    if (context->isCompareIntervals) {
        createHtml();
        view->page()->setHtml(currentPage);
    } else {
        stale=true;
        rideSelected();
    }
}

void
RideMapWindow::rideSelected()
{
    if (context->isCompareIntervals) {
        return;
    }

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

    if (context->athlete->zones(ride->sport)) {
        range = context->athlete->zones(ride->sport)->whichRange(ride->dateTime.date());
        if (range >= 0) rideCP = context->athlete->zones(ride->sport)->getCP(range);
    }

    loadRide();
    smallPlot->setData(ride);
}

void RideMapWindow::loadRide()
{
    createHtml();
    buildPositionList();

    view->page()->setHtml(currentPage);
}

void RideMapWindow::updateFrame()
{
    // deleting the web bridge seems to be the only way to
    // reset state between it and the webpage.
    delete webBridge;
    webBridge = new MapWebBridge(context, this);
    connect(context, SIGNAL(intervalsChanged()), webBridge, SLOT(intervalsChanged()));
    connect(context, SIGNAL(intervalSelected()), webBridge, SLOT(intervalsChanged()));
}

void RideMapWindow::createHtml()
{
    RideItem * ride = myRideItem;
    currentPage = "";

    double minLat = 1000;
    double minLon = 1000;
    double maxLat = -1000;
    double maxLon = -1000;
    bool hasComparePositions = false;

    if (context->isCompareIntervals) {
        hasComparePositions = getCompareBoundingBox(minLat, maxLat, minLon, maxLon);
    }

    // No GPS data, so sorry no map
    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);
    if (   (   context->isCompareIntervals
            && ! hasComparePositions)
        || (   ! context->isCompareIntervals
            && (!ride || !ride->ride() || ride->ride()->areDataPresent()->lat == false || ride->ride()->areDataPresent()->lon == false))) {
        currentPage = QString("<STYLE>BODY { background-color: %1; color: %2 }</STYLE><center>%3</center>").arg(bgColor.name()).arg(fgColor.name()).arg(tr("No GPS Data Present"));
        setIsBlank(true);
        return;
    } else {
        setIsBlank(false);
    }

    // get bounding coordinates for ride
    if (! context->isCompareIntervals) {
        foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints()) {
            if (rfp->lat || rfp->lon) {
                minLat = std::min(minLat,rfp->lat);
                maxLat = std::max(maxLat,rfp->lat);
                minLon = std::min(minLon,rfp->lon);
                maxLon = std::max(maxLon,rfp->lon);
            }
        }
    }

    // html page
    currentPage = QString("<!DOCTYPE html> \n"
    "<html>\n"
    "<head>\n"
    "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=yes\"/> \n"
    "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n"
    "<title>Golden Cheetah Map</title>\n"
    "<style type=\"text/css\">\n"
    "   html { height: 100% }\n"
    "   body { height: 100%; margin: 0; padding: 0 }\n"
    "   #map-canvas { height: 100% }\n"
    "</style>\n");

    // load the js API
    if (mapCombo->currentIndex() == OSM) {
        // Load leaflet (1.9.4) API
        currentPage += QString("<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.css\" integrity=\"sha256-p4NxAoJBhIIN+hmNHrzRCf9tD/miZyoHS5obTRR9BMY=\" crossorigin=\"\" /> \n"
                               "<script src=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.js\" integrity=\"sha256-20nQCchB9co0qIjJZRGuk2/Z9VM+kNiyxNV1lvTlZBo=\" crossorigin=\"\"></script>\n");

        // Add grayscale filter if required
        if (osmGraySlider->value() > 0) {
            currentPage += QString("<style type=\"text/css\">\n"
                                   "    .leaflet-tile-pane {\n"
                                   "        filter: grayscale(%1);\n"
                                   "    }\n"
                                   "</style>\n").arg(osmGraySlider->value() / 10.0);
        }
    } else if (mapCombo->currentIndex() == GOOGLE) {
        // Load Google Map v3 API
        currentPage += QString("<script type=\"text/javascript\" src=\"http://maps.googleapis.com/maps/api/js?key=%1\"></script> \n").arg(gkey->text());
    }

    currentPage += QString("<script type=\"text/javascript\" src=\"qrc:///qtwebchannel/qwebchannel.js\"></script>\n");

    currentPage += QString("<script type=\"text/javascript\"> \n"
    "var webBridge; \n"
    "window.onload = function () { \n"
    "<!-- it's a good idea to initialize webchannel after DOM ready, if the code is going to manipulate the DOM -->\n"
    "   new QWebChannel(qt.webChannelTransport, function (channel) { \n"
    "       webBridge = channel.objects.webBridge; \n"
    "       initialize(); \n"
    "   }); \n"
    "}; \n"
    "</script>");

    // fg/bg
    currentPage += QString("<STYLE>BODY { background-color: %1; color: %2 }</STYLE>")
                                          .arg(bgColor.name()).arg(fgColor.name());

    currentPage += QString("</head>\n"
            "<body>\n"
            "<div id=\"map-canvas\"></div>\n");

    // local functions
    currentPage += QString("<script type=\"text/javascript\">\n"
                           "var map;\n");  // the map object
    if (context->isCompareIntervals) {
        currentPage += QString("var compareIntervalGroup;\n");  // compare Intervals
    } else {
        currentPage += QString("var intervalList;\n"  // array of intervals
                               "var markerList;\n"  // array of markers
                               "var polyList;\n"  // array of polylines
                               "var tmpIntervalHighlighter;\n"  // temp interval
                               "var posMarker;\n");  // marker for position tracking
    }

    if (mapCombo->currentIndex() == OSM) {
        currentPage += QString("const svgPosIcon = L.divIcon({\n"
            "html: `\n"
            "<svg\n"
            "  width=\"16\"\n"
            "  height=\"16\"\n"
            "  viewBox=\"0 0 100 100\"\n"
            "  version=\"1.1\"\n"
            "  preserveAspectRatio=\"none\"\n"
            "  xmlns=\"http://www.w3.org/2000/svg\"\n"
            ">\n"
            "  <circle cx=\"50\" cy=\"50\" r=\"50\" fill=\"green\"/>\n"
            "</svg>`,\n"
            "  className: \"svg-pos-marker\",\n"
            "  iconSize: [16, 16],\n"
            "  iconAnchor: [8, 8],\n"
            "});\n");
    }


    //////////////////////////////////////////////////////////////////////
    // drawRoute

    if (! context->isCompareIntervals) {
        currentPage += QString(""
                               // Draw the entire route, we use a local webbridge
                               // to supply the data to a) reduce bandwidth and
                               // b) allow local manipulation. This makes the UI
                               // considerably more 'snappy'
                               "function drawRoute() {\n"
                               // load the GPS co-ordinates
                               "   webBridge.getLatLons(0, drawRouteForLatLons);\n"
                               "}\n"
                               "\n");

        if (mapCombo->currentIndex() == OSM) {
            currentPage += QString("function drawRouteForLatLons(latlons) {\n"

                // route will be drawn with these options
                "    var routeOptionsYellow = {\n"
                "        stroke : true,\n"
                "        color: '#FFFF00',\n"
                "        opacity: %1,\n"
                "        weight: 10,\n"
                "        zIndex: -2\n"
                "    };\n"

                // lastly, populate the route path
                "    var path = [];\n"
                "    var j=0;\n"
                "    while (j < latlons.length) { \n"
                "        path.push(new L.LatLng(latlons[j], latlons[j+1]));\n"
                "        j += 2;\n"
                "    };\n"

                "    var routeYellow = new L.Polyline(path, routeOptionsYellow).addTo(map);\n"

                // Listen mouse events
                "    routeYellow.on('mousedown', function(event) { map.dragging.disable();L.DomEvent.stopPropagation(event);webBridge.clickPath(event.latlng.lat, event.latlng.lng); });\n" // map.setOptions({draggable: false, zoomControl: false, scrollwheel: false, disableDoubleClickZoom: true});
                "    routeYellow.on('mouseup',   function(event) { map.dragging.enable();L.DomEvent.stopPropagation(event);webBridge.mouseup(); });\n" // setOptions ?
                "    routeYellow.on('mouseover', function(event) { webBridge.hoverPath(event.latlng.lat, event.latlng.lng); });\n"
                "    routeYellow.on('mousemove', function(event) { webBridge.hoverPath(event.latlng.lat, event.latlng.lng); });\n"

                "}\n").arg(hideYellowLine() ? 0.0 : 0.4f);
        }
        else if (mapCombo->currentIndex() == GOOGLE) {

           currentPage += QString("function drawRouteForLatLons(latlons) {\n"

               // route will be drawn with these options
               "    var routeOptionsYellow = {\n"
               "        strokeColor: '#FFFF00',\n"
               "        strokeOpacity: %1,\n"
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

               "}\n").arg(hideYellowLine() ? 0.0 : 0.4f);
        }
    }


    //////////////////////////////////////////////////////////////////////
    // drawCompareIntervals

    if (context->isCompareIntervals) {
        currentPage += QString("function drawCompareIntervals() { \n"
                               "   webBridge.setCompareIntervals(doDrawCompareIntervals);\n"
                               "} \n");

        if (mapCombo->currentIndex() == OSM) {
            currentPage += QString("function doDrawCompareIntervals(intervalInfos) { \n"
                                   "    compareIntervalGroup.clearLayers();\n"
                                   "    for (i in intervalInfos) {\n"
                                   "        var interval = intervalInfos[i];\n"
                                   "        var polyOptions = {\n"
                                   "            stroke : true,\n"
                                   "            color: interval['color'],\n"
                                   "            opacity: 0.8,\n"
                                   "            weight: 5,\n"
                                   "            zIndex: -1\n"
                                   "        }\n"
                                   "        var path = [];\n"
                                   "        for (j in interval['lats']) {\n"
                                   "            path.push([interval['lats'][j], interval['lons'][j]]);\n"
                                   "        }\n"
                                   "        compareIntervalGroup.addLayer(L.polyline(path, polyOptions));\n"
                                   "    }\n"
                                   "    map.fitBounds(compareIntervalGroup.getBounds());\n"
                                   "} \n");
        } else if (mapCombo->currentIndex() == GOOGLE) {
            currentPage += QString("function doDrawCompareIntervals(intervalInfos) { \n"
                                   "    var minLat = 1000;\n"
                                   "    var minLon = 1000;\n"
                                   "    var maxLat = -1000;\n"
                                   "    var maxLon = -1000;\n"
                                   "    for (r in compareIntervalGroup) {\n"
                                   "        compareIntervalGroup[r].setMap(null);\n"
                                   "    }\n"
                                   "    compareIntervalGroup = [];\n"
                                   "    for (i in intervalInfos) {\n"
                                   "        var interval = intervalInfos[i];\n"
                                   "        var routeOptions = {\n"
                                   "            strokeColor: interval['color'],\n"
                                   "            strokeOpacity: 0.8,\n"
                                   "            strokeWeight: 5,\n"
                                   "            zIndex: -1\n"
                                   "        };\n"
                                   "        var route = new google.maps.Polyline(routeOptions);\n"
                                   "        route.setMap(map);\n"
                                   "        var path = route.getPath();\n"
                                   "        for (j in interval['lats']) {\n"
                                   "            path.push(new google.maps.LatLng(interval['lats'][j], interval['lons'][j]));\n"
                                   "            minLat = Math.min(minLat, interval['lats'][j]);\n"
                                   "            maxLat = Math.max(maxLat, interval['lats'][j]);\n"
                                   "            minLon = Math.min(minLon, interval['lons'][j]);\n"
                                   "            maxLon = Math.max(maxLon, interval['lons'][j]);\n"
                                   "        }\n"
                                   "        compareIntervalGroup.push(route);\n"
                                   "    }\n"
                                   "    var southwest = new google.maps.LatLng(minLat, minLon);\n"
                                   "    var northeast = new google.maps.LatLng(maxLat, maxLon);\n"
                                   "    map.fitBounds(new google.maps.LatLngBounds(southwest, northeast));\n"
                                   "} \n");
        }
    }


    //////////////////////////////////////////////////////////////////////
    // drawIntervals

    if (! context->isCompareIntervals) {
        currentPage += QString("function drawIntervals() { \n"
        // how many to draw?
        "   webBridge.intervalCount(drawIntervalsCount);\n"
        "}\n"

        "function drawIntervalsCount(intervals) { \n"
        // remove previous intervals highlighted
        "   j= intervalList.length;\n"
        "    while (j) {\n"
        "       var highlighted = intervalList.pop();\n");

        // remove highlighted
        if (mapCombo->currentIndex() == OSM) {
            currentPage += QString(""
            "       map.removeLayer(highlighted);\n");
        } else if (mapCombo->currentIndex() == GOOGLE) {
            currentPage += QString(""
            "       highlighted.setMap(null);\n");
        }

        currentPage += QString(""
        "       j--;\n"
        "    }\n"

        "   while (intervals > 0) {\n"
        "       webBridge.getLatLons(intervals, drawInterval);\n"
        "       intervals--;\n"
        "   }\n"
        "}\n");

        if (mapCombo->currentIndex() == OSM) {
            currentPage += QString("function drawInterval(latlons) { \n"
                                   // intervals will be drawn with these options
                                   "   var polyOptions = {\n"
                                   "       stroke : true,\n"
                                   "       color: '#0000FF',\n"
                                   "       opacity: 0.6,\n"
                                   "       weight: 10,\n"
                                   "       zIndex: -1\n"  // put at the bottom
                                   "   }\n"
                                   "   var path = [];\n"
                                   "   var j=0;\n"
                                   "   while (j<latlons.length) {\n"
                                   "       path.push([latlons[j], latlons[j+1]]);\n"
                                   "       j += 2;\n"
                                   "   }\n"

                                   "   var intervalHighlighter = L.polyline(path, polyOptions).addTo(map);\n"
                                   "   intervalHighlighter.on('mouseover', function(event) { L.DomEvent.stopPropagation(event); webBridge.hoverPath(event.latlng.lat, event.latlng.lng); });\n"
                                   "   intervalHighlighter.on('mousemove', function(event) { L.DomEvent.stopPropagation(event); webBridge.hoverPath(event.latlng.lat, event.latlng.lng); });\n"
                                   "   intervalList.push(intervalHighlighter);\n"
                                   "}\n");
        } else if (mapCombo->currentIndex() == GOOGLE) {
            currentPage += QString("function drawInterval(latlons) { \n"
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
                                   "}\n");
        }
    }


    //////////////////////////////////////////////////////////////////////
    // Position Marker

    if (mapCombo->currentIndex() == OSM) {
        currentPage += QString("function setPosMarker(lat, lng) {\n"
                               "    var latlng = new L.LatLng(lat, lng);\n"
                               "    if (typeof posMarker !== 'undefined') {\n"
                               "        posMarker.setLatLng(latlng);\n"
                               "    } else {\n"
                               "        posMarker = new L.marker(latlng, {icon: svgPosIcon});\n"
                               "        posMarker.addTo(map);\n"
                               "    }\n"
                               "}\n"
                               "\n"
                               "function hidePosMarker() {\n"
                               "    if (typeof posMarker !== 'undefined') {\n"
                               "        posMarker.remove();\n"
                               "        posMarker = undefined;\n"
                               "    }\n"
                               "}\n");
    } else if (mapCombo->currentIndex() == GOOGLE) {
        currentPage += QString("function setPosMarker(lat, lng) {\n"
                               "    var latlng = new google.maps.LatLng(lat, lng);\n"
                               "    if (typeof posMarker !== 'undefined') {\n"
                               "        posMarker.setPosition(latlng);\n"
                               "    } else {\n"
                               "        posMarker = new google.maps.Marker({ position: latlng });\n"
                               "        posMarker.setMap(map);\n"
                               "    }\n"
                               "}\n"
                               "\n"
                               "function hidePosMarker() {\n"
                               "    if (typeof posMarker !== 'undefined') {\n"
                               "        posMarker.setMap(null);\n"
                               "        posMarker = undefined;\n"
                               "    }\n"
                               "}\n");
    }


    //////////////////////////////////////////////////////////////////////
    // Initialize

    currentPage += QString(""
                           // initialise function called when map loaded
                           "function initialize() {\n");
    if (mapCombo->currentIndex() == OSM) {
        currentPage += QString(""
                               // TERRAIN style map please and make it draggable
                               // note that because QT webkit offers touch/gesture
                               // support the Google API only supports dragging
                               // via gestures - this is alrady registered as a bug
                               // with the google map team
                               "    var controlOptions = {\n"
                               "    };\n"
                               "    var myOptions = {\n"
                               "      draggable: true,\n"
                               "      mapTypeId: \"OSM\",\n"
                               "      mapTypeControl: false,\n"
                               "      streetViewControl: false,\n"
                               "      tilt: 45,\n"
                               "    };\n");

        currentPage += QString(""
                               // setup the map, and fit to contain the limits of the route
                               "    map = new L.map('map-canvas');\n"
                               "    map.fitBounds([[%1, %2], [%3, %4]]);\n").
                               arg(minLat,0,'g',GPS_COORD_TO_STRING).
                               arg(minLon,0,'g',GPS_COORD_TO_STRING).
                               arg(maxLat,0,'g',GPS_COORD_TO_STRING).
                               arg(maxLon,0,'g',GPS_COORD_TO_STRING);

        // If provided URL doesn't contain the required part, we add it,
        // otherwise it is used without changes to allow inclusion of apikey.
        QString tsReq = "/{z}/{x}/{y}.png";
        QString tsUrl = osmTSUrl->text().contains(tsReq) ? osmTSUrl->text() : osmTSUrl->text() + tsReq;
        currentPage += QString(""
                               "    L.tileLayer('%1', {"
                               "                 attribution: '&copy; <a href=\"http://openstreetmap.org\">OpenStreetMap</a> contributors',"
                               "                 maxZoom: 18"
                               "              }).addTo(map);\n").arg(tsUrl);

        if (context->isCompareIntervals) {
            currentPage += QString(""
                                   // initialise local variables
                                   "    compareIntervalGroup = L.featureGroup().addTo(map);\n"

                                   "    drawCompareIntervals();\n"
                                   // catch signals to redraw intervals
                                   "    webBridge.drawCompareIntervals.connect(drawCompareIntervals);\n");
        } else {
            currentPage += QString(""
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
                                   "    map.on('mouseup', function(event) { map.dragging.enable();L.DomEvent.stopPropagation(event); webBridge.mouseup(); });\n");
        }
        currentPage += QString(""
                               "}\n"
                               "</script>\n");
    } else if (mapCombo->currentIndex() == GOOGLE) {
        // TERRAIN style map please and make it draggable
        // note that because QT webkit offers touch/gesture
        // support the Google API only supports dragging
        // via gestures - this is alrady registered as a bug
        // with the google map team
        currentPage += QString(""
            "    var controlOptions = {\n"
            "      style: google.maps.MapTypeControlStyle.DEFAULT\n"
            "    };\n");

        currentPage += QString(
            "    var myOptions = {\n"
            "      draggable: true,\n"
            "      mapTypeControlOptions: { mapTypeIds: ['roadmap', 'satellite', 'hybrid', 'terrain', 'styled_map'] },\n"
            "      mapTypeId: %1,\n"
            "      disableDefaultUI: %2,\n"
            "      tilt: 45,\n"
            "      streetViewControl: false,\n"
            "    };\n").arg("google.maps.MapTypeId.TERRAIN")
                       .arg("false");

        currentPage += QString(""
           // setup the map, and fit to contain the limits of the route
           "    map = new google.maps.Map(document.getElementById('map-canvas'), myOptions);\n"
           "    var sw = new google.maps.LatLng(%1,%2);\n"  // .ARG 1, 2
           "    var ne = new google.maps.LatLng(%3,%4);\n"  // .ARG 3, 4
           "    var bounds = new google.maps.LatLngBounds(sw,ne);\n"
           "    map.fitBounds(bounds);\n").arg(minLat,0,'g',GPS_COORD_TO_STRING).
                arg(minLon,0,'g',GPS_COORD_TO_STRING).
                arg(maxLat,0,'g',GPS_COORD_TO_STRING).
                arg(maxLon,0,'g',GPS_COORD_TO_STRING);

        currentPage += QString(""
            // add the bike layer, useful in some areas, but coverage
            // is limited, US gets best coverage at this point (Summer 2011)
            "    var bikeLayer = new google.maps.BicyclingLayer();\n"
            "    bikeLayer.setMap(map);\n");

        if (context->isCompareIntervals) {
            currentPage += QString(""
                // initialise local variables
                "    compareIntervalGroup = new Array();\n"

                "    drawCompareIntervals();\n"
                // catch signals to redraw intervals
                "    webBridge.drawCompareIntervals.connect(drawCompareIntervals);\n");
        } else {
            currentPage += QString(""
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
                "    google.maps.event.addListener(map, 'mouseup', function(event) { map.setOptions({draggable: true, zoomControl: true, scrollwheel: true, disableDoubleClickZoom: false}); webBridge.mouseup(); });\n");
        }
        currentPage += QString(""
                               "}\n"
                               "</script>\n");
    }

    // the main page is rather trivial
    currentPage += QString("</body>\n"
    "</html>\n");
}


bool
RideMapWindow::getCompareBoundingBox
(double &minLat, double &maxLat, double &minLon, double &maxLon) const
{
    // values outside -360..360
    minLat = minLon = 1000;
    maxLat = maxLon = -1000;
    if (! context->isCompareIntervals) {
        return false;
    }
    bool hasPositions = false;
    for (const CompareInterval &ci : context->compareIntervals) {
        if (ci.isChecked()) {
            for (RideFilePoint const * const &rfp : ci.data->dataPoints()) {
                if (rfp->lat || rfp->lon) {
                    minLat = std::min(minLat, rfp->lat);
                    maxLat = std::max(maxLat, rfp->lat);
                    minLon = std::min(minLon, rfp->lon);
                    maxLon = std::max(maxLon, rfp->lon);
                    hasPositions = true;
                }
            }
        }
    }
    return hasPositions;
}


void
RideMapWindow::buildPositionList
()
{
    double lastLat = 1000;
    double lastLon = 1000;
    long lastSecs = -5;
    bool first = true;
    positionItems.clear();
    foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints()) {
        long secs = rfp->secs;
        if (first) {
            secs = 0;
            first = false;
        }
        if (secs % 5 == 0 && secs - lastSecs == 5) {
            lastLat = rfp->lat;
            lastLon = rfp->lon;
            positionItems.append(PositionItem(lastLat, lastLon));
            lastSecs = secs;
        } else if (secs - lastSecs > 5) {
            // Add dummy points with last known position if not moving
            while (lastSecs < secs) {
                lastSecs += 5;
                positionItems.append(PositionItem(lastLat, lastLon));
            }
        }
    }
}


QColor RideMapWindow::GetColor(int watts)
{
    if (range < 0 || hideShadedZones()) return GColor(MAPROUTELINE);
    else return zoneColor(context->athlete->zones(myRideItem ? myRideItem->sport : "Bike")->whichZone(range, watts), 7);
}

// create the ride line
void
RideMapWindow::drawShadedRoute()
{
    if (context->isCompareIntervals) {
        return;
    }
    int intervalTime = 60;  // 60 seconds
    double rtime=0; // running total for accumulated data
    int count=0;  // how many samples ?
    int rwatts=0; // running total of watts
    double prevtime=0; // time for previous point
    double endRideItemtime = myRideItem->ride()->dataPoints().last()->secs;

    QString code;

    foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints()) {
        if (mapCombo->currentIndex() == OSM) {
            // Get GPS coordinates.
            if (count == 0) {
                // Start of segment.
                code = QString("{\n"
                               "var latLons = [");
            }

            if (rfp->lat || rfp->lon)
                code += QString("[%1, %2], ").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING);
        } else if (mapCombo->currentIndex() == GOOGLE) {
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
        }

        // running total of time
        rtime += rfp->secs - prevtime;
        rwatts += rfp->watts;
        prevtime = rfp->secs;
        count++;

        // end of segment or the segment is truncated by finding the last data point
        if ((rtime >= intervalTime) || (rfp->secs >= endRideItemtime)) {
            if (mapCombo->currentIndex() == OSM) {
                // Finalize variable "latLons" for the segment.
                if (code.endsWith(", "))
                    code.resize(code.length() - 2);
                code += QString("];\n");
            }

            int avgWatts = rwatts / count;
            QColor color = GetColor(avgWatts);
            // thats this segment done, so finish off and
            // add tooltip junk
            count = rwatts = rtime = 0;

            if (mapCombo->currentIndex() == OSM) {
                // Create and color a polyline for the segment.
                code += QString("var polyOptions = {\n"
                                "    stroke: true,\n"
                                "    color: '%1',\n"
                                "    weight: 3,\n"
                                "    opacity: %2,\n" // for out and backs, we need both
                                "    zIndex: 0\n"
                                "};\n"
                                "var polyline = new L.Polyline(latLons, polyOptions).addTo(map);\n"
                                "polyline.on('mousedown', function(event) { map.dragging.disable();L.DomEvent.stopPropagation(event);webBridge.clickPath(event.latlng.lat, event.latlng.lng); });\n" // map.setOptions({draggable: false, zoomControl: false, scrollwheel: false, disableDoubleClickZoom: true});
                                "polyline.on('mouseup',   function(event) { map.dragging.enable();L.DomEvent.stopPropagation(event);webBridge.mouseup(); });\n" // setOptions ?
                                "polyline.on('mouseover', function(event) { webBridge.hoverPath(event.latlng.lat, event.latlng.lng); });\n"
                                "path = polyline.getLatLngs();\n"
                                "}\n").arg(color.name())
                                      .arg(hideRouteLineOpacity() ? 1.0 : 0.5f);
            } else if (mapCombo->currentIndex() == GOOGLE) {
                // color the polyline
                code += QString("var polyOptions = {\n"
                                "    strokeColor: '%1',\n"
                                "    strokeWeight: 3,\n"
                                "    strokeOpacity: %2,\n" // for out and backs, we need both
                                "    zIndex: 0,\n"
                                "}\n"
                                "polyline.setOptions(polyOptions);\n"
                                "}\n").arg(color.name())
                                      .arg(hideRouteLineOpacity() ? 1.0 : 0.5f);

            }
            view->page()->runJavaScript(code);
        }
    }

}

void
RideMapWindow::clearTempInterval() {
    if (context->isCompareIntervals) {
        return;
    }
    QString code;
    if (mapCombo->currentIndex() == OSM) {
        code = QString( "{ \n"
                            "    if (tmpIntervalHighlighter)\n"
                            "       tmpIntervalHighlighter.setLatLngs([]);\n"
                            "}\n" );
    } else if (mapCombo->currentIndex() == GOOGLE) {
        code = QString( "{ \n"
                            "    tmpIntervalHighlighter.getPath().clear();\n"
                            "}\n" );
    }

    view->page()->runJavaScript(code);
}


void
RideMapWindow::compareIntervalsStateChanged
(bool state)
{
    countCompareIntervals = context->compareIntervals.size();
    smallPlot->setVisible(! state && showFullPlotCk->checkState() != Qt::Unchecked);
    overlayWidget->setVisible(! state && showInt->checkState() != Qt::Unchecked);

    forceReplot();
}


void
RideMapWindow::compareIntervalsChanged
()
{
    if (! context->isCompareIntervals) {
        return;
    }
    int oldCountCompareIntervals = countCompareIntervals;
    countCompareIntervals = 0;
    for (const CompareInterval &ci : context->compareIntervals) {
        if (ci.isChecked()) {
            for (RideFilePoint const * const &rfp : ci.data->dataPoints()) {
                if (rfp->lat || rfp->lon) {
                    ++countCompareIntervals;
                    break;
                }
            }
        }
    }

    if (oldCountCompareIntervals == 0 || countCompareIntervals == 0) {
        createHtml();
        view->page()->setHtml(currentPage);
    }
}

void
RideMapWindow::drawTempInterval(IntervalItem *current) {
    if (context->isCompareIntervals) {
        return;
    }
    QString code;

    if (mapCombo->currentIndex() == OSM) {
        code = QString( "{ \n"
                    // interval will be drawn with these options
                    "    var polyOptions = {\n"
                    "        stroke: true,\n"
                    "        color: '#00FFFF',\n"
                    "        opacity: 0.6,\n"
                    "        weight: 10,\n"
                    "        zIndex: -1\n"  // put at the bottom
                    "    };\n"

                    "    if (!tmpIntervalHighlighter) {\n"
                    "       tmpIntervalHighlighter = new L.Polyline([], polyOptions);\n"
                    "       tmpIntervalHighlighter.addTo(map);\n"
                    "       tmpIntervalHighlighter.on('mouseup',   function(event) { map.dragging.enable();L.DomEvent.stopPropagation(event); webBridge.mouseup(); });\n" // map.setOptions({draggable: true, zoomControl: true, scrollwheel: true, disableDoubleClickZoom: false});
                    "    } \n"
                    "    tmpIntervalHighlighter.setLatLngs([]);\n"
                    "    var latLons = ["
                    "\n");
    } else if (mapCombo->currentIndex() == GOOGLE) {
        code = QString( "{ \n"
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
    }

    foreach(RideFilePoint *rfp, myRideItem->ride()->dataPoints()) {
        if (rfp->secs+myRideItem->ride()->recIntSecs() > current->start
            && rfp->secs< current->stop) {

            if (rfp->lat || rfp->lon) {
                if (mapCombo->currentIndex() == OSM) {
                    code += QString("[%1, %2], ").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING);
                }
            } else if (mapCombo->currentIndex() == GOOGLE) {
                code += QString("    path.push(new google.maps.LatLng(%1,%2));\n").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING);
            }
        }
    }

    if (mapCombo->currentIndex() == OSM) {
        // Finalize variable "latLons" for the segment.
        if (code.endsWith(", "))
            code.resize(code.length() - 2);
        code += QString("];\n"
                        "path = latLons\n"  // Set the path to the current interval.
                        "}\n" );
    } else if (mapCombo->currentIndex() == GOOGLE) {
        code += QString("}\n" );
    }
    view->page()->runJavaScript(code);
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
RideMapWindow::createMarkers()
{
    if (!showMarkersCk->checkState() || context->isCompareIntervals)
        return;
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
        if (mapCombo->currentIndex() == OSM) {
            code = QString("{ var latlng = new L.LatLng(%1,%2);"
                           "var image = new L.icon({iconUrl:'qrc:images/maps/loop.png',iconAnchor:[16,37]});"
                           "var marker = new L.marker(latlng, { icon: image });"
                           "marker.addTo(map); }").arg(points[0]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[0]->lon,0,'g',GPS_COORD_TO_STRING);
        } else if (mapCombo->currentIndex() == GOOGLE) {
            code = QString("{ var latlng = new google.maps.LatLng(%1,%2);"
                           "var image = new google.maps.MarkerImage('qrc:images/maps/loop.png');"
                           "var marker = new google.maps.Marker({ icon: image, animation: google.maps.Animation.DROP, position: latlng });"
                           "marker.setMap(map); }").arg(points[0]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[0]->lon,0,'g',GPS_COORD_TO_STRING);
        }
        view->page()->runJavaScript(code);
    } else {
        // start / finish markers
        QString marker = "qrc:images/maps/cycling.png";
        if (myRideItem->isRun)
            marker = "qrc:images/maps/running.png";

        if (mapCombo->currentIndex() == OSM) {
            code = QString("{ var latlng = new L.LatLng(%1,%2);"
                           "var image = new L.icon({iconUrl:'%3',iconAnchor:[16,37]});"
                           "var marker = new L.marker(latlng, { icon: image });"
                           "marker.addTo(map); }").arg(points[0]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[0]->lon,0,'g',GPS_COORD_TO_STRING).arg(marker);
        } else if (mapCombo->currentIndex() == GOOGLE) {
            code = QString("{ var latlng = new google.maps.LatLng(%1,%2);"
                       "var image = new google.maps.MarkerImage('%3');"
                       "var marker = new google.maps.Marker({ icon: image, animation: google.maps.Animation.DROP, position: latlng });"
                       "marker.setMap(map); }").arg(points[0]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[0]->lon,0,'g',GPS_COORD_TO_STRING).arg(marker);
        }
        view->page()->runJavaScript(code);
        if (mapCombo->currentIndex() == OSM) {
            code = QString("{ var latlng = new L.LatLng(%1,%2);"
                           "var image = new L.icon({iconUrl:'qrc:images/maps/finish.png',iconAnchor:[16,37]});"
                           "var marker = new L.marker(latlng, { icon: image });"
                           "marker.addTo(map); }").arg(points[points.count()-1]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[points.count()-1]->lon,0,'g',GPS_COORD_TO_STRING);
        } else if (mapCombo->currentIndex() == GOOGLE) {
            code = QString("{ var latlng = new google.maps.LatLng(%1,%2);"
                       "var image = new google.maps.MarkerImage('qrc:images/maps/finish.png');"
                       "var marker = new google.maps.Marker({ icon: image, animation: google.maps.Animation.DROP, position: latlng });"
                       "marker.setMap(map); }").arg(points[points.count()-1]->lat,0,'g',GPS_COORD_TO_STRING).arg(points[points.count()-1]->lon,0,'g',GPS_COORD_TO_STRING);
        }
        view->page()->runJavaScript(code);
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

        QString marker = "qrc:images/maps/cycling_feed.png";
        if (myRideItem->isRun)
            marker = "qrc:images/maps/running_feed.png";

        if (stoptime > BEERANDBURRITO) { // 3 minutes is more than a traffic light stop dude.

            if ((!lastlat && !lastlon) || distanceBetween(lastlat, lastlon, stoplat, stoplon)>100) {
                lastlat = stoplat;
                lastlon = stoplon;

                if (mapCombo->currentIndex() == OSM) {
                    code = QString("{ var latlng = new L.LatLng(%1,%2);"
                                   "var image = new L.icon({iconUrl:'%3',iconAnchor:[16,37]});"
                                   "var marker = new L.marker(latlng, { icon: image });"
                                   "marker.addTo(map); }").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING).arg(marker);
                } else if (mapCombo->currentIndex() == GOOGLE) {
                    code = QString(
                        "{ var latlng = new google.maps.LatLng(%1,%2);"
                        "var image = new google.maps.MarkerImage('%3');"
                        "var marker = new google.maps.Marker({ icon: image, animation: google.maps.Animation.DROP, position: latlng });"
                        "marker.setMap(map);"
                    "}").arg(rfp->lat,0,'g',GPS_COORD_TO_STRING).arg(rfp->lon,0,'g',GPS_COORD_TO_STRING).arg(marker);
                }
                view->page()->runJavaScript(code);
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

        if (mapCombo->currentIndex() == OSM) {
            QString wPopupText = tr("Interval");
            code = QString("{ var latlng = new L.LatLng(%1,%2);"
                           "var marker = new L.marker(latlng);"
                           "marker.bindTooltip('%3: %4').openTooltip();"
                           "marker.on('click', function(event) { webBridge.toggleInterval(%5); });"
                           "marker.on('mouseover', function(event) { webBridge.hoverInterval(%5); });"
                           "marker.addTo(map);"
                           "markerList.push(marker);"
                           "}").arg(myRideItem->ride()->dataPoints()[offset]->lat,0,'g',GPS_COORD_TO_STRING)
                           .arg(myRideItem->ride()->dataPoints()[offset]->lon,0,'g',GPS_COORD_TO_STRING)
                           .arg(wPopupText)
                           .arg(x->name)
                           .arg(interval);
        } else if (mapCombo->currentIndex() == GOOGLE) {
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
        }
        view->page()->runJavaScript(code);
        interval++;
    }

    return;
}

void RideMapWindow::zoomInterval(IntervalItem *which)
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
    QString code;

    if (mapCombo->currentIndex() == GOOGLE) {
        code= QString("{ var southwest = new google.maps.LatLng(%1, %2);\n"
                               "var northeast = new google.maps.LatLng(%3, %4);\n"
                               "var bounds = new google.maps.LatLngBounds(southwest, northeast);\n"
                               "map.fitBounds(bounds);\n }")
                        .arg(minLat,0,'g',GPS_COORD_TO_STRING)
                        .arg(minLon,0,'g',GPS_COORD_TO_STRING)
                        .arg(maxLat,0,'g',GPS_COORD_TO_STRING)
                        .arg(maxLon,0,'g',GPS_COORD_TO_STRING);
    }

    view->page()->runJavaScript(code);
}

// quick diag, used to debug code only
void MapWebBridge::call(int count)
{
    Q_UNUSED(count);
    qDebug()<<"webBridge call:"<<count;
}

// how many intervals are highlighted?
int
MapWebBridge::intervalCount()
{
    RideItem *rideItem = mw->property("ride").value<RideItem*>();
    if (rideItem) return rideItem->intervalsSelected().count();
    return 0;
}

// get a latlon array for the i'th selected interval
QVariantList
MapWebBridge::getLatLons(int i)
{
    QVariantList latlons;
    RideItem *rideItem = mw->property("ride").value<RideItem*>();

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

    } else if (rideItem) {

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
MapWebBridge::drawOverlays()
{
    if (context->isCompareIntervals) {
        return;
    }

    // overlay the markers
    mw->createMarkers();

    // overlay a shaded route
    mw->drawShadedRoute();

    // Get the latest new selection lap number.
    RideItem *rideItem = mw->property("ride").value<RideItem*>();
    if (rideItem)
    {
        QList<QString> wIntervalNames;
        for(auto wInterval : rideItem->intervals())
        {
            wIntervalNames.append(wInterval->name);
        }

        // Get all intervals with name containing "Selection #".
        QVector<int> wNameSelectionIndexList;
        for(auto &wNameItr : wIntervalNames)
        {
            // Extract the number after the specified string.
            QString wNameEx = tr("Selection #");
            if (wNameItr.contains(wNameEx, Qt::CaseSensitive))
            {
                int wSelCount = wNameItr.count() - wNameEx.count();
                wNameSelectionIndexList.append(wNameItr.right(wSelCount).toInt());
            }
        }

        // Get the highest value and set the next selection number.
        auto maxElem = std::max_element(wNameSelectionIndexList.constBegin(), wNameSelectionIndexList.constEnd());
        if (maxElem != wNameSelectionIndexList.constEnd()) {
            selection = *maxElem + 1;
        } else {
            selection = 1;
        }
    }
    else
        selection = 1;
}

// interval marker was clicked on the map, toggle its display
void
MapWebBridge::toggleInterval(int x)
{
    RideItem *rideItem = mw->property("ride").value<RideItem*>();
    if (x < 0 || rideItem->intervals().count() <= x) return;

    IntervalItem *current = rideItem->intervals().at(x);
    if (current) {
        current->selected = !current->selected;
        context->notifyIntervalItemSelectionChanged(current);
    }
}

void
MapWebBridge::hoverInterval(int n)
{
    RideItem *rideItem = mw->property("ride").value<RideItem*>();
    if (rideItem && rideItem->ride() && rideItem->intervals().count() > n) {
        context->notifyIntervalHover(rideItem->intervals().at(n));
    }
}

void
MapWebBridge::clearHover()
{
}

RideFilePoint const *
MapWebBridge::searchPoint(double lat, double lng) const
{
    RideItem *rideItem = mw->property("ride").value<RideItem*>();

    RideFilePoint *candidate = nullptr;
    double candidateDist = std::numeric_limits<double>::max();
    foreach (RideFilePoint *p1, rideItem->ride()->dataPoints()) {
        if (p1->lat == 0 && p1->lon == 0)
            continue;

        double deltaLat = fabs(p1->lat - lat);
        double deltaLon = fabs(p1->lon - lng);
        if (deltaLat < 0.001 && deltaLon < 0.001) {
            // exact distance if of no interest, a rough approximation is sufficient
            double x = deltaLon * cos(lat);
            double approxDist = sqrt(x * x + deltaLat * deltaLat);
            if (approxDist < candidateDist) {
                candidate = p1;
                candidateDist = approxDist;
            }
        }
    }

    return candidate;
}

void
MapWebBridge::hoverPath(double lat, double lng)
{
    if (point) {
        RideItem *rideItem = mw->property("ride").value<RideItem*>();
        QString name = QString(tr("Selection #%1 ")).arg(selection);

        // Check if we are starting to drag a new selection to create a lap.
        if (m_startDrag && !m_drag)
        {
            m_startDrag = false;

            // Create interval.
            IntervalItem *add = rideItem->newInterval(name, point->secs, point->secs, 0, 0, Qt::black, false);
            add->selected = true;

            // rebuild list in sidebar
            context->notifyIntervalsUpdate(rideItem);

            m_drag = true;
        }

        if (rideItem->intervals(RideFileInterval::USER).count()) {

            IntervalItem *last = rideItem->intervals(RideFileInterval::USER).last();

            if (last->name.startsWith(name) && last->rideInterval) {
                RideFilePoint const *secondPoint = searchPoint(lat, lng);

                if (secondPoint != nullptr) {
                    if (secondPoint->secs > point->secs) {
                        if (last->stop == secondPoint->secs) {
                            return;
                        }
                        last->rideInterval->start = last->start = point->secs;
                        last->rideInterval->stop = last->stop = secondPoint->secs;
                    } else {
                        if (last->start == secondPoint->secs) {
                            return;
                        }
                        last->rideInterval->start = last->start = secondPoint->secs;
                        last->rideInterval->stop = last->stop = point->secs;
                    }
                    last->startKM = last->rideItem()->ride()->timeToDistance(last->start);
                    last->stopKM = last->rideItem()->ride()->timeToDistance(last->stop);

                    // update metrics
                    last->refresh();

                    // mark dirty
                    last->rideItem()->setDirty(true);

                    // overlay a shaded route
                    mw->drawTempInterval(last);

                    // update charts etc
                    context->notifyIntervalsChanged();
                 } else
                    qDebug() << "MapWebBridge::hoverPath(.): no point";
            }
        }

        // add average power to the end of the selection name
        //name += QString("(%1 watts)").arg(round((wattsTotal && arrayLength) ? wattsTotal/arrayLength : 0));


        // now update the RideFileIntervals and all the plots etc
        //context->athlete->updateRideFileIntervals();
    }
}

void
MapWebBridge::clickPath(double lat, double lng)
{
    if ((point = searchPoint(lat, lng)) != nullptr) {
        m_startDrag = true;
    } else {
        qDebug() << "MapWebBridge::clickPath(.): No Point near click";
        point = nullptr;
    }
}

void
MapWebBridge::mouseup()
{
    // clear the temorary highlighter
    if (point) {
        mw->clearTempInterval();
        point = nullptr;
        if (m_drag) {
            selection++;
            m_drag = false;
        }
        m_startDrag = false;
    }
}


QVariantList
MapWebBridge::setCompareIntervals
()
{
    QVariantList intervals;
    for (const CompareInterval &ci : context->compareIntervals) {
        if (ci.isChecked()) {
            QVariantMap interval;
            interval["color"] = ci.color.name();
            QVariantList lats;
            QVariantList lons;
            for (RideFilePoint const * const &rfp : ci.data->dataPoints()) {
                if (rfp->lat || rfp->lon) {
                    lats << rfp->lat;
                    lons << rfp->lon;
                }
            }
            interval["lons"] = lons;
            interval["lats"] = lats;
            intervals << interval;
        }
    }
    return intervals;
}


bool
RideMapWindow::event(QEvent *event)
{
    // nasty nasty nasty hack to move widgets as soon as the widget geometry
    // is set properly by the layout system, by default the width is 100 and
    // we wait for it to be set properly then put our helper widget on the RHS
    if (event->type() == QEvent::Resize && geometry().width() != 100) {

        // put somewhere nice on first show
        if (firstShow) {
            firstShow = false;
            helperWidget()->move(mainWidget()->geometry().width()-(275*dpiXFactor), 50*dpiYFactor);
            if (mapCombo->currentIndex() == OSM) {
                // Leaflet on OSM does not set the bounds correctly when its widget / canvas was never visible
                // Leaflets map.whenReady() does not help resolving this
                // Therefore forcing a replot (Google Maps does not have this issue)
                // This helps to display the correct zoomlevel after perspective change
                forceReplot();
            }
        }

        // if off the screen move on screen
        if (helperWidget()->geometry().x() > geometry().width() || helperWidget()->geometry().x() < geometry().x()) {
            helperWidget()->move(mainWidget()->geometry().width()-(275*dpiXFactor), 50*dpiYFactor);
        }
    }
    return QWidget::event(event);
}
