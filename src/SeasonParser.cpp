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
#include <assert.h>

bool SeasonParser::startDocument()
{
    buffer.clear();
    return TRUE;
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
    else if(qName == "season")
    {
        if(seasons.size() >= 1) {
            // only set end date for previous season if
            // it is not null
            if (seasons[seasons.size()-1].getEnd() == QDate())
                seasons[seasons.size()-1].setEnd(season.getStart());
        }
        seasons.append(season);
    }
    return TRUE;
}

bool SeasonParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes & )
{
    buffer.clear();
    if(name == "season")
        season = Season();

    return TRUE;
}

bool SeasonParser::characters( const QString& str )
{
    buffer += str;
    return TRUE;
}

QList<Season> SeasonParser::getSeasons()
{
    return seasons;
}

QDate SeasonParser::seasonDateToDate(QString seasonDate)
{
    QRegExp rx(".*([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9]$)");
        if (rx.exactMatch(seasonDate)) {
            assert(rx.numCaptures() == 3);
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
    return TRUE;
}

bool
SeasonParser::serialize(QString filename, QList<Season>Seasons)
{
    // open file - truncate contents
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);

    // Character set encoding added to support international characters in names
    out.setCodec("UTF-8");
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    // begin document
    out << "<seasons>\n";

    // write out to file
    foreach (Season season, Seasons) {
        if (season.getType() != Season::temporary) {
            out<<QString("\t<season>\n"
                  "\t\t<name>%1</name>\n"
                  "\t\t<startdate>%2</startdate>\n"
                  "\t\t<enddate>%3</enddate>\n"
                  "\t\t<type>%4</type>\n"
                  "\t</season>\n")
            .arg(season.getName())
            .arg(season.getStart().toString("yyyy-MM-dd"))
            .arg(season.getEnd().toString("yyyy-MM-dd"))
            .arg(season.getType());
        }
    }

    // end document
    out << "</seasons>\n";

    // close file
    file.close();

    return true; // success
}
