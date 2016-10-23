/*
 * Copyright (c) 2011 Darren Hague & Eric Brandt
 *               Modified to support Linux and OSX by Mark Liversedge
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

#if defined GC_HAVE_LIBUSB
#include <QString>
#include <QDebug>

#ifdef WIN32
#include <QLibrary> // for dynamically loading libusb0.dll
#endif

#ifndef Q_CC_MSVC
#include <unistd.h>
#endif

// EZ-USB firmware loader for Fortius
extern "C" {
#include "EzUsb.h"
}

#include "LibUsb.h"
#include "Settings.h"
#include "Context.h"

LibUsb::LibUsb(int type) : type(type), usbLib(new LibUsbLib)
{

    intf = NULL;
    readBufIndex = 0;
    readBufSize = 0;

#ifdef WIN32
    QLibrary lib("libusb0");
    if (!lib.isLoaded()) {
        if(!lib.load()) {
            libNotInstalled = true;
            return;
        }
    }
    libNotInstalled = false;

    // get the Functions for all used signatures

    usb_claim_interface = PrototypeInt_Handle_Int(lib.resolve("usb_claim_interface"));
    usb_set_altinterface = PrototypeInt_Handle_Int(lib.resolve("usb_set_altinterface"));
    usb_strerror = PrototypeChar_Void(lib.resolve("usb_strerror"));

#endif

    // Initialize the library.
    usbLib->initialize(0);
}

LibUsb::~LibUsb()
{
    delete usbLib;
}

int LibUsb::open()
{

#ifdef WIN32
    if (libNotInstalled) return -1;
#endif
    // reset counters
    intf = NULL;
    readBufIndex = 0;
    readBufSize = 0;

    // Find all busses and connected devices.
    usbLib->findDevices();

    switch (type) {

    // Search USB busses for USB2 ANT+ stick host controllers
    default:
    case TYPE_ANT: device = openAntStick();
              break;

    case TYPE_FORTIUS: device = openFortius();
              break;
    }

    if (device == NULL) return -1;

    // Clear halt is needed, but ignore return code
    device->clearHalt(intf->writeEndpoint());
    device->clearHalt(intf->readEndpoint());

    return 0;
}

bool LibUsb::find()
{
#ifdef WIN32
    if (libNotInstalled) return false;
#endif

    usbLib->findDevices();

    UsbDevice *device = getDevice();
    bool result = device;
    delete device;
    return result;
}

void LibUsb::close()
{
#ifdef WIN32
    if (libNotInstalled) return;
#endif
    if (!device)
    {
        return;
    }

    device->releaseInterface(intf->interfaceNumber());
    delete intf;
    intf = NULL;

    delete device;
    device = NULL;
}

int LibUsb::read(char *buf, int bytes)
{
#ifdef WIN32
    if (libNotInstalled) return -1;
#endif
	return this->read(buf, bytes, 125);
}

int LibUsb::read(char *buf, int bytes, int timeout)
{
#ifdef WIN32
    if (libNotInstalled) return -1;
#endif
    // check it isn't closed already
    if (!device) return -1;

    // The USB2 stick really doesn't like you reading 1 byte when more are available
    // so we use a local buffered read
    int bufRemain = readBufSize - readBufIndex;

    // Can we entirely satisfy the request from the buffer?
    if (bufRemain >= bytes)
    {
        // Yes, so do it
        memcpy(buf, readBuf+readBufIndex, bytes);
        readBufIndex += bytes;
        return bytes;
    }

    // No, so partially satisfy by emptying the buffer, then refill the buffer for the rest
    // !!! CHECK bufRemain > 0 rather than non-zero fixes annoying bug with ANT+
    //     devices not working again after restart "sometimes"
    if (bufRemain > 0) {
        memcpy(buf, readBuf+readBufIndex, bufRemain);
    }

    readBufSize = 0;
    readBufIndex = 0;

    int rc = device->bulkRead(intf->readEndpoint(), readBuf, 64, &readBufSize, timeout);
    if (rc < 0)
    {
        return rc;
    }

    int bytesToGo = bytes - bufRemain;
    if (bytesToGo < readBufSize)
    {
        // If we have enough bytes in the buffer, return them
        memcpy(buf+bufRemain, readBuf, bytesToGo);
        readBufIndex += bytesToGo;
        rc = bytes;
    } else {
        // Otherwise, just return what we can
        memcpy(buf+bufRemain, readBuf, readBufSize);
        rc = bufRemain + readBufSize;
        readBufSize = 0;
        readBufIndex = 0;
    }

    return rc;
}

int LibUsb::write(char *buf, int bytes)
{
#ifdef WIN32
    if (libNotInstalled) return -1;
#endif
	return this->write(buf, bytes, 125);
}

int LibUsb::write(char *buf, int bytes, int timeout)
{
#ifdef WIN32
    if (libNotInstalled) return -1;
#endif

    // check it isn't closed
    if (!device) return -1;

    int rc, bytesWritten;
#ifdef WIN32
    rc = device->interruptWrite(intf->writeEndpoint(), buf, bytes, &bytesWritten, 1000);
#else
    // we use a non-interrupted write on Linux/Mac since the interrupt
    // write block size is incorrectly implemented in the version of
    // libusb we build with. It is no less efficient.
    rc = device->bulkWrite(intf->writeEndpoint(), buf, bytes, &bytesWritten, timeout);
#endif

    if (rc < 0)
    {
        // Report timeouts - previously we ignored -110 errors. This masked a serious
        // problem with write on Linux/Mac, the USB stick needed a reset to avoid
        // these error messages, so we DO report them now
        qDebug()<<"usb_interrupt_write Error writing ["<<rc<<"]: "<< usb_strerror();
    }

    return rc < 0 ? rc : bytesWritten;
}

UsbDevice* LibUsb::getDevice() const
{
    QVector<UsbDevice *> deviceList;
    if (!usbLib->getDevices(deviceList))
    {
        return NULL;
    }

    // Look for matching device. Delete all unused devices.
    // Never leave any items in the list unprocessed, else we have leaks.
    UsbDevice *matchingDev = NULL;
    foreach (UsbDevice *dev, deviceList)
    {
        // We've already found a matching dev -> dispose and continue
        if (matchingDev)
        {
            delete dev;
            continue;
        }

        switch (type)
        {
        case TYPE_ANT:
            if (dev->vendorId() == GARMIN_USB2_VID &&
                    (dev->productId() == GARMIN_USB2_PID ||
                     dev->productId() == GARMIN_OEM_PID))
            {
                matchingDev = dev;
                continue;
            }

            break;

        case TYPE_FORTIUS:
            if (dev->vendorId() == FORTIUS_VID &&
                    (dev->productId() == FORTIUS_INIT_PID ||
                     dev->productId() == FORTIUS_PID ||
                     dev->productId() == FORTIUSVR_PID))
            {
                matchingDev = dev;
                continue;
            }

            break;
        }

        // Not a match -> dispose
        delete dev;
    }

    return matchingDev;
}

// Open connection to a Tacx Fortius
//
// The Fortius handlebar controller is an EZ-USB device. This is an
// embedded system using an 8051 microcontroller. Firmware must be
// downloaded to it once it is connected. This firmware is obviously
// copyrighted by Tacx and is not distributed with Golden Cheetah.
// Instead we ask the user to tell us where it can be found when they
// configure the device. (On Windows platforms the firmware is installed
// by the standard Tacx software as c:\windows\system32\FortiusSWPID1942Renum.hex).
//
// So when we open a Fortius device we need to search for an unitialised
// handlebar controller 3651:e6be and upload the firmware using the EzUsb
// functions. Once that has completed the controller will represent itself
// as 3651:1942.
// 
// Firmware will need to be reloaded if the device is disconnected or the
// USB controller is reset after sleep/resume.
//
// The EZUSB code is found in EzUsb.c, which is essentially the code used
// in the 'fxload' command on Linux, some minor changes have been made to the
// standard code wrt to logging errors.
//
// The only function we use is:
// int ezusb_load_ram (usb_dev_handle *device, const char *path, int fx2, int stage)
//      device is a usb device handle
//      path is the filename of the firmware file
//      fx2 is non-zero to indicate an fx2 device (we pass 0, since the Fortius is fx)
//      stage is to control two stage loading, we load in a single stage
UsbDeviceHandle* LibUsb::openFortius()
{

#ifdef WIN32
    if (libNotInstalled) return NULL;
#endif

    UsbDevice *dev;
    UsbDeviceHandle *udev = NULL;

    //
    // Search for an UN-INITIALISED Fortius device
    //
    dev = getDevice();
    if (dev)
    {
        if (dev->productId() == FORTIUS_INIT_PID)
        {
            if ((udev = dev->open()))
            {
                // LOAD THE FIRMWARE
                ezusb_load_ram (udev->rawHandle(), appsettings->value(NULL, FORTIUS_FIRMWARE, "").toString().toLatin1(), 0, 0);
            }

            // Now close the connection, our work here is done
            delete udev;
            udev = NULL;

            // We need to rescan devices, since once the Fortius has
            // been programmed it will present itself again with a
            // different PID. But it takes its time, so we sleep for
            // 3 seconds. This may be too short on some operating
            // systems. We can fix if issues are reported.  On my Linux
            // host running a v3 kernel on an AthlonXP 2 seconds is not
            // long enough.
            //
            // Given this is only required /the first time/ the Fortius
            // is connected, it can't be that bad?

#ifdef WIN32
            Sleep(3000); // windows sleep is in milliseconds
#else
            sleep(3);  // do not be tempted to reduce this, it really does take that long!
#endif
            usbLib->findDevices();
        }

        delete dev;
    }

    //
    // Now search for an INITIALISED Fortius device
    //
    dev = getDevice();
    if (dev)
    {
        if (dev->productId() == FORTIUS_PID || dev->productId() == FORTIUSVR_PID)
        {
            //Avoid noisy output
            //qDebug() << "Found a Garmin USB2 ANT+ stick";

            udev = openUsb(dev, false);
        }

        delete dev;
    }

    return udev;
}

UsbDeviceHandle* LibUsb::openAntStick()
{
#ifdef WIN32
    if (libNotInstalled) return NULL;
#endif

    UsbDevice *dev;
    UsbDeviceHandle *udev = NULL;

// for Mac and Linux we do a bus reset on it first...
#ifndef WIN32
    dev = getDevice();
    if (dev)
    {
        if ((udev = dev->open()))
        {
            udev->reset();
            delete udev;
            udev = NULL;
        }

        delete dev;
    }
#endif

    dev = getDevice();
    if (dev)
    {
        //Avoid noisy output
        //qDebug() << "Found a Garmin USB2 ANT+ stick";

        udev = openUsb(dev, true);
        delete dev;
    }

    return udev;
}

UsbDeviceHandle* LibUsb::openUsb(UsbDevice *dev, bool detachKernelDriver)
{
    UsbDeviceHandle *udev;
    if ((udev = dev->open()))
    {
        if ((intf = dev->getInterface()))
        {
#ifdef Q_OS_LINUX
            if (detachKernelDriver)
            {
                udev->detachKernelDriver(intf->interfaceNumber());
            }
#endif

            int rc = udev->setConfiguration(1);
            if (rc < 0)
            {
                qDebug()<<"usb_set_configuration Error: "<< usb_strerror();
                if (OperatingSystem == LINUX)
                {
                    // looks like the udev rule has not been implemented
                    QString path = QString("/dev/bus/usb/%1/%2")
                            .arg(dev->busNumber(), 3, 10, QChar('0'))
                            .arg(dev->deviceAddress(), 3, 10, QChar('0'));
                    qDebug() << "check permissions on:" << path;
                    qDebug() << "did you remember to setup a udev rule for this device?";
                }
            }

            rc = usb_claim_interface(udev->rawHandle(), intf->interfaceNumber());
            if (rc < 0) qDebug()<<"usb_claim_interface Error: "<< usb_strerror();

            if (OperatingSystem != OSX)
            {
                // fails on Mac OS X, we don't actually need it anyway
                rc = usb_set_altinterface(udev->rawHandle(), intf->alternateSetting());
                if (rc < 0) qDebug()<<"usb_set_altinterface Error: "<< usb_strerror();
            }

            return udev;
        }

        delete udev;
    }

    return NULL;
}
#else

// if we don't have libusb use stubs
LibUsb::LibUsb() {}

int LibUsb::open()
{
    return -1;
}

void LibUsb::close()
{
}

int LibUsb::read(char *, int)
{
    return -1;
}

int LibUsb::write(char *, int)
{
    return -1;
}

#endif // Have LIBUSB and WIN32 or LINUX
