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
#include "LocationInterpolation.h"

//#include <QtWebChannel>
//#include <QWebEngineProfile>

// overlay helper
#include "TabView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"

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

    rButton = new QPushButton(application->style()->standardIcon(QStyle::SP_ArrowRight), "", this);
    rCustomUrl = new QLineEdit(this);
    revealLayout->addStretch();
    revealLayout->addWidget(rButton);
    revealLayout->addWidget(rCustomUrl);
    revealLayout->addStretch();
    setRevealLayout(revealLayout);

    //
    // Chart settings
    QWidget * settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0,0,0,0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    QFormLayout* commonLayout = new QFormLayout(settingsWidget);

    QString sValue = "";
    customUrlLabel = new QLabel(tr("OSM Base URL"));
    customUrl = new QLineEdit(this);
    customUrl->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_OSM_LM_BASEURL, "").toString();
    if (sValue.isEmpty()) customUrl->setText("http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png");
    else customUrl->setText(sValue);
    commonLayout->addRow(customUrlLabel, customUrl);

    customLatLabel = new QLabel(tr("Initial Lat"));
    customLat = new QLineEdit(this);
    customLat->setFixedWidth(100);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_OSM_LM_LAT, "").toString();
    if (sValue.isEmpty()) customLat->setText("43.827907");
    else customLat->setText(sValue);
    commonLayout->addRow(customLatLabel, customLat);

    customLonLabel = new QLabel(tr("Initial Lon"));
    customLon = new QLineEdit(this);
    customLon->setFixedWidth(100);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_OSM_LM_LON, "").toString();
    if (sValue.isEmpty()) customLon->setText("7.814973");
    else customLon->setText(sValue);
    commonLayout->addRow(customLonLabel, customLon);

    customZoomLabel = new QLabel(tr("Initial Zoom"));
    customZoom = new QLineEdit(this);
    customZoom->setFixedWidth(100);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_OSM_LM_ZOOM, "").toString();
    if (sValue.isEmpty()) customZoom->setText("16");
    else customZoom->setText(sValue);
    commonLayout->addRow(customZoomLabel, customZoom);

    applyButton = new QPushButton(application->style()->standardIcon(QStyle::SP_ArrowRight), "Apply changes", this);
    commonLayout->addRow(applyButton);

    // Connect signal to update map
    connect(applyButton, SIGNAL(clicked(bool)), this, SLOT(setMapOptions()));
    setControls(settingsWidget);
    setContentsMargins(0, 0, 0, 0);
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2, 0, 2, 2);
    setChartLayout(layout);

    //setChartLayout(layout);

    // set webview for map
    view = new QWebEngineView(this);
    webPage = view->page();
    view->setPage(webPage);

    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,10,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

   configChanged(CONFIG_APPEARANCE);

    markerIsVisible = false;
    createHtml(customUrl->text(), customLat->text(), customLon->text(), customZoom->text());
    view->page()->setHtml(currentPage);
}

LiveMapWebPageWindow::~LiveMapWebPageWindow()
{
}

//Set default map options from athlete
void LiveMapWebPageWindow::setMapOptions()
{
    appsettings->setCValue(context->athlete->cyclist, GC_OSM_LM_BASEURL, customUrl->text());
    appsettings->setCValue(context->athlete->cyclist, GC_OSM_LM_LAT, customLat->text());
    appsettings->setCValue(context->athlete->cyclist, GC_OSM_LM_LON, customLon->text());
    appsettings->setCValue(context->athlete->cyclist, GC_OSM_LM_ZOOM, customZoom->text());
    redrawMap();
}

void LiveMapWebPageWindow::start()
{
    routeLatLngs = "[";
    QString code = "";

    for (int pt = 0; pt < context->currentErgFile()->Points.size()-1; pt++) {

        geolocation geoloc(context->currentErgFile()->Points[pt].lat, context->currentErgFile()->Points[pt].lon, context->currentErgFile()->Points[pt].y);
        if (geoloc.IsReasonableGeoLocation()) {
            if (pt == 0) { routeLatLngs += "["; }
            else { routeLatLngs += ",["; }
            routeLatLngs += QVariant(context->currentErgFile()->Points[pt].lat).toString();
            routeLatLngs += ",";
            routeLatLngs += QVariant(context->currentErgFile()->Points[pt].lon).toString();
            routeLatLngs += "]";
        }

    }
    routeLatLngs += "]";
    code = QString("showRoute (" + routeLatLngs + ");");
    view->page()->runJavaScript(code);
}

// Reset map to preferred View when the activity is stopped.
void LiveMapWebPageWindow::stop()
{
    markerIsVisible = false;
    redrawMap();
}

void LiveMapWebPageWindow::pause()
{
}

void LiveMapWebPageWindow::redrawMap()
{
    createHtml(customUrl->text(), customLat->text(), customLon->text(), customZoom->text());
    view->page()->setHtml(currentPage);
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
    geolocation geoloc(rtd.getLatitude(), rtd.getLongitude(), rtd.getAltitude());
    if (geoloc.IsReasonableGeoLocation()) {
        QString sLat = QVariant(rtd.getLatitude()).toString();
        QString sLon = QVariant(rtd.getLongitude()).toString();
        code = "";
        if (!markerIsVisible) 
        {
            code = QString("centerMap (" + sLat + ", " + sLon + ", " + customZoom->text() + ");");
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
void LiveMapWebPageWindow::createHtml(QString sBaseUrl, QString sInitLat, QString sInitLon, QString sInitZoom)
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
        "    mylayer = new L.tileLayer('" + sBaseUrl +"');\n"
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
        //"    mymap.fitBounds(routepolyline.getBounds());\n"
        "}\n"
        "</script>\n"
        "<div><script type=\"text/javascript\">initMap (" + sInitLat + ", " + sInitLon +", " + sInitZoom + ");</script></div>\n"
        "</body></html>\n"
     );
}
