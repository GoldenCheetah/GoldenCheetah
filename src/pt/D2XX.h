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

#include <QtCore>
#include <D2XX/ftd2xx.h>
#include <boost/shared_ptr.hpp>

class Device;
typedef boost::shared_ptr<Device> DevicePtr;

class Device
{
    Device(const Device &);
    Device& operator=(const Device &);

    FT_DEVICE_LIST_INFO_NODE info;
    FT_HANDLE ftHandle;
    bool isOpen;
    Device(const FT_DEVICE_LIST_INFO_NODE &info);

    public:

    static QVector<DevicePtr> listDevices(QString &err);
    ~Device();
    bool open(QString &err);
    void close();
    int read(void *buf, size_t nbyte, QString &err);
    int write(void *buf, size_t nbyte, QString &err);

};

