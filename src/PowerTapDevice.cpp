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

#include "PowerTapDevice.h"
#include "PowerTapUtil.h"
#include <math.h>

#define PT_DEBUG false

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
doWrite(CommPortPtr dev, char c, bool hwecho, QString &err)
{
    if (PT_DEBUG) printf("writing '%c' to device\n", c);
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
readUntilNewline(CommPortPtr dev, char *buf, int len, QString &err)
{
    int sofar = 0;
    while (!hasNewline(buf, sofar)) {
        assert(sofar < len);
        // Read one byte at a time to avoid waiting for timeout.
        int n = dev->read(buf + sofar, 1, err);
        if (n <= 0) {
            err = (n < 0) ? ("read error: " + err) : "read timeout";
            err += QString(", read %1 bytes so far: \"%2\"")
                .arg(sofar).arg(cEscape(buf, sofar));
            return -1;
        }
        sofar += n;
    }
    return sofar;
}

bool
PowerTapDevice::download(CommPortPtr dev, QByteArray &version,
                         QVector<unsigned char> &records,
                         StatusCallback statusCallback, QString &err)
{
    if (!dev->open(err)) {
        err = "ERROR: open failed: " + err;
        return false;
    } 

    if (!doWrite(dev, 0x56, false, err)) // 'V'
        return false;

    QString cbtext = "Reading version...";
    if (!statusCallback(cbtext)) {
        err = "download cancelled";
        return false;
    }
    char vbuf[256];
    int version_len = readUntilNewline(dev, vbuf, sizeof(vbuf), err);
    if (version_len < 0) {
        err = "Error reading version: " + err;
        return false;
    }
    if (PT_DEBUG) {
        printf("read version \"%s\"\n",
               cEscape(vbuf, version_len).toAscii().constData());
    }
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
    if (PT_DEBUG) printf("hwecho=%s\n", hwecho ? "true" : "false");

    cbtext += "done.\nReading header...";
    if (!statusCallback(cbtext)) {
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
    if (PT_DEBUG) {
        printf("read header \"%s\"\n",
               cEscape((char*) header, 
                       sizeof(header)).toAscii().constData());
    }
    for (size_t i = 0; i < sizeof(header); ++i)
        records.append(header[i]);

    cbtext += "done.\nReading ride data...\n";
    if (!statusCallback(cbtext)) {
        err = "download cancelled";
        return false;
    }
    int cbtextlen = cbtext.length();
    double recIntSecs = 0.0;

    fflush(stdout);
    while (true) {
        if (PT_DEBUG) printf("reading block\n");
        unsigned char buf[256 * 6 + 1];
        int n = dev->read(buf, 2, err);
        if (n < 2) {
            if (n < 0)
                err = "ERROR: reading first two: " + err;
            else
                err = "ERROR: timeout reading first two";
            return false;
        }
        if (PT_DEBUG) {
            printf("read 2 bytes: \"%s\"\n", 
                   cEscape((char*) buf, 2).toAscii().constData());
        }
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
            if (PT_DEBUG) {
                printf("read %d bytes: \"%s\"\n", n, 
                       cEscape((char*) buf + count, n).toAscii().constData());
            }
            count += n;
        }
        unsigned csum = 0;
        for (int i = 0; i < ((int) sizeof(buf)) - 1; ++i) 
            csum += buf[i];
        if ((csum % 256) != buf[sizeof(buf) - 1]) {
            err = "ERROR: bad checksum";
            return false;
        }
        if (PT_DEBUG) printf("good checksum\n");
        for (size_t i = 0; i < sizeof(buf) - 1; ++i)
            records.append(buf[i]);
        if (recIntSecs == 0.0) {
            unsigned char *data = records.data();
            bool bIsVer81 = PowerTapUtil::is_Ver81(data);
            for (int i = 0; i < records.size(); i += 6) {
                if (PowerTapUtil::is_config(data + i, bIsVer81)) {
                    unsigned unused1, unused2, unused3;
                    PowerTapUtil::unpack_config(
                        data + i, &unused1, &unused2,
                        &recIntSecs, &unused3, bIsVer81);
                }
            }
        }
        if (recIntSecs != 0.0) {
            int min = (int) round(records.size() / 6 * recIntSecs);
            cbtext.chop(cbtext.size() - cbtextlen);
            cbtext.append(QString("Ride data read: %1:%2").arg(min / 60)
                            .arg(min % 60, 2, 10, QLatin1Char('0')));
        }
        if (!statusCallback(cbtext)) {
            err = "download cancelled";
            return false;
        }
        if (!doWrite(dev, 0x71, hwecho, err)) // 'q'
            return false;
    }
    return true;
}

