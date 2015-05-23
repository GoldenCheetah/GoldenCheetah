/*
 * Copyright (c) 2011,2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "Route.h"

#include "Athlete.h"
#include "MainWindow.h"
#include "RideCache.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RouteParser.h"
#include "RideFile.h"
#include "GProgressDialog.h"

#include <QString>
#include <QFile>
#include <QXmlInputSource>
#include <QXmlSimpleReader>


#define tr(s) QObject::tr(s)

#define pi 3.14159265358979323846

/*
 * RouteSegment
 *
 */
RouteSegment::RouteSegment()
{
    _id = QUuid::createUuid(); // in case it isn't set yet
}

RouteSegment::RouteSegment(Routes *routes) : routes(routes)
{
    _id = QUuid::createUuid(); // in case it isn't set yet
}

QString RouteSegment::getName() {
    return name;
}

void
RouteSegment::setName(QString _name)
{
    name = _name;
}

QList<RoutePoint> RouteSegment::getPoints() {
    return points;
}

int
RouteSegment::addPoint(RoutePoint _point)
{
    points.append(_point);
    return points.count();
}

QList<RouteRide> RouteSegment::getRides() {
    return rides;
}

int
RouteSegment::addRide(RouteRide _ride)
{
    rides.append(_ride);
    return rides.count();
}

int
RouteSegment::addRideForRideFile(const RideFile *ride, double start, double stop, double precision)
{
    RouteRide _route = RouteRide(ride->getTag("Filename",""), ride->startTime(), start, stop, precision);
    rides.append(_route);
    return rides.count();
}

void
RouteSegment::removeRideInRoute(RideItem* ride)
{
    for (int n=0; n< this->getRides().count();n++) {
        const RouteRide* routeride = &getRides().at(n);
        if (ride && routeride->startTime == ride->dateTime) {
            rides.removeAt(n);
        }
    }
}

void
RouteSegment::searchRouteInRide(RideFile* ride)
{
    double minimumprecision = 0.100; //100m
    double maximumprecision = 0.010; //10m
    double precision = -1;

    int diverge = 0;
    int lastpoint = -1; // Last point to match
    double start = -1, stop = -1; // Start and stop secs

    for (int n=0; n< this->getPoints().count();n++) {
        RoutePoint routepoint = this->getPoints().at(n);

        bool present = false;
        RideFilePoint* point;

        for (int i=lastpoint+1; i<ride->dataPoints().count();i++) {
            point = ride->dataPoints().at(i);

            double minimumdistance = -1;

            if (point->lat != 0 && point->lon !=0 &&
                ceil(point->lat) != 180 && ceil(point->lon) != 180 &&
                ceil(point->lat) != 540 && ceil(point->lon) != 540) {
                // Valid GPS value
                if (start == -1) {
                    diverge = 0;
                    // Calculate distance to route point
                    double _dist = distance(routepoint.lat, routepoint.lon, point->lat, point->lon) ;
                    minimumdistance = _dist;

                    if (precision == -1 || _dist<precision)
                        precision = _dist;

                    if (_dist>1) {
                        // fare away from reference point
                        i += 50;
                    } else {
                        if (_dist<minimumprecision) {
                            start = 0; //try to start
                        }
                    }
                }

                if (start != -1) {
                    int end = i+10;
                    for (int j=i; j<ride->dataPoints().count() && j<end;j++) {
                        RideFilePoint* nextpoint = ride->dataPoints().at(j);

                        if (nextpoint->lat != 0 && nextpoint->lon !=0 && ceil(nextpoint->lat) != 180 && ceil(nextpoint->lon) != 180) {
                            double _nextdist = distance(routepoint.lat, routepoint.lon, nextpoint->lat, nextpoint->lon) ;

                            if (minimumdistance ==-1 || _nextdist<minimumdistance){
                                //new minimumdistance
                                point = nextpoint;
                                i = j;
                                minimumdistance = _nextdist;
                            }
                            if (_nextdist <= minimumprecision) {
                                end = j+10;
                            }

                            if (_nextdist <= maximumprecision) {
                                // maximum precision reached
                                j = end;
                            }
                        }
                    }

                    if (minimumdistance <= minimumprecision) {
                        // Close enough from reference point

                        present = true;
                        lastpoint = i;
                        if (start == 0) {
                            start = point->secs;
                            precision = 0;
                        }

                        if (minimumdistance>precision)
                            precision = minimumdistance;

                        break;
                    }
                    else {
                        diverge++;
                        if (diverge>2) {
                            start = -1; //try to restart
                            n = 0;
                        }
                        //present = true;
                    }



                }
            }

        }
        stop = point->secs;


        if (!present) {
            break;
        } else {
            if (n == this->getPoints().count()-1){
                // OK
                //Add the interval and continue search
                this->addRideForRideFile(ride, start, stop, precision);
                start = -1;
                n=0;
                //break;
            }
        }
    }
}


//  This function converts decimal degrees to radians
double deg2rad(double deg) {
  return (deg * pi / 180);
}

// This function converts radians to decimal degrees
double rad2deg(double rad) {
  return (rad * 180 / pi);
}

// lat1, lon1 = point 1, Latitude and Longitude of
// lat2, lon2 = Latitude and Longitude of point 2
double
RouteSegment::distance(double lat1, double lon1, double lat2, double lon2) {
  double _theta, _dist;
  _theta = lon1 - lon2;
  if (_theta == 0 && (lat1 - lat2) == 0)
      _dist = 0;
  else {
      _dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(_theta));
      _dist = acos(_dist) * 6371;
  }
  return (_dist);
}



/*
 * Routes (list of RouteSegment)
 *
 *
 */

Routes::Routes(Context *context, const QDir &home)
{
    this->home = home;
    this->context = context;

    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(addRide(RideItem*)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(deleteRide(RideItem*)));

    readRoutes();
}

void
Routes::readRoutes()
{
    QFile routeFile(home.canonicalPath() + "/routes.xml");

    QXmlInputSource source( &routeFile );
    QXmlSimpleReader xmlReader;
    RouteParser( handler );
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );
    routes = handler.getRoutes();
}

int
Routes::newRoute(QString name)
{
    RouteSegment add;
    add.setName(name);
    routes.insert(0, add);

    return 0; // always add at the top
}

void
Routes::updateRoute(int index, QString name)
{
    routes[index].setName(name);

    // save changes away
    writeRoutes();

}

void
Routes::deleteRoute(int index)
{
    // now delete!
    routes.removeAt(index);
    writeRoutes();
}

void
Routes::writeRoutes()
{
    // update routes.xml

    QString file = QString(home.canonicalPath() + "/routes.xml");
    RouteParser::serialize(file, routes);

    routesChanged(); // signal!
}

// add a ride (after import / download)
void
Routes::addRide(RideItem* ride)
{
    searchRoutesInRide(ride);
    writeRoutes();
}

// delete a ride
void
Routes::deleteRide(RideItem* ride)
{
    for (int routecount=0;routecount<routes.count();routecount++) {
        RouteSegment *route = &routes[routecount];
        route->removeRideInRoute(ride);
    }
}

void
Routes::searchRoutesInRide(RideItem* item)
{
    if (item->present.contains("G")) {

        bool doclose=true;
        if(item->isOpen()) doclose=false;
        RideFile *ride = item->ride();

        if (ride) {

            // we opened the ride so search all segments
            for (int routecount=0;routecount<routes.count();routecount++) {
                RouteSegment *segment = &routes[routecount];
                segment->searchRouteInRide(ride);
            }

            if (doclose) item->close();
        }
    }
}

void
Routes::createRouteFromInterval(IntervalItem *activeInterval) {
    // create a new route
    int index = context->athlete->routes->newRoute("route");
    RouteSegment *route = &context->athlete->routes->routes[index];

    QRegExp watts("\\([0-9]* *watts\\)");

    QString name = activeInterval->name; //activeInterval->text(0).trimmed();
    if (name.contains(watts))
        name = name.left(name.indexOf(watts)).trimmed();

    if (name.length()<4 || name.startsWith("Selection #") )
        name = QString(tr("Route #%1")).arg(context->athlete->routes->routes.length());

    route->setName(name);

    // Construct the route with interval gps data
    double dist = 0, lastLat = 0, lastLon = 0;

    foreach (RideFilePoint *point, activeInterval->rideItem()->ride()->dataPoints()) {
        if (point->secs >= activeInterval->start && point->secs < activeInterval->stop) {
            if (lastLat != 0 && lastLon != 0 &&
                point->lat != 0 && point->lon != 0 &&
                ceil(point->lat) != 180 && ceil(point->lon) != 180) {
                // distance ith last point
                double _dist = route->distance(lastLat, lastLon, point->lat, point->lon);

                if (_dist>=0.001)
                    dist += _dist;

                if (dist>0.02) {
                   RoutePoint _point = RoutePoint(point->lon, point->lat);
                   route->addPoint(_point);
                   dist = 0;
                }
            }
            lastLat = point->lat;
            lastLon = point->lon;
        }
    }
}
