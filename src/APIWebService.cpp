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

#include "Settings.h"
#include "GcUpgrade.h"
#include "RideDB.h"

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
        listAthletes(request, response); // return csv list of all athlete and their characteristics
        return;
    }

    // we don't have a fave icon
    if (paths[0] == "favicon.ico") return;

    // Call to retreive athlete data, downstream will resolve
    // which functions to call for different data requests
    athleteData(paths, request, response);
}

void
APIWebService::athleteData(QStringList &paths, HttpRequest &request, HttpResponse &response)
{

    // LIST ACTIVITIES FOR ATHLETE
    if (paths.count() == 2) listRides(paths[0], request, response);
    else if (paths.count() > 2) {

        QString athlete = paths[0];
        paths.removeFirst();

        // GET ACTIVITY
        if (paths[0] == "activity") listActivity(athlete, paths, request, response);

        // GET MMP
        if (paths[0] == "mmp") listMMP(athlete, paths, request, response);
    }
}

void
APIWebService::listAthletes(HttpRequest &request, HttpResponse &response)
{
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    // This will read the user preferences and change the file list order as necessary:
    QFlags<QDir::Filter> spec = QDir::Dirs;
    QStringList names;
    names << "*"; // anything

    response.write("name,dob,weight,height,sex\n");
    foreach(QString name, home.entryList(names, spec, QDir::Name)) {

        // sure fire sign the athlete has been upgraded to post 3.2 and not some
        // random directory full of other things & check something basic is set
        QString ridedb = home.absolutePath() + "/" + name + "/cache/rideDB.json";
        if (QFile(ridedb).exists() && appsettings->cvalue(name, GC_SEX, "") != "") {
            // we got one
            QString line = name;
            line += ", " + appsettings->cvalue(name, GC_DOB).toDate().toString("yyyy/MM/dd");
            line += ", " + QString("%1").arg(appsettings->cvalue(name, GC_WEIGHT).toDouble());
            line += ", " + QString("%1").arg(appsettings->cvalue(name, GC_HEIGHT).toDouble());
            line += (appsettings->cvalue(name, GC_SEX).toInt() == 0) ? ", Male" : ", Female";
            line += "\n";

            // out a line
            response.write(line.toLocal8Bit());
        }
    }
}


void 
APIWebService::writeRideLine(QList<int> wanted, RideItem &item, HttpRequest *request, HttpResponse *response)
{
    // date, time, filename
    response->write(item.dateTime.date().toString("yyyy/MM/dd").toLocal8Bit());
    response->write(",");
    response->write(item.dateTime.time().toString("hh:mm:ss").toLocal8Bit());;
    response->write(",");
    response->write(item.fileName.toLocal8Bit());

    if (wanted.count()) {
        // specific metrics
        foreach(int index, wanted) {
            double value = item.metrics()[index];
            response->write(",");
            response->write(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
        }
    } else {

        // all metrics...
        foreach(double value, item.metrics()) {
            response->write(",");
            response->write(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
        }
    }
    response->write("\n");
}

void
APIWebService::listActivity(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("get activity under construction");
}

void
APIWebService::listMMP(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("get mmp under construction");
}
