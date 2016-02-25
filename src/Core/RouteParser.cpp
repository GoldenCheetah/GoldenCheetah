/*
 * Copyright (c) 2011, 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "RouteParser.h"
#include <QDate>
#include <QDebug>
#include <assert.h>

#include "Route.h"

#define DATETIME_FORMAT "yyyy/MM/dd hh:mm:ss' UTC'"

bool RouteParser::startDocument()
{
    buffer.clear();
    return true;
}

bool RouteParser::endElement( const QString&, const QString&, const QString &qName )
{
    if(qName == "name")
        route.setName(buffer.trimmed());
    else if (qName == "id")
        route.setId(QUuid(buffer.trimmed()));
    else if(qName == "route") {
        route.setMinLat(minLat);
        route.setMaxLat(maxLat);
        route.setMinLon(minLon);
        route.setMaxLon(maxLon);

        routes.append(route);
    } else if(qName == "point") {
        route.addPoint(point);
    } else if(qName == "lat") {
        point.lat = buffer.trimmed().toDouble();

        if (minLat==180 || point.lat<minLat)
            minLat = point.lat;
        if (maxLat==-180 || point.lat>maxLat)
            maxLat = point.lat;
    } else if(qName == "lon") {
        point.lon = buffer.trimmed().toDouble();

        if (minLon==180 || point.lon<minLon)
            minLon = point.lon;
        if (maxLon==-180 || point.lon>maxLon)
            maxLon = point.lon;
    }
    return true;
}

bool RouteParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes & )
{
    buffer.clear();
    if(name == "route") {
        route = RouteSegment();
        minLat = 180;
        minLon = 180;
        maxLat = -180;
        maxLon = -180;
    } else if(name == "point") {
        point = RoutePoint();
    }
    return true;
}

bool RouteParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

QList<RouteSegment> RouteParser::getRoutes()
{
    return routes;
}

bool
RouteParser::endDocument()
{
    return true;
}

bool
RouteParser::serialize(QString filename, QList<RouteSegment>routes)
{
    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Saving Route Data"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return false;
    };
    file.resize(0);
    QTextStream out(&file);

    // begin document
    out << "<routes>\n";

    // write out to file
    foreach (RouteSegment route, routes) {
            // main attributes
            //qDebug() << "write route " << route.id().toString();
            out<<QString("\t<route>\n"
                  "\t\t<name>%1</name>\n"
                  "\t\t<id>%2</id>\n") .arg(route.getName())
                                           .arg(route.id().toString());

            out << "\t\t<points>\n";
            //qDebug() << "route->getPoints() " << route.getPoints().count();
            foreach (RoutePoint point, route.getPoints() ) {
                out << QString("\t\t\t<point><lat>%1</lat><lon>%2</lon></point>\n").arg(point.lat).arg(point.lon);
            }
            out << "\t\t</points>\n";
            out <<QString("\t</route>\n");
    }

    // end document
    out << "</routes>\n";

    // close file
    file.close();

    return true; // success
}

