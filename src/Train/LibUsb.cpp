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

#ifndef Q_CC_MSVC
#include <unistd.h>
#endif
#include "LibUsb.h"
#include "Settings.h"
#include "Context.h"

Q_DECLARE_LOGGING_CATEGORY(gcUsb)
Q_LOGGING_CATEGORY(gcUsb, "gc.usb")

#ifdef LIBUSB_V_1
#define USB_ENDPOINT_DIR_MASK LIBUSB_ENDPOINT_DIR_MASK

int (&usb_clear_halt)(usb_dev_handle*, unsigned char) = libusb_clear_halt;
int (&usb_set_configuration)(usb_dev_handle*, int) = libusb_set_configuration;
int (&usb_claim_interface)(usb_dev_handle*, int) = libusb_claim_interface;
int (&usb_release_interface)(usb_dev_handle*, int) = libusb_release_interface;
int (&usb_reset)(usb_dev_handle*) = libusb_reset_device;
void (&usb_close)(usb_dev_handle*) = libusb_close;
#endif

LibUsb::LibUsb(int type) : type(type)
{

    intf = NULL;
    readBufIndex = 0;
    readBufSize = 0;
    rc = 0;

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

    usb_init = PrototypeVoid(lib.resolve("usb_init"));
    usb_set_debug = PrototypeVoid_Int(lib.resolve("usb_set_debug"));
    usb_find_busses = PrototypeVoid(lib.resolve("usb_find_busses"));
    usb_find_devices = PrototypeVoid(lib.resolve("usb_find_devices"));
    usb_clear_halt = PrototypeInt_Handle_Int(lib.resolve("usb_clear_halt"));
    usb_close = PrototypeInt_Handle(lib.resolve("usb_close"));
    usb_bulk_read = PrototypeInt_Handle_Int_Char_Int_Int(lib.resolve("usb_bulk_read"));
    usb_bulk_write = PrototypeInt_Handle_Int_Char_Int_Int(lib.resolve("usb_bulk_write"));
    usb_get_busses = PrototypeBus(lib.resolve("usb_get_busses"));
    usb_open = PrototypeHandle_Device(lib.resolve("usb_open"));
    usb_set_configuration = PrototypeInt_Handle_Int(lib.resolve("usb_set_configuration"));
    usb_claim_interface = PrototypeInt_Handle_Int(lib.resolve("usb_claim_interface"));
    usb_release_interface = PrototypeInt_Handle_Int(lib.resolve("usb_release_interface"));
    usb_interrupt_write = PrototypeInt_Handle_Int_Char_Int_Int(lib.resolve("usb_interrupt_write"));
    usb_set_altinterface = PrototypeInt_Handle_Int(lib.resolve("usb_set_altinterface"));
    usb_strerror = PrototypeChar_Void(lib.resolve("usb_strerror"));

    usb_control_msg = Prototype_EzUsb_control_msg(lib.resolve("usb_control_msg"));

#endif

    // Initialize the library.
    usb_init();
    usb_set_debug(0);
    usb_find_busses();
    usb_find_devices();

}

LibUsb::~LibUsb()
{
#ifdef LIBUSB_V_1
    libusb_exit(ctx);
#endif
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

    case TYPE_IMAGIC: device = OpenImagic();
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
#ifdef WIN32
    if (libNotInstalled) return false;
#endif

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

    case TYPE_IMAGIC: return findImagic();
              break;
    }
}

void LibUsb::close()
{
#ifdef WIN32
    if (libNotInstalled) return;
#endif
    if (device) {
        qCDebug(gcUsb) << "Closing usb device" << device;
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
        qCDebug(gcUsb) << "Read" << QByteArray((const char *)buf, bytes).toHex(':') << "from buffer for usb device" << device;
        return bytes;
    }

    // No, so partially satisfy by emptying the buffer, then refill the buffer for the rest
    // !!! CHECK bufRemain > 0 rather than non-zero fixes annoying bug with ANT+
    //     devices not working again after restart "sometimes"
    if (bufRemain > 0) {
        memcpy(buf, readBuf+readBufIndex, bufRemain);
        qCDebug(gcUsb) << "Read first" << bufRemain << "bytes from buffer for usb device" << device;
    }

    readBufSize = 0;
    readBufIndex = 0;

    int rc = usb_bulk_read(device, readEndpoint, readBuf, 64, timeout);
    if (rc < 0)
    {
        // don't report timeouts by default, lots of noise, active only in gcUsb logging category
        qCDebug(gcUsb) << "usb_bulk_read Error reading:" << rc << usb_strerror();
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

    qCDebug(gcUsb) << "Read" << QByteArray((const char *)buf, rc).toHex(':') << "from usb device" << device;
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

    int rc;
    if (OperatingSystem == WINDOWS) {
        rc = usb_interrupt_write(device, writeEndpoint, buf, bytes, 1000);
        qCDebug(gcUsb) << "Write" << QByteArray((const char *)buf, bytes).toHex(':') << "to usb device" << device;
    } else {
        // we use a non-interrupted write on Linux/Mac since the interrupt
        // write block size is incorrectly implemented in the version of
        // libusb we build with. It is no less efficient.
        rc = usb_bulk_write(device, writeEndpoint, buf, bytes, timeout);
        qCDebug(gcUsb) << "Write" << QByteArray((const char *)buf, bytes).toHex(':') << "to usb device" << device;
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
#ifdef WIN32
    if (libNotInstalled) return false;
#endif
    struct usb_bus* bus, *firstBus;
    struct usb_device* dev;

    bool found = false;

    //
    // Search for an UN-INITIALISED Fortius device
    //
    firstBus = usb_get_busses();
    for (bus = firstBus; bus; bus = bus->next) {


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

#ifdef LIBUSB_V_1
    delete firstBus;
#endif

    qCDebug(gcUsb) << "findFortius returns" << found;
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
usb_dev_handle* LibUsb::OpenFortius()
{
    Prototype_EzUsb_control_msg usb_control_msg_ptr = 0;
#ifdef WIN32
    if (libNotInstalled) return NULL;
    usb_control_msg_ptr = usb_control_msg;
#endif
    struct usb_bus* bus, *firstBus;
    struct usb_device* dev;
    usb_dev_handle* udev;

    bool programmed = false;

    //
    // Search for an UN-INITIALISED Fortius device
    //
    firstBus = usb_get_busses();
    for (bus = firstBus; bus; bus = bus->next) {


        for (dev = bus->devices; dev; dev = dev->next) {


            if (dev->descriptor.idVendor == FORTIUS_VID && dev->descriptor.idProduct == FORTIUS_INIT_PID) {

                if ((udev = usb_open(dev))) {

                    // LOAD THE FIRMWARE
                    qCDebug(gcUsb) << "Load firmware to uninitialized Fortius usb device" << dev->filename;
                    ezusb_load_ram (udev, appsettings->value(NULL, FORTIUS_FIRMWARE, "").toString().toLatin1(), 0, 0, usb_control_msg_ptr);
                }

                // Now close the connection, our work here is done
                usb_close(udev);
                programmed = true;

            }
        }
    }

#ifdef LIBUSB_V_1
    delete firstBus;
#endif

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
    firstBus = usb_get_busses();
    for (bus = firstBus; bus; bus = bus->next) {

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
                            } else {
                                rc = usb_claim_interface(udev, interface);
                                if (rc < 0) {
                                    qDebug()<<"usb_claim_interface Error: "<< usb_strerror();
                                } else {
#ifdef LIBUSB_V_1
                                    delete firstBus;
#endif
                                    qCDebug(gcUsb) << "Open initialized Fortius usb device" << udev << dev->filename;
                                    return udev;
                                }
                            }
                        }
                    }

                    usb_close(udev);
                }
            }
        }
    }

#ifdef LIBUSB_V_1
    delete firstBus;
#endif

    return NULL;
}

bool LibUsb::findImagic()
{
#ifdef WIN32
    if (libNotInstalled) return false;
#endif
    struct usb_bus* bus, *firstBus;
    struct usb_device* dev;

    bool found = false;

    //
    // Search for an Imagic device
    //
    firstBus = usb_get_busses();
    for (bus = firstBus; bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor == FORTIUS_VID && dev->descriptor.idProduct == IMAGIC_PID) {
                found = true;
            }
        }
    }

#ifdef LIBUSB_V_1
    delete firstBus;
#endif

    qCDebug(gcUsb) << "findImagic returns" << found;
    return found;
}

// Open connection to a Tacx Imagic
//
// The Imagic handlebar controller is an EZ-USB device. This is an
// embedded system using an 8051 microcontroller. Firmware must be
// downloaded to it once it is connected. This firmware is embedded in
// the Tacx windows device driver called imagic.sys. This is
// copyrighted by Tacx and is therefore not distributed with Golden Cheetah.
// Instead we ask the user to tell us where it can be found when they
// configure the device. (On Windows platforms the driver is installed
// by the standard Tacx software as c:\windows\system32\drivers\imagic.sys).
//
// So when we open an imagic device we need to search for a
// handlebar controller 3651:1902 and upload the firmware using the EzUsb
// functions. Unlike the fortius, there is no change of productid after
// the firmware load, but we still need to delay a couple of seconds to
// allow it to "settle".
//
// Firmware will need to be reloaded if the device is disconnected or the
// USB controller is reset after sleep/resume.
//
// The same ezusb.c module is used to load the imagic firmware as is used
// for Fortius (see above comments), although some new procedures have been
// added specific to imagic
usb_dev_handle* LibUsb::OpenImagic()
{
    Prototype_EzUsb_control_msg usb_control_msg_ptr = 0;

#ifdef WIN32
#define IMAGIC_SLEEP Sleep(3000)
    if (libNotInstalled) return NULL;
    usb_control_msg_ptr = usb_control_msg;
#else
#define IMAGIC_SLEEP sleep(3)
#endif

    struct usb_bus* bus, *firstBus;
    struct usb_device* dev;
    usb_dev_handle* udev;

    //
    // Search for an Imagic device
    //
    firstBus = usb_get_busses();
    for (bus = firstBus; bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == FORTIUS_VID && dev->descriptor.idProduct == IMAGIC_PID) {

                if ((udev = usb_open(dev))) {

                    // LOAD THE FIRMWARE
                    qCDebug(gcUsb) << "Load firmware to uninitialized Imagic usb device" << dev->filename;
                    int rc = ezusb_load_ram_imagic(udev, appsettings->value(NULL, IMAGIC_FIRMWARE, "").toString().toLatin1(), usb_control_msg_ptr);

                    if (rc < 0) {
                        qDebug()<<"Unable to load controller RAM - did you specify a valid I-Magic.sys file?";
                    }
                    else {
                        // Now delay for a while to allow the imagic to load its firmware
                        IMAGIC_SLEEP;
                        if (dev->descriptor.bNumConfigurations) {
                            if ((intf = usb_find_imagic_interface(&dev->config[0])) != NULL) {

                                rc = usb_set_configuration(udev, 1);
                                if (rc < 0) {
                                        qDebug()<<"usb_set_configuration Error: "<< usb_strerror();
                                        if (OperatingSystem == LINUX) {
                                            // looks like the udev rule has not been implemented
                                            qDebug()<<"check permissions on:"<<QString("/dev/bus/usb/%1/%2").arg(bus->dirname).arg(dev->filename);
                                            qDebug()<<"did you remember to setup a udev rule for this device?";
                                        }
                                }
                                else {
                                        rc = usb_claim_interface(udev, interface);
                                        if (rc < 0) {
                                            qDebug()<<"usb_claim_interface Error: "<< usb_strerror();
                                        }
                                        else {
                                            if (OperatingSystem != OSX) {
                                                // fails on Mac OS X, we don't actually need it anyway
                                                rc = usb_set_altinterface(udev, alternate);
                                                if (rc < 0) qDebug()<<"usb_set_altinterface Error: "<< usb_strerror();
                                            }
#ifdef LIBUSB_V_1
                                            delete firstBus;
#endif
                                        qCDebug(gcUsb) << "Open initialized Imagic usb device" << udev << dev->filename;
                                        return udev;
                                        }
                                     }
                            }
                        }
                    }
                    // Close the connection if validation failed after open
                    usb_close(udev);
                }
            }
        }
    }

#ifdef LIBUSB_V_1
    delete firstBus;
#endif

    return NULL;
}

bool LibUsb::findAntStick()
{

#ifdef WIN32
    if (libNotInstalled) return false;
#endif

    struct usb_bus* bus, *firstBus;
    struct usb_device* dev;
    bool found = false;
    firstBus = usb_get_busses();
    for (bus = firstBus; bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor == GARMIN_USB2_VID && 
                (dev->descriptor.idProduct == GARMIN_USB2_PID || dev->descriptor.idProduct == GARMIN_OEM_PID)) {
                found = true;
            }
        }
    }

#ifdef LIBUSB_V_1
    delete firstBus;
#endif

    qCDebug(gcUsb) << "findAnt returns" << found;
    return found;
}

usb_dev_handle* LibUsb::OpenAntStick()
{
#ifdef WIN32
    if (libNotInstalled) return NULL;
#endif
    struct usb_bus* bus, *firstBus;
    struct usb_device* dev;
    usb_dev_handle* udev;

// for Mac and Linux we do a bus reset on it first...
#ifndef WIN32
    firstBus = usb_get_busses();
    for (bus = firstBus; bus; bus = bus->next) {

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

#ifdef LIBUSB_V_1
    delete firstBus;
#endif
#endif

    firstBus = usb_get_busses();
    for (bus = firstBus; bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor == GARMIN_USB2_VID && 
                (dev->descriptor.idProduct == GARMIN_USB2_PID || dev->descriptor.idProduct == GARMIN_OEM_PID)) {

                //Avoid noisy output
                qDebug() << "Found a Garmin USB2 ANT+ stick:" << QString("/dev/bus/usb/%1/%2").arg(bus->dirname).arg(dev->filename);

                if ((udev = usb_open(dev))) {

                    if (dev->descriptor.bNumConfigurations) {

                        if ((intf = usb_find_interface(&dev->config[0])) != NULL) {

#ifdef Q_OS_LINUX
                            usb_detach_kernel_driver_np(udev, interface);
#endif

                            rc = usb_set_configuration(udev, 1);
                            if (rc < 0) {
                                qDebug()<<"usb_set_configuration Error: "<< usb_strerror();
                                if (OperatingSystem == LINUX) {
                                    // looks like the udev rule has not been implemented
                                    qDebug()<<"check permissions on:"<<QString("/dev/bus/usb/%1/%2").arg(bus->dirname).arg(dev->filename);
                                    qDebug()<<"did you remember to setup a udev rule for this device?";
                                }
                            } else {
                                rc = usb_claim_interface(udev, interface);
                                if (rc < 0) {
                                    qDebug()<<"usb_claim_interface Error: "<< usb_strerror();
                                } else {
                                    if (OperatingSystem != OSX) {
                                        // fails on Mac OS X, we don't actually need it anyway
                                        rc = usb_set_altinterface(udev, alternate);
                                        if (rc < 0) qDebug()<<"usb_set_altinterface Error: "<< usb_strerror();
                                    }

#ifdef LIBUSB_V_1
                                    delete firstBus;
#endif
                                    qCDebug(gcUsb) << "Open Ant usb device" << udev << dev->filename;
                                    return udev;
                                }
                            }
                        }
                    }

                    usb_close(udev);
                }
            }
        }
    }

#ifdef LIBUSB_V_1
    delete firstBus;
#endif

    return NULL;
}

usb_interface_descriptor* LibUsb::usb_find_interface(usb_config_descriptor* config_descriptor)
{
#ifdef WIN32
    if (libNotInstalled) return NULL;
#endif

    usb_interface_descriptor* intf;

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

usb_interface_descriptor* LibUsb::usb_find_imagic_interface(usb_config_descriptor* config_descriptor)
{
#ifdef WIN32
    if (libNotInstalled) return NULL;
#endif

    usb_interface_descriptor* intf;

    readEndpoint = -1;
    writeEndpoint = -1;
    interface = -1;
    alternate = -1;

    if (!config_descriptor) return NULL;

    if (!config_descriptor->bNumInterfaces) return NULL;

    if (!config_descriptor->interface[0].num_altsetting) return NULL;

    intf = &config_descriptor->interface[0].altsetting[1];

    if (intf->bNumEndpoints != 13) return NULL;

    interface = intf->bInterfaceNumber;
    alternate = intf->bAlternateSetting;

    for (int i = 1 ; i < 3; i++)
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

#ifdef LIBUSB_V_1

usb_device::usb_device()
    : descriptor()
{
}

usb_device::~usb_device()
{
    libusb_free_config_descriptor(config);
    libusb_unref_device(rawDev);

    if (prev)
    {
        prev->next = NULL;
        delete prev;
    }

    if (next)
    {
        next->prev = NULL;
        delete next;
    }
}

usb_bus::~usb_bus()
{
    if (devices)
        delete devices;

    if (prev)
    {
        prev->next = NULL;
        delete prev;
    }

    if (next)
    {
        next->prev = NULL;
        delete next;
    }
}

void LibUsb::usb_init()
{
    rc = libusb_init(&ctx);
    if (rc < 0)
    {
        qCritical() << "libusb_init failed with rc=" << rc;
    }
}

void LibUsb::usb_set_debug(int logLevel)
{
    libusb_set_debug(ctx, logLevel);
}

usb_bus *LibUsb::usb_get_busses()
{
    libusb_device **list;
    ssize_t listCount = libusb_get_device_list(ctx, &list);
    if (listCount < 0)
    {
        qInfo() << "libusb_get_device_list returned rc=" << listCount;
        return NULL;
    }

    usb_bus *firstBus;
    usb_bus *bus = NULL;
    usb_bus *lastBus = NULL;
    int lastBusNumber = -1;
    usb_device *lastDev = NULL;
    for (int i = 0; i < listCount; i++)
    {
        libusb_device *rawDev = list[i];
        libusb_device_descriptor deviceDescriptor;

        rc = libusb_get_device_descriptor(rawDev, &deviceDescriptor);
        if (rc < 0)
        {
            qDebug() << "libusb_get_device_descriptor returned rc=" << rc << " for dev no " << i;
            libusb_unref_device(rawDev);
            continue;
        }

        libusb_config_descriptor *configDescriptor;
        rc = libusb_get_config_descriptor(rawDev, 0, &configDescriptor);
        if (rc < 0)
        {
            qDebug() << "libusb_get_config_descriptor returned rc=" << rc << " for dev no " << i;
            libusb_unref_device(rawDev);
            continue;
        }

        int busNumber = libusb_get_bus_number(rawDev);
        if (busNumber != lastBusNumber)
        {
            bus = new usb_bus;
            bus->dirname = QString("%1").arg(busNumber, 3, 10, QChar('0')).toLatin1();

            if (lastBus)
            {
                lastBus->next = bus;
                bus->prev = lastBus;
            }
            else
            {
                firstBus = bus;
            }

            lastBusNumber = busNumber;
            lastBus = bus;
        }

        usb_device *dev = new usb_device;
        if (!lastDev)
        {
            bus->devices = dev;
        }
        else
        {
            lastDev->next = dev;
            dev->prev = lastDev;
        }

        lastDev = dev;

        int deviceAddress = libusb_get_device_address(rawDev);
        dev->filename = QString("%1").arg(deviceAddress, 3, 10, QChar('0')).toLatin1();

        dev->descriptor.idVendor = deviceDescriptor.idVendor;
        dev->descriptor.idProduct = deviceDescriptor.idProduct;
        dev->rawDev = rawDev;
        dev->config = configDescriptor;
    }

    libusb_free_device_list(list, 0);
    return firstBus;
}

usb_dev_handle *LibUsb::usb_open(usb_device *dev)
{
    usb_dev_handle *handle;
    rc = libusb_open(dev->rawDev, &handle);
    if (rc < 0)
    {
        qCritical() << "libusb_open failed with rc=" << rc;
        return NULL;
    }

    return handle;
}

void LibUsb::usb_detach_kernel_driver_np(usb_dev_handle *udev, int interfaceNumber)
{
    rc = libusb_detach_kernel_driver(udev, interfaceNumber);

    // LIBUSB_ERROR_NOT_FOUND means no kernel driver was attached
    if (rc != LIBUSB_ERROR_NOT_FOUND && rc < 0)
    {
        qCritical() << "libusb_detach_kernel_driver failed with rc=" << rc;
    }
}

int LibUsb::usb_set_altinterface(usb_dev_handle *udev, int alternate)
{
    return libusb_set_interface_alt_setting(udev, interface, alternate);
}

const char *LibUsb::usb_strerror()
{
    return libusb_strerror((libusb_error)rc);
}

int LibUsb::usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
    int actualSize;
    rc = libusb_bulk_transfer(dev, ep, (unsigned char*) bytes, size, &actualSize, timeout);

    // don't report timeouts
    if (rc < 0 && rc != LIBUSB_ERROR_TIMEOUT)
    {
        qWarning() << "libusb_bulk_transfer failed with rc=" << rc;
        return rc;
    }

    return actualSize;
}

int LibUsb::usb_bulk_write(usb_dev_handle *dev, int ep, const char *bytes, int size, int timeout)
{
    int actualSize;
    rc = libusb_bulk_transfer(dev, ep, (unsigned char*) bytes, size, &actualSize, timeout);
    return rc < 0 ? rc : actualSize;
}

int LibUsb::usb_interrupt_write(usb_dev_handle *dev, int ep, const char *bytes, int size, int timeout)
{
    int actualSize;
    rc = libusb_interrupt_transfer(dev, ep, (unsigned char*) bytes, size, &actualSize, timeout);
    return rc < 0 ? rc : actualSize;
}

int LibUsb::ezusb_load_ram(usb_dev_handle *device, const char *path, int fx2, int stage, Prototype_EzUsb_control_msg uptr)
{
    return ezusb_load_ram(device, path, fx2, stage, uptr);
}

int LibUsb::ezusb_load_ram_imagic(usb_dev_handle *device, const char *path, Prototype_EzUsb_control_msg uptr)
{
    return ezusb_load_ram_imagic(device, path, uptr);
}
#endif

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
