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

#include <QTreeWidgetItem>
#include "RideItem.h"
#include "RideMetric.h"
#include "RideFile.h"
#include "Context.h"
#include "Context.h"
#include "Zones.h"
#include "HrZones.h"
#include <math.h>

RideItem::RideItem(int type,
                   QString path, QString fileName, const QDateTime &dateTime,
                   const Zones *zones, const HrZones *hrZones, Context *context) :
    QTreeWidgetItem(type), ride_(NULL), context(context), isdirty(false), isedit(false), path(path), fileName(fileName),
    dateTime(dateTime), zones(zones), hrZones(hrZones)
{ }

RideFile *RideItem::ride(bool open)
{
    if (!open || ride_) return ride_;

    // open the ride file
    QFile file(path + "/" + fileName);
    ride_ = RideFileFactory::instance().openRideFile(context, file, errors_);
    if (ride_ == NULL) return NULL; // failed to read ride

    setDirty(false); // we're gonna use on-disk so by
                     // definition it is clean - but do it *after*
                     // we read the file since it will almost
                     // certainly be referenced by consuming widgets

    // stay aware of state changes to our ride
    // Context saves and RideFileCommand modifies
    connect(ride_, SIGNAL(modified()), this, SLOT(modified()));
    connect(ride_, SIGNAL(saved()), this, SLOT(saved()));
    connect(ride_, SIGNAL(reverted()), this, SLOT(reverted()));

    return ride_;
}

void
RideItem::notifyRideDataChanged()
{
    emit rideDataChanged();
}

void
RideItem::notifyRideMetadataChanged()
{
    emit rideMetadataChanged();
}

void
RideItem::modified()
{
    setDirty(true);
}

void
RideItem::saved()
{
    setDirty(false);
}

void
RideItem::reverted()
{
    setDirty(false);
}

void
RideItem::setDirty(bool val)
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
RideItem::setFileName(QString path, QString fileName)
{
    this->path = path;
    this->fileName = fileName;
}

int RideItem::zoneRange()
{
    return zones->whichRange(dateTime.date());
}

int RideItem::hrZoneRange()
{
    return hrZones->whichRange(dateTime.date());
}
int RideItem::numZones()
{
    int zone_range = zoneRange();
    return (zone_range >= 0) ? zones->numZones(zone_range) : 0;
}

int RideItem::numHrZones()
{
    int hr_zone_range = hrZoneRange();
    return (hr_zone_range >= 0) ? hrZones->numZones(hr_zone_range) : 0;
}

void
RideItem::freeMemory()
{
    if (ride_) {
        delete ride_;
        ride_ = NULL;
    }
}

void
RideItem::setStartTime(QDateTime newDateTime)
{
    dateTime = newDateTime;
    setText(0, dateTime.toString(tr("ddd")));
    setText(1, dateTime.toString(tr("MMM d, yyyy")));
    setText(2, dateTime.toString("hh:mm"));

    ride()->setStartTime(newDateTime);
    context->notifyRideSelected(this);
}
