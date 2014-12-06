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

#include "RideItem.h"
#include "RideMetric.h"
#include "RideFile.h"
#include "Context.h"
#include "DBAccess.h" // only for DBSchemaVersion
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include <math.h>

// used to create a temporary ride item that is not in the cache and just
// used to enable using the same calling semantics in things like the
// merge wizard and interval navigator
RideItem::RideItem(RideFile *ride, Context *context) 
    : 
    ride_(ride), context(context), isdirty(false), isstale(true), isedit(false), path(""), fileName(""),
    fingerprint(0), crc(0), timestamp(0), dbversion(0) {}

RideItem::RideItem(QString path, QString fileName, QDateTime &dateTime, Context *context) 
    :
    ride_(NULL), context(context), isdirty(false), isstale(true), isedit(false), path(path), 
    fileName(fileName), dateTime(dateTime), fingerprint(0), crc(0), timestamp(0), dbversion(0) {}

// Create a new RideItem destined for the ride cache and used for caching
// pre-computed metrics and storing ride metadata
RideItem::RideItem(RideFile *ride, QDateTime &dateTime, Context *context)
    :
    ride_(ride), context(context), isdirty(true), isstale(true), isedit(false), dateTime(dateTime),
    fingerprint(0), crc(0), timestamp(0), dbversion(0)
{

    const RideMetricFactory &factory = RideMetricFactory::instance();

    // ressize and initialize so we can store metric values at
    // RideMetric::index offsets into the metrics_ qvector
    metrics_.fill(0, factory.metricCount());
}

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
RideItem::setRide(RideFile *overwrite)
{
    RideFile *old = ride_;
    ride_ = overwrite; // overwrite

    // connect up to new one
    connect(ride_, SIGNAL(modified()), this, SLOT(modified()));
    connect(ride_, SIGNAL(saved()), this, SLOT(saved()));
    connect(ride_, SIGNAL(reverted()), this, SLOT(reverted()));

    // don't bother with the old one any more
    disconnect(old);

    // update status
    setDirty(true);
    notifyRideDataChanged();

    //XXX SORRY ! memory leak XXX
    //XXX delete old; // now wipe it once referrers had chance to change
    //XXX this is only used by MergeActivityWizard and causes issues
    //XXX because the data is accessed in separate threads (Wizard is a dialog)
    //XXX because it is such an edge case (Merge) we will leave it for now
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

        context->notifyRideDirty();

    } else {

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

bool
RideItem::isOpen()
{
    return ride_ != NULL;
}

void
RideItem::close()
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
    ride()->setStartTime(newDateTime);
}

// check if we need to be refreshed
bool
RideItem::checkStale()
{
    // if we're marked stale already then just return that !
    if (isstale) return true;

    // is db version different ?
    if (dbversion != DBSchemaVersion) {

        isstale = true;

    } else {

        // or have cp / zones have changed ?
        // note we now get the fingerprint from the zone range
        // and not the entire config so that if you add a new
        // range (e.g. set CP from today) but none of the other
        // ranges change then there is no need to recompute the
        // metrics for older rides !

        // get the new zone configuration fingerprint that applies for the ride date
        unsigned long rfingerprint = static_cast<unsigned long>(context->athlete->zones()->getFingerprint(context, dateTime.date()))
                      + static_cast<unsigned long>(context->athlete->paceZones()->getFingerprint(dateTime.date()))
                      + static_cast<unsigned long>(context->athlete->hrZones()->getFingerprint(dateTime.date()));

        if (fingerprint != rfingerprint) {

            isstale = true;

        } else {

            // or has file content changed ?
            QString fullPath =  QString(context->athlete->home->activities().absolutePath()) + "/" + fileName;
            QFile file(fullPath);

            // has timestamp changed ?
            if (timestamp < QFileInfo(file).lastModified().toTime_t()) {

                // if timestamp has changed then check crc
                unsigned long fcrc = DBAccess::computeFileCRC(fullPath);

                if (crc == 0 || crc != fcrc) {
                    crc = fcrc; // update as expensive to calculate
                    isstale = true;
                }
            }
        }
    }

    return isstale;
}
