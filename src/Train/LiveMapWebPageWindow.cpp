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
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"
#include "Library.h"
#include "ErgFile.h"



// overlay helper
#include "AbstractView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include "HelpWhatsThis.h"

// declared in main, we only want to use it to get QStyle
extern QApplication *application;

LiveMapWebPageWindow::LiveMapWebPageWindow(Context *context) : GcChartWindow(context), context(context)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_LiveMap));

    // Connect signal to receive updates on lat/lon for ploting on map.
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));
    connect(context, SIGNAL(SimRiderStateUpdate(SimRiderStateData)), this, SLOT(SimRiderStateUpdate(SimRiderStateData)));

    // Chart settings
    QWidget * settingsWidget = new QWidget(this);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(settingsWidget);
    settingsWidget->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrain_LiveMap));
    settingsWidget->setContentsMargins(0,0,0,0);  // Margings for the settings widget on the right side
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));
    
    QFormLayout* commonLayout = new QFormLayout(settingsWidget);
    QString sValue = "";
    customUrlLabel = new QLabel(tr("OSM Base URL"));
    customUrl = new QLineEdit(this);
    customUrl->setFixedWidth(300);
    if (customUrl->text() == "") {
        customUrl->setText("http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png");
    }
    commonLayout->addRow(customUrlLabel, customUrl);

    applyButton = new QPushButton(application->style()->standardIcon(QStyle::SP_ArrowRight), tr("Apply changes"), this);
    commonLayout->addRow(applyButton);

    //Set widget as configuration settings
    setControls(settingsWidget);
    setContentsMargins(0, 0, 0, 0);
    
    //Create a VBox layout to show both widgets (settings and stats) when adding the chart
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2, 0, 2, 2);
    setChartLayout(layout);

    // Connect signal to update map
    connect(applyButton, SIGNAL(clicked(bool)), this, SLOT(userUrl()));
    connect(customUrl, SIGNAL(returnPressed()), this, SLOT(userUrl()));

    // New widget for the webview for map
    view = new QWebEngineView(this);
    // stop stealing focus!
    //view->settings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);
    webPage = view->page();
    view->setPage(webPage);
    view->setContentsMargins(10,10,10,10);
    view->page()->view()->setContentsMargins(10,10,10,10);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);

    layout->addWidget(view);
    configChanged(CONFIG_APPEARANCE);

    // Finish initialization using current settings
    userUrl();
    ergFileSelected(context->currentErgFile());
    
}

void LiveMapWebPageWindow::userUrl()
{
    // add http:// if scheme is missing
    QRegExp hasscheme("^[^:]*://.*");
    QString url = customUrl->text();
    if (!hasscheme.exactMatch(url)) url = "http://" + url;
    view->setZoomFactor(dpiXFactor);
    view->setUrl(QUrl(url));
}

LiveMapWebPageWindow::~LiveMapWebPageWindow()
{
  if (view) delete view->page();
}

void LiveMapWebPageWindow::ergFileSelected(ErgFile* f)
{
    // rename window to workout name and draw route if data exists
    if (f && f->filename != "" )
    {
        setIsBlank(false);
        // these values need extended precision or place marker jumps around.
        QString startingLat = QString::number(((f->Points)[0]).lat, 'g', 10);
        QString startingLon = QString::number(((f->Points)[0]).lon, 'g', 10);
        if (startingLat == "0" && startingLon == "0")
        {
            markerIsVisible = false;
            setIsBlank(true);
        }
        else
        {
            QString js = ("<div><script type=\"text/javascript\">initMap (" + startingLat + ", " + startingLon + ",13);</script></div>\n");
            routeLatLngs = "[";
            QString code = "";

            for (int pt = 0; pt < f->Points.size() - 1; pt++) {

                geolocation geoloc(f->Points[pt].lat, f->Points[pt].lon, f->Points[pt].y);
                if (geoloc.IsReasonableGeoLocation()) {
                    if (pt == 0) { routeLatLngs += "["; }
                    else { routeLatLngs += ",["; }
                    routeLatLngs += QVariant(f->Points[pt].lat).toString();
                    routeLatLngs += ",";
                    routeLatLngs += QVariant(f->Points[pt].lon).toString();
                    routeLatLngs += "]";
                }

            }
            routeLatLngs += "]";
            // We can either setHTML page or runJavaScript but not both.
            // So we create divs with the 2 methods we need to run when the document loads
            code = QString("showRoute (" + routeLatLngs + ");");
            js += ("<div><script type=\"text/javascript\">" + code + "</script></div>\n");
            //createHtml(customUrl->text(), js);
            //view->page()->setHtml(currentPage);
            view->page()->setHtml(createHtml(customUrl->text(), js));
        }
    }
    else
    {
        markerIsVisible = false;
        setIsBlank(true);
    }
}

void LiveMapWebPageWindow::drawRoute(ErgFile* f) {
    routeLatLngs = "[";
    QString code = "";

    for (int pt = 0; pt < f->Points.size() - 1; pt++) {

        geolocation geoloc(f->Points[pt].lat, f->Points[pt].lon, f->Points[pt].y);
        if (geoloc.IsReasonableGeoLocation()) {
            if (pt == 0) { routeLatLngs += "["; }
            else { routeLatLngs += ",["; }
            routeLatLngs += QVariant(f->Points[pt].lat).toString();
            routeLatLngs += ",";
            routeLatLngs += QVariant(f->Points[pt].lon).toString();
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
    userUrl();
    ergFileSelected(context->currentErgFile());
    for (int i = 0; i < 5; i++) markerVisable[i] = false;
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

void LiveMapWebPageWindow::SimRiderStateUpdate(SimRiderStateData srData) {
    localSrData = srData;
}

// Update position on the map when telemetry changes.
void LiveMapWebPageWindow::telemetryUpdate(RealtimeData rtd) {
    QString code2;
    if (rtd.getDistanceRemaining() > 0) { // Don't run code if workout is finished
        if (localSrData.srFeatureEnabled()) {  // If SimRider feature is enable show markers for all riders
            code2 = "";
            for (int i = 0; i < localSrData.srCount(); i++) {
                geolocation gl(localSrData.srLatitudeFor(i), localSrData.srLongitudeFor(i), localSrData.srAltitudeFor(i));
                if (gl.IsReasonableGeoLocation()) {
                    QString sLat = QVariant(localSrData.srLatitudeFor(i)).toString();
                    QString sLon = QVariant(localSrData.srLongitudeFor(i)).toString();
                    if (!markerVisable[i]) { // create and show markers
                        if (i == 0) {
                            code2 = QString("centerMap (" + sLat + ", " + sLon + ", " + "17" + ");");
                            code2 += QString("showMyMarker (" + sLat + ", " + sLon + ");");
                        }
                        else code2 += QString("showVPMarker" + QVariant(i).toString() + "(" +  sLat + ", " +  sLon + ");");
                        markerVisable[i] = true;
                    } else { // Move markers
                        if (i == 0) code2 += QString("moveMarker (" + sLat + ", " + sLon + ");");
                        else code2 += QString("moveVPMarker" + QVariant(i).toString() + "(" + sLat + ", " + sLon + ");");
                    }
                }
            }
            //qDebug() << "Code2: " << code2;
            view->page()->runJavaScript(code2);
        }
        else { // SimRider is disabled, just show one marker for the user
            QString code = "";
            geolocation geoloc(rtd.getLatitude(), rtd.getLongitude(), rtd.getAltitude());
            if (geoloc.IsReasonableGeoLocation()) {
                QString sLat = QVariant(rtd.getLatitude()).toString();
                QString sLon = QVariant(rtd.getLongitude()).toString();
                code = "";
                if (!markerIsVisible) {
                    code = QString("centerMap (" + sLat + ", " + sLon + ", " + "15" + ");");
                    code += QString("showMyMarker (" + sLat + ", " + sLon + ");");
                    markerIsVisible = true;
                }
                else {
                    code += QString("moveMarker (" + sLat + ", " + sLon + ");");
                }
                //qDebug() << "Code: " << code;
                view->page()->runJavaScript(code);
            }
        }

    }
}

// Build HTML code with all the javascript functions to be called later
// to update the postion on the mapp
QString LiveMapWebPageWindow::createHtml(QString sBaseUrl, QString autoRunJS)
{
    QString currentPage = "";

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
        "var mapOptions, mymap, mylayer, mymarker, latlng, myscale, routepolyline, layerGroup, vpIcon\n"
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
        "    createGroupLayer();\n"
        "    vpIcon = L.icon({\n"
        "    iconUrl: 'qrc:images/vp-marker-icon3.png',\n"
        "    iconAnchor : [12, 60] });\n"
         "}\n"
        "function showMyMarker(myLat, myLon) {\n"
        "    createMyMarker(myLat, myLon);\n"
        "    layerGroup.addLayer(mymarker);\n"
        "}\n"
        "function createMyMarker(myLat, myLon) {\n"
        "    mymarker = new L.marker([myLat, myLon], {\n"
        "    draggable: false,\n"
        "    title : \"Athlete\",\n"
        "    alt : \"Athlete\",\n"
        "    riseOnHover : true\n"
        "        })\n"
        "}\n"
        "function createGroupLayer() {\n"
        "    layerGroup = L.layerGroup([]);\n"
        "    layerGroup.addTo(mymap);\n"
        "}\n"
        "function showVPMarker1(myLat, myLon) {\n"
        "    createVPMarker1(myLat, myLon);\n"
        "    layerGroup.addLayer(vpmarker1);\n"
        "}\n"
        "function createVPMarker1(vpLat, vpLon) {\n"
        "    vpmarker1 = new L.marker([vpLat, vpLon], {\n"
        "    draggable: false,\n"
        "    title : \"\",\n"
        "    alt : \"VP\",\n"
        "    riseOnHover : true,\n"
        "    icon: vpIcon\n"
        "        })\n"
        "}\n"
        "function moveVPMarker1(myLat, myLon) {\n"
        "    vpmarker1.setLatLng(new L.latLng(myLat, myLon));\n"
        "}\n"


        "function showVPMarker2(myLat, myLon) {\n"
        "    createVPMarker2(myLat, myLon);\n"
        "    layerGroup.addLayer(vpmarker2);\n"
        "}\n"
        "function createVPMarker2(vpLat, vpLon) {\n"
        "    vpmarker2 = new L.marker([vpLat, vpLon], {\n"
        "    draggable: false,\n"
        "    title : \"\",\n"
        "    alt : \"VP\",\n"
        "    riseOnHover : true,\n"
        "    icon: vpIcon\n"
        "        })\n"
        "}\n"
        "function moveVPMarker2(myLat, myLon) {\n"
        "    vpmarker2.setLatLng(new L.latLng(myLat, myLon));\n"
        "}\n"

        "function showVPMarker3(myLat, myLon) {\n"
        "    createVPMarker3(myLat, myLon);\n"
        "    layerGroup.addLayer(vpmarker3);\n"
        "}\n"
        "function createVPMarker3(vpLat, vpLon) {\n"
        "    vpmarker3 = new L.marker([vpLat, vpLon], {\n"
        "    draggable: false,\n"
        "    title : \"\",\n"
        "    alt : \"VP\",\n"
        "    riseOnHover : true,\n"
        "    icon: vpIcon\n"
        "        })\n"
        "}\n"
        "function moveVPMarker3(myLat, myLon) {\n"
        "    vpmarker3.setLatLng(new L.latLng(myLat, myLon));\n"
        "}\n"

        "function showVPMarker4(myLat, myLon) {\n"
        "    createVPMarker4(myLat, myLon);\n"
        "    layerGroup.addLayer(vpmarker4);\n"
        "}\n"
        "function createVPMarker4(vpLat, vpLon) {\n"
        "    vpmarker4 = new L.marker([vpLat, vpLon], {\n"
        "    draggable: false,\n"
        "    title : \"\",\n"
        "    alt : \"VP\",\n"
        "    riseOnHover : true,\n"
        "    icon: vpIcon\n"
        "        })\n"
        "}\n"
        "function moveVPMarker4(myLat, myLon) {\n"
        "    vpmarker4.setLatLng(new L.latLng(myLat, myLon));\n"
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
        + autoRunJS +
        "</body></html>\n"
     );
     //qDebug() << "HTML code: " << currentPage;
     return currentPage;
}
