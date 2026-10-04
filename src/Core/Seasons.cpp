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

#include "Seasons.h"
#include "Utils.h"
#include <QFile>
#include <QDate>
#include <QRegularExpression>
#include <QDebug>
#include <QMessageBox>


//
// Manage the seasons array
//

void
Seasons::readSeasons()
{
    QFile seasonFile(home.canonicalPath() + "/seasons.xml");
    seasons = SeasonParser::readSeasons(&seasonFile);

    Season season;

    // add Default Date Ranges
    season.setName(tr("All Dates"));
    season.setOffsetAndLength(-50, 1, 1, 100, 0, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000001}"));
    seasons.append(season);

    season.setName(tr("This Year"));
    season.setOffsetAndLength(0, 1, 1, 1, 0, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000002}"));
    seasons.append(season);

    season.setName(tr("This Month"));
    season.setOffsetAndLength(1, 0, 1, 0, 1, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000003}"));
    seasons.append(season);

    season.setName(tr("Last Month"));
    season.setOffsetAndLength(1, -1, 1, 0, 1, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000013}"));
    seasons.append(season);

    season.setName(tr("This Week"));
    season.setOffsetAndLength(1, 1, 0, 0, 0, 7);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000004}"));
    seasons.append(season);

    season.setName(tr("Last Week"));
    season.setOffsetAndLength(1, 1, -1, 0, 0, 7);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000014}"));
    seasons.append(season);

    season.setName(tr("Last 24 hours"));
    season.setLengthOnly(0, 0, 2);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000016}"));
    seasons.append(season);

    season.setName(tr("Last 7 days"));
    season.setLengthOnly(0, 0, 7);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000005}"));
    seasons.append(season);

    season.setName(tr("Last 14 days"));
    season.setLengthOnly(0, 0, 14);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000006}"));
    seasons.append(season);

    season.setName(tr("Last 21 days"));
    season.setLengthOnly(0, 0, 21);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000011}"));
    seasons.append(season);

    season.setName(tr("Last 28 days"));
    season.setLengthOnly(0, 0, 28);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000007}"));
    seasons.append(season);

    season.setName(tr("Last 6 weeks"));
    season.setLengthOnly(0, 0, 42);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000015}"));
    seasons.append(season);

    season.setName(tr("Last 2 months"));
    season.setLengthOnly(0, 2, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000008}"));
    seasons.append(season);

    season.setName(tr("Last 3 months"));
    season.setLengthOnly(0, 3, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000012}"));
    seasons.append(season);

    season.setName(tr("Last 6 months"));
    season.setLengthOnly(0, 6, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000009}"));
    seasons.append(season);

    season.setName(tr("Last 12 months"));
    season.setLengthOnly(1, 0, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000010}"));
    seasons.append(season);

    seasonsChanged(); // signal!
}

int
Seasons::newSeason(QString name, QDate start, QDate end, int type)
{
    Season add;
    add.setName(name);
    add.setAbsoluteStart(start);
    add.setAbsoluteEnd(end);
    add.setType(type);
    seasons.insert(0, add);

    // save changes away
    writeSeasons();

    return 0; // always add at the top
}

void
Seasons::updateSeason(int index, QString name, QDate start, QDate end, int type)
{
    seasons[index].setName(name);
    seasons[index].setAbsoluteStart(start);
    seasons[index].setAbsoluteEnd(end);
    seasons[index].setType(type);

    // save changes away
    writeSeasons();

}

void
Seasons::deleteSeason(int index)
{
    // now delete!
    seasons.removeAt(index);
    writeSeasons();
}

void
Seasons::writeSeasons()
{
    // update seasons.xml
    QString file = QString(home.canonicalPath() + "/seasons.xml");
    SeasonParser::serialize(file, seasons);

    seasonsChanged(); // signal!
}


//
// SeasonParser
//

QList<Season>
SeasonParser::readSeasons
(QFile * const file, bool *idEnriched)
{
    QList<Season> seasons;
    if (! file->open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "Cannot read file" << file->fileName() << "-" << file->errorString();
        return seasons;
    }
    QXmlStreamReader reader(file);
    Season season;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartDocument) {
            continue;
        }
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            if (elemName == "season") {
                season = parseSeason(reader, idEnriched);
                if (seasons.size() >= 1 && seasons[seasons.size() - 1].getEnd() == QDate()) {
                    // only set end date for previous season if
                    // it is not null
                    seasons[seasons.size() - 1].setAbsoluteEnd(season.getStart());
                }
                // just check the dates are the right way around
                if (   season.getAbsoluteStart().isValid()
                    && season.getAbsoluteEnd().isValid()
                    && season.getAbsoluteStart() > season.getAbsoluteEnd()) {
                    QDate temp = season.getAbsoluteStart();
                    season.setAbsoluteStart(season.getAbsoluteEnd());
                    season.setAbsoluteEnd(temp);
                }
                if (season.getStart().isValid() && season.getEnd().isValid()) {
                    seasons.append(season);
                }
            }
        }
    }
    if (reader.hasError()) {
        qCritical() << "Can't parse" << file->fileName() << "-" << reader.errorString() << "in line" << reader.lineNumber() << "column" << reader.columnNumber();
    }
    file->close();
    return seasons;
}


bool
SeasonParser::serialize(QString filename, QList<Season> Seasons)
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
                out<<QString("\t\t<phase>\n");
                out<<QString(
                      "\t\t\t<name>%1</name>\n"
                      "\t\t\t<startdate>%2</startdate>\n"
                      "\t\t\t<enddate>%3</enddate>\n"
                      "\t\t\t<type>%4</type>\n"
                      "\t\t\t<id>%5</id>\n"
                      "\t\t\t<seed>%6</seed>\n"
                      "\t\t\t<low>%7</low>\n")
                            .arg(Utils::xmlprotect(phase.getName()))
                            .arg(phase.getAbsoluteStart().toString("yyyy-MM-dd"))
                            .arg(phase.getEnd().toString("yyyy-MM-dd"))
                            .arg(phase.getType())
                            .arg(phase.id().toString())
                            .arg(phase.getSeed())
                            .arg(phase.getLow());
                if (! phase.getDescription().isEmpty()) {
                    out<<QString("\t\t\t<description>%1</description>\n")
                                .arg(Utils::xmlprotect(phase.getDescription()));
                }
                if (phase.hasMesocycle()) {
                    Mesocycle *meso = phase.getMesocycle();
                    out<<QString("\t\t\t<mesocycle>\n");
                    out<<QString("\t\t\t\t<mesointent>\n");
                    if (meso->getTargetCP() > 0) {
                        out<<QString("\t\t\t\t\t<targetCP>%1</targetCP>\n").arg(meso->getTargetCP());
                    }
                    if (meso->getTargetFTP() > 0) {
                        out<<QString("\t\t\t\t\t<targetFTP>%1</targetFTP>\n").arg(meso->getTargetFTP());
                    }
                    if (meso->getTargetLTS() > 0) {
                        out<<QString("\t\t\t\t\t<targetLTS>%1</targetLTS>\n").arg(meso->getTargetLTS());
                    }
                    out<<QString("\t\t\t\t</mesointent>\n");
                    out<<QString("\t\t\t\t<microcycles>\n");
                    for (int i = 0; i < meso->getMaxMicrocycles(); ++i) {
                        Microcycle dummy;
                        Microcycle *micro = meso->getMicrocycle(i);
                        if (micro == nullptr) {
                            micro = &dummy;
                        }
                        out<<QString("\t\t\t\t\t<microcycle frequency=\"%1\" load=\"%2\" intensity=\"%3\"")
                                    .arg(micro->getFrequency())
                                    .arg(micro->getLoad())
                                    .arg(micro->getIntensity());
                        if (micro->getTime() > 0) {
                            out<<QString(" time=\"%1\"").arg(micro->getTime());
                        }
                        if (! micro->getDescription().isEmpty()) {
                            out<<QString(" description=\"%1\"").arg(Utils::xmlprotect(micro->getDescription()));
                        }
                        out<<QString(" />\n");
                    }
                    out<<QString("\t\t\t\t</microcycles>\n");
                    QMap<int, QList<QString>> slotMap = meso->getSlotOffsetSports();
                    if (slotMap.count() > 0) {
#warning ADD DESCRIPTION (?), LINKS TO PLANNED ACTIVITIES
                        out<<QString("\t\t\t\t<slots>\n");
                        for (const std::pair<const int&, const QList<QString>&> &pair : slotMap.asKeyValueRange()) {
                            int offset = pair.first;
                            for (const QString &sport : pair.second) {
                                Slot const *slot = meso->getSlot(offset, sport);
                                out<<QString("\t\t\t\t\t<slot dayoffset=\"%1\" sport=\"%2\" status=\"%3\"")
                                            .arg(slot->getDayOffset())
                                            .arg(Utils::xmlprotect(slot->getSport()))
                                            .arg(Utils::xmlprotect(Slot::statusToString(slot->getStatus())));
                                if (slot->hasLoad()) {
                                    out<<QString(" load=\"%1\"").arg(slot->getLoad());
                                }
                                if (slot->hasIntensity()) {
                                    out<<QString(" intensity=\"%1\"").arg(slot->getIntensity());
                                }
                                if (slot->hasTime()) {
                                    out<<QString(" time=\"%1\"").arg(slot->getTime());
                                }
                                if (slot->hasDescription()) {
                                    out<<QString(" description=\"%1\"").arg(Utils::xmlprotect(slot->getDescription()));
                                }
                                if (slot->getActivities().count() > 0) {
                                    QString ids;
                                    for (const QString &id : slot->getActivities()) {
                                        if (! ids.isEmpty()) {
                                            ids += u' ';
                                        }
                                        ids += id;
                                    }
                                    out<<QString(" activityIds=\"%1\"").arg(Utils::xmlprotect(ids));
                                }
                                out<<QString(" />\n");
                            }
                        }
                        out<<QString("\t\t\t\t</slots>\n");
                    }
                    out<<QString("\t\t\t</mesocycle>\n");
                }
                out<<QString("\t\t</phase>\n");
            }

            // season infos
            out<<QString("\t\t<name>%1</name>\n"
                  "\t\t<type>%2</type>\n"
                  "\t\t<id>%3</id>\n"
                  "\t\t<seed>%4</seed>\n"
                  "\t\t<low>%5</low>\n"
                  "\t\t<firstDayOfWeek>%6</firstDayOfWeek>\n"
                  ) .arg(Utils::xmlprotect(season.getName()))
                                         .arg(season.getType())
                                         .arg(season.id().toString())
                                         .arg(season.getSeed())
                                         .arg(season.getLow())
                                         .arg(season.getFirstDayOfWeek());
            if (! season.getDescription().isEmpty()) {
                out<<QString("\t\t<description>%1</description>\n").arg(Utils::xmlprotect(season.getDescription()));
            }
            if (season.getModelType() != Season::ModelType::none) {
                out<<QString("\t\t<model>%1</model>\n").arg(Season::modelToString(season.getModelType()));
            }
            if (season.getAbsoluteStart().isValid()) {
                out<<QString("\t\t<startdate>%1</startdate>\n").arg(season.getAbsoluteStart().toString("yyyy-MM-dd"));
            }
            if (season.getOffsetStart().isValid()) {
                std::pair<SeasonOffset::SeasonOffsetType, int> item = season.getOffsetStart().getSignificantItem();
                out<<QString("\t\t<startoffset>%1-%2</startoffset>\n").arg(item.first)
                                                                      .arg(std::abs(item.second));
            }
            if (season.getAbsoluteEnd().isValid()) {
                out<<QString("\t\t<enddate>%1</enddate>\n").arg(season.getAbsoluteEnd().toString("yyyy-MM-dd"));
            }
            if (season.getOffsetEnd().isValid()) {
                std::pair<SeasonOffset::SeasonOffsetType, int> item = season.getOffsetEnd().getSignificantItem();
                out<<QString("\t\t<endoffset>%1-%2</endoffset>\n").arg(item.first)
                                                                  .arg(std::abs(item.second));
            }
            if (season.isYtd()) {
                out<<QString("\t\t<ytd/>\n");
            }
            if (season.getLength().isValid()) {
                out<<QString("\t\t<length>%1-%2-%3</length>\n").arg(season.getLength().getYears())
                                                               .arg(season.getLength().getMonths())
                                                               .arg(season.getLength().getDays());
            }

            foreach(SeasonEvent x, season.events) {

                out<<QString("\t\t<event date=\"%1\" priority=\"%2\" description=\"%3\" id=\"%4\"")
                            .arg(x.date.toString("yyyy-MM-dd"))
                            .arg(x.priority)
                            .arg(Utils::xmlprotect(x.description))
                            .arg(x.id);
                if (x.targetCP > 0) {
                    out<<QString(" targetCP=\"%1\"").arg(x.targetCP);
                }
                if (x.targetFTP > 0) {
                    out<<QString(" targetFTP=\"%1\"").arg(x.targetFTP);
                }
                if (x.targetLTS > 0) {
                    out<<QString(" targetLTS=\"%1\"").arg(x.targetLTS);
                }
                out<<QString(">\"%1\"</event>\n").arg(Utils::xmlprotect(x.name));
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


Season
SeasonParser::parseSeason
(QXmlStreamReader &reader, bool *idEnriched)
{
    Season season;
    QString text;
    QXmlStreamAttributes attributes;
    int loadcount = 0;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name().toString() == "phase") {
                season.phases.append(parsePhase(reader));
            } else {
                text = "";
                attributes = reader.attributes();
            }
        } else if (reader.tokenType() == QXmlStreamReader::Characters) {
            text = reader.text().toString().trimmed();
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            QString elemName = reader.name().toString();
            if (elemName == "name") {
                season.setName(Utils::unprotect(text));
            } else if (elemName == "startdate") {
                season.setAbsoluteStart(parseDate(text));
            } else if (elemName == "startoffset") {
                season.setOffsetStart(parseOffset(text));
            } else if (elemName == "enddate") {
                season.setAbsoluteEnd(parseDate(text));
            } else if (elemName == "endoffset") {
                season.setOffsetEnd(parseOffset(text));
            } else if (elemName == "ytd") {
                season.setYtd();
            } else if (elemName == "length") {
                season.setLength(parseLength(text));
            } else if (elemName == "type") {
                season.setType(text.toInt());
            } else if (elemName == "id") {
                season.setId(QUuid(text));
            } else if (elemName == "low") {
                season.setLow(text.toInt());
            } else if (elemName == "seed") {
                season.setSeed(text.toInt());
            } else if (elemName == "description") {
                season.setDescription(Utils::unprotect(text));
            } else if (elemName == "firstDayOfWeek") {
                season.setFirstDayOfWeek(static_cast<Qt::DayOfWeek>(std::clamp(text.toInt(), 1, 7)));
            } else if (elemName == "model") {
                season.setModelType(Season::modelFromString(text));
            } else if (elemName == "event") {
                QDate date = parseDate(attributes.value("date").toString());
                int priority = attributes.value("priority").toString().toInt();
                QString description = Utils::unprotect(attributes.value("description").toString());
                QString id = attributes.value("id").toString();
                if (idEnriched != nullptr && id.isEmpty()) {
                    *idEnriched = true;
                    // default constructed id (see SeasonEvent::SeasonEvent) will be used, dont overwrite
                }
                int targetCP = std::max(0, attributes.value("targetCP").toInt());
                int targetFTP = std::max(0, attributes.value("targetFTP").toInt());
                int targetLTS = std::max(0, attributes.value("targetLTS").toInt());
                season.events.append(SeasonEvent(Utils::unprotect(text), date, priority, description, id, targetLTS, targetCP, targetFTP));
            } else if (elemName == "season") {
                break;
            }
        }
    }
    return season;
}


void
SeasonParser::parseMicrocycles
(QXmlStreamReader &reader, Mesocycle &meso)
{
    int idx = -1;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name().toString() == "microcycle") {
                ++idx;
                Microcycle *microcycle = meso.getMicrocycle(idx);
                microcycle->setDescription(Utils::unprotect(reader.attributes().value("description").toString()));
                microcycle->setFrequency(reader.attributes().value("frequency").toString().toInt());
                microcycle->setLoad(reader.attributes().value("load").toString().toInt());
                microcycle->setIntensity(reader.attributes().value("intensity").toString().toFloat());
                microcycle->setTime(reader.attributes().value("time").toInt());
            }
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            if (reader.name().toString() == "microcycles") {
                break;
            }
        }
    }
}


void
SeasonParser::parseSlots
(QXmlStreamReader &reader, Mesocycle &meso)
{
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name().toString() == "slot") {
                bool ok = false;
                bool allOk = true;
                int offset = reader.attributes().value("dayoffset").toString().toInt(&ok);
                QString sport = Utils::unprotect(reader.attributes().value("sport").toString());
                Slot::StatusType status = Slot::StatusType::generated;
                int load = 0;
                float intensity = 0.0f;
                int time = 0;
                QString desc;
                QStringList activityIds;
                if (reader.attributes().hasAttribute("status")) {
                    status = Slot::stringToStatus(reader.attributes().value("status").toString());
                    allOk &= ok;
                }
                Slot tmpSlot(offset, sport, status);
                if (allOk && reader.attributes().hasAttribute("load")) {
                    load = reader.attributes().value("load").toString().toInt(&ok);
                    allOk &= ok;
                    if (ok) {
                        allOk &= tmpSlot.setLoad(load);
                    }
                }
                if (reader.attributes().hasAttribute("intensity")) {
                    intensity = reader.attributes().value("intensity").toString().toFloat(&ok);
                    allOk &= ok;
                    if (ok) {
                        allOk &= tmpSlot.setIntensity(intensity);
                    }
                }
                if (reader.attributes().hasAttribute("time")) {
                    time = reader.attributes().value("time").toString().toInt(&ok);
                    allOk &= ok;
                    if (ok) {
                        allOk &= tmpSlot.setIntensity(time);
                    }
                }
                if (reader.attributes().hasAttribute("description")) {
                    desc = Utils::unprotect(reader.attributes().value("description").toString());
                    allOk &= ok;
                    if (ok) {
                        allOk &= tmpSlot.setDescription(desc);
                    }
                }
                if (reader.attributes().hasAttribute("activityIds")) {
                    QString activityStr = Utils::unprotect(reader.attributes().value("activityIds").toString());
                    activityIds = activityStr.split(u' ');
                }
                if (allOk && offset >= 0 && ! sport.isEmpty()) {
                    Slot *slot = meso.addSlot(offset, sport);
                    if (slot != nullptr) {
                        slot->setStatus(status);
                        slot->setLoad(load);
                        slot->setIntensity(intensity);
                        slot->setTime(time);
                        slot->setDescription(desc);
                        slot->setActivities(activityIds);
                    }
                }
            }
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            if (reader.name().toString() == "slots") {
                break;
            }
        }
    }
}


void
SeasonParser::parseMesocycleRaw
(QXmlStreamReader &reader, Mesocycle &meso)
{
    QString text;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            if (elemName == "microcycles") {
                parseMicrocycles(reader, meso);
            } else if (elemName == "slots") {
                parseSlots(reader, meso);
            } else {
                text = "";
            }
        } else if (reader.tokenType() == QXmlStreamReader::Characters) {
            text = reader.text().toString().trimmed();
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            QString elemName = reader.name().toString();
            if (elemName == "targetCP") {
                meso.setTargetCP(std::max(0, text.toInt()));
            } else if (elemName == "targetFTP") {
                meso.setTargetFTP(std::max(0, text.toInt()));
            } else if (elemName == "targetLTS") {
                meso.setTargetLTS(std::max(0, text.toInt()));
            } else if (reader.name().toString() == "mesocycle") {
                break;
            }
        }
    }
}


Phase
SeasonParser::parsePhase
(QXmlStreamReader &reader)
{
    QString name;
    QDate start, end;
    int type = Phase::phase;
    QUuid id;
    int seed = 0;
    int low = -50;
    QString description;

    QString text;
    bool hasMesoElement = false;
    Mesocycle meso;
    meso.setStart(QDate(1, 1, 2000));
    meso.setMaxMicrocycles(1000);
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            if (elemName == "mesocycle") {
                hasMesoElement = true;
                parseMesocycleRaw(reader, meso);
                continue;
            }
            text = "";
        } else if (reader.tokenType() == QXmlStreamReader::Characters) {
            text = reader.text().toString().trimmed();
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            QString elemName = reader.name().toString();
            if (elemName == "name") {
                name = Utils::unprotect(text);
            } else if (elemName == "startdate") {
                start = parseDate(text);
            } else if (elemName == "enddate") {
                end = parseDate(text);
            } else if (elemName == "type") {
                type = text.toInt();
            } else if (elemName == "id") {
                id = QUuid(text);
            } else if (elemName == "low") {
                low = text.toInt();
            } else if (elemName == "seed") {
                seed = text.toInt();
            } else if (elemName == "description") {
                description = Utils::unprotect(text);
            } else if (elemName == "phase") {
                break;
            }
        }
    }

    Phase phase(name, start, end);
    phase.setId(id);
    phase.setLow(low);
    phase.setSeed(seed);
    phase.setDescription(description);
    phase.setType(hasMesoElement ? Phase::mesocycle : type);

    if (hasMesoElement) {
        Mesocycle *m = phase.getMesocycle();
        *m = meso;
        phase.setType(Phase::mesocycle);
        m = phase.getMesocycle();
    }

    return phase;
}


QDate SeasonParser::parseDate(QString seasonDate)
{
    QRegularExpression re(".*([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9]$)");
    QRegularExpressionMatch match = re.match(seasonDate.trimmed());
    if (match.hasMatch()) {
        return QDate(match.captured(1).toInt(),
                     match.captured(2).toInt(),
                     match.captured(3).toInt());
    } else {
        return QDate(); // return value was 1 Jan: changed to null
    }
}


SeasonOffset
SeasonParser::parseOffset
(QString offsetStr)
{
    QRegularExpression re("^([0-9]+)-([0-9]+)$");
    QRegularExpressionMatch match = re.match(offsetStr.trimmed());
    if (match.hasMatch()) {
        return SeasonOffset(std::make_pair(SeasonOffset::SeasonOffsetType(match.captured(1).toInt()), -(match.captured(2).toInt())), false);
    } else {
        return SeasonOffset();
    }
}


SeasonLength
SeasonParser::parseLength
(QString lengthStr)
{
    QRegularExpression re("^([0-9]+)-([0-9]+)-([0-9]+)$");
    QRegularExpressionMatch match = re.match(lengthStr.trimmed());
    if (match.hasMatch()) {
        return SeasonLength(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
    } else {
        return SeasonLength();
    }
}
