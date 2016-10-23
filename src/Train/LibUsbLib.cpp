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

#include <QLibrary>
#include <usb.h>

// EZ-USB firmware loader for Fortius
extern "C" {
#include "EzUsb.h"
}

#include "LibUsbLib.h"

//-----------------------------------------------------------------------------
// Declarations
//
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
    UsbDeviceInterface* getInterface(struct usb_config_descriptor *configDescriptor);

    struct usb_device *dev;

#ifdef WIN32
    void libInit(QLibrary *lib);

    QLibrary *lib;
#endif
};


class LibUsbLib::Impl
{
public:
    Impl();

    void initialize(int logLevel);
    void findDevices();
    bool getDevices(QVector<UsbDevice *> &deviceList) const;

#ifdef WIN32
    bool isLibUsbInstalled() const;

    QLibrary &lib = QLibrary("libusb0");

    typedef void (*PrototypeVoid)();
    typedef void (*PrototypeVoid_Int)(int);
    typedef struct usb_bus* (*PrototypeBus)(void);

    PrototypeVoid usb_init;
    PrototypeVoid_Int usb_set_debug;
    PrototypeVoid usb_find_busses;
    PrototypeVoid usb_find_devices;
    PrototypeBus usb_get_busses;
#endif
};
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
#define USB_CONFIG_DESCRIPTOR struct usb_config_descriptor
#define USB_INTERFACE_DESCRIPTOR struct usb_interface_descriptor
#include "LibUsbLib_UsbDeviceImplGetInterface.cpp"

#ifdef WIN32
void UsbDevice::Impl::libInit(QLibrary *lib)
{
    this->lib = lib;
}
#endif
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// UsbDevice
//
UsbDevice::UsbDevice() : impl(new Impl)
{
}

UsbDevice::~UsbDevice()
{
    delete impl;
}

int UsbDevice::vendorId() const
{
    return impl->dev->descriptor.idVendor;
}

int UsbDevice::productId() const
{
    return impl->dev->descriptor.idProduct;
}

int UsbDevice::busNumber() const
{
    return impl->dev->bus->location;
}

int UsbDevice::deviceAddress() const
{
    return impl->dev->devnum;
}

UsbDeviceInterface* UsbDevice::getInterface()
{
    if (!impl->dev->descriptor.bNumConfigurations)
    {
        return NULL;
    }

    struct usb_config_descriptor *configDescriptor = &impl->dev->config[0];
    return impl->getInterface(configDescriptor);
}

// REMOVE ME!!!!!!!!!!!!
struct usb_device* UsbDevice::rawDev() const
{
    return impl->dev;
}
// REMOVE ME!!!!!!!!!!!!
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLib::Impl
//
LibUsbLib::Impl::Impl()
{
#ifdef WIN32
    if (!lib.load())
    {
        return;
    }

    usb_init = PrototypeVoid(lib.resolve("usb_init"));
    usb_set_debug = PrototypeVoid_Int(lib.resolve("usb_set_debug"));
    usb_find_busses = PrototypeVoid(lib.resolve("usb_find_busses"));
    usb_find_devices = PrototypeVoid(lib.resolve("usb_find_devices"));
    usb_get_busses = PrototypeBus(lib.resolve("usb_get_busses"));
#endif
}

void LibUsbLib::Impl::initialize(int logLevel)
{
#ifdef WIN32
    if (!isLibUsbInstalled())
    {
        return;
    }
#endif

    usb_init();
    usb_set_debug(logLevel);
    findDevices();
}

void LibUsbLib::Impl::findDevices()
{
#ifdef WIN32
    if (!isLibUsbInstalled())
    {
        return;
    }
#endif

    usb_find_busses();
    usb_find_devices();
}

bool LibUsbLib::Impl::getDevices(QVector<UsbDevice *> &deviceList) const
{
#ifdef WIN32
    if (!isLibUsbInstalled())
    {
        return false;
    }
#endif

    for (struct usb_bus *bus = usb_get_busses(); bus; bus = bus->next)
    {
        for (struct usb_device *dev = bus->devices; dev; dev = dev->next)
        {
            UsbDevice *device = new UsbDevice;
            device->impl->dev = dev;
            deviceList.append(device);

#ifdef WIN32
            device->impl->libInit(&lib);
#endif
        }
    }

    return true;
}

#ifdef WIN32
bool LibUsbLib::Impl::isLibUsbInstalled() const
{
    return lib.isLoaded();
}
#endif
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLib
//
LibUsbLib::LibUsbLib() : impl(new Impl)
{
}

LibUsbLib::~LibUsbLib()
{
    delete impl;
}

void LibUsbLib::initialize(int logLevel)
{
    impl->initialize(logLevel);
}

void LibUsbLib::findDevices()
{
    impl->findDevices();
}

bool LibUsbLib::getDevices(QVector<UsbDevice *> &deviceList)
{
    return impl->getDevices(deviceList);
}
//-----------------------------------------------------------------------------
