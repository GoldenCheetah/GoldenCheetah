/*
 * Copyright (c) 2011 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef ROUTE_H
#define ROUTE_H
#include "GoldenCheetah.h"

#include <QProgressBar>
#include <QMessageBox>
#include <QString>
#include <QDate>
#include <QFile>

#include "Context.h"

class  RideFile;
class  RouteRide2;
class  Routes;

struct RouteRide;
struct RoutePoint;

class RouteSegment // represents a segment we match against
{
    public:

        RouteSegment();
        RouteSegment(Routes *routes);

        // accessors
        QString getName();
        void setName(QString _name);
        QUuid id() const { return _id; }
        QList<RoutePoint> getPoints();
        QList<RouteRide> getRides();
        QList<RouteRide2> getRouteRides();
        void setId(QUuid x) { _id = x; }

        // managing points and matched rides
        int addPoint(RoutePoint _point);
        int addRide(RouteRide _ride);
        int addRideForRideFile(const RideFile *ride, double start, double stop, double precision);
        bool parseRideFileName(Context *context, const QString &name, QString *notesFileName, QDateTime *dt);

        double distance(double lat1, double lon1, double lat2, double lon2);

        // segments always work on actual ridefiles
        void searchRouteInRide(RideFile* ride);
        void removeRideInRoute(RideItem* ride);

    private:

        Routes *routes;
        QUuid _id; // unique id

        QString name; // name, typically users name them by year e.g. "Col de Saxel"
        QList<RoutePoint> points;
        QList<RouteRide> rides;
};

class Routes : public QObject { // top-level object with API and map of segments/rides

    Q_OBJECT;

    friend class ::RideItem; // access the route/ride map

    public:

        Routes(Context *context, const QDir &home);

        // managing the list of route segments
        void readRoutes();
        int newRoute(QString name);
        void createRouteFromInterval(IntervalItem *activeInterval);
        void updateRoute(int index, QString name);
        void deleteRoute(int);
        void writeRoutes();

        // remove/searching rides 
        void removeRideInRoutes(RideFile* ride);
        void searchRoutesInRide(RideItem* ride);

    public slots:

        // adding and deleting rides
        void addRide(RideItem*);
        void deleteRide(RideItem* ride);

    signals:
        void routesChanged();

    protected:
        QList<RouteSegment> routes;

    private:
        QDir home;
        Context *context;
};


struct RouteRide { // represents a section of a ride that matches a segment

    RouteRide() {}
    RouteRide(QString filename, QDateTime startTime, double start, double stop, double precision) : 
              filename(filename), startTime(startTime), start(start), stop(stop), precision(precision)  {}


    QString filename;
    QDateTime startTime;
    double start, stop;
    double precision;

};

struct RoutePoint // represents a point within a segment
{
    RoutePoint() : lon(0.0), lat(0.0) {}
    RoutePoint(double lon, double lat) : lon(lon), lat(lat) {}

    double lon, lat;
};

#endif // ROUTE_H
