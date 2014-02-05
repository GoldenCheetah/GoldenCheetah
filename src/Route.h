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

class RouteSegment
{
    public:
        RouteSegment();
        RouteSegment(Routes *routes);

        Routes *routes;
        QString getName();
        void setName(QString _name);

        QUuid id() const { return _id; }
        void setId(QUuid x) { _id = x; }

        int addPoint(RoutePoint _point);
        int addRide(RouteRide _ride);
        int addRideForRideFile(const RideFile *ride, double start, double stop, double precision);
        bool parseRideFileName(Context *context, const QString &name, QString *notesFileName, QDateTime *dt);

        double distance(double lat1, double lon1, double lat2, double lon2);

        void searchRouteInAllRides(Context *context);
        void searchRouteInRide(RideFile* ride, bool freememory, QTextStream* log);

        void removeRideInRoute(RideFile* ride);

        QList<RoutePoint> getPoints();

        QList<RouteRide> getRides();

        QList<RouteRide2> getRouteRides();

    private:
        QUuid _id; // unique id

        QString name; // name, typically users name them by year e.g. "Col de Saxel"

        QList<RoutePoint> points;

        QList<RouteRide> rides;

};

class Routes : public QObject {

    Q_OBJECT;

    public:
        Routes(Context *context, const QDir &home);
        void readRoutes();
        int newRoute(QString name);
        void updateRoute(int index, QString name);
        void deleteRoute(int);
        void writeRoutes();
        QList<RouteSegment> routes;

        void createRouteFromInterval(IntervalItem *activeInterval);
        void searchRoutesInRide(RideFile* ride);
        void removeRideInRoutes(RideFile* ride);

    public slots:
        void addRide(RideItem*);
        void deleteRide(RideItem* ride);

    signals:
        void routesChanged();


    private:
        QDir home;
        Context *context;
};


/*class RouteRide2 : public QObject {

    Q_OBJECT;

    public :
        RouteRide2();

        RouteRide2(QDateTime startTime, double start, double stop, double precision);// : startTime(startTime), start(start), stop(stop), precision(precision)  {}

        QDateTime startTime;
        double start, stop;
        double precision;

};*/

struct RouteRide {

    //
    QDateTime startTime;
    double start, stop;
    double precision;



    RouteRide(QDateTime startTime, double start, double stop, double precision) : startTime(startTime), start(start), stop(stop), precision(precision)  {}

    RouteRide() {}
};

struct RoutePoint
{
    double lon, lat;

    RoutePoint() : lon(0.0), lat(0.0) {}

    RoutePoint(double lon, double lat) : lon(lon), lat(lat) {}
    //double value(RideFile::SeriesType series) const;
};

#endif // ROUTE_H
