/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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


// Lists all the device types supported

#ifndef _GC_DeviceTypes_h
#define _GC_DeviceTypes_h 1
#include "GoldenCheetah.h"

#include <QList>

#define DEV_PT         0x0001
#define DEV_SRM        0x0002
#define DEV_CT         0x0010
#define DEV_ANTPLUS    0x0020   // Quarqd ANT+ device
#define DEV_NULL       0x0040
#define DEV_ANTLOCAL   0x0080   // Local ANT+ device
#define DEV_GSERVER    0x0100   // NOT IMPLEMENTED IN THIS RELEASE XXX
#define DEV_GCLIENT    0x0200   // NOT IMPLEMENTED IN THIS RELEASE XXX
#define DEV_FORTIUS    0x0800   // Tacx Fortius
#define DEV_IMAGIC     0x1000   // Tacx Imagic
#define DEV_BT40       0x2000   // QT Bluetooth support
#define DEV_MONARK     0x4000   // Monark USB
#define DEV_KETTLER    0x8000   // Kettler Serial
#define DEV_KETTLER_RACER    0x8100   // Kettler racer Serial
#define DEV_ERGOFIT    0x9000   // Ergofit Serial
#define DEV_DAUM       0x10000   // Daum Serial

#define DEV_QUARQ      0x01     // ants use id:hostname:port
#define DEV_SERIAL     0x02     // use filename COMx or /dev/cuxxxx
#define DEV_TCP        0x03     // tcp port is hostname:port NOT IMPLEMENTED IN THIS RELEASE
#define DEV_USB        0x04     // use filename COMx or /dev/cuxxxx
#define DEV_LIBUSB     0x08     // will interact directly (i.e. no device file needed)
#define DEV_BTLE       0x10     // bluetooth

class DeviceType
{
    public:
    int type;           // type specifier - not sure if neccessary
    int connector;      // is it a serial or tcp device?
    char *name;         // narrative name
    bool realtime;      // can it do realtime
    bool download;      // can it do download?
    QString description; // tell me about it
    QString image;      // filename for image
};

class DeviceTypes
{
    Q_DECLARE_TR_FUNCTIONS(DeviceTypes)

    public:
        DeviceTypes();
        ~DeviceTypes();

    QList<DeviceType> Supported;       // all the supported types in a list
    QList<DeviceType> getList();          // returns a list of the supported device types

    DeviceType getType(int);            // return all details for type x
};

#endif // _GC_DeviceTypes_h

