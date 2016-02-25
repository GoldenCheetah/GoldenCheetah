#ifndef MACRODEVICE_H
#define MACRODEVICE_H

#include "CommPort.h"
#include "Device.h"

class DeviceFileInfo;

struct MacroDevices : public Devices
{
    Q_DECLARE_TR_FUNCTIONS(MacroDevices)

    public:
    virtual DevicePtr newDevice( CommPortPtr dev );
    virtual QString downloadInstructions() const;
    virtual bool canCleanup( void ) {return true; };
};

struct MacroDevice : public Device
{
    Q_DECLARE_TR_FUNCTIONS(MacroDevice)

    public:
    MacroDevice( CommPortPtr dev ) :
        Device( dev ) {};

    virtual bool download( const QDir &tmpdir,
                          QList<DeviceDownloadFile> &files,
                          QString &err);

    virtual bool cleanup( QString &err );
};

class MacroPacket
{
    Q_DECLARE_TR_FUNCTIONS(MacroPacket)
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
