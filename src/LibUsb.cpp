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

#include <unistd.h>
#include "LibUsb.h"
#include "Settings.h"
#include "Context.h"

LibUsb::LibUsb(int type) : type(type)
{

    intf = NULL;
    readBufIndex = 0;
    readBufSize = 0;

    // Initialize the library.
    usb_init();
    usb_set_debug(0);
    usb_find_busses();
    usb_find_devices();
}

int LibUsb::open()
{
    // reset counters
    intf = NULL;
    readBufIndex = 0;
    readBufSize = 0;

    // Find all busses.
    usb_find_busses();

    // Find all connected devices.
    usb_find_devices();

    switch (type) {

    // Search USB busses for USB2 ANT+ stick host controllers
    default:
    case TYPE_ANT: device = OpenAntStick();
              break;

    case TYPE_FORTIUS: device = OpenFortius();
              break;
    }

    if (device == NULL) return -1;

    // Clear halt is needed, but ignore return code
    usb_clear_halt(device, writeEndpoint);
    usb_clear_halt(device, readEndpoint);

    return 0;
}

bool LibUsb::find()
{
    usb_set_debug(0);
    usb_find_busses();
    usb_find_devices();

    switch (type) {

    // Search USB busses for USB2 ANT+ stick host controllers
    default:
    case TYPE_ANT: return findAntStick();
              break;

    case TYPE_FORTIUS: return findFortius();
              break;
    }
}

void LibUsb::close()
{
    if (device) {
        // stop any further write attempts whilst we close down
        usb_dev_handle *p = device;
        device = NULL;

        usb_release_interface(p, interface);
        //usb_reset(p);
        usb_close(p);
    }
}

int LibUsb::read(char *buf, int bytes)
{
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

    int rc;
    if (OperatingSystem == WINDOWS) {
        rc = usb_interrupt_write(device, writeEndpoint, buf, bytes, 1000);
    } else {
        // we use a non-interrupted write on Linux/Mac since the interrupt
        // write block size is incorrectly implemented in the version of
        // libusb we build with. It is no less efficient.
        rc = usb_bulk_write(device, writeEndpoint, buf, bytes, 125);
    }

    if (rc < 0)
    {
        // Report timeouts - previously we ignored -110 errors. This masked a serious
        // problem with write on Linux/Mac, the USB stick needed a reset to avoid
        // these error messages, so we DO report them now
        qDebug()<<"usb_interrupt_write Error writing ["<<rc<<"]: "<< usb_strerror();
    }

    return rc;
}

bool LibUsb::findFortius()
{
    struct usb_bus* bus;
    struct usb_device* dev;

    bool found = false;

    //
    // Search for an UN-INITIALISED Fortius device
    //
    for (bus = usb_get_busses(); bus; bus = bus->next) {


        for (dev = bus->devices; dev; dev = dev->next) {


            if (dev->descriptor.idVendor == FORTIUS_VID && dev->descriptor.idProduct == FORTIUS_INIT_PID) {
                found = true;
            }
            if (dev->descriptor.idVendor == FORTIUS_VID && dev->descriptor.idProduct == FORTIUS_PID) {
                found = true;
            }
            if (dev->descriptor.idVendor == FORTIUS_VID && dev->descriptor.idProduct == FORTIUSVR_PID) {
                found = true;
            }
        }
    }
    return found;
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
struct usb_dev_handle* LibUsb::OpenFortius()
{
    struct usb_bus* bus;
    struct usb_device* dev;
    struct usb_dev_handle* udev;

    bool programmed = false;

    //
    // Search for an UN-INITIALISED Fortius device
    //
    for (bus = usb_get_busses(); bus; bus = bus->next) {


        for (dev = bus->devices; dev; dev = dev->next) {


            if (dev->descriptor.idVendor == FORTIUS_VID && dev->descriptor.idProduct == FORTIUS_INIT_PID) {

                if ((udev = usb_open(dev))) {

                    // LOAD THE FIRMWARE
                    ezusb_load_ram (udev, appsettings->value(NULL, FORTIUS_FIRMWARE, "").toString().toLatin1(), 0, 0);
                }

                // Now close the connection, our work here is done
                usb_close(udev);
                programmed = true;

            }
        }
    }

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
    if (programmed == true) {
#ifdef WIN32
        Sleep(3000); // windows sleep is in milliseconds
#else
        sleep(3);  // do not be tempted to reduce this, it really does take that long!
#endif
        usb_find_busses();
        usb_find_devices();
    }

    //
    // Now search for an INITIALISED Fortius device
    //
    for (bus = usb_get_busses(); bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor == FORTIUS_VID && 
                (dev->descriptor.idProduct == FORTIUS_PID || dev->descriptor.idProduct == FORTIUSVR_PID)) {

                //Avoid noisy output
                //qDebug() << "Found a Garmin USB2 ANT+ stick";

                if ((udev = usb_open(dev))) {

                    if (dev->descriptor.bNumConfigurations) {

                        if ((intf = usb_find_interface(&dev->config[0])) != NULL) {

                            int rc = usb_set_configuration(udev, 1);
                            if (rc < 0) {
                                qDebug()<<"usb_set_configuration Error: "<< usb_strerror();

                                if (OperatingSystem == LINUX) {
                                    // looks like the udev rule has not been implemented
                                    qDebug()<<"check permissions on:"<<QString("/dev/bus/usb/%1/%2").arg(bus->dirname).arg(dev->filename);
                                    qDebug()<<"did you remember to setup a udev rule for this device?";
                                }
                            }

                            rc = usb_claim_interface(udev, interface);
                            if (rc < 0) qDebug()<<"usb_claim_interface Error: "<< usb_strerror();

                            if (OperatingSystem != OSX) {
                                // fails on Mac OS X, we don't actually need it anyway
                                rc = usb_set_altinterface(udev, alternate);
                                if (rc < 0) qDebug()<<"usb_set_altinterface Error: "<< usb_strerror();
                            }

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

bool LibUsb::findAntStick()
{
    struct usb_bus* bus;
    struct usb_device* dev;
    bool found = false;
    for (bus = usb_get_busses(); bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor == GARMIN_USB2_VID && 
                (dev->descriptor.idProduct == GARMIN_USB2_PID || dev->descriptor.idProduct == GARMIN_OEM_PID)) {
                found = true;
            }
        }
    }
    return found;
}

struct usb_dev_handle* LibUsb::OpenAntStick()
{
    struct usb_bus* bus;
    struct usb_device* dev;
    struct usb_dev_handle* udev;

// for Mac and Linux we do a bus reset on it first...
#ifndef WIN32
    for (bus = usb_get_busses(); bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor == GARMIN_USB2_VID &&
                (dev->descriptor.idProduct == GARMIN_USB2_PID || dev->descriptor.idProduct == GARMIN_OEM_PID)) {

                if ((udev = usb_open(dev))) {
                    usb_reset(udev);
                    usb_close(udev);
                }
            }
        }
    }
#endif

    for (bus = usb_get_busses(); bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor == GARMIN_USB2_VID && 
                (dev->descriptor.idProduct == GARMIN_USB2_PID || dev->descriptor.idProduct == GARMIN_OEM_PID)) {

                //Avoid noisy output
                //qDebug() << "Found a Garmin USB2 ANT+ stick";

                if ((udev = usb_open(dev))) {

                    if (dev->descriptor.bNumConfigurations) {

                        if ((intf = usb_find_interface(&dev->config[0])) != NULL) {

#ifdef Q_OS_LINUX
                            usb_detach_kernel_driver_np(udev, interface);
#endif

                            int rc = usb_set_configuration(udev, 1);
                            if (rc < 0) {
                                qDebug()<<"usb_set_configuration Error: "<< usb_strerror();
                                if (OperatingSystem == LINUX) {
                                    // looks like the udev rule has not been implemented
                                    qDebug()<<"check permissions on:"<<QString("/dev/bus/usb/%1/%2").arg(bus->dirname).arg(dev->filename);
                                    qDebug()<<"did you remember to setup a udev rule for this device?";
                                }
                            }

                            rc = usb_claim_interface(udev, interface);
                            if (rc < 0) qDebug()<<"usb_claim_interface Error: "<< usb_strerror();

                            if (OperatingSystem != OSX) {
                                // fails on Mac OS X, we don't actually need it anyway
                                rc = usb_set_altinterface(udev, alternate);
                                if (rc < 0) qDebug()<<"usb_set_altinterface Error: "<< usb_strerror();
                            }

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
