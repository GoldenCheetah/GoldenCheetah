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

#include <QLibrary>
#include <usb.h>

#include "LibUsbLib.h"

//-----------------------------------------------------------------------------
// Declarations
//
class LibUsbLib::Impl
{
public:
    Impl();

    void initialize(int logLevel);

#ifdef WIN32
    bool isLibUsbInstalled() const;

    QLibrary &lib = QLibrary("libusb0");

    typedef void (*PrototypeVoid)();
    typedef void (*PrototypeVoid_Int)(int);

    PrototypeVoid usb_init;
    PrototypeVoid_Int usb_set_debug;
    PrototypeVoid usb_find_busses;
    PrototypeVoid usb_find_devices;
#endif
};
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLib::Impl
//
LibUsbLib::Impl::Impl()
{
#ifdef WIN32
    if (!lib.load())
    {
        return;
    }

    usb_init = PrototypeVoid(lib.resolve("usb_init"));
    usb_set_debug = PrototypeVoid_Int(lib.resolve("usb_set_debug"));
    usb_find_busses = PrototypeVoid(lib.resolve("usb_find_busses"));
    usb_find_devices = PrototypeVoid(lib.resolve("usb_find_devices"));
#endif
}

void LibUsbLib::Impl::initialize(int logLevel)
{
#ifdef WIN32
    if (!isLibUsbInstalled())
    {
        return;
    }
#endif

    usb_init();
    usb_set_debug(logLevel);
    usb_find_busses();
    usb_find_devices();
}

#ifdef WIN32
bool LibUsbLib::Impl::isLibUsbInstalled() const
{
    return lib.isLoaded();
}
#endif
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLib
//
LibUsbLib::LibUsbLib() : impl(new Impl)
{
}

LibUsbLib::~LibUsbLib()
{
    delete impl;
}

void LibUsbLib::initialize(int logLevel)
{
    impl->initialize(logLevel);
}
//-----------------------------------------------------------------------------
