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

bool ZwoParser::startDocument()
{
    buffer.clear();
    secs = 0;
    sSecs = 0;
    watts = 0;
    return true;
}

bool ZwoParser::endElement( const QString&, const QString&, const QString &qName)
{
    // METADATA
    if(qName == "name") {
        name = buffer.trimmed();
    } else if(qName == "description") {
        description = buffer.trimmed();
    } else if(qName == "tag") {
        tags << buffer.trimmed();
    } else if (qName == "author") {
        author = buffer.trimmed();
    } else if (qName == "category") {
        category = buffer.trimmed();
    } else if (qName == "categoryIndex") {
        categoryIndex = buffer.trimmed().toInt();
    }
    return true;
}

bool
ZwoParser::startElement(const QString &, const QString &, const QString &qName, const QXmlAttributes &attrs)
{
    int Duration = qRound(attrs.value("Duration").toDouble());
    double PowerLow = attrs.value("PowerLow").toDouble();
    double PowerHigh = attrs.value("PowerHigh").toDouble();
    double Power = attrs.value("Power").toDouble();
    bool ftpTest = attrs.value("ftptest").toInt();

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

    // basic from/to, with different names for some odd reason
    if (qName == "Warmup" || qName == "SteadyState"  || qName == "Cooldown" || qName == "Ramp" || qName == "FreeRide" || qName == "Freeride") {

        int from = int(100.0 * PowerLow);
        int to = int(100.0 * PowerHigh);

        if (qName == "Cooldown" && from < to) {
            std::swap(from, to);
        }

        // some kind of old kludge, should be flat, but isn't always
        if (qName == "SteadyState") {
            int ap = (from+to) / 2;
            from = to = ap;
        }

        if (qName == "FreeRide" || qName == "Freeride") {
            if (ftpTest) from = to = 100; // if marked as FTP test, assume 100%
            else if (watts == 0) from = to = 70;
            else from = to = watts; // whatever we were just doing keep doing it
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

    // repeated intervals with a recovery portion
    } else if (qName == "IntervalsT") {

        // optional
        int count = attrs.value("Repeat").toInt();
        if (count == 0) count = 1;

        // effort on
        int onDuration = qRound(attrs.value("OnDuration").toDouble()); // duration may be double
        int onLow = attrs.value("PowerOnLow").toDouble() * 100.0;
        int onHigh = attrs.value("PowerOnHigh").toDouble() * 100.0;
        int onPower = attrs.value("OnPower").toDouble() * 100.0;

        // effort off
        int offDuration = qRound(attrs.value("OffDuration").toDouble()); // duration may be double
        int offLow = attrs.value("PowerOffLow").toDouble() * 100.0;
        int offHigh = attrs.value("PowerOffHigh").toDouble() * 100.0;
        int offPower = attrs.value("OffPower").toDouble() * 100.0;

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

    // TEXTS
    //<textevent timeoffset="1070" message="Final Push!!! You can do it."/>
    } else if (qName == "textevent") {
        int offset = attrs.value("timeoffset").toInt();
        QString message=attrs.value("message");
        int duration = attrs.value("duration").toInt();
        int pos = attrs.value("y").toInt();
        if (pos >= 240) pos -= 240; // no position, use excess as delay
        texts << ErgFileText((sSecs+offset+pos)*1000, duration, message);
    }

    // and clear anything left in the buffer
    buffer.clear();
    return true;
}


bool ZwoParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

bool ZwoParser::endDocument()
{
    // do nothing for now, but a good place to fix up
    // any oddities etc before parser completes.
    return true;
}
