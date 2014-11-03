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

#include "MoxyDevice.h"
#include "DownloadRideDialog.h"
#include "PowerTapUtil.h"
#include "Device.h"
#include "RideFile.h"

#include <math.h>
#include <errno.h>
#include <string.h>

#include <QThread> // for msleep

static bool moxyRegistered =
    Devices::addType("Moxy Muscle Oxygen Monitor", DevicesPtr(new MoxyDevices()) );

QString
MoxyDevices::downloadInstructions() const
{
    return (tr("NOT WORKING YET .. DO NOT USE\nMake sure the Moxy is connected via USB"));
}

DevicePtr
MoxyDevices::newDevice( CommPortPtr dev )
{
    return DevicePtr( new MoxyDevice( dev ));
}

bool
MoxyDevice::download( const QDir &tmpdir,
                         QList<DeviceDownloadFile> &files,
                         QString &err)
{ Q_UNUSED(tmpdir);
  Q_UNUSED(files);

    int bytes=0;
    char vbuf[256];

    QString deviceInfo;

    if (!dev->open(err)) {
        err = tr("ERROR: open failed: ") + err;
        return false;
    }
    dev->setBaudRate(115200, err);

    // get into engineering mode
    if (writeCommand(dev, "\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    }

    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        return false;
    }

    if (writeCommand(dev, "\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    }

    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        return false;
    }

    // now get a prompt
    if (writeCommand(dev, "\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    }

    // get a prompt back ?
    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        return false;
    }

    // now get version
    if (writeCommand(dev, "ver\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        deviceInfo += vbuf;
    } else {
        return false;
    }

    // now get time on unit
    if (writeCommand(dev, "time\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        deviceInfo += vbuf;
    } else {
        return false;
    }

    // now get battery status
    if (writeCommand(dev, "batt\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        deviceInfo += vbuf;
    } else {
        return false;
    }

    // now get update mode
    if (writeCommand(dev, "um\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        deviceInfo += vbuf;
    } else {
        return false;
    }

    emit updateStatus(deviceInfo);

    // now lets get the data
    if (writeCommand(dev, "gd\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    }
    emit updateStatus(deviceInfo);
    emit updateStatus(QString(tr("Downloading ... \n")));

    do {
        if ((bytes=readData(dev, vbuf, 256, err)) > 0) {
            vbuf[bytes] = '\0';
            qDebug()<<vbuf;
        } else break;
    } while(vbuf[0] != '>');

    // exit
    if (writeCommand(dev, "exit\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    }

    // close device
    dev->close();

    return true;
}


bool
MoxyDevice::cleanup( QString &err ) {
    emit updateStatus(tr("Erase all records on computer"));

    // open device
    if (!dev->open(err)) {
        err = tr("ERROR: open failed: ") + err;
    }

    dev->setBaudRate(115200, err);

    dev->close();

    return true;
}

static bool
hasNewline(const char *buf, int len)
{
    static char newline[] = { 0x0d, 0x0a };
    if (len < 2)
        return false;
    for (int i = 0; i < len; ++i) {
        bool success = true;
        for (int j = 0; j < 2; ++j) {
            if (buf[i+j] != newline[j]) {
                success = false;
                break;
            }
        }
        if (success)
            return true;
    }
    return false;
}

static QString
cEscape(const char *buf, int len)
{
    char *result = new char[4 * len + 1];
    char *tmp = result;
    for (int i = 0; i < len; ++i) {
        if (buf[i] == '"')
            tmp += sprintf(tmp, "\\\"");
        else if (isprint(buf[i]))
            *(tmp++) = buf[i];
        else
            tmp += sprintf(tmp, "\\x%02x", 0xff & (unsigned) buf[i]);
    }
    return result;
}

// read the entire response after a command issued it ends with
// a command prompt
int
MoxyDevice::readUntilPrompt(CommPortPtr dev, char *buf, int len, QString &err)
{

    int sofar = 0;
    while (sofar < len) {
        // Read one byte at a time to avoid waiting for timeout.
        int n = dev->read(buf + sofar, 1, err);
        if (n <= 0) {
            err = (n < 0) ? (tr("read error: ") + err) : tr("read timeout");
            err += QString(tr(", read %1 bytes so far: \"%2\""))
                .arg(sofar).arg(sofar > 0 ? cEscape(buf, sofar) : "");
            return -1;
        }

        sofar += n;

        // we got our prompt
        if (*(buf+sofar-1) == '>') break;
    }
    return sofar;
}

// just read whatever we get before the new line
int
MoxyDevice::readUntilNewline(CommPortPtr dev, char *buf, int len, QString &err)
{
    int sofar = 0;
    while (!hasNewline(buf, sofar) && sofar < len) {
        // Read one byte at a time to avoid waiting for timeout.
        int n = dev->read(buf + sofar, 1, err);
        if (n <= 0) {
            err = (n < 0) ? (tr("read error: ") + err) : tr("read timeout");
            err += QString(tr(", read %1 bytes so far: \"%2\""))
                .arg(sofar).arg(cEscape(buf, sofar));
            return -1;
        }
        sofar += n;
    }
    return sofar;
}

// read a line of data, if it starts with a prompt ">" then its done
int
MoxyDevice::readData(CommPortPtr dev, char *buf, int len, QString &err)
{
    int sofar = 0;
    while (sofar < len) {
        // Read one byte at a time to avoid waiting for timeout.
        int n = dev->read(buf + sofar, 1, err);
        if (n <= 0) {
            err = (n < 0) ? (tr("read error: ") + err) : tr("read timeout");
            err += QString(tr(", read %1 bytes so far: \"%2\""))
                .arg(sofar).arg(cEscape(buf, sofar));
            return -1;
        }

        sofar += n;

        if (hasNewline(buf, sofar) || *buf == '>') {
            break;
        }
    }
    return sofar;
}

bool
MoxyDevice::writeCommand(CommPortPtr dev, const char *command, QString &err)
{
    QThread::msleep(100); // wait a tenth of a second

    int len = strlen(command);
    int n = dev->write(const_cast<char*>(command), len, err);

    if (n != len) {
        if (n < 0)
            err = QString(tr("failed to write '%1' to device: %2")).arg(const_cast<char*>(command)).arg(err);
        else
            err = QString(tr("timeout writing '%1' to device")).arg(const_cast<char*>(command));
        return false;
    }
    return true;
}
