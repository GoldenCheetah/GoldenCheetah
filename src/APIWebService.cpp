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

#include "RideFile.h"
#include "RideFileCache.h"
#include "CsvRideFile.h"

#include <QTemporaryFile>

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
    if (paths.count() == 2) {

        listRides(paths[0], request, response);
        return;

    } else if (paths.count() == 3) {

        QString athlete = paths[0];
        paths.removeFirst();

        // GET ACTIVITY
        if (paths[0] == "activity") {
            paths.removeFirst();
            listActivity(athlete, paths, request, response);
            return;
        }

        // GET MMP
        if (paths[0] == "meanmax") {
            paths.removeFirst();
            listMMP(athlete, paths, request, response);
            return;
        }
    }

    // GET HERE ITS BAD!
    response.setStatus(404); // malformed URL
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("malformed url");
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

    // honour the since parameter
    QString sincep(request->getParameter("since"));
    QDate since(1900,01,01);
    if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");

    // before parameter
    QString beforep(request->getParameter("before"));
    QDate before(3000,01,01);
    if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");

    // in range?
    if (item.dateTime.date() < since) return;
    if (item.dateTime.date() > before) return;

    // date, time, filename
    response->bwrite(item.dateTime.date().toString("yyyy/MM/dd").toLocal8Bit());
    response->bwrite(",");
    response->bwrite(item.dateTime.time().toString("hh:mm:ss").toLocal8Bit());;
    response->bwrite(",");
    response->bwrite(item.fileName.toLocal8Bit());

    if (wanted.count()) {
        // specific metrics
        foreach(int index, wanted) {
            double value = item.metrics()[index];
            response->bwrite(",");
            response->bwrite(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
        }
    } else {

        // all metrics...
        foreach(double value, item.metrics()) {
            response->bwrite(",");
            response->bwrite(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
        }
    }
    response->bwrite("\n");
}

void
APIWebService::listActivity(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    // does it exist ?
    QString filename = QString("%1/%2/activities/%3").arg(home.absolutePath()).arg(athlete).arg(paths[0]);

    QString contents;
    QFile file(filename);
    if (file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {

        // close as we will open properly below
        file.close();

        // what format to use ?
        QString format(request.getParameter("format"));
        if (format == "") {

            // if not passed in the URL then is content type
            // caller can accept listed in the header?
            // there is probably a more complete way of handling
            // wildcards etc, but the user can always force via     
            // the format parameter in the URL
            foreach(QByteArray accepts, request.getHeaders("Accept")) {
                if (accepts == "application/json") format="json";
                if (accepts == "text/csv") format="csv";
                if (accepts == "application/vnd.garmin.tcx") format="tcx";
                if (accepts == "application/vnd.trainingpeaks.pwx") format="pwx";
                if (accepts == "application/xml" || accepts == "text/xml") format="tcx";
                if (format != "") break;
            }
        }

        if (format == "") {

            // list activities and associated metrics
            response.setHeader("Content-Type", "application/json; charset=ISO-8859-1");

            // read in the whole thing
            QTextStream in(&file);
            // GC .JSON is stored in UTF-8 with BOM(Byte order mark) for identification
            in.setCodec ("UTF-8");
            contents = in.readAll();
            file.close();

            // write back in one hit
            response.write(contents.toLocal8Bit(), true);

        } else {

            // lets go with tcx/pwx as xml, full csv (not powertap) and GC json
            QStringList formats;
            formats << "tcx"; // garmin training centre
            formats << "csv"; // full csv list (not powertap)
            formats << "json"; // gc json
            formats << "pwx"; // gc json

            // unsupported format
            if (!formats.contains(format)) {
                response.setStatus(500);
                response.write("unsupported format; we support:");
                foreach(QString fmt, formats) {
                    response.write(" ");
                    response.write(fmt.toLocal8Bit());
                }
                response.write("\r\n");
                return;
            } else {

                // set the content type appropriately
                if (format == "tcx") response.setHeader("Content-Type", "application/vnd.garmin.tcx+xml; charset=ISO-8859-1");
                if (format == "csv") response.setHeader("Content-Type", "text/csv; charset=ISO-8859-1");
                if (format == "json") response.setHeader("Content-Type", "application/json; charset=ISO-8859-1");
                if (format == "pwx") response.setHeader("Content-Type", "application/vnd.trainingpeaks.pwx+xml; charset=ISO-8859-1");
            }

            // lets read the file in as a ridefile
            QStringList errors;
            RideFile *f = RideFileFactory::instance().openRideFile(NULL, file, errors);

            // error reading (!)
            if (f == NULL) {
                response.setStatus(500);
                foreach(QString error, errors) {
                    response.write(error.toLocal8Bit());
                    response.write("\r\n");
                }
                return;
            }

            // write out to a temporary file in
            // the format requested
            bool success;
            QTemporaryFile tempfile; // deletes file when goes out of scope
            QString tempname;
            if (tempfile.open()) tempname = tempfile.fileName();
            else {
                response.setStatus(500);
                response.write("error opening temporary file");
                return;
            }
            QFile out(tempname);

            if (format == "csv") {
                CsvFileReader writer;
                success = writer.writeRideFile(NULL, f, out, CsvFileReader::gc);
            } else {
                success = RideFileFactory::instance().writeRideFile(NULL, f, out, format);
            }

            if (success) {

                // read in the whole thing
                out.open(QFile::ReadOnly | QFile::Text);
                QTextStream in(&out);
                in.setCodec ("UTF-8");
                contents = in.readAll();
                out.close();

                // write back in one hit
                response.write(contents.toLocal8Bit(), true);
                return;

            } else {
                response.setStatus(500);
                response.write("unable to write output, internal error.\n");
                return;
            }
        }

    } else {

       // nope?
       response.setStatus(404);
       response.write("file not found");
       return;
    }
}

void
APIWebService::listMMP(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    QString filename=paths[0];
    QString CPXfilename = home.absolutePath() + "/" + athlete + "/cache/" + QFileInfo(filename).completeBaseName() + ".cpx";

    response.bwrite("secs, watts\n");
    if (QFileInfo(CPXfilename).exists()) {
        int secs=0;
        foreach(float value, RideFileCache::meanMaxFor(CPXfilename, RideFile::watts)) {
            if (secs >0) response.bwrite(QString("%1, %2\n").arg(secs).arg(value).toLocal8Bit());
            secs++;
        }
    }
    response.flush();
}
