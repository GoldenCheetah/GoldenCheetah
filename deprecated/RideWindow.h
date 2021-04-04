/*
 * Copyright (c) 2011 Greg Lonnon (greg.lonnon@gmail.com)
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

#ifndef RIDEWINDOW_H
#define RIDEWINDOW_H

#include <QWidget>
#include <QWebView>
#include <QWebPage>
#include <QWebFrame>
#include <QWebInspector>
#include <QDebug>
#include <limits>
#include "TrainSidebar.h"
#include "RealtimeController.h"
#include "RideItem.h"
#include "RideFile.h"

class Route
{
    QVector<RideFilePoint> route;
public:
    Route(const QVector<RideFilePoint*> &r)
    {
        foreach(RideFilePoint *rfp, r)
        {
            route.push_back(*rfp);
        }
    }
    // look 50 meters down and find the local max
    double calcGrade(int position)
    {
        int i = position;
        double distance = 0.05;  // 50 meters
        double endDistance = route[position].km + distance;
        while(route[i].km < endDistance)
        {
            i++;
        }
        double grade = (route[i].alt - route[position].alt) / (route[i].km - route[position].km) * 100;
        return grade;
    }

};


class Bike
{
    double ccr;  // road resistance
    double speed; // speed
public:
    double getCcr() { return ccr;}
    void setCcr(double c) { ccr = c; }
    double getSpeed() { return speed; }
    void setSpeed(double s) { speed = s; }
};

class Rider
{
private:
    Route route;

};

class BikePhysicsEngine
{
    // check out http://www.flacyclist.com/content/perf/science.html for the
    // math behind this...

    double ccr;  // roll resistance 0.001 track, 0.002 concrete, 0.004 asphalt, 0.008 rough road
    double weight; // kg rider + bike
    double wind; // head wind only
    double cda; // areo
    double temp; // C
    double air; // air pressure
    double speed;
    double grade;

protected:

    double calcFAir(double speed)
    {
        return 0.5 * cda * (air/((temp + 273.15) * 287.058) * pow((speed + wind), 2));
    }

    double calcFGrade()
    {
        return 9.8 * weight * grade;
    }

    double calcFRoll()
    {
        return 9.8 * weight * ccr;
    }

public:
    void setCcr(double c) { ccr = c; }
    void setWeight(double w) { weight = w; }
    void setWind(double w) { wind = w; }
    void setGrade(double g) { grade = g; }
    void setSpeed(double s) { speed = s; }

    double SpeedGivenPower(double watts)
    {
        return watts/20;
    }
    double PowerGivenSpeed(double speed)
    {
        return calcFAir(speed) + calcFGrade() + calcFRoll();
    }
};

/// The rider class holds all the information needed to display the rider
/// on the web page
class RiderBridge : public QObject
{
    Q_OBJECT

protected:
    QString name;

    QString markerColor;

    // current ride item
    RideItem *ride;
    // current data
    RideFilePoint rfp;

    // currnet position in the ride
    int curPosition;
    double curTime; // in seconds

    int load;  // load generator (computrainer only)

public:
    RiderBridge() {}
    void setRide(RideItem *ri) { ride = ri; }

    // list of all properties that are visible
    // in js

    Q_PROPERTY(QString name READ getName WRITE setName);
    QString getName() { return name; }
    void setName(QString s) { name = s; }

    Q_PROPERTY(int time READ getSecs WRITE setSecs);
    double getSecs() { return rfp.secs; }
    void setSecs(double s) { rfp.secs = s; }

    Q_PROPERTY(double hr READ getHr WRITE setHr);
    double getHr() { return rfp.hr; }
    void setHr(double h) { rfp.hr= h; }

    Q_PROPERTY(double Watts READ getWatts WRITE setWatts);
    double getWatts() { return rfp.watts; }
    void setWatts(double w) { rfp.watts = w; }

    Q_PROPERTY(double speed READ getSpeed WRITE setSpeed);
    double getSpeed() { return rfp.kph; }
    void setSpeed(double s) { rfp.kph = s; }

   // Q_PROPERTY(double wheelRpm READ getWheelRpm WRITE setWheelRpm);
   // double getWheelRpm() { return rfp.cad; }
   // void setWheelRpm(double w) { rfp.cad = w; }

    Q_PROPERTY(double load READ getLoad WRITE setLoad);
    double getLoad() { return load; }
    void setLoad(double l) { load = l; }

    Q_PROPERTY(double cadence READ getCadence WRITE setCadence);
    double getCadence() { return rfp.cad; }
    void setCadence(double c) { rfp.cad = c; }

    Q_PROPERTY(double distance READ getDistance WRITE setDistance);
    double getDistance() { return rfp.km; }
    void setDistance(double d) { rfp.km = d;}

    Q_PROPERTY(double lon READ getLon WRITE setLon);
    double getLon() { return rfp.lon; }
    void setLon(double l) { rfp.lon = l;}

    Q_PROPERTY(double lat READ getLat WRITE setLat);
    double getLat() { return rfp.lat; }
    void setLat(double l) { rfp.lat = l;}

    Q_PROPERTY(QVariantList routeLon READ getRouteLon);
    QVariantList getRouteLon() {
        QVariantList route;
        foreach(RideFilePoint *rfp, ride->ride()->dataPoints())
        {
            route.push_back(QVariant(rfp->lon));
        }
        return route;
    }
    Q_PROPERTY(QVariantList routeLat READ getRouteLat);
    QVariantList getRouteLat() {
        QVariantList route;
        foreach(RideFilePoint *rfp, ride->ride()->dataPoints())
        {
            route.push_back(QVariant(rfp->lat));
        }
        return route;
    }

    Q_PROPERTY(QVariantList profile READ getProfile);
    QVariantList getProfile() {
        QVariantList route;
        foreach(RideFilePoint *rfp, ride->ride()->dataPoints())
        {
            route.push_back(QVariant(rfp->alt));
        }
        return route;
    }

    Q_PROPERTY(int position READ getPosition);
    int getPosition() {
        return curPosition;
    }


public slots:
    // set the curTime and find the position in the array closest to the time
    Q_INVOKABLE void SetTime(double secs)
    {
        // find the curPosition
        QVector<RideFilePoint*> dataPoints =  ride->ride()->dataPoints();
        curTime = secs;

        if(dataPoints.count() < curPosition)
        {
            curPosition = 0;
            return;
        }
        // make sure the current position is less than the new time
        if (dataPoints[curPosition]->secs < secs)
        {
            for( ; curPosition < dataPoints.count(); curPosition++)
            {
                if(dataPoints[curPosition]->secs < secs)
                {
                    return;
                }
            }
        }
        else
        {
            for( ; curPosition > 0; curPosition--)
            {
                if(dataPoints[curPosition]->secs > secs)
                {
                    return;
                }
            }
        }
        // update the rfp
        rfp = *dataPoints[curPosition];
    }
};



class RealtimeRider: public RiderBridge
{
    Q_OBJECT;
    //Route route;
    BikePhysicsEngine engine;

protected:
    int findCurrentPosition()
    {
        QVector<RideFilePoint*> route =  ride->ride()->dataPoints();
        // this is by distance
        for(int i = 0; i < route.count(); i++)
        {
            if(rfp.km < route[i]->km)
                return i;
        }
        return 1;
    }
    int findCurrentPositionByPower()
    {
        double speed = engine.SpeedGivenPower(rfp.watts);
        return (int) speed;

    }
    int findCurrentPositionByTime()
    {

        return 0;
    }

public slots:
    virtual void telemetryUpdate(const RealtimeData &data)
    {
        rfp.watts = data.getWatts();
        rfp.kph = data.getSpeed();
        rfp.cad = data.getCadence();
        rfp.km = data.getDistance();
        rfp.secs = data.getMsecs() / 1000;
        load = data.getLoad();
        curPosition = findCurrentPosition();
    }
public:
    RealtimeRider(Context *context)
    {
        curPosition = 1;
        connect(context,SIGNAL(telemetryUpdate(const RealtimeData &)), this,SLOT(telemetryUpdate(const RealtimeData &)));
    }
};

class RideWindow : public GcChartWindow
{
    Q_OBJECT

protected:
    RideItem *ride;
    QWebView *view;
    bool rideLoaded;
    Context *context;

    virtual void loadRide();
    RiderBridge *rider;

    void EnableWebInspector(QWebPage *page)
    {
        // put the QWebInspector on the page...
        QWebInspector * qwi = new QWebInspector();
        qwi->setPage(page);
        qwi->setVisible(true);
    }
public:
    explicit RideWindow(Context *);
signals:

public slots:
    void addJSObjects();
    void rideSelected();
};


class MapWindow : public RideWindow
{

protected:
    virtual void loadRide()
    {
        RideWindow::loadRide();
        view->page()->mainFrame()->load(QUrl("qrc:/ride/web/MapWindow.html"));
       //EnableWebInspector(view->page());  // turns on the javascript debugger
    }
public:
    explicit MapWindow(Context *context) : RideWindow(context) {}
};


class StreetViewWindow : public RideWindow
{
protected:
    virtual void loadRide()
    {
        RideWindow::loadRide();
        view->page()->mainFrame()->load(QUrl("qrc:/ride/web/StreetViewWindow.html"));
        // EnableWebInspector(view->page());  // turns on the javascript debugger
    }
public:
     explicit StreetViewWindow(Context *context) : RideWindow(context) {}

};

#endif // RIDEWINDOW_H
