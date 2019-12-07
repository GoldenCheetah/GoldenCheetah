/*
 * Copyright (c) 2010 Damien Grauser (Damien.Grauser@pev-geneve.ch), Sean C. Rhea (srhea@srhea.net)
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

#ifndef _HrZones_h
#define _HrZones_h
#include "GoldenCheetah.h"

#include <QtCore>

// A zone "scheme" defines how power zones
// are calculated as a percentage of LT
// The default is to use Coggan percentages
// but this can be overriden in hr.zones
struct HrZoneScheme {
    QList <int> zone_default;
    QList <bool> zone_default_is_pct;
    QList <QString> zone_default_name;
    QList <QString> zone_default_desc;
    QList <double> zone_default_trimp;
    int nzones_default;
};

// A zone "info" defines a *single zone*
// in absolute watts terms e.g.
// "L4" "Threshold"
struct HrZoneInfo {
    QString name, desc;
    int lo, hi;
    double trimp;
    HrZoneInfo(const QString &n, const QString &d, int l, int h, double t) :
        name(n), desc(d), lo(l), hi(h), trimp(t) {}

    // used by qSort()
    bool operator< (const HrZoneInfo &right) const {
        return ((lo < right.lo) || ((lo == right.lo) && (hi < right.hi)));
    }
};

// A zone "range" defines the power zones
// that are active for a *specific date period*
// e.g. between 01/01/2008 and 01/04/2008
//      my LT was 170 and I chose to setup
//      5 zoneinfos from Active Recovery through
//      to VO2Max
struct HrZoneRange {
    QDate begin, end;
    int lt;
    int restHr;
    int maxHr;

    QList<HrZoneInfo> zones;
    bool hrZonesSetFromLT;
    HrZoneRange(const QDate &b, const QDate &e) :
        begin(b), end(e), lt(0), hrZonesSetFromLT(false) {}
    HrZoneRange(const QDate &b, const QDate &e, int _lt, int _restHr, int _maxHr) :
        begin(b), end(e), lt(_lt), restHr(_restHr), maxHr(_maxHr), hrZonesSetFromLT(false) {}

    // used by qSort()
    bool operator< (const HrZoneRange &right) const {
        return (((! right.begin.isNull()) &&
                (begin.isNull() || begin < right.begin )) ||
                ((begin == right.begin) && (! end.isNull()) &&
                ( right.end.isNull() || end < right.end )));
    }
};


class HrZones : public QObject
{
    Q_OBJECT
    G_OBJECT


    private:

        // Sport
        bool run;

        // Scheme
        bool defaults_from_user;
        HrZoneScheme scheme;

        // LT History
        QList<HrZoneRange> ranges;

        // utility
        QString err, warning, fileName_;
        void setHrZonesFromLT(HrZoneRange &range);

    public:

        HrZones(bool run=false) : run(run), defaults_from_user(false) {
                initializeZoneParameters();
        }

        //
        // Zone settings - Scheme (& default scheme)
        //
        HrZoneScheme getScheme() const { return scheme; }
        void setScheme(HrZoneScheme x) { scheme = x; }

        // get defaults from the current scheme
        QString getDefaultZoneName(int z) const;
        QString getDefaultZoneDesc(int z) const;

        // set zone parameters to either user-specified defaults
        // or to defaults using Coggan's coefficients
        void initializeZoneParameters();

        // Sport
        bool isRun() { return run; }

        //
        // Zone history - Ranges
        //
        // How many ranges in our history
        int getRangeSize() const;

        // Add ranges
        void addHrZoneRange(QDate _start, QDate _end, int _lt, int _restHr, int _maxHr);
        int addHrZoneRange(QDate _start, int _lt, int _restHr, int _maxHr);
        void addHrZoneRange();

        // insert a range from the given date to the end date of the range
        // presently including the date
        int insertRangeAtDate(QDate date, int lt = 0);

        // Get / Set ZoneRange details
        HrZoneRange getHrZoneRange(int rnum) { return ranges[rnum]; }
        void setHrZoneRange(int rnum, HrZoneRange x) { ranges[rnum] = x; }

        // get and set LT for a given range
        int getLT(int rnum) const;
        void setLT(int rnum, int cp);

        // get and set Rest Hr for a given range
        int getRestHr(int rnum) const;
        void setRestHr(int rnum, int restHr);

        // get and set LT for a given range
        int getMaxHr(int rnum) const;
        void setMaxHr(int rnum, int maxHr);

        // calculate and then set zoneinfo for a given range
        void setHrZonesFromLT(int rnum);

        // delete the range rnum, and adjust dates on adjacent zone; return
        // the range number of the range extended to cover the deleted zone
        int deleteRange(const int rnum);

        //
        // read and write hr.zones
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
                      int &low, int &high, double &trimp) const;

        QString summarize(int rnum, QVector<double> &time_in_zone, QColor color = QColor(Qt::darkGray)) const;

        // get all highs/lows for zones (plot shading uses these)
        int lowsFromLT(QList <int> *lows, int LT) const;
        QList <int> getZoneLows(int rnum) const;
        QList <int> getZoneHighs(int rnum) const;
        QList <double> getZoneTrimps(int rnum) const;
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
};

QColor hrZoneColor(int zone, int num_zones);

#endif // _HrZones_h
