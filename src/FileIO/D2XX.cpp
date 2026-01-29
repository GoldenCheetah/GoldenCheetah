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
#include <dlfcn.h>

// D2XXWrapper is a wrapper around libftd2xx to make it amenable to loading
// with dlopen().

#define LOAD_SYM(type,var,name) \
    var = (type*) dlsym(handle, name); \
    if (!var) { \
        error = QString("could not load symbol ") + name; \
        return false; \
    }
#ifdef WIN32
#define WIN32_STDCALL __stdcall
#else
#define WIN32_STDCALL
#endif

typedef FT_STATUS WIN32_STDCALL FP_OpenEx(PVOID pArg1, DWORD Flags, FT_HANDLE *pHandle);
typedef FT_STATUS WIN32_STDCALL FP_Close(FT_HANDLE ftHandle);
typedef FT_STATUS WIN32_STDCALL FP_SetBaudRate(FT_HANDLE ftHandle, ULONG BaudRate);
typedef FT_STATUS WIN32_STDCALL FP_SetDataCharacteristics(FT_HANDLE ftHandle, UCHAR WordLength, UCHAR StopBits, UCHAR Parity);
typedef FT_STATUS WIN32_STDCALL FP_SetFlowControl(FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar);
typedef FT_STATUS WIN32_STDCALL FP_GetQueueStatus(FT_HANDLE ftHandle, DWORD *dwRxBytes);
typedef FT_STATUS WIN32_STDCALL FP_SetTimeouts(FT_HANDLE ftHandle, ULONG ReadTimeout, ULONG WriteTimeout);
typedef FT_STATUS WIN32_STDCALL FP_Read(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesReturned);
typedef FT_STATUS WIN32_STDCALL FP_Write(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesWritten);
typedef FT_STATUS WIN32_STDCALL FP_CreateDeviceInfoList(LPDWORD lpdwNumDevs);
typedef FT_STATUS WIN32_STDCALL FP_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE *pDest, LPDWORD lpdwNumDevs);

struct D2XXWrapper {
    void *handle;
    FP_OpenEx *open_ex;
    FP_Close *close;
    FP_SetBaudRate *set_baud_rate;
    FP_SetDataCharacteristics *set_data_characteristics;
    FP_SetFlowControl *set_flow_control;
    FP_GetQueueStatus *get_queue_status;
    FP_SetTimeouts *set_timeouts;
    FP_Read *read;
    FP_Write *write;
    FP_CreateDeviceInfoList *create_device_info_list;
    FP_GetDeviceInfoList *get_device_info_list;
    D2XXWrapper() : handle(NULL) {}
    ~D2XXWrapper() { if (handle) dlclose(handle); }
    bool init(QString &error)
    {
        #if defined(Q_OS_LINUX)
            const char *libname = "libftd2xx.so";
        #elif defined(Q_OS_WIN32)
            const char *libname = "ftd2xx.dll";
        #elif defined(Q_OS_DARWIN)
            const char *libname = "libftd2xx.dylib";
        #endif
        handle = dlopen(libname, RTLD_NOW);
        if (!handle) {
            error = QString("Couldn't load library ") + libname + ".";
            return false;
        }
        LOAD_SYM(FP_OpenEx, open_ex, "FT_OpenEx");
        LOAD_SYM(FP_Close, close, "FT_Close");
        LOAD_SYM(FP_SetBaudRate, set_baud_rate, "FT_SetBaudRate");
        LOAD_SYM(FP_SetDataCharacteristics, set_data_characteristics, "FT_SetDataCharacteristics");
        LOAD_SYM(FP_SetFlowControl, set_flow_control, "FT_SetFlowControl");
        LOAD_SYM(FP_GetQueueStatus, get_queue_status, "FT_GetQueueStatus");
        LOAD_SYM(FP_SetTimeouts, set_timeouts, "FT_SetTimeouts");
        LOAD_SYM(FP_Read, read, "FT_Read");
        LOAD_SYM(FP_Write, write, "FT_Write");
        LOAD_SYM(FP_CreateDeviceInfoList, create_device_info_list, "FT_CreateDeviceInfoList");
        LOAD_SYM(FP_GetDeviceInfoList, get_device_info_list, "FT_GetDeviceInfoList");
        return true;
    }
};

static D2XXWrapper *lib; // singleton lib instance

bool D2XXRegistered = CommPort::addListFunction(&D2XX::myListCommPorts);

D2XX::D2XX(const FT_DEVICE_LIST_INFO_NODE &info) :
    CommPort( "D2XX" ), info(info), _isOpen(false)
{
}

D2XX::~D2XX()
{
    if (_isOpen)
        close();
}

bool
D2XX::isOpen()
{
    return _isOpen;
}

bool
D2XX::open(QString &err)
{
    FT_STATUS ftStatus =
        lib->open_ex(info.Description, FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus != FT_OK) {
        err = QString("FT_Open: %1").arg(ftStatus);
        return false;
    }
    _isOpen = true;
    ftStatus = lib->set_baud_rate(ftHandle, 9600);
    if (ftStatus != FT_OK) {
        err = QString("FT_SetBaudRate: %1").arg(ftStatus);
        close();
    }

    ftStatus = lib->set_data_characteristics(ftHandle,FT_BITS_8,FT_STOP_BITS_1,
                                             FT_PARITY_NONE);
    if (ftStatus != FT_OK) {
        err = QString("FT_SetDataCharacteristics: %1").arg(ftStatus);
        close();
    }

    ftStatus = lib->set_flow_control(ftHandle,FT_FLOW_NONE,
                                     '0','0'); //the 0's are ignored
    if (ftStatus != FT_OK) {
        err = QString("FT_SetFlowControl: %1").arg(ftStatus);
        close();
    }

    return true;
}

void
D2XX::close()
{
    lib->close(ftHandle);
    _isOpen = false;
}

int
D2XX::read(void *buf, size_t nbyte, QString &err)
{
    DWORD rxbytes;
    FT_STATUS ftStatus = lib->get_queue_status(ftHandle, &rxbytes);
    if (ftStatus != FT_OK) {
        err = QString("FT_GetQueueStatus: %1").arg(ftStatus);
        return -1;
    }
    // printf("rxbytes=%d\n", (int) rxbytes);
    // Return immediately whenever there's something to read.
    if (rxbytes > 0 && rxbytes < nbyte)
        nbyte = rxbytes;
    if (nbyte > rxbytes)
        lib->set_timeouts(ftHandle, 5000, 5000);
    DWORD n;
    ftStatus = lib->read(ftHandle, buf, static_cast<DWORD>(nbyte), &n);
    if (ftStatus == FT_OK)
        return n;
    err = QString("FT_Read: %1").arg(ftStatus);
    return -1;
}

int
D2XX::write(void *buf, size_t nbyte, QString &err)
{
    DWORD n;
    FT_STATUS ftStatus = lib->write(ftHandle, buf, static_cast<DWORD>(nbyte), &n);
    if (ftStatus == FT_OK)
        return n;
    err = QString("FT_Write: %1").arg(ftStatus);
    return -1;
}

QString
D2XX::name() const
{
    return info.Description;
}

bool
D2XX::setBaudRate(int speed, QString &err)
{
    FT_STATUS ftStatus = lib->set_baud_rate(ftHandle, speed);
    if (ftStatus != FT_OK) {
        err = QString("FT_SetBaudRate: %1").arg(ftStatus);
        return false;
    }

    return true;
}

QVector<CommPortPtr>
D2XX::myListCommPorts(QString &err)
{
    QVector<CommPortPtr> result;
    if (!lib) {
        lib = new D2XXWrapper;
        if (!lib->init(err)) {
            delete lib;
            lib = NULL;
            return result;
        }
    }
    DWORD numDevs;
    FT_STATUS ftStatus = lib->create_device_info_list(&numDevs);
    if(ftStatus != FT_OK) {
        err = QString("FT_CreateDeviceInfoList: %1").arg(ftStatus);
        return result;
    }
    FT_DEVICE_LIST_INFO_NODE *devInfo = new FT_DEVICE_LIST_INFO_NODE[numDevs];
    ftStatus = lib->get_device_info_list(devInfo, &numDevs);
    if (ftStatus != FT_OK)
        err = QString("FT_GetDeviceInfoList: %1").arg(ftStatus);
    else {
        for (DWORD i = 0; i < numDevs; i++)
            result.append(CommPortPtr(new D2XX(devInfo[i])));
    }
    delete [] devInfo;
    // If we can't open a D2XX device, it's usually because the VCP drivers
    // are installed, so it should also show up in the list of serial devices.
    for (int i = 0; i < result.size(); ++i) {
        CommPortPtr dev = result[i];
        QString tmp;
        if (dev->open(tmp))
            dev->close();
        else
            result.remove(i--);
    }
    return result;
}

