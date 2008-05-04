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
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

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
    struct termios tty;
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        assert(0);
    }
    if (tcgetattr(fd, &tty) == -1) {
        perror("tcgetattr");
        assert(0);
    }
    tty.c_cflag &= ~CRTSCTS; /* no hardware flow control */
    tty.c_cflag &= ~(PARENB | PARODD); /* no parity */
    tty.c_cflag &= ~CSTOPB; /* 1 stop bit */
    tty.c_cflag &= ~CSIZE; /* clear size bits */
    tty.c_cflag |= CS8; /* 8 bits */
    tty.c_cflag |= CLOCAL | CREAD; /* ignore modem control lines */
    if (cfsetspeed(&tty, B9600) == -1) {
        perror("cfsetspeed");
        assert(0);
    }
    tty.c_iflag = IGNBRK; /* ignore BREAK condition on input */
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1; /* all reads return at least one character */
    if (tcsetattr(fd, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        assert(0);
    }
    return true;
}

void
Serial::close()
{
    assert(fd >= 0);
    ::close(fd);
    fd = -1;
}

static struct timeval &
operator-=(struct timeval &left, const struct timeval &right)
{
    if (left.tv_usec < right.tv_usec) {
        left.tv_sec -= 1;
        left.tv_usec += 1000000;
    }
    left.tv_sec -= right.tv_sec;
    left.tv_usec -= right.tv_usec;
    return left;
}

static bool
operator<(struct timeval &left, const struct timeval &right)
{
    if (left.tv_sec < right.tv_sec)
        return true;
    if (left.tv_sec > right.tv_sec)
        return false;
    return left.tv_usec < right.tv_usec;
}

int
Serial::read(void *buf, size_t nbyte, QString &err)
{
    assert(fd >= 0);
    assert(nbyte > 0);
    struct timeval start;
    if (gettimeofday(&start, NULL) < 0)
        assert(false);
    struct timeval stop = start;
    stop.tv_sec += 5;
    size_t count = 0;
    while (start < stop) {
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);
        struct timeval remaining = stop;
        remaining -= start;
        int n = select(fd + 1, &fdset, NULL, NULL, &remaining);
        if (n == 0)
            break;
        if (n < 0) {
            err = QString("select: ") + strerror(errno);
            return -1;
        }
        assert(nbyte > count);
        int result = ::read(fd, ((char*) buf) + count, nbyte - count);
        if (result < 0)
            err = QString("read: ") + strerror(errno);
        assert(result > 0); // else why did select return > 0?
        count += result;
        assert(count <= nbyte);
        if (count == nbyte)
            break;
        if (gettimeofday(&start, NULL) < 0)
            assert(false);
    }
    return count;
}

int
Serial::write(void *buf, size_t nbyte, QString &err)
{
    assert(fd >= 0);
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

static int
find_devices(char *result[], int capacity)
{
    regex_t reg;
    DIR *dirp;
    struct dirent *dp;
    int count = 0;
    if (regcomp(&reg, 
                "^(cu\\.(usbmodem[0-9A-F]+|usbserial-[0-9A-F]+|KeySerial[0-9])|ttyUSB[0-9]|ttyS[0-2])$", 
                REG_EXTENDED|REG_NOSUB)) {
        assert(0);
    }
    dirp = opendir("/dev");
    while ((count < capacity) && ((dp = readdir(dirp)) != NULL)) {
        if (regexec(&reg, dp->d_name, 0, NULL, 0) == 0) {
            result[count] = (char*) malloc(6 + strlen(dp->d_name));
            sprintf(result[count], "/dev/%s", dp->d_name);
            ++count;
        }
    }
    return count;
}

QVector<DevicePtr>
Serial::myListDevices(QString &err)
{
    static const int MAX_DEVICES = 100;
    (void) err;
    QVector<DevicePtr> result;
    char *devices[MAX_DEVICES];
    int devcnt = find_devices(devices, MAX_DEVICES);
    for (int i = 0; i < devcnt; ++i) {
        result.append(DevicePtr(new Serial(devices[i])));
        free(devices[i]);
    }
    return result;
}

