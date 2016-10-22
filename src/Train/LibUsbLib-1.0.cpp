/*
 * Copyright (c) 2016 Antonius Riha (antoniusriha@gmail.com)
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

#include <QDebug>
#include <libusb-1.0/libusb.h>

// EZ-USB firmware loader for Fortius
#include "EzUsb-1.0.h"

#include "LibUsbLib.h"

class LibUsbLibUtils
{
public:
    void logError(const char *fctName, int errorCode) const;
};

//-----------------------------------------------------------------------------
// Declarations
//
class UsbDevice::Impl
{
public:
    ~Impl();

    libusb_device *dev = NULL;
    int vendorId = 0;
    int productId = 0;
};


class LibUsbLib::Impl
{
public:
    ~Impl();
    bool getDeviceDescriptor(libusb_device *device, libusb_device_descriptor &deviceDescriptor) const;

    LibUsbLibUtils *utils = new LibUsbLibUtils;
    libusb_context *ctx = NULL;
};
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLibUtils
//
void LibUsbLibUtils::logError(const char *fctName, int errorCode) const
{
    qDebug().nospace() << "Error in function '" << fctName << "'. ErrorCode=" << errorCode
             << ". ErrorMessage: '" << libusb_error_name(errorCode) << "'";
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// UsbDevice::Impl
//
UsbDevice::Impl::~Impl()
{
    libusb_unref_device(dev);
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// UsbDevice
//
UsbDevice::UsbDevice() : impl(new Impl())
{
}

UsbDevice::~UsbDevice()
{
    delete impl;
}

int UsbDevice::vendorId() const
{
    return impl->vendorId;
}

int UsbDevice::productId() const
{
    return impl->productId;
}

// REMOVE ME!!!!!!!!!!!!
struct usb_device* UsbDevice::rawDev() const
{
    return NULL;
}
// REMOVE ME!!!!!!!!!!!!
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLib::Impl
//
LibUsbLib::Impl::~Impl()
{
    delete utils;
}

bool LibUsbLib::Impl::getDeviceDescriptor(libusb_device *device, libusb_device_descriptor &deviceDescriptor) const
{
    int rc = libusb_get_device_descriptor(device, &deviceDescriptor);
    if (rc < 0)
    {
        utils->logError("libusb_get_device_descriptor", rc);
        return false;
    }

    return true;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLib
//
LibUsbLib::LibUsbLib() : impl(new Impl)
{
}

LibUsbLib::~LibUsbLib()
{
    libusb_exit(impl->ctx);
    delete impl;
}

void LibUsbLib::initialize(int logLevel)
{
    int rc = libusb_init(&impl->ctx);
    if (rc < 0)
    {
        impl->utils->logError("libusb_init", rc);
        return;
    }

    libusb_set_debug(impl->ctx, logLevel);
}

void LibUsbLib::findDevices()
{
}

bool LibUsbLib::getDevices(QVector<UsbDevice *> &deviceList)
{
    libusb_device **list;
    ssize_t listCount = libusb_get_device_list(impl->ctx, &list);
    if (listCount < 0)
    {
        impl->utils->logError("libusb_get_device_list", listCount);
        return false;
    }

    for (int i = 0; i < listCount; i++)
    {
        libusb_device *rawDev = list[i];
        libusb_device_descriptor deviceDescriptor;
        if (!impl->getDeviceDescriptor(rawDev, deviceDescriptor))
        {
            // Useless device, unref it
            libusb_unref_device(rawDev);
            continue;
        }

        UsbDevice *device = new UsbDevice;
        device->impl->dev = rawDev;
        device->impl->vendorId = deviceDescriptor.idVendor;
        device->impl->productId = deviceDescriptor.idProduct;
        deviceList.append(device);
    }

    libusb_free_device_list(list, 0);
    return true;
}
//-----------------------------------------------------------------------------
