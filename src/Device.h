/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_Device_h
#define _GC_Device_h 1
#include "GoldenCheetah.h"

#include "CommPort.h"
#include <boost/function.hpp>

struct DeviceDownloadFile
{
    QString     name;
    QDateTime   startTime;
    QString     extension;
};

struct DeviceRideItem
{
    bool        wanted;
    QDateTime   startTime;
    QDateTime   endTime; // optional, check with isValid()
    unsigned    work;   // for progress indication
};
typedef boost::shared_ptr<DeviceRideItem> DeviceRideItemPtr;

struct Device;
typedef boost::shared_ptr<Device> DevicePtr;

struct Device
{
    Device( CommPortPtr dev ) : dev( dev ) {};
    virtual ~Device();

    typedef boost::function<bool (void)> CancelCallback;
    typedef boost::function<void (const QString &statusText)> StatusCallback;
    typedef boost::function<void (const QString &progressText)> ProgressCallback;

    virtual bool preview( StatusCallback statusCallback, QString &err );
    virtual QList<DeviceRideItemPtr> &rides();

    virtual bool download( const QDir &tmpdir,
                          QList<DeviceDownloadFile> &files,
                          CancelCallback cancelCallback,
                          StatusCallback statusCallback,
                          ProgressCallback progressCallback,
                          QString &err) = 0;

    virtual bool cleanup( QString &err );

protected:
    QList<DeviceRideItemPtr> rideList;
    CommPortPtr dev;

};

struct Devices;
typedef boost::shared_ptr<Devices> DevicesPtr;

struct Devices
{
    virtual DevicePtr newDevice( CommPortPtr ) = 0;

    // port *might* be supported by this device type implementation:
    virtual bool supportsPort( CommPortPtr dev ) { (void)dev; return true; };

    // port is only useful for this device type - no matter if it's
    // actually supported. Prevents this port to be offered for download
    // with other device types:
    virtual bool exclusivePort( CommPortPtr dev ) { (void)dev; return false; };

    // cleanup for this device type is implemented:
    virtual bool canCleanup() { return false; };

    virtual QString downloadInstructions() const { return ""; };

    static QList<QString> typeNames();
    static DevicesPtr getType(const QString &deviceTypeName );
    static bool addType(const QString &deviceTypeName, DevicesPtr p );
};


#endif // _GC_Device_h

