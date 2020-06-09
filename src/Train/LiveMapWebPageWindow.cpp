/*
 * Copyright (c) 2020 Peter Kanatselis (pkanatselis@gmail.com) 
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
#include "LiveMapWebPageWindow.h"

#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
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
#include "Library.h"
#include "ErgFile.h"

#include <QtWebChannel>
#include <QWebEngineProfile>

// overlay helper
#include "TabView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
//#include <QDebug>

// declared in main, we only want to use it to get QStyle
extern QApplication *application;

LiveMapWebPageWindow::LiveMapWebPageWindow(Context *context) : GcChartWindow(context), context(context)
{
     // Conent signal to recieve updates and latlon for ploting on map.
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(pause()), this, SLOT(pause()));
     
    //reveal controls widget
    // layout reveal controls
    QHBoxLayout *revealLayout = new QHBoxLayout;
    revealLayout->setContentsMargins(0,0,0,0);

    //
    // Chart settings
    //

    QWidget *settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0,0,0,0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    setControls(settingsWidget);
    setContentsMargins(0,0,0,0);

    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);

    setChartLayout(layout);

    // set view
    view = new QWebEngineView(this);
    view->setPage(new LiveMapsimpleWebPage());
    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,10,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

   configChanged(CONFIG_APPEARANCE);


   // Create route latlons


    //Show marker on map
    markerIsVisible = false;
    createHtml();
    view->page()->setHtml(currentPage);
}

LiveMapWebPageWindow::~LiveMapWebPageWindow()
{
}

void LiveMapWebPageWindow::start()
{
    routeLatLngs = "[";
    QString code = "";

    for (int pt = 0; pt < context->currentErgFile()->Points.size()-1; pt++) {
        if (pt == 0) {routeLatLngs += "["; }
        else { routeLatLngs += ",["; }
        routeLatLngs += QVariant(context->currentErgFile()->Points[pt].lat).toString();
        routeLatLngs += ",";
        routeLatLngs += QVariant(context->currentErgFile()->Points[pt].lon).toString();
        routeLatLngs += "]";
    }
    routeLatLngs += "]";
    code = QString("showRoute (" + routeLatLngs + ");");
    view->page()->runJavaScript(code);
}

// Reset map to World View when the activity is stopped.
void LiveMapWebPageWindow::stop()
{
    // Reset to world map
    markerIsVisible = false;
    createHtml();
    view->page()->setHtml(currentPage);
}

void LiveMapWebPageWindow::pause()
{
}

void LiveMapWebPageWindow::configChanged(qint32)
{

    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);

}

// Update position on the map when telemetry changes.
void LiveMapWebPageWindow::telemetryUpdate(RealtimeData rtd)
{
    QString code = "";

    if (rtd.getLatitude() != 0 && rtd.getLongitude() != 0)
    {
        QString sLat = QVariant(rtd.getLatitude()).toString();
        QString sLon = QVariant(rtd.getLongitude()).toString();
        QString sZoom = "16";
        code = "";
        if (!markerIsVisible) 
        {
            code = QString("centerMap (" + sLat + ", " + sLon + ", " + sZoom + ");");
            code += QString("showMyMarker (" + sLat + ", " + sLon + ");");
            markerIsVisible = true;
        }
        else
        {
            code += QString("moveMarker (" + sLat + ", " + sLon + ");");
        }
        view->page()->runJavaScript(code);
    }
}
// Build HTML code with all the javascript functions to be called later
// to update the postion on the mapp
void LiveMapWebPageWindow::createHtml()
{
    currentPage = "";

    currentPage = QString("<html><head>\n"
        "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=yes\"/> \n"
        "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n"
        "<title>GoldenCheetah LiveMap - TrainView</title>\n"
        "<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.6.0/dist/leaflet.css\"\n"
        "integrity=\"sha512-xwE/Az9zrjBIphAcBb3F6JVqxf46+CDLwfLMHloNu6KEQCAWi6HcDUbeOfBIptF7tcCzusKFjFw2yuvEpDL9wQ==\" crossorigin=\"\"/>\n"
        "<script src=\"https://unpkg.com/leaflet@1.6.0/dist/leaflet.js\"\n"
        "integrity=\"sha512-gZwIG9x3wUXg2hdXF6+rVkLF/0Vi9U8D2Ntg4Ga5I5BZpVkVxlJWbSQtXPSiUTtC0TjtGOmxa1AJPuV0CPthew==\" crossorigin=\"\"></script>\n"
        "<style>#mapid {height:100%;width:100%}</style></head>\n"
        "<body><div id=\"mapid\"></div>\n"
        "<script type=\"text/javascript\">\n"
        "var mapOptions, mymap, mylayer, mymarker, latlng, myscale, routepolyline\n"
        "function moveMarker(myLat, myLon) {\n"
        "    mymap.panTo(new L.LatLng(myLat, myLon));\n"
        "    mymarker.setLatLng(new L.latLng(myLat, myLon));\n"
        "}\n"
        "function initMap(myLat, myLon, myZoom) {\n"
        "    mapOptions = {\n"
        "    center: [myLat, myLon],\n"
        "    zoom : myZoom,\n"
        "    zoomControl : true,\n"
        "    scrollWheelZoom : false,\n"
        "    dragging : false,\n"
        "    doubleClickZoom : false }\n"
        "    mymap = L.map('mapid', mapOptions);\n"
        "    myscale = L.control.scale().addTo(mymap);\n"
        "    mylayer = new L.tileLayer('http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png');\n"
        "    mymap.addLayer(mylayer);\n"
        "}\n"
        "function showMyMarker(myLat, myLon) {\n"
        "    mymarker = new L.marker([myLat, myLon], {\n"
        "    draggable: false,\n"
        "    title : \"GoldenCheetah - Workout LiveMap\",\n"
        "    alt : \"GoldenCheetah - Workout LiveMap\",\n"
        "    riseOnHover : true\n"
        "        }).addTo(mymap);\n"
        "}\n"
        "function centerMap(myLat, myLon, myZoom) {\n"
        "    latlng = L.latLng(myLat, myLon);\n"
        "    mymap.setView(latlng, myZoom)\n"
        "}\n"
        "function showRoute(myRouteLatlngs) {\n"
        "    routepolyline = L.polyline(myRouteLatlngs, { color: 'red' }).addTo(mymap);\n"
        "    mymap.fitBounds(routepolyline.getBounds());\n"
        "}\n"
        "</script>\n"
        "<div><script type=\"text/javascript\">initMap (0, 0, 0);</script></div>\n"
        "</body></html>\n"
     );
}
