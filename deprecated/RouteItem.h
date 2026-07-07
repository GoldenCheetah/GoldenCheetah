/*
 * Copyright (c) 2011, 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef _GC_RouteItem_h
#define _GC_RouteItem_h 1
#include "GoldenCheetah.h"

#include "Route.h"
#include <QtGui>
#include <QTreeWidgetItem>

class RideFile;
class MainWindow;

// Because we have subclassed QTreeWidgetItem we
// need to use our own type, this MUST be greater than
// QTreeWidgetItem::UserType according to the docs
#define ROUTE_TYPE 0 // (QTreeWidgetItem::UserType+2)
#define FOLDER_TYPE 1 // (QTreeWidgetItem::UserType+2)

class RouteItem : public QObject, public QTreeWidgetItem //<< for signals/slots
{

    Q_OBJECT
    G_OBJECT


    protected:

        RideFile *ride_;
        RouteSegment *route;
        const RouteRide *routeRide;

        QStringList errors_;
        Context *context; // to notify widgets when date/time changes
        bool isdirty;

    public slots:
        void modified();
        void reverted();
        void saved();

    signals:

    public:

        bool isedit; // is being edited at the moment



        QString path;
        QString fileName;
        QDateTime dateTime;
        RideFile *ride();
        const QStringList errors() { return errors_; }

        QString notesFileName;

        RouteItem(RouteSegment *route, int type, QString path,
                 QString fileName, const QDateTime &dateTime,
                 MainWindow *main);

        RouteItem(RouteSegment *route, const RouteRide *routeItem, QString path, Context *context);

        RouteItem(RouteSegment *route, int type, QTreeWidget *parent, Context *context);

        void setText(int column, const QString &atext);
        void setTextAlignment(int column, int alignment);

        void setData(int column, int role, const QVariant &value, bool write);
        void setData(int column, int role, const QVariant &value);

        void setDirty(bool);
        bool isDirty() { return isdirty; }
        void setFileName(QString, QString);
        void setStartTime(QDateTime);
        void freeMemory();

};
#endif // _GC_RouteItem_h
