/* 
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
#include "TimeUtils.h"
#include <assert.h>
#include <math.h>

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
                    "((\\d\\d\\d\\d)/(\\d\\d)/(\\d\\d)|END)\\s*"
                    "(,\\s*(FTP|CP)\\s*=\\s*(\\d+)\\s*)?:\\s*$",
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
            // fprintf(stderr, "removing comment line %d; now: \"%s\"\n", 
            //         lineno, line.toAscii().constData());
        }
        if (blankrx.indexIn(line, 0) == 0) {
            // fprintf(stderr, "line %d: blank\n", lineno);
        }
        else if (rangerx.indexIn(line, 0) != -1) {
            // fprintf(stderr, "line %d: matched range: %s to %s\n", 
            //         lineno,
            //         rangerx.cap(1).toAscii().constData(),
            //         rangerx.cap(5).toAscii().constData());
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
            // fprintf(stderr, "begin=%s, end=%s\n",
            //         begin.toString().toAscii().constData(),
            //         end.toString().toAscii().constData());
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
            if (rangerx.numCaptures() == 11)
                range->ftp = rangerx.cap(11).toInt();
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
            // fprintf(stderr, "line %d: matched zones: "
            //         "\"%s\", \"%s\", %s, %s\n", lineno,
            //         zonerx.cap(1).toAscii().constData(),
            //         zonerx.cap(2).toAscii().constData(),
            //         zonerx.cap(3).toAscii().constData(),
            //         zonerx.cap(4).toAscii().constData());
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

int Zones::whichRange(const QDate &date) const
{
    int rnum = 0;
    QListIterator<ZoneRange*> i(ranges);
    while (i.hasNext()) {
        ZoneRange *range = i.next();
        if ((date >= range->begin) && (date < range->end))
            return rnum;
        ++rnum;
    }
    return -1;
}

int Zones::numZones(int rnum) const
{
    assert(rnum < ranges.size());
    return ranges[rnum]->zones.size();
}

int Zones::whichZone(int rnum, double value) const 
{
    assert(rnum < ranges.size());
    ZoneRange *range = ranges[rnum];
    for (int j = 0; j < range->zones.size(); ++j) {
        ZoneInfo *info = range->zones[j];
        if ((value >= info->lo) && (value < info->hi))
            return j;
    }
    return -1;
}

void Zones::zoneInfo(int rnum, int znum, 
                     QString &name, QString &description,
                     int &low, int &high) const
{
    assert(rnum < ranges.size());
    ZoneRange *range = ranges[rnum];
    assert(znum < range->zones.size());
    ZoneInfo *zone = range->zones[znum];
    name = zone->name;
    description = zone->desc;
    low = zone->lo;
    high = zone->hi;
}

int Zones::getCP(int rnum) const
{
    assert(rnum < ranges.size());
    return ranges[rnum]->ftp;
}

void Zones::setCP(int rnum, int ftp)
{
    ranges[rnum]->ftp = ftp;
}


QString Zones::summarize(int rnum, double *time_in_zone, int num_zones) const
{
    assert(rnum < ranges.size());
    ZoneRange *range = ranges[rnum];
    assert(num_zones == range->zones.size());
    QString summary;
    if(range->ftp > 0){
        summary += "<table align=\"center\" width=\"70%\" border=\"0\">";
        summary += "<tr><td align=\"center\">";
        summary += tr("Critical Power: %1").arg(range->ftp);
        summary += "</td></tr></table>";
    }
    summary += "<table align=\"center\" width=\"70%\" ";
    summary += "border=\"0\">";
    summary += "<tr>";
    summary += "<td align=\"center\">Zone</td>";
    summary += "<td align=\"center\">Description</td>";
    summary += "<td align=\"center\">Low</td>";
    summary += "<td align=\"center\">High</td>";
    summary += "<td align=\"center\">Time</td>";
    summary += "</tr>";
    for (int zone = 0; zone < num_zones; ++zone) {
        if (time_in_zone[zone] > 0.0) {
            QString name, desc;
            int lo, hi;
            zoneInfo(rnum, zone, name, desc, lo, hi);
            summary += "<tr>";
            summary += QString("<td align=\"center\">%1</td>").arg(name);
            summary += QString("<td align=\"center\">%1</td>").arg(desc);
            summary += QString("<td align=\"center\">%1</td>").arg(lo);
            if (hi == INT_MAX)
                summary += "<td align=\"center\">MAX</td>";
            else
                summary += QString("<td align=\"center\">%1</td>").arg(hi);
            summary += QString("<td align=\"center\">%1</td>")
                .arg(time_to_string((unsigned) round(time_in_zone[zone])));
            summary += "</tr>";
        }
    }
    summary += "</table>";
    return summary;
}

void Zones::write(QDir home)
{
    int active_recovery;
    int endurance_start;
    int endurance_end;
    int tempo_start;
    int tempo_end;
    int threshold_start;
    int threshold_end;
    int vo2max_start;
    int vo2max_end;
    int anaerobicCapacity_start;
    int anaerobicCapacity_end;
    int neuromuscular;
    QString strzones;

    for (int i = 0; i < ranges.size(); i++)
    {
        int LT = getCP(i);

        active_recovery = 0;
        endurance_start = 0;
        endurance_end = 0;
        tempo_start = 0;
        tempo_end = 0;
        threshold_start = 0;
        threshold_end = 0;
        vo2max_start = 0;
        vo2max_end = 0;
        anaerobicCapacity_start = 0;
        anaerobicCapacity_end = 0;
        neuromuscular = 0;


        active_recovery = int (LT * .55);
        endurance_start = int (LT * .56);
        endurance_end = int (LT * .75);
        tempo_start = int (LT * .76);
        tempo_end = int (LT * .90);
        threshold_start = int (LT * .91);
        threshold_end = int (LT * 1.05);
        vo2max_start = int (LT * 1.06);
        vo2max_end = int (LT * 1.2);
        anaerobicCapacity_start = int (LT * 1.21);
        anaerobicCapacity_end = int (LT * 1.5);
        neuromuscular =  int (LT * 1.51);

        if(ranges.size() <= 1)
            strzones += QString("FROM BEGIN UNTIL END, CP=%1:").arg(LT);
        else if (i == 0)
            strzones += QString("FROM BEGIN UNTIL %1, CP=%2:").arg(getEndDate(i).toString("yyyy/MM/dd")).arg(LT);
        else if (i == ranges.size() - 1)
            strzones += QString("FROM %1 UNTIL END, CP=%2:").arg(getStartDate(i).toString("yyyy/MM/dd")).arg(LT);
        else
            strzones += QString("FROM %1 UNTIL %2, CP=%3:").arg(getStartDate(i).toString("yyyy/MM/yy")).arg(getEndDate(i).toString("yyyy/MM/dd")).arg(LT);


        strzones += QString("\n");
        strzones += QString("1,Active Recovery, 1, %1").arg(active_recovery);
        strzones += QString("\n");
        strzones += QString("2,Endurance, %1, %2").arg(endurance_start).arg(endurance_end);
        strzones += QString("\n");
        strzones += QString("3,Tempo, %1, %2").arg(tempo_start).arg(tempo_end);
        strzones += QString("\n");
        strzones += QString("4,Threshold, %1, %2").arg(threshold_start).arg(threshold_end);
        strzones += QString("\n");
        strzones += QString("5,VO2Max, %1, %2").arg(vo2max_start).arg(vo2max_end);
        strzones += QString("\n");
        strzones += QString("6,Anaerobic, %1, %2").arg(anaerobicCapacity_start).arg(anaerobicCapacity_end);
        strzones += QString("\n");
        strzones += QString("7,Neuromuscular, %1,").arg(neuromuscular);
        strzones += QString("MAX");
        strzones += QString("\n");
        strzones += QString("\n");

    }


    QFile file(home.absolutePath() + "/power.zones");
    if (file.open(QFile::WriteOnly))
    {
        QTextStream stream(&file);
        stream << strzones << endl;
        file.close();
    }

}

void Zones::addZoneRange(QDate _start, QDate _end, int _ftp)
{
    ZoneRange *range = new ZoneRange(_start, _end);
    range->ftp = _ftp;
    ranges.append(range);
}
/*
From 2008/01/01 until END,CP=270:
    1,Active Recovery,      1, 150
    2,Endurance,          151, 204
    3,Tempo,              205, 245
    4,Threshold,          246, 285
    5,VO2Max,             286, 326
    6,Anaerobic,          327, MAX
*/

void Zones::setEndDate(int rnum, QDate endDate)
{
    ranges[rnum]->end = endDate;
}
void Zones::setStartDate(int rnum, QDate startDate)
{
    ranges[rnum]->begin = startDate;
}

QDate Zones::getStartDate(int rnum)
{
    return ranges[rnum]->begin;
}

QDate Zones::getEndDate(int rnum)
{
    return ranges[rnum]->end;
}

int Zones::getRangeSize()
{
    return ranges.size();
}

