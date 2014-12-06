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

#ifndef _Zones_h
#define _Zones_h
#include "GoldenCheetah.h"
#include "Context.h"
#include "Athlete.h"

#include <QtCore>

// A zone "scheme" defines how power zones
// are calculated as a percentage of CP
// The default is to use Coggan percentages
// but this can be overriden in power.zones
struct ZoneScheme {
    QList <int> zone_default;
    QList <bool> zone_default_is_pct;
    QList <QString> zone_default_name;
    QList <QString> zone_default_desc;
    int nzones_default;
};

// A zone "info" defines a *single zone*
// in absolute watts terms e.g.
// "L4" "Threshold" is between 270w and 315w
struct ZoneInfo {
    QString name, desc;
    int lo, hi;
    ZoneInfo(const QString &n, const QString &d, int l, int h) :
        name(n), desc(d), lo(l), hi(h) {}

    // used by qSort()
    bool operator< (ZoneInfo right) const {
        return ((lo < right.lo) || ((lo == right.lo) && (hi < right.hi)));
    }
};

// A zone "range" defines the power zones
// that are active for a *specific date period*
// e.g. between 01/01/2008 and 01/04/2008
//      my CP was 290 and I chose to setup
//      7 zoneinfos from Active Recovery through
//      to muscular Endurance
struct ZoneRange {
    QDate begin, end;
    int cp;
    int wprime; // aka awc
    QList<ZoneInfo> zones;
    bool zonesSetFromCP;
    ZoneRange(const QDate &b, const QDate &e) :
        begin(b), end(e), cp(0), wprime(0), zonesSetFromCP(false) {}
    ZoneRange(const QDate &b, const QDate &e, int _cp, int _wprime) :
        begin(b), end(e), cp(_cp), wprime(_wprime), zonesSetFromCP(false) {}

    // used by qSort()
    bool operator< (ZoneRange right) const {
        return (((! right.begin.isNull()) &&
                (begin.isNull() || begin < right.begin )) ||
                ((begin == right.begin) && (! end.isNull()) &&
                ( right.end.isNull() || end < right.end )));
    }
};


class Zones : public QObject
{
    Q_OBJECT
    G_OBJECT


    private:

        // Scheme
        bool defaults_from_user;
        ZoneScheme scheme;

        // CP History
        QList<ZoneRange> ranges;

        // utility
        QString err, warning;
        void setZonesFromCP(ZoneRange &range);

    public:

        Zones() : defaults_from_user(false) {
                initializeZoneParameters();
        }

        //
        // Zone settings - Scheme (& default scheme)
        //
        ZoneScheme getScheme() const { return scheme; }
        void setScheme(ZoneScheme x) { scheme = x; }

        // get defaults from the current scheme
        QString getDefaultZoneName(int z) const;
        QString getDefaultZoneDesc(int z) const;

        // set zone parameters to either user-specified defaults
        // or to defaults using Coggan's coefficients
        void initializeZoneParameters();

        //
        // Zone history - Ranges
        //
        // How many ranges in our history
        int getRangeSize() const;

        // Add ranges
        void addZoneRange(QDate _start, QDate _end, int _cp, int _wprime);
        int addZoneRange(QDate _start, int _cp, int _wprime);
        void addZoneRange();

        // Get / Set ZoneRange details
        ZoneRange getZoneRange(int rnum) { return ranges[rnum]; }
        void setZoneRange(int rnum, ZoneRange x) { ranges[rnum] = x; }

        // get and set CP for a given range
        int getCP(int rnum) const;
        void setCP(int rnum, int cp);
        int getWprime(int rnum) const;
        void setWprime(int rnum, int wprime);

        // calculate and then set zoneinfo for a given range
        void setZonesFromCP(int rnum);

        // delete the range rnum, and adjust dates on adjacent zone; return
        // the range number of the range extended to cover the deleted zone
        int deleteRange(const int rnum);

        //
        // read and write power.zones
        //
        bool read(QFile &file);
        void write(QDir home);
        const QString &errorString() const { return err; }
        const QString &warningString() const { return warning; }


        //
        // Typical APIs to get details of ranges and zones
        //

        // which range is active for a particular date
        int whichRange(const QDate &date) const;

        // which zone is the power value in for a given range
        int whichZone(int range, double value) const;

        // how many zones are there for a given range
        int numZones(int range) const;

        // get zoneinfo  for a given range and zone
        void zoneInfo(int range, int zone,
                      QString &name, QString &description,
                      int &low, int &high) const;

        QString summarize(int rnum, QVector<double> &time_in_zone, QColor color = QColor(Qt::darkGray)) const;

        // get all highs/lows for zones (plot shading uses these)
        int lowsFromCP(QList <int> *lows, int CP) const;
        QList <int> getZoneLows(int rnum) const;
        QList <int> getZoneHighs(int rnum) const;
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
        quint16 getFingerprint(Context *context) const;

        // this is the fingerprint for a specific DATE so that we
        // can be more granular -- did the zone config for the date of
        // a particular ride change ?
        quint16 getFingerprint(Context *context, QDate date) const;
};

QColor zoneColor(int zone, int num_zones);

#endif // _Zones_h
