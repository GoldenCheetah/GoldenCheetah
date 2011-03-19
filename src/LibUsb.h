/*
 * Copyright (c) 2011 Darren Hague & Eric Brandt
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

#if defined WIN32

#ifdef GC_HAVE_LIBUSB
#include <usb.h> // for the constants etc
#endif

#define GARMIN_USB2_VID 0x0fcf
#define GARMIN_USB2_PID 0x1008

class LibUsb {

public:
    LibUsb();
    int open();
    void close();
    int read(char *buf, int bytes);
    int write(char *buf, int bytes);
private:
#ifdef GC_HAVE_LIBUSB
    struct usb_dev_handle* OpenAntStick();
    struct usb_interface_descriptor* usb_find_interface(struct usb_config_descriptor* config_descriptor);
    struct usb_dev_handle* device;
    struct usb_interface_descriptor* intf;
    int readEndpoint, writeEndpoint;

    char readBuf[64];
    int readBufIndex;
    int readBufSize;
#endif
};
#endif // WIN32
#endif // gc_LibUsb_h
