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

static const int read_transfer_timeout_ms = 50; //125;
static const int write_transfer_timeout_ms = 125;

LibUsb::LibUsb(int type) : type(type)
{
    read_buffer.clear();
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
  qDebug() << "LibUsb::reset_device: resetting device";
  struct libusb_device_handle *h;
  int r = libusb_open(device, &h);
  if (r != LIBUSB_SUCCESS)
    return -1;
  libusb_reset_device(h);
  libusb_close(h);
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
  qDebug() << "LibUsb::initialise_ant: readEndpoint=" << readEndpoint;
  qDebug() << "LibUsb::initialise_ant: writeEndpoint=" << writeEndpoint;
  // Clear halt is needed, but ignore return code
  libusb_clear_halt(handle, readEndpoint);
  libusb_clear_halt(handle, writeEndpoint);
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
      if (bytes == 1) {
	// ANT always performs one byte requests.
	if (!read_buffer.isEmpty()) {
	  *buf = read_buffer.dequeue();
	  return 1;
	}
      } else {
	// Try to satisfy the read-request using content already
	// in the buffer.
	int read_buffer_queue_size = read_buffer.size();
	if (bytes <= read_buffer_queue_size) {
	  for(int c = 0; c < bytes; c++)
	    buf[c] = read_buffer.dequeue();
	  return bytes;
	}
      }
      
      // Read a bunch of data from the USB stick of at least what would
      // satisfy the user request.
      uint8_t ibuf[128];
      int actual_length;
      int rc = libusb_bulk_transfer(handle, readEndpoint,
				    ibuf, sizeof(ibuf) - 1,
				    &actual_length,
				    read_transfer_timeout_ms);
      if (rc == 0) {
	qDebug() << "LibUsb::read - read " << actual_length << " bytes";
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
      qDebug() << "LibUsb::write writing " << bytes << " bytes, written "
	       << transferred << " bytes";
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

int LibUsb::read(char *, int)
{
    return -1;
}

int LibUsb::write(char *, int)
{
    return -1;
}

#endif // Have LIBUSB and WIN32 or LINUX
