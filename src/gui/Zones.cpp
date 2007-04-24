/* 
 * $Id: RideItem.cpp,v 1.3 2006/07/09 15:30:34 srhea Exp $
 *
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "Zones.h"
#include <assert.h>

bool Zones::read(QFile &file) 
{
    if (!file.open(QFile::ReadOnly)) {
        err = "can't open file";
        return false;
    }
    qint64 n;
    char buf[1024];
    QRegExp commentrx("\\s*#.*$");
    QRegExp blankrx("^[ \t]*$");
    QRegExp rangerx("^\\s*from\\s+"
                    "((\\d\\d\\d\\d)/(\\d\\d)/(\\d\\d)|BEGIN)"
                    "\\s+until\\s+"
                    "((\\d\\d\\d\\d)/(\\d\\d)/(\\d\\d)|END)\\s*:\\s*$",
                    Qt::CaseInsensitive);
    QRegExp zonerx("^\\s*([^ ,][^,]*),\\s*([^ ,][^,]*),\\s*"
                   "(\\d+)\\s*,\\s*(\\d+|MAX)\\s*$",
                   Qt::CaseInsensitive);
    int lineno = 0;
    ZoneRange *range = NULL;
    while ((n = file.readLine(buf, sizeof(buf))) > 0) {
        ++lineno;
        if (n < sizeof(buf))
            buf[n - 1] = '\0'; // strip newline
        QString line(buf);
        // fprintf(stderr, "line %d: \"%s\"\n", 
        //         lineno, line.toAscii().constData());
        int pos = commentrx.indexIn(line, 0);
        if (pos != -1) {
            line = line.left(pos);
            fprintf(stderr, "removing comment line %d; now: \"%s\"\n", 
                    lineno, line.toAscii().constData());
        }
        if (blankrx.indexIn(line, 0) == 0) {
            fprintf(stderr, "line %d: blank\n", lineno);
        }
        else if (rangerx.indexIn(line, 0) != -1) {
            fprintf(stderr, "line %d: matched range: %s to %s\n", 
                    lineno,
                    rangerx.cap(1).toAscii().constData(),
                    rangerx.cap(5).toAscii().constData());
            QDate begin, end;
            if (rangerx.cap(1) == "BEGIN")
                begin = QDate::currentDate().addYears(-1000);
            else {
                begin = QDate(rangerx.cap(2).toInt(),
                              rangerx.cap(3).toInt(),
                              rangerx.cap(4).toInt());
            }
            if (rangerx.cap(5) == "END")
                end = QDate::currentDate().addYears(1000);
            else {
                end = QDate(rangerx.cap(6).toInt(),
                            rangerx.cap(7).toInt(),
                            rangerx.cap(8).toInt());
            }
            fprintf(stderr, "begin=%s, end=%s\n",
                    begin.toString().toAscii().constData(),
                    end.toString().toAscii().constData());
            if (range) {
                if (range->zones.empty()) {
                    err = tr("line %1: read new range without reading "
                             "any zones for previous one").arg(lineno);
                    file.close();
                    return false;
                }
                ranges.append(range);
            }
            range = new ZoneRange(begin, end);
        }
        else if (zonerx.indexIn(line, 0) != -1) {
            if (!range) {
                err = tr("line %1: read zone without "
                         "preceeding date range").arg(lineno);
                file.close();
                return false;
            }
            int lo = zonerx.cap(3).toInt();
            int hi;
            if (zonerx.cap(4) == "MAX")
                hi = INT_MAX;
            else
                hi = zonerx.cap(4).toInt();
            ZoneInfo *zone = 
                new ZoneInfo(zonerx.cap(1), zonerx.cap(2), lo, hi);
            fprintf(stderr, "line %d: matched zones: "
                    "\"%s\", \"%s\", %s, %s\n", lineno,
                    zonerx.cap(1).toAscii().constData(),
                    zonerx.cap(2).toAscii().constData(),
                    zonerx.cap(3).toAscii().constData(),
                    zonerx.cap(4).toAscii().constData());
            range->zones.append(zone);
        }
    }
    if (range) {
        if (range->zones.empty()) {
            err = tr("file ended without reading "
                     "any zones for last range");
            file.close();
            return false;
        }
        ranges.append(range);
    }
    file.close();
    return true;
}

int Zones::whichZone(const QDate &date, double value) const 
{
    QListIterator<ZoneRange*> i(ranges);
    while (i.hasNext()) {
        ZoneRange *range = i.next();
        if ((date >= range->begin) && (date < range->end)) {
            for (int j = 0; j < range->zones.size(); ++j) {
                ZoneInfo *info = range->zones[j];
                if ((value >= info->lo) && (value < info->hi))
                    return j;
            }
        }
    }
    return -1;
}

void Zones::zoneInfo(const QDate &date, int zone, 
                     QString &name, QString &description,
                     int &low, int &high)
{
    QListIterator<ZoneRange*> i(ranges);
    while (i.hasNext()) {
        ZoneRange *range = i.next();
        if ((date >= range->begin) && (date < range->end)) {
            assert(zone < range->zones.size());
            ZoneInfo *info = range->zones[zone];
            name = info->name;
            description = info->desc;
            low = info->lo;
            high = info->hi;
            return;
        }
    }
    assert(false);
}

