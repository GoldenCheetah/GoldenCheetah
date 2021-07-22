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
#include "Utils.h"
#include <QDate>
#include <QDebug>
#include <QMessageBox>

bool SeasonParser::startDocument()
{
    buffer.clear();
    return true;
}

bool SeasonParser::endElement( const QString&, const QString&, const QString &qName )
{
    if(qName == "name") {
        if (isPhase)
            phase.setName(Utils::unprotect(buffer));
        else
            season.setName(Utils::unprotect(buffer));
    } else if(qName == "startdate") {
        if (isPhase)
            phase.setStart(seasonDateToDate(buffer.trimmed()));
        else
            season.setStart(seasonDateToDate(buffer.trimmed()));
    } else if(qName == "enddate") {
        if (isPhase)
            phase.setEnd(seasonDateToDate(buffer.trimmed()));
        else
            season.setEnd(seasonDateToDate(buffer.trimmed()));
    } else if (qName == "type") {
        if (isPhase)
            phase.setType(buffer.trimmed().toInt());
        else
            season.setType(buffer.trimmed().toInt());
    } else if (qName == "id") {
        if (isPhase)
            phase.setId(QUuid(buffer.trimmed()));
        else
            season.setId(QUuid(buffer.trimmed()));
    } else if (qName == "load") {
        season.load().resize(loadcount+1);
        season.load()[loadcount] = buffer.trimmed().toInt();
        loadcount++;
    } else if (qName == "low") {
        if (isPhase)
            phase.setLow(buffer.trimmed().toInt());
        else
            season.setLow(buffer.trimmed().toInt());
    } else if (qName == "seed") {
        if (isPhase)
            phase.setSeed(buffer.trimmed().toInt());
        else
            season.setSeed(buffer.trimmed().toInt());
    } else if (qName == "event") {

        season.events.append(SeasonEvent(Utils::unprotect(buffer), seasonDateToDate(dateString), priorityString.toInt(), eventDescription, eventId));

    } else if(qName == "season") {

        if(seasons.size() >= 1) {
            // only set end date for previous season if
            // it is not null
            if (seasons[seasons.size()-1].getEnd() == QDate())
                seasons[seasons.size()-1].setEnd(season.getStart());
        }
        if (season.getStart().isValid() && season.getEnd().isValid()) {

            // just check the dates are the right way around
            if (season.getStart() > season.getEnd()) {
                QDate temp = season.getStart();
                season.setStart(season.getEnd());
                season.setEnd(temp);
            }

            seasons.append(season);
        }
    } else if(qName == "phase") {
        season.phases.append(phase);
        isPhase = false;
    }

    return true;
}

bool SeasonParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
    buffer.clear();
    if(name == "season") {
        season = Season();
        loadcount = 0;
        isPhase = false;
    }

    if(name == "phase") {
        phase = Phase();
        isPhase = true;
    }

    if (name == "event") {

        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "date") dateString=attrs.value(i);
            else if (attrs.qName(i) == "priority") priorityString=attrs.value(i);
            else if (attrs.qName(i) == "description") eventDescription = Utils::unprotect(attrs.value(i));
            else if (attrs.qName(i) == "id") eventId = attrs.value(i);
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
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Saving Seasons"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return false;
    };
    file.resize(0);
    QTextStream out(&file);

    // begin document
    out << "<seasons>\n";

    // write out to file
    foreach (Season season, Seasons) {
        if (season.getType() != Season::temporary) {

            // main attributes
            out<<QString("\t<season>\n");

            // Phases
            foreach (Phase phase, season.phases) {
                out<<QString("\t\t<phase>\n"
                      "\t\t\t<name>%1</name>\n"
                      "\t\t\t<startdate>%2</startdate>\n"
                      "\t\t\t<enddate>%3</enddate>\n"
                      "\t\t\t<type>%4</type>\n"
                      "\t\t\t<id>%5</id>\n"
                      "\t\t\t<seed>%6</seed>\n"
                      "\t\t\t<low>%7</low>\n"
                      "\t\t</phase>\n") .arg(Utils::xmlprotect(phase.getName()))
                                             .arg(phase.getStart().toString("yyyy-MM-dd"))
                                             .arg(phase.getEnd().toString("yyyy-MM-dd"))
                                             .arg(phase.getType())
                                             .arg(phase.id().toString())
                                             .arg(phase.getSeed())
                                             .arg(phase.getLow());
            }

            // season infos
            out<<QString("\t\t<name>%1</name>\n"
                  "\t\t<startdate>%2</startdate>\n"
                  "\t\t<enddate>%3</enddate>\n"
                  "\t\t<type>%4</type>\n"
                  "\t\t<id>%5</id>\n"
                  "\t\t<seed>%6</seed>\n"
                  "\t\t<low>%7</low>\n") .arg(Utils::xmlprotect(season.getName()))
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

                out<<QString("\t\t<event date=\"%1\" priority=\"%3\" description=\"%4\" id=\"%5\">\"%2\"</event>\n")
                            .arg(x.date.toString("yyyy-MM-dd"))
                            .arg(Utils::xmlprotect(x.name))
                            .arg(x.priority)
                            .arg(Utils::xmlprotect(x.description))
                            .arg(x.id);
            
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
