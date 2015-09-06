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

#include "APIWebService.h"
#include "GcUpgrade.h"

void
APIWebService::service(HttpRequest &request, HttpResponse &response)
{
    // remove trailing '/' from request, just to be consistent
    QString fullPath = request.getPath();
    while (fullPath.endsWith("/")) fullPath.chop(1);

    // get the paths
    QStringList paths = QString(request.getPath()).split("/");

    // we need to look at the first path to determine action
    if (paths.count() < 2) return; // impossible really

    // get ride of first blank, all URLs start with a '/'
    paths.removeFirst();

    // ROOT PATH RETURNS A LIST OF ATHLETES
    if (paths[0] == "") {
        listAthletes(response); // return csv list of all athlete and their characteristics
        return;
    }

    // we don't have a fave icon
    if (paths[0] == "favicon.ico") return;

    // Call to retreive athlete data, downstream will resolve
    // which functions to call for different data requests
    athleteData(paths, response);
}

void
APIWebService::athleteData(QStringList &paths, HttpResponse &response)
{

    // LIST ACTIVITIES FOR ATHLETE
    if (paths.count() == 2) listRides(paths[0], response);
    else if (paths.count() > 2) {

        QString athlete = paths[0];
        paths.removeFirst();

        // GET ACTIVITY
        if (paths[0] == "activity") listActivity(athlete, paths, response);

        // GET MMP
        if (paths[0] == "mmp") listMMP(athlete, paths, response);
    }
}

void
APIWebService::listAthletes(HttpResponse &response)
{
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("list athletes under construction");
}

void
APIWebService::listRides(QString athlete, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("get athlete ride list under construction");
}

void
APIWebService::listActivity(QString athlete, QStringList paths, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("get activity under construction");
}

void
APIWebService::listMMP(QString athlete, QStringList paths, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("get mmp under construction");
}
