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

#include "D2XX.h"

bool D2XXRegistered = Device::addListFunction(&D2XX::myListDevices);

D2XX::D2XX(const FT_DEVICE_LIST_INFO_NODE &info) :
    info(info), isOpen(false)
{
}

D2XX::~D2XX()
{
    if (isOpen)
        close();
}

bool
D2XX::open(QString &err)
{
    assert(!isOpen);
    FT_STATUS ftStatus =
        FT_OpenEx(info.Description, FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus != FT_OK) {
        err = QString("FT_Open: %1").arg(ftStatus);
        return false;
    }
    isOpen = true;
    ftStatus = FT_SetBaudRate(ftHandle, 9600);
    if (ftStatus != FT_OK) {
        err = QString("FT_SetBaudRate: %1").arg(ftStatus);
        close();
    }
    return true;
}

void
D2XX::close()
{
    assert(isOpen);
    FT_Close(ftHandle); 
    isOpen = false;
}

int
D2XX::read(void *buf, size_t nbyte, QString &err)
{
    assert(isOpen);
    DWORD rxbytes;
    FT_STATUS ftStatus = FT_GetQueueStatus(ftHandle, &rxbytes);
    if (ftStatus != FT_OK) {
        err = QString("FT_GetQueueStatus: %1").arg(ftStatus);
        return -1;
    }
    // printf("rxbytes=%d\n", (int) rxbytes);
    // Return immediately whenever there's something to read.
    if (rxbytes > 0 && rxbytes < nbyte)
        nbyte = rxbytes;
    if (nbyte > rxbytes)
        FT_SetTimeouts(ftHandle, 5000, 5000);
    DWORD n;
    ftStatus = FT_Read(ftHandle, buf, nbyte, &n);
    if (ftStatus == FT_OK)
        return n;
    err = QString("FT_Read: %1").arg(ftStatus);
    return -1;
}

int
D2XX::write(void *buf, size_t nbyte, QString &err)
{
    assert(isOpen);
    DWORD n;
    FT_STATUS ftStatus = FT_Write(ftHandle, buf, nbyte, &n);
    if (ftStatus == FT_OK)
        return n;
    err = QString("FT_Write: %1").arg(ftStatus);
    return -1;
}

QString
D2XX::name() const
{
    return QString("D2XX: ") + info.Description;
}

QVector<DevicePtr>
D2XX::myListDevices(QString &err)
{
    QVector<DevicePtr> result;
    DWORD numDevs;
    FT_STATUS ftStatus = FT_CreateDeviceInfoList(&numDevs); 
    if(ftStatus != FT_OK) {
        err = QString("FT_CreateDeviceInfoList: %1").arg(ftStatus); 
        return result;
    }
    FT_DEVICE_LIST_INFO_NODE *devInfo = new FT_DEVICE_LIST_INFO_NODE[numDevs];
    ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs); 
    if (ftStatus != FT_OK)
        err = QString("FT_GetDeviceInfoList: %1").arg(ftStatus); 
    else {
        for (DWORD i = 0; i < numDevs; i++)
            result.append(DevicePtr(new D2XX(devInfo[i])));
    }
    delete [] devInfo;
    // If we can't open a D2XX device, it's usually because the VCP drivers
    // are installed, so it should also show up in the list of serial devices.
    for (int i = 0; i < result.size(); ++i) {
        DevicePtr dev = result[i];
        QString tmp;
        if (dev->open(tmp))
            dev->close();
        else
            result.remove(i--);
    }
    return result;
}

