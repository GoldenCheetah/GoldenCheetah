/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "ZwoParser.h"

#include <QDebug>


bool
ZwoParser::parseFile
(QFile *file)
{
    resetData();

#if QT_VERSION < 0x060000
    file->open(QIODevice::ReadOnly);
#else
    file->open(QIODeviceBase::ReadOnly);
#endif
    QXmlStreamReader reader(file);
    int numWorkoutFile = 0;
    bool ok = true;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartDocument) {
            continue;
        } else if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            if (elemName == "workout_file" && numWorkoutFile == 0 && parseWorkoutFile(reader)) {
                ++numWorkoutFile;
            } else {
                ok = false;
                break;
            }
        }
    }
    ok &= (! reader.hasError());
    if (reader.hasError()) {
        qCritical() << "Can't parse" << file->fileName() << "-" << reader.errorString() << "in line" << reader.lineNumber() << "column" << reader.columnNumber();
    }
    file->close();
    if (! ok) {
        resetData();
    }
    return ok;
}


bool
ZwoParser::parseWorkoutFile
(QXmlStreamReader &reader)
{
    QString text;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::Characters) {
            text = reader.text().toString().trimmed();
        } else if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            if (elemName == "tags" && ! parseTags(reader)) {
                break;
            } else if (elemName == "workout" && ! parseWorkout(reader)) {
                break;
            }
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            QString elemName = reader.name().toString();
            if (elemName == "workout_file") {
                break;
            } else if (elemName == "name") {
                name = text;
            } else if (elemName == "description") {
                description = text;
            } else if (elemName == "author") {
                author = text;
            } else if (elemName == "category") {
                category = text;
            } else if (elemName == "categoryIndex") {
                categoryIndex = text.toInt();
            } else if (elemName == "subcategory") {
                subcategory = text;
            }
        }
    }
    return ! reader.hasError();
}


bool
ZwoParser::parseTags
(QXmlStreamReader &reader)
{
    QXmlStreamAttributes attributes;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            if (elemName == "tag") {
                QString tag = reader.attributes().value("name").toString().trimmed();
                if (! tag.isEmpty()) {
                    tags << tag;
                }
            }
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            QString elemName = reader.name().toString();
            if (elemName == "tags") {
                break;
            }
        }
    }
    return ! reader.hasError();
}


bool
ZwoParser::parseWorkout
(QXmlStreamReader &reader)
{
    QXmlStreamAttributes attributes;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            QXmlStreamAttributes attributes = reader.attributes();

            int Duration = qRound(attributes.value("Duration").toDouble());
            double PowerLow = attributes.value("PowerLow").toDouble();
            double PowerHigh = attributes.value("PowerHigh").toDouble();
            double Power = attributes.value("Power").toDouble();
            bool ftpTest = attributes.value("ftptest").toInt();

            // Either Power or PowerLow / PowerHigh are available
            // PowerHigh may be optional and should be same as low.
            // e.g. steadystate interval doesn't need both and may not
            // get both in future iterations of the format
            if (Power > 0) {
                PowerHigh = PowerLow = Power;
            } else if (PowerHigh == 0.0) {
                PowerHigh = PowerLow;
            }

            // POINTS
            if (   elemName == "Warmup"
                || elemName == "SteadyState"
                || elemName == "Cooldown"
                || elemName == "Ramp"
                || elemName == "FreeRide"
                || elemName == "Freeride") {
                // basic from/to, with different names for some odd reason
                int from = int(100.0 * PowerLow);
                int to = int(100.0 * PowerHigh);

                // some kind of old kludge, should be flat, but isn't always
                if (elemName == "SteadyState") {
                    int ap = (from + to) / 2;
                    from = to = ap;
                }

                if (elemName == "FreeRide" || elemName == "Freeride") {
                    if (ftpTest) {
                        from = to = 100; // if marked as FTP test, assume 100%
                    } else if (watts == 0) {
                        from = to = 70;
                    } else {
                        from = to = watts; // whatever we were just doing keep doing it
                    }
                }

                // if we are not already at from add a point, or if first
                if (secs == 0 || watts != from) {
                    points << ErgFilePoint(secs * 1000, from, from);
                    watts = from;
                }
                sSecs = secs;
                secs += Duration;
                points << ErgFilePoint(secs * 1000, to, to);
                watts = to;
            } else if (elemName == "IntervalsT") { // repeated intervals with a recovery portion
                // optional
                int count = attributes.value("Repeat").toInt();
                if (count == 0) {
                    count = 1;
                }

                // effort on
                int onDuration = qRound(attributes.value("OnDuration").toDouble()); // duration may be double
                int onLow = attributes.value("PowerOnLow").toDouble() * 100.0;
                int onHigh = attributes.value("PowerOnHigh").toDouble() * 100.0;
                int onPower = attributes.value("OnPower").toDouble() * 100.0;

                // effort off
                int offDuration = qRound(attributes.value("OffDuration").toDouble()); // duration may be double
                int offLow = attributes.value("PowerOffLow").toDouble() * 100.0;
                int offHigh = attributes.value("PowerOffHigh").toDouble() * 100.0;
                int offPower = attributes.value("OffPower").toDouble() * 100.0;

                if (onPower > 0) {
                    onHigh = onLow = onPower;
                } else if (onHigh == 0.0) {
                    onHigh = onLow;
                }

                if (offPower > 0) {
                    offHigh = offLow = offPower;
                } else if (offHigh == 0.0) {
                    offHigh = offLow;
                }

                sSecs = secs;
                while (count--) {
                    // add if not already on that wattage
                    if (watts != onLow) {
                        points << ErgFilePoint(secs * 1000, onLow, onLow);
                        watts = onLow;
                    }
                    secs += onDuration;
                    points << ErgFilePoint(secs * 1000, onHigh, onHigh);
                    watts = onHigh;

                    // recovery interval (if defined)
                    if (offDuration > 0) {
                        if (watts != offLow) {
                            points << ErgFilePoint(secs * 1000, offLow, offLow);
                            watts = offLow;
                        }

                        secs += offDuration;
                        points << ErgFilePoint(secs * 1000, offHigh, offHigh);
                        watts = offHigh;
                    }
                }
            } else if (elemName == "textevent") {
                int offset = attributes.value("timeoffset").toInt();
                QString message = attributes.value("message").toString().trimmed();
                int duration = attributes.value("duration").toInt();
                int pos = attributes.value("y").toInt();
                if (pos >= 240) { // no position, use excess as delay
                    pos -= 240;
                }
                texts << ErgFileText((sSecs + offset + pos) * 1000, duration, message);
            }
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            QString elemName = reader.name().toString();
            if (elemName == "workout") {
                break;
            }
        }
    }
    return ! reader.hasError();
}


void
ZwoParser::resetData
()
{
    secs = 0;
    sSecs = 0;
    watts = 0;
    name.clear();
    description.clear();
    author.clear();
    tags.clear();
    categoryIndex = -1;
    category.clear();
    subcategory.clear();
    points.clear();
    texts.clear();
}
