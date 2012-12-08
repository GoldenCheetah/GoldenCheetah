#ifndef JOULEDEVICE_H
#define JOULEDEVICE_H

#include "CommPort.h"
#include "Device.h"

class DeviceFileInfo;
class JoulePacket;

struct JouleDevices : public Devices
{
    Q_DECLARE_TR_FUNCTIONS(JouleDevices)
    virtual DevicePtr newDevice( CommPortPtr dev, Device::StatusCallback cb );
    virtual QString downloadInstructions() const;
    virtual bool canCleanup( void ) {return true; };
};

struct JouleDevice : public Device
{
    JouleDevice( CommPortPtr dev, StatusCallback cb ) :
        Device( dev, cb ) {};

    virtual bool download( const QDir &tmpdir,
                          QList<DeviceDownloadFile> &files,
                          CancelCallback cancelCallback,
                          ProgressCallback progressCallback,
                          QString &err);

    virtual bool cleanup( QString &err );

    bool getUnitVersion(JoulePacket &response, QString &err);
    bool getSystemInfo(JoulePacket &response, QString &err);

    bool getJouleGPS(JoulePacket &versionResponse);

    bool getUnitFreeSpace(QString &txt, QString &err);
    bool getDownloadableRides(QList<DeviceStoredRideItem> &rides, bool isJouleGPS, QString &err);

};

class JoulePacket
{
    public:
        JoulePacket();
        JoulePacket(uint16_t command);
        void addToPayload(char bites);
        void addToPayload(uint16_t bytes);
        void addToPayload(char *bytes, int len);
        bool verifyCheckSum(CommPortPtr dev, QString &err);
        bool write(CommPortPtr dev, QString &err);
        bool read(CommPortPtr dev, QString &err);

        //char* data();
        QByteArray dataArray();
        QByteArray dataArrayForUnit();

        uint16_t startpattern;
        uint16_t command;
        uint16_t length;

        QByteArray payload;
        char checksum;

    private:
};

#endif // JOULEDEVICE_H
