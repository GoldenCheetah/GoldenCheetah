/*
 * Copyright (c) 2011 Darren Hague & Eric Brandt
 *               Modified to suport Linux and OSX by Mark Liversedge
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

#include "LibUsb.h"

LibUsb::LibUsb()
{

// dynamic load of libusb on Windows, it is statically linked in Linux
// this is to avoid dll conflicts where the lib has already been installed
#ifdef WIN32
    QLibrary libusb0("libusb0");

    usb_set_debug = (VoidIntProto) libusb0.resolve("usb_set_debug");
    usb_strerror = (CharVoidProto) libusb0.resolve("usb_strerror");
    usb_init = (IntVoidProto) libusb0.resolve("usb_init");
    usb_find_busses = (IntVoidProto) libusb0.resolve("usb_find_busses");
    usb_find_devices = (IntVoidProto) libusb0.resolve("usb_find_devices");
    usb_clear_halt = (IntUsb_dev_handleUintProto) libusb0.resolve("usb_clear_halt");
    usb_release_interface = (IntUsb_dev_handleIntProto) libusb0.resolve("usb_release_interface");
    usb_close = (IntUsb_dev_handleProto) libusb0.resolve("usb_close");
    usb_bulk_read = (IntUsb_dev_handleIntCharIntIntProto) libusb0.resolve("usb_bulk_read");
    usb_interrupt_write = (IntUsb_dev_handleIntCharIntIntProto) libusb0.resolve("usb_interrupt_write");
    usb_get_busses = (Usb_busVoidProto) libusb0.resolve("usb_get_busses");
    usb_open = (Usb_dev_handleUsb_deviceProto) libusb0.resolve("usb_open");
    usb_set_configuration = (IntUsb_dev_handleIntProto) libusb0.resolve("usb_set_configuration");
    usb_claim_interface = (IntUsb_dev_handleIntProto) libusb0.resolve("usb_claim_interface");
    usb_set_altinterface = (IntUsb_dev_handleIntProto) libusb0.resolve("usb_set_altinterface");

    if (!(isDllLoaded = libusb0.isLoaded()))
    {
        qWarning("libusb0.dll was not loaded");
        return;
    }
#endif

    intf = NULL;
    readBufIndex = 0;
    readBufSize = 0;

    usb_set_debug(0);

#ifdef WIN32 // we initialise whenever we open on Linux/Mac
    // Initialize the library.
    usb_init();

    // Find all busses.
    usb_find_busses();

    // Find all connected devices.
    usb_find_devices();
#endif
}

int LibUsb::open()
{

#ifdef WIN32
    if (!isDllLoaded) return -1;
#else

   // Initialize the library.
    usb_init();

    // Find all busses.
    usb_find_busses();

    // Find all connected devices.
    usb_find_devices();
#endif

    readBufSize = 0;
    readBufIndex = 0;

    // Search USB busses for USB2 ANT+ stick host controllers
    device = OpenAntStick();

    if (device == NULL) return -1;

#ifndef WIN32
    // reset the device, god only knows what mess we left it in...
    int rc = usb_reset(device);
    if (rc < 0) qDebug()<<"usb_reset Error: "<< usb_strerror();
#endif

#ifndef Q_OS_MAC
    // these functions fail on OS X Lion
    rc = usb_clear_halt(device, writeEndpoint);
    if (rc < 0) qDebug()<<"usb_clear_halt writeEndpoint Error: "<< usb_strerror();

    rc = usb_clear_halt(device, readEndpoint);
    if (rc < 0) qDebug()<<"usb_clear_halt readEndpoint Error: "<< usb_strerror();
#endif

    return rc;
}

void LibUsb::close()
{

    if (device) {
        // stop any further write attempts whilst we close down
        usb_dev_handle *p = device;
        device = NULL;

        usb_release_interface(p, interface);
        usb_close(p);
    }
}

int LibUsb::read(char *buf, int bytes)
{
    // check it isn't closed already
    if (!device) return -1;

#ifdef WIN32
    if (!isDllLoaded) return -1;
#endif

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
    if (bufRemain) {
        memcpy(buf, readBuf+readBufIndex, bufRemain);
    }

    readBufSize = 0;
    readBufIndex = 0;

    int rc = usb_bulk_read(device, readEndpoint, readBuf, 64, 125);
    if (rc < 0)
    {
        // don't report timeouts - lots of noise so commented out
        //qDebug()<<"usb_bulk_read Error reading: "<<rc<< usb_strerror();
        return rc;
    }
    readBufSize = rc;

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

    // check it isn't closed
    if (!device) return -1;

#ifdef WIN32
    if (!isDllLoaded) return -1;
#endif

#ifdef WIN32
    int rc = usb_interrupt_write(device, writeEndpoint, buf, bytes, 1000);
#else
    // we use a non-interrupted write on Linux/Mac since the interrupt
    // write block size is incorectly implemented in the version of
    // libusb we build with. It is no less efficent.
    int rc = usb_bulk_write(device, writeEndpoint, buf, bytes, 125);
#endif

    if (rc < 0)
    {
        // don't report timeouts - lots of noise
        if (rc != -110) qDebug()<<"usb_interrupt_write Error writing: "<< usb_strerror();
    }

    return rc;
}

struct usb_dev_handle* LibUsb::OpenAntStick()
{

    struct usb_bus* bus;
    struct usb_device* dev;
    struct usb_dev_handle* udev;

    for (bus = usb_get_busses(); bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor == GARMIN_USB2_VID && dev->descriptor.idProduct == GARMIN_USB2_PID) {

                //Avoid noisy output
                //qDebug() << "Found a Garmin USB2 ANT+ stick";

                if ((udev = usb_open(dev))) {

                    if (dev->descriptor.bNumConfigurations) {

                        if ((intf = usb_find_interface(&dev->config[0])) != NULL) {

                            int rc = usb_set_configuration(udev, 1);
                            if (rc < 0) {
                                qDebug()<<"usb_set_configuration Error: "<< usb_strerror();
#ifdef __linux__
                                // looks like the udev rule has not been implemented
                                qDebug()<<"check permissions on:"<<QString("/dev/bus/usb/%1/%2").arg(bus->dirname).arg(dev->filename);
                                qDebug()<<"did you remember to setup a udev rule for this device?";
#endif
                            }

                            rc = usb_claim_interface(udev, interface);
                            if (rc < 0) qDebug()<<"usb_claim_interface Error: "<< usb_strerror();

#ifndef Q_OS_MAC
                            // fails on Mac OS X, we don't actually need it anyway
                            rc = usb_set_altinterface(udev, alternate);
                            if (rc < 0) qDebug()<<"usb_set_altinterface Error: "<< usb_strerror();
#endif

                            return udev;
                        }
                    }

                    usb_close(udev);
                }
            }
        }
    }
    return NULL;
}

struct usb_interface_descriptor* LibUsb::usb_find_interface(struct usb_config_descriptor* config_descriptor)
{

    struct usb_interface_descriptor* intf;

    readEndpoint = -1;
    writeEndpoint = -1;
    interface = -1;
    alternate = -1;

    if (!config_descriptor) return NULL;

    if (!config_descriptor->bNumInterfaces) return NULL;

    if (!config_descriptor->interface[0].num_altsetting) return NULL;

    intf = &config_descriptor->interface[0].altsetting[0];

    if (intf->bNumEndpoints != 2) return NULL;

    interface = intf->bInterfaceNumber;
    alternate = intf->bAlternateSetting;

    for (int i = 0 ; i < 2; i++)
    {
        if (intf->endpoint[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK)
            readEndpoint = intf->endpoint[i].bEndpointAddress;
        else
            writeEndpoint = intf->endpoint[i].bEndpointAddress;
    }

    if (readEndpoint < 0 || writeEndpoint < 0)
        return NULL;

    return intf;
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
