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

#ifdef WIN32
#include <QString>
#include <QDebug>
#include "USBXpress.h"

#ifdef GC_HAVE_USBXPRESS

USBXpress::USBXpress() {} // nothing to do - all members are static

bool USBXpress::find()
{
    DWORD numDevices;

    // any USBXpress devices connected?
    SI_GetNumDevices(&numDevices);
    if (numDevices == 0) return false;

    // lets see if one of them is a GARMIN USB1 stick and open it
    for (unsigned int i=0; i<numDevices; i++) {

        char buffer[128];
        bool vidok, pidok;

        // we want product 1004 and vendor 0fcf
        SI_GetProductString (i, &buffer, SI_RETURN_PID);
        unsigned int vid = QString(buffer).toInt(&vidok, 16);
        SI_GetProductString (i, &buffer, SI_RETURN_VID);
        unsigned int pid = QString(buffer).toInt(&pidok, 16);

        // we found ours?
        if (vidok && vid == GARMIN_USB1_VID && pidok && pid == GARMIN_USB1_PID) {
            return true;
        }
    }

    // no dice. fail.
    return false;
}

int USBXpress::open(HANDLE *handle)
{
    DWORD numDevices;

    // any USBXpress devices connected?
    SI_GetNumDevices(&numDevices);
    if (numDevices == 0) return -1;

    // lets see if one of them is a GARMIN USB1 stick and open it
    for (unsigned int i=0; i<numDevices; i++) {

        char buffer[128];
        bool vidok, pidok;

        // we want product 1004 and vendor 0fcf
        SI_GetProductString (i, &buffer, SI_RETURN_PID);
        unsigned int vid = QString(buffer).toInt(&vidok, 16);
        SI_GetProductString (i, &buffer, SI_RETURN_VID);
        unsigned int pid = QString(buffer).toInt(&pidok, 16);

        // we found ours?
        if (vidok && vid == GARMIN_USB1_VID && pidok && pid == GARMIN_USB1_PID) {

            // bingo
            if (SI_Open (i, handle) == SI_SUCCESS) {

                // clear out any crap
                SI_FlushBuffers(*handle, 1, 1);
                SI_SetTimeouts(5,5); // 5ms timeout seems ok
                SI_SetBaudRate(*handle, 115200); // USB1 supports this
                SI_SetLineControl(*handle, 0x800); // 8bit, 1 stop, no parity
                SI_SetFlowControl(*handle, /*bCTS_MaskCode */ SI_STATUS_INPUT,
                                           /*bRTS_MaskCode */ SI_HELD_INACTIVE,
                                           /*bDTR_MaskCode */ SI_HELD_INACTIVE,
                                           /*bDSRMaskCode  */ SI_STATUS_INPUT,
                                           /*bDCD_MaskCode */ SI_STATUS_INPUT, 
                                           /*bFlowXonXoff  */ FALSE);

                // success!
                return 0;
                
            }
        }
    }

    // no dice. fail.
    return -1;
}

int USBXpress::close(HANDLE *handle)
{
    SI_Close(*handle);
    return 0;
}


int USBXpress::read(HANDLE *handle, unsigned char *buf, int bytes)
{
    DWORD read;
    if (SI_Read (*handle, buf, (DWORD) bytes, &read, NULL) == SI_SUCCESS)
        return read;
    else return -1;
}

int USBXpress::write(HANDLE *handle, unsigned char *buf, int bytes)
{
    DWORD wrote;
    if (SI_Write (*handle, buf, (DWORD) bytes, &wrote, NULL) == SI_SUCCESS)
        return bytes;
    else
        return -1;
}

#else

// if we don't have USBXpress installed then stubs return fail
USBXpress::USBXpress() {} // nothing to do - all members are static

int USBXpress::open(HANDLE *)
{
    return -1;
}

int USBXpress::close(HANDLE *)
{
    return -1;
}


int USBXpress::read(HANDLE *, unsigned char *, int)
{
    return -1;
}

int USBXpress::write(HANDLE *, unsigned char *, int)
{
    return -1;
}

#endif // USBXpress
#endif // Win32
