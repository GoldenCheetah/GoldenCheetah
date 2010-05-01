/*
 * Copyright (c) 2009 Greg Lonnon (greg.lonnon@gmail.com)
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
#include "RideItem.h"
#include "RideFile.h"
#include "MainWindow.h"
#include "Zones.h"
#include "Settings.h"
#include "Units.h"

#include <QDebug>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/circular_buffer.hpp>

using namespace std;

namespace gm
{
// quick ideas on a math pipeline, kindof like this...
// but more of a pipeline...
// it makes the math somewhat easier to do and understand...

    class RideFilePointAlgorithm
    {
    protected:
        RideFilePoint prevRfp;
        bool first;
        RideFilePointAlgorithm() { first = false; }
    };

    // Algorithm to find the meidan of a set of data
    template<typename T> class Median
    {
        boost::circular_buffer<T> buffer;
    public:
        Median(int size)
        {
            buffer.set_capacity(size);
        }
        // add the new data
        void add(T a) { buffer.push_back(a); }
        operator T()
        {
            if(buffer.size() == 0)
            {
                return 0;
            }

            T total = 0;
            BOOST_FOREACH(T point, buffer)
            {
                total += point;
            }
            return total / buffer.size();
        }
    };

    class AltGained : private RideFilePointAlgorithm
    {
    protected:
        double gained;
        double curAlt, prevAlt;
        Median<double> median;
    public:
        AltGained(): gained(0), curAlt(0), prevAlt(0), median(20) {}

        void operator()(RideFilePoint rfp)
        {
            median.add(rfp.alt);
            curAlt = median;
            if(prevAlt == 0)
            {
                prevAlt = median;
            }
            if(curAlt> prevAlt)
            {
                gained += curAlt - prevAlt;
            }
            prevAlt = curAlt;
        }
        int TotalGained() { return gained; }
        operator int() { return TotalGained(); }
    };

    class AvgHR
    {
        int samples;
        int totalHR;
    public:
        AvgHR() : samples(0),totalHR(0.0) {}
        void operator()(RideFilePoint rfp)
        {
            totalHR += rfp.hr;
            samples++;
        }
        int HR() { return totalHR / samples; }
        operator int() { return HR(); }
    };

    class AvgPower
    {
        int samples;
        double totalPower;
    public:
        AvgPower() : samples(0), totalPower(0.0) { }
        void operator()(RideFilePoint rfp)
        {
            totalPower += rfp.watts;
            samples++;
        }
        int Power() { return (int) (totalPower / samples); }
        operator int() { return Power(); }
    };
}

using namespace gm;

#define GOOGLE_KEY "ABQIAAAAS9Z2oFR8vUfLGYSzz40VwRQ69UCJw2HkJgivzGoninIyL8-QPBTtnR-6pM84ljHLEk3PDql0e2nJmg"

GoogleMapControl::GoogleMapControl(MainWindow *mw) : main(mw), range(-1), current(NULL)
{
    parent = mw;
    view = new QWebView();
    layout = new QVBoxLayout();
    layout->addWidget(view);
    setLayout(layout);
    connect(parent, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(view, SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    connect(view, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    loadingPage = false;
    newRideToLoad = false;
}

void
GoogleMapControl::rideSelected()
{
  if (parent->activeTab() != this) return;

  RideItem * ride = parent->rideItem();
  if (ride == current || !ride || !ride->ride())
      return;
  else
      current = ride;

  range =ride->zoneRange();
  if(range < 0)
  {
      rideCP = 300;  // default cp to 300 watts
  }
  else
  {
      rideCP = ride->zones->getCP(range);
  }

  rideData.clear();
  double prevLon = 0;
  double prevLat = 0;

  foreach(RideFilePoint *rfp,ride->ride()->dataPoints())
  {
      RideFilePoint curRfp = *rfp;

      if(ceil(rfp->lon) == 180 ||
         ceil(rfp->lat) == 180)
      {
          curRfp.lon = prevLon;
          curRfp.lat = prevLat;
      }
      // wko imports can have -320 type of values for roughly 40 degrees
      if((curRfp.lat) < -180)
      {
          curRfp.lat = 360 + curRfp.lat;
      }
      prevLon = curRfp.lon;
      prevLat = curRfp.lat;
      rideData.push_back(curRfp);
  }
  newRideToLoad = true;
  loadRide();
}

void GoogleMapControl::resizeEvent(QResizeEvent * )
{
    static bool first = true;
    if (parent->activeTab() != this) return;

    if (first == true) {
        first = false;
    } else {
        newRideToLoad = true;
        loadRide();
    }
}

void GoogleMapControl::loadStarted()
{
    loadingPage = true;
}

/// called after the load is finished
void GoogleMapControl::loadFinished(bool)
{
    loadingPage = false;
    loadRide();
}

void GoogleMapControl::loadRide()
{
    if(loadingPage == true)
    {
        return;
    }

    if(newRideToLoad == true)
    {
        createHtml();
        newRideToLoad = false;
        loadingPage = true;

        QString htmlFile(QDir::tempPath());
        htmlFile.append("/maps.html");
        QFile file(htmlFile);
        file.remove();
        file.open(QIODevice::ReadWrite);
        file.write(currentPage.str().c_str(),currentPage.str().length());
        file.flush();
        file.close();
        QString filename("file:///");
        filename.append(htmlFile);
        QUrl url(filename);
        view->load(url);
    }
}

void GoogleMapControl::createHtml()
{
    currentPage.str("");
    double minLat, minLon, maxLat, maxLon;
    minLat = minLon = 1000;
    maxLat = maxLon = -1000; // larger than 360

    BOOST_FOREACH(RideFilePoint rfp, rideData)
    {
        minLat = std::min(minLat,rfp.lat);
        maxLat = std::max(maxLat,rfp.lat);
        minLon = std::min(minLon,rfp.lon);
        maxLon = std::max(maxLon,rfp.lon);
    }
    if(floor(minLat) == 0)
    {
        currentPage << tr("No GPS Data Present").toStdString();
        return;
    }

    /// seems to be the magic number... to stop the scrollbars
    int width = view->width() -16;
    int height = view->height() -16;

    std::ostringstream oss;
    oss.precision(6);
    oss.setf(ios::fixed,ios::floatfield);

    oss << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"" << endl
        << "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl
        << "<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:v=\"urn:schemas-microsoft-com:vml\">" << endl
        << "<head>" << endl
        << "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"/>" << endl
        << "<title>GoldenCheetah</title>" << endl
        << "<script src=\"http://maps.google.com/maps?file=api&amp;v=2&amp;key=" << GOOGLE_KEY <<"\"" << endl
        << "type=\"text/javascript\"></script>" << endl
        << CreateMapToolTipJavaScript() << endl
        << "<script type=\"text/javascript\">"<< endl
        << "var map;" << endl
        << "function initialize() {" << endl
        << "if (GBrowserIsCompatible()) {" << endl
        << "map = new GMap2(document.getElementById(\"map_canvas\")," << endl
        << " {size: new GSize(" << width << "," << height << ") } );" << endl
        << "map.setUIToDefault()" << endl
        << CreatePolyLine() << endl
        << CreateIntervalMarkers() << endl
        << "var sw = new GLatLng(" << minLat << "," << minLon << ");" << endl
        << "var ne = new GLatLng(" << maxLat << "," << maxLon << ");" << endl
        << "var bounds = new GLatLngBounds(sw,ne);" << endl
        << "var zoomLevel = map.getBoundsZoomLevel(bounds);" << endl
        << "var center = bounds.getCenter(); " << endl
        << "map.setCenter(bounds.getCenter(),map.getBoundsZoomLevel(bounds),G_PHYSICAL_MAP);" << endl
        << "map.enableScrollWheelZoom();" << endl
        << "}" << endl
        << "}" << endl
        << "function animate() {"  << endl
        << "map.panTo(new GLatLng(" << maxLat << "," << minLon << "));" << endl
        << "}" << endl
        << "</script>" << endl
        << "</head>" << endl
        << "<body onload=\"initialize()\" onunload=\"GUnload()\">" << endl
        << "<div id=\"map_canvas\" style=\"width: " << width <<"px; height: "
        << height <<"px\"></div>" << endl
        << "<form action=\"#\" onsubmit=\"animate(); return false\">" << endl
        << "</form>" << endl
        << "</body>" << endl
        << "</html>" << endl;

    currentPage << oss.str();
}


QColor GoogleMapControl::GetColor(int watts)
{
    if (range < 0) return Qt::red;
    else return zoneColor(main->zones()->whichZone(range, watts), 7);
}


/// create the ride line
string GoogleMapControl::CreatePolyLine()
{
    std::vector<RideFilePoint> intervalPoints;
    ostringstream oss;
    int intervalTime = 30;  // 30 seconds

    BOOST_FOREACH(RideFilePoint rfp, rideData)
    {
        intervalPoints.push_back(rfp);
        if((intervalPoints.back().secs - intervalPoints.front().secs) >
           intervalTime)
        {
            // find the avg power and color code it and create a polyline...
            AvgPower avgPower = for_each(intervalPoints.begin(),
                                         intervalPoints.end(),
                                         AvgPower());
            // find the color

            // create the polyline
            CreateSubPolyLine(intervalPoints,oss,avgPower);
            intervalPoints.clear();
            intervalPoints.push_back(rfp);

        }

    }
    return oss.str();
}


void GoogleMapControl::CreateSubPolyLine(const std::vector<RideFilePoint> &points,
                                         std::ostringstream &oss,
                                         int avgPower)
{
    oss.precision(6);
    QColor color = GetColor(avgPower);
    QString colorstr = color.name();
    oss.setf(ios::fixed,ios::floatfield);
    oss << "var polyline = new GPolyline([";

    BOOST_FOREACH(RideFilePoint rfp, points)
    {
        if (ceil(rfp.lat) != 180 && ceil(rfp.lon) != 180)
        {
            oss << "new GLatLng(" << rfp.lat << "," << rfp.lon << ")," << endl;
        }
    }
    oss << "],\"" << colorstr.toStdString() << "\",4);" << endl;

    oss << "GEvent.addListener(polyline, 'mouseover', function() {" << endl
        << "var tooltip_text = '30s Power: " << avgPower << "';" << endl
        << "var ss={'weight':8};" << endl
        << "this.setStrokeStyle(ss);" << endl
        << "this.overlay = new MapTooltip(this,tooltip_text);" << endl
        << "map.addOverlay(this.overlay);" << endl
        << "});" << endl
        << "GEvent.addListener(polyline, 'mouseout', function() {" << endl
        << "map.removeOverlay(this.overlay);" << endl
        << "var ss={'weight':5};" << endl
        << "this.setStrokeStyle(ss);" << endl
        << "});" << endl;
    oss << "map.addOverlay (polyline);" << endl;
}


string GoogleMapControl::CreateIntervalMarkers()
{
    bool useMetricUnits = (GetApplicationSettings()->value(GC_UNIT).toString()
                           == "Metric");

    ostringstream oss;
    oss.precision(6);
    oss.setf(ios::fixed,ios::floatfield);

    oss << "var marker;" << endl;
    int currentInterval = 0;
    std::vector<RideFilePoint> intervalPoints;
    BOOST_FOREACH(RideFilePoint rfp, rideData)
    {
        intervalPoints.push_back(rfp);
        if(currentInterval < rfp.interval)
        {
            // want to see avg power, avg speed, alt changes, avg hr
            double distance = intervalPoints.back().km -
                               intervalPoints.front().km ;
            distance = (useMetricUnits ? distance : distance * MILES_PER_KM);

            int secs = intervalPoints.back().secs -
                intervalPoints.front().secs;

            double avgSpeed = (distance/((double)secs)) * 3600;

            QTime time;
            time = time.addSecs(secs);

            AvgHR avgHr = for_each(intervalPoints.begin(),
                                   intervalPoints.end(),
                                       AvgHR());
            AvgPower avgPower = for_each(intervalPoints.begin(),
                                         intervalPoints.end(),
                                         AvgPower());

            int altGained =ceil(for_each(intervalPoints.begin(),
                                       intervalPoints.end(),
                                            AltGained()));
            altGained = ceil((useMetricUnits ? altGained :
                              altGained * FEET_PER_METER));
            oss.precision(6);
            oss << "marker = new GMarker(new GLatLng( ";
            oss<< rfp.lat << "," << rfp.lon << "));" << endl;
            oss << "marker.bindInfoWindowHtml(" <<endl;
            oss << "\"<p><h3>Lap: " << currentInterval << "</h3></p>" ;
            oss.precision(1);
            oss << "<p>Distance: " << distance << " "
                << (useMetricUnits ? "KM" : "Miles") << "</p>" ;
            oss << "<p>Time: " << time.toString().toStdString() << "</p>";
            oss << "<p>Avg Speed</>: " << avgSpeed << " "
                << (useMetricUnits ? tr("KPH") : tr("MPH")).toStdString()
                << "</p>";
            if(avgHr != 0) {
                oss << "<p>Avg HR: " << avgHr << "</p>";
            }
            if(avgPower != 0)
            {
                oss << "<p>Avg Power: " << avgPower << " Watts</p>";
            }
            oss << "<p>Alt Gained: " << altGained << " "
                << (useMetricUnits ? tr("Meters") : tr("Feet")).toStdString()
                << "</p>";
            oss << "\");" << endl;
            oss << "map.addOverlay(marker);" << endl;
            currentInterval = rfp.interval;
            intervalPoints.clear();
        }
    }
    return oss.str();
}

string GoogleMapControl::CreateMapToolTipJavaScript()
{
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
