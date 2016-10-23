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
class UsbDeviceHandle::Impl
{
public:
    libusb_device_handle *handle;
};


class UsbDeviceInterface::Impl
{
public:
    int interfaceNumber;
    int alternateSetting;
    int readEndpoint;
    int writeEndpoint;
};


class UsbDevice::Impl
{
public:
    ~Impl();
    UsbDeviceInterface* getInterface(libusb_config_descriptor *configDescriptor);

    LibUsbLibUtils *utils;
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
// UsbDeviceHandle
//
UsbDeviceHandle::UsbDeviceHandle() : impl(new Impl)
{
}

UsbDeviceHandle::~UsbDeviceHandle()
{
    delete impl;
}

// REMOVE ME!!!!!!!!!!!!
usb_dev_handle* UsbDeviceHandle::rawHandle() const
{
    return NULL;
}
// REMOVE ME!!!!!!!!!!!!
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// UsbDeviceInterface
//
UsbDeviceInterface::UsbDeviceInterface() : impl(new Impl)
{
}

UsbDeviceInterface::~UsbDeviceInterface()
{
    delete impl;
}

int UsbDeviceInterface::interfaceNumber() const
{
    return impl->interfaceNumber;
}

int UsbDeviceInterface::alternateSetting() const
{
    return impl->alternateSetting;
}

int UsbDeviceInterface::readEndpoint() const
{
    return impl->readEndpoint;
}

int UsbDeviceInterface::writeEndpoint() const
{
    return impl->writeEndpoint;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// UsbDevice::Impl
//
UsbDevice::Impl::~Impl()
{
    libusb_unref_device(dev);
}

#define USB_CONFIG_DESCRIPTOR libusb_config_descriptor
#define USB_INTERFACE_DESCRIPTOR libusb_interface_descriptor

// UNCOMMENT ME!!!!!!!!!!!
//#define USB_ENDPOINT_DIR_MASK LIBUSB_ENDPOINT_DIR_MASK
// UNCOMMENT ME!!!!!!!!!!!

#include "LibUsbLib_UsbDeviceImplGetInterface.cpp"
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

int UsbDevice::busNumber() const
{
    return libusb_get_bus_number(impl->dev);
}

int UsbDevice::deviceAddress() const
{
    return libusb_get_device_address(impl->dev);
}

UsbDeviceInterface* UsbDevice::getInterface()
{
    libusb_config_descriptor *configDescriptor;
    int rc = libusb_get_config_descriptor(impl->dev, 0, &configDescriptor);
    if (rc < 0)
    {
        libusb_free_config_descriptor(configDescriptor);
        return NULL;
    }

    UsbDeviceInterface *interface = impl->getInterface(configDescriptor);
    libusb_free_config_descriptor(configDescriptor);
    return interface;
}

UsbDeviceHandle* UsbDevice::open()
{
    libusb_device_handle *handle;
    int rc = libusb_open(impl->dev, &handle);
    if (rc < 0)
    {
        impl->utils->logError("libusb_open", rc);
        libusb_close(handle);
        return NULL;
    }

    UsbDeviceHandle *usbHandle = new UsbDeviceHandle;
    usbHandle->impl->handle = handle;
    return usbHandle;
}
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
        device->impl->utils = impl->utils;
        device->impl->dev = rawDev;
        device->impl->vendorId = deviceDescriptor.idVendor;
        device->impl->productId = deviceDescriptor.idProduct;
        deviceList.append(device);
    }

    libusb_free_device_list(list, 0);
    return true;
}
//-----------------------------------------------------------------------------
