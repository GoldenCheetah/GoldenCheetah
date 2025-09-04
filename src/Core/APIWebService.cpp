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

#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include "Measures.h"

#include <QTemporaryFile>
#include <QFile>

void
APIWebService::service(HttpRequest &request, HttpResponse &response)
{
    // remove trailing '/' from request, just to be consistent
    QString fullPath = request.getPath();
    while (fullPath.endsWith("/")) fullPath.chop(1);

    // get the paths, strip empty stuff
    QStringList paths = QString(request.getPath()).split("/");
    while (paths.count() && paths[paths.count()-1] == "") paths.removeLast();
    while (paths.count() && paths[0] == "") paths.removeFirst();

    // we don't have a fave icon
    if (paths.count() && paths[0] == "favicon.ico") return;

    // ROOT PATH RETURNS A LIST OF ATHLETES
    if (paths.count() == 0) {
        listAthletes(request, response); // return csv list of all athlete and their characteristics
        return;
    }

    // Call to retreive athlete data, downstream will resolve
    // which functions to call for different data requests
    athleteData(paths, request, response);
}

void
APIWebService::athleteData(QStringList &paths, HttpRequest &request, HttpResponse &response)
{

    // check we have an athlete and it is valid
    if (paths.count() == 0) {

        response.setStatus(404); // malformed URL
        response.setHeader("Content-Type", "text; charset=ISO-8859-1");
        response.write("missing athlete.");
        return;
    } else {
        QFile ridedb(home.absolutePath() + "/" + paths[0] + "/cache/rideDB.json");
        if (!ridedb.exists()) {
            response.setStatus(404); // malformed URL
            response.setHeader("Content-Type", "text; charset=ISO-8859-1");
            response.write("unknown athlete " + paths[0].toLocal8Bit());
            return;
        }
    }
    if (paths.count() == 1) {

        // LIST ACTIVITIES FOR ATHLETE
        // http://localhost:12021/athlete
        listRides(paths[0], request, response);
        return;

    } else if (paths.count() == 2) {

        QString athlete = paths[0];
        paths.removeFirst();

        // GET ZONES
        // http://localhost:12021/athlete/zones
        if (paths[0] == "zones") {
            listZones(athlete, paths, request, response);
            return;
        }

        // GET Measures
        if (paths[0] == "measures") {

            // http://localhost:12021/athlete/measures
            paths.removeFirst();
            listMeasures(athlete, paths, request, response);
            return;
        }

    } else if (paths.count() == 3) {

        QString athlete = paths[0];
        paths.removeFirst();

        // GET ACTIVITY
        // http://localhost:12021/athlete/activity/filename
        // optional query parameters:
        //      ?format=json    (default)
        //      ?format=<xx>    xx = one of (csv, tcx, pwx)
        if (paths[0] == "activity") {

            paths.removeFirst();
            listActivity(athlete, paths, request, response);
            return;
        }

        // GET MMP
        if (paths[0] == "meanmax") {

            // http://localhost:12021/athlete/meanmax/filename
            // optional query parameter:
            //    ?series=watts     (default)
            //    ?series=<xx>  xx= one of (cad, speed, vam, IsoPower, xPower, nm)
            // http://localhost:12021/athlete/meanmax/bests
            // optional query parameter:
            //    ?series=watts     (default)
            //    ?series=<xx>  xx=1 of (cad, speed, vam, IsoPower, xPower, nm)
            paths.removeFirst();
            listMMP(athlete, paths, request, response);
            return;
        }

        // GET Measures
        if (paths[0] == "measures") {

            // http://localhost:12021/athlete/measures/Body
            // http://localhost:12021/athlete/measures/Hrv
            paths.removeFirst();
            listMeasures(athlete, paths, request, response);
            return;
        }

     }

    // GET HERE ITS BAD!
    response.setStatus(404); // malformed URL
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("malformed url");
}

void
APIWebService::listAthletes(HttpRequest &, HttpResponse &response)
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
        if (QFile(ridedb).exists()) {
            // we need to initialize athlete settings for cvalue to work
            appsettings->initializeQSettingsAthlete(home.absolutePath(), name);
            if (appsettings->cvalue(name, GC_SEX, "") == "") continue;

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
APIWebService::writeRideLine(RideItem &item, HttpRequest *request, HttpResponse *response)
{

    // honour the since parameter
    QString sincep(request->getParameter("since"));
    QDate since(GC_EPOCH);
    if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");

    // before parameter
    QString beforep(request->getParameter("before"));
    QDate before(3000,01,01);
    if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");

    // in range?
    if (item.dateTime.date() < since) return;
    if (item.dateTime.date() > before) return;

    // are we doing rides or intervals?
    listRideSettings *settings = static_cast<listRideSettings *>(response->userData());

    if (settings->intervals == true) {

        // loop through all available intervals for this ride item
        foreach(IntervalItem *interval, item.intervals()){ 

            // date, time, filename
            response->bwrite(item.dateTime.date().toString("yyyy/MM/dd").toLocal8Bit());
            response->bwrite(", ");
            response->bwrite(item.dateTime.time().toString("hh:mm:ss").toLocal8Bit());;
            response->bwrite(", ");
            response->bwrite(item.fileName.toLocal8Bit());

            // now the interval name and type
            response->bwrite(", \"");
            response->bwrite(interval->name.toLocal8Bit());
            response->bwrite("\", ");
            response->bwrite(QString("%1").arg(static_cast<int>(interval->type)).toLocal8Bit());

            // essentially the same as below .. cut and paste (refactor?XXX)
            if (settings->wanted.count()) {
                // specific metrics
                foreach(int index, settings->wanted) {
                    double value = interval->metrics()[index];
                    response->bwrite(",");
                    response->bwrite(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
                }
            } else {
    
                // all metrics...
                foreach(double value, interval->metrics()) {
                    response->bwrite(",");
                    response->bwrite(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
                }
            }
            response->bwrite("\n");
        }

    } else {

        // date, time, filename
        response->bwrite(item.dateTime.date().toString("yyyy/MM/dd").toLocal8Bit());
        response->bwrite(",");
        response->bwrite(item.dateTime.time().toString("hh:mm:ss").toLocal8Bit());;
        response->bwrite(",");
        response->bwrite(item.fileName.toLocal8Bit());

        if (settings->wanted.count()) {
            // specific metrics
            foreach(int index, settings->wanted) {
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

        // all the metadata asked for
        foreach(QString name, settings->metawanted) {
            QString text = item.getText(name,"");
            text.replace("\"","'");   // don't use double quotes...
            text.replace("\n","\\n"); // newlines
            text.replace("\r","\\r"); // carriage returns
            text.replace("\t","\\t"); // tabs

            response->bwrite(",\"");
            response->bwrite(text.toLocal8Bit());
            response->bwrite("\"");
        }

        response->bwrite("\n");
    }
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

        // default to json
        if (format == "") format = "json";

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
#if QT_VERSION < 0x060000
            in.setCodec ("UTF-8");
#endif
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

    // what series do we want ?
    QString seriesp = request.getParameter("series");
    if (seriesp == "") seriesp = "watts";
    RideFile::SeriesType series;

    // what asked for ?
    if (seriesp == "hr") series = RideFile::hr;
    else if (seriesp == "cad") series = RideFile::cad;
    else if (seriesp == "speed") series = RideFile::kph;
    else if (seriesp == "watts") series = RideFile::watts;
    else if (seriesp == "vam") series = RideFile::vam;
    else if (seriesp == "IsoPower") series = RideFile::IsoPower;
    else if (seriesp == "xPower") series = RideFile::xPower;
    else if (seriesp == "nm") series = RideFile::nm;
    else {

        // unknown series
        response.setStatus(500);
        response.write("unknown series requested.\n");
        return;
    }

    QString filename=paths[0];

    if (paths[0] == "bests") {

        // header
        response.bwrite("secs, ");
        response.bwrite(seriesp.toLocal8Bit());
        response.bwrite("\n");

        // honour the since parameter
        QString sincep(request.getParameter("since"));
        QDate since(GC_EPOCH);
        if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");

        // before parameter
        QString beforep(request.getParameter("before"));
        QDate before(3000,01,01);
        if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");

        int secs=0;
        foreach(float value, RideFileCache::meanMaxFor(home.absolutePath() + "/" + athlete + "/cache", series, since, before)) {
            if (secs >0) response.bwrite(QString("%1, %2\n").arg(secs).arg(value).toLocal8Bit());
            secs++;
        }


    } else {
        QString CPXfilename = home.absolutePath() + "/" + athlete + "/cache/" + QFileInfo(filename).completeBaseName() + ".cpx";

        // header
        response.bwrite("secs, ");
        response.bwrite(seriesp.toLocal8Bit());
        response.bwrite("\n");

        if (QFileInfo(CPXfilename).exists()) {
            int secs=0;
            foreach(float value, RideFileCache::meanMaxFor(CPXfilename, series)) {
                if (secs >0) response.bwrite(QString("%1, %2\n").arg(secs).arg(value).toLocal8Bit());
                secs++;
            }
        }
        response.flush();
    }
}

void
APIWebService::listZones(QString athlete, QStringList, HttpRequest &request, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    // what zones we support
    QStringList zonelist;
    zonelist << "power" << "hr" << "pace" << "swimpace";

    // what series do we want ?
    QString zonesFor = request.getParameter("for");
    if (zonesFor == "") zonesFor = "power";
    else if (!zonelist.contains(zonesFor)) {
        response.setStatus(404);
        response.write("unknown zones type; one of power, hr, pace and swimpace expected.\n");
        return;
    }

    // power zones
    if (zonesFor == "power") {

        // Power Zones
        QFile zonesFile(home.absolutePath() + "/" + athlete + "/config/power.zones");
        if (zonesFile.exists()) {
            Zones *zones = new Zones;
            if (zones->read(zonesFile)) {

                // success - write out
                response.write("date, cp, w', pmax, aetp, ftp\n");
                for(int i=0; i<zones->getRangeSize(); i++) {
                    response.write(
                    QString("%1, %2, %3, %4, %5, %6\n")
                           .arg(zones->getStartDate(i).toString("yyyy/MM/dd"))
                           .arg(zones->getCP(i))
                           .arg(zones->getWprime(i))
                           .arg(zones->getPmax(i))
                           .arg(zones->getAeT(i))
                           .arg(zones->getFTP(i))
                           .toLocal8Bit()
                    );
                }
                return;
            }
        }

        // drop here on fail
        response.setStatus(500);
        response.write("unable to read/parse the athlete's power.zones file.\n");
        return;
    }

    // hr zones
    if (zonesFor == "hr") {

        // Zones
        QFile zonesFile(home.absolutePath() + "/" + athlete + "/config/hr.zones");
        if (zonesFile.exists()) {
            HrZones *zones = new HrZones;
            if (zones->read(zonesFile)) {

                // success - write out
                response.write("date, lthr, aethr, maxhr, rhr\n");
                for(int i=0; i<zones->getRangeSize(); i++) {
                    response.write(
                    QString("%1, %2, %3, %4, %5\n")
                           .arg(zones->getStartDate(i).toString("yyyy/MM/dd"))
                           .arg(zones->getLT(i))
                           .arg(zones->getAeT(i))
                           .arg(zones->getMaxHr(i))
                           .arg(zones->getRestHr(i))
                           .toLocal8Bit()
                    );
                }
                return;
            }
        }

        // drop here on fail
        response.setStatus(500);
        response.write("unable to read/parse the athlete's hr.zones file.\n");
        return;
    }

    // pace zones
    if (zonesFor == "pace") {

        // Zones
        QFile zonesFile(home.absolutePath() + "/" + athlete + "/config/run-pace.zones");
        if (zonesFile.exists()) {
            PaceZones *zones = new PaceZones;
            if (zones->read(zonesFile)) {

                // success - write out
                response.write("date, CV, AeTV\n");
                for(int i=0; i<zones->getRangeSize(); i++) {
                    response.write(
                    QString("%1, %2, %3\n")
                           .arg(zones->getStartDate(i).toString("yyyy/MM/dd"))
                           .arg(zones->getCV(i))
                           .arg(zones->getAeT(i))
                           .toLocal8Bit()
                    );
                }
                return;
            }
        }

        // drop here on fail
        response.setStatus(500);
        response.write("unable to read/parse the athlete's run-pace.zones file.\n");
        return;
    }

    // swim pace zones
    if (zonesFor == "swimpace") {

        // Zones
        QFile zonesFile(home.absolutePath() + "/" + athlete + "/config/swim-pace.zones");
        if (zonesFile.exists()) {
            PaceZones *zones = new PaceZones;
            if (zones->read(zonesFile)) {

                // success - write out
                response.write("date, CV, AeTV\n");
                for(int i=0; i<zones->getRangeSize(); i++) {
                    response.write(
                    QString("%1, %2, %3\n")
                           .arg(zones->getStartDate(i).toString("yyyy/MM/dd"))
                           .arg(zones->getCV(i))
                           .arg(zones->getAeT(i))
                           .toLocal8Bit()
                    );
                }
                return;
            }
        }

        // drop here on fail
        response.setStatus(500);
        response.write("unable to read/parse the athlete's swim-pace.zones file.\n");
        return;
    }
}

void
APIWebService::listMeasures(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    QDir configDir(home.absolutePath() + "/" + athlete + "/config");

    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    if (paths.isEmpty()) {

        foreach (QString group, Measures(configDir).getGroupSymbols()) {
            response.write(group.toLocal8Bit());
            response.write("\n");
        }
        return;
    }

    Measures measures = Measures(configDir, true);
    int group_index = measures.getGroupSymbols().indexOf(paths[0]);
    MeasuresGroup* measuresGroup = measures.getGroup(group_index);
    if (group_index < 0 || measuresGroup == NULL) {

        // unknown group
        response.setStatus(500);
        response.write("unknown measures group requested.\n");
        return;
    }

    response.write("Date");
    QStringList field_symbols = measuresGroup->getFieldSymbols();
    for (int i=0; i<field_symbols.count(); i++) {
        response.write(", ");
        response.write(field_symbols[i].toLocal8Bit());
    }

    // honour the since parameter
    QString sincep(request.getParameter("since"));
    QDate since(GC_EPOCH);
    if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");
    QDate date = measuresGroup->getStartDate();
    if (since > date) date = since;

    // before parameter
    QString beforep(request.getParameter("before"));
    QDate before(3000,01,01);
    if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");
    QDate endDate = measuresGroup->getEndDate();
    if (before < endDate) endDate = before;

    while (date <= endDate) {
        response.write("\n");
        response.write(date.toString("yyyy/MM/dd").toLocal8Bit());

        for (int i=0; i<field_symbols.count(); i++)
            response.write(QString(", %1").arg(measuresGroup->getFieldValue(date, i)).toLocal8Bit());

        date = date.addDays(1);
    }
    response.write("\n");

}
