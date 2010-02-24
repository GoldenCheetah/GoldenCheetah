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

#include <QDebug>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/foreach.hpp>
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

	class AltGained : private RideFilePointAlgorithm
	{
	protected:
		RideFilePoint prevRfp;
		double gained;
	public:
		AltGained() { gained = 0; }

		void operator()(RideFilePoint rfp)
		{
			if(rfp.alt > prevRfp.alt)
			{
				gained += rfp.alt - prevRfp.alt;
			}
			prevRfp = rfp;
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

GoogleMapControl::GoogleMapControl(MainWindow *mw, QTabWidget *tw)
{
	parent = mw;
	tabWidget = tw;
	view = new QWebView();
	layout = new QVBoxLayout();
	layout->addWidget(view);
	setLayout(layout);
	connect(parent, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
	view->load(QUrl("http://google.com"));
	tabIndex = -1;
}

void
GoogleMapControl::rideSelected() {

  ride = parent->rideItem();

  if (!ride)
	  return;

  int currentTab = tabWidget->currentIndex();

  // Does the ride have GPS data
  if(ride->ride()->dataPoints().front()->lat == 0.0)
  {
	  if(tabIndex > -1)
	  {
		  tabWidget->removeTab(tabIndex);
          if (currentTab == tabIndex) tabWidget->setCurrentIndex(0);
	  }
	  tabIndex = -1;
  }
  else
  {
	  if (tabIndex == -1) tabIndex = tabWidget->addTab(this, tr("Map"));
      if (currentTab != tabIndex) tabWidget->setCurrentIndex(currentTab);
	  createHtml();
  }
}

void GoogleMapControl::resizeEvent(QResizeEvent * )
{
	// createHtml will handle resize the widget correct
	createHtml();
}

void GoogleMapControl::createHtml()
{
	if(ride == NULL) return;

	double minLat, minLon, maxLat, maxLon;
	minLat = minLon = 1000;
	maxLat = maxLon = -1000; // larger than 360

	foreach(RideFilePoint *rfp, ride->ride()->dataPoints())
	{
		minLat = std::min(minLat,rfp->lat);
		maxLat = std::max(maxLat,rfp->lat);
		minLon = std::min(minLon,rfp->lon);
		maxLon = std::max(maxLon,rfp->lon);
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
		<< CreatePolyLine(ride) << endl
		<< CreateIntervalMarkers(ride) << endl
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
		<< "<div id=\"map_canvas\" style=\"width: " << width <<"px; height: "<< height <<"px\"></div>" << endl
		<< "<form action=\"#\" onsubmit=\"animate(); return false\">" << endl
		<< "</form>" << endl
		<< "</body>" << endl
		<< "</html>" << endl;

#define DEBUG_GMAPS 1
#if DEBUG_GMAPS
	 QString htmlFile(QDir::tempPath());
	 htmlFile.append("/maps.html");
	 QFile file(htmlFile);
	 file.remove();
	 file.open(QIODevice::ReadWrite);
	 file.write(oss.str().c_str(),oss.str().length());
	 file.flush();
	 file.close();
	 QString filename("file:///");
	 filename.append(htmlFile);
	 QUrl url(filename);
	 view->load(url);
#endif
	 view->setHtml(QString(oss.str().c_str()));
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
string GoogleMapControl::CreatePolyLine(RideItem *ride)
{
	std::vector<RideFilePoint> intervalPoints;
	ostringstream oss;

	int cp;
	int intervalTime = 30;  // 30 seconds
	int zone =ride->zoneRange();
	if(zone >= 0)
	{
		cp = 300;  // default cp to 300 watts
	}
	else
	{
		cp = ride->zones->getCP(zone);
	}

	foreach(RideFilePoint* rfp, ride->ride()->dataPoints())
	{
		intervalPoints.push_back(*rfp);
		if((intervalPoints.back().secs - intervalPoints.front().secs) > intervalTime)
		{
			// find the avg power and color code it and create a polyline...
			AvgPower avgPower = for_each(intervalPoints.begin(),
										 intervalPoints.end(),
										 AvgPower());
			// find the color
			QColor color = GetColor(cp,avgPower);
			// create the polyline
			CreateSubPolyLine(intervalPoints,oss,color);
			intervalPoints.clear();
			intervalPoints.push_back(*rfp);
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


string GoogleMapControl::CreateIntervalMarkers(RideItem *ride)
{
	ostringstream oss;
	oss.precision(6);
	oss.setf(ios::fixed,ios::floatfield);

	oss << "var marker;" << endl;
	int currentInterval = 0;
	std::vector<RideFilePoint> intervalPoints;
	foreach(RideFilePoint* rfp, ride->ride()->dataPoints())
	{
		intervalPoints.push_back(*rfp);
		if(currentInterval < rfp->interval)
		{
			// want to see avg power, avg speed, alt changes, avg hr
			double distance = intervalPoints.back().km -
							   intervalPoints.front().km ;
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

			AltGained altGained =for_each(intervalPoints.begin(),
									   intervalPoints.end(),
									   AltGained());
			oss << "marker = new GMarker(new GLatLng( ";
			oss<< rfp->lat << "," << rfp->lon << "));" << endl;
			oss << "marker.bindInfoWindowHtml(" <<endl;
			oss << "\"<p><h3>Lap: " << currentInterval << "</h3></p>" ;
			oss << "<p>Distance: " << distance << "</p>" ;
			oss << "<p>Time: " << time.toString().toStdString() << "</p>";
			oss << "<p>Avg Speed</>: " << avgSpeed << "</p>";
			if(avgHr != 0) {
				oss << "<p>Avg HR: " << avgHr << "</p>";
			}
			if(avgPower != 0)
			{
				oss << "<p>Avg Power: " << avgPower << "</p>";
			}
			oss << "<p>Alt Gained: " << altGained << "</p>";
			oss << "\");" << endl;
			oss << "map.addOverlay(marker);" << endl;

			currentInterval = rfp->interval;
			intervalPoints.clear();
		}
	}
	return oss.str();
}
