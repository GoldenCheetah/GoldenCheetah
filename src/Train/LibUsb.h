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

#include <usb.h> // for the constants etc

#include "LibUsbLib.h"

#define GARMIN_USB2_VID   0x0fcf
#define GARMIN_USB2_PID   0x1008
#define GARMIN_OEM_PID    0x1009

#define FORTIUS_VID       0x3561
#define FORTIUS_INIT_PID  0xe6be    // uninitialised Fortius PID
#define FORTIUS_PID       0x1942    // once firmware loaded Fortius PID
#define FORTIUSVR_PID     0x1932    // Fortius VR doesn't need firmware download ?

#define TYPE_ANT     0
#define TYPE_FORTIUS 1

class Context;

class LibUsb {

public:
    LibUsb(int type);
    ~LibUsb();
    int open();
    void close();
    int read(char *buf, int bytes);
	int read(char *buf, int bytes, int timeout);
    int write(char *buf, int bytes);
	int write(char *buf, int bytes, int timeout);
    bool find();
private:

    UsbDevice *getDevice() const;
    struct usb_dev_handle* OpenAntStick();
    struct usb_dev_handle* OpenFortius();
    struct usb_dev_handle* openUsb(UsbDevice *device, bool detachKernelDriver);

    struct usb_interface_descriptor* usb_find_interface(struct usb_config_descriptor* config_descriptor);
    struct usb_dev_handle* device;
    struct usb_interface_descriptor* intf;

    int readEndpoint, writeEndpoint;

    char readBuf[64];
    int readBufIndex;
    int readBufSize;

    int type;

    LibUsbLib *usbLib;

#ifdef WIN32
    bool libNotInstalled;
    typedef char* (*PrototypeChar_Void)();
    typedef int (*PrototypeInt_Handle)(usb_dev_handle*);
    typedef int (*PrototypeInt_Handle_Int)(usb_dev_handle*, unsigned int);
    typedef int (*PrototypeInt_Handle_Int_Char_Int_Int)(usb_dev_handle*, int, char*, int, int);
    typedef usb_dev_handle* (*PrototypeHandle_Device) (struct usb_device *dev);


    PrototypeInt_Handle_Int usb_clear_halt;
    PrototypeInt_Handle_Int usb_release_interface;
    PrototypeInt_Handle usb_close;
    PrototypeInt_Handle_Int_Char_Int_Int usb_bulk_read;
    PrototypeInt_Handle_Int_Char_Int_Int usb_bulk_write;
    PrototypeHandle_Device usb_open;
    PrototypeInt_Handle_Int usb_set_configuration;
    PrototypeInt_Handle_Int usb_claim_interface;
    PrototypeInt_Handle_Int_Char_Int_Int usb_interrupt_write;
    PrototypeInt_Handle_Int usb_set_altinterface;
    PrototypeChar_Void usb_strerror;

#endif
};
#endif
#endif // gc_LibUsb_h
