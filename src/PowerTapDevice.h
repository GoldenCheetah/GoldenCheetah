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

#ifndef _GC_PowerTapDevice_h
#define _GC_PowerTapDevice_h 1
#include "GoldenCheetah.h"

#include "Device.h"

struct PowerTapDevices : public Devices
{
    Q_DECLARE_TR_FUNCTIONS(PowerTapDevices)

    public:
    virtual DevicePtr newDevice( CommPortPtr dev );
    virtual QString downloadInstructions() const;
};

struct PowerTapDevice : public Device
{
    Q_DECLARE_TR_FUNCTIONS(PowerTapDevices)

    public:
    PowerTapDevice( CommPortPtr dev ) :
        Device( dev ) {};

    virtual bool download( const QDir &tmpdir,
                          QList<DeviceDownloadFile> &files,
                          QString &err);

    private:
    static bool
    doWrite(CommPortPtr dev, char c, bool hwecho, QString &err);
    static int
    readUntilNewline(CommPortPtr dev, char *buf, int len, QString &err);
};

#endif // _GC_PowerTapDevice_h

