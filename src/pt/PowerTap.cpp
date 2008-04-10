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

#include "D2XX.h"
#include "../lib/pt.h"
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

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

static void
doWrite(DevicePtr dev, char c)
{
    // printf("writing '%c' to device\n", c);
    QString err;
    int n = dev->write(&c, 1, err);
    if (n != 1) {
        if (n < 0)
            printf("ERROR: failed to write %c to device: %s\n",
                   c, err.toAscii().constData());
        else
            printf("ERROR: timeout writing %c to device\n", c);
        exit(1);
    }
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
            /*printf("read %d bytes so far: \"%s\"\n", sofar, 
                   cEscape(buf, sofar).toAscii().constData());*/
            exit(1);
        }
        sofar += n;
    }
    return sofar;
}

typedef void (*StatusCallback)(QString status);

static bool
download(DevicePtr dev, QString &version,
         QVector<unsigned char> &records,
         StatusCallback statusCallback, QString &err)
{
    if (!dev->open(err)) {
        err = "ERROR: open failed: " + err;
        return false;
    } 

    /*
    printf("clearing read buffer\n");
    while (true) {
        QString err;
        char buf[256];
        if (dev->read(buf, sizeof(buf), err) < (int) sizeof(buf))
            break;
    }
    */

    doWrite(dev, 0x56); // 'V'
    statusCallback("Reading version string...");
    char vbuf[256];
    int version_len = readUntilNewline(dev, vbuf, sizeof(vbuf));
    printf("read version \"%s\"\n",
           cEscape(vbuf, version_len).toAscii().constData());
    vbuf[version_len] = '\0';
    version = vbuf;

    statusCallback("Reading header...");

    doWrite(dev, 0x44); // 'D'
    unsigned char header[6];
    int header_len = dev->read(header, sizeof(header), err);
    if (header_len != 6) {
        if (header_len < 0)
            err = "ERROR: reading header: " + err;
        else
            err = "ERROR: timeout reading header";
        return false;
    }
    /*printf("read header \"%s\"\n",
           cEscape((char*) header, 
                   sizeof(header)).toAscii().constData());*/
    for (size_t i = 0; i < sizeof(header); ++i)
        records.append(header[i]);

    statusCallback("Downloading ride data...");

    unsigned recInt = 0;
    double secs = 0.0;

    fflush(stdout);
    while (true) {
        // printf("reading block\n");
        unsigned char buf[256 * 6 + 1];
        int n = dev->read(buf, 2, err);
        if (n < 2) {
            if (n < 0)
                err = "ERROR: reading first two: " + err;
            else
                err = "ERROR: timeout reading first two";
            return false;
        }
        /*printf("read 2 bytes: \"%s\"\n", 
               cEscape((char*) buf, 2).toAscii().constData());*/
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
            /*printf("read %d bytes: \"%s\"\n", n, 
                   cEscape((char*) buf + count, n).toAscii().constData());*/
            count += n;
        }
        unsigned csum = 0;
        for (int i = 0; i < ((int) sizeof(buf)) - 1; ++i) 
            csum += buf[i];
        if ((csum % 256) != buf[sizeof(buf) - 1]) {
            err = "ERROR: bad checksum";
            return false;
        }
        for (size_t i = 0; i < sizeof(buf) - 1; ++i) {
            records.append(buf[i]);
            if (i % 6 == 0) {
                if (pt_is_config(buf + i)) {
                    unsigned unused1, unused2, unused3;
                    pt_unpack_config(buf + i, &unused1, &unused2,
                                     &recInt, &unused3);
                 }
                else if (pt_is_data(buf + i)) {
                    secs += recInt * 1.26;
                }
            }
        }
        int rem = (int) round(secs);
        int min = rem / 60;
        statusCallback(QString("%1 minutes downloaded...").arg(min));
        doWrite(dev, 0x71); // 'q'
    }
    return true;
}

static void
statusCallback(QString status)
{
    printf("STATUS: %s\n", status.toAscii().constData());
}

int
main()
{
    QString err;
    QVector<DevicePtr> devList = Device::listDevices(err);
    if (devList.empty()) {
        printf("ERROR: no devices\n");
        exit(1);
    }
    if (devList.size() > 1) {
        printf("ERROR: found %d devices\n", devList.size());
        exit(1);
    }
    printf("Opening device\n");
    DevicePtr dev = devList[0];
    QString version;
    QVector<unsigned char> records;
    if (!download(dev, version, records, &statusCallback, err)) {
        printf("%s\n", err.toAscii().constData());
        exit(1);
    }
    char tmpname[32];
    snprintf(tmpname, sizeof(tmpname), ".ptdl.XXXXXX");
    int fd = mkstemp(tmpname);
    if (fd == -1)
        assert(false);
    FILE *file = fdopen(fd, "w");
    if (file == NULL)
        assert(false);
    unsigned char *data = records.data();
    struct tm time;
    bool time_set = false;

    for (int i = 0; i < records.size(); i += 6) {
        if (data[i] == 0)
            continue;
        fprintf(file, "%02x %02x %02x %02x %02x %02x\n", 
                data[i], data[i+1], data[i+2],
                data[i+3], data[i+4], data[i+5]); 
        if (!time_set && pt_is_time(data + i)) {
            pt_unpack_time(data + i, &time);
            time_set = true;
        }
    }
    fclose(file);
    assert(time_set);

    if (chmod(tmpname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
        perror("chmod");
        assert(false);
    }

    char filename[64];
    sprintf(filename, "%04d_%02d_%02d_%02d_%02d_%02d.raw", 
            time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, 
            time.tm_hour, time.tm_min, time.tm_sec);

    if (rename(tmpname, filename) == -1) {
        perror("rename");
        assert(false);
    }

    printf("Ride successfully downloaded to %s!\n", filename);
    return 0;
}

