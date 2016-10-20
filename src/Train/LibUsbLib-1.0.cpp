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

#include <QDebug>
#include <libusb-1.0/libusb.h>

// EZ-USB firmware loader for Fortius
#include "EzUsb-1.0.h"

#include "LibUsbLib.h"

class LibUsbLibUtils
{
public:
    void logError(const char *fctName, int errorCode) const;
};

//-----------------------------------------------------------------------------
// Declarations
//
class LibUsbLib::Impl
{
public:
    ~Impl();

    LibUsbLibUtils *utils = new LibUsbLibUtils;
    libusb_context *ctx = NULL;
};
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLibUtils
//
void LibUsbLibUtils::logError(const char *fctName, int errorCode) const
{
    qDebug().nospace() << "Error in function '" << fctName << "'. ErrorCode=" << errorCode
             << ". ErrorMessage: '" << libusb_error_name(errorCode) << "'";
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LibUsbLib::Impl
//
LibUsbLib::Impl::~Impl()
{
    delete utils;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLib
//
LibUsbLib::LibUsbLib() : impl(new Impl)
{
}

LibUsbLib::~LibUsbLib()
{
    libusb_exit(impl->ctx);
    delete impl;
}

void LibUsbLib::initialize(int logLevel)
{
    int rc = libusb_init(&impl->ctx);
    if (rc < 0)
    {
        impl->utils->logError("libusb_init", rc);
        return;
    }

    libusb_set_debug(impl->ctx, logLevel);
}
//-----------------------------------------------------------------------------
