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

#ifndef _GC_RideItem_h
#define _GC_RideItem_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTreeWidgetItem>
#include "RideMetric.h"

class RideFile;
class RideEditor;
class Context;
class Zones;
class HrZones;

// Because we have subclassed QTreeWidgetItem we
// need to use our own type, this MUST be greater than
// QTreeWidgetItem::UserType according to the docs
#define FOLDER_TYPE 0
#define RIDE_TYPE (QTreeWidgetItem::UserType+1)

class RideItem : public QObject, public QTreeWidgetItem //<< for signals/slots
{

    Q_OBJECT
    G_OBJECT


    protected:

        QVector<double> time_in_zone;
        QVector<double> time_in_hr_zone;
        RideFile *ride_;
        QStringList errors_;
        Context *context; // to notify widgets when date/time changes
        bool isdirty;

    public slots:
        void modified();
        void reverted();
        void saved();
        void notifyRideDataChanged();
        void notifyRideMetadataChanged();

    signals:
        void rideDataChanged();
        void rideMetadataChanged();

    public:

        bool isedit; // is being edited at the moment

        QString path;
        QString fileName;
        QDateTime dateTime;
        // ride() will open the ride if it isn't already when open=true
        // if we pass false then it will just return ride_ so we can
        // traverse currently open rides when config changes
        RideFile *ride(bool open=true);
        const QStringList errors() { return errors_; }
        const Zones *zones;
        const HrZones *hrZones;

        RideItem(int type, QString path,
                 QString fileName, const QDateTime &dateTime,
                 const Zones *zones, const HrZones *hrZones, Context *context);

        void setDirty(bool);
        bool isDirty() { return isdirty; }
        void setFileName(QString, QString);
        void setStartTime(QDateTime);
        void freeMemory();

        int zoneRange();
        int hrZoneRange();
        int numZones();

        int numHrZones();
#if 0
        double timeInZone(int zone);
        double timeInHrZone(int zone);
#endif
};
Q_DECLARE_METATYPE(RideItem*)
#endif // _GC_RideItem_h
