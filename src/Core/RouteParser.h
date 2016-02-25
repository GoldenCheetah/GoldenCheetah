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

#ifndef _ROUTEPARSER_H
#define _ROUTEPARSER_H

#include "GoldenCheetah.h"

#include <QXmlDefaultHandler>
#include "Route.h"

class RouteParser : public QXmlDefaultHandler
{

public:
    // marshall
    static bool serialize(QString, QList<RouteSegment>);

    // unmarshall
    bool startDocument();
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );
    QList<RouteSegment> getRoutes();

protected:
    QString buffer;
    RouteSegment route;
    QList<RouteSegment> routes;
    RoutePoint point;
    int loadcount;

    double minLon, maxLon;
    double minLat, maxLat;

};

#endif // ROUTEPARSER_H
