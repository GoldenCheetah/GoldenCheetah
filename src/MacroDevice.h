#ifndef MACRODEVICE_H
#define MACRODEVICE_H

#include "CommPort.h"
#include "Device.h"

class DeviceFileInfo;

struct MacroDevice : public Device
{
    Q_DECLARE_TR_FUNCTIONS(MacroDevice)

    virtual QString downloadInstructions() const;

    virtual bool download(CommPortPtr dev, const QDir &tmpdir,
                          QString &tmpname, QString &filename,
                          StatusCallback statusCallback, QString &err);

    virtual void cleanup(CommPortPtr dev);
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
