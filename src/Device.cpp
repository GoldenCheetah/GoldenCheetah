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

#include "Device.h"

#define tr(s) QObject::tr(s)

typedef QMap<QString,DevicesPtr> DevicesMap;

static DevicesMap *devicesPtr;

inline DevicesMap &
devices()
{
    if (devicesPtr == NULL)
        devicesPtr = new QMap<QString,DevicesPtr>;
    return *devicesPtr;
}

Device::~Device()
{
    if( dev->isOpen() )
        dev->close();
}

QList<QString>
Devices::typeNames()
{
    return devices().keys();
}

DevicesPtr
Devices::getType(const QString &deviceTypeName )
{
    assert(devices().contains(deviceTypeName));
    return devices().value(deviceTypeName);
}

bool
Devices::addType(const QString &deviceTypeName, DevicesPtr p )
{
    assert(!devices().contains(deviceTypeName));
    devices().insert(deviceTypeName, p);
    return true;
}

