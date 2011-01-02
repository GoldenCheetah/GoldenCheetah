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

#ifndef _GC_GoogleMapControl_h
#define _GC_GoogleMapControl_h

#include <QWidget>
#include <QtWebKit>
#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include "RideFile.h"
#include "MainWindow.h"

class QMouseEvent;
class RideItem;
class MainWindow;
class QColor;
class QVBoxLayout;
class QTabWidget;

class GoogleMapControl : public QWidget
{
Q_OBJECT

 private:
    MainWindow *main;
    QVBoxLayout *layout;
    QWebView *view;
    MainWindow *parent;
    GoogleMapControl();  // default ctor
    int range;
    std::string CreatePolyLine();
    void CreateSubPolyLine(const std::vector<RideFilePoint> &points,
                           std::ostringstream &oss,
                           int avgPower);
    std::string CreateMapToolTipJavaScript();
    std::string CreateIntervalHtml(QHash<QString,RideMetricPtr> &metrics, QStringList &intervalMetrics,
                                   QString &intervalName, bool useMetrics);
    std::string CreateMarkers(RideItem *ride);
    std::string CreateIntervalMarkers(RideItem *ride);
    void loadRide();
    std::string CreateMarker(int number, double lat, double lon, std::string &html);
    // the web browser is loading a page, do NOT start another load
    bool loadingPage;
    // the ride has changed, load a new page
    bool newRideToLoad;

    QColor GetColor(int watts);

    // a GPS normalized vectory of ride data points,
    // when a GPS unit loses signal it seems to
    // put a coordinate close to 180 into the data
    std::vector<RideFilePoint> rideData;
    // current ride CP
    int rideCP;
    // current HTML for the ride
    std::ostringstream currentPage;
    RideItem *current;

 public slots:
    void rideSelected();

 private slots:
    void loadStarted();
    void loadFinished(bool);

 protected:
    void createHtml(RideItem *);
    void resizeEvent(QResizeEvent *);

 public:
    GoogleMapControl(MainWindow *);
    virtual ~GoogleMapControl() { }
};

#endif
