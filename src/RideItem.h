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

#include <QtGui>
#include "RideMetric.h"

class RideFile;
class MainWindow;
class Zones;

class RideItem : public QTreeWidgetItem {

    protected:

        QVector<double> time_in_zone;
        RideFile *ride_;
        QStringList errors_;
        MainWindow *main; // to notify widgets when date/time changes
        bool isdirty;

    public:

        QString path;
        QString fileName;
        QDateTime dateTime;
	QDateTime computeMetricsTime;
        RideFile *ride();
        const QStringList errors() { return errors_; }
        const Zones *zones;
        QString notesFileName;

        QHash<QString,RideMetricPtr> metrics;

        RideItem(int type, QString path, 
                 QString fileName, const QDateTime &dateTime,
                 const Zones *zones, QString notesFileName, MainWindow *main);

        void setDirty(bool);
        bool isDirty() { return isdirty; }
        void setFileName(QString, QString);
        void setStartTime(QDateTime);
        void computeMetrics();
        void freeMemory();

        int zoneRange();
        int numZones();
        double timeInZone(int zone);
};
#endif // _GC_RideItem_h

