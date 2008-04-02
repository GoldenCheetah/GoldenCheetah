
#include <QtCore>
#include <D2XX/ftd2xx.h>
#include <boost/shared_ptr.hpp>
#include <ctype.h>

class Device
{
    Device(const Device &);
    Device& operator=(const Device &);

    FT_DEVICE_LIST_INFO_NODE info;
    FT_HANDLE ftHandle;
    bool isOpen;

    public:

    Device(const FT_DEVICE_LIST_INFO_NODE &info) : info(info), isOpen(false) {}

    ~Device() {
        if (isOpen)
            close();
    }

    bool open(QString &err) {
        assert(!isOpen);
        FT_STATUS ftStatus =
            FT_OpenEx(info.Description, FT_OPEN_BY_DESCRIPTION, &ftHandle);
        if (ftStatus == FT_OK)
            isOpen = true;
        else
            err = QString("FT_Open: %1").arg(ftStatus);
        ftStatus = FT_SetBaudRate(ftHandle, 9600);
        if (ftStatus != FT_OK) {
            err = QString("FT_SetBaudRate: %1").arg(ftStatus);
            close();
        }
        return isOpen;
    }

    void close() {
        assert(isOpen);
        FT_Close(ftHandle); 
        isOpen = false;
    }

    int read(void *buf, size_t nbyte, QString &err) {
        assert(isOpen);
        DWORD rxbytes;
        FT_STATUS ftStatus = FT_GetQueueStatus(ftHandle, &rxbytes);
        if (ftStatus != FT_OK) {
            err = QString("FT_GetQueueStatus: %1").arg(ftStatus);
            return -1;
        }
        // printf("rxbytes=%d\n", (int) rxbytes);
        // Return immediately whenever there's something to read.
        if (rxbytes > 0 && rxbytes < nbyte)
            nbyte = rxbytes;
        if (nbyte > rxbytes)
            FT_SetTimeouts(ftHandle, 5000, 5000);
        DWORD n;
        ftStatus = FT_Read(ftHandle, buf, nbyte, &n);
        if (ftStatus == FT_OK)
            return n;
        err = QString("FT_Read: %1").arg(ftStatus);
        return -1;
    }

    int write(void *buf, size_t nbyte, QString &err) {
        assert(isOpen);
        DWORD n;
        FT_STATUS ftStatus = FT_Write(ftHandle, buf, nbyte, &n);
        if (ftStatus == FT_OK)
            return n;
        err = QString("FT_Write: %1").arg(ftStatus);
        return -1;
    }

    void setTimeout(int millis) {
        assert(isOpen);
        FT_SetTimeouts(ftHandle, millis, millis);
    }
};

typedef boost::shared_ptr<Device> DevicePtr;

static QVector<DevicePtr>
listDevices(QString &err)
{
    QVector<DevicePtr> result;
    DWORD numDevs;
    FT_STATUS ftStatus = FT_CreateDeviceInfoList(&numDevs); 
    if(ftStatus != FT_OK) {
        err = QString("FT_CreateDeviceInfoList: %1").arg(ftStatus); 
        return result;
    }
    FT_DEVICE_LIST_INFO_NODE *devInfo = new FT_DEVICE_LIST_INFO_NODE[numDevs];
    ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs); 
    if (ftStatus != FT_OK)
        err = QString("FT_GetDeviceInfoList: %1").arg(ftStatus); 
    else {
        for (DWORD i = 0; i < numDevs; i++)
            result.append(DevicePtr(new Device(devInfo[i])));
    }
    delete [] devInfo;
    return result;
}

static bool
hasNewline(const char *buf, int len)
{
    static char newline[] = { 0x0d, 0x0a };
    if (len < 2)
        return false;
    for (int i = 0; i < len; ++i) {
        bool success = true;
        for (int j = 0; j < 2; ++j) {
            if (buf[i+j] != newline[j]) {
                success = false;
                break;
            }
        }
        if (success)
            return true;
    }
    return false;
}

static QString
cEscape(const char *buf, int len)
{
    char *result = new char[4 * len + 1];
    char *tmp = result;
    for (int i = 0; i < len; ++i) {
        if (buf[i] == '"')
            tmp += sprintf(tmp, "\\\"");
        else if (isprint(buf[i]))
            *(tmp++) = buf[i];
        else
            tmp += sprintf(tmp, "\\x%02x", 0xff & (unsigned) buf[i]);
    }
    return result;
}

static void
doWrite(DevicePtr dev, char c)
{
    // printf("writing '%c' to device\n", c);
    QString err;
    int n = dev->write(&c, 1, err);
    if (n != 1) {
        if (n < 0)
            printf("ERROR: failed to write %c to device: %s\n",
                   c, err.toAscii().constData());
        else
            printf("ERROR: timeout writing %c to device\n", c);
        exit(1);
    }
}

static int
readUntilNewline(DevicePtr dev, char *buf, int len)
{
    int sofar = 0;
    while (!hasNewline(buf, sofar)) {
        QString err;
        int n = dev->read(buf + sofar, len - sofar, err);
        if (n <= 0) {
            if (n < 0)
                printf("read error: %s\n", err.toAscii().constData());
            else
                printf("read timeout\n");
            /*printf("read %d bytes so far: \"%s\"\n", sofar, 
                   cEscape(buf, sofar).toAscii().constData());*/
            exit(1);
        }
        sofar += n;
    }
    return sofar;
}

int
main()
{
    QString err;
    QVector<DevicePtr> devList = listDevices(err);
    if (devList.empty()) {
        printf("ERROR: no devices\n");
        exit(1);
    }
    if (devList.size() > 1) {
        printf("ERROR: found %d devices\n", devList.size());
        exit(1);
    }
    printf("opening device\n");
    DevicePtr dev = devList[0];
    if (!dev->open(err)) {
        printf("ERROR: open failed: %s\n", err.toAscii().constData());
        exit(1);
    } 

    /*
    printf("clearing read buffer\n");
    while (true) {
        QString err;
        char buf[256];
        if (dev->read(buf, sizeof(buf), err) < (int) sizeof(buf))
            break;
    }
    */

    doWrite(dev, 0x56); // 'V'
    printf("reading version string\n");
    char version[256];
    int version_len = readUntilNewline(dev, version, sizeof(version));
    printf("read version \"%s\"\n",
           cEscape(version, version_len).toAscii().constData());

    printf("reading header\n");
    doWrite(dev, 0x44); // 'D'
    unsigned char header[6];
    int header_len = dev->read(header, sizeof(header), err);
    if (header_len != 6) {
        if (header_len < 0)
            printf("ERROR: reading header: %s\n", err.toAscii().constData());
        else
            printf("ERROR: timeout reading header\n");
        exit(1);
    }
    /*printf("read header \"%s\"\n",
           cEscape((char*) header, 
                   sizeof(header)).toAscii().constData());*/

    printf("reading blocks...");
    fflush(stdout);
    while (true) {
        // printf("reading block\n");
        unsigned char buf[256 * 6 + 1];
        int n = dev->read(buf, 2, err);
        if (n < 2) {
            if (n < 0)
                printf("\nERROR: reading first two: %s\n",
                       err.toAscii().constData());
            else
                printf("\nERROR: timeout reading first two\n");
            exit(1);
        }
        /*printf("read 2 bytes: \"%s\"\n", 
               cEscape((char*) buf, 2).toAscii().constData());*/
        if (hasNewline((char*) buf, 2))
            break;
        unsigned count = 2;
        while (count < sizeof(buf)) {
            n = dev->read(buf + count, sizeof(buf) - count, err);
            if (n < 0) {
                printf("\nERROR: reading block: %s\n", err.toAscii().constData());
                exit(1);
            }
            if (n == 0) {
                printf("\nERROR: timeout reading block\n");
                exit(1);
            }
            /*printf("read %d bytes: \"%s\"\n", n, 
                   cEscape((char*) buf + count, n).toAscii().constData());*/
            count += n;
        }
        unsigned csum = 0;
        for (int i = 0; i < ((int) sizeof(buf)) - 1; ++i) 
            csum += buf[i];
        if ((csum % 256) != buf[sizeof(buf) - 1]) {
            printf("\nERROR: bad checksum\n");
            exit(1);
        }
        printf(".");
        fflush(stdout);
        doWrite(dev, 0x71); // 'q'
    }
    printf("done\n");
    printf("download complete\n");
    return 0;
}

