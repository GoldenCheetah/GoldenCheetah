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
#include <ctype.h>

Device::Device(const FT_DEVICE_LIST_INFO_NODE &info) :
    info(info), isOpen(false)
{
}

Device::~Device()
{
    if (isOpen)
        close();
}

bool Device::open(QString &err)
{
    assert(!isOpen);
    FT_STATUS ftStatus =
        FT_OpenEx(info.Description, FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus == FT_OK)
        isOpen = true;
    else
        err = QString("FT_Open: %1").arg(ftStatus);
    ftStatus = FT_SetBaudRate(ftHandle, 9600);
    if (ftStatus != FT_OK) {
        err = QString("FT_SetBaudRate: %1").arg(ftStatus);
        close();
    }
    return isOpen;
}

void Device::close()
{
    assert(isOpen);
    FT_Close(ftHandle); 
    isOpen = false;
}

int Device::read(void *buf, size_t nbyte, QString &err)
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

int Device::write(void *buf, size_t nbyte, QString &err)
{
    assert(isOpen);
    DWORD n;
    FT_STATUS ftStatus = FT_Write(ftHandle, buf, nbyte, &n);
    if (ftStatus == FT_OK)
        return n;
    err = QString("FT_Write: %1").arg(ftStatus);
    return -1;
}

QVector<DevicePtr>
Device::listDevices(QString &err)
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
            result.append(DevicePtr(new Device(devInfo[i])));
    }
    delete [] devInfo;
    return result;
}


