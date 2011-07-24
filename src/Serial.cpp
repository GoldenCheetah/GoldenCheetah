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

#ifdef Q_OS_WIN32

// WIN32 includes
#include <windows.h>
#include <winbase.h>
#else

// Linux and Mac Includes
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
#endif

bool SerialRegistered = CommPort::addListFunction(&Serial::myListCommPorts);

#ifdef Q_OS_WIN32
Serial::Serial(const QString &path) : path(path), isOpen(false)
{
}
#else
Serial::Serial(const QString &path) : path(path), fd(-1) 
{
}
#endif

Serial::~Serial()
{
#ifdef Q_OS_WIN32
    if (isOpen == true) {
        close();
    }
#else
    if (fd >= 0) close();
#endif
}

bool
Serial::open(QString &err)
{
#ifndef Q_OS_WIN32

    //
    // Linux and Mac OSX use stdio / termio / tcsetattr
    //
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
#else

    //
    // Windows uses CreateFile / DCB / SetCommState
    //

    DCB deviceSettings;    // serial port settings baud rate et al
    COMMTIMEOUTS timeouts; // timeout settings on serial ports

    // if deviceFilename references a port above COM9
    // then we need to open "\\.\COMX" not "COMX"
	QString portSpec = "\\\\.\\" + path;
    wchar_t deviceFilenameW[32]; // \\.\COM32 needs 9 characters, 32 should be enough?
    MultiByteToWideChar(CP_ACP, 0, portSpec.toAscii(), -1, (LPWSTR)deviceFilenameW,
                    sizeof(deviceFilenameW));

    // win32 commport API
    fd = CreateFile (deviceFilenameW, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (fd == INVALID_HANDLE_VALUE) return isOpen = false;

    if (GetCommState (fd, &deviceSettings) == false) return isOpen = false;

    // so we've opened the comm port lets set it up for
    deviceSettings.BaudRate = CBR_9600;
    deviceSettings.fParity = NOPARITY;
    deviceSettings.ByteSize = 8;
    deviceSettings.StopBits = ONESTOPBIT;
    deviceSettings.EofChar = 0x0;
    deviceSettings.ErrorChar = 0x0;
    deviceSettings.EvtChar = 0x0;
    deviceSettings.fBinary = TRUE;
    deviceSettings.fRtsControl = 0x0;
    deviceSettings.fOutxCtsFlow = FALSE;


    if (SetCommState(fd, &deviceSettings) == false) {
        CloseHandle(fd);
        return isOpen = false;
    }

    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 5000;
    timeouts.ReadTotalTimeoutMultiplier = 50;
    timeouts.WriteTotalTimeoutConstant = 5000;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(fd, &timeouts);

    return isOpen = true;
#endif
}

void
Serial::close()
{
#ifndef Q_OS_WIN32
    assert(fd >= 0);
    ::close(fd);
    fd = -1;
#else
    if (isOpen == true) {
        CloseHandle(fd);
        isOpen = false;
    }
#endif
}


#ifndef Q_OS_WIN32
//
// On Mac/Linux read/write timeout periods
// use timeval, whilst on Win32 the readfile
// API does this for you.
//
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
#endif

int
Serial::read(void *buf, size_t nbyte, QString &err)
{
#ifndef Q_OS_WIN32
    //
    // Mac and Linux use select and timevals to
    // do non-blocking reads with a timeout
    //
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
#else

    //
    // Win32 API uses readfile
    //           which handles timeouts / ready read
    //
    DWORD cBytes;
    int rc = ReadFile(fd, buf, nbyte, &cBytes, NULL);
    if (rc) return (int)cBytes;
    else return (-1);

#endif
}

int
Serial::write(void *buf, size_t nbyte, QString &err)
{
#ifndef Q_OS_WIN32
    //
    // Max and Linux use select() and write()
    //
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
#else

    //
    // Windows use the writefile WIN32 API
    //
    DWORD cBytes;
    int rc = WriteFile(fd, buf, nbyte, &cBytes, NULL);
    if (!rc) {
        err = QString("write: error %1").arg(rc);
        return -1;
    } else {
        return (int)cBytes; // how much was written?
    }
#endif
}

QString
Serial::name() const
{
    return QString("Serial: ") + path;
}

#ifndef Q_OS_WIN32
//
// Linux and Mac device enumerator matches wildcards in /dev
//
static int
find_devices(char *result[], int capacity)
{
    regex_t reg;
    DIR *dirp;
    struct dirent *dp;
    int count = 0;
    if (regcomp(&reg, 
                "^(cu\\.(PL2303-[0-9A-F]+|SLAB_USBtoUART|usbmodem[0-9A-F]+|usbserial-[0-9A-F]+|KeySerial[0-9])|ttyUSB[0-9]|ttyS[0-2])$",
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
#else
//
// Windows uses WIN32 API to open file to check existence
//
static int
find_devices(char *result[], int capacity)
{
    // it is valid to have up to 255 com devices!
    int count=0;

    for (int i=1; i<256 && count < capacity; i++) {

        // longCOM is in form "\\.\COMn" which is used by the API
        // shortCOM is in form COMn which is familiar to users
        QString shortCOM = QString("COM%1").arg(i);
	    QString longCOM = "\\\\.\\" + shortCOM;
        wchar_t deviceFilenameW[32];
        MultiByteToWideChar(CP_ACP, 0, longCOM.toAscii(), -1, (LPWSTR)deviceFilenameW,
                        sizeof(deviceFilenameW));

        // Try to open the port
        BOOL bSuccess = FALSE;
        HANDLE hPort = CreateFile(deviceFilenameW, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

        if (hPort == INVALID_HANDLE_VALUE) {
            DWORD dwError = GetLastError();
            // Check to see if the error was because some other app had the port open
            // or a general and valid failure

            if (dwError == ERROR_ACCESS_DENIED || dwError == ERROR_GEN_FAILURE ||
                dwError == ERROR_SHARING_VIOLATION || dwError == ERROR_SEM_TIMEOUT)
            bSuccess = TRUE;
        } else {

            // The port was opened successfully
            bSuccess = TRUE;

            // Don't forget to close the port, since we are going to do nothing with it anyway
            CloseHandle(hPort);
        }

        // If success then add to the list
        if (bSuccess) {
            result[count] = (char*) malloc(shortCOM.length() + 1); // include '\0' terminator
            strcpy(result[count], shortCOM.toLatin1().constData());
            count++;
        }
    }
    return count;
}
#endif

QVector<CommPortPtr>
Serial::myListCommPorts(QString &err)
{
    static const int MAX_DEVICES = 100;
    (void) err;
    QVector<CommPortPtr> result;
    char *devices[MAX_DEVICES];
    int devcnt = find_devices(devices, MAX_DEVICES);
    if (devcnt == 0)
        err = "No serial devices found.";
    for (int i = 0; i < devcnt; ++i) {
        result.append(CommPortPtr(new Serial(devices[i])));
        free(devices[i]);
    }
    return result;
}

