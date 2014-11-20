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
static RideFileCalibration JsonCalibration;
static QString JsonString,
               JsonTagKey, JsonTagValue,
               JsonOverName, JsonOverKey, JsonOverValue;
static double JsonNumber;
static QStringList JsonRideFileerrors;
static QMap <QString, QString> JsonOverrides;

// Lex scanner
extern int JsonRideFilelex(); // the lexer aka yylex()
extern void JsonRideFile_setString(QString);
extern int JsonRideFilelex_destroy(void); // the cleaner for lexer

// yacc parser
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

    // add a trailing space to avoid conflicting with GC special tokens
    s += " "; 

    return s;
}

// Un-Escape special characters (JSON compliance)
static QString unprotect(const char * string)
{
    // sending UTF-8 to FLEX demands symetric conversion back to QString
    QString string2 = QString::fromUtf8(string);

    // this is a lexer string so it will be enclosed
    // in quotes. Lets strip those first
    QString s = string2.mid(1,string2.length()-2);

    // does it end with a space (to avoid token conflict) ?
    if (s.endsWith(" ")) s = s.mid(0, s.length()-1);

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

%token JS_STRING JS_INTEGER JS_FLOAT
%token RIDE STARTTIME RECINTSECS DEVICETYPE IDENTIFIER
%token OVERRIDES
%token TAGS INTERVALS NAME START STOP
%token CALIBRATIONS VALUE
%token REFERENCES
%token SAMPLES SECS KM WATTS NM CAD KPH HR ALTITUDE LAT LON HEADWIND SLOPE TEMP 
%token LRBALANCE LTE RTE LPS RPS THB SMO2 RVERT RCAD RCON

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
            | calibrations
            | references
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
identifier: IDENTIFIER ':' string       { JsonRide->setId(JsonString); }

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
 * Calibrations
 */
calibrations: CALIBRATIONS ':' '[' calibration_list ']' ;
calibration_list: calibration | calibration_list ',' calibration ;
calibration: '{' NAME ':' string ','    { JsonCalibration.name = JsonString; }
                 START ':' number ','   { JsonCalibration.start = JsonNumber; }
                 VALUE ':' number       { JsonCalibration.value = JsonNumber; }
             '}'
                                        { JsonRide->addCalibration(JsonCalibration.start,
                                                                   JsonCalibration.value,
                                                                   JsonCalibration.name);
                                          JsonCalibration = RideFileCalibration();
                                        }


/*
 * Ride references
 */
references: REFERENCES ':' '[' reference_list ']'
                                        {
                                          JsonPoint = RideFilePoint();
                                        }
reference_list: reference | reference_list ',' reference;
reference: '{' series_list '}'          { JsonRide->appendReference(JsonPoint);
                                          JsonPoint = RideFilePoint();
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
                                                    JsonPoint.headwind,
                                                    JsonPoint.slope, JsonPoint.temp, JsonPoint.lrbalance,
                                                    JsonPoint.lte, JsonPoint.rte,
                                                    JsonPoint.lps, JsonPoint.rps,
                                                    JsonPoint.smo2, JsonPoint.thb,
                                                    JsonPoint.rvert, JsonPoint.rcad, JsonPoint.rcontact,
                                                    JsonPoint.interval);
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
        | SLOPE ':' number              { JsonPoint.slope = JsonNumber; }
        | TEMP ':' number               { JsonPoint.temp = JsonNumber; }
        | LRBALANCE ':' number          { JsonPoint.lrbalance = JsonNumber; }
        | LTE ':' number                { JsonPoint.lte = JsonNumber; }
        | RTE ':' number                { JsonPoint.rte = JsonNumber; }
        | LPS ':' number                { JsonPoint.lps = JsonNumber; }
        | RPS ':' number                { JsonPoint.rps = JsonNumber; }
        | SMO2 ':' number               { JsonPoint.smo2 = JsonNumber; }
        | THB ':' number                { JsonPoint.thb = JsonNumber; }
        | RVERT ':' number              { JsonPoint.rvert = JsonNumber; }
        | RCAD ':' number               { JsonPoint.rcad = JsonNumber; }
        | RCON ':' number               { JsonPoint.rcontact = JsonNumber; }
        | string ':' number             { }
        | string ':' string
        ;


/*
 * Primitives
 */
number: JS_INTEGER                         { JsonNumber = QString(JsonRideFiletext).toInt(); }
        | JS_FLOAT                         { JsonNumber = QString(JsonRideFiletext).toDouble(); }
        ;

string: JS_STRING                          { JsonString = unprotect(JsonRideFiletext); }
        ;
%%


static int jsonFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "json", "GoldenCheetah Json", new JsonFileReader());

RideFile *
JsonFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    // Read the entire file into a QString -- we avoid using fopen since it
    // doesn't handle foreign characters well. Instead we use QFile and parse
    // from a QString
    QString contents;
    if (file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {

        // read in the whole thing
        QTextStream in(&file);
        // GC .JSON is stored in UTF-8 with BOM(Byte order mark) for identification
        in.setCodec ("UTF-8");
        contents = in.readAll();
        file.close();

        // check if the text string contains the replacement character for UTF-8 encoding
        // if yes, try to read with Latin1/ISO 8859-1 (assuming this is an "old" non-UTF-8 Json file)
        if (contents.contains(QChar::ReplacementCharacter)) {
           if (file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {
             QTextStream in(&file);
             in.setCodec ("ISO 8859-1");
             contents = in.readAll();
             file.close();
           }
         }

    } else {

        errors << "unable to open file" + file.fileName();
        return NULL; 
    }

    // inform the parser/lexer we have a new file
    JsonRideFile_setString(contents);

    // setup
    JsonRide = new RideFile;
    JsonRideFileerrors.clear();

    // set to non-zero if you want to
    // to debug the yyparse() state machine
    // sending state transitions to stderr
    //yydebug = 0;

    // parse it
    JsonRideFileparse();

    // clean up
    JsonRideFilelex_destroy();

    // Only get errors so fail if we have any
    if (errors.count()) {
        errors << JsonRideFileerrors;
        delete JsonRide;
        return NULL;
    } else return JsonRide;
}

// Writes valid .json (validated at www.jsonlint.com)
bool
JsonFileReader::writeRideFile(Context *, const RideFile *ride, QFile &file) const
{
    // can we open the file for writing?
    if (!file.open(QIODevice::WriteOnly)) return false;

    // truncate existing
    file.resize(0);

    // setup streamer
    QTextStream out(&file);
    // unified codepage and BOM for identification on all platforms
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);

    // start of document and ride
    out << "{\n\t\"RIDE\":{\n";

    // first class variables
    out << "\t\t\"STARTTIME\":\"" << protect(ride->startTime().toUTC().toString(DATETIME_FORMAT)) << "\",\n";
    out << "\t\t\"RECINTSECS\":" << ride->recIntSecs() << ",\n";
    out << "\t\t\"DEVICETYPE\":\"" << protect(ride->deviceType()) << "\",\n";
    out << "\t\t\"IDENTIFIER\":\"" << protect(ride->id()) << "\"";

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
    // CALIBRATION
    //
    if (!ride->calibrations().empty()) {

        out << ",\n\t\t\"CALIBRATIONS\":[\n";
        bool first = true;

        foreach (RideFileCalibration i, ride->calibrations()) {
            if (first) first=false;
            else out << ",\n";

            out << "\t\t\t{ ";
            out << "\"NAME\":\"" << protect(i.name) << "\"";
            out << ", \"START\": " << QString("%1").arg(i.start);
            out << ", \"VALUE\": " << QString("%1").arg(i.value) << " }";
        }
        out <<"\n\t\t]";
    }

    //
    // REFERENCES
    //
    if (!ride->referencePoints().empty()) {

        out << ",\n\t\t\"REFERENCES\":[\n";
        bool first = true;

        foreach (RideFilePoint *p, ride->referencePoints()) {
            if (first) first=false;
            else out << ",\n";

            out << "\t\t\t{ ";

            if (p->watts > 0) out << " \"WATTS\":" << QString("%1").arg(p->watts);
            if (p->cad > 0) out << " \"CAD\":" << QString("%1").arg(p->cad);
            if (p->hr > 0) out << " \"HR\":"  << QString("%1").arg(p->hr);

            // sample points in here!
            out << " }";
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
            if (ride->areDataPresent()->slope) out << ", \"SLOPE\":" << QString("%1").arg(p->slope);
            if (ride->areDataPresent()->temp && p->temp != RideFile::NoTemp) out << ", \"TEMP\":" << QString("%1").arg(p->temp);
            if (ride->areDataPresent()->lrbalance) out << ", \"LRBALANCE\":" << QString("%1").arg(p->lrbalance);
            if (ride->areDataPresent()->lte) out << ", \"LTE\":" << QString("%1").arg(p->lte);
            if (ride->areDataPresent()->rte) out << ", \"RTE\":" << QString("%1").arg(p->rte);
            if (ride->areDataPresent()->lps) out << ", \"LPS\":" << QString("%1").arg(p->lps);
            if (ride->areDataPresent()->rps) out << ", \"RPS\":" << QString("%1").arg(p->rps);
            if (ride->areDataPresent()->smo2) out << ", \"SMO2\":" << QString("%1").arg(p->smo2);
            if (ride->areDataPresent()->thb) out << ", \"THB\":" << QString("%1").arg(p->thb);
            if (ride->areDataPresent()->rcad) out << ", \"RCAD\":" << QString("%1").arg(p->rcad);
            if (ride->areDataPresent()->rvert) out << ", \"RVERT\":" << QString("%1").arg(p->rvert);
            if (ride->areDataPresent()->rcontact) out << ", \"RCON\":" << QString("%1").arg(p->rcontact);

            // sample points in here!
            out << " }";
        }
        out <<"\n\t\t]";
    }

    // end of ride and document
    out << "\n\t}\n}\n";

    // close
    file.close();

    return true;
}
