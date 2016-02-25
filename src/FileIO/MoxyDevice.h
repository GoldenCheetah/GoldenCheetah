/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#ifndef MOXYDEVICE_H
#define MOXYDEVICE_H

#include "CommPort.h"
#include "Device.h"
#include "stdint.h"

#include <QThread>

class DeviceFileInfo;

struct MoxyDevices : public Devices
{
    Q_DECLARE_TR_FUNCTIONS(MoxyDevices)

    public:
    virtual DevicePtr newDevice( CommPortPtr dev );
    virtual QString downloadInstructions() const;
    virtual bool canCleanup( void ) {return true; };
    virtual bool canPreview() { return false; }; // Moxy is dumb ass
};

struct MoxyDevice : public Device,QThread
{
    Q_DECLARE_TR_FUNCTIONS(MoxyDevice)

    public:
    MoxyDevice( CommPortPtr dev ) : Device( dev ) {};

    // io with the moxy is text based lines
    // they even give you a prompt ">" when
    // the command completes
    int readUntilPrompt(CommPortPtr dev, char *buf, int len, QString &err);
    int readUntilNewline(CommPortPtr dev, char *buf, int len, QString &err);
    int readData(CommPortPtr dev, char *buf, int len, QString &err);

    bool writeCommand(CommPortPtr dev, const char *command, QString &err);

    virtual bool download( const QDir &tmpdir,
                          QList<DeviceDownloadFile> &files,
                          QString &err);

    virtual bool cleanup( QString &err );

};

#endif // MOXYDEVICE_H
