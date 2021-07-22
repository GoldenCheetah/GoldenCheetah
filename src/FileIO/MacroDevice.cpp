/*
 * Copyright (c) 2011 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "MacroDevice.h"
#include "DownloadRideDialog.h"
#include "PowerTapUtil.h"
#include "Device.h"
#include <cmath>
#include <errno.h>

#define MACRO_DEBUG false

#define UNKNOWN 0x00

// Commands send from PC to computer
#define ACKNOWLEDGE 0x0A // Acknowledge received data
#define NUMBER_OF_TRAINING_REQUESTS 0x06 // Request the number of trainings
#define TRAINING_DETAIL_REQUEST 0x09 // Request details of a training
#define ERASE_ALL_RECORDS 0x0B // Erase all records on computer

// Commands send from computer to PC
#define NUMBER_OF_TRAININGS 0x86 // Packet with the trainings count
#define TRAINING_DETAIL 0x89 // Packet with details about a specific training
#define ERASE_DONE 0x8B //  Packet which acknowledges that all data is erased in computer

#define LAST_PAGE 0xFA0A // LastPage number

static bool macroRegistered =
    Devices::addType("O-Synce Macro PC-Link", DevicesPtr(new MacroDevices()) );

QString
MacroDevices::downloadInstructions() const
{
    return (tr("Make sure the Macro unit is turned\n"
            "on and that its display says, \"PC Link\""));
}

DevicePtr
MacroDevices::newDevice( CommPortPtr dev )
{
    return DevicePtr( new MacroDevice( dev ));
}

static QString
cEscape(char *buf, int len)
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

static int
bcd2Int(char c)
{
    return (0xff & c) - (((0xff & c)/16)*6);
}

static int
hex2Int(char c)
{
    return (0xff & c);
}

static int
hexHex2Int(char c, char c2)
{
    return (0xff & c) + 256*(0xff & c2);
}

bool
MacroDevice::download( const QDir &tmpdir,
                         QList<DeviceDownloadFile> &files,
                         QString &err)
{
    if (MACRO_DEBUG) printf("download O-Synce Macro");

    if (!dev->open(err)) {
        err = tr("ERROR: open failed: ") + err;
        return false;
    }

    emit updateStatus(tr("Request number of training..."));
    if (MACRO_DEBUG) printf("Request number of training\n");

    MacroPacket cmd(NUMBER_OF_TRAINING_REQUESTS);
    cmd.addToPayload(UNKNOWN);

    if (!cmd.write(dev, err)) return false;

    if(m_Cancelled)
    {
        err = tr("download cancelled");
        return false;
    }

    MacroPacket response = MacroPacket();
    response.read(dev, 2, err);

    if (response.payload.size() == 0)
    {
        err = tr("no data");
        return false;
    }

    char count = response.payload.at(0);

    if (count == 0)
    {
        err = tr("no data");
        return false;
    }

    response.read(dev, 7*count, err);

    if (!response.verifyCheckSum(dev, err))
    {
        err = tr("data error");
        return false;
    }

    if(m_Cancelled)
    {
        err = tr("download cancelled");
        return false;
    }

    // create temporary file
    QString tmpl = tmpdir.absoluteFilePath(".macrodl.XXXXXX");
    QTemporaryFile tmp(tmpl);
    tmp.setAutoRemove(false);

    if (!tmp.open()) {
        err = tr("Failed to create temporary file ")
            + tmpl + ": " + tmp.error();
        return false;
    }

    if (MACRO_DEBUG) printf("Acknowledge");
    cmd= MacroPacket(ACKNOWLEDGE);
    cmd.addToPayload(response.command);
    if (!cmd.write(dev, err)) return false;

    // timestamp from the first training
    struct tm start;
    start.tm_sec = bcd2Int(response.payload.at(2));
    start.tm_min = bcd2Int(response.payload.at(3));
    start.tm_hour = bcd2Int(response.payload.at(4));
    start.tm_mday = bcd2Int(response.payload.at(5));
    start.tm_mon = hex2Int(response.payload.at(6)) -1;
    start.tm_year = bcd2Int(response.payload.at(7)) -100;
    start.tm_isdst = -1;

    DeviceDownloadFile file;
    file.extension = "osyn";
    file.name = tmp.fileName();
    file.startTime.setSecsSinceEpoch( mktime( &start ));
    files.append(file);

    QTextStream os(&tmp);
    os << hex;

    for (int i = 0; i < count; i++)
    {
        if (MACRO_DEBUG) printf("Request training %d\n",i);
        emit updateStatus( QString(tr("Request datas of training %1 / %2..."))
            .arg(i+1).arg((int)count) );

        if(m_Cancelled)
        {
            err = tr("download cancelled");
            return false;
        }

        cmd = MacroPacket(TRAINING_DETAIL_REQUEST);
        cmd.addToPayload(i);
        if (!cmd.write(dev, err)) return false;

        if(m_Cancelled)
        {
            err = tr("download cancelled");
            return false;
        }
        bool lastpage = false;
        while (!lastpage)
        {
            MacroPacket response2 = MacroPacket();
            response2.read(dev, 259, err);

            if (!response2.verifyCheckSum(dev, err))
            {
                err = tr("data error");
                return false;
            }

            if (hexHex2Int(response2.payload.at(0), response2.payload.at(1)) == LAST_PAGE)
                lastpage = true;

            //int training_flag = hex2Int(response2.payload.at(43));
            tmp.write(response2.dataArray());
            emit updateProgress( QString(tr("training %1/%2... (%3 Bytes)"))
                .arg(i+1)
                .arg((int)count)
                .arg(tmp.size()) );
            if(m_Cancelled)
            {
                err = tr("download cancelled");
                return false;
            }

            if (MACRO_DEBUG) printf("Acknowledge\n");

            cmd= MacroPacket(ACKNOWLEDGE);
            cmd.addToPayload(response2.command);
            if (!cmd.write(dev, err)) return false;
        }

     }


    tmp.close();

    dev->close();

    // QTemporaryFile initially has permissions set to 0600.
    // Make it readable by everyone.
    tmp.setPermissions(tmp.permissions()
                       | QFile::ReadOwner | QFile::ReadUser
                       | QFile::ReadGroup | QFile::ReadOther);


    return true;
}

bool
MacroDevice::cleanup( QString &err ){
    if (MACRO_DEBUG) printf("Erase all records on computer\n");

    if (!dev->open(err)) {
        err = tr("ERROR: open failed: ") + err;
    }

    MacroPacket cmd(ERASE_ALL_RECORDS);
    cmd.addToPayload(UNKNOWN);

    if (!cmd.write(dev, err)) if (MACRO_DEBUG) printf("Error during erase all records on computer\n");

    MacroPacket response = MacroPacket();
    response.read(dev, 3, err);

    if (!response.verifyCheckSum(dev, err) || (response.payload.at(0) != (char)ERASE_DONE))
    {
        if (MACRO_DEBUG) printf("Error during erase all records on computer\n");
    }

    dev->close();

    return true;
}

// --------------------------------------
// MacroPacket
// --------------------------------------

MacroPacket::MacroPacket() : command(0)
{
    checksum = 0;
    payload.clear();
}

MacroPacket::MacroPacket(char command) : command(command), checksum(command)
{
    payload.clear();
}

void
MacroPacket::addToPayload(char byte)
{
    checksum += byte;
    payload.append(byte);
}

void
MacroPacket::addToPayload(char *bytes, int len)
{
    for (int i = 0; i < len; ++i) {
        char byte = bytes[i];
        checksum += byte;
        payload.append(byte);
    }
}

bool
MacroPacket::verifyCheckSum(CommPortPtr dev, QString &err)
{
    char _checksum;
    if (MACRO_DEBUG) printf("reading checksum from device\n");
    int n = dev->read(&_checksum, 1, err);
    if (n <= 0) {
        err = (n < 0) ? (tr("read checksum error: ") + err) : tr("read timeout");
        return false;
    }
    if (MACRO_DEBUG) printf("CheckSum1 %d CheckSum2 %d", (0xff & (unsigned) checksum) , (0xff & (unsigned) _checksum));

    return checksum == _checksum;
}

QByteArray
MacroPacket::dataArray()
{
    QByteArray result;
    result.append(command);
    result.append(payload);
    result.append(checksum);

    return result;
}

char*
MacroPacket::data()
{
    QByteArray result;
    result.append(command);
    result.append(payload);
    result.append(checksum);

    return result.data();
}

bool
MacroPacket::write(CommPortPtr dev, QString &err)
{
    const char *msg = cEscape(data(), payload.count()+2).toLatin1().constData();

    if (MACRO_DEBUG) printf("writing '%s' to device\n", msg);

    int n = dev->write(data(), payload.count()+2, err);
    if (n != payload.count()+2) {
        if (n < 0) {
            if (MACRO_DEBUG) printf("failed to write %s to device: %s\n", msg, err.toLatin1().constData());
            err = QString(tr("failed to write to device: %1")).arg(err);
        }
        else {
            if (MACRO_DEBUG) printf("timeout writing %s to device\n", msg);
            err = QString(tr("timeout writing to device"));
        }
        return false;
    }

    if (MACRO_DEBUG) printf("writing to device ok\n");
    return true;
}

bool
MacroPacket::read(CommPortPtr dev, int len, QString &err)
{
    if (command == 0) {
        if (MACRO_DEBUG) printf("reading command from device\n");
        int n = dev->read(&command, 1, err);
        if (n <= 0) {
            err = (n < 0) ? (tr("read command error: ") + err) : tr("read timeout");
            return false;
        }
        checksum += command;
        len--;
        if (MACRO_DEBUG) printf("command %s\n" ,cEscape(&command,n).toLatin1().constData());
    }

    if (MACRO_DEBUG) printf("reading %d from device\n", len);
#ifdef Q_CC_MSVC
    char* buf = new char[len];
    int n = dev->read(buf, len, err);
#else
    char buf [len];
    int n = dev->read(&buf, len, err);
#endif

    if (n <= 0) {
        err = (n < 0) ? (tr("read error: ") + err) : tr("read timeout");
#ifdef Q_CC_MSVC
        delete[] buf;
#endif
        return false;
    } else if (n < len) {
        err += QString(tr(", read only %1 bytes instead of: %2"))
            .arg(n).arg(len);
#ifdef Q_CC_MSVC
        delete[] buf;
#endif
        return false;
    }

    if (MACRO_DEBUG) printf("payload %s\n" ,cEscape(buf,n).toLatin1().constData());
    addToPayload(buf,n);
#ifdef Q_CC_MSVC
    delete[] buf;
#endif
    return true;
}
