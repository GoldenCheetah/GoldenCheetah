/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
 *
 * [mostly cut and paste from Zones.cpp]
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

#include <QMessageBox>
#include "PaceZones.h"
#include "Colors.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include <QtGui>
#include <QtAlgorithms>
#include <qcolor.h>
#include <assert.h>
#include <cmath>
#include <float.h>


// the infinity endpoints are indicated with extreme date ranges
// but not zero dates so we can edit and compare them
static const QDate date_zero(1900, 01, 01);
static const QDate date_infinity(9999,12,31);

// initialize default static zone parameters
void PaceZones::initializeZoneParameters()
{
    int* initial_zone_default;
    int initial_nzones_default;

    // these default zones are based upon the Skiba run pace zones
    // but expressed as a percentage of Critical Velocity in km/h
    // rather than as a percentage of LT Pace in minutes/mile
    //
    // Name         %LT Pace         %CV
    // AR              > 125        0 - 80
    // Endurance    124 - 115      80 - 87
    // Tempo        114 - 105      88 - 95
    // Threshold    104 - 95       95 - 105
    // Vo2Max        94 - 84      105 - 119
    // Anaerobic       < 84         > 119
    //
    static int initial_zone_default_run[] = {
        0, 80, 87, 95, 105, 119
    };

    // these default zones are based upon the Skiba swim pace zones
    // but expressed as a percentage of Critical Velocity in km/h
    // rather than as a percentage of LT Pace in minutes/100yd
    //
    // Name         %LT Pace         %CV
    // AR              > 107        0 - 93
    // Endurance    107 - 103      93 - 97
    // Tempo        103 - 101      97 - 99
    // Threshold    101 - 98       99 - 102
    // Vo2Max        98 - 92      102 - 109
    // Anaerobic       < 92         > 109
    //
    static int initial_zone_default_swim[] = {
        0, 93, 97, 99, 102, 109
    };

    if (swim) {
        fileName_ = "swim-pace.zones";
        initial_zone_default = initial_zone_default_swim;
        initial_nzones_default =
            sizeof(initial_zone_default_swim) /
            sizeof(initial_zone_default_swim[0]);

    } else {
        fileName_ = "run-pace.zones";
        initial_zone_default = initial_zone_default_run;
        initial_nzones_default =
            sizeof(initial_zone_default_run) /
            sizeof(initial_zone_default_run[0]);
    }

    static const QString initial_zone_default_desc[] = {
        tr("Active Recovery"), tr("Endurance"), tr("Tempo"), tr("Threshold"),
        tr("VO2Max"), tr("Anaerobic")
    };
    static const char *initial_zone_default_name[] = {
        "Z1", "Z2", "Z3", "Z4", "Z5", "Z6"
    };

    scheme.zone_default.clear();
    scheme.zone_default_is_pct.clear();
    scheme.zone_default_desc.clear();
    scheme.zone_default_name.clear();
    scheme.nzones_default = 0;

    scheme.nzones_default = initial_nzones_default;

    for (int z = 0; z < scheme.nzones_default; z++) {
        scheme.zone_default.append(initial_zone_default[z]);
        scheme.zone_default_is_pct.append(true);
        scheme.zone_default_name.append(QString(initial_zone_default_name[z]));
        scheme.zone_default_desc.append(QString(initial_zone_default_desc[z]));
    }
}

// read zone file, allowing for zones with or without end dates
bool PaceZones::read(QFile &file)
{
    defaults_from_user = false;
    scheme.zone_default.clear();
    scheme.zone_default_is_pct.clear();
    scheme.zone_default_name.clear();
    scheme.zone_default_desc.clear();
    scheme.nzones_default = 0;
    ranges.clear();

    // set up possible warning dialog
    warning = QString();
    int warning_lines = 0;
    const int max_warning_lines = 100;

    // macro to append lines to the warning
    #define append_to_warning(s) \
    if (warning_lines < max_warning_lines) { \
        warning.append(s); }  \
    else if (warning_lines == max_warning_lines) { \
        warning.append("...\n"); } \
    warning_lines++;

    // read using text mode takes care of end-lines
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        err = tr("can't open file");
        return false;
    }
    QTextStream fileStream(&file);
    fileStream.setCodec("UTF-8");

    QRegExp commentrx("\\s*#.*$");
    QRegExp blankrx("^[ \t]*$");
    QRegExp rangerx[] = {
        QRegExp("^\\s*(?:from\\s+)?"                                 // optional "from"
                "((\\d\\d\\d\\d)[-/](\\d{1,2})[-/](\\d{1,2})|BEGIN)" // begin date
                "\\s*([,:]?\\s*(FTP|CV)\\s*=\\s*([\\d\\.]+))?"            // optional {CV/FTP = integer (optional %)}
                "\\s*:?\\s*$",                                       // optional :
                Qt::CaseInsensitive),
        QRegExp("^\\s*(?:from\\s+)?"                                 // optional "from"
                "((\\d\\d\\d\\d)[-/](\\d{1,2})[-/](\\d{1,2})|BEGIN)" // begin date
                "\\s+(?:until|to|-)\\s+"                             // until
                "((\\d\\d\\d\\d)[-/](\\d{1,2})[-/](\\d{1,2})|END)?"  // end date
                "\\s*:?,?\\s*((FTP|CV)\\s*=\\s*([\\d\\.]+))?"          // optional {CV/FTP = integer (optional %)}
                "\\s*:?\\s*$",                                       // optional :
                Qt::CaseInsensitive)
    };
    QRegExp zonerx("^\\s*([^ ,][^,]*),\\s*([^ ,][^,]*),\\s*"
                   "([\\d\\.]+)\\s*(%?)\\s*(?:,\\s*([\\d\\.]+|MAX)\\s*(%?)\\s*)?$",
                   Qt::CaseInsensitive);
    QRegExp zonedefaultsx("^\\s*(?:zone)?\\s*defaults?\\s*:?\\s*$",
                          Qt::CaseInsensitive);

    int lineno = 0;

    // the current range in the file
    // PaceZoneRange *range = NULL;
    bool in_range = false;
    QDate begin = date_zero, end = date_infinity;
    double cv=0;
    QList<PaceZoneInfo> zoneInfos;

    // true if zone defaults are found in the file (then we need to write them)
    bool zones_are_defaults = false;

    while (!fileStream.atEnd() ) {
        ++lineno;
        QString line = fileStream.readLine();
        int pos = commentrx.indexIn(line, 0);
        if (pos != -1)
            line = line.left(pos);
        if (blankrx.indexIn(line, 0) == 0)
            continue;
        // check for default zone range definition (may be followed by zone definitions)
        if (zonedefaultsx.indexIn(line, 0) != -1) {
            zones_are_defaults = true;

            // defaults are allowed only at the beginning of the file
            if (ranges.size()) {
                err = tr("Zone defaults must be specified at head of %1 file").arg(fileName_);
                return false;
            }

            // only one set of defaults is allowed
            if (scheme.nzones_default) {
                err = tr("Only one set of zone defaults may be specified in %1 file").arg(fileName_);
                return false;
            }

            continue;
        }

        // check for range specification (may be followed by zone definitions)
        for (int r=0; r<2; r++) {

            if (rangerx[r].indexIn(line, 0) != -1) {

                if (in_range) {

                    // if zones are empty, then generate them
                    PaceZoneRange range(begin, end, cv);
                    range.zones = zoneInfos;

                    if (range.zones.empty()) {

                        if (range.cv > 0) { 

                            setZonesFromCV(range); 

                        } else {

                            err = tr("line %1: read new range without reading "
                                     "any zones for previous one").arg(lineno);
                            file.close();
                            return false;
                        }

                    } else {

                        qSort(range.zones);

                    }
                    ranges.append(range);
                }

                in_range = true;
                zones_are_defaults = false;
                zoneInfos.clear();

                // process the beginning date
                if (rangerx[r].cap(1) == "BEGIN") {

                    begin = date_zero; 

                } else {

                    begin = QDate(rangerx[r].cap(2).toInt(),
                                  rangerx[r].cap(3).toInt(),
                                  rangerx[r].cap(4).toInt());
                }

                // process an end date, if any, else it is null
                if (rangerx[r].cap(5) == "END") {

                    end = date_infinity; 

                } else if (rangerx[r].cap(6).toInt() || rangerx[r].cap(7).toInt() || rangerx[r].cap(8).toInt()) {

                    end = QDate(rangerx[r].cap(6).toInt(),
                                rangerx[r].cap(7).toInt(),
                                rangerx[r].cap(8).toInt());

                } else {

                    end = QDate();
                }

                // set up the range, capturing CV if it's specified
                // range = new PaceZoneRange(begin, end);
                int nCV = (r ? 11 : 7);
                if (rangerx[r].captureCount() == (nCV)) cv = rangerx[r].cap(nCV).toDouble(); 
                else cv = 0;

                // bleck
                goto next_line;
            }
        }

        // check for zone definition
        if (zonerx.indexIn(line, 0) != -1) {

            if (!(in_range || zones_are_defaults)) {

                err = tr("line %1: read zone without "
                         "preceding date range").arg(lineno);
                file.close();
                return false;
            }

            double lo = zonerx.cap(3).toDouble();

            // allow for zone specified as % of CV
            bool lo_is_pct = false;
            if (zonerx.cap(4) == "%") {

                if (zones_are_defaults) {

                    lo_is_pct = true;

                } else if (cv > 0)  {

                    lo = double(lo * cv / 100.00f);

                } else {

                    err = tr("attempt to set zone based on % of "
                             "CV without setting CV in line number %1.\n").arg(lineno);
                    file.close();
                    return false;
                }
            }

            double hi;
            // if this is not a zone defaults specification, process possible hi end of zones
            if (zones_are_defaults || zonerx.cap(5).isEmpty()) {

                hi = -1;   // signal an undefined number

            } else if (zonerx.cap(5) == "MAX") {

                hi = INT_MAX;

            } else {

                hi = zonerx.cap(5).toDouble();

                // allow for zone specified as % of CV
                if (zonerx.cap(5) == "%") {

                    if (cv > 0) {

                        hi = double(hi * cv / 100.00f);

                    } else {

                        err = tr("attempt to set zone based on % of CV "
                                 "without setting CV in line number %1.\n").
                              arg(lineno);
                        file.close();
                        return false;

                    }
                }
            }

            if (zones_are_defaults) {

                scheme.nzones_default++;
                scheme.zone_default_is_pct.append(lo_is_pct);
                scheme.zone_default.append(lo);
                scheme.zone_default_name.append(zonerx.cap(1));
                scheme.zone_default_desc.append(zonerx.cap(2));
                defaults_from_user = true;

            } else {

                PaceZoneInfo zone(zonerx.cap(1), zonerx.cap(2), lo, hi);
                zoneInfos.append(zone);
            }
        }
next_line: {}
    }

    if (in_range) {

        PaceZoneRange range(begin, end, cv);
        range.zones = zoneInfos;

        if (range.zones.empty()) {

            if (range.cv > 0) {

                setZonesFromCV(range);

            } else {

                err = tr("file ended without reading any zones for last range");
                file.close();
                return false;
            }

        } else {

            qSort(range.zones);
        }

        ranges.append(range);
    }
    file.close();

    // sort the ranges
    qSort(ranges);

    // set the default zones if not in file
    if (!scheme.nzones_default)  {

        // do we have a zone which is explicitly set?
        for (int i=0; i<ranges.count(); i++) {

            if (ranges[i].zonesSetFromCV == false) {

                // set the defaults using this one!
                scheme.nzones_default = ranges[i].zones.count();
                for (int j=0; j<scheme.nzones_default; j++) {

                    scheme.zone_default.append(((double)ranges[i].zones[j].lo / (double)ranges[i].cv) * 100.00);
                    scheme.zone_default_is_pct.append(true);
                    scheme.zone_default_name.append(ranges[i].zones[j].name);
                    scheme.zone_default_desc.append(ranges[i].zones[j].desc);
                }
            }
        }

        // still not set then reset to defaults as usual
        if (!scheme.nzones_default) initializeZoneParameters();
    }

    // resolve undefined endpoints in ranges and zones
    for (int nr = 0; nr < ranges.size(); nr++) {

        // clean up gaps or overlaps in zone ranges
        if (ranges[nr].end.isNull()) {
            ranges[nr].end =
                (nr < ranges.size() - 1) ?
                ranges[nr + 1].begin :
                date_infinity;

        } else if ((nr < ranges.size() - 1) && (ranges[nr + 1].begin != ranges[nr].end)) {

            append_to_warning(tr("Setting end date of range %1 "
                                 "to start date of range %2.\n").
                              arg(nr + 1).
                              arg(nr + 2)
                              );

            ranges[nr].end = ranges[nr + 1].begin;

        } else if ((nr == ranges.size() - 1) && (ranges[nr].end < QDate::currentDate())) {

            append_to_warning(tr("Extending final range %1 to infinite "
                                 "to include present date.\n").arg(nr + 1));
            ranges[nr].end = date_infinity;
        }

        if (ranges[nr].cv <= 0) {

            err = tr("CV must be greater than zero in zone "
                     "range %1 of %2").arg(nr + 1).arg(fileName_);
            return false;
        }

        if (ranges[nr].zones.size()) {

            // check that the first zone starts with zero
            // ranges[nr].zones[0].lo = 0; // there is no reason we should enforce this

            // resolve zone end powers
            for (int nz = 0; nz < ranges[nr].zones.size(); nz++) {

                if (ranges[nr].zones[nz].hi == -1) {
                    ranges[nr].zones[nz].hi =
                        (nz < ranges[nr].zones.size() - 1) ?
                        ranges[nr].zones[nz + 1].lo :
                        INT_MAX;

                } else if ((nz < ranges[nr].zones.size() - 1) && (ranges[nr].zones[nz].hi != ranges[nr].zones[nz + 1].lo)) {

                    if (std::abs(ranges[nr].zones[nz].hi - ranges[nr].zones[nz + 1].lo) > 4.0) {

                        append_to_warning(tr("Range %1: matching top of zone %2 "
                                             "(%3) to bottom of zone %4 (%5).\n").
                                          arg(nr + 1).
                                          arg(ranges[nr].zones[nz].name).
                                          arg(ranges[nr].zones[nz].hi).
                                          arg(ranges[nr].zones[nz + 1].name).
                                          arg(ranges[nr].zones[nz + 1].lo)
                                          );

                    }
                    ranges[nr].zones[nz].hi = ranges[nr].zones[nz + 1].lo;

                } else if ((nz == ranges[nr].zones.size() - 1) && (ranges[nr].zones[nz].hi < INT_MAX)) {

                    append_to_warning(tr("Range %1: setting top of zone %2 from %3 to MAX.\n").
                                      arg(nr + 1).
                                      arg(ranges[nr].zones[nz].name).
                                      arg(ranges[nr].zones[nz].hi)
                                      );
                    ranges[nr].zones[nz].hi = INT_MAX;
                }
            }
        }
    }

    // mark zones as modified so pages which depend on zones can be updated
    modificationTime = QDateTime::currentDateTime();

    return true;
}

// note empty dates are treated as automatic matches for begin or
// end of range
int PaceZones::whichRange(const QDate &date) const
{
    for (int rnum = 0; rnum < ranges.size(); ++rnum) {

        const PaceZoneRange &range = ranges[rnum];
        if (((date >= range.begin) || (range.begin.isNull())) &&
            ((date < range.end) || (range.end.isNull())))
            return rnum;

    }
    return -1;
}

int PaceZones::numZones(int rnum) const
{
    assert(rnum < ranges.size());
    return ranges[rnum].zones.size();
}

int PaceZones::whichZone(int rnum, double value) const
{
    assert(rnum < ranges.size());
    const PaceZoneRange &range = ranges[rnum];

    for (int j = 0; j < range.zones.size(); ++j) {

        const PaceZoneInfo &info = range.zones[j];

        // note: the "end" of range is actually in the next zone
        if ((value >= info.lo) && (value < info.hi))
            return j;

    }

    // if we got here either it is negative, nan, inf or way high
    return -1;
}

void PaceZones::zoneInfo(int rnum, int znum, QString &name, QString &description, double &low, double &high) const
{
    assert(rnum < ranges.size());
    const PaceZoneRange &range = ranges[rnum];
    assert(znum < range.zones.size());
    const PaceZoneInfo &zone = range.zones[znum];
    name = zone.name;
    description = zone.desc;
    low = zone.lo;
    high = zone.hi;
}

double PaceZones::getCV(int rnum) const
{
    assert(rnum < ranges.size());
    return ranges[rnum].cv;
}

void PaceZones::setCV(int rnum, double cv)
{
    ranges[rnum].cv = cv;
    modificationTime = QDateTime::currentDateTime();
}

// generate a list of zones from CV
int PaceZones::lowsFromCV(QList <double> *lows, double cv) const {
    lows->clear();

    for (int z = 0; z < scheme.nzones_default; z++) {

        lows->append(scheme.zone_default_is_pct[z] ?  scheme.zone_default[z] * cv / 100.00f : scheme.zone_default[z]);

    }

    return scheme.nzones_default;
}

// access the zone name
QString PaceZones::getDefaultZoneName(int z) const {
    return scheme.zone_default_name[z];
}

// access the zone description
QString PaceZones::getDefaultZoneDesc(int z) const {
    return scheme.zone_default_desc[z];
}

// set the zones from the CV value (the cv variable)
void PaceZones::setZonesFromCV(PaceZoneRange &range)
{
    range.zones.clear();

    if (scheme.nzones_default == 0) initializeZoneParameters();

    for (int i = 0; i < scheme.nzones_default; i++) {

        double lo = scheme.zone_default_is_pct[i] ? scheme.zone_default[i] * range.cv / 100.00f : scheme.zone_default[i];
        double hi = lo;
        PaceZoneInfo zone(scheme.zone_default_name[i], scheme.zone_default_desc[i], lo, hi);
        range.zones.append(zone);
    }

    // sort the zones (some may be pct, others absolute, so zones need to be sorted,
    // rather than the defaults
    qSort(range.zones);

    // set zone end dates
    for (int i = 0; i < range.zones.size(); i++) {

        range.zones[i].hi =
            (i < scheme.nzones_default - 1) ?
            range.zones[i + 1].lo :
            INT_MAX;
    }

    // mark that the zones were set from CV, so if zones are subsequently
    // written, only CV is saved
    range.zonesSetFromCV = true;
}

void PaceZones::setZonesFromCV(int rnum)
{
    assert((rnum >= 0) && (rnum < ranges.size()));
    setZonesFromCV(ranges[rnum]);
}

// return the list of starting values of zones for a given range
QList <double> PaceZones::getZoneLows(int rnum) const
{
    if (rnum >= ranges.size()) return QList <double>();

    const PaceZoneRange &range = ranges[rnum];
    QList <double> return_values;

    for (int i = 0; i < range.zones.size(); i++) {
        return_values.append(ranges[rnum].zones[i].lo);
    }

    return return_values;
}

// return the list of ending values of zones for a given range
QList <double> PaceZones::getZoneHighs(int rnum) const
{

    if (rnum >= ranges.size()) return QList <double>();

    const PaceZoneRange &range = ranges[rnum];
    QList <double> return_values;

    for (int i = 0; i < range.zones.size(); i++) {
        return_values.append(ranges[rnum].zones[i].hi);
    }

    return return_values;
}

// return the list of zone names
QList <QString> PaceZones::getZoneNames(int rnum) const
{
    if (rnum >= ranges.size()) return QList <QString>(); 

    const PaceZoneRange &range = ranges[rnum];
    QList <QString> return_values;

    for (int i = 0; i < range.zones.size(); i++) {
        return_values.append(ranges[rnum].zones[i].name);
    }

    return return_values;
}

QString PaceZones::summarize(int rnum, QVector<double> &time_in_zone, QColor color) const
{

    assert(rnum < ranges.size());
    const PaceZoneRange &range = ranges[rnum];
    if (time_in_zone.size() != range.zones.size()) time_in_zone.resize(range.zones.size());

    // are we in metric or imperial ?
    bool metric = appsettings->value(this, GC_PACE, true).toBool();
    QString cvunit = metric ? "kph" : "mph";
    double cvfactor = metric ? 1.0f : KM_PER_MILE;
    QString paceunit = this->paceUnits(metric);

    QString summary;
    if(range.cv > 0) {
        summary += "<table align=\"center\" width=\"70%\" border=\"0\">";
        summary += "<tr><td align=\"center\">";
        summary += tr("Critical Velocity: %3%4 (%2%1)").arg(cvunit).arg(range.cv / cvfactor, 0, 'f', 2)
                                                        .arg(this->kphToPaceString(range.cv, metric))
                                                        .arg(paceunit);
        summary += "</td></tr></table>";
    }
    summary += "<table align=\"center\" width=\"70%\" ";
    summary += "border=\"0\">";
    summary += "<tr>";
    summary += tr("<td align=\"center\">Zone</td>");
    summary += tr("<td align=\"center\">Description</td>");
    summary += tr("<td align=\"center\">Low (%1)</td>").arg(paceunit);
    summary += tr("<td align=\"center\">High (%1)</td>").arg(paceunit);
    summary += tr("<td align=\"center\">Time</td>");
    summary += tr("<td align=\"center\">%</td>");
    summary += "</tr>";

    double duration = 0;
    foreach(double v, time_in_zone) {
        duration += v;
    }

    for (int zone = 0; zone < time_in_zone.size(); ++zone) {

        if (time_in_zone[zone] > 0.0) {
            QString name, desc;
            double lo, hi;
            zoneInfo(rnum, zone, name, desc, lo, hi);
            if (zone % 2 == 0)
                summary += "<tr bgcolor='" + color.name() + "'>";
            else
                summary += "<tr>";
            summary += QString("<td align=\"center\">%1</td>").arg(name);
            summary += QString("<td align=\"center\">%1</td>").arg(desc);
            summary += QString("<td align=\"center\">%1</td>").arg(this->kphToPaceString(lo, metric));
            if (hi == DBL_MAX)
                summary += "<td align=\"center\">MAX</td>";
            else
                summary += QString("<td align=\"center\">%1</td>").arg(this->kphToPaceString(hi, metric));
            summary += QString("<td align=\"center\">%1</td>")
                    .arg(time_to_string((unsigned) round(time_in_zone[zone])));
            summary += QString("<td align=\"center\">%1</td>")
                    .arg((double)time_in_zone[zone]/duration * 100, 0, 'f', 0);
            summary += "</tr>";
        }
    }
    summary += "</table>";
    return summary;
}

#define USE_SHORT_POWER_ZONES_FORMAT true   /* whether a less redundent format should be used */
void PaceZones::write(QDir home)
{
    QString strzones;

    // always write the defaults (config pane can adjust)
    strzones += QString("DEFAULTS:\n");

    for (int z = 0; z < scheme.nzones_default; z++) {

        strzones += QString("%1,%2,%3%4\n").
                    arg(scheme.zone_default_name[z]).
                    arg(scheme.zone_default_desc[z]).
                    arg(scheme.zone_default[z]).
                    arg(scheme.zone_default_is_pct[z] ? "%" : "");

    }
    strzones += QString("\n");

    for (int i = 0; i < ranges.size(); i++) {

        double cv = getCV(i);

        // print header for range
        // note this explicitly sets the first and last ranges such that all time is spanned

#if USE_SHORT_POWER_ZONES_FORMAT

        // note: BEGIN is not needed anymore
        //       since it becomes Jan 01 1900
        strzones += QString("%1: CV=%2").arg(getStartDate(i).toString("yyyy/MM/dd")).arg(cv);
        strzones += QString("\n");

        // step through and print the zones if they've been explicitly set
        if (!ranges[i].zonesSetFromCV) {
            for (int j = 0; j < ranges[i].zones.size(); j++) {
                const PaceZoneInfo &zi = ranges[i].zones[j];
                strzones += QString("%1,%2,%3\n").arg(zi.name).arg(zi.desc).arg(zi.lo);
            }
            strzones += QString("\n");
        }
#else
        if(ranges.size() <= 1) {

            strzones += QString("FROM BEGIN UNTIL END, CV=%1:").arg(cv);

        } else if (i == 0) {

            strzones += QString("FROM BEGIN UNTIL %1, CV=%2:").arg(getEndDate(i).toString("yyyy/MM/dd")).arg(cv);

        } else if (i == ranges.size() - 1) {

            strzones += QString("FROM %1 UNTIL END, CV=%2:").arg(getStartDate(i).toString("yyyy/MM/dd")).arg(cv);
        } else {
            strzones += QString("FROM %1 UNTIL %2, CV=%3:").arg(getStartDate(i).toString("yyyy/MM/dd")).arg(getEndDate(i).toString("yyyy/MM/dd")).arg(cv);
        }

        strzones += QString("\n");
        for (int j = 0; j < ranges[i].zones.size(); j++) {

            const PaceZoneInfo &zi = ranges[i].zones[j];

            if (ranges[i].zones[j].hi == INT_MAX) {
                strzones += QString("%1,%2,%3,MAX\n").arg(zi.name).arg(zi.desc).arg(zi.lo);

            } else {

                strzones += QString("%1,%2,%3,%4\n").arg(zi.name).arg(zi.desc).arg(zi.lo).arg(zi.hi);
            }
        }
        strzones += QString("\n");
#endif
    }

    QFile file(home.canonicalPath() + "/" + fileName_);
    if (file.open(QFile::WriteOnly)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << strzones;
        file.close();
    } else {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving Pace Zones"));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(home.canonicalPath() + "/" + fileName_));
        msgBox.exec();
        return;
    }
}

void PaceZones::addZoneRange(QDate _start, QDate _end, double _cv)
{
    ranges.append(PaceZoneRange(_start, _end, _cv));
}

// insert a new zone range using the current scheme
// return the range number
int PaceZones::addZoneRange(QDate _start, double _cv)
{
    int rnum;

    // where to add this range?
    for(rnum=0; rnum < ranges.count(); rnum++) if (ranges[rnum].begin > _start) break;

    // at the end ?
    if (rnum == ranges.count()) ranges.append(PaceZoneRange(_start, date_infinity, _cv)); 
    else ranges.insert(rnum, PaceZoneRange(_start, ranges[rnum].begin, _cv));

    // modify previous end date
    if (rnum) ranges[rnum-1].end = _start;

    // set zones from CV
    if (_cv > 0) {
        setCV(rnum, _cv);
        setZonesFromCV(rnum);
    }

    return rnum;
}

void PaceZones::addZoneRange()
{
    ranges.append(PaceZoneRange(date_zero, date_infinity));
}

void PaceZones::setEndDate(int rnum, QDate endDate)
{
    ranges[rnum].end = endDate;
    modificationTime = QDateTime::currentDateTime();
}

void PaceZones::setStartDate(int rnum, QDate startDate)
{
    ranges[rnum].begin = startDate;
    modificationTime = QDateTime::currentDateTime();
}

QDate PaceZones::getStartDate(int rnum) const
{
    assert(rnum >= 0);
    return ranges[rnum].begin;
}

QString PaceZones::getStartDateString(int rnum) const
{
    assert(rnum >= 0);
    QDate d = ranges[rnum].begin;
    return (d.isNull() ? "BEGIN" : d.toString());
}

QDate PaceZones::getEndDate(int rnum) const
{
    assert(rnum >= 0);
    return ranges[rnum].end;
}

QString PaceZones::getEndDateString(int rnum) const
{
    assert(rnum >= 0);
    QDate d = ranges[rnum].end;
    return (d.isNull() ? "END" : d.toString());
}

int PaceZones::getRangeSize() const
{
    return ranges.size();
}

// generate a zone color with a specific number of zones
QColor paceZoneColor(int z, int) {

    switch(z) {
    case 0: return GColor(CZONE1); break;
    case 1: return GColor(CZONE2); break;
    case 2: return GColor(CZONE3); break;
    case 3: return GColor(CZONE4); break;
    case 4: return GColor(CZONE5); break;
    case 5: return GColor(CZONE6); break;
    case 6: return GColor(CZONE7); break;
    case 7: return GColor(CZONE8); break;
    case 8: return GColor(CZONE9); break;
    case 9: return GColor(CZONE10); break;
    default: return QColor(128,128,128); break;
    }
}

// delete a range, extend an adjacent (prior if available, otherwise next)
// range to cover the same time period, then return the number of the new range
// covering the date range of the deleted range or -1 if none left
int PaceZones::deleteRange(int rnum) {
    // check bounds - silently fail, don't assert
    assert (rnum < ranges.count() && rnum >= 0);

    // extend the previous range to the end of this range
    // but only if we have a previous range
    if (rnum > 0) setEndDate(rnum-1, getEndDate(rnum));
    // delete this range then
    ranges.removeAt(rnum);

    return rnum-1;
}

quint16
PaceZones::getFingerprint() const
{
    quint64 x = 0;
    for (int i=0; i<ranges.size(); i++) {
        // from
        x += ranges[i].begin.toJulianDay();

        // to
        x += ranges[i].end.toJulianDay();

        // CV
        x += int(double(100.0f * ranges[i].cv));

        // each zone definition (manual edit/default changed)
        for (int j=0; j<ranges[i].zones.count(); j++) {
            x += int(double(100.0f * ranges[i].zones[j].lo));
        }
    }
    QByteArray ba = QByteArray::number(x);

    return qChecksum(ba, ba.length()); 
}

quint16
PaceZones::getFingerprint(QDate forDate) const
{
    quint64 x = 0;

    int i = whichRange(forDate);
    if (i>=0) {

        // CV
        x += int(double(100.0f * ranges[i].cv));

        // each zone definition (manual edit/default changed)
        for (int j=0; j<ranges[i].zones.count(); j++) {
            x += int(double(100.0f * ranges[i].zones[j].lo));
        }
    }
    QByteArray ba = QByteArray::number(x);

    return qChecksum(ba, ba.length()); 
}

double
PaceZones::kphFromTime(QTimeEdit *cvedit, bool metric) const
{
    // get the value from a time edit and convert
    // it to kph so we can store it in the zones file

    double secs = cvedit->time().secsTo(QTime(0,0,0)) * -1;
    if (swim)
        return (metric ? 1.00f : METERS_PER_YARD ) * (360.00f / secs);
    else
        return (metric ? 1.00f : KM_PER_MILE ) * (3600.00f / secs);
}

QString
PaceZones::kphToPaceString(double kph, bool metric) const
{
    return kphToPace(kph, metric, swim);
}

QString
PaceZones::paceUnits(bool metric) const
{
    if (swim)
        return metric ? tr("min/100m") : tr("min/100yd");
    else
        return metric ? tr("min/km") : tr("min/mile");
}

QString
PaceZones::paceSetting() const
{
    return swim ? GC_SWIMPACE : GC_PACE;
}

bool
PaceZones::isPaceUnit(QString units)
{
    return (units == "min/km") || (units == tr("min/km")) ||
           (units == "min/mile") || (units == tr("min/mile")) ||
           (units == "min/100m") || (units == tr("min/100m")) ||
           (units == "min/100yd") || (units ==  tr("min/100yd"));
}
