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

#ifndef gc_LibUsb_h
#define gc_LibUsb_h

#if defined GC_HAVE_LIBUSB

#ifdef WIN32
#include <windows.h>
#endif

#if defined GC_HAVE_LIBUSB1
#include <libusb-1.0/libusb.h>
#elif defined GC_HAVE_LIBUSB
#include <usb.h> // for the constants etc
#include <errno.h>
const int LIBUSB_ERROR_IO = -EIO;
const int LIBUSB_ERROR_TIMEOUT = -ETIMEDOUT;
const int LIBUSB_ERROR_PIPE = -EPIPE;
const int LIBUSB_ERROR_NO_DEVICE = -ENODEV;
#else
const int LIBUSB_ERROR_IO = -1;
const int LIBUSB_ERROR_TIMEOUT = -1;
const int LIBUSB_ERROR_PIPE = -1;
const int LIBUSB_ERROR_NO_DEVICE = -1;
#endif

// EZ-USB firmware loader for Fortius
#include "EzUsb.h"

#ifdef WIN32
#include <QLibrary> // for dynamically loading libusb0.dll
#endif

#include <QQueue>

const int GARMIN_USB2_VID = 0x0fcf;
const int GARMIN_USB2_PID = 0x1008;
const int GARMIN_OEM_PID  = 0x1009;

const int FORTIUS_VID      = 0x3561;
const int FORTIUS_INIT_PID = 0xe6be;  // uninitialised Fortius PID
const int FORTIUS_PID      = 0x1942;  // once firmware loaded Fortius PID
const int FORTIUSVR_PID    = 0x1932;  // Fortius VR doesn't need firmware download ?

const int TYPE_ANT = 0;
const int TYPE_FORTIUS = 1;

class Context;

class LibUsb {

public:
    LibUsb(int type);
    ~LibUsb();

    // Finds an appropriate USB device and opens a handle to it.
    // Returns 0 on success, -1 on error.
    int open();
    void close();
    int read(uint8_t *buf, int bytes);
    int write(uint8_t *buf, int bytes);
    bool find();
private:
#if defined GC_HAVE_LIBUSB1
    // Resprentation of a libusb session.
    libusb_context *ctx;
    libusb_device *device;
    libusb_device_handle *handle;

    // The list of USB devices currently attached to the system.
    libusb_device **devlist;
    // The size of that USB device list.
    ssize_t devlist_size;

    // Refresh our copy of the USB device list.
    void refresh_device_list();

    // Reset the USB interface.
    int reset_device(libusb_device *device);

    int open_device(libusb_device *device,
		    const struct libusb_device_descriptor &desc);
    
    // Initialise a USB ANT+ device.
    int initialise_ant(libusb_device *device,
		       const struct libusb_device_descriptor &desc);

    // Initialise the Fortius device by loading the firmware.
    int initialise_fortius(libusb_device *device);

    QQueue<uint8_t> read_buffer;
#else
    struct usb_dev_handle* OpenAntStick();
    struct usb_dev_handle* OpenFortius();
    bool findAntStick();
    bool findFortius();

    struct usb_interface_descriptor* usb_find_interface(struct usb_config_descriptor* config_descriptor);
    struct usb_dev_handle* device;
    struct usb_interface_descriptor* intf;

    char readBuf[64];
    int readBufIndex;
    int readBufSize;
#endif

    // The I/O endpoints on the USB ANT+ device.
    // These four values are set by open_device().
    int readEndpoint, writeEndpoint;
    int interface;
    int alternate;

    // The type of device -- ANT or FORTIUS.
    int type;
};
#endif
#endif // gc_LibUsb_h
