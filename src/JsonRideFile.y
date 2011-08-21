%{
/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

// This grammar should work with yacc and bison, but has
// only been tested with bison. In addition, since qmake
// uses the -p flag to rename all the yy functions to
// enable multiple grammars in a single executable you
// should make sure you use the very latest bison since it
// has been known to be problematic in the past. It is
// know to work well with bison v2.4.1.
//
// To make the grammar readable I have placed the code
// for each nterm at column 40, this source file is best
// edited / viewed in an editor which is at least 120
// columns wide (e.g. vi in xterm of 120x40)
//
//
// The grammar is specific to the RideFile format serialised
// in writeRideFile below, this is NOT a generic json parser.

#include "JsonRideFile.h"

// Set during parser processing, using same
// naming conventions as yacc/lex -p
static RideFile *JsonRide;

// term state data is held in these variables
static RideFilePoint JsonPoint;
static RideFileInterval JsonInterval;
static QString JsonString,
               JsonTagKey, JsonTagValue,
               JsonOverName, JsonOverKey, JsonOverValue;
static double JsonNumber;
static QStringList JsonRideFileerrors;
static QMap <QString, QString> JsonOverrides;

// Standard yacc/lex variables / functions
extern int JsonRideFilelex(); // the lexer aka yylex()
extern void JsonRideFilerestart (FILE *input_file); // the lexer file restart aka yyrestart()
extern FILE *JsonRideFilein; // used by the lexer aka yyin
extern char *JsonRideFiletext; // set by the lexer aka yytext
void JsonRideFileerror(const char *error) // used by parser aka yyerror()
{ JsonRideFileerrors << error; }

//
// Utility functions
//

// Escape special characters (JSON compliance)
static QString protect(const QString string)
{
    QString s = string;
    s.replace("\\", "\\\\"); // backslash
    s.replace("\"", "\\\""); // quote
    s.replace("\t", "\\t");  // tab
    s.replace("\n", "\\n");  // newline
    s.replace("\r", "\\r");  // carriage-return
    s.replace("\b", "\\b");  // backspace
    s.replace("\f", "\\f");  // formfeed
    s.replace("/", "\\/");   // solidus
    return s;
}

// Un-Escape special characters (JSON compliance)
static QString unprotect(const QString string)
{
    // this is a lexer string so it will be enclosed
    // in quotes. Lets strip those first
    QString s = string.mid(1,string.length()-2);

    // now un-escape the control characters
    s.replace("\\t", "\t");  // tab
    s.replace("\\n", "\n");  // newline
    s.replace("\\r", "\r");  // carriage-return
    s.replace("\\b", "\b");  // backspace
    s.replace("\\f", "\f");  // formfeed
    s.replace("\\/", "/");   // solidus
    s.replace("\\\"", "\""); // quote
    s.replace("\\\\", "\\"); // backslash
    return s;
}

%}

%token STRING INTEGER FLOAT
%token RIDE STARTTIME RECINTSECS DEVICETYPE IDENTIFIER
%token OVERRIDES
%token TAGS INTERVALS NAME START STOP
%token SAMPLES SECS KM WATTS NM CAD KPH HR ALTITUDE LAT LON HEADWIND

%start document
%%

/* We allow a .json file to be encapsulated within optional braces */
document: '{' ride_list '}'
        | ride_list
        ;
/* multiple rides in a single file are supported, rides will be joined */
ride_list:
        ride
        | ride_list ',' ride
        ;

ride: RIDE ':' '{' rideelement_list '}' ;
rideelement_list: rideelement_list ',' rideelement
                | rideelement
                ;

rideelement: starttime
            | recordint
            | devicetype
            | identifier
            | overrides
            | tags
            | intervals
            | samples
            ;

/*
 * First class variables
 */
starttime: STARTTIME ':' string         {
                                          QDateTime aslocal = QDateTime::fromString(JsonString, DATETIME_FORMAT);
                                          QDateTime asUTC = QDateTime(aslocal.date(), aslocal.time(), Qt::UTC);
                                          JsonRide->setStartTime(asUTC.toLocalTime());
                                        }
recordint: RECINTSECS ':' number        { JsonRide->setRecIntSecs(JsonNumber); }
devicetype: DEVICETYPE ':' string       { JsonRide->setDeviceType(JsonString); }
identifier: IDENTIFIER ':' string       { /* not supported in v2.1 */ }

/*
 * Metric Overrides
 */
overrides: OVERRIDES ':' '[' overrides_list ']' ;
overrides_list: override | overrides_list ',' override ;

override: '{' override_name ':' override_values '}' { JsonRide->metricOverrides.insert(JsonOverName, JsonOverrides);
                                                      JsonOverrides.clear();
                                                    }
override_name: string                   { JsonOverName = JsonString; }

override_values: '{' override_value_list '}';
override_value_list: override_value | override_value_list ',' override_value ;
override_value: override_key ':' override_value { JsonOverrides.insert(JsonOverKey, JsonOverValue); }
override_key : string                   { JsonOverKey = JsonString; }
override_value : string                 { JsonOverValue = JsonString; }

/*
 * Ride metadata tags
 */
tags: TAGS ':' '{' tags_list '}'
tags_list: tag | tags_list ',' tag ;
tag: tag_key ':' tag_value              { JsonRide->setTag(JsonTagKey, JsonTagValue); }

tag_key : string                        { JsonTagKey = JsonString; }
tag_value : string                      { JsonTagValue = JsonString; }

/*
 * Intervals
 */
intervals: INTERVALS ':' '[' interval_list ']' ;
interval_list: interval | interval_list ',' interval ;
interval: '{' NAME ':' string ','       { JsonInterval.name = JsonString; }
              START ':' number ','      { JsonInterval.start = JsonNumber; }
              STOP ':' number           { JsonInterval.stop = JsonNumber; }
          '}'
                                        { JsonRide->addInterval(JsonInterval.start,
                                                                JsonInterval.stop,
                                                                JsonInterval.name);
                                          JsonInterval = RideFileInterval();
                                        }
/*
 * Ride datapoints
 */
samples: SAMPLES ':' '[' sample_list ']' ;
sample_list: sample | sample_list ',' sample ;
sample: '{' series_list '}'             { JsonRide->appendPoint(JsonPoint.secs, JsonPoint.cad,
                                                    JsonPoint.hr, JsonPoint.km, JsonPoint.kph,
                                                    JsonPoint.nm, JsonPoint.watts, JsonPoint.alt,
                                                    JsonPoint.lon, JsonPoint.lat,
                                                    JsonPoint.headwind, JsonPoint.interval);
                                          JsonPoint = RideFilePoint();
                                        }

series_list: series | series_list ',' series ;
series: SECS ':' number                 { JsonPoint.secs = JsonNumber; }
        | KM ':' number                 { JsonPoint.km = JsonNumber; }
        | WATTS ':' number              { JsonPoint.watts = JsonNumber; }
        | NM ':' number                 { JsonPoint.nm = JsonNumber; }
        | CAD ':' number                { JsonPoint.cad = JsonNumber; }
        | KPH ':' number                { JsonPoint.kph = JsonNumber; }
        | HR ':' number                 { JsonPoint.hr = JsonNumber; }
        | ALTITUDE ':' number           { JsonPoint.alt = JsonNumber; }
        | LAT ':' number                { JsonPoint.lat = JsonNumber; }
        | LON ':' number                { JsonPoint.lon = JsonNumber; }
        | HEADWIND ':' number           { JsonPoint.headwind = JsonNumber; }
        ;

/*
 * Primitives
 */
number: INTEGER                         { JsonNumber = QString(JsonRideFiletext).toInt(); }
        | FLOAT                         { JsonNumber = QString(JsonRideFiletext).toDouble(); }
        ;

string: STRING                          { JsonString = unprotect(JsonRideFiletext); }
        ;
%%


static int jsonFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "json", "GoldenCheetah Json  Format", new JsonFileReader());

RideFile *
JsonFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    // jsonRide is local and static, used in the parser
    // JsonRideFilein is the FILE * used by the lexer
    JsonRideFilein = fopen(file.fileName().toLatin1(), "r");
    if (JsonRideFilein == NULL) {
        errors << "unable to open file" + file.fileName();
    }
    // inform the parser/lexer we have a new file
    JsonRideFilerestart(JsonRideFilein);

    // setup
    JsonRide = new RideFile;
    JsonRideFileerrors.clear();

    // set to non-zero if you want to
    // to debug the yyparse() state machine
    // sending state transitions to stderr
    //yydebug = 0;

    // parse it
    JsonRideFileparse();

    // release the file handle
    fclose(JsonRideFilein);

    // Only get errors so fail if we have any
    if (errors.count()) {
        errors << JsonRideFileerrors;
        delete JsonRide;
        return NULL;
    } else return JsonRide;
}

// Writes valid .json (validated at www.jsonlint.com)
void
JsonFileReader::writeRideFile(const RideFile *ride, QFile &file) const
{
    // can we open the file for writing?
    if (!file.open(QIODevice::WriteOnly)) return;

    // truncate existing
    file.resize(0);

    // setup streamer
    QTextStream out(&file);

    // start of document and ride
    out << "{\n\t\"RIDE\":{\n";

    // first class variables
    out << "\t\t\"STARTTIME\":\"" << protect(ride->startTime().toUTC().toString(DATETIME_FORMAT)) << "\",\n";
    out << "\t\t\"RECINTSECS\":" << ride->recIntSecs() << ",\n";
    out << "\t\t\"DEVICETYPE\":\"" << protect(ride->deviceType()) << "\",\n";
    // not available in v2.1 -- out << "\t\t\"IDENTIFIER\":\"" << protect(ride->id()) << "\"";

    //
    // OVERRIDES
    //
    bool nonblanks = false; // if an override has been deselected it may be blank
                            // so we only output the OVERRIDES section if we find an
                            // override whilst iterating over the QMap

    if (ride->metricOverrides.count()) {


        QMap<QString,QMap<QString, QString> >::const_iterator k;
        for (k=ride->metricOverrides.constBegin(); k != ride->metricOverrides.constEnd(); k++) {

            // may not contain anything
            if (k.value().isEmpty()) continue;

            if (nonblanks == false) {
                out << ",\n\t\t\"OVERRIDES\":[\n";
                nonblanks = true;

            }
            // begin of overrides
            out << "\t\t\t{ \"" << k.key() << "\":{ ";

            // key/value pairs
            QMap<QString, QString>::const_iterator j;
            for (j=k.value().constBegin(); j != k.value().constEnd(); j++) {

                // comma separated
                out << "\"" << j.key() << "\":\"" << j.value() << "\"";
                if (j+1 != k.value().constEnd()) out << ", ";
            }
            if (k+1 != ride->metricOverrides.constEnd()) out << " }},\n";
            else out << " }}\n";
        }

        if (nonblanks == true) {
            // end of the overrides
            out << "\t\t]";
        }
    }

    //
    // TAGS
    //
    if (ride->tags().count()) {

        out << ",\n\t\t\"TAGS\":{\n";

        QMap<QString,QString>::const_iterator i;
        for (i=ride->tags().constBegin(); i != ride->tags().constEnd(); i++) {

                out << "\t\t\t\"" << i.key() << "\":\"" << protect(i.value()) << "\"";
                if (i+1 != ride->tags().constEnd()) out << ",\n";
                else out << "\n";
        }

        // end of the tags
        out << "\t\t}";
    }

    //
    // INTERVALS
    //
    if (!ride->intervals().empty()) {

        out << ",\n\t\t\"INTERVALS\":[\n";
        bool first = true;

        foreach (RideFileInterval i, ride->intervals()) {
            if (first) first=false;
            else out << ",\n";

            out << "\t\t\t{ ";
            out << "\"NAME\":\"" << protect(i.name) << "\"";
            out << ", \"START\": " << QString("%1").arg(i.start);
            out << ", \"STOP\": " << QString("%1").arg(i.stop) << " }";
        }
        out <<"\n\t\t]";
    }

    //
    // SAMPLES
    //
    if (ride->dataPoints().count()) {

        out << ",\n\t\t\"SAMPLES\":[\n";
        bool first = true;

        foreach (RideFilePoint *p, ride->dataPoints()) {

            if (first) first=false;
            else out << ",\n";

            out << "\t\t\t{ ";

            // always store time
            out << "\"SECS\":" << QString("%1").arg(p->secs);

            if (ride->areDataPresent()->km) out << ", \"KM\":" << QString("%1").arg(p->km);
            if (ride->areDataPresent()->watts) out << ", \"WATTS\":" << QString("%1").arg(p->watts);
            if (ride->areDataPresent()->nm) out << ", \"NM\":" << QString("%1").arg(p->nm);
            if (ride->areDataPresent()->cad) out << ", \"CAD\":" << QString("%1").arg(p->cad);
            if (ride->areDataPresent()->kph) out << ", \"KPH\":" << QString("%1").arg(p->kph);
            if (ride->areDataPresent()->hr) out << ", \"HR\":"  << QString("%1").arg(p->hr);
            if (ride->areDataPresent()->alt) out << ", \"ALT\":" << QString("%1").arg(p->alt);
            if (ride->areDataPresent()->lat)
                out << ", \"LAT\":" << QString("%1").arg(p->lat, 0, 'g', 11);
            if (ride->areDataPresent()->lon)
                out << ", \"LON\":" << QString("%1").arg(p->lon, 0, 'g', 11);
            if (ride->areDataPresent()->headwind) out << ", \"HEADWIND\":" << QString("%1").arg(p->headwind);

            // sample points in here!
            out << " }";
        }
        out <<"\n\t\t]";
    }

    // end of ride and document
    out << "\n\t}\n}\n";

    // close
    file.close();
}
