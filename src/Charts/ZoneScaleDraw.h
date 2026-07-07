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
        PolarisedZoneScaleDraw(const Zones *zones, int range, bool zoneLimited) {
            setTickLength(QwtScaleDiv::MajorTick, 3);

            int AeT = zones && range >= 0 ? zones->getAeT(range) : 0;
            int CP = zones && range >= 0 ? zones->getCP(range) : 0;
            if (zoneLimited && AeT > 0 && CP > 0) {
                labels << QString("I (%1- %2)").arg(AeT).arg(RideFile::unitName(RideFile::watts, nullptr));
                labels << QString("II (%1-%2)").arg(AeT).arg(CP);
                labels << QString("III (%1+ %2)").arg(CP).arg(RideFile::unitName(RideFile::watts, nullptr));
            } else {
                labels << "I";
                labels << "II";
                labels << "III";
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

class HrPolarisedZoneScaleDraw: public QwtScaleDraw
{
    public:
        HrPolarisedZoneScaleDraw(const HrZones *zones, int range, bool zoneLimited) {
            setTickLength(QwtScaleDiv::MajorTick, 3);

            int AeT = zones && range >= 0 ? zones->getAeT(range) : 0;
            int LT = zones && range >= 0 ? zones->getLT(range) : 0;
            if (zoneLimited && AeT > 0 && LT > 0) {
                labels << QString("I (%1- %2)").arg(AeT).arg(RideFile::unitName(RideFile::hr, nullptr));
                labels << QString("II (%1-%2)").arg(AeT).arg(LT);
                labels << QString("III (%1+ %2)").arg(LT).arg(RideFile::unitName(RideFile::hr, nullptr));
            } else {
                labels << "I";
                labels << "II";
                labels << "III";
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

class PacePolarisedZoneScaleDraw: public QwtScaleDraw
{
    public:
        PacePolarisedZoneScaleDraw(const PaceZones *zones, int range, bool zoneLimited) {
            setTickLength(QwtScaleDiv::MajorTick, 3);

            double AeT = zones && range >= 0 ? zones->getAeT(range) : 0;
            double CV = zones && range >= 0 ? zones->getCV(range) : 0;
            if (zoneLimited && AeT > 0 && CV > 0) {
                bool metricPace = appsettings->value(nullptr, zones->paceSetting(), GlobalContext::context()->useMetricUnits).toBool();
                labels << QString("I (%1- %2)").arg(zones->kphToPaceString(AeT, metricPace)).arg(zones->paceUnits(metricPace));
                labels << QString("II (%1-%2)").arg(zones->kphToPaceString(AeT, metricPace)).arg(zones->kphToPaceString(CV, metricPace));
                labels << QString("III (%1+ %2)").arg(zones->kphToPaceString(CV, metricPace)).arg(zones->paceUnits(metricPace));
            } else {
                labels << "I";
                labels << "II";
                labels << "III";
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
#endif
