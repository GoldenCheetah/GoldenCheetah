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

version: VERSION ':' string                                     { /* V1.0 we don't do anything for this version */; }

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
                                                                     else if ($1 == "timestamp") jc->item.timestamp = $3.toULongLong();
                                                                     else if ($1 == "dbversion") jc->item.dbversion = $3.toInt();
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
