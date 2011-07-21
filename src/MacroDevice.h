#ifndef MACRODEVICE_H
#define MACRODEVICE_H

#include "CommPort.h"
#include "Device.h"

class DeviceFileInfo;

struct MacroDevices : public Devices
{
    virtual DevicePtr newDevice( CommPortPtr dev );
    virtual QString downloadInstructions() const;
    virtual bool canCleanup( void ) {return true; };
};

struct MacroDevice : public Device
{
    MacroDevice( CommPortPtr dev ) :
        Device( dev ) {};

    virtual bool download( const QDir &tmpdir,
                          QString &tmpname, QString &filename,
                          StatusCallback statusCallback, QString &err);

    virtual void cleanup();
};

class MacroPacket
{
    public:
        MacroPacket();
        MacroPacket(char command);
        void addToPayload(char bites);
        void addToPayload(char *bytes, int len);
        bool verifyCheckSum(CommPortPtr dev, QString &err);
        bool write(CommPortPtr dev, QString &err);
        bool read(CommPortPtr dev, int len, QString &err);

        char* data();
        QByteArray dataArray();

        char command;
        QByteArray payload;
        char checksum;

    private:
};

#endif // MACRODEVICE_H
