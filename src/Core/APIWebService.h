/*
 * Copyright 2015 (c) Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_APIWebService_h
#define _GC_APIWebService_h

#include "httprequesthandler.h"
#include "RideItem.h"
#include "RideMetadata.h"
#include <QDir>

struct listRideSettings {
    bool intervals;
    QList<int> wanted; // metrics to list
    QList<FieldDefinition> metafields;
    QList<QString> metawanted; // metadata to list
};

class APIWebService : public HttpRequestHandler
{

    public:

        // nothing to do in constructor
        APIWebService(QDir home, QObject *parent=NULL) : HttpRequestHandler(parent), home(home) {}

        // request despatchers
        void service(HttpRequest &request, HttpResponse &response);
        void athleteData(QStringList &paths, HttpRequest &request, HttpResponse &response);

        // Discrete API endpoints
        void listAthletes(HttpRequest &request, HttpResponse &response);
        void listRides(QString athlete, HttpRequest &request, HttpResponse &response);
        void listActivity(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response);
        void listMMP(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response);
        void listZones(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response);

        // utility
        void writeRideLine(RideItem &item, HttpRequest *request, HttpResponse *response);

    private:
        QDir home;
};

#endif
