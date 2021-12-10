/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "ErgFile.h"
#include "Athlete.h"

// Zwift XML handing
#include "ZwoParser.h"
#include <QFile>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

#include <stdint.h>
#include "Units.h"
#include "Utils.h"

#include "TTSReader.h"

// Supported file types
static QStringList supported;
static bool setSupported()
{
    ::supported << ".erg";
    ::supported << ".mrc";
    ::supported << ".crs";
    ::supported << ".pgmf";
    ::supported << ".zwo";
    ::supported << ".gpx";
    ::supported << ".tts";
    ::supported << ".json";

    return true;
}
static bool isinit = setSupported();
bool ErgFile::isWorkout(QString name)
{
    foreach(QString extension, supported) {
        if (name.endsWith(extension, Qt::CaseInsensitive))
            return true;
    }
    return false;
}
ErgFile::ErgFile(QString filename, int mode, Context *context) :
    filename(filename), mode(mode), StrictGradient(true), context(context)
{
    if (context->athlete->zones("Bike")) {
        int zonerange = context->athlete->zones("Bike")->whichRange(QDateTime::currentDateTime().date());
        if (zonerange >= 0) CP = context->athlete->zones("Bike")->getCP(zonerange);
    }
    reload();
}

ErgFile::ErgFile(Context *context) : mode(0), StrictGradient(true), context(context)
{
    if (context->athlete->zones("Bike")) {
        int zonerange = context->athlete->zones("Bike")->whichRange(QDateTime::currentDateTime().date());
        if (zonerange >= 0) CP = context->athlete->zones("Bike")->getCP(zonerange);
    } else {
        CP = 300;
    }
    filename ="";
}

void
ErgFile::setFrom(ErgFile *f)
{
    // don't rename it !
    QString filename=this->filename;
    QString Filename=this->filename;

    // take a copy
    *this = *f;

    // keep filename
    this->filename = filename;
    this->Filename = Filename;
}

ErgFile *
ErgFile::fromContent(QString contents, Context *context)
{
    ErgFile *p = new ErgFile(context);

    p->parseComputrainer(contents);
    p->finalize();

    return p;
}

ErgFile *
ErgFile::fromContent2(QString contents, Context *context)
{
    ErgFile *p = new ErgFile(context);

    p->parseErg2(contents);
    p->finalize();

    return p;
}

void ErgFile::reload()
{
    // All file endings that can be loaded as ergfile from rideFileFactory.
    // Actual permitted file types are controlled by ::supported list at
    // top of this file.
    QRegExp fact(".+[.](gpx|json)$", Qt::CaseInsensitive);

    // which parser to call? NOTE: we should look at moving to an ergfile factory
    // like we do with ride files if we end up with lots of different formats
    if      (filename.endsWith(".pgmf",   Qt::CaseInsensitive)) parseTacx();
    else if (filename.endsWith(".zwo",    Qt::CaseInsensitive)) parseZwift();
    else if (fact.exactMatch(filename))                         parseFromRideFileFactory();
    else if (filename.endsWith(".erg2",   Qt::CaseInsensitive)) parseErg2();
    else if (filename.endsWith(".tts",    Qt::CaseInsensitive)) parseTTS();
    else parseComputrainer();

    finalize();
}

void ErgFile::parseZwift()
{
    // Initialise
    Version = "";
    Units = "";
    Filename = "";
    Name = "";
    Duration = -1;
    Ftp = 0;       // FTP this file was targetted at
    MaxWatts = 0;  // maxWatts in this ergfile (scaling)
    valid = false; // did it parse ok?
    format = ERG;  // default to couse until we know
    Points.clear();
    Laps.clear();
    Texts.clear();

    // parse the file
    QFile zwo(filename);
    QXmlInputSource source(&zwo);
    QXmlSimpleReader xmlReader;
    ZwoParser handler;
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse(source);

    // save the metadata into our fields
    // we lose category and category index (for now)
    Name=handler.name;
    Description=handler.description;
    Source=handler.author;
    Tags=handler.tags;

    // extract contents into ErgFile....
    // each watts value is in percent terms so apply CP
    // and put into our format as integer
    foreach(ErgFilePoint p, handler.points) {
        double watts = int(p.y * CP / 100.0);
        Points << ErgFilePoint(p.x, watts, watts);
        if (watts > MaxWatts) MaxWatts = watts;
        Duration = p.x;
    }

    // texts
    Texts = handler.texts;

    if (Points.count()) {
        valid = true;
        Duration = Points.last().x;      // last is the end point in msecs

        // calculate climbing etc
        calculateMetrics();
    }
}

void ErgFile::parseErg2(QString p)
{
    QFile ergFile(filename);
    // Initialise
    Version = "";
    Units = "";
    Filename = "";
    Name = "";
    Duration = -1;
    Ftp = 0;       // FTP this file was targetted at
    MaxWatts = 0;  // maxWatts in this ergfile (scaling)
    valid = false; // did it parse ok?
    mode = format = ERG;  // default to couse until we know
    Points.clear();
    Laps.clear();
    Texts.clear();

    // open the file
    if (p == "" && ergFile.open(QIODevice::ReadOnly | QIODevice::Text) == false) {
        valid = false;
        return;
    }
    // ok. opened ok lets parse.
    QTextStream inputStream(&ergFile);
    QTextStream stringStream(&p);
    if (p != "") inputStream.setString(&p); // use a string not a file!

    QJsonParseError parseError;
    QByteArray byteArray = inputStream.readAll().toUtf8();
    QJsonDocument document = QJsonDocument::fromJson(byteArray, &parseError);

    if (parseError.error == QJsonParseError::NoError) {

        QJsonObject object = document.object();

        QJsonArray segments = object["segments"].toArray();

        int time = 0;
        if (segments.size()>0) {
            for(int i=0; i<segments.size(); i++) {
                QJsonArray segment = segments.at(i).toArray();

                ErgFilePoint add;
                add.x = time;
                add.val = add.y = int((segment.at(1).toDouble() /100.00) * CP);
                Points.append(add);

                time += segment.at(0).toDouble() * 60000;// from mins to 1000ths of a second
                add.x = time;
                add.val = add.y = int((segment.at(2).toDouble() /100.00) * CP);
                Points.append(add);
            }
            // set ErgFile duration
            Duration = Points.last().x;      // last is the end point in msecs

            calculateMetrics();
            valid = true;
        }
    }
}

void ErgFile::parseTacx()
{
    // Initialise
    Version = "";
    Units = "";
    Filename = "";
    Name = "";
    Duration = -1;
    Ftp = 0;        // FTP this file was targetted at
    MaxWatts = 0;   // maxWatts in this ergfile (scaling)
    valid = false;  // did it parse ok?
    format = CRS;   // default to couse until we know
    Points.clear();
    Laps.clear();
    Texts.clear();

    // running totals
    double rdist = 0; // running total for distance
    double ralt = 200; // always start at 200 meters just to prettify the graph

    // open the file for binary reading and open a datastream
    QFile pgmfFile(filename);
    if (pgmfFile.open(QIODevice::ReadOnly) == false) return;
    QDataStream input(&pgmfFile);
    input.setByteOrder(QDataStream::LittleEndian);
    input.setVersion(QDataStream::Qt_4_0); // 32 bit floats not 64 bit.

    bool happy = true; // are we ok to continue reading?

    //
    // BASIC DATA STRUCTURES
    //
    struct {
        uint16_t fingerprint;
        uint16_t version;
        uint32_t blocks;
    } header; // file header

    struct {
        uint16_t type;
        uint16_t version;
        uint32_t records;
        uint32_t recordSize;
    } info; // tells us what to read

    struct {
        quint32 checksum; // 4
        // we don't use an array for the filename since C++ arrays are prepended by a 16bit size
        quint8 name[34];
        qint32 wattSlopePulse; // 42
        qint32 timeDist; // 46
        double totalTimeDist; // 54
        double energyCons;  // 62
        float  altStart; // 66
        qint32 brakeCategory; // 70
    } general; // type 1010

    struct {
        float distance;
        float slope;
        float friction;
    } program; // type 1020

    //
    // FILE HEADER
    //
    int rc = input.readRawData((char*)&header, sizeof(header));
    if (rc == sizeof(header)) {
        if (header.fingerprint == 1000 && header.version == 100) happy = true;
        else happy = false;
    } else happy = false;

    unsigned int block = 0; // keep track of how many blocks we have read

    //
    // READ THE BLOCKS INSIDE THE FILE
    //
    while (happy && block < header.blocks) {

        // read the info for this block
        rc = input.readRawData((char*)&info, sizeof(info));
        if (rc == sizeof(info)) {

            // okay now read tha block
            switch (info.type) {

            case 1010 : // general
                {
                    // read it but mostly ignore -- for now
                    // we read member by member to avoid struct word alignment problem caused
                    // by the filename being 34 bytes long (why didn't they use 32 or 36?)
                    input>>general.checksum;
                    input.readRawData((char*)&general.name[0], 34);
                    input>>general.wattSlopePulse;
                    input>>general.timeDist;
                    input>>general.totalTimeDist;
                    input>>general.energyCons;
                    input>>general.altStart;
                    input>>general.brakeCategory;
                    switch (general.wattSlopePulse) {
                    case 0 :
                        mode = format = ERG;
                        break;
                    case 1 :
                        mode = format = CRS;
                        break;
                    default:
                        happy = false;
                        break;
                    }
                    ralt = general.altStart;
                }
                break;

            case 1020 : // program
                {
                    // read in the program records
                    for (unsigned int record=0; record < info.records; record++) {

                        // get the next record
                        if (sizeof(program) != input.readRawData((char*)&program, sizeof(program))) {
                            happy = false;
                            break;
                        }

                        ErgFilePoint add;

                        if (format == CRS) {
		                    // distance guff
                            add.x = rdist;
		                    double distance = program.distance; // in meters
		                    rdist += distance;

		                    // gradient and altitude
                            add.val = program.slope;
                            add.y = ralt;
		                    ralt += (distance * add.val) / 100;

                        } else {

                            add.x = rdist;
                            rdist += program.distance * 1000; // 1000ths of a second
                            add.val = add.y = program.slope; // its watts now
                        }

                        Points.append(add);
                        if (add.y > MaxWatts) MaxWatts=add.y;

                    }
                }
                break;

            default: // unexpected block type
                happy = false;
                break;
            }

            block++;

        } else happy = false;
    }

    // done
    pgmfFile.close();

    // if we got here and are still happy then it
    // must have been a valid file.
    if (happy) {
        // Set the end point for the workout
        ErgFilePoint end;
        end.x = rdist;
        end.y = Points.last().y;
        end.val = Points.last().val;
        Points.append(end);

        valid = true;

        // set ErgFile duration
        Duration = Points.last().x;      // last is the end point in msecs

        // calculate climbing etc
        calculateMetrics();
    }
}

void ErgFile::parseComputrainer(QString p)
{
    QFile ergFile(filename);
    int section = NOMANSLAND;            // section 0=init, 1=header data, 2=course data
    MaxWatts = Ftp = 0;
    int lapcounter = 0;
    format = ERG;                         // either ERG or MRC
    Points.clear();
    Texts.clear();

    // start by assuming the input file is Metric
    bool bIsMetric = true;

    // running totals for CRS file format
    double rdist = 0; // running total for distance
    double ralt = 200; // always start at 200 meters just to prettify the graph

    // open the file
    if (p == "" && ergFile.open(QIODevice::ReadOnly | QIODevice::Text) == false) {
        valid = false;
        return;
    }
    // Section markers
    QRegExp startHeader("^.*\\[COURSE HEADER\\].*$", Qt::CaseInsensitive);
    QRegExp endHeader("^.*\\[END COURSE HEADER\\].*$", Qt::CaseInsensitive);
    QRegExp startData("^.*\\[COURSE DATA\\].*$", Qt::CaseInsensitive);
    QRegExp endData("^.*\\[END COURSE DATA\\].*$", Qt::CaseInsensitive);
    QRegExp startText("^.*\\[COURSE TEXT\\].*$", Qt::CaseInsensitive);
    QRegExp endText("^.*\\[END COURSE TEXT\\].*$", Qt::CaseInsensitive);
    // ignore whitespace and support for ';' comments (a GC extension)
    QRegExp ignore("^(;.*|[ \\t\\n]*)$", Qt::CaseInsensitive);
    // workout settings
    QRegExp settings("^([^=]*)=[ \\t]*([^=\\n\\r\\t]*).*$", Qt::CaseInsensitive);

    // format setting for ergformat
    QRegExp ergformat("^[;]*(MINUTES[ \\t]+WATTS).*$", Qt::CaseInsensitive);
    QRegExp mrcformat("^[;]*(MINUTES[ \\t]+(PERCENT|FTP)).*$", Qt::CaseInsensitive);
    QRegExp crsformat("^[;]*(DISTANCE[ \\t]+GRADE[ \\t]+WIND).*$", Qt::CaseInsensitive);

    // time watts records
    QRegExp absoluteWatts("^[ \\t]*([0-9\\.]+)[ \\t]*([0-9\\.]+)[ \\t\\n]*$", Qt::CaseInsensitive);
    QRegExp relativeWatts("^[ \\t]*([0-9\\.]+)[ \\t]*([0-9\\.]+)%[ \\t\\n]*$", Qt::CaseInsensitive);

    // distance slope wind records
    QRegExp absoluteSlope("^[ \\t]*([0-9\\.]+)[ \\t]*([-0-9\\.]+)[ \\t\\n]*([-0-9\\.]+)[ \\t\\n]*$",
                           Qt::CaseInsensitive);

    // Lap marker in an ERG/MRC file
    QRegExp lapmarker("^[ \\t]*([0-9\\.]+)[ \\t]*LAP[ \\t\\n]*(.*)$", Qt::CaseInsensitive);
    QRegExp crslapmarker("^[ \\t]*LAP[ \\t\\n]*(.*)$", Qt::CaseInsensitive);

    // text cue records - try and be flexible about formatting:
    // sol <number> <whitespace> <text including whitespace and numbers> <whitespace> <number> <optional whitespace> eol
    //
    // there is no standard, so we need to try our best to accomodate different formats and trim
    // the text to remove any whitespace before and after it when displaying to the user
    //
    // since we only match with this expression when in text section there is no need to worry
    // about it clashing with any other rows which helps.
    QRegExp textCue("^([0-9\\.]+)[ \\t]*(.*)[ \\t]([0-9]+)[ \\t\\n]*$", Qt::CaseInsensitive);

    // ok. opened ok lets parse.
    QTextStream inputStream(&ergFile);
    QTextStream stringStream(&p);
    if (p != "") inputStream.setString(&p); // use a string not a file!
    while ((p=="" && !inputStream.atEnd()) || (p!="" && !stringStream.atEnd())) {

        // Code plagiarised from CsvRideFile.
        // the readLine() method doesn't handle old Macintosh CR line endings
        // this workaround will load the entire file if it has CR endings
        // then split and loop through each line
        // otherwise, there will be nothing to split and it will read each line as expected.
        QString linesIn = (p != "" ? stringStream.readLine() : ergFile.readLine());
        QStringList lines = linesIn.split('\r');

        // workaround for empty lines
        if(lines.isEmpty()) {
            continue;
        }

        for (int li = 0; li < lines.size(); ++li) {
            QString line = lines[li];

            // so what we go then?
            if (startHeader.exactMatch(line)) {
                section = SETTINGS;
            } else if (endHeader.exactMatch(line)) {
                section = NOMANSLAND;
            } else if (startData.exactMatch(line)) {
                section = DATA;
            } else if (endData.exactMatch(line)) {
                section = END;
            } else if (startText.exactMatch(line)) {
                section = TEXTS;
            } else if (endText.exactMatch(line)) {
                section = END;
            } else if (ergformat.exactMatch(line)) {
                // save away the format
                mode = format = ERG;
            } else if (mrcformat.exactMatch(line)) {
                // save away the format
                mode = format = MRC;
            } else if (crsformat.exactMatch(line)) {
                // save away the format
                mode = format = CRS;
            } else if (lapmarker.exactMatch(line)) {
                // lap marker found
                ErgFileLap add;

                add.x = lapmarker.cap(1).toDouble() * 60000; // from mins to 1000ths of a second
                add.LapNum = ++lapcounter;
                add.selected =false;
                add.name = lapmarker.cap(2).simplified();
                Laps.append(add);

            } else if (crslapmarker.exactMatch(line)) {
                // new distance lapmarker
                ErgFileLap add;

                add.x = rdist;
                add.LapNum = ++lapcounter;
                add.selected =false;
                add.name = lapmarker.cap(2).simplified();
                Laps.append(add);

            } else if (settings.exactMatch(line)) {
                // we have name = value setting
                QRegExp pversion("^VERSION *", Qt::CaseInsensitive);
                if (pversion.exactMatch(settings.cap(1))) Version = settings.cap(2);

                QRegExp pfilename("^FILE NAME *", Qt::CaseInsensitive);
                if (pfilename.exactMatch(settings.cap(1))) Filename = settings.cap(2);

                QRegExp pname("^DESCRIPTION *", Qt::CaseInsensitive);
                if (pname.exactMatch(settings.cap(1))) Name = settings.cap(2);

                QRegExp iname("^ERGDBID *", Qt::CaseInsensitive);
                if (iname.exactMatch(settings.cap(1))) ErgDBId = settings.cap(2);

                QRegExp sname("^SOURCE *", Qt::CaseInsensitive);
                if (sname.exactMatch(settings.cap(1))) Source = settings.cap(2);

                QRegExp punit("^UNITS *", Qt::CaseInsensitive);
                if (punit.exactMatch(settings.cap(1))) {
                    Units = settings.cap(2);
                    // UNITS can be ENGLISH or METRIC (miles/km)
                    QRegExp penglish(" ENGLISH$", Qt::CaseInsensitive);
                    if (penglish.exactMatch(Units)) { // Units <> METRIC
                      //qDebug("Setting conversion to ENGLISH");
                      bIsMetric = false;
                    }
                  }

                QRegExp pftp("^FTP *", Qt::CaseInsensitive);
                if (pftp.exactMatch(settings.cap(1))) Ftp = settings.cap(2).toInt();

            } else if (absoluteWatts.exactMatch(line)) {
                // we have mins watts line
                ErgFilePoint add;

                add.x = absoluteWatts.cap(1).toDouble() * 60000; // from mins to 1000ths of a second
                add.val = add.y = round(absoluteWatts.cap(2).toDouble());             // plain watts

                switch (format) {

                case ERG:       // its an absolute wattage
                    if (Ftp) { // adjust if target FTP is set.
                        // if ftp is set then convert to the users CP

                        double watts = add.y;
                        double ftp = Ftp;
                        watts *= CP/ftp;
                        add.y = add.val = int(watts);
                    }
                    break;
                case MRC:       // its a percent relative to CP (mrc file)
                    add.y *= CP;
                    add.y /= 100.00;
                    add.val = add.y = int(add.y);
                    break;
                }
                Points.append(add);
                if (add.y > MaxWatts) MaxWatts=add.y;

            } else if (relativeWatts.exactMatch(line)) {

                // we have a relative watts match
                ErgFilePoint add;
                add.x = relativeWatts.cap(1).toDouble() * 60000; // from mins to 1000ths of a second
                add.val = add.y = int((relativeWatts.cap(2).toDouble() /100.00) * CP);
                Points.append(add);
                if (add.y > MaxWatts) MaxWatts=add.y;

            } else if (absoluteSlope.exactMatch(line)) {
                // dist, grade, wind strength
                ErgFilePoint add;

                // distance guff
                add.x = rdist;
                double distance = absoluteSlope.cap(1).toDouble() * 1000.; // convert to meters

                if (!bIsMetric) distance *= KM_PER_MILE;
                rdist += distance;

                // gradient and altitude
                add.val = absoluteSlope.cap(2).toDouble();
                add.y = ralt;
                ralt += distance * add.val / 100.; /* paused */

                Points.append(add);
                if (add.y > MaxWatts) MaxWatts=add.y;


            } else if (ignore.exactMatch(line)) {
                // do nothing for this line

            } else if (section == TEXTS && textCue.exactMatch(line)) {
                // x, text cue, duration
                double x = textCue.cap(1).toDouble() * 1000.; // convert to msecs or m
                int duration = textCue.cap(3).toInt(); // duration in secs
                Texts<<ErgFileText(x, duration, textCue.cap(2).trimmed());
            } else {
              // ignore bad lines for now. just bark.
              //qDebug()<<"huh?" << line;
            }

        }

    }

    // done.
    if (p=="") ergFile.close();

    if (section == END && Points.count() > 0) {
        valid = true;

        if (mode == CRS) {
            // Add the last point for a crs file
            ErgFilePoint add;

            add.x = rdist;
            add.val = 0.0;
            add.y = ralt;
            Points.append(add);
            if (add.y > MaxWatts) MaxWatts=add.y;

            // Add a final meta-lap at the end - causes the system to correctly mark off laps.
            // An improvement would be to check for a lap marker at end, and only add this if required.
            ErgFileLap lap;

            lap.x = rdist;
            lap.LapNum = ++lapcounter;
            lap.selected = false;
            lap.name = lapmarker.cap(2).simplified();
            Laps.append(lap);
        }

        // add a start point if it doesn't exist
        if (Points.at(0).x > 0) {
            ErgFilePoint add;

            add.x = 0;
            add.y = Points.at(0).y;
            add.val = Points.at(0).val;
            Points.insert(0, add);
        }

        // set ErgFile duration
        Duration = Points.last().x;      // last is the end point in msecs

        calculateMetrics();

    } else {
        valid = false;
    }
}

// Parse anything that the ridefile factory knows how to load
// and which contains gps data, altitude or slope.
//
// File types supported:
// .gpx     GPS Track
// .json    GoldenCheetah JSON
void ErgFile::parseFromRideFileFactory()
{
    // Initialise
    Version = "";
    Units = "";
    Filename = "";
    Name = "";
    Duration = -1;
    Ftp = 0;       // FTP this file was targetted at
    MaxWatts = 0;  // maxWatts in this ergfile (scaling)
    valid = false; // did it parse ok?
    format = mode = CRS;
    Points.clear();
    Laps.clear();
    Texts.clear();

    // TTS File Gradient Should be smoothly interpolated from Altitude.
    StrictGradient = false;

    static double km = 0;

    QFile gpxFile(filename);

    // check file exists
    if (!gpxFile.exists()) {
        valid = false;
        return;
    }

    // instantiate ride
    QStringList errors_;
    RideFile* ride = RideFileFactory::instance().openRideFile(context, gpxFile, errors_);
    if (ride == NULL) {
        valid = false;
        return;
    }

    // Enumerate the data types that are available from this gpx.
    bool fHasKm    = ride->areDataPresent()->km;
    bool fHasLat   = ride->areDataPresent()->lat;
    bool fHasLon   = ride->areDataPresent()->lon;
    bool fHasGPS   = fHasLat && fHasLon;
    bool fHasAlt   = ride->areDataPresent()->alt;
    bool fHasSlope = ride->areDataPresent()->slope;

    if (fHasKm && fHasSlope) {}     // same as crs file
    else if (fHasKm && fHasAlt) {}  // derive slope from distance and alt
    else {
        valid = false;
        return;
    }

    int pointCount = ride->dataPoints().count();

    RideFilePoint* prevPoint = NULL;
    RideFilePoint* point = NULL;
    RideFilePoint* nextPoint = (ride->dataPoints()[0]);
    double alt = 0;
    for (int i = 0; i < pointCount; i++) {
        ErgFilePoint add;

        prevPoint = point;
        point = nextPoint;

        if (!point)
            break;

        nextPoint = ((i + 1) < pointCount) ? (ride->dataPoints()[i + 1]) : NULL;

        // Determine slope to next point
        double slope = 0.0;

        if (fHasAlt)
            alt = point->alt;

        if (fHasSlope) {
            slope = point->slope;

            if (!fHasAlt && prevPoint)
            {
                alt += ((slope * (point->km - prevPoint->km))) / 100.0;
            }

        } else if (nextPoint && fHasAlt)
        {
            double km0 = point->km;
            double alt0 = point->alt / 1000;

            double km1 = nextPoint->km;
            double alt1 = nextPoint->alt / 1000;

            double rise = alt1 - alt0;
            double h = km1 - km0;

            slope = 100 * (rise / (sqrt(h * h - rise * rise)));
        }

        add.x   = 1000 * point->km; // record distance in meters
        add.y   = alt;
        add.val = slope;

        if (fHasGPS) {
            add.lat = point->lat;
            add.lon = point->lon;
        }

        if (add.y > MaxWatts) MaxWatts = add.y;

        Points.append(add);
    }

    // Add intervals as lap markers and text cues
    int i = 0;
    foreach(const RideFileInterval* lap, ride->intervals()) {
        double x = ride->timeToDistance(lap->start) * 1000.0;
        double y = ride->timeToDistance(lap->stop) * 1000.0;
        int duration = lap->stop - lap->start + 1;
        if (x > 0 && duration > 0 && !lap->name.isEmpty()) {
            // In order to support navigation, each interval is converted into
            // two laps that are linked by sharing a non-zero group id.
            Laps<<ErgFileLap(x, 2*i+1, i+1, lap->name);
            Laps<<ErgFileLap(y, 2*i+2, i+1, "");
            i++;
            Texts<<ErgFileText(x, duration, lap->name);
        }
    }

    gpxFile.close();

    valid = true;

    // set ErgFile duration
    Duration = Points.last().x;      // end point in meters

    // calculate climbing etc
    calculateMetrics();
}

// Parse TTS into ergfile
void ErgFile::parseTTS()
{
    // Initialise
    Version = "";
    Units = "";
    Filename = "";
    Name = "";
    Duration = -1;
    Ftp = 0;       // FTP this file was targetted at
    MaxWatts = 0;  // maxWatts in this ergfile (scaling)
    valid = false; // did it parse ok?
    format = mode = CRS;
    Points.clear();
    Laps.clear();
    Texts.clear();

    // TTS File Gradient Should be smoothly interpolated from Altitude.
    StrictGradient = false;

    QFile ttsFile(filename);
    if (ttsFile.open(QIODevice::ReadOnly) == false) {
        valid = false;
       return;
    }
    QStringList errors_;

    QDataStream qttsStream(&ttsFile);
    qttsStream.setByteOrder(QDataStream::LittleEndian);

    NS_TTSReader::TTSReader ttsReader;
    bool success = ttsReader.parseFile(qttsStream);
    if (!success) {
        valid = false;
        return;
    }

    const std::vector<NS_TTSReader::Point> &ttsPoints = ttsReader.getPoints();
    int pointCount = (int)ttsPoints.size();
    if (pointCount < 2) {
        valid = false;
        return;
    }

    // Enumerate the data types that are available.
    bool fHasKm    = ttsReader.hasKm();
    bool fHasGPS   = ttsReader.hasGPS();
    bool fHasAlt   = ttsReader.hasElevation();
    bool fHasSlope = ttsReader.hasGradient();

    if (fHasKm && fHasSlope) {}     // same as crs file
    else if (fHasKm && fHasAlt) {}  // derive slope from distance and alt
    else {
        valid = false;
        return;
    }

    Name = QString::fromStdWString(ttsReader.getRouteName());               // description in file
    Description = QString::fromStdWString(ttsReader.getRouteDescription()); // long narrative for workout

    const NS_TTSReader::Point *prevPoint, *point, *nextPoint;

    prevPoint = NULL;
    point = NULL;
    nextPoint = &ttsPoints[0];

    double alt = 0;
    if (fHasAlt) {
        alt = ttsPoints[0].getElevation();
    }

    for (int i = 0; i < pointCount; i++) { // first data point is actually the last...
        ErgFilePoint add;

        prevPoint = point;
        point = nextPoint;

        if (!point)
            break;

        nextPoint = ((i + 1) >= pointCount) ? &ttsPoints[i] : &ttsPoints[i + 1];

        // Determine slope to next point
        double slope = 0.0;

        if (fHasAlt) {
            alt = point->getElevation();
        }

        if (fHasSlope) {
            slope = point->getGradient();
            if (!fHasAlt && prevPoint)
            {
                alt += ((slope * (point->getDistanceFromStart() - prevPoint->getDistanceFromStart()))) / 100.0;
            }
        } else if (nextPoint && fHasAlt) {
            double km0 = (i == 0) ? 0 : point->getDistanceFromStart(); // first entry holds total distance
            double alt0 = point->getElevation();
        
            double km1 = nextPoint->getDistanceFromStart();
            double alt1 = nextPoint->getElevation();
        
            double rise = alt1 - alt0;
            double h = km1 - km0;

            slope = 100 * (rise / (sqrt(h*h - rise * rise)));
        }

        add.x = point->getDistanceFromStart(); // record distance in meters
        add.y = alt;
        add.val = slope;

        if (fHasGPS) {
            add.lat = point->getLatitude();
            add.lon = point->getLongitude();
        }

        if (add.y > MaxWatts) MaxWatts = add.y;

        Points.append(add);
    }

    // There is an outlier near the end of T1956.54 Alpine Classic I that will
    // cause 4.01m of videosync skew because it incorrectly extends max ride
    // distance. This is caused because an outlier point near the end has a
    // distance that exceeds the final route distance.
    //
    // 1
    // 1.3
    // 7    <--- Outlier
    // 1.6
    // 1.8
    // 2
    // -- end --
    //
    // When an outlier distance appears towards the end it can skew all of videosync,
    // so its important to remove those points. In above case route distance would be
    // seen as 7 but videosync intended max distance to be 1.3. If the '7' point is
    // left in the entire videosync will be skewed by +5m.
    //
    // There are lots of complicated ways to remediate this issue but none
    // are perfect.
    //
    // Approach taken here is to identify and rewrite distance of all single point
    // outliers.
    //
    // We consider two sorts of outlier:
    //
    //   Negative Outlier:
    //
    //   A negative outlier is a single point that is less than both its successor
    //   and its predecessor.
    //
    //   1
    //   2
    //   0 <- negative outlier
    //   4
    //   5
    //
    //   Postive outlier:
    //
    //   A positive outlier is a single point that is higher than its predecessor and
    //   its successor, and the successor is greater than the predecessor.
    //
    //   1
    //   2
    //   5 <- positive outlier
    //   4
    //   6
    //
    //   These two cases occur on tacx tts files and can be easily and cleanly handled - so we do.

    double predDistance = Points[0].x;
    double curDistance = Points[1].x;
    double nextDistance;
    int count = Points.size();
    for (int i = 1; i < count - 1; i++) {
        nextDistance = Points[i + 1].x;

        bool fIsNegativeOutlier = curDistance < predDistance && predDistance < nextDistance;
        bool fIsPositiveOutlier = nextDistance < curDistance&& curDistance > predDistance && predDistance <= nextDistance;

        if (fIsNegativeOutlier || fIsPositiveOutlier) {

            double newDistance = (nextDistance + predDistance) / 2.;

            qDebug() << curDistance << (fIsNegativeOutlier ? ": NegativeOutlier " : ": PositiveOutlier ") << "rewriting to " << newDistance;

            curDistance = newDistance;

            Points[i].x = curDistance;
        }

        predDistance = curDistance;
        curDistance = nextDistance;
    }

    // Altitude is interpolated if there is slope. Actually this slope is
    // smooth so we might consider it usable (even though its wrong.)
    if (fHasSlope) {
        fHasAlt = true;
    }

    // Populate lap and text lists with route and segment info from tts.
    // Push everything in segment order, then sort by location.
    std::vector<NS_TTSReader::Segment> segments = ttsReader.getSegments();
    int segmentCount = (int)segments.size();
    if (segmentCount) {
        std::sort(segments.begin(), segments.end(), [](const NS_TTSReader::Segment& a, const NS_TTSReader::Segment& b) {
            // Segment sort order:
            // 1) Start Distance, first comes first
            // 2) Segment Distance, longest comes first
            if (a.startKM != b.startKM) return a.startKM < b.startKM;
            return (a.endKM - a.startKM) > (b.endKM - b.startKM);
        });

        // Now segments are sorted by start and longer duration.

        for (int i = 0; i < segmentCount; i++) {
            const NS_TTSReader::Segment& segment = segments[i];

            // Truncate distances to meter precision for text name.
            int segmentStart = (int)(segment.startKM * 1000);
            int segmentEnd = (int)(segment.endKM * 1000);

            std::wstring rangeString = L" ["
                + std::to_wstring(segmentStart)
                + L"m ->"
                + std::to_wstring(segmentEnd)
                + L"m]";

            // Populate Texts with segment text.
            if (segment.name.length() + segment.description.length() > 0) {
                QString text(QString::fromStdWString(
                    segment.name + L": " +
                    segment.description +
                    rangeString));

                double x = segment.startKM * 1000.;
                int duration = (segment.endKM - segment.startKM) * 1000.;
                Texts << ErgFileText(x, duration, text);
            }

            // Populate Laps with segment info.
            // In order to support navigation, each segment is converted into
            // two laps that are linked by sharing a non-zero group id.
            Laps << ErgFileLap(segment.startKM * 1000., (2 * i) + 1, i + 1,
                QString::fromStdWString(segment.name));

            Laps << ErgFileLap(segment.endKM * 1000, (2 * i) + 2, i + 1, "");
        }
    }

    // The following sort preserves segment overlap. If A is a superset of B then order will be:
    // - start A
    // - start B
    // - end B
    // - end A
    //
    // For cases of partial overlap of A then B it will do:
    // - Start A
    // - Start B
    // - End A
    // - End B

    // Sort laps and texts
    sortLaps();
    sortTexts();

    // This is load time so debug print isn't so expensive.
    // Laps seem pretty buggy right now. Lets debug print the data
    // to help people figure out what is happening.
    int xx = 0;
    qDebug() << "LAPS:";
    for (const auto &a : Laps) {
        qDebug() << xx << ": " << a.LapNum << " start:" << a.x / 1000. << "km, rangeId: " << a.lapRangeId << "km, name: " << a.name;
        xx++;
    }

    qDebug() << "TEXTS:";
    xx = 0;
    for (const auto &a : Texts) {
        qDebug() << xx << ": " << a.x / 1000. << " duration: " << a.duration << " text:" << a.text;
        xx++;
    }

    ttsFile.close();

    valid = true;

    // set ErgFile duration
    Duration = Points.last().x;      // end point in meters

    // calculate climbing etc
    calculateMetrics();
}


// convert points to set of sections
QList<ErgFileSection>
ErgFile::Sections()
{
    QList<ErgFileSection> returning;

    int secs=0;
    for(int i=0; i<Points.count(); i++) {

        // add a section from 0 to here if not starting at zero
        if (i==0 && Points[i].x > 0) {
            returning << ErgFileSection(Points[i].x, Points[i].y, Points[i].y);
        }

        // first section
        if (i+1 < Points.count() && Points[i+1].x > Points[i].x) {
            returning << ErgFileSection(Points[i+1].x-secs, Points[i].y, Points[i+1].y);
            secs= Points[i+1].x;
        }
    }
    return returning;
}

bool
ErgFile::save(QStringList &errors)
{
    // save the file including changes
    // XXX TODO we don't support writing pgmf or CRS just yet...
    if (filename.endsWith("pgmf", Qt::CaseInsensitive) || format == CRS) {
        errors << QString(QObject::tr("Unsupported file format"));
        return false;
    }

    // type
    QString typestring("UNKNOWN");
    if (format==ERG) typestring = "ERG";
    if (format==MRC) typestring = "MRC";
    if (format==CRS) typestring = "CRS";
    if (filename.endsWith("zwo", Qt::CaseInsensitive)) typestring="ZWO";

    // get CP so we can scale back etc
    int CP=0;
    if (context->athlete->zones("Bike")) {
        int zonerange = context->athlete->zones("Bike")->whichRange(QDateTime::currentDateTime().date());
        if (zonerange >= 0) CP = context->athlete->zones("Bike")->getCP(zonerange);
    }

    // MRC and ERG formats are ok

    //
    // ERG file
    //
    if (typestring == "ERG" && format == ERG) {

        // open the file etc
        QFile f(filename);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) == false) {
            errors << "Unable to open file for writing.";
            return false;
        }

        // setup output stream to file
        QTextStream out(&f);
        out.setCodec("UTF-8");

        // write the header
        //
        // An example header:
        //
        // [COURSE HEADER]
        // FTP=300
        // SOURCE=ErgDB
        // ERGDBID=455
        // VERSION=2
        // UNITS=ENGLISH
        // DESCRIPTION=Weston Loop
        // FILE NAME=weston
        // MINUTES  WATTS
        // [END COURSE HEADER]
        out << "[COURSE HEADER]\n";
        if (Ftp) out <<"FTP="<<QString("%1").arg(Ftp)<<"\n";
        if (Source != "") out<<"SOURCE="<<Source<<"\n";
        if (ErgDBId != "") out<<"ERGDBID="<<ErgDBId<<"\n";
        if (Version != "") out<<"VERSION="<<Version<<"\n";
        if (Units != "") out<<"UNITS="<<Units<<"\n";
        if (Name != "") out<<"DESCRIPTION="<<Name<<"\n";
        if (Filename != "") out<<"FILE NAME="<<Filename<<"\n";
        out << "MINUTES WATTS\n";
        out << "[END COURSE HEADER]\n";

        //
        // Write the line items
        //
        // IMPORTANT: if FTP is set then the contents
        //            were scaled to local FTP when read
        //            so need to be scaled back to FTP=xxx
        //            when writing out.
        out << "[COURSE DATA]\n";

        bool first=true;
        long lastLap = 0;
        double lastPointX = 0;

        foreach(ErgFilePoint p, Points) {

            // output
            int watts = p.y;
            double minutes = double(p.x) / (1000.0f*60.0f);

            // we scale back if needed
            if (Ftp && CP) watts = (double(p.y)/CP) * Ftp;

            // check if a lap marker should be inserted
            foreach(ErgFileLap l, Laps) {
                bool addLap = false;
                if (l.x == p.x && lastLap != l.x) {
                    // this is the case where a lap marker matches a load marker
                    addLap = true;
                } else if (l.x > lastPointX && l.x < p.x) {
                    // this is the case when a lap marker exist between two load markers
                    addLap = true;
                }
                if (addLap) {
                    out <<QString("%1    LAP\n").arg(minutes, 0, 'f', 2);
                    lastLap = l.x;
                }
            }

            if (first) {
                first=false;

                // doesn't start at 0!
                if (p.x > 0) {
                    out <<QString("%1    %2\n").arg(0.0f, 0, 'f', 2).arg(watts);
                }
            }

            // output in minutes and watts
            out <<QString("%1    %2\n").arg(minutes, 0, 'f', 2).arg(watts);

            lastPointX = p.x;
        }

        out << "[END COURSE DATA]\n";

        // TEXTS in TrainerRoad compatible format
        if (Texts.count() > 0) {
            out << "[COURSE TEXT]\n";
            foreach(ErgFileText cue, Texts)
                out <<QString("%1\t%2\t%3\n").arg(cue.x/1000)
                                             .arg(cue.text)
                                             .arg(cue.duration);
            out << "[END COURSE TEXT]\n";
        }

        f.close();

    }

    //
    // MRC
    //
    if (typestring == "MRC" && format == MRC) {

        // open the file etc
        QFile f(filename);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) == false) {
            errors << "Unable to open file for writing.";
            return false;
        }

        // setup output stream to file
        QTextStream out(&f);
        out.setCodec("UTF-8");

        // write the header
        //
        // An example header:
        //
        // [COURSE HEADER]
        // FTP=300
        // SOURCE=ErgDB
        // ERGDBID=455
        // VERSION=2
        // UNITS=ENGLISH
        // DESCRIPTION=Weston Loop
        // FILE NAME=weston
        // MINUTES  WATTS
        // [END COURSE HEADER]
        out << "[COURSE HEADER]\n";
        if (Source != "") out<<"SOURCE="<<Source<<"\n";
        if (ErgDBId != "") out<<"ERGDBID="<<ErgDBId<<"\n";
        if (Version != "") out<<"VERSION="<<Version<<"\n";
        if (Units != "") out<<"UNITS="<<Units<<"\n";
        if (Name != "") out<<"DESCRIPTION="<<Name<<"\n";
        if (Filename != "") out<<"FILE NAME="<<Filename<<"\n";
        out << "MINUTES PERCENT\n";
        out << "[END COURSE HEADER]\n";

        //
        // Write the line items
        //
        // IMPORTANT: if FTP is set then the contents
        //            were scaled to local FTP when read
        //            so need to be scaled back to FTP=xxx
        //            when writing out.
        out << "[COURSE DATA]\n";

        bool first=true;
        long lastLap = 0;
        double lastPointX = 0;
        foreach(ErgFilePoint p, Points) {

            // output watts as a percent of CP
            double watts = p.y;
            double minutes = double(p.x) / (1000.0f*60.0f);

            // we scale back if needed
            if (CP) watts = (double(p.y)/CP) * 100.0f;

            // check if a lap marker should be inserted
            foreach(ErgFileLap l, Laps) {
                bool addLap = false;
                if (l.x == p.x && lastLap != l.x) {
                    // this is the case where a lap marker matches a load marker
                    addLap = true;
                } else if (l.x > lastPointX && l.x < p.x) {
                    // this is the case when a lap marker exist between two load markers
                    addLap = true;
                }
                if (addLap) {
                    out <<QString("%1    LAP\n").arg(minutes, 0, 'f', 2);
                    lastLap = l.x;
                }
            }

            if (first) {
                first=false;

                // doesn't start at 0!
                if (p.x > 0) {
                    out <<QString("%1    %2\n").arg(0.0f, 0, 'f', 2).arg(watts, 0, 'f', 0);
                }
            }

            // output in minutes and watts percent with no precision
            out <<QString("%1    %2\n").arg(minutes, 0, 'f', 2).arg(watts, 0, 'f', 0);

            lastPointX = p.x;
        }

        out << "[END COURSE DATA]\n";

        // TEXTS in TrainerRoad compatible format
        if (Texts.count() > 0) {
            out << "[COURSE TEXT]\n";
            foreach(ErgFileText cue, Texts)
                out <<QString("%1\t%2\t%3\n").arg(cue.x/1000)
                                             .arg(cue.text)
                                             .arg(cue.duration);
            out << "[END COURSE TEXT]\n";
        }

        f.close();

    }

    if (typestring == "ZWO" && format == ERG) {

        // open the file etc
        QFile f(filename);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) == false) {
            errors << "Unable to open file for writing.";
            return false;
        }

        // setup output stream to file
        QTextStream out(&f);
        out.setCodec("UTF-8");

        out << "<workout_file>\n";

        // metadata at top
        if (Name != "") out << "    <name>"<<Utils::xmlprotect(Name)<<"</name>\n";
        if (Source != "") out << "    <author>"<<Utils::xmlprotect(Source)<<"</author>\n";
        if (Description != "") out << "    <description>"<<Utils::xmlprotect(Description)<<"</description>\n";
        if (Tags.count()) {
            out << "    <tags>\n";
            foreach(QString tag, Tags)
                out << "        <tag>"<<Utils::xmlprotect(tag)<<"</tag>\n";
            out << "    </tags>\n";
        }

        // workout
        // We can write three types of sections
        // a) Warmup where power rises
        // b) SteadyState where power is constant
        // c) Cooldown where port drops
        // d) IntervalsT where intervals are repeated (N x effort then recovery)
        //
        // we do not write the fifth kind of section since we don't support them
        // e) Freeride where the user can do whatever they please...
        //
        // interspersed with the data will be texts that are displayed
        //
        // alll watts are factors to apply to CP - where 1.0 is CP 0.5 is 50%.
        // the data in memory has been scaled to CP so we need to divide it by
        // CP to get the values to write to disk

        // lets work in sections not points
        // this means we work with duration/power rather than
        // individual points, which is how the ZWO file is constructed

        out << "    <workout>\n";
        QList<ErgFileSection> sections = Sections();
        int msecs = 0;
        for(int i=0; i<sections.count(); i++) {

            // are there repeated sections of efforts and recovery?
            int count=0;
            for(int j=2; (i+j+1) < sections.count(); j += 2) {
                if (sections[i].duration == sections[i+j].duration &&
                    sections[i].start == sections[i+j].start &&
                    sections[i].end == sections[i+j].end &&
                    sections[i+1].duration == sections[i+j+1].duration &&
                    sections[i+1].start == sections[i+j+1].start &&
                    sections[i+1].end == sections[i+j+1].end) {
                    count++;
                } else {
                    break;
                }
            }

            if (count) {

                // not including this one there are a number of repeats, so we need to output
                // an IntervalsT element to repeat on/off intervals
                out << "        <IntervalsT Repeat=\""<<(count+1)<<"\" "
                    << "OnDuration=\"" << sections[i].duration/1000 << "\" "
                    << "OffDuration=\"" << sections[i+1].duration/1000 << "\" "
                    << "PowerOnLow=\"" << sections[i].start/CP << "\" "
                    << "PowerOnHigh=\"" << sections[i].end/CP << "\" "
                    << "PowerOffLow=\"" << sections[i+1].start/CP << "\" "
                    << "PowerOffHigh=\"" << sections[i+1].end/CP << "\" ";

                int d = (count+1)*(sections[i].duration+sections[i+1].duration);
                if (Texts.count() > 0) {
                    out << ">\n";
                    foreach (ErgFileText cue, Texts)
                        if (cue.x >= msecs && cue.x <= msecs+d)
                            out << "          <textevent "
                                << "timeoffset=\""<<(cue.x-msecs)/1000
                                << "\" message=\"" << cue.text
                                << "\" duration=\"" << cue.duration << "\"/>\n";
                    out << "        </IntervalsT>\n";
                } else {
                    out << "/>\n";
                }
                msecs += d;
                // skip on, bearing in mind the main loop increases i by 1
                i += 1 + (count*2);

            } else {

                QString tag;

                // just a section, but is it SteadyState, Warmup or Cooldown
                if (sections[i].start == sections[i].end) tag = "SteadyState";
                else if (sections[i].start >  sections[i].end) tag = "Cooldown";
                else if (sections[i].start <  sections[i].end) tag = "Warmup";

                out << "        <" << tag << " Duration=\""<<sections[i].duration/1000 << "\" "
                                  << "PowerLow=\"" <<sections[i].start/CP << "\" "
                                  << "PowerHigh=\"" <<sections[i].end/CP << "\"";
                if (Texts.count() > 0) {
                    out << ">\n";
                    foreach (ErgFileText cue, Texts) {
                        if (cue.x >= msecs && cue.x <= msecs+sections[i].duration) {
                            out << "          <textevent "
                                << "timeoffset=\""<<(cue.x-msecs)/1000
                                << "\" message=\"" << Utils::xmlprotect(cue.text)
                                << "\" duration=\"" << cue.duration << "\"/>\n";
                        }
                    }
                    out << "        </" << tag << ">\n";
                } else {
                    out << "/>\n";
                }
                msecs += sections[i].duration;
            }
        }
        out << "    </workout>\n";
        out << "</workout_file>\n";
        f.close();
    }
    return true;
}

ErgFile::~ErgFile()
{
    Points.clear();
}

void ErgFile::sortLaps() const
{
    if (Laps.count()) {
        // Sort laps by start, then by existing lap num
        std::sort(Laps.begin(), Laps.end(), [](const ErgFileLap& a, const ErgFileLap& b) {
            // 1) Start, first comes first
            if (a.x != b.x) return a.x < b.x;

            // 2) range id, lowest first. This ordering is chosen prior to segments being
            // devolved into lap markers, so can be used to impose semantic order on laps.
            if (a.lapRangeId != b.lapRangeId) return a.lapRangeId < b.lapRangeId;

            // 3) LapNum
            return a.LapNum < b.LapNum;
        });

        // Renumber laps to follow the new entry distance order:
        int lapNum = 1;
        for (auto& a : Laps)
            a.LapNum = lapNum++;
    }
}

void ErgFile::sortTexts() const
{
    std::sort(Texts.begin(), Texts.end(), [](const ErgFileText& a, const ErgFileText& b) {
        // If distance is the same the lesser distance comes first.
        if (a.x == b.x) return a.duration < b.duration;
        return a.x < b.x;
    });
}

void ErgFile::finalize()
{
    if (Laps.count() == 0) {
        // If there are no laps then add a pair of laps which bracket the entire workout.

        // Start lap
        ErgFileLap lap;
        lap.x = 0;
        lap.LapNum = 1;
        lap.selected = false;
        lap.name = "Route Start";
        Laps.append(lap);

        // End lap
        lap.x = Duration;
        lap.LapNum = 2;
        lap.selected = false;
        lap.name = "Route End";
        Laps.append(lap);
    }

    sortLaps();
    sortTexts();
}

bool
ErgFile::isValid() const
{
    return valid;
}

// Retrieve the offset for the start of next lap.
// Params: x - current workout distance (m) / time (ms)
// Returns: distance (m) / time (ms) offset for next lap.
double ErgFile::nextLap(double x) const
{
    if (!isValid()) return -1; // not a valid ergfile

    // do we need to return the Lap marker?
    if (Laps.count() > 0) {
        // If the current position is before the start the lap, then the lap is next
        for (int i=0; i<Laps.count(); i++) {
            if (x<Laps.at(i).x) return Laps.at(i).x;
        }
    }
    return -1; // nope, no marker ahead of there
}

// Retrieve the offset for the start of previous lap.
// Params: x - current workout distance (m) / time (ms)
// Returns: distance (m) / time (ms) offset for previous lap.
double ErgFile::prevLap(double x) const
{
    if (!isValid()) return -1; // not a valid ergfile

    // do we need to return the Lap marker?
    if (Laps.count() > 0) {
        for (int i = Laps.count()-1; i >= 0; i--) {
            if (x > Laps.at(i).x) return Laps.at(i).x;
        }
    }
    return -1; // nope, no marker behind us.
}


// Retrieve the offset for the start of current lap.
// Params: x - current workout distance (m) / time (ms)
// Returns: distance (m) / time (ms) offset for start of current lap.
double
ErgFile::currentLap(double x) const
{
    if (!isValid()) return -1; // not a valid ergfile

    // If the current position is before the start of the next lap, return this lap
    for (int i=0; i<Laps.count() - 1; i++) {
        if (x<Laps.at(i+1).x) return Laps.at(i).x;
    }
    return -1; // No matching lap
}

// Adds new lap at location, returns index of new lap
int
ErgFile::addNewLap(double loc) const
{
    if (isValid())
    {
        ErgFileLap add(loc, Laps.count(), "user lap");

        Laps.append(add);

        sortLaps();

        auto itr = std::find_if(Laps.begin(), Laps.end(), [&add](const ErgFileLap& otherLap) {
            return add.x == otherLap.x && add.lapRangeId == otherLap.lapRangeId && add.name == otherLap.name;
        });
        if (itr != Laps.end()) 
            return (*itr).LapNum;
    }

    return -1;
}

bool ErgFile::textsInRange(double searchStart, double searchRange, int& rangeStart, int& rangeEnd) const
{
    bool retVal = false;

    if (isValid()) {

        searchStart = std::floor(searchStart);

        // find first text in range, continue to last text in range.
        rangeStart = -1;
        rangeEnd = -1;

        double searchLimit = searchStart + searchRange;

        for (int i = 0; i < Texts.count(); i++) {
            double distance = Texts.at(i).x;
            if (distance >= searchStart && distance <= searchLimit) {
                if (rangeStart < 0) {
                    rangeStart = i;
                    retVal = true;
                }
                rangeEnd = i;
            }

            // Texts are sorted by distance so no need to look further.
            if (distance > searchLimit)
                break;
        }
    }
    return retVal;
}

void
ErgFile::calculateMetrics()
{
    // reset metrics
    XP = CP = AP = IsoPower = IF = RI = BikeStress = BS = SVI = VI = 0;
    ELE = ELEDIST = GRADE = 0;

    minY = 0;
    maxY = 0;

    // is it valid?
    if (!isValid()) return;

    if (format == CRS) {

        ErgFilePoint last;
        bool first = true;
        foreach(ErgFilePoint p, Points) {

            if (first == true) {
                minY = p.y;
                maxY = p.y;
                first = false;
            } else {
                minY = std::min(minY, p.y);
                maxY = std::max(maxY, p.y);

                if (p.y > last.y) {
                    ELEDIST += p.x - last.x;
                    ELE += p.y - last.y;
                }
            }
            last = p;
        }
        if (ELE == 0 || ELEDIST == 0) GRADE = 0;
        else GRADE = ELE / ELEDIST * 100;

    }
    else {

        QVector<double> rolling(30);
        rolling.fill(0.0f);
        int index = 0;
        double sum = 0; // 30s rolling average
        double apsum = 0; // average power used in VI calculation

        double total = 0;
        double count = 0;
        long nextSecs = 0;

        static const double EPSILON = 0.1;
        static const double NEGLIGIBLE = 0.1;

        double secsDelta = 1;
        double sampsPerWindow = 25.0;
        double attenuation = sampsPerWindow / (sampsPerWindow + secsDelta);
        double sampleWeight = secsDelta / (sampsPerWindow + secsDelta);

        double lastSecs = 0.0;
        double weighted = 0.0;

        double sktotal = 0.0;
        int skcount = 0;

        ErgFilePoint last;
        bool first = true;
        foreach (ErgFilePoint p, Points) {

            // set the minimum/maximum Y value
            if (first) {
                minY = p.y;
                maxY = p.y;
                first = false;
            } else {
                minY = std::min(minY, p.y);
                maxY = std::min(maxY, p.y);
            }

            while (nextSecs < p.x) {

                // CALCULATE IsoPower
                apsum += last.y;
                sum += last.y;
                sum -= rolling[index];

                // update 30s circular buffer
                rolling[index] = last.y;
                if (index == 29) index = 0;
                else index++;

                total += pow(sum / 30, 4);
                count++;

                // CALCULATE XPOWER
                while ((weighted > NEGLIGIBLE) && ((nextSecs / 1000) > (lastSecs / 1000) + 1000 + EPSILON)) {
                    weighted *= attenuation;
                    lastSecs += 1000;
                    sktotal += pow(weighted, 4.0);
                    skcount++;
                }
                weighted *= attenuation;
                weighted += sampleWeight * last.y;
                lastSecs = p.x;
                sktotal += pow(weighted, 4.0);
                skcount++;

                nextSecs += 1000;
            }
            last = p;
        }

        // XP, IsoPower and AP
        XP = pow(sktotal / skcount, 0.25);
        IsoPower = pow(total / count, 0.25);
        AP = apsum / count;

        // CP
        if (context->athlete->zones("Bike")) {
            int zonerange = context->athlete->zones("Bike")->whichRange(QDateTime::currentDateTime().date());
            if (zonerange >= 0) CP = context->athlete->zones("Bike")->getCP(zonerange);
        }

        // IF
        if (CP) {
            IF = IsoPower / CP;
            RI = XP / CP;
        }

        // BikeStress
        double normWork = IsoPower * (Duration / 1000); // msecs
        double rawTSS = normWork * IF;
        double workInAnHourAtCP = CP * 3600;
        BikeStress = rawTSS / workInAnHourAtCP * 100.0;

        // BS
        double xWork = XP * (Duration / 1000); // msecs
        double rawBS = xWork * RI;
        BS = rawBS / workInAnHourAtCP * 100.0;

        // VI and RI
        if (AP) {
            VI = IsoPower / AP;
            SVI = XP / AP;
        }
    }
}

// Update query state to bracket query location.
// Returns false if bracket cannot be established, otherwise true.
bool
ErgFileQueryAdapter::updateQueryStateFromDistance(double x, int& lapnum) const
{
    if (!ergFile || !ergFile->isValid()) return false; // not a valid ergfile

    // is it in bounds?
    if (x < 0 || x > Duration()) return false;

    // do we need to return the Lap marker?
    if (Laps().count() > 0) {
        int lap = 0;
        for (int i = 0; i < Laps().count(); i++) {
            if (x >= Laps().at(i).x) lap += 1;
        }
        lapnum = lap;

    }
    else lapnum = 0;

    // find right section of the file
    while (x < Points().at(qs.leftPoint).x || x > Points().at(qs.rightPoint).x) {
        if (x < Points().at(qs.leftPoint).x) {
            qs.leftPoint--;
            qs.rightPoint--;
        }
        else if (x > Points().at(qs.rightPoint).x) {
            qs.leftPoint++;
            qs.rightPoint++;
        }
    }

    return true;
}

double
ErgFileQueryAdapter::wattsAt(double x, int& lapnum) const
{
    // Establish index bracket for query.
    if (!updateQueryStateFromDistance(x, lapnum))
        return -100;

    if (!hasWatts()) {
        qDebug() << "wattsAt should not be called unless ergfile has watts";
        return -100;
    }

    // two different points in time but the same watts
    // at both, it doesn't really matter which value
    // we use
    if (Points().at(qs.leftPoint).val == Points().at(qs.rightPoint).val)
        return Points().at(qs.rightPoint).val;

    // the erg file will list the point in time twice
    // to show a jump from one wattage to another
    // at this point in ime (i.e x=100 watts=100 followed
    // by x=100 watts=200)
    if (Points().at(qs.leftPoint).x == Points().at(qs.rightPoint).x)
        return Points().at(qs.rightPoint).val;

    // so this point in time between two points and
    // we are ramping from one point and another
    // the steps in the calculation have been explicitly
    // listed for code clarity
    double deltaW = Points().at(qs.rightPoint).val - Points().at(qs.leftPoint).val;
    double deltaT = Points().at(qs.rightPoint).x - Points().at(qs.leftPoint).x;
    double offT = x - Points().at(qs.leftPoint).x;
    double factor = offT / deltaT;

    double nowW = Points().at(qs.leftPoint).val + (deltaW * factor);

    return nowW;
}

// Uninterpolated gradient from ergfile. Used when running in strict gradient/workout mode.
double
ErgFileQueryAdapter::gradientAt(double x, int& lapnum) const
{
    // Establish index bracket for query.
    if (!updateQueryStateFromDistance(x, lapnum))
        return -100;

    if (!hasGradient()) {
        qDebug() << "gradientAt should not be called if ergfile has no gradient";
        return -100;
    }

    double gradient = Points().at(qs.leftPoint).val;

    return gradient;
}

// Altitude
double
ErgFileQueryAdapter::altitudeAt(double x, int& lapnum) const
{
    // Establish index bracket for query.
    if (!updateQueryStateFromDistance(x, lapnum))
        return -1000;

    if (!hasGradient()) {
        qDebug() << "altitudeAt should not be called if ergfile has no gradient";
        return -1000;
    }

    ErgFilePoint p1 = Points().at(qs.leftPoint);
    ErgFilePoint p2 = Points().at(qs.rightPoint);
    double altitude = p1.y;
    if(p1.x != p2.x) altitude += (p2.y - p1.y) * (x - p1.x) / (p2.x - p1.x);

    return altitude;
}

// Returns true if a location is determined, otherwise returns false.
// This is the const stateless edition(tm), which does not rely on location
// state. This method is used to make queries independant of the main ergfile
// 'owner'.
bool ErgFileQueryAdapter::locationAt(double meters, int& lapnum, geolocation& geoLoc, double& slope100) const
{
    // Establish index bracket for query.
    if (!updateQueryStateFromDistance(meters, lapnum))
        return false;

    if (!hasGradient()) {
        qDebug() << "locationAt should not be called if ergfile has no gradient";
        return false;
    }

    // At this point leftpoint and rightpoint bracket the query distance. Three cases:
    // Bracket Covered: If query bracket compatible with the current interpolation bracket then simply interpolate
    // Bracket Ahead:   If query bracket is ahead of current interpolation bracket then feed points to interpolator until it matches.
    // Bracket Behind:  New bracket is behind current interpolation bracket then interpolator must be reset and new values fed in.

    double d0, d1;

    if (!qs.gpi.GetBracket(d0, d1))
    {
        qs.interpolatorReadIndex = 0;
        qs.gpi.Reset();
    }
    else if (d0 > Points().at(qs.leftPoint).x) {
        // Current bracket is ahead of interpolator so reset and set index back
        // so interpolator will be re-primed.
        qs.gpi.Reset();
        qs.interpolatorReadIndex = (qs.leftPoint > 0) ? qs.leftPoint - 1 : qs.leftPoint;
    }
    else {
        // Otherwise interpolatorReadIndex is set reasonably.
        // Is possible to skip ahead though...
        if (qs.interpolatorReadIndex < qs.rightPoint - 4)
        {
            qs.interpolatorReadIndex = qs.rightPoint - 4;
        }
    }

    // Push points until distance satisfied
    while (qs.gpi.WantsInput(meters)) {
        if (qs.interpolatorReadIndex < Points().count()) {
            const ErgFilePoint* pii = &(Points().at(qs.interpolatorReadIndex));

            // When there is no geolocation, invent something to support tangent interpolation.
            if (pii->lat || pii->lon) {
                geolocation geo(pii->lat, pii->lon, pii->y);
                qs.gpi.Push(pii->x, geo);
            }
            else {
                qs.gpi.Push(pii->x, pii->y);
            }

            qs.interpolatorReadIndex++;
        }
        else {
            qs.gpi.NotifyInputComplete();
            break;
        }
    }

    geoLoc = qs.gpi.Location(meters, slope100);

    slope100 *= 100;

    return true;
}

int ErgFileQueryAdapter::addNewLap(double loc) const
{
    return getErgFile() ? getErgFile()->addNewLap(loc) : -1;
}

