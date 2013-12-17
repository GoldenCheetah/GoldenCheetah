/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "SeasonParser.h"
#include <QDate>
#include <QDebug>

static inline QString unquote(QString quoted)
{
    return quoted.mid(1,quoted.length()-2);
}

bool SeasonParser::startDocument()
{
    buffer.clear();
    return true;
}

bool SeasonParser::endElement( const QString&, const QString&, const QString &qName )
{
    if(qName == "name")
        season.setName(buffer.trimmed());
    else if(qName == "startdate")
        season.setStart(seasonDateToDate(buffer.trimmed()));
    else if(qName == "enddate")
        season.setEnd(seasonDateToDate(buffer.trimmed()));
    else if (qName == "type")
        season.setType(buffer.trimmed().toInt());
    else if (qName == "id")
        season.setId(QUuid(buffer.trimmed()));
    else if (qName == "load") {
        season.load().resize(loadcount+1);
        season.load()[loadcount] = buffer.trimmed().toInt();
        loadcount++;
    } else if (qName == "low") {
        season.setLow(buffer.trimmed().toInt());
    } else if (qName == "seed") {
        season.setSeed(buffer.trimmed().toInt());
    } else if (qName == "event") {

        season.events.append(SeasonEvent(unquote(buffer.trimmed()), seasonDateToDate(dateString)));

    } else if(qName == "season") {

        if(seasons.size() >= 1) {
            // only set end date for previous season if
            // it is not null
            if (seasons[seasons.size()-1].getEnd() == QDate())
                seasons[seasons.size()-1].setEnd(season.getStart());
        }
        seasons.append(season);
    }
    return true;
}

bool SeasonParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
    buffer.clear();
    if(name == "season") {
        season = Season();
        loadcount = 0;
    }

    if (name == "event") {

        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "date") dateString=attrs.value(i);
        }
    }

    return true;
}

bool SeasonParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

QList<Season> SeasonParser::getSeasons()
{
    return seasons;
}

QDate SeasonParser::seasonDateToDate(QString seasonDate)
{
    QRegExp rx(".*([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9]$)");
        if (rx.exactMatch(seasonDate)) {
            QDate date = QDate(
                               rx.cap(1).toInt(),
                               rx.cap(2).toInt(),
                               rx.cap(3).toInt()
                               );

            if (! date.isValid()) {
                return QDate();
            }
            else
                return date;
        }
        else
            return QDate(); // return value was 1 Jan: changed to null
}
bool SeasonParser::endDocument()
{
    // Go 10 years into the future if not defined in the file
    if (seasons.size() > 0) {
        if (seasons[seasons.size()-1].getEnd() == QDate())
            seasons[seasons.size()-1].setEnd(QDate::currentDate().addYears(10));
    }
    return true;
}

bool
SeasonParser::serialize(QString filename, QList<Season>Seasons)
{
    // open file - truncate contents
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // begin document
    out << "<seasons>\n";

    // write out to file
    foreach (Season season, Seasons) {
        if (season.getType() != Season::temporary) {

            // main attributes
            out<<QString("\t<season>\n"
                  "\t\t<name>%1</name>\n"
                  "\t\t<startdate>%2</startdate>\n"
                  "\t\t<enddate>%3</enddate>\n"
                  "\t\t<type>%4</type>\n"
                  "\t\t<id>%5</id>\n"
                  "\t\t<seed>%6</seed>\n"
                  "\t\t<low>%7</low>\n") .arg(season.getName())
                                           .arg(season.getStart().toString("yyyy-MM-dd"))
                                           .arg(season.getEnd().toString("yyyy-MM-dd"))
                                           .arg(season.getType())
                                           .arg(season.id().toString())
                                           .arg(season.getSeed())
                                           .arg(season.getLow());
                                    

            // load profile
            for (int i=9; i<season.load().count(); i++)
                out <<QString("\t<load>%1</load>\n").arg(season.load()[i]);

            foreach(SeasonEvent x, season.events) {

                out<<QString("\t\t<event date=\"%1\">\"%2\"</event>")
                            .arg(x.date.toString("yyyy-MM-dd"))
                            .arg(x.name);
            
            }
            out <<QString("\t</season>\n");
        }
    }

    // end document
    out << "</seasons>\n";

    // close file
    file.close();

    return true; // success
}
