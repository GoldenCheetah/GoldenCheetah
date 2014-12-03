/*
 * Copyright (c) 2011, 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
 *
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

#include <QTreeWidgetItem>
#include "RouteItem.h"
#include "RideMetric.h"
#include "RideFile.h"
#include "RideItem.h"
#include "Athlete.h"
#include "Zones.h"
#include "HrZones.h"
#include <assert.h>
#include <math.h>

/*RouteItem::RouteItem(RouteSegment *route, int type,
                   QString path, QString fileName, const QDateTime &dateTime,
                   MainWindow *main) :
    QTreeWidgetItem(type), route(route), ride_(NULL), main(main), isdirty(false), isedit(false), path(path), fileName(fileName),
    dateTime(dateTime)
{
    setText(0, dateTime.toString("MMM d, yyyy"));
    setText(1, dateTime.toString("h:mm AP"));
    setText(2, "");
    setTextAlignment(1, Qt::AlignRight);
    setTextAlignment(2, Qt::AlignRight);
}*/

RouteItem::RouteItem(RouteSegment *route, const RouteRide *routeRide,
                   QString path, Context *context) :
    QTreeWidgetItem(ROUTE_TYPE), route(route), routeRide(routeRide),
    ride_(NULL), context(context), isdirty(false), isedit(false), path(path)
{
    QDateTime dateTime = routeRide->startTime.addSecs(routeRide->start);

    setText(0, dateTime.toString("MMM d, yyyy"));
    setText(1, dateTime.toString("h:mm AP"));
    setText(2, "");
    setTextAlignment(1, Qt::AlignRight);
    setTextAlignment(2, Qt::AlignRight);

    QDateTime dt;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(context->athlete->home->activities()));
    while (i.hasNext()) {
        QString name = i.next(), notesFileName;

        if ((route->parseRideFileName(context, name, &notesFileName, &dt)) && (dt == routeRide->startTime)) { //
            fileName = name;
        }
    }
}

RouteItem::RouteItem(RouteSegment *route, int type,
                     QTreeWidget *parent,
                     Context *context) :
        QTreeWidgetItem(parent, type), route(route), context(context)
{

}

void
RouteItem::setText(int column, const QString &atext)
{
    //qDebug() << "setText";
    setData(column, Qt::DisplayRole, atext, false);
}

void
RouteItem::setTextAlignment(int column, int alignment)
{
    //qDebug() << "setTextAlignment";
    setData(column, Qt::TextAlignmentRole, alignment, false);
}

void
RouteItem::setData(int column, int role, const QVariant &value, bool write)
{
    //qDebug() << "setData";
    QTreeWidgetItem::setData(column, role, value);
    //if (write)
        //route->routes->writeRoutes();
}

void
RouteItem::setData(int column, int role, const QVariant &value)
{
    //qDebug() << "setData2";
    //qDebug() << "role" << role;
    if (role == Qt::EditRole) {
        qDebug() << "rename";
        route->setName(value.toString());
    }
    QTreeWidgetItem::setData(column, role, value);

    context->athlete->routes->writeRoutes();
}

RideFile*
RouteItem::ride()
{
    if (ride_ != NULL) return ride_;

    // open the ride file
    qDebug() << "path" << path;
    qDebug() << "fileName" << fileName;

    qDebug() << path << "/" << fileName;
    QFile file(path + "/" + fileName);

    ride_ = RideFileFactory::instance().openRideFile(context, file, errors_);
    if (ride_ == NULL) return NULL; // failed to read ride

    setDirty(false); // we're gonna use on-disk so by
                     // definition it is clean - but do it *after*
                     // we read the file since it will almost
                     // certainly be referenced by consuming widgets

    // stay aware of state changes to our ride
    // MainWindow saves and RideFileCommand modifies
    connect(ride_, SIGNAL(modified()), this, SLOT(modified()));
    connect(ride_, SIGNAL(saved()), this, SLOT(saved()));
    connect(ride_, SIGNAL(reverted()), this, SLOT(reverted()));

    return ride_;
}


void
RouteItem::modified()
{
    setDirty(true);
}

void
RouteItem::saved()
{
    setDirty(false);
}

void
RouteItem::reverted()
{
    setDirty(false);
}

void
RouteItem::setDirty(bool val)
{
    if (isdirty == val) return; // np change

    isdirty = val;

    if (isdirty == true) {

        // show ride in bold on the list view
        for (int i=0; i<3; i++) {
            QFont current = font(i);
            current.setWeight(QFont::Black);
            setFont(i, current);
        }

        context->notifyRideDirty();

    } else {

        // show ride in normal on the list view
        for (int i=0; i<3; i++) {
            QFont current = font(i);
            current.setWeight(QFont::Normal);
            setFont(i, current);
        }

        context->notifyRideClean();
    }
}

// name gets changed when file is converted in save
void
RouteItem::setFileName(QString path, QString fileName)
{
    this->path = path;
    this->fileName = fileName;
}




void
RouteItem::freeMemory()
{
    if (ride_) {
        delete ride_;
        ride_ = NULL;
    }
}


void
RouteItem::setStartTime(QDateTime newDateTime)
{
    dateTime = newDateTime;
    setText(0, dateTime.toString("ddd"));
    setText(1, dateTime.toString("MMM d, yyyy"));
    setText(2, dateTime.toString("h:mm AP"));

    ride()->setStartTime(newDateTime);
    //context->notifyRideSelected();
}
