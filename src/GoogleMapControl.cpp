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

GoogleMapControl::GoogleMapControl(MainWindow *mw)
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
  if (parent->activeTab() != this)
      return;
  RideItem * ride = parent->rideItem();

  if (!ride)
      return;

  int zone =ride->zoneRange();
  if(zone < 0)
  {
      rideCP = 300;  // default cp to 300 watts
  }
  else
  {
      rideCP = ride->zones->getCP(zone);
  }

  rideData.clear();
  double prevLon = 0;
  double prevLat = 0;

  if (ride == NULL || ride->ride() == NULL) return;

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
    newRideToLoad = true;
    loadRide();
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


QColor GoogleMapControl::GetColor(int cp, int watts)
{
    double effort = watts/ (double) cp /1.25;

    if(effort < 0.25)
    {
        effort = 0;
    }
    if(effort > 1.0)
    {
        effort = 1;
    }
    QColor color;
    color.setHsv(int(360 - (360 * effort)), 255, 255);
    return color;
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
            QColor color = GetColor(rideCP,avgPower);
            // create the polyline
            CreateSubPolyLine(intervalPoints,oss,color);
            intervalPoints.clear();
            intervalPoints.push_back(rfp);
        }

    }
    return oss.str();
}


void GoogleMapControl::CreateSubPolyLine(const std::vector<RideFilePoint> &points,
                                         std::ostringstream &oss,
                                         QColor color)
{
    oss.precision(6);
    QString colorstr = color.name();
    oss.setf(ios::fixed,ios::floatfield);
    oss << "map.addOverlay (new GPolyline([";

    BOOST_FOREACH(RideFilePoint rfp, points)
    {
        if (ceil(rfp.lat) != 180 && ceil(rfp.lon) != 180)
        {
            oss << "new GLatLng(" << rfp.lat << "," << rfp.lon << ")," << endl;
        }
    }

    oss << "],\"" << colorstr.toStdString() << "\",5));";
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
