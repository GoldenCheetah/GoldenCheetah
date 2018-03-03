/*
 * Copyright (c) 2016 Antonius Riha (antoniusriha@gmail.com)
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

#ifndef gc_LibUsbLib_h
#define gc_LibUsbLib_h

#include <QVector>


//-----------------------------------------------------------------------------
// UsbDeviceHandle
//
class UsbDeviceHandle
{
    friend class UsbDevice;

public:
    UsbDeviceHandle();
    ~UsbDeviceHandle();

    void clearHalt(int endpoint);
    void releaseInterface(int interfaceNumber);
    void reset();
    int bulkRead(int endpoint, char *bytes, int size, int *actualSize, int timeout);
    int bulkWrite(int endpoint, char *bytes, int size, int *actualSize, int timeout);
    int interruptWrite(int endpoint, char *bytes, int size, int *actualSize, int timeout);
    int setConfiguration(int configuration);
    int claimInterface(int interfaceNumber);
    int setAltInterface(int interfaceNumber, int altSetting);
    void loadRam(const char *path);

#ifdef Q_OS_LINUX
    void detachKernelDriver(int interfaceNumber);
#endif

private:
    UsbDeviceHandle(const UsbDeviceHandle&);
    UsbDeviceHandle& operator=(const UsbDeviceHandle&);

    class Impl;
    Impl *impl;
};
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// UsbDeviceInterface
//
class UsbDeviceInterface
{
    friend class UsbDevice;

public:
    UsbDeviceInterface();
    ~UsbDeviceInterface();

    int interfaceNumber() const;
    int alternateSetting() const;
    int readEndpoint() const;
    int writeEndpoint() const;

private:
    UsbDeviceInterface(const UsbDeviceInterface&);
    UsbDeviceInterface& operator=(const UsbDeviceInterface&);

    class Impl;
    Impl *impl;
};
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// UsbDevice
//
class LibUsbLib;

class UsbDevice
{
    friend class LibUsbLib;

public:
    UsbDevice();
    ~UsbDevice();

    int vendorId() const;
    int productId() const;
    int busNumber() const;
    int deviceAddress() const;

    UsbDeviceInterface* getInterface();
    UsbDeviceHandle* open();

private:
    UsbDevice(const UsbDevice&);
    UsbDevice& operator=(const UsbDevice&);

    class Impl;
    Impl *impl;
};
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LibUsbLib
//
class LibUsbLib
{
public:
    LibUsbLib();
    ~LibUsbLib();

    void initialize(int logLevel);
    void findDevices();
    bool getDevices(QVector<UsbDevice *> &deviceList);
    const char* getErrorMessage(int errorCode);
    
#ifdef WIN32
    bool isLibUsbInstalled() const;
#endif

private:
    LibUsbLib(const LibUsbLib&);
    LibUsbLib& operator=(const LibUsbLib&);

    class Impl;
    Impl *impl;
};
//-----------------------------------------------------------------------------

#endif // gc_LibUsbLib_h
