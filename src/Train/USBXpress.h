/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef gc_USBXpress_h
#define gc_USBXpress_h

#if defined WIN32 

#define NOMINMAX // prevents windows.h defining max & min macros
#include <windows.h>

#ifdef GC_HAVE_USBXPRESS
#include <SiUSBXp.h> // for the constants etc
#endif

#define GARMIN_USB1_PID 0x0fcf
#define GARMIN_USB1_VID 0x1004

class USBXpress {

public:
    USBXpress();
    static int open(HANDLE *handle);
    static int close(HANDLE *handle);
    static int read(HANDLE *handle, unsigned char *buf, int bytes);
    static int write(HANDLE *handle, unsigned char *buf, int bytes);
    static bool find();
};

#endif // WIN32
#endif // gc_USBXpress_h
