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

// using context (we are reentrant)
struct RideDBContext {

    // the cache
    RideCache *cache;

    // the scanner
    void *scanner;

    // Set during parser processing, using same
    // naming conventions as yacc/lex -p
    RideItem item;

    // term state data is held in these variables
    QString JsonString;
    QString key, value;
    QStringList errors;

    // is cache/rideDB.json an older version ?
    bool old;
};

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
%token VERSION RIDES METRICS TAGS

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
                                                                    bool found = false;
                                                                    foreach(RideItem *i, jc->cache->rides()) {
                                                                        if (i->fileName == jc->item.fileName) {

                                                                            found = true;

                                                                            // update from our loaded value
                                                                            i->setFrom(jc->item);
                                                                            break;
                                                                        }
                                                                    }
                                                                    // not found !
                                                                    if (found == false)
                                                                        qDebug()<<"unable to load:"<<jc->item.fileName<<jc->item.dateTime<<jc->item.weight;

                                                                    // now set our ride item clean again, so we don't
                                                                    // overwrite with prior data
                                                                    jc->item.metadata().clear();
                                                                    jc->item.metrics().fill(0.0f);
                                                                    jc->item.fileName = "";
                                                                }


rideelement_list: rideelement_list ',' rideelement              
                | rideelement                                   ;

/*
 * Ride state data
 */
rideelement: ride_tuple
            | metrics
            | tags
            ;

                                                                /* RideItem state data */
ride_tuple: string ':' string                                   { 
                                                                     if ($1 == "filename") jc->item.fileName = $3;
                                                                     else if ($1 == "fingerprint") jc->item.fingerprint = $3.toULongLong();
                                                                     else if ($1 == "crc") jc->item.crc = $3.toULongLong();
                                                                     else if ($1 == "metacrc") jc->item.metacrc = $3.toULongLong();
                                                                     else if ($1 == "timestamp") jc->item.timestamp = $3.toULongLong();
                                                                     else if ($1 == "dbversion") jc->item.dbversion = $3.toInt();
                                                                     else if ($1 == "color") jc->item.color = QColor($3);
                                                                     else if ($1 == "isRun") jc->item.isRun = $3.toInt();
                                                                     else if ($1 == "isSwim") jc->item.isSwim = $3.toInt();
                                                                     else if ($1 == "present") jc->item.present = $3;
                                                                     else if ($1 == "weight") jc->item.weight = $3.toDouble();
                                                                     else if ($1 == "date") {
                                                                         QDateTime aslocal = QDateTime::fromString($3, DATETIME_FORMAT);
                                                                         QDateTime asUTC = QDateTime(aslocal.date(), aslocal.time(), Qt::UTC);
                                                                         jc->item.dateTime = asUTC.toLocalTime();
                                                                     }
                                                                }

/*
 * METRIC values
 */
metrics: METRICS ':' '{' metrics_list '}'                       ;
metrics_list: metric | metrics_list ',' metric ;
metric: metric_key ':' metric_value                             /* metric computed value */
                                                                {
                                                                    const RideMetric *m = RideMetricFactory::instance().rideMetric($1);
                                                                    if (m) jc->item.metrics()[m->index()] = $3.toDouble();
                                                                    else qDebug()<<"metric not found:"<<$1;
                                                                }

metric_key : string                                             { jc->key = jc->JsonString; }
metric_value : string                                           { jc->value = jc->JsonString; }

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
 * Primitives
 */

string: STRING                                               { jc->JsonString = $1; }
        ;
%%


void 
RideCache::load()
{
    // only load if it exists !
    QFile rideDB(QString("%1/rideDB.json").arg(context->athlete->home->cache().canonicalPath()));
    if (rideDB.exists() && rideDB.open(QFile::ReadOnly)) {

        // ok, lets read it in
        QTextStream stream(&rideDB);
        stream.setCodec("UTF-8");

        // Read the entire file into a QString -- we avoid using fopen since it
        // doesn't handle foreign characters well. Instead we use QFile and parse
        // from a QString
        QString contents = stream.readAll();
        rideDB.close();

        // create scanner context for reentrant parsing
        RideDBContext *jc = new RideDBContext;
        jc->cache = this;
        jc->old = false;

        // clean item
        jc->item.path = context->athlete->home->activities().canonicalPath();
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

// save cache to disk, "cache/rideDB.json"
void RideCache::save()
{

    // now save data away
    QFile rideDB(QString("%1/rideDB.json").arg(context->athlete->home->cache().canonicalPath()));
    if (rideDB.open(QFile::WriteOnly)) {

        const RideMetricFactory &factory = RideMetricFactory::instance();

        // ok, lets write out the cache
        QTextStream stream(&rideDB);
        stream.setCodec("UTF-8");
        stream.setGenerateByteOrderMark(true);

        stream << "{" ;
        stream << QString("\n  \"VERSION\":\"%1\",").arg(RIDEDB_VERSION);
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
            stream << "\t\t\"filename\":\"" <<item->fileName <<"\",\n";
            stream << "\t\t\"date\":\"" <<item->dateTime.toUTC().toString(DATETIME_FORMAT) << "\",\n";
            stream << "\t\t\"fingerprint\":\"" <<item->fingerprint <<"\",\n";
            stream << "\t\t\"crc\":\"" <<item->crc <<"\",\n";
            stream << "\t\t\"metacrc\":\"" <<item->metacrc <<"\",\n";
            stream << "\t\t\"timestamp\":\"" <<item->timestamp <<"\",\n";
            stream << "\t\t\"dbversion\":\"" <<item->dbversion <<"\",\n";
            stream << "\t\t\"color\":\"" <<item->color.name() <<"\",\n";
            stream << "\t\t\"present\":\"" <<item->present <<"\",\n";
            stream << "\t\t\"isRun\":\"" <<item->isRun <<"\",\n";
            stream << "\t\t\"isSwim\":\"" <<item->isSwim <<"\",\n";
            stream << "\t\t\"weight\":\"" <<item->weight <<"\",\n";

            // pre-computed metrics
            stream << "\n\t\t\"METRICS\":{\n";

            bool firstMetric = true;
            for(int i=0; i<factory.metricCount(); i++) {
                QString name = factory.metricName(i);
                int index = factory.rideMetric(name)->index();

                if (!firstMetric) stream << ",\n";
                firstMetric = false;

                stream << "\t\t\t\"" << name << "\":\"" << QString("%1").arg(item->metrics()[index], 0, 'f', 5) <<"\"";
            }
            stream << "\n\t\t}";

            // pre-loaded metadata
            if (item->metadata().count()) {

                stream << ",\n\t\t\"TAGS\":{\n";

                QMap<QString,QString>::const_iterator i;
                for (i=item->metadata().constBegin(); i != item->metadata().constEnd(); i++) {

                    stream << "\t\t\t\"" << i.key() << "\":\"" << protect(i.value()) << "\"";
                    if (i+1 != item->metadata().constEnd()) stream << ",\n";
                    else stream << "\n";
                }

                // end of the tags
                stream << "\n\t\t}";

            }

            // end of the ride
            stream << "\n\t}";
        }

        stream << "\n  ]\n}";

        rideDB.close();
    }
}

