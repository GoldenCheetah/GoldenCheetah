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

#ifdef LIBUSB_V_1
#include <libusb-1.0/libusb.h>
#include "EzUsb-1.0.h"
#else
#include <usb.h> // for the constants etc

// EZ-USB firmware loader for Fortius
extern "C" {
#include "EzUsb.h"
}
#endif

#ifdef WIN32
#include <QLibrary> // for dynamically loading libusb0.dll
#endif

#define GARMIN_USB2_VID   0x0fcf
#define GARMIN_USB2_PID   0x1008
#define GARMIN_OEM_PID    0x1009

#define FORTIUS_VID       0x3561
#define FORTIUS_INIT_PID  0xe6be    // uninitialised Fortius PID
#define FORTIUS_PID       0x1942    // once firmware loaded Fortius PID
#define FORTIUSVR_PID     0x1932    // Fortius VR doesn't need firmware download ?
#define IMAGIC_PID        0x1902    // Imagic keeps same PID before and after firmware load

#define TYPE_ANT     0
#define TYPE_FORTIUS 1
#define TYPE_IMAGIC  2

#ifdef LIBUSB_V_1
typedef libusb_device_handle usb_dev_handle;
typedef libusb_config_descriptor usb_config_descriptor;
typedef const libusb_interface_descriptor usb_interface_descriptor;

struct usb_device_descriptor
{
    uint16_t idVendor;
    uint16_t idProduct;
    uint8_t  bNumConfigurations = 1;
};

struct usb_device
{
    usb_device();
    ~usb_device();

    usb_device *next = nullptr, *prev = nullptr;
    usb_device_descriptor descriptor;
    libusb_device *rawDev = nullptr;
    usb_config_descriptor *config = nullptr;

    const char *filename;
};

struct usb_bus
{
    ~usb_bus();

    usb_bus *next = nullptr, *prev = nullptr;
    usb_device *devices = nullptr;

    const char *dirname;
};
#endif

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

    usb_dev_handle* OpenAntStick();
    usb_dev_handle* OpenFortius();
    usb_dev_handle* OpenImagic();
    bool findAntStick();
    bool findFortius();
    bool findImagic();


    usb_interface_descriptor* usb_find_interface(usb_config_descriptor* config_descriptor);
    usb_interface_descriptor* usb_find_imagic_interface(usb_config_descriptor* config_descriptor);
    usb_dev_handle* device;
    usb_interface_descriptor* intf;

#if LIBUSB_V_1
    void usb_init();
    void usb_set_debug(int logLevel);
    void usb_find_busses() { /* no-op */ }
    void usb_find_devices() { /* no-op */ }
    usb_bus *usb_get_busses();
    usb_dev_handle *usb_open(usb_device *dev);
    void usb_detach_kernel_driver_np(usb_dev_handle *udev, int interfaceNumber);
    int usb_set_altinterface(usb_dev_handle *udev, int alternate);
    const char *usb_strerror();
    int usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
    int usb_bulk_write(usb_dev_handle *dev, int ep, const char *bytes, int size, int timeout);
    int usb_interrupt_write(usb_dev_handle *dev, int ep, const char *bytes, int size, int timeout);

    int ezusb_load_ram(usb_dev_handle *device, const char *path, int fx2, int stage, Prototype_EzUsb_control_msg uptr);
    int ezusb_load_ram_imagic(usb_dev_handle *device, const char *path, Prototype_EzUsb_control_msg uptr);

    libusb_context *ctx = nullptr;
#endif

    int readEndpoint, writeEndpoint;
    int interface;
    int alternate;

    char readBuf[64];
    int readBufIndex;
    int readBufSize;

    int type;
    int rc;

#ifdef WIN32
    bool libNotInstalled;
    typedef void (*PrototypeVoid)();
    typedef char* (*PrototypeChar_Void)();
    typedef void (*PrototypeVoid_Int)(int);
    typedef int (*PrototypeInt_Handle)(usb_dev_handle*);
    typedef int (*PrototypeInt_Handle_Int)(usb_dev_handle*, unsigned int);
    typedef int (*PrototypeInt_Handle_Int_Char_Int_Int)(usb_dev_handle*, int, char*, int, int);
    typedef struct usb_bus* (*PrototypeBus) (void);
    typedef usb_dev_handle* (*PrototypeHandle_Device) (struct usb_device *dev);


    PrototypeVoid usb_init;
    PrototypeVoid_Int usb_set_debug;
    PrototypeVoid usb_find_busses;
    PrototypeVoid usb_find_devices;
    PrototypeInt_Handle_Int usb_clear_halt;
    PrototypeInt_Handle_Int usb_release_interface;
    PrototypeInt_Handle usb_close;
    PrototypeInt_Handle_Int_Char_Int_Int usb_bulk_read;
    PrototypeInt_Handle_Int_Char_Int_Int usb_bulk_write;
    PrototypeBus usb_get_busses;
    PrototypeHandle_Device usb_open;
    PrototypeInt_Handle_Int usb_set_configuration;
    PrototypeInt_Handle_Int usb_claim_interface;
    PrototypeInt_Handle_Int_Char_Int_Int usb_interrupt_write;
    PrototypeInt_Handle_Int usb_set_altinterface;
    PrototypeChar_Void usb_strerror;

    Prototype_EzUsb_control_msg usb_control_msg;

#endif
};
#endif
#endif // gc_LibUsb_h
