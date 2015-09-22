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

static const int read_transfer_timeout_ms = 125;
static const int write_transfer_timeout_ms = 125;

#if defined GC_HAVE_LIBUSB1
LibUsb::LibUsb(int type) : type(type)
{
    // Clear the QQueue of any data and reserve space
    // for 256 elements to avoid unnecessary allocations.
    read_buffer.clear();
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    read_buffer.reserve(256);
#endif

    // Initialize the library.
    devlist = NULL;
    handle = NULL;
    libusb_init(&ctx);
    libusb_set_debug(ctx, 0);
}

LibUsb::~LibUsb() {
    if (devlist)
        libusb_free_device_list(devlist, 0);
    // Ensure any open device is closed.
    close();
    libusb_exit(ctx);
}

void LibUsb::refresh_device_list() {
    if (devlist)
        libusb_free_device_list(devlist, 0);
    devlist_size = libusb_get_device_list(ctx, &devlist);
}

int LibUsb::open()
{
    // reset counters
    read_buffer.clear();
    handle = NULL;
    // Ensure our information about current USB devices is up-to-date
    // just in-case the user moved the ANT stick to a different port.
    refresh_device_list();

    for (ssize_t i = 0; i < devlist_size; i++) {
        libusb_device *device = devlist[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r < 0)
        return -1;
        if (type == TYPE_ANT) {
            if (desc.idVendor == GARMIN_USB2_VID
                    && (desc.idProduct == GARMIN_USB2_PID
                            || desc.idProduct == GARMIN_OEM_PID)) {
                return initialise_ant(device, desc);
            }
        } else if (type == TYPE_FORTIUS) {
            if (desc.idVendor == FORTIUS_VID
                    && desc.idProduct == FORTIUS_INIT_PID) {
                initialise_fortius(device);
                // Do not be tempted to reduce this; it really does that this long.
#ifdef WIN32
                Sleep(3000); // Windows sleep is in ms.
#else
                sleep(3);
#endif
                // Re-scans the USB bus, which should mean that the Fortius
                // USB device is now reporting a different PID, causing this open()
                // call to open the device.
                return open();
            }
            // Search for an initialised Fortius device.
            if (desc.idVendor == FORTIUS_VID
                    && (desc.idProduct == FORTIUS_PID
                            || desc.idProduct == FORTIUSVR_PID)) {
                return initialise_ant(device, desc);
            }
        }
    }
    return -1;
}

int LibUsb::reset_device(libusb_device *device) {
    // Reset USB device on Mac and Linux to ensure that ANT
    // devices can bind properly.
#ifndef WIN32
    qDebug() << "LibUsb::reset_device: resetting device";
    struct libusb_device_handle *h;
    int r = libusb_open(device, &h);
    if (r != LIBUSB_SUCCESS)
        return -1;
    libusb_reset_device(h);
    libusb_close(h);
#endif
    return 0;
}

int LibUsb::initialise_ant(libusb_device *device,
        const struct libusb_device_descriptor &desc) {
    int r = reset_device(device);
    if (r != LIBUSB_SUCCESS)
        return r;
    return open_device(device, desc);
}

int LibUsb::open_device(libusb_device *device,
        const struct libusb_device_descriptor &desc) {
    qDebug() << "LibUsb::initialise_ant: opening device";
    int r = libusb_open(device, &handle);
    if (r != LIBUSB_SUCCESS)
        return -1;
    readEndpoint = -1;
    writeEndpoint = -1;
    interface = -1;
    alternate = -1;

#ifdef Q_OS_LINUX
    // On Linux, the usb_serial kernel driver will automatically claim the
    // ANT+ stick, so we need to detach the driver to use it.  Setting this
    // will ensure libusb automatically detaches and re-attaches when the
    // interface is claimed/released.
    libusb_set_auto_detach_kernel_driver(handle, 1);
#endif

    // Within the device, find the read/write endpoints for I/O.
    qDebug() << "LibUsb::initialise_ant: desc.bNumConfigurations="
    << desc.bNumConfigurations;
    if (desc.bNumConfigurations) {
        struct libusb_config_descriptor *cdesc;
        r = libusb_get_config_descriptor(device, 0, &cdesc);
        if (r < 0) {
            qDebug() << "LibUsb::initialise_ant: get_config_descriptor failed "
                    << libusb_strerror((libusb_error)r);
            return -1;
        }

        if (cdesc->bNumInterfaces == 0) {
            qDebug() << "LibUsb::open_device found no interfaces.";
            libusb_free_config_descriptor(cdesc);
            return -1;
        }
        for (int ifnum = 0; ifnum < cdesc->bNumInterfaces; ifnum++) {
            const struct libusb_interface_descriptor *intf = cdesc->interface[ifnum].altsetting;
            interface = intf->bInterfaceNumber;
            alternate = intf->bAlternateSetting;
            qDebug() << "LibUsb::initialise_ant: checking "
                    << intf->bNumEndpoints
                    << " endpoints";
            if (intf->bNumEndpoints != 2) {
                qDebug() << "LibUsb::initialise_ant: not enough endpoints";
                continue;
            }

            for(int ep = 0; ep < intf->bNumEndpoints; ep++) {
                int ea = intf->endpoint[ep].bEndpointAddress;
                if ((ea & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                readEndpoint = ea | LIBUSB_ENDPOINT_IN;
                else
                writeEndpoint = ea & ~LIBUSB_ENDPOINT_OUT;
            }
        }
        libusb_free_config_descriptor(cdesc);
    }
    if (readEndpoint == -1) {
        qDebug() << "LibUsb::open_device - no read/write endpoints found!";
        return -1;
    }

    // Set an active configuration on the device before claiming the interface.
    r = libusb_set_configuration(handle, 1);
    if (r != LIBUSB_SUCCESS) {
        qDebug() << "LibUsb::open_device: set_configuration error "
                << libusb_strerror((libusb_error)r);
        if (OperatingSystem == LINUX) {
            // looks like the udev rule has not been implemented
            //qDebug() << "Check permissions on: "
            //       << QString("/dev/bus/usb/%1/%2").arg(bus->dirname).
            //arg(dev->filename);
            qDebug() << "Did you remember to setup a udev rule for this device?";
        }
    }

    // Claim the interface.  On Linux this will implicitly detach the kernel
    // driver and a message to that effect will be present in dmesg.
    qDebug() << "LibUsb::initialise_ant: claiming interface " << interface;
    r = libusb_claim_interface(handle, interface);
    if (r != LIBUSB_SUCCESS) {
        qDebug() << "LibUsb::open_device: claiming interface failed "
        << libusb_strerror((libusb_error)r);
        return -1;
    }

    // This call fails on OSX.
    // FIXME: test if this is still true with libusb-1.0.
    if (OperatingSystem != OSX) {
        r = libusb_set_interface_alt_setting(handle, interface, alternate);
        if (r != LIBUSB_SUCCESS) {
            qDebug() << "LibUsb::open_device: set_interface_alt_setting failed "
            << libusb_strerror((libusb_error)r);
            return -1;
        }
    }

    // Clear halt is needed, but ignore return code
    libusb_clear_halt(handle, readEndpoint);
    libusb_clear_halt(handle, writeEndpoint);

    qDebug() << "LibUsb::open_device: ANT+ device successfully initialised";
    return 0;
}

int LibUsb::initialise_fortius(libusb_device *device) {
    int r = reset_device(device);
    if (r != LIBUSB_SUCCESS)
    return r;

    r = libusb_open(device, &handle);
    if (r != LIBUSB_SUCCESS)
    return -1;
    // Load the firmware.
    ezusb_load_ram(handle,
            appsettings->value(NULL, FORTIUS_FIRMWARE,
                    "").toString().toLatin1(), 0, 0, 0);
    libusb_close(handle);
    return 0;
}

bool LibUsb::find()
{
    refresh_device_list();
    for (ssize_t i = 0; i < devlist_size; i++) {
        libusb_device *device = devlist[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r < 0)
        return false;
        if (type == TYPE_ANT) {
            if (desc.idVendor == GARMIN_USB2_VID
                    && (desc.idProduct == GARMIN_USB2_PID
                            || desc.idProduct == GARMIN_OEM_PID)) {
                return true;
            }
        } else if (type == TYPE_FORTIUS) {
            // Looks for Fortius devices that are unitialised.
            if (desc.idVendor == FORTIUS_VID
                    && (desc.idProduct == FORTIUS_INIT_PID
                            || desc.idProduct == FORTIUS_PID
                            || desc.idProduct == FORTIUSVR_PID)) {
                return true;
            }
        }
    }
    return false;
}

void LibUsb::close()
{
    if (handle) {
        // stop any further write attempts whilst we close down
        libusb_device_handle *p = handle;
        handle = NULL;

        libusb_release_interface(p, interface);
        libusb_close(p);
    }
}

// Returns the number of bytes successfully read, or -1 on error.
int LibUsb::read(uint8_t *buf, int bytes)
{
    // check it isn't closed already
    if (!handle)
        return -1;

    for(int retries = 0; retries < 2; retries++) {
        // Try to satisfy the read-request using content already
        // in the buffer.  If the request cannot be fully satisfied
        // then fetch from the USB device and re-try.
        int read_buffer_queue_size = read_buffer.size();
        if (bytes <= read_buffer_queue_size) {
            for(int c = 0; c < bytes; c++)
                buf[c] = read_buffer.dequeue();
            return bytes;
        }

        // Read a bunch of data from the USB stick of at least what would
        // satisfy the user request.  256 bytes should be plenty here
        // because packets are typically 13 bytes and we read them fast
        // enough.  An overflow indicates that something got stuck for
        // a while, blocking the read() call.
        uint8_t ibuf[256];
        int actual_length;
        int rc = libusb_bulk_transfer(handle, readEndpoint,
                ibuf, sizeof(ibuf) - 1,
                &actual_length,
                read_transfer_timeout_ms);
        if (rc == 0) {
            //qDebug() << "LibUsb::read - read " << actual_length << " bytes";
        } else if (rc == LIBUSB_ERROR_OVERFLOW) {
            qDebug() << "LibUsb::read - overflow";
        } else if (rc == LIBUSB_ERROR_TIMEOUT) {
            // Timeouts are frequent, so don't log these.
            return rc;
        } else {
            qDebug() << "LibUsb::read - read failed - "
                    << libusb_strerror((libusb_error)rc);
            return rc;
        }

        // Push all the USB-read data onto our internal queue.
        for(int c = 0; c < actual_length; c++)
            read_buffer.enqueue(ibuf[c]);
    }

    // If the user request couldn't be fully satisfied, return what remains.
    qDebug() << "LibUsb::read - failed to satisfy read request of "
            << " bytes; returning remains of buffer";
    for(int c = 0; c < bytes; c++) {
        buf[c] = read_buffer.dequeue();
        if (read_buffer.isEmpty())
        return c;
    }
    return bytes;
}

int LibUsb::write(uint8_t *buf, int bytes)
{

    // check it isn't closed
    if (!handle)
        return -1;

    int transferred;
    int rc = libusb_bulk_transfer(handle, writeEndpoint, buf, bytes,
            &transferred,
            write_transfer_timeout_ms);
    if (rc == 0) {
        //qDebug() << "LibUsb::write writing " << bytes << " bytes, written "
        //        <<  transferred << " bytes";
    } else {
        // Report timeouts - previously we ignored -110 errors.
        // This masked a serious problem with write on Linux/Mac, the
        // USB stick needed a reset to avoid these error messages, so we
        // DO report them now
        qDebug() << "LibUsb::write error ["<<rc<<"]: "
        << libusb_strerror((libusb_error)rc);
    }

    return rc;
}

#elif defined GC_HAVE_LIBUSB

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

LibUsb::~LibUsb() {}

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

int LibUsb::read(uint8_t *buf, int bytes)
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

int LibUsb::write(uint8_t *buf, int bytes)
{

    // check it isn't closed
    if (!device) return -1;

    int rc;
    if (OperatingSystem == WINDOWS) {
        rc = usb_interrupt_write(device, writeEndpoint, (const char *)buf, bytes, 1000);
    } else {
        // we use a non-interrupted write on Linux/Mac since the interrupt
        // write block size is incorrectly implemented in the version of
        // libusb we build with. It is no less efficient.
        rc = usb_bulk_write(device, writeEndpoint, (const char *)buf, bytes, 125);
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
                    ezusb_load_ram (udev, appsettings->value(NULL, FORTIUS_FIRMWARE, "").toString().toLatin1(), 0, 0, 0);
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

#endif

#else

// if we don't have libusb use stubs
LibUsb::LibUsb() {}

LibUsb::~LibUsb() {}

int LibUsb::open()
{
    return -1;
}

void LibUsb::close()
{
}

int LibUsb::read(uint8_t *, int)
{
    return -1;
}

int LibUsb::write(uint8_t *, int)
{
    return -1;
}

#endif // Have LIBUSB and WIN32 or LINUX
