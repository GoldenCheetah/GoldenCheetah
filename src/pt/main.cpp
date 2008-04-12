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
#include <sys/types.h>
#include <sys/stat.h>

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
    printf("Available devices:\n");
    for (int i = 0; i < devList.size(); ++i) {
        DevicePtr dev = devList[i];
        printf("  %s\n", dev->name().toAscii().constData());
    }
    if (devList.size() > 1) {
        printf("ERROR: found %d devices\n", devList.size());
        exit(1);
    }
    DevicePtr dev = devList[0];
    printf("Downloading from device %s\n", dev->name().toAscii().constData());
    QByteArray version;
    QVector<unsigned char> records;
    if (!PowerTap::download(dev, version, records, &statusCallback, err)) {
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

