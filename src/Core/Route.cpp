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
RouteSegment::RouteSegment() : minLat(180), maxLat(-180), minLon(180), maxLon(-180)
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

double
RouteSegment::getMinLat() {
    return minLat;
}

void
RouteSegment::setMinLat(double _minLat) {
    minLat = _minLat;
}

double
RouteSegment::getMaxLat() {
    return maxLat;
}

void
RouteSegment::setMaxLat(double _maxLat) {
    maxLat = _maxLat;
}

double
RouteSegment::getMinLon() {
    return minLon;
}

void
RouteSegment::setMinLon(double _minLon) {
    minLon = _minLon;
}

double
RouteSegment::getMaxLon() {
    return maxLon;
}

void
RouteSegment::setMaxLon(double _maxLon) {
    maxLon = _maxLon;
}

QList<RoutePoint> RouteSegment::getPoints() {
    return points;
}

int
RouteSegment::addPoint(RoutePoint _point)
{
    points.append(_point);

    // Update Min-Max
    if (minLat==180 || _point.lat<minLat)
        minLat = _point.lat;
    if (maxLat==-180 || _point.lat>maxLat)
        maxLat = _point.lat;
    if (minLon==180 || _point.lon<minLon)
        minLon = _point.lon;
    if (maxLon==-180 || _point.lon>maxLon)
        maxLon = _point.lon;

    return points.count();
}

void 
RouteSegment::search(RideItem *item, RideFile*ride, QList<IntervalItem*>&here)
{
    //qDebug() << "Opening ride: " << item->fileName << " for " << name;


    double minimumprecision = 0.100; //100m
    double maximumprecision = 0.001; //1m , was 10m but changed to 1m for small segment.
                                     // if there is performance issue we can perhaps have 1m for small segments
                                     // and keep 10m for longer.

    double precision = -1;

    int found = 0;
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
                            // qDebug() << "    Start point identified...";
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

                                if (_nextdist<minimumdistance*1.2)
                                    end = j+10;
                                else // we move away
                                    j = end;
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
                            // qDebug() << "    Start time " << start << "\r\n";
                        }

                        if (minimumdistance>precision)
                            precision = minimumdistance;

                        break;
                    }
                    else {
                        //qDebug() << "    WARNING route diverge at " << point->secs << "(" << i <<") after " << (point->secs-start)<< "secs for " << minimumdistance << "km " << routepoint.lat << "-" << routepoint.lon << "/" << point->lat << "-" << point->lon << "\r\n";

                        diverge++;
                        if (diverge>2) {
                            //qDebug() << "    STOP route diverge at " << point->secs << "(" << i <<") after " << (point->secs-start)<< "secs for " << minimumdistance << "km " << routepoint.lat << "-" << routepoint.lon << "/" << point->lat << "-" << point->lon << "\r\n";

                            start = -1; //try to restart
                            n = 0;
                        }
                        //present = true;
                    }
                }
            }
        }


        if (!present) {
            //qDebug() << "    Route not identified (distance " << precision << "km)\r\n";

            break;

        }
        
        stop = point->secs;
        
        if (n == this->getPoints().count()-1) {

            // Add the interval and continue search
            //qDebug() << "    >>> Route identified in ride: " << name << " start: " << start << " stop: " << stop << " (distance " << precision << "km)\r\n";

            IntervalItem *intervalItem = new IntervalItem(item, name,
                                                          start, stop,
                                                          ride->timeToDistance(start),
                                                          ride->timeToDistance(stop),
                                                          ++found,
                                                          QColor(255,127,80),
                                                          false,
                                                          RideFileInterval::ROUTE);
            intervalItem->route = id();
            here << intervalItem;

            // reset to restart find on next iteration
            start = -1;
            n=0;

            // skip on a few samples to avoid finding the same
            // segment again - this happens when a segment is very
            // short and ends at lights or top of a hill.
            lastpoint += 30;
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
    readRoutes();
}

Routes::~Routes()
{
    writeRoutes();
}

quint16
Routes::getFingerprint() const
{
    // all the QUuids will suffice
    QByteArray ba;
    foreach(RouteSegment segment, routes) ba += segment.id().toByteArray();

    // we spot other things separately
    return qChecksum(ba, ba.length());
}

void
Routes::readRoutes()
{
    QFile routeFile(home.canonicalPath() + "/routes.xml");

    QXmlInputSource source( &routeFile );
    QXmlSimpleReader xmlReader;
    RouteParser handler;
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
Routes::deleteRoute(QUuid identifier)
{
    // find via Identifier then delete
    for(int i=0; i<routes.count(); i++) {
        if (routes.at(i).id() == identifier) {
            deleteRoute(i);
            return;
        }
    }
}

void
Routes::deleteRoute(int index)
{
    // now delete!
    routes.removeAt(index);
    writeRoutes();
}

void
Routes::renameRoute(QUuid identifier, QString newname)
{
    // find via Identifier then delete
    for(int i=0; i<routes.count(); i++) {
        if (routes.at(i).id() == identifier) {
            RouteSegment route = routes.at(i);
            route.setName(newname);
            routes.replace(i, route);
            writeRoutes();
            return;
        }
    }
}

void
Routes::writeRoutes()
{
    // update routes.xml

    QString file = QString(home.canonicalPath() + "/routes.xml");
    RouteParser::serialize(file, routes);
}

void
Routes::search(RideItem *item, RideFile*ride, QList<IntervalItem*>&here)
{
    if (ride) {

        // search all segments
        for (int routecount=0;routecount<routes.count();routecount++) {
            RouteSegment *segment = &routes[routecount];

            // The third decimal place is worth up to 110 m
            if (ride->getMinPoint(RideFile::lat).toDouble()<segment->getMinLat()+0.001 &&
                ride->getMaxPoint(RideFile::lat).toDouble()>segment->getMaxLat()-0.001 &&
                ride->getMinPoint(RideFile::lon).toDouble()<segment->getMinLon()+0.001 &&
                ride->getMaxPoint(RideFile::lon).toDouble()>segment->getMaxLon()-0.001   )

            segment->search(item, ride, here);
        }
    }
}

void
Routes::createRouteFromInterval(IntervalItem *activeInterval) 
{

    // if refresh is running cancel it now, as we're adding
    // a new route to search, so will need to restart
    context->athlete->rideCache->cancel();

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
        if (point->secs >= activeInterval->start && point->secs <= activeInterval->stop) {
            if (point->lat != 0 && point->lon != 0 &&
                ceil(point->lat) != 180 && ceil(point->lon) != 180) {

                if (lastLat == 0 || lastLon == 0 || point->secs == activeInterval->stop) {
                    RoutePoint _point = RoutePoint(point->lon, point->lat);
                    route->addPoint(_point);
                } else  {
                    // distance with last point
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

    // update on disk
    context->athlete->routes->writeRoutes();

    // now go and refresh !
    context->athlete->rideCache->refresh();
}
