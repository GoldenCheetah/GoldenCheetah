/*
 * Copyright (c) 2016 Antonius Riha (antoniusriha@gmail.com)
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

#if !defined USB_CONFIG_DESCRIPTOR || \
    !defined USB_ENDPOINT_DIR_MASK || \
    !defined USB_INTERFACE_DESCRIPTOR
#error You must define USB_CONFIG_DESCRIPTOR, USB_INTERFACE_DESCRIPTOR and USB_ENDPOINT_DIR_MASK.
#endif

#include "LibUsbLib.h"

UsbDeviceInterface* UsbDevice::Impl::getInterface(USB_CONFIG_DESCRIPTOR *configDescriptor)
{
    if (!configDescriptor ||
        !configDescriptor->bNumInterfaces ||
        !configDescriptor->interface[0].num_altsetting)
    {
        return NULL;
    }

    const USB_INTERFACE_DESCRIPTOR *intf = &configDescriptor->interface[0].altsetting[0];
    if (intf->bNumEndpoints != 2)
    {
        return NULL;
    }

    int readEndpoint = -1;
    int writeEndpoint = -1;
    for (int i = 0 ; i < 2; i++)
    {
        if (intf->endpoint[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK)
        {
            readEndpoint = intf->endpoint[i].bEndpointAddress;
        }
        else
        {
            writeEndpoint = intf->endpoint[i].bEndpointAddress;
        }
    }

    if (readEndpoint < 0 || writeEndpoint < 0)
    {
        return NULL;
    }

    UsbDeviceInterface *interface = new UsbDeviceInterface;
    interface->impl->interfaceNumber = intf->bInterfaceNumber;
    interface->impl->alternateSetting = intf->bAlternateSetting;
    interface->impl->readEndpoint = readEndpoint;
    interface->impl->writeEndpoint = writeEndpoint;
    return interface;
}
