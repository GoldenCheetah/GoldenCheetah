#ifndef JOULEDEVICE_H
#define JOULEDEVICE_H

#include "CommPort.h"
#include "Device.h"
#include "stdint.h"

class DeviceFileInfo;

struct MoxyDevices : public Devices
{
    Q_DECLARE_TR_FUNCTIONS(MoxyDevices)

    public:
    virtual DevicePtr newDevice( CommPortPtr dev );
    virtual QString downloadInstructions() const;
    virtual bool canCleanup( void ) {return true; };
    virtual bool canPreview() { return false; }; // Moxy is dumb ass
};

struct MoxyDevice : public Device
{
    Q_DECLARE_TR_FUNCTIONS(MoxyDevice)

    public:
    MoxyDevice( CommPortPtr dev ) : Device( dev ) {};

    // io with the moxy is text based lines
    // they even give you a prompt ">" when
    // the command completes
    int readUntilPrompt(CommPortPtr dev, char *buf, int len, QString &err);
    int readUntilNewline(CommPortPtr dev, char *buf, int len, QString &err);
    int readData(CommPortPtr dev, char *buf, int len, QString &err);

    bool writeCommand(CommPortPtr dev, const char *command, QString &err);

    virtual bool download( const QDir &tmpdir,
                          QList<DeviceDownloadFile> &files,
                          QString &err);

    virtual bool cleanup( QString &err );

};

#endif // JOULEDEVICE_H
