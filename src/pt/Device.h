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

#ifndef _GC_PT_Device_h
#define _GC_PT_Device_h 1

#include <QtCore>
#include <boost/shared_ptr.hpp>

class Device;
typedef boost::shared_ptr<Device> DevicePtr;

class Device
{
    typedef QVector<DevicePtr> (*ListFunction)(QString &err);
    static QVector<ListFunction> listFunctions;

    public:

    static bool addListFunction(ListFunction f);
    static QVector<DevicePtr> listDevices(QString &err);

    virtual ~Device() {}
    virtual bool open(QString &err) = 0;
    virtual void close() = 0;
    virtual int read(void *buf, size_t nbyte, QString &err) = 0;
    virtual int write(void *buf, size_t nbyte, QString &err) = 0;
    virtual QString name() const = 0;

};

#endif // _GC_PT_Device_h

