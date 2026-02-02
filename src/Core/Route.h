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
class  Routes;
struct RoutePoint;
struct RouteRide;

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
        void setId(QUuid x) { _id = x; }
        QList<RouteRide> getRides() const { return rides; }

        double getMinLat();
        void setMinLat(double);
        double getMaxLat();
        void setMaxLat(double);
        double getMinLon();
        void setMinLon(double);
        double getMaxLon();
        void setMaxLon(double);

        // managing points and matched rides
        int addPoint(RoutePoint _point);
        double distance(double lat1, double lon1, double lat2, double lon2);

        // find segments in ridefiles
        void search(RideItem *, RideFile*, QList<IntervalItem*>&);

    private:

        Routes *routes;
        QUuid _id; // unique id

        QString name; // name, typically users name them by year e.g. "Col de Saxel"
        QList<RoutePoint> points;
        QList<RouteRide> rides;

        double minLat, maxLat;
        double minLon, maxLon;
};

struct RoutePoint // represents a point within a segment
{
    RoutePoint() : lon(0.0), lat(0.0) {}
    RoutePoint(double lon, double lat) : lon(lon), lat(lat) {}

    double lon, lat;
};


class Routes : public QObject { // top-level object with API and map of segments/rides

    Q_OBJECT;

    friend class ::RideItem; // access the route/ride map

    public:

        Routes(Context *context, const QDir &home);
        ~Routes();

        // checksum changes as routes added
        quint16 getFingerprint() const;

        // managing the list of route segments
        void readRoutes();
        int newRoute(QString name);
        void createRouteFromInterval(IntervalItem *activeInterval);
        void deleteRoute(QUuid);
        void deleteRoute(int);
        void renameRoute(QUuid identifier, QString newname);
        void writeRoutes();

        // find in a ride
        void search(RideItem*, RideFile* ride, QList<IntervalItem*>&here);

        // public access to routes list
        QList<RouteSegment> routes;

    protected:

    private:
        QDir home;
        Context *context;
};

struct RouteRide { // represents a section of a ride that matches a segment

    RouteRide() {}
    RouteRide(QString filename, QDateTime startTime, double start, double stop, double precision) :
              filename(filename), startTime(startTime), start(start), stop(stop), precision(precision) {}


    QString filename;
    QDateTime startTime;
    double start, stop;
    double precision;

};

#endif // ROUTE_H
