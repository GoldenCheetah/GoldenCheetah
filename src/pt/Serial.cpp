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

#include "Serial.h"
#include "../lib/pt.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>

bool SerialRegistered = Device::addListFunction(&Serial::myListDevices);

Serial::Serial(const QString &path) : path(path), fd(-1) 
{
}

Serial::~Serial()
{
    if (fd >= 0)
        close();
}

bool
Serial::open(QString &err)
{
    assert(fd < 0);
    fd = ::open(path.toAscii().constData(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        err = QString("open: ") + strerror(errno);
        return false;
    }
    pt_make_async(fd);
    return true;
}

void
Serial::close()
{
    assert(fd >= 0);
    ::close(fd);
    fd = -1;
}

int
Serial::read(void *buf, size_t nbyte, QString &err)
{
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    struct timeval tv;
    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = 5;
    int n = select(fd + 1, &fdset, NULL, NULL, &tv);
    if (n == 0)
        return 0;
    if (n < 0) {
        err = QString("select: ") + strerror(errno);
        return -1;
    }
    int result = ::read(fd, buf, nbyte);
    if (result < 0)
        err = QString("read: ") + strerror(errno);
    return result;
}

int
Serial::write(void *buf, size_t nbyte, QString &err)
{
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    struct timeval tv;
    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = 5;
    int n = select(fd + 1, NULL, &fdset, NULL, &tv);
    if (n < 0) {
        err = QString("select: ") + strerror(errno);
        return -1;
    }
    if (n == 0) {
        err = "timeout";
        return 0;
    }
    int result = ::write(fd, buf, nbyte);
    if (result < 0)
        err = QString("write: ") + strerror(errno);
    return result;
}

QString
Serial::name() const
{
    return QString("Serial: ") + path;
}

QVector<DevicePtr>
Serial::myListDevices(QString &err)
{
    static const int MAX_DEVICES = 100;
    (void) err;
    QVector<DevicePtr> result;
    char *devices[MAX_DEVICES];
    int devcnt = pt_find_device(devices, MAX_DEVICES);
    for (int i = 0; i < devcnt; ++i) {
        result.append(DevicePtr(new Serial(devices[i])));
        free(devices[i]);
    }
    return result;
}

