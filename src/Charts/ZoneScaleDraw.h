/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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
 *
 */

#ifndef _GC_ZoneScaleDraw_h
#define _GC_ZoneScaleDraw_h 1
#include "GoldenCheetah.h"

#include <qwt_scale_draw.h>
#include "Zones.h"
#include "HrZones.h"

class ZoneScaleDraw: public QwtScaleDraw
{
    public:
        ZoneScaleDraw(const Zones *zones, int range=-1, bool zoneLimited=false) : zones(zones), zoneLimited(zoneLimited) {
            setRange(range);
            setTickLength(QwtScaleDiv::MajorTick, 3);
        }

        // modify later if neccessary
        void setZones(Zones *z) {
            zones=z;
            names.clear();
            from.clear();
            to.clear();
        }

        // when we set the range we are choosing the texts
        void setRange(int x) {
            range=x;
            if (range >= 0) {
                names = zones->getZoneNames(range);
                from = zones->getZoneLows(range);
                to = zones->getZoneHighs(range);
            } else {
                names.clear();
                from.clear();
                to.clear();
            }
        }

        // return label
        virtual QwtText label(double v) const
        {
            if (v <0 || v > (names.count()-1) || range < 0) return QString("");
            else if (!zoneLimited) return names[v];
            else {
                if (v == names.count()-1) return QString("%1 (%2+ %3)").arg(names[v]).arg(from[v]).arg(RideFile::unitName(RideFile::watts, nullptr));
                else return QString("%1 (%2-%3)").arg(names[v]).arg(from[v]).arg(to[v]);
            }
        }

    private:
        const Zones *zones;
        int range;
        bool zoneLimited;
        QList<QString> names;
        QList <int> from, to;
};

class PolarisedZoneScaleDraw: public QwtScaleDraw
{
    public:
        PolarisedZoneScaleDraw() {
            setTickLength(QwtScaleDiv::MajorTick, 3);

            labels << "I"; // translate tr macros !?
            labels << "II";
            labels << "III";
        }

        // return label
        virtual QwtText label(double v) const
        {
            int index = v;
            if (index < 0 || index > labels.count()-1) return QString("");
            return labels.at(index);
        }

    private:
        QList <QString> labels;
};

class WbalZoneScaleDraw: public QwtScaleDraw
{
    public:
        WbalZoneScaleDraw(const Zones *zones, int range=-1, bool zoneLimited=false) {
            setTickLength(QwtScaleDiv::MajorTick, 3);

            int WPRIME = range >= 0 ? zones->getWprime(range) : 0;
            for (int i=0; i<WPrime::zoneCount(); i++) {
                if (zoneLimited && WPRIME > 0) {
                    if (i == WPrime::zoneCount()-1) labels << QString("%1 (%2- %3)").arg(WPrime::zoneName(i)).arg(WPrime::zoneLo(i, WPRIME)).arg(RideFile::unitName(RideFile::wprime, nullptr));
		    else labels << QString("%1 (%2-%3)").arg(WPrime::zoneName(i)).arg(WPrime::zoneLo(i, WPRIME)).arg(WPrime::zoneHi(i, WPRIME));
                } else {
                    labels << WPrime::zoneDesc(i);
                }
            }
        }

        // return label
        virtual QwtText label(double v) const
        {
            int index = v;
            if (index < 0 || index > labels.count()-1) return QString("");
            return labels.at(index);
        }

    private:
        QList <QString> labels;
};

class HrZoneScaleDraw: public QwtScaleDraw
{
    public:
        HrZoneScaleDraw(const HrZones *zones, int range=-1, bool zoneLimited=false) : zones(zones), zoneLimited(zoneLimited) {
            setRange(range);
            setTickLength(QwtScaleDiv::MajorTick, 3);
        }

        // modify later if neccessary
        void setHrZones(HrZones *z) {
            zones=z;
            names.clear();
            from.clear();
            to.clear();
        }

        // when we set the range we are choosing the texts
        void setRange(int x) {
            range=x;
            if (range >= 0) {
                names = zones->getZoneNames(range);
                from = zones->getZoneLows(range);
                to = zones->getZoneHighs(range);
            } else {
                names.clear();
                from.clear();
                to.clear();
            }
        }

        // return label
        virtual QwtText label(double v) const
        {
            if (v < 0 || v > (names.count()-1) || range < 0) return QString("");
            else if (!zoneLimited) return names[v];
            else {
                if (v == names.count()-1) return QString("%1 (%2+ %3)").arg(names[v]).arg(from[v]).arg(RideFile::unitName(RideFile::hr, nullptr));
                else return QString("%1 (%2-%3)").arg(names[v]).arg(from[v]).arg(to[v]);
            }
        }

    private:
        const HrZones *zones;
        int range;
        bool zoneLimited;
        QList<QString> names;
        QList <int> from, to;

};

class PaceZoneScaleDraw: public QwtScaleDraw
{
    public:
        PaceZoneScaleDraw(const PaceZones *zones, int range=-1, bool zoneLimited=false) : zones(zones), zoneLimited(zoneLimited) {
            setRange(range);
            setTickLength(QwtScaleDiv::MajorTick, 3);
        }

        // modify later if neccessary
        void setPaceZones(PaceZones *z) {
            zones=z;
            names.clear();
            from.clear();
            to.clear();
        }

        // when we set the range we are choosing the texts
        void setRange(int x) {
            range=x;
            if (range >= 0) {
                names = zones->getZoneNames(range);
                from = zones->getZoneLows(range);
                to = zones->getZoneHighs(range);
            } else {
                names.clear();
                from.clear();
                to.clear();
            }
            metricPace = appsettings->value(nullptr, zones->paceSetting(), GlobalContext::context()->useMetricUnits).toBool();
        }

        // return label
        virtual QwtText label(double v) const
        {
            if (v < 0 || v > (names.count()-1) || range < 0) return QString("");
            else if (!zoneLimited) return names[v];
            else {
                if (v == names.count()-1) return QString("%1 (%2+ %3)").arg(names[v]).arg(zones->kphToPaceString(from[v], metricPace)).arg(zones->paceUnits(metricPace));
                else return QString("%1 (%2-%3)").arg(names[v]).arg(zones->kphToPaceString(from[v], metricPace)).arg(zones->kphToPaceString(to[v], metricPace));
            }
        }

    private:
        const PaceZones *zones;
        int range;
        bool zoneLimited;
        QList<QString> names;
        QList <double> from, to;
        bool metricPace;

};
#endif
