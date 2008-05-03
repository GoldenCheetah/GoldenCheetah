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

#include "PowerTap.h"
#include "../lib/pt.h"
#include <math.h>

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

static bool
doWrite(DevicePtr dev, char c, bool hwecho, QString &err)
{
    printf("writing '%c' to device\n", c);
    int n = dev->write(&c, 1, err);
    if (n != 1) {
        if (n < 0)
            err = QString("failed to write %1 to device: %2").arg(c).arg(err);
        else
            err = QString("timeout writing %1 to device").arg(c);
        return false;
    }
    if (hwecho) {
        char c;
        int n = dev->read(&c, 1, err);
        if (n != 1) {
            if (n < 0)
                err = QString("failed to read back hardware echo: %2").arg(err);
            else
                err = "timeout reading back hardware echo";
            return false;
        }
    }
    return true;
}

static int
readUntilNewline(DevicePtr dev, char *buf, int len)
{
    int sofar = 0;
    while (!hasNewline(buf, sofar)) {
        QString err;
        int n = dev->read(buf + sofar, len - sofar, err);
        if (n <= 0) {
            if (n < 0)
                printf("read error: %s\n", err.toAscii().constData());
            else
                printf("read timeout\n");
            printf("read %d bytes so far: \"%s\"\n", sofar, 
                   cEscape(buf, sofar).toAscii().constData());
            exit(1);
        }
        sofar += n;
    }
    return sofar;
}

bool
PowerTap::download(DevicePtr dev, QByteArray &version,
                   QVector<unsigned char> &records,
                   StatusCallback statusCallback, QString &err)
{
    if (!dev->open(err)) {
        err = "ERROR: open failed: " + err;
        return false;
    } 

    if (!doWrite(dev, 0x56, false, err)) // 'V'
        return false;
    if (!statusCallback(STATE_READING_VERSION)) {
        err = "download cancelled";
        return false;
    }
    char vbuf[256];
    int version_len = readUntilNewline(dev, vbuf, sizeof(vbuf));
    printf("read version \"%s\"\n",
           cEscape(vbuf, version_len).toAscii().constData());
    version = QByteArray(vbuf, version_len);

    // We expect the version string to be something like 
    // "VER 02.21 PRO...", so if we see two V's, it's probably 
    // because there's a hardware echo going on.

    int veridx = version.indexOf("VER");
    if (veridx < 0) {
        err = QString("Unrecognized version \"%1\"")
                .arg(cEscape(vbuf, version_len));
        return false;
    }
    bool hwecho = version.indexOf('V') < veridx;
    printf("hwecho=%s\n", hwecho ? "true" : "false");

    if (!statusCallback(STATE_READING_HEADER)) {
        err = "download cancelled";
        return false;
    }

    if (!doWrite(dev, 0x44, hwecho, err)) // 'D'
        return false;
    unsigned char header[6];
    int header_len = dev->read(header, sizeof(header), err);
    if (header_len != 6) {
        if (header_len < 0)
            err = "ERROR: reading header: " + err;
        else
            err = "ERROR: timeout reading header";
        return false;
    }
    printf("read header \"%s\"\n",
           cEscape((char*) header, 
                   sizeof(header)).toAscii().constData());
    for (size_t i = 0; i < sizeof(header); ++i)
        records.append(header[i]);

    if (!statusCallback(STATE_READING_DATA)) {
        err = "download cancelled";
        return false;
    }

    fflush(stdout);
    while (true) {
        printf("reading block\n");
        unsigned char buf[256 * 6 + 1];
        int n = dev->read(buf, 2, err);
        if (n < 2) {
            if (n < 0)
                err = "ERROR: reading first two: " + err;
            else
                err = "ERROR: timeout reading first two";
            return false;
        }
        printf("read 2 bytes: \"%s\"\n", 
               cEscape((char*) buf, 2).toAscii().constData());
        if (hasNewline((char*) buf, 2))
            break;
        unsigned count = 2;
        while (count < sizeof(buf)) {
            n = dev->read(buf + count, sizeof(buf) - count, err);
            if (n < 0) {
                err = "ERROR: reading block: " + err;
                return false;
            }
            if (n == 0) {
                err = "ERROR: timeout reading block";
                return false;
            }
            printf("read %d bytes: \"%s\"\n", n, 
                   cEscape((char*) buf + count, n).toAscii().constData());
            count += n;
        }
        unsigned csum = 0;
        for (int i = 0; i < ((int) sizeof(buf)) - 1; ++i) 
            csum += buf[i];
        if ((csum % 256) != buf[sizeof(buf) - 1]) {
            err = "ERROR: bad checksum";
            return false;
        }
        printf("good checksum\n");
        for (size_t i = 0; i < sizeof(buf) - 1; ++i)
            records.append(buf[i]);
        if (!statusCallback(STATE_DATA_AVAILABLE)) {
            err = "download cancelled";
            return false;
        }
        if (!doWrite(dev, 0x71, hwecho, err)) // 'q'
            return false;
    }
    return true;
}


