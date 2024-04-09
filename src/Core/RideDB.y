%{
/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "RideDB.h"
#include "RideFileCache.h"
#include "Settings.h"
#ifdef GC_WANT_HTTP
#include "APIWebService.h"
#endif

#define YYSTYPE QString

// Lex scanner
extern int RideDBlex(YYSTYPE*,void*); // the lexer aka yylex()
extern int RideDBlex_init(void**);
extern void RideDB_setString(QString, void *);
extern int RideDBlex_destroy(void*); // the cleaner for lexer

// yacc parser
void RideDBerror(void*jc, const char *error) // used by parser aka yyerror()
{ static_cast<RideDBContext*>(jc)->errors << error; }

// extract scanner from the context
#define scanner jc->scanner

%}

%pure-parser
%lex-param { void *scanner }
%parse-param { struct RideDBContext *jc }

%token STRING
%token VERSION RIDES METRICS TAGS XDATA INTERVALS

%start document
%%

/* We allow a .json file to be encapsulated within optional braces */
document: '{' elements '}'

elements: element
        | elements ',' element
        ;

element: version
        | RIDES ':' '[' ride_list ']';

version: VERSION ':' string                                     {
                                                                    if ($3 != RIDEDB_VERSION) {
                                                                        jc->old=true; 
                                                                        jc->item.isstale=true; // force refresh after load
                                                                    }
                                                                }

ride_list: ride
        | ride_list ',' ride
        ;

ride: '{' rideelement_list '}'                                  { 
                                                                    // search for one to update using serial search,
                                                                    // if the performance is too slow we can move to
                                                                    // a binary search, but suspect this ok < 10000 rides
                                                                    if (jc->api != NULL) {
                                                                    #ifdef GC_WANT_HTTP
                                                                        // we're listing rides in the api
                                                                        jc->api->writeRideLine(jc->item, jc->request, jc->response);
                                                                    #endif
                                                                    } else {

                                                                        int progress = static_cast<int>((double(jc->loading++) / double(jc->cache->rides().count()) * 100.0f) + 0.5);

                                                                        // limit the number of notifyLoadProgress updates to a maximum of 100.
                                                                        if (progress > jc->loadProgress) {

                                                                            jc->loadProgress = progress;
                                                                            jc->context->notifyLoadProgress(jc->folder,(jc->loadProgress));
                                                                        }

                                                                        // find entry and update it
                                                                        int index=jc->cache->find(&jc->item);
                                                                        if (index==-1)  qDebug()<<"unable to load:"<<jc->item.fileName<<jc->item.dateTime<<jc->item.weight;
                                                                        else  jc->cache->rides().at(index)->setFrom(jc->item);
                                                                    }

                                                                    // now set our ride item clean again, so we don't
                                                                    // overwrite with prior data
                                                                    jc->item.metadata().clear();
                                                                    jc->item.xdata().clear();
                                                                    jc->item.metrics().fill(0.0f);
                                                                    jc->item.counts().fill(0.0f);
                                                                    jc->item.stdmeans().clear();
                                                                    jc->item.stdvariances().clear();
                                                                    jc->interval.metrics().fill(0.0f);
                                                                    jc->interval.counts().fill(0.0f);
                                                                    jc->interval.stdmeans().clear();
                                                                    jc->interval.stdvariances().clear();
                                                                    jc->interval.route = QUuid();
                                                                    jc->item.clearIntervals();
                                                                    jc->item.overrides_.clear();
                                                                    jc->item.fileName = "";
                                                                    jc->count = "";
                                                                    jc->value = "";
                                                                }


rideelement_list: rideelement_list ',' rideelement              
                | rideelement                                   ;

rideelement: ride_tuple
            | metrics
            | tags
            | xdata
            | intervals
            ;

/*
 * RIDES
 */
                                                                /* RideItem state data */
ride_tuple: string ':' string                                   { 
                                                                     if ($1 == "filename") jc->item.fileName = $3;
                                                                     else if ($1 == "fingerprint") jc->item.fingerprint = $3.toULongLong();
                                                                     else if ($1 == "crc") jc->item.crc = $3.toULongLong();
                                                                     else if ($1 == "metacrc") jc->item.metacrc = $3.toULongLong();
                                                                     else if ($1 == "timestamp") jc->item.timestamp = $3.toULongLong();
                                                                     else if ($1 == "dbversion") jc->item.dbversion = $3.toInt();
                                                                     else if ($1 == "udbversion") jc->item.udbversion = $3.toInt();
                                                                     else if ($1 == "color") jc->item.color = QColor($3);
                                                                     else if ($1 == "aero") jc->item.isAero = $3.toInt() ? true: false;
                                                                     else if ($1 == "sport") {
                                                                         jc->item.sport = ($3);
                                                                         jc->item.isBike=jc->item.isRun=jc->item.isSwim=jc->item.isXtrain=jc->item.isAero=false;
                                                                         if ($3 == "Bike") jc->item.isBike = true;
                                                                         else if ($3 == "Run") jc->item.isRun = true;
                                                                         else if ($3 == "Swim") jc->item.isSwim = true;
                                                                         else if ($3 == "Aero") jc->item.isAero = true;
                                                                         else jc->item.isXtrain = true;
                                                                     }
                                                                     else if ($1 == "present") jc->item.present = $3;
                                                                     else if ($1 == "overrides") jc->item.overrides_ = $3.split(",");
                                                                     else if ($1 == "weight") jc->item.weight = $3.toDouble();
                                                                     else if ($1 == "samples") jc->item.samples = $3.toInt() > 0 ? true : false;
                                                                     else if ($1 == "zonerange") jc->item.zoneRange = $3.toInt();
                                                                     else if ($1 == "hrzonerange") jc->item.hrZoneRange = $3.toInt();
                                                                     else if ($1 == "pacezonerange") jc->item.paceZoneRange = $3.toInt();
                                                                     else if ($1 == "date") {
                                                                         QDateTime aslocal = QDateTime::fromString($3, DATETIME_FORMAT);
                                                                         QDateTime asUTC = QDateTime(aslocal.date(), aslocal.time(), Qt::UTC);
                                                                         jc->item.dateTime = asUTC.toLocalTime();
                                                                     }
                                                                }

/*
 * RIDE METRIC values
 */
metrics: METRICS ':' '{' metrics_list '}'                       ;
metrics_list: metric | metrics_list ',' metric ;
metric: metric_key ':' metric_value                             /* metric computed value */
                                                                {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric($1);
                                                                    if (m) {
                                                                        jc->item.metrics()[m->index()] = $3.toDouble();
                                                                        jc->item.counts()[m->index()] = 0; // we don't write zero values
                                                                    }
                                                                    else qDebug()<<"metric not found:"<<$1;
                                                                }
      | metric_key ':' '[' metric_value ',' metric_count ']'    {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric($1);
                                                                    if (m) {
                                                                        jc->item.metrics()[m->index()] = $4.toDouble();
                                                                        jc->item.counts()[m->index()] = $6.toDouble();
                                                                    }
                                                                    else qDebug()<<"metric not found:"<<$1;

                                                                }

      | metric_key ':' '[' metric_value ',' metric_count ',' metric_stdmean ',' metric_stdvariance ']'    {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric($1);
                                                                    if (m) {
                                                                        jc->item.metrics()[m->index()] = $4.toDouble();
                                                                        jc->item.counts()[m->index()] = $6.toDouble();
                                                                        jc->item.stdmeans().insert(m->index(), $8.toDouble());
                                                                        jc->item.stdvariances().insert(m->index(), $10.toDouble());
                                                                    }
                                                                    else qDebug()<<"metric not found:"<<$1;

                                                                }

metric_key : string                                             { jc->key = jc->JsonString; }
metric_value : string                                           { jc->value = jc->JsonString; }
metric_count : string                                           { jc->count = jc->JsonString; }
metric_stdmean : string                                         { jc->count = jc->JsonString; }
metric_stdvariance : string                                     { jc->count = jc->JsonString; }

/*
 * INTERVALS
 */

intervals: INTERVALS ':' '[' intervals_list ']'                 ;
intervals_list: intervals_list ',' interval
                | interval
                ;

interval: '{' intervalelement_list '}'                          {
                                                                     jc->item.addInterval(jc->interval);
                                                                    jc->interval.metrics().fill(0.0f);
                                                                    jc->interval.counts().fill(0.0f);
                                                                    jc->interval.stdmeans().clear();
                                                                    jc->interval.stdvariances().clear();

                                                                }

intervalelement_list: intervalelement_list ',' interval_element
                    | interval_element
                    ;

interval_element: interval_tuple
                  | interval_metrics
                  ;

                                                                /* IntervalItem state data */
interval_tuple: string ':' string                               { 
                                                                     if ($1 == "name") jc->interval.name = $3;
                                                                     else if ($1 == "start") jc->interval.start = $3.toDouble();
                                                                     else if ($1 == "stop") jc->interval.stop = $3.toDouble();
                                                                     else if ($1 == "startKM") jc->interval.startKM = $3.toDouble();
                                                                     else if ($1 == "stopKM") jc->interval.stopKM = $3.toDouble();
                                                                     else if ($1 == "type") jc->interval.type = static_cast<RideFileInterval::intervaltype>($3.toInt());
                                                                     else if ($1 == "color") jc->interval.color = QColor($3);
                                                                     else if ($1 == "seq") jc->interval.displaySequence = $3.toInt();
                                                                     else if ($1 == "route") jc->interval.route = QUuid($3);
                                                                     else if ($1 == "test") jc->interval.test = $3 == "true" ? true : false;
                                                                }

interval_metrics: METRICS ':' '{' interval_metrics_list '}'                       ;
interval_metrics_list: interval_metric | interval_metrics_list ',' interval_metric ;
interval_metric: interval_metric_key ':' interval_metric_value                             /* metric computed value */
                                                                { 
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric($1);
                                                                    if (m) {
                                                                        jc->interval.metrics()[m->index()] = $3.toDouble();
                                                                        jc->interval.counts()[m->index()] = 0; /* we don't write zeroes */
                                                                    } else qDebug()<<"metric not found:"<<$1;
                                                               }
               | interval_metric_key ':' '[' interval_metric_value ',' interval_metric_count ']'
                                                                {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric($1);
                                                                    if (m) {
                                                                        jc->interval.metrics()[m->index()] = $4.toDouble();
                                                                        jc->interval.counts()[m->index()] = $6.toDouble();
                                                                    } else qDebug()<<"metric not found:"<<$1;
                                                               }
               | interval_metric_key ':' '[' interval_metric_value ',' interval_metric_count ',' interval_metric_stdmean ',' interval_metric_stdvariance ']'
                                                                {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric($1);
                                                                    if (m) {
                                                                        jc->interval.metrics()[m->index()] = $4.toDouble();
                                                                        jc->interval.counts()[m->index()] = $6.toDouble();
                                                                        jc->interval.stdmeans().insert(m->index(), $8.toDouble());
                                                                        jc->interval.stdvariances().insert(m->index(), $10.toDouble());
                                                                    } else qDebug()<<"metric not found:"<<$1;
                                                               }
               ;

interval_metric_key : string                                    { jc->key = jc->JsonString; }
interval_metric_value : string                                  { jc->value = jc->JsonString; }
interval_metric_count : string                                  { jc->count = jc->JsonString; }
interval_metric_stdmean : string                                { jc->count = jc->JsonString; }
interval_metric_stdvariance : string                            { jc->count = jc->JsonString; }

/*
 * Metadata TAGS
 */
tags: TAGS ':' '{' tags_list '}'                                ;
tags_list: tag | tags_list ',' tag ;
tag: tag_key ':' tag_value                                      /* metadata value */
                                                                {
                                                                    jc->item.metadata().insert(jc->key, jc->value);
                                                                }

tag_key : string                                                { jc->key = jc->JsonString; }
tag_value : string                                              { jc->value = jc->JsonString; }

/*
 * XDATA definitions
 */

xdata: XDATA ':' '{' xdata_list '}'
xdata_list: xdata | xdata_list ',' xdata ;
xdata: xdata_name ':' '[' xdata_values ']'                      { jc->item.xdata().insert(jc->key,jc->JsonStringList);
                                                                  jc->key = ""; jc->JsonStringList.clear(); }

xdata_name : string                                             { jc->key = jc->JsonString; }
xdata_values:
    | xdata_value
    | xdata_value ',' xdata_values
    ;
xdata_value: string                                             { jc->JsonStringList << jc->JsonString; }

/*
 * Primitives
 */

string: STRING                                               { jc->JsonString = $1; }
        ;
%%


void 
RideCache::load()
{
    // only load if it exists !
    QFile rideDB(QString("%1/%2").arg(context->athlete->home->cache().canonicalPath()).arg("rideDB.json"));
    if (rideDB.exists() && rideDB.open(QFile::ReadOnly)) {

        QDir directory = context->athlete->home->activities();
        QDir plannedDirectory = context->athlete->home->planned();

        // ok, lets read it in

        // Read the entire file into a QString -- we avoid using fopen since it
        // doesn't handle foreign characters well. Instead we use QFile and parse
        // from a QString
        QString contents = QString(rideDB.readAll());
        rideDB.close();

        // create scanner context for reentrant parsing
        RideDBContext *jc = new RideDBContext;
        jc->context = context;
        jc->cache = this;
        jc->api = NULL;
        jc->old = false;
        jc->loading = 0;
        jc->loadProgress = 0;
        jc->folder = context->athlete->home->root().canonicalPath();

        // clean item
        jc->item.path = directory.canonicalPath(); // TODO use plannedDirectory for planned
        jc->item.context = context;
        jc->item.isstale = jc->item.isdirty = jc->item.isedit = false;

        RideDBlex_init(&scanner);

        // inform the parser/lexer we have a new file
        RideDB_setString(contents, scanner);

        // setup
        jc->errors.clear();

        // set to non-zero if you want to
        // to debug the yyparse() state machine
        // sending state transitions to stderr
        //yydebug = 0;

        // parse it
        RideDBparse(jc);

        // clean up
        RideDBlex_destroy(scanner);

        // regardless of errors we're done !
        delete jc;

        return;
    }
}

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

static QVector<int> mmp_durations;
static bool setup_mmp_durations()
{
    // 1s - 3mins every second
    for(int i=1; i<=180; i++) mmp_durations << i;

    // 3mins to 10mins every 5 seconds
    for(int i=185; i<=600; i+=5) mmp_durations << i;

    // 10mins to 30 mins every 30 seconds
    for(int i=630; i<=1800; i+=30) mmp_durations << i;

    // 30mins to 2hrs every minute
    for(int i=1860; i<=(120*60); i += 60) mmp_durations << i;

    // 2hrs+ every 30 mins
    for(int i=(120*60)+1800; i <= (10 * 60 * 60); i+=1800) mmp_durations << i;

    return true;
}
static bool did_mmp_durations = setup_mmp_durations();

// Fast constructon of common string. In the future we'll achieve this with a single variadic template function.
QString ConstructNameNumberString(QString s0, QString name, QString s1, double num, QString s2)
{
    QString numStr;                        
    numStr.setNum(num, 'f', 5);
    
    QString m;
    m.reserve(s0.length() + name.length() + s1.length() + numStr.length() + s2.length());
    
    m.append(s0);
    m.append(name);
    m.append(s1);
    m.append(numStr);
    m.append(s2);
    
    return m;
}

// Fast constructon of common string. In the future we'll achieve this with a single variadic template function.
QString ConstructNameNumberNumberString(QString s0, QString name, QString s1, double num0, QString s2, double num1, QString s3)
{
    QString num0Str;
    num0Str.setNum(num0, 'f', 5);

    QString num1Str;
    num1Str.setNum(num1, 'f', 5);
    
    QString m;
    m.reserve(s0.length() + name.length() + s1.length() + num0Str.length() + s2.length() + num1Str.length() + s3.length());
    
    m.append(s0);
    m.append(name);
    m.append(s1);
    m.append(num0Str);
    m.append(s2);
    m.append(num1Str);
    m.append(s3);
    
    return m;
}


// save cache to disk
//
// if opendata is true then save in format for sending to the GC OpenData project
// the filename may be supplied if exporting for other purposes, if empty then save
// to ~athlete/cache/rideDB.json
//
// When writing for opendata the file this doesn't (and must not) contain PII or
// metadata, but does include some distributions for Heartrate, Power, Cadence
// and Speed along with MMP data. We need to consider what athlete info to add
// possibly only gender and year of birth.
//
// it must be valid json and can be parse with python using
//      import json
//      with open('rideDB.json') as json_data:
//          d = json.load(json_data)
//      print(len(d["RIDES"])
//
void RideCache::save(bool opendata, QString filename)
{

    // now save data away - use passed filename if set
    QFile rideDB(QString("%1/%2").arg(context->athlete->home->cache().canonicalPath()).arg("rideDB.json"));
    if (filename != "") rideDB.setFileName(filename);

    if (rideDB.open(QFile::WriteOnly)) {

        const RideMetricFactory &factory = RideMetricFactory::instance();

        // ok, lets write out the cache
        QTextStream stream(&rideDB);
#if QT_VERSION < 0x060000
        stream.setCodec("UTF-8");
#endif

        // no BOM needed for opendata as it doesn't contain textual data
        if (!opendata) stream.setGenerateByteOrderMark(true);

        stream << "{" ;
        stream << QString("\n  \"VERSION\":\"%1\",").arg(RIDEDB_VERSION);

        // send very basic athlete info with no PII
        // year of birth, gender, uuid
        if (opendata) {
            stream << QString("\n  \"ATHLETE\":{ \"gender\":\"%1\", \"yob\":\"%2\", \"id\":\"%3\" },")
                        .arg(appsettings->cvalue(context->athlete->cyclist, GC_SEX).toInt() == 0 ? "M" : "F")
                        .arg(appsettings->cvalue(context->athlete->cyclist, GC_DOB).toDate().year())
                        .arg(context->athlete->id.toString());
        }

        stream << "\n  \"RIDES\":[\n";

        bool firstRide = true;
        foreach(RideItem *item, rides()) {

            // skip if not loaded/refreshed, a special case
            // if saving during an initial refresh
            if (item->metrics().count() == 0) continue;

            // don't save files with discarded changes at exit
            if (item->skipsave == true) continue;

            // comma separate each ride
            if (!firstRide) stream << ",\n";
            firstRide = false;

            // basic ride information
            stream << "\t{\n";
            stream << "\t\t\"date\":\"" <<item->dateTime.toUTC().toString(DATETIME_FORMAT) << "\",\n";

            if (!opendata) {
                // we don't send this info when sharing as opendata
                stream << "\t\t\"filename\":\"" <<item->fileName <<"\",\n";
                stream << "\t\t\"fingerprint\":\"" <<item->fingerprint <<"\",\n";
                stream << "\t\t\"crc\":\"" <<item->crc <<"\",\n";
                stream << "\t\t\"metacrc\":\"" <<item->metacrc <<"\",\n";
                stream << "\t\t\"timestamp\":\"" <<item->timestamp <<"\",\n";
                stream << "\t\t\"dbversion\":\"" <<item->dbversion <<"\",\n";
                stream << "\t\t\"udbversion\":\"" <<item->udbversion <<"\",\n";
                stream << "\t\t\"color\":\"" <<item->color.name() <<"\",\n";
                stream << "\t\t\"present\":\"" <<item->present <<"\",\n";
                stream << "\t\t\"sport\":\"" <<item->sport <<"\",\n";
                stream << "\t\t\"aero\":\"" << (item->isAero ? "1" : "0") <<"\",\n";
                stream << "\t\t\"weight\":\"" <<item->weight <<"\",\n";

                if (item->zoneRange >= 0) stream << "\t\t\"zonerange\":\"" <<item->zoneRange <<"\",\n";
                if (item->hrZoneRange >= 0) stream << "\t\t\"hrzonerange\":\"" <<item->hrZoneRange <<"\",\n";
                if (item->paceZoneRange >= 0) stream << "\t\t\"pacezonerange\":\"" <<item->paceZoneRange <<"\",\n";

                // if there are overrides, do share them
                if (item->overrides_.count()) stream << "\t\t\"overrides\":\"" <<item->overrides_.join(",") <<"\",\n";

                stream << "\t\t\"samples\":\"" <<(item->samples ? "1" : "0") <<"\",\n";
            } else {

                // need to know what data was collected
                stream << "\t\t\"data\":\"" <<item->getText("Data","") <<"\",\n";
                stream << "\t\t\"sport\":\"" <<item->getText("Sport","") <<"\",\n";
            }

            // pre-computed metrics
            stream << "\n\t\t\"METRICS\":{\n";

            bool firstMetric = true;
            for(int i=0; i<factory.metricCount(); i++) {
                QString name = factory.metricName(i);
                int index = factory.rideMetric(name)->index();

                // don't output 0, nan or inf values, they're set to 0 by default
                // unless aggregateZero indicates the count is relevant
                if ((!std::isinf(item->metrics()[index]) && !std::isnan(item->metrics()[index]) &&
                    (item->metrics()[index] > 0.00f || item->metrics()[index] < 0.00f)) ||
                    (item->metrics()[index] == 0.00f && item->counts()[index] > 1.0 && factory.rideMetric(name)->aggregateZero())) {
                    if (!firstMetric) stream << ",\n";
                    firstMetric = false;

                    // if stdmean or variance is non-zero we write all 4
                    if (item->stdmeans().value(index, 0.0f) || item->stdvariances().value(index, 0.0f)) {

                        stream << "\t\t\t\"" << name << "\":[\"" << QString("%1").arg(item->metrics()[index], 0, 'f', 5) <<"\",\""
                                                                   << QString("%1").arg(item->counts()[index], 0, 'f', 5) << "\",\""
                                                                   << QString("%1").arg(item->stdmeans().value(index, 0.0f), 0, 'f', 5) << "\",\""
                                                                   << QString("%1").arg(item->stdvariances().value(index, 0.0f), 0, 'f', 5) <<"\"]";
                    } else if (item->counts()[index] == 0) {
                        // if count is 0 don't write it
                        stream << ConstructNameNumberString(QString("\t\t\t\""), name,
                            QString("\":\""), item->metrics()[index], QString("\""));
                    } else {
                        stream << ConstructNameNumberNumberString(QString("\t\t\t\""), name,
                            QString("\":[\""), item->metrics()[index], QString("\",\""), item->counts()[index], QString("\"]"));
                    }
                }
            }

            // if opendata lets put in the distributions for power, hr, cadence and speed
            if (opendata) {

                QString data = item->getText("Data","");

                if (data.contains("P") || data.contains("H") || data.contains("S") || data.contains("C")) {

                    // get the cache -- may refresh if its out of data, so this might take a while ....
                    RideFileCache *cache =  new RideFileCache(context, item->fileName, item->weight, NULL, false, true);

                    QList<RideFile::SeriesType> list;
                    if (data.contains("P")) list <<RideFile::watts;
                    if (data.contains("H")) list <<RideFile::hr;
                    if (data.contains("S")) list <<RideFile::kph;
                    if (data.contains("C")) list <<RideFile::cad;

                    // output distribution for each series
                    foreach(RideFile::SeriesType x, list) {

                        QVector<double> &array = cache->distributionArray(x);
                        int split= x==RideFile::cad ? 5 : 10; // set bin size to use
                        int div=   x==RideFile::kph ? 10 : 1; // speed needs to be divided by 10 to get kph

                        // distribution arrays:
                        // power_dist_bins:[ n1, n2, n3 ... ],
                        // power_dist:[ p1, p2, p3 ... ]
                        //
                        // the bins are always 10w wide, so we aggregate the data from
                        // the ridefilecache before outputting the bin starts and values
                        QVector<int> bins, totals;

                        // lets aggregates
                        int count=0;
                        double total=0;
                        for(int i=0; i< array.count(); i++) {

                            // save value away
                            if (count==split) {
                                if (total > 0) { // need a value !
                                    bins << i-split;
                                    totals << total;
                                }
                                count=0;
                                total=0;
                            }

                            total += array[i];
                            count++;
                        }

                        // add last partial bin
                        if (total > 0 && count > 0) {
                            bins << array.count()-count;
                            totals << total;
                        }

                        // series name
                        QString type=RideFile::seriesName(x, true).toLower();

                        // now write the distribution and the bins
                        if (bins.count()) {

                            // totals
                            stream << ",\n\t\t\t\""<<type<<"_dist\":[" ;
                            for(int i=0; i < totals.count(); i++) {
                                stream<< QString("%1").arg(totals[i]);
                                if (i+1 != bins.count()) stream<<", ";
                            }
                            stream << " ]";

                            // bins
                            stream << ",\n\t\t\t\""<<type<<"_dist_bins\":[" ;
                            for(int i=0; i < bins.count(); i++) {
                                stream<< QString("%1").arg(bins[i] > 0 ? bins[i] / div : 0);
                                if (i+1 != bins.count()) stream<<", ";
                            }
                            stream << " ]";

                        }
                    }

                    // MMP - We want to try and keep some of the resolution as this is likely to be filtered
                    //       and/or aggregated in some fashion by the user. So we choose to keep resolution
                    //       high at short durations and low at long durations with the points at which the
                    //       resolution degrades at some point after a physiological boundary.
                    //
                    // power_mmp - the mean max values
                    // power_mmp_secs - the mean max durations
                    if (data.contains("P")) {

                        QVector<double> &array = cache->meanMaxArray(RideFile::watts);

                        if (array.count() > 0) {

                            // power_mmp
                            stream << ",\n\t\t\t\"power_mmp\":[" ;
                            for (int i=0; i<mmp_durations.count() && mmp_durations[i] < array.count(); i++) {
                                if (i>0) stream << ", "; // for all but first value add comma
                                stream<< QString("%1").arg(array[mmp_durations[i]], 0, 'f', 0);
                            }
                            stream << " ]";

                            // power_mmp_secs
                            stream << ",\n\t\t\t\"power_mmp_secs\":[" ;
                            for (int i=0; i<mmp_durations.count() && mmp_durations[i] < array.count(); i++) {
                                if (i>0) stream << ", "; // for all but first value add comma
                                stream<< QString("%1").arg(mmp_durations[i]);
                            }
                            stream << " ]";
                        }
                    }
                    delete cache;
                }

            }
            stream << "\n\t\t}";

            // pre-loaded metadata - don't send to OpenData
            if (!opendata && item->metadata().count()) {

                stream << ",\n\t\t\"TAGS\":{\n";

                QMap<QString,QString>::const_iterator i;
                for (i=item->metadata().constBegin(); i != item->metadata().constEnd(); i++) {

                    stream << "\t\t\t\"" << i.key() << "\":\"" << protect(i.value()) << "\"";
                    if (std::next(i) != item->metadata().constEnd()) stream << ",\n";
                    else stream << "\n";
                }

                // end of the tags
                stream << "\n\t\t}";

            }

            // xdata definitions
            if (item->xdata().count()) {
                stream << ",\n\t\t\"XDATA\":{\n";

                QMap<QString, QStringList>::const_iterator i;
                for (i=item->xdata().constBegin(); i != item->xdata().constEnd(); i++) {

                    stream << "\t\t\t\"" << i.key() << "\":[ ";
                    bool first=true;
                    foreach(QString x, i.value()) {
                        if (!first) {
                            stream << ", ";
                        }
                        stream << "\"" << protect(x) << "\"";
                        first=false;
                    }

                    if (std::next(i) != item->xdata().constEnd()) stream << "],\n";
                    else stream << "]\n";
                }

                // end of the xdata
                stream << "\n\t\t}";
            }

            // intervals - but not for opendata
            if (!opendata && item->intervals().count()) {

                stream << ",\n\t\t\"INTERVALS\":[\n";
                bool firstInterval = true;
                foreach(IntervalItem *interval, item->intervals()) {

                    // comma separate
                    if (!firstInterval) stream << ",\n";
                    firstInterval = false;

                    stream << "\t\t\t{\n";

                    // interval main data 
                    stream << "\t\t\t\"name\":\"" << protect(interval->name) <<"\",\n";
                    stream << "\t\t\t\"start\":\"" << interval->start <<"\",\n";
                    stream << "\t\t\t\"stop\":\"" << interval->stop <<"\",\n";
                    stream << "\t\t\t\"startKM\":\"" << interval->startKM <<"\",\n";
                    stream << "\t\t\t\"stopKM\":\"" << interval->stopKM <<"\",\n";
                    stream << "\t\t\t\"type\":\"" << static_cast<int>(interval->type) <<"\",\n";
                    stream << "\t\t\t\"test\":\"" << (interval->test ? "true" : "false") <<"\",\n";
                    stream << "\t\t\t\"color\":\"" << interval->color.name() <<"\",\n";

                    // routes have a segment identifier
                    if (interval->type == RideFileInterval::ROUTE) {
                        stream << "\t\t\t\"route\":\"" << interval->route.toString() <<"\",\n"; // last one no ',\n' see METRICS below..
                    }

                    stream << "\t\t\t\"seq\":\"" << interval->displaySequence <<"\""; // last one no ',\n' see METRICS below..


                    // check if we have any non-zero metrics
                    bool hasMetrics=false;
                    foreach(double v, interval->metrics()) {
                        if (v > 0.00f || v < 0.00f) {
                            hasMetrics=true;
                            break;
                        }
                    }

                    if (hasMetrics) {
                        stream << ",\n\n\t\t\t\"METRICS\":{\n";

                        bool firstMetric = true;
                        for(int i=0; i<factory.metricCount(); i++) {
                            QString name = factory.metricName(i);
                            int index = factory.rideMetric(name)->index();
        
                            // don't output 0 values, they're set to 0 by default
                            // unless aggregateZero indicates the count is relevant
                            if ((interval->metrics()[index] > 0.00f || interval->metrics()[index] < 0.00f) ||
                                (item->metrics()[index] == 0.00f && item->counts()[i] > 1.0 && factory.rideMetric(name)->aggregateZero())) {
                                if (!firstMetric) stream << ",\n";
                                firstMetric = false;

                                if (interval->stdmeans().value(index, 0.0f) || interval->stdvariances().value(index, 0.0f)) {

                                    stream << "\t\t\t\t\"" << name << "\": [ \"" << QString("%1").arg(interval->metrics()[index], 0, 'f', 5) <<"\",\""
                                                                               << QString("%1").arg(interval->counts()[index], 0, 'f', 5) << "\",\""
                                                                               << QString("%1").arg(interval->stdmeans().value(index, 0.0f), 0, 'f', 5) << "\",\""
                                                                               << QString("%1").arg(interval->stdvariances().value(index, 0.0f), 0, 'f', 5) <<"\"]";

                                // if count is 0 don't write it
                                } else if (interval->counts()[index] == 0) {
                                    stream << ConstructNameNumberString(QString("\t\t\t\""), name,
                                        QString("\":\""), interval->metrics()[index], QString("\""));
                                } else {
                                    stream << ConstructNameNumberNumberString(QString("\t\t\t\""), name,
                                        QString("\":[\""), interval->metrics()[index], QString("\",\""), interval->counts()[index], QString("\"]"));
                                }
                            }
                        }
                        stream << "\n\t\t\t\t}";
                    }

                    // endof interval
                    stream << "\n\t\t\t}";
                }
                // end of intervals
                stream <<"\n\t\t]";

            }


            // end of the ride
            stream << "\n\t}";
        }

        stream << "\n  ]\n}";

        rideDB.close();
    }
}

#ifdef GC_WANT_HTTP
#include "RideMetadata.h"

void
APIWebService::listRides(QString athlete, HttpRequest &request, HttpResponse &response)
{
    listRideSettings settings;

    // the ride db
    QString ridedb = QString("%1/%2/cache/rideDB.json").arg(home.absolutePath()).arg(athlete);
    QFile rideDB(ridedb);

    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    // not known..
    if (!rideDB.exists()) {
        response.setStatus(404);
        response.write("malformed URL or unknown athlete.\n");
        return;
    }

    // intervals or rides?
    QString intervalsp = request.getParameter("intervals");
    if (intervalsp.toUpper() == "TRUE") settings.intervals = true;
    else settings.intervals = false;

    // set user data
    response.setUserData(&settings);

    // write headings
    const RideMetricFactory &factory = RideMetricFactory::instance();
    QVector<const RideMetric *> indexed(factory.metricCount());

    // get metrics indexed in same order as the array
    foreach(QString name, factory.allMetrics()) {

        const RideMetric *m = factory.rideMetric(name);
        indexed[m->index()] = m;
    }

    // was the metric parameter passed?
    QString metrics(request.getParameter("metrics"));

    // do all ?
    bool nometrics = false;
    QStringList wantedNames;
    if (metrics != "") wantedNames = metrics.split(",");

    // write headings
    response.bwrite("date, time, filename");

    // don't want metrics, so do it fast by traversing the ride directory
    if (wantedNames.count() == 1 && wantedNames[0].toUpper() == "NONE") nometrics = true;

    // if intervals, add interval name
    if (settings.intervals == true) response.bwrite(", interval name, interval type");

    // get metadata definitions into settings
    QString metadata = request.getParameter("metadata");
    bool nometa = true;
    if (metadata.toUpper() != "NONE" && metadata != "") {

        // first lets read in meta config
        QString metaConfig = home.canonicalPath()+"/metadata.xml";
        if (!QFile(metaConfig).exists()) metaConfig = ":/xml/metadata.xml";

        // params to readXML - we ignore them
        QList<KeywordDefinition> keywordDefinitions;
        QString colorfield;
        QList<DefaultDefinition> defaultDefinitions;

        RideMetadata::readXML(metaConfig, keywordDefinitions, settings.metafields, colorfield, defaultDefinitions);

        SpecialFields sp;

        // what is being asked for ?
        if (metadata.toUpper() == "ALL") {

            // output all metadata
            foreach(FieldDefinition field, settings.metafields) {
                // don't do metric overrides !
                if(!sp.isMetric(field.name)) settings.metawanted << field.name;
            }

        } else {

            // selected fields - check they exist !
            QStringList meta = metadata.split(",");
            foreach(QString field, meta) {

                // don't do metric overrides !
                if(sp.isMetric(field)) continue;

                // does it exist ?
                QString lookup = field;
                lookup.replace("_", " ");
                foreach(FieldDefinition field, settings.metafields) {
                    if (field.name == lookup) settings.metawanted << field.name;
                }
            }
        }

        // we found some?
        if(settings.metawanted.count()) nometa = false;
    }

    // list 'em by reading the ride cache from disk
    if ((nometa == false || nometrics == false) && settings.intervals == false) {

        int i=0;
        foreach(const RideMetric *m, indexed) {

            i++;

            // if limited don't do limited headings
            QString underscored = m->name().replace(" ","_");
            if (wantedNames.count() && !wantedNames.contains(underscored)) continue;

            if (m->name().startsWith("BikeScore"))
                response.bwrite(", BikeScore");
            else {
                response.bwrite(", ");
                response.bwrite(underscored.toLocal8Bit());
            }

            // index of wanted metrics
            settings.wanted << (i-1);
        }

        // do we want metadata too ?
        foreach(QString meta, settings.metawanted) {
            meta.replace(" ", "_");
            response.bwrite(", \"");
            response.bwrite(meta.toLocal8Bit());
            response.bwrite("\"");
        }
        response.bwrite("\n");

        // parse the rideDB and write a line for each entry
        if (rideDB.exists() && rideDB.open(QFile::ReadOnly)) {

            // ok, lets read it in
            QTextStream stream(&rideDB);
#if QT_VERSION < 0x060000
            stream.setCodec("UTF-8");
#endif

            // Read the entire file into a QString -- we avoid using fopen since it
            // doesn't handle foreign characters well. Instead we use QFile and parse
            // from a QString
            QString contents = stream.readAll();
            rideDB.close();

            // create scanner context for reentrant parsing
            RideDBContext *jc = new RideDBContext;
            jc->cache = NULL;
            jc->api = this;
            jc->response = &response;
            jc->request = &request;
            jc->old = false;

            // clean item
            jc->item.path = home.absolutePath() + "/activities";
            jc->item.context = NULL;
            jc->item.isstale = jc->item.isdirty = jc->item.isedit = false;

            RideDBlex_init(&scanner);

            // inform the parser/lexer we have a new file
            RideDB_setString(contents, scanner);

            // setup
            jc->errors.clear();

            // parse it
            RideDBparse(jc);

            // clean up
            RideDBlex_destroy(scanner);

            // regardless of errors we're done !
            delete jc;
        }

    } else {

        // honour the since parameter
        QString sincep(request.getParameter("since"));
        QDate since(1900,01,01);
        if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");

        // before parameter
        QString beforep(request.getParameter("before"));
        QDate before(3000,01,01);
        if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");

        // fast list of rides by traversing the directory
        response.bwrite("\n"); // headings have no metric columns

        // This will read the user preferences and change the file list order as necessary:
        QFlags<QDir::Filter> spec = QDir::Files;
        QStringList names;
        names << "*"; // anything

        // loop through files, make sure in time range wanted
        QDir activities(home.absolutePath() + "/" + athlete + "/activities");
        foreach(QString name, activities.entryList(names, spec, QDir::Name)) {

            // parse it into date and time
            QDateTime dateTime;
            if (!RideFile::parseRideFileName(name, &dateTime)) continue; 

            // in range?
            if (dateTime.date() < since || dateTime.date() > before) continue;

            // is it a backup ?
            if (name.endsWith(".bak")) continue;

            // out a line
            response.bwrite(dateTime.date().toString("yyyy/MM/dd").toLocal8Bit());
            response.bwrite(", ");
            response.bwrite(dateTime.time().toString("hh:mm:ss").toLocal8Bit());;
            response.bwrite(", ");
            response.bwrite(name.toLocal8Bit());
            response.bwrite("\n");
        }
    }
    response.flush();
}
#endif
