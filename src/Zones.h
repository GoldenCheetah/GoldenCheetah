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

#include <QtCore>

class Zones : public QObject
{
    Q_OBJECT

    protected:

        struct ZoneInfo {
            QString name, desc;
            int lo, hi;
            ZoneInfo(const QString &n, const QString &d, int l, int h) :
                name(n), desc(d), lo(l), hi(h) {}
        };

        struct ZoneRange {
            QDate begin, end;
            int ftp;
            QList<ZoneInfo*> zones;
            ZoneRange(const QDate &b, const QDate &e) : 
                begin(b), end(e), ftp(0) {}
            ~ZoneRange() {
                QListIterator<ZoneInfo*> i(zones); 
                while (i.hasNext()) 
                    delete i.next();
            }
        };

        QString err;

    public:

        Zones() {}

        ~Zones() {
            QListIterator<ZoneRange*> i(ranges); 
            while (i.hasNext()) 
                delete i.next();
        }

        void addZoneRange(QDate _start, QDate _end, int _ftp);

        bool read(QFile &file);
        void write(QDir home);
        const QString &errorString() const { return err; }

        int whichRange(const QDate &date) const;
        int numZones(int range) const;
        int whichZone(int range, double value) const;
        void zoneInfo(int range, int zone,
                      QString &name, QString &description,
                      int &low, int &high) const;
        QString summarize(int rnum, double *time_in_zone, int num_zones) const;
        int getCP(int rnum) const;
        void setCP(int rnum, int ftp);
        QDate getStartDate(int rnum);
        QDate getEndDate(int rnum);
        void setEndDate(int rnum, QDate date);
        void setStartDate(int rnum, QDate date);
        QList<ZoneRange*> ranges;

};

#endif // _Zones_h
