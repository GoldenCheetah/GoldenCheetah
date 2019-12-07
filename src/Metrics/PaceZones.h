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

#ifndef _PaceZones_h
#define _PaceZones_h
#include "GoldenCheetah.h"
#include "Context.h"
#include "Athlete.h"

#include <QtCore>

// A zone "scheme" defines how power zones
// are calculated as a percentage of CV
// The default is to use Coggan percentages
// but this can be overriden in power.zones
struct PaceZoneScheme {
    QList <int> zone_default;
    QList <bool> zone_default_is_pct;
    QList <QString> zone_default_name;
    QList <QString> zone_default_desc;
    int nzones_default;
};

// A zone "info" defines a *single zone*
// in absolute watts terms e.g.
// "L4" "Threshold" is between 270w and 315w
struct PaceZoneInfo {
    QString name, desc;
    double lo, hi;
    PaceZoneInfo(const QString &n, const QString &d, double l, double h) :
        name(n), desc(d), lo(l), hi(h) {}

    // used by qSort()
    bool operator< (const PaceZoneInfo &right) const {
        return ((lo < right.lo) || ((lo == right.lo) && (hi < right.hi)));
    }
};

// A zone "range" defines the power zones
// that are active for a *specific date period*
// e.g. between 01/01/2008 and 01/04/2008
//      my CV was 290 and I chose to setup
//      7 zoneinfos from Active Recovery through
//      to muscular Endurance
struct PaceZoneRange {
    QDate begin, end;
    double cv;
    QList<PaceZoneInfo> zones;
    bool zonesSetFromCV;
    PaceZoneRange(const QDate &b, const QDate &e) :
        begin(b), end(e), cv(0), zonesSetFromCV(false) {}
    PaceZoneRange(const QDate &b, const QDate &e, double _cv) :
        begin(b), end(e), cv(_cv), zonesSetFromCV(false) {}

    // used by qSort()
    bool operator< (const PaceZoneRange &right) const {
        return (((! right.begin.isNull()) &&
                (begin.isNull() || begin < right.begin )) ||
                ((begin == right.begin) && (! end.isNull()) &&
                ( right.end.isNull() || end < right.end )));
    }
};


class PaceZones : public QObject
{
    Q_OBJECT
    G_OBJECT


    private:

        // Sport
        bool swim;

        // Scheme
        bool defaults_from_user;
        PaceZoneScheme scheme;

        // CV History
        QList<PaceZoneRange> ranges;

        // utility
        QString err, warning, fileName_;
        void setZonesFromCV(PaceZoneRange &range);

    public:

        PaceZones(bool swim=false) : swim(swim), defaults_from_user(false) {
                initializeZoneParameters();
        }

        //
        // Zone settings - Scheme (& default scheme)
        //
        PaceZoneScheme getScheme() const { return scheme; }
        void setScheme(PaceZoneScheme x) { scheme = x; }

        // get defaults from the current scheme
        QString getDefaultZoneName(int z) const;
        QString getDefaultZoneDesc(int z) const;

        // set zone parameters to either user-specified defaults
        // or to defaults using Skiba's coefficients
        void initializeZoneParameters();

        // Sport
        bool isSwim() { return swim; }

        //
        // Zone history - Ranges
        //
        // How many ranges in our history
        int getRangeSize() const;

        // Add ranges
        void addZoneRange(QDate _start, QDate _end, double _cv);
        int addZoneRange(QDate _start, double _cv);
        void addZoneRange();

        // Get / Set ZoneRange details
        PaceZoneRange getZoneRange(int rnum) { return ranges[rnum]; }
        void setZoneRange(int rnum, PaceZoneRange x) { ranges[rnum] = x; }

        // get and set CV for a given range
        double getCV(int rnum) const;
        void setCV(int rnum, double cv);

        // calculate and then set zoneinfo for a given range
        void setZonesFromCV(int rnum);

        // delete the range rnum, and adjust dates on adjacent zone; return
        // the range number of the range extended to cover the deleted zone
        int deleteRange(const int rnum);

        //
        // read and write power.zones
        //
        bool read(QFile &file);
        void write(QDir home);
        const QString &fileName() const { return fileName_; }
        const QString &errorString() const { return err; }
        const QString &warningString() const { return warning; }


        //
        // Typical APIs to get details of ranges and zones
        //

        // which range is active for a particular date
        int whichRange(const QDate &date) const;

        // which zone is the power value in for a given range
        // will return -1 if not in any zone
        int whichZone(int range, double value) const;

        // how many zones are there for a given range
        int numZones(int range) const;

        // get zoneinfo  for a given range and zone
        void zoneInfo(int range, int zone,
                      QString &name, QString &description,
                      double &low, double &high) const;

        QString summarize(int rnum, QVector<double> &time_in_zone, QColor color = QColor(Qt::darkGray)) const;

        // get all highs/lows for zones (plot shading uses these)
        int lowsFromCV(QList <double> *lows, double CV) const;
        QList <double> getZoneLows(int rnum) const;
        QList <double> getZoneHighs(int rnum) const;
        QList <QString> getZoneNames(int rnum) const;

        // get/set range start and end date
        QDate getStartDate(int rnum) const;
        QDate getEndDate(int rnum) const;
        QString getStartDateString(int rnum) const;
        QString getEndDateString(int rnum) const;
        void setEndDate(int rnum, QDate date);
        void setStartDate(int rnum, QDate date);

        // When was this last updated?
        QDateTime modificationTime;

        // calculate a CRC for the zones data - used to see if zones
        // data is changed since last referenced in Metric code
        // could also be used in Configuration pages (later)
        quint16 getFingerprint() const;

        // this is the fingerprint for a specific DATE so that we
        // can be more granular -- did the zone config for the date of
        // a particular ride change ?
        quint16 getFingerprint(QDate date) const;

        // convert to/from Pace
        double kphFromTime(QTimeEdit *cvedit, bool metric) const;
        QString kphToPaceString(double kph, bool metric) const;
        QString paceUnits(bool metric) const;
        QString paceSetting() const;
        static bool isPaceUnit(QString unit);
};

QColor paceZoneColor(int zone, int num_zones);

#endif // _PaceZones_h
