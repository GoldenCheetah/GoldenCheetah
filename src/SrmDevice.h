/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_SrmDevice_h
#define _GC_SrmDevice_h 1
#include "GoldenCheetah.h"

#include "Device.h"

struct SrmDevices : public Devices
{
    SrmDevices( int protoVersion ) : protoVersion( protoVersion ) {}

    virtual DevicePtr newDevice( CommPortPtr dev );
    virtual bool canCleanup( void ) {return true; };

private:
    int protoVersion;
};

struct SrmDevice : public Device
{
    SrmDevice( CommPortPtr dev, int protoVersion ) :
        Device( dev ),
        protoVersion( protoVersion ) {};

    virtual bool download( const QDir &tmpdir,
                          QString &tmpname, QString &filename,
                          StatusCallback statusCallback, QString &err);

    virtual void cleanup();

private:
    int protoVersion;
};

#endif // _GC_SrmDevice_h

