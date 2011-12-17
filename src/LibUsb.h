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

#ifndef gc_LibUsb_h
#define gc_LibUsb_h

#if defined GC_HAVE_LIBUSB

#ifdef WIN32
#include <windows.h>
#endif

#include <usb.h> // for the constants etc

// EZ-USB firmware loader for Fortius
extern "C" {
#include "EzUsb.h"
}

#ifdef WIN32
#include <QLibrary> // for dynamically loading libusb0.dll
#endif

#define GARMIN_USB2_VID   0x0fcf
#define GARMIN_USB2_PID   0x1008

#define FORTIUS_VID       0x3561
#define FORTIUS_INIT_PID  0xe6be    // uninitialised Fortius PID
#define FORTIUS_PID       0x1942    // once firmware loaded Fortius PID

#define TYPE_ANT     0
#define TYPE_FORTIUS 1

class MainWindow;

class LibUsb {

public:
    LibUsb(int type);
    int open();
    void close();
    int read(char *buf, int bytes);
    int write(char *buf, int bytes);
private:

#ifdef WIN32 // we only do dynamic loading on Windows
    /*************************************************************************
     * Functions to load from libusb0.dll
     */
    typedef void (*VoidIntProto)(int);
    typedef void (*VoidProto)();
    typedef int (*IntVoidProto)();
    typedef char * (*CharVoidProto)();
    typedef int (*IntUsb_dev_handleUintProto)(usb_dev_handle *dev, unsigned int);
    typedef int (*IntUsb_dev_handleIntProto)(usb_dev_handle *, int);
    typedef int (*IntUsb_dev_handleProto)(usb_dev_handle *);
    typedef int (*IntUsb_dev_handleIntCharIntIntProto)(usb_dev_handle *, int, char *, int, int);
    typedef struct usb_bus * (*Usb_busVoidProto)();
    typedef struct usb_dev_handle * (*Usb_dev_handleUsb_deviceProto)(struct usb_device *);

    VoidIntProto usb_set_debug;
    CharVoidProto usb_strerror;
    IntVoidProto usb_init;
    IntVoidProto usb_find_busses;
    IntVoidProto usb_find_devices;
    IntUsb_dev_handleUintProto usb_clear_halt;
    IntUsb_dev_handleIntProto usb_release_interface;
    IntUsb_dev_handleProto usb_close;
    IntUsb_dev_handleIntCharIntIntProto usb_bulk_read;
    IntUsb_dev_handleIntCharIntIntProto usb_interrupt_write;
    Usb_busVoidProto usb_get_busses;
    Usb_dev_handleUsb_deviceProto usb_open;
    IntUsb_dev_handleIntProto usb_set_configuration;
    IntUsb_dev_handleIntProto usb_claim_interface;
    IntUsb_dev_handleIntProto usb_set_altinterface;
    /************************************************************************/
#endif

    struct usb_dev_handle* OpenAntStick();
    struct usb_dev_handle* OpenFortius();
    struct usb_interface_descriptor* usb_find_interface(struct usb_config_descriptor* config_descriptor);
    struct usb_dev_handle* device;
    struct usb_interface_descriptor* intf;

    int readEndpoint, writeEndpoint;
    int interface;
    int alternate;

    char readBuf[64];
    int readBufIndex;
    int readBufSize;

    bool isDllLoaded;
    int type;
};
#endif
#endif // gc_LibUsb_h
