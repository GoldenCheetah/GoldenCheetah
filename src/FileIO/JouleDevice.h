#ifndef JOULEDEVICE_H
#define JOULEDEVICE_H

#include "CommPort.h"
#include "Device.h"
#include "stdint.h"

class DeviceFileInfo;
class JoulePacket;

struct JouleDevices : public Devices
{
    Q_DECLARE_TR_FUNCTIONS(JouleDevices)

    public:
    virtual DevicePtr newDevice( CommPortPtr dev );
    virtual QString downloadInstructions() const;
    virtual bool canCleanup( void ) {return true; };
};

struct JouleDevice : public Device
{
    Q_DECLARE_TR_FUNCTIONS(JouleDevice)

    public:
    JouleDevice( CommPortPtr dev ) :
        Device( dev ) {};

    virtual bool download( const QDir &tmpdir,
                          QList<DeviceDownloadFile> &files,
                          QString &err);

    virtual bool cleanup( QString &err );

    bool getUnitVersion(JoulePacket &response, QString &err);
    bool getSystemInfo(JoulePacket &response, QString &err);

    enum JouleType { JOULE_1_0, JOULE_GPS, JOULE_GPS_PLUS, JOULE_UNKNOWN };
    JouleType getJouleType(JoulePacket &versionResponse);

    bool getUnitFreeSpace(QString &txt, QString &err);
    bool getDownloadableRides(QList<DeviceStoredRideItem> &rides, bool isJouleGPS_GPSPLUS, QString &err);


};

class JoulePacket
{
    Q_DECLARE_TR_FUNCTIONS(JoulePacket)

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
