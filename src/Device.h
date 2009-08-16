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

#include "CommPort.h"
#include <boost/function.hpp>

struct Device
{
    virtual ~Device() {}

    typedef boost::function<bool (const QString &statusText)> StatusCallback;

    virtual QString downloadInstructions() const = 0;
    virtual bool download(CommPortPtr dev, const QDir &tmpdir,
                          QString &tmpname, QString &filename,
                          StatusCallback statusCallback, QString &err) = 0;
    virtual void cleanup(CommPortPtr dev) { (void) dev; }

    static QList<QString> deviceTypes();
    static Device &device(const QString &deviceType);
    static bool addDevice(const QString &deviceType, Device *device);
};
 
#endif // _GC_Device_h

