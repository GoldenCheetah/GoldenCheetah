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
#include "RideMetadata.h"

// now we have a reentrant parser we save context data
// in a structure rather than in global variables -- so
// you can run the parser concurrently.
struct JsonContext {

    // the scanner
    void *scanner;

    // Set during parser processing, using same
    // naming conventions as yacc/lex -p
    RideFile *JsonRide;

    // term state data is held in these variables
    RideFilePoint JsonPoint;
    RideFileInterval JsonInterval;
    RideFileCalibration JsonCalibration;
    QString JsonString,
                JsonTagKey, JsonTagValue,
                JsonOverName, JsonOverKey, JsonOverValue;
    double JsonNumber;
    QStringList JsonRideFileerrors;
    QMap <QString, QString> JsonOverrides;

    XDataSeries xdataseries;
    XDataPoint xdatapoint;
    QStringList stringlist;
    QVector<double> numberlist;

};

#define YYSTYPE QString

// Lex scanner
extern int JsonRideFilelex(YYSTYPE*,void*); // the lexer aka yylex()
extern int JsonRideFilelex_init(void**);
extern void JsonRideFile_setString(QString, void *);
extern int JsonRideFilelex_destroy(void*); // the cleaner for lexer

// yacc parser
void JsonRideFileerror(void*jc, const char *error) // used by parser aka yyerror()
{ static_cast<JsonContext*>(jc)->JsonRideFileerrors << error; }

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

// extract scanner from the context
#define scanner jc->scanner

%}

%pure-parser
%lex-param { void *scanner }
%parse-param { struct JsonContext *jc }

%token JS_STRING JS_INTEGER JS_FLOAT
%token RIDE STARTTIME RECINTSECS DEVICETYPE IDENTIFIER
%token OVERRIDES
%token TAGS INTERVALS NAME START STOP COLOR TEST
%token CALIBRATIONS VALUE VALUES UNIT UNITS
%token REFERENCES
%token XDATA
%token SAMPLES SECS KM WATTS NM CAD KPH HR ALTITUDE LAT LON HEADWIND SLOPE TEMP
%token LRBALANCE LTE RTE LPS RPS THB SMO2 RVERT RCAD RCON
%token LPCO RPCO LPPB RPPB LPPE RPPE LPPPB RPPPB LPPPE RPPPE

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
            | xdata
            ;

/*
 * First class variables
 */
starttime: STARTTIME ':' string         {
                                          QDateTime aslocal = QDateTime::fromString(jc->JsonString, DATETIME_FORMAT);
                                          QDateTime asUTC = QDateTime(aslocal.date(), aslocal.time(), Qt::UTC);
                                          jc->JsonRide->setStartTime(asUTC.toLocalTime());
                                        }
recordint: RECINTSECS ':' number        { jc->JsonRide->setRecIntSecs(jc->JsonNumber); }
devicetype: DEVICETYPE ':' string       { jc->JsonRide->setDeviceType(jc->JsonString); }
identifier: IDENTIFIER ':' string       { jc->JsonRide->setId(jc->JsonString); }

/*
 * Metric Overrides
 */
overrides: OVERRIDES ':' '[' overrides_list ']' ;
overrides_list: override | overrides_list ',' override ;

override: '{' override_name ':' override_values '}' { jc->JsonRide->metricOverrides.insert(jc->JsonOverName, jc->JsonOverrides);
                                                      jc->JsonOverrides.clear();
                                                    }

                                         // we renamed time riding to time moving ...
override_name: string                   { if (jc->JsonString == "Time Riding") jc->JsonOverName = "Time Moving";
                                          else jc->JsonOverName = jc->JsonString; }

override_values: '{' override_value_list '}';
override_value_list: override_value | override_value_list ',' override_value ;
override_value: override_key ':' override_value { jc->JsonOverrides.insert(jc->JsonOverKey, jc->JsonOverValue); }
override_key : string                   { jc->JsonOverKey = jc->JsonString; }
override_value : string                 { jc->JsonOverValue = jc->JsonString; }

/*
 * Ride metadata tags
 */
tags: TAGS ':' '{' tags_list '}'
tags_list: tag | tags_list ',' tag ;
tag: tag_key ':' tag_value              { jc->JsonRide->setTag(jc->JsonTagKey, jc->JsonTagValue); }

                                          // we renamed time riding to time moving ...
tag_key : string                        { if (jc->JsonString == "Time Riding") jc->JsonTagKey = "Time Moving";
                                          else jc->JsonTagKey = jc->JsonString; }

tag_value : string                      { jc->JsonTagValue = jc->JsonString; }

/*
 * Intervals
 */
intervals: INTERVALS ':' '[' interval_list ']' ;
interval_list: interval | interval_list ',' interval ;
interval_test:
                | ',' TEST ':' string       { jc->JsonInterval.test = (jc->JsonString == "true" ? true : false); }
                ;

interval_color:
                | ',' COLOR ':' string      { jc->JsonInterval.color.setNamedColor(jc->JsonString); }
                ;

interval: '{' NAME ':' string ','       { jc->JsonInterval.name = jc->JsonString; }
              START ':' number ','      { jc->JsonInterval.start = jc->JsonNumber; }
              STOP ':' number           { jc->JsonInterval.stop = jc->JsonNumber; }
              interval_color
              interval_test
          '}'
                                        { jc->JsonRide->addInterval(RideFileInterval::USER,
                                                                jc->JsonInterval.start,
                                                                jc->JsonInterval.stop,
                                                                jc->JsonInterval.name,
                                                                jc->JsonInterval.color,
                                                                jc->JsonInterval.test);
                                          jc->JsonInterval = RideFileInterval();
                                        }

/*
 * Calibrations
 */
calibrations: CALIBRATIONS ':' '[' calibration_list ']' ;
calibration_list: calibration | calibration_list ',' calibration ;
calibration: '{' NAME ':' string ','    { jc->JsonCalibration.name = jc->JsonString; }
                 START ':' number ','   { jc->JsonCalibration.start = jc->JsonNumber; }
                 VALUE ':' number       { jc->JsonCalibration.value = jc->JsonNumber; }
             '}'
                                        { jc->JsonRide->addCalibration(jc->JsonCalibration.start,
                                                                   jc->JsonCalibration.value,
                                                                   jc->JsonCalibration.name);
                                          jc->JsonCalibration = RideFileCalibration();
                                        }


/*
 * Ride references
 */
references: REFERENCES ':' '[' reference_list ']'
                                        {
                                          jc->JsonPoint = RideFilePoint();
                                        }
reference_list: reference | reference_list ',' reference;
reference: '{' series '}'               { jc->JsonRide->appendReference(jc->JsonPoint);
                                          jc->JsonPoint = RideFilePoint();
                                        }
/*
 * XData series
 */

xdata: XDATA ':' '[' xdata_list ']'
xdata_list: xdata_series
            | xdata_list ',' xdata_series
            ;

xdata_series: '{' xdata_items '}'              { XDataSeries *add = new XDataSeries;
                                                 add->name=jc->xdataseries.name;
                                                 add->datapoints=jc->xdataseries.datapoints;
                                                 add->valuename=jc->xdataseries.valuename;
                                                 add->unitname=jc->xdataseries.unitname;
                                                 jc->JsonRide->addXData(add->name, add);

                                                 // clear for next one
                                                 jc->xdataseries = XDataSeries();
                                               }


xdata_items: xdata_item
            | xdata_items ',' xdata_item
            ;

xdata_item: NAME ':' string                     { jc->xdataseries.name = $3; }
          | VALUE ':' string                    { jc->xdataseries.valuename << $3; }
          | UNIT ':' string                     { jc->xdataseries.unitname << $3; }
          | VALUES ':' '[' string_list ']'      { jc->xdataseries.valuename = jc->stringlist;
                                                  jc->stringlist.clear(); }
          | UNITS ':' '[' string_list ']'       { jc->xdataseries.unitname = jc->stringlist;
                                                  jc->stringlist.clear(); }
          | SAMPLES ':' '[' xdata_samples ']'
          ;

xdata_samples: xdata_sample
         | xdata_samples ',' xdata_sample
         ;
xdata_sample: '{' xdata_value_list '}'          { jc->xdataseries.datapoints.append(new XDataPoint(jc->xdatapoint));
                                                  jc->xdatapoint = XDataPoint();
                                                }
          ;

xdata_value_list: xdata_value | xdata_value_list ',' xdata_value
xdata_value:
        SECS ':' number                         { jc->xdatapoint.secs = jc->JsonNumber; }
        | KM ':' number                         { jc->xdatapoint.km = jc->JsonNumber; }
        | VALUE ':' number                      { jc->xdatapoint.number[0] = jc->JsonNumber; }
        | VALUES ':' '[' number_list ']'        { for(int i=0; i<jc->numberlist.count() && i<XDATA_MAXVALUES; i++)
                                                      jc->xdatapoint.number[i]= jc->numberlist[i];
                                                  jc->numberlist.clear(); }
        | string ':' number                     { /* ignored for future compatibility */ }
        | string ':' string                     { /* ignored for future compatibility */ }
        ;

/*
 * Ride datapoints
 */
samples: SAMPLES ':' '[' sample_list ']' ;
sample_list: sample | sample_list ',' sample ;
sample: '{' series_list '}'             { jc->JsonRide->appendPoint(jc->JsonPoint.secs, jc->JsonPoint.cad,
                                                    jc->JsonPoint.hr, jc->JsonPoint.km, jc->JsonPoint.kph,
                                                    jc->JsonPoint.nm, jc->JsonPoint.watts, jc->JsonPoint.alt,
                                                    jc->JsonPoint.lon, jc->JsonPoint.lat,
                                                    jc->JsonPoint.headwind,
                                                    jc->JsonPoint.slope, jc->JsonPoint.temp, jc->JsonPoint.lrbalance,
                                                    jc->JsonPoint.lte, jc->JsonPoint.rte,
                                                    jc->JsonPoint.lps, jc->JsonPoint.rps,
                                                    jc->JsonPoint.lpco, jc->JsonPoint.rpco,
                                                    jc->JsonPoint.lppb, jc->JsonPoint.rppb,
                                                    jc->JsonPoint.lppe, jc->JsonPoint.rppe,
                                                    jc->JsonPoint.lpppb, jc->JsonPoint.rpppb,
                                                    jc->JsonPoint.lpppe, jc->JsonPoint.rpppe,
                                                    jc->JsonPoint.smo2, jc->JsonPoint.thb,
                                                    jc->JsonPoint.rvert, jc->JsonPoint.rcad, jc->JsonPoint.rcontact,
                                                    jc->JsonPoint.tcore,
                                                    jc->JsonPoint.interval);
                                          jc->JsonPoint = RideFilePoint();
                                        }

series_list: series | series_list ',' series ;
series: SECS ':' number                 { jc->JsonPoint.secs = jc->JsonNumber; }
        | KM ':' number                 { jc->JsonPoint.km = jc->JsonNumber; }
        | WATTS ':' number              { jc->JsonPoint.watts = jc->JsonNumber; }
        | NM ':' number                 { jc->JsonPoint.nm = jc->JsonNumber; }
        | CAD ':' number                { jc->JsonPoint.cad = jc->JsonNumber; }
        | KPH ':' number                { jc->JsonPoint.kph = jc->JsonNumber; }
        | HR ':' number                 { jc->JsonPoint.hr = jc->JsonNumber; }
        | ALTITUDE ':' number           { jc->JsonPoint.alt = jc->JsonNumber; }
        | LAT ':' number                { jc->JsonPoint.lat = jc->JsonNumber; }
        | LON ':' number                { jc->JsonPoint.lon = jc->JsonNumber; }
        | HEADWIND ':' number           { jc->JsonPoint.headwind = jc->JsonNumber; }
        | SLOPE ':' number              { jc->JsonPoint.slope = jc->JsonNumber; }
        | TEMP ':' number               { jc->JsonPoint.temp = jc->JsonNumber; }
        | LRBALANCE ':' number          { jc->JsonPoint.lrbalance = jc->JsonNumber; }
        | LTE ':' number                { jc->JsonPoint.lte = jc->JsonNumber; }
        | RTE ':' number                { jc->JsonPoint.rte = jc->JsonNumber; }
        | LPS ':' number                { jc->JsonPoint.lps = jc->JsonNumber; }
        | RPS ':' number                { jc->JsonPoint.rps = jc->JsonNumber; }
        | LPCO ':' number               { jc->JsonPoint.lpco = jc->JsonNumber; }
        | RPCO ':' number               { jc->JsonPoint.rpco = jc->JsonNumber; }
        | LPPB ':' number               { jc->JsonPoint.lppb = jc->JsonNumber; }
        | RPPB ':' number               { jc->JsonPoint.rppb = jc->JsonNumber; }
        | LPPE ':' number               { jc->JsonPoint.lppe = jc->JsonNumber; }
        | RPPE ':' number               { jc->JsonPoint.rppe = jc->JsonNumber; }
        | LPPPB ':' number              { jc->JsonPoint.lpppb = jc->JsonNumber; }
        | RPPPB ':' number              { jc->JsonPoint.rpppb = jc->JsonNumber; }
        | LPPPE ':' number              { jc->JsonPoint.lpppe = jc->JsonNumber; }
        | RPPPE ':' number              { jc->JsonPoint.rpppe = jc->JsonNumber; }
        | SMO2 ':' number               { jc->JsonPoint.smo2 = jc->JsonNumber; }
        | THB ':' number                { jc->JsonPoint.thb = jc->JsonNumber; }
        | RVERT ':' number              { jc->JsonPoint.rvert = jc->JsonNumber; }
        | RCAD ':' number               { jc->JsonPoint.rcad = jc->JsonNumber; }
        | RCON ':' number               { jc->JsonPoint.rcontact = jc->JsonNumber; }
        | string ':' number             { }
        | string ':' string
        ;


/*
 * Primitives
 */
number: JS_INTEGER                         { jc->JsonNumber = QString($1).toInt(); }
        | JS_FLOAT                         { jc->JsonNumber = QString($1).toDouble(); }
        ;

string: JS_STRING                          { jc->JsonString = $1; }
        ;

 string_list: string                       { jc->stringlist << $1; }
            | string_list ',' string       { jc->stringlist << $3; }
            ;

 number_list: number                       { jc->numberlist << QString($1).toDouble(); }
            | number_list ',' number       { jc->numberlist << QString($3).toDouble(); }

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

    // create scanner context for reentrant parsing
    JsonContext *jc = new JsonContext;
    JsonRideFilelex_init(&scanner);

    // inform the parser/lexer we have a new file
    JsonRideFile_setString(contents, scanner);

    // setup
    jc->JsonRide = new RideFile;
    jc->JsonRideFileerrors.clear();

    // set to non-zero if you want to
    // to debug the yyparse() state machine
    // sending state transitions to stderr
    //yydebug = 1;

    // parse it
    JsonRideFileparse(jc);

    // clean up
    JsonRideFilelex_destroy(scanner);

    // Only get errors so fail if we have any
    // and always delete context now we're done
    if (errors.count()) {
        errors << jc->JsonRideFileerrors;
        delete jc->JsonRide;
        delete jc;
        return NULL;
    }

    RideFile *returning = jc->JsonRide;
    delete jc;
    return returning;
}

QByteArray
JsonFileReader::toByteArray(Context *, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad) const
{
    QString out;

    // start of document and ride
    out += "{\n\t\"RIDE\":{\n";

    // first class variables
    out += "\t\t\"STARTTIME\":\"" + protect(ride->startTime().toUTC().toString(DATETIME_FORMAT)) + "\",\n";
    out += "\t\t\"RECINTSECS\":" + QString("%1").arg(ride->recIntSecs()) + ",\n";
    out += "\t\t\"DEVICETYPE\":\"" + protect(ride->deviceType()) + "\",\n";
    out += "\t\t\"IDENTIFIER\":\"" + protect(ride->id()) + "\"";

    //
    // OVERRIDES
    //
    bool nonblanks = false; // if an override has been deselected it may be blank
                            // so we only output the OVERRIDES section if we find an
                            // override whilst iterating over the QMap

    if (ride->metricOverrides.count()) {


        QMap<QString,QMap<QString, QString> >::const_iterator k;
        for (k=ride->metricOverrides.constBegin(); k != ride->metricOverrides.constEnd(); k++) {

            if (nonblanks == false) {
                out += ",\n\t\t\"OVERRIDES\":[\n";
                nonblanks = true;

            }
            // begin of overrides
            out += "\t\t\t{ \"" + k.key() + "\":{ ";

            // key/value pairs
            QMap<QString, QString>::const_iterator j;
            for (j=k.value().constBegin(); j != k.value().constEnd(); j++) {

                // comma separated
                out += "\"" + j.key() + "\":\"" + j.value() + "\"";
                if (j+1 != k.value().constEnd()) out += ", ";
            }
            if (k+1 != ride->metricOverrides.constEnd()) out += " }},\n";
            else out += " }}\n";
        }

        if (nonblanks == true) {
            // end of the overrides
            out += "\t\t]";
        }
    }

    //
    // TAGS
    //
    if (ride->tags().count()) {

        out += ",\n\t\t\"TAGS\":{\n";

        QMap<QString,QString>::const_iterator i;
        for (i=ride->tags().constBegin(); i != ride->tags().constEnd(); i++) {

                out += "\t\t\t\"" + i.key() + "\":\"" + protect(i.value()) + "\"";
                if (i+1 != ride->tags().constEnd()) out += ",\n";
        }

        foreach(RideFileInterval *inter, ride->intervals()) {
            bool first=true;
            QMap<QString,QString>::const_iterator i;
            for (i=inter->tags().constBegin(); i != inter->tags().constEnd(); i++) {

                    if (first) {
                        out += ",\n";
                        first=false;
                    }
                    out += "\t\t\t\"" + inter->name + "##" + i.key() + "\":\"" + protect(i.value()) + "\"";
                    if (i+1 != inter->tags().constEnd()) out += ",\n";
            }
        }

        // end of the tags
        out += "\n\t\t}";
    }

    //
    // INTERVALS
    //
    if (!ride->intervals().empty()) {

        out += ",\n\t\t\"INTERVALS\":[\n";
        bool first = true;

        foreach (RideFileInterval *i, ride->intervals()) {
            if (first) first=false;
            else out += ",\n";

            out += "\t\t\t{ ";
            out += "\"NAME\":\"" + protect(i->name) + "\"";
            out += ", \"START\": " + QString("%1").arg(i->start);
            out += ", \"STOP\": " + QString("%1").arg(i->stop);
            out += ", \"COLOR\":" + QString("\"%1\"").arg(i->color.name());
            out += ", \"PTEST\":\"" + QString("%1").arg(i->test ? "true" : "false") + "\" }";
        }
        out += "\n\t\t]";
    }

    //
    // CALIBRATION
    //
    if (!ride->calibrations().empty()) {

        out += ",\n\t\t\"CALIBRATIONS\":[\n";
        bool first = true;

        foreach (RideFileCalibration *i, ride->calibrations()) {
            if (first) first=false;
            else out += ",\n";

            out += "\t\t\t{ ";
            out += "\"NAME\":\"" + protect(i->name) + "\"";
            out += ", \"START\": " + QString("%1").arg(i->start);
            out += ", \"VALUE\": " + QString("%1").arg(i->value) + " }";
        }
        out += "\n\t\t]";
    }

    //
    // REFERENCES
    //
    if (!ride->referencePoints().empty()) {

        out += ",\n\t\t\"REFERENCES\":[\n";
        bool first = true;

        foreach (RideFilePoint *p, ride->referencePoints()) {
            if (first) first=false;
            else out += ",\n";

            out += "\t\t\t{ ";

            if (p->watts > 0) out += " \"WATTS\":" + QString("%1").arg(p->watts);
            if (p->cad > 0) out += " \"CAD\":" + QString("%1").arg(p->cad);
            if (p->hr > 0) out += " \"HR\":"  + QString("%1").arg(p->hr);
            if (p->secs > 0) out += " \"SECS\":" + QString("%1").arg(p->secs);

            // sample points in here!
            out += " }";
        }
        out +="\n\t\t]";
    }

    //
    // SAMPLES
    //
    if (ride->dataPoints().count()) {

        out += ",\n\t\t\"SAMPLES\":[\n";
        bool first = true;

        foreach (RideFilePoint *p, ride->dataPoints()) {

            if (first) first=false;
            else out += ",\n";

            out += "\t\t\t{ ";

            // always store time
            out += "\"SECS\":" + QString("%1").arg(p->secs);

            if (ride->areDataPresent()->km) out += ", \"KM\":" + QString("%1").arg(p->km);
            if (ride->areDataPresent()->watts && withWatts) out += ", \"WATTS\":" + QString("%1").arg(p->watts);
            if (ride->areDataPresent()->nm) out += ", \"NM\":" + QString("%1").arg(p->nm);
            if (ride->areDataPresent()->cad && withCad) out += ", \"CAD\":" + QString("%1").arg(p->cad);
            if (ride->areDataPresent()->kph) out += ", \"KPH\":" + QString("%1").arg(p->kph);
            if (ride->areDataPresent()->hr && withHr) out += ", \"HR\":"  + QString("%1").arg(p->hr);
            if (ride->areDataPresent()->alt && withAlt)
			    out += ", \"ALT\":" + QString("%1").arg(p->alt, 0, 'g', 11);
            if (ride->areDataPresent()->lat)
                out += ", \"LAT\":" + QString("%1").arg(p->lat, 0, 'g', 11);
            if (ride->areDataPresent()->lon)
                out += ", \"LON\":" + QString("%1").arg(p->lon, 0, 'g', 11);
            if (ride->areDataPresent()->headwind) out += ", \"HEADWIND\":" + QString("%1").arg(p->headwind);
            if (ride->areDataPresent()->slope) out += ", \"SLOPE\":" + QString("%1").arg(p->slope);
            if (ride->areDataPresent()->temp && p->temp != RideFile::NA) out += ", \"TEMP\":" + QString("%1").arg(p->temp);
            if (ride->areDataPresent()->lrbalance && p->lrbalance != RideFile::NA) out += ", \"LRBALANCE\":" + QString("%1").arg(p->lrbalance);
            if (ride->areDataPresent()->lte) out += ", \"LTE\":" + QString("%1").arg(p->lte);
            if (ride->areDataPresent()->rte) out += ", \"RTE\":" + QString("%1").arg(p->rte);
            if (ride->areDataPresent()->lps) out += ", \"LPS\":" + QString("%1").arg(p->lps);
            if (ride->areDataPresent()->rps) out += ", \"RPS\":" + QString("%1").arg(p->rps);
            if (ride->areDataPresent()->lpco) out += ", \"LPCO\":" + QString("%1").arg(p->lpco);
            if (ride->areDataPresent()->rpco) out += ", \"RPCO\":" + QString("%1").arg(p->rpco);
            if (ride->areDataPresent()->lppb) out += ", \"LPPB\":" + QString("%1").arg(p->lppb);
            if (ride->areDataPresent()->rppb) out += ", \"RPPB\":" + QString("%1").arg(p->rppb);
            if (ride->areDataPresent()->lppe) out += ", \"LPPE\":" + QString("%1").arg(p->lppe);
            if (ride->areDataPresent()->rppe) out += ", \"RPPE\":" + QString("%1").arg(p->rppe);
            if (ride->areDataPresent()->lpppb) out += ", \"LPPPB\":" + QString("%1").arg(p->lpppb);
            if (ride->areDataPresent()->rpppb) out += ", \"RPPPB\":" + QString("%1").arg(p->rpppb);
            if (ride->areDataPresent()->lpppe) out += ", \"LPPPE\":" + QString("%1").arg(p->lpppe);
            if (ride->areDataPresent()->rpppe) out += ", \"RPPPE\":" + QString("%1").arg(p->rpppe);
            if (ride->areDataPresent()->smo2) out += ", \"SMO2\":" + QString("%1").arg(p->smo2);
            if (ride->areDataPresent()->thb) out += ", \"THB\":" + QString("%1").arg(p->thb);
            if (ride->areDataPresent()->rcad) out += ", \"RCAD\":" + QString("%1").arg(p->rcad);
            if (ride->areDataPresent()->rvert) out += ", \"RVERT\":" + QString("%1").arg(p->rvert);
            if (ride->areDataPresent()->rcontact) out += ", \"RCON\":" + QString("%1").arg(p->rcontact);

            // sample points in here!
            out += " }";
        }
        out +="\n\t\t]";
    }

    //
    // XDATA
    //
    if (const_cast<RideFile*>(ride)->xdata().count()) {
        // output the xdata series
        out += ",\n\t\t\"XDATA\":[\n";

        bool first = true;
        QMapIterator<QString,XDataSeries*> xdata(const_cast<RideFile*>(ride)->xdata());
        xdata.toFront();
        while(xdata.hasNext()) {

            // iterate
            xdata.next();

            XDataSeries *series = xdata.value();

            // does it have values names?
            if (series->valuename.isEmpty()) continue;

            if (!first) out += ",\n";
            out += "\t\t{\n";

            // series name
            out += "\t\t\t\"NAME\" : \"" + xdata.key() + "\",\n";

            // value names
            if (series->valuename.count() > 1) {
                out += "\t\t\t\"VALUES\" : [ ";
                bool firstv=true;
                foreach(QString x, series->valuename) {
                    if (!firstv) out += ", ";
                    out += "\"" + x + "\"";
                    firstv=false;
                }
                out += " ]";
            } else {
                out += "\t\t\t\"VALUE\" : \"" + series->valuename[0] + "\"";
            }

            // unit names
            if (series->unitname.count() > 1) {
                out += ",\n\t\t\t\"UNITS\" : [ ";
                bool firstv=true;
                foreach(QString x, series->unitname) {
                    if (!firstv) out += ", ";
                    out += "\"" + x + "\"";
                    firstv=false;
                }
                out += " ]";
            } else {
                if (series->unitname.count() > 0) out += ",\n\t\t\t\"UNIT\" : \"" + series->unitname[0] + "\"";
            }

            // samples
            if (series->datapoints.count()) {
                out += ",\n\t\t\t\"SAMPLES\" : [\n";

                bool firsts=true;
                foreach(XDataPoint *p, series->datapoints) {
                    if (!firsts) out += ",\n";

                    // multi value sample
                    if (series->valuename.count()>1) {

                        out += "\t\t\t\t{ \"SECS\":"+QString("%1").arg(p->secs) +", "
                            + "\"KM\":"+QString("%1").arg(p->km) + ", "
                            + "\"VALUES\":[ ";

                        bool firstvv=true;
                        for(int i=0; i<series->valuename.count(); i++) {
                            if (!firstvv) out += ", ";
                            out += QString("%1").arg(p->number[i]);
                            firstvv=false;
                         }
                         out += " ] }";

                    } else {

                        out += "\t\t\t\t{ \"SECS\":"+QString("%1").arg(p->secs) + ", "
                            + "\"KM\":"+QString("%1").arg(p->km) + ", "
                            + "\"VALUE\":" + QString("%1").arg(p->number[0]) + " }";
                    }
                    firsts = false;
                }

                out += "\n\t\t\t]\n";
            } else {
                out += "\n";
            }

            out += "\t\t}";

            // now do next
            first = false;
        }

        out += "\n\t\t]";
    }

    // end of ride and document
    out += "\n\t}\n}\n";

    return out.toUtf8();
}

// Writes valid .json (validated at www.jsonlint.com)
bool
JsonFileReader::writeRideFile(Context *context, const RideFile *ride, QFile &file) const
{
    // can we open the file for writing?
    if (!file.open(QIODevice::WriteOnly)) return false;

    // truncate existing
    file.resize(0);

    QByteArray xml = toByteArray(context, ride, true, true, true, true);

    // setup streamer
    QTextStream out(&file);
    // unified codepage and BOM for identification on all platforms
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);

    out << xml;
    out.flush();

    // close
    file.close();

    return true;
}
