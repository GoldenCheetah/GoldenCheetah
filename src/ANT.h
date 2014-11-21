/*
 * Copyright (c) 2009 Mark Rages
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef gc_ANT_h
#define gc_ANT_h

//
// GC specific stuff
//
#include "GoldenCheetah.h"
#include "RealtimeData.h"
#include "DeviceConfiguration.h"

//
// QT stuff
//
#include <QThread>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QProgressDialog>
#include <QFile>

//
// Time
//
#include <sys/time.h>

//
// Serial i/o stuff
//
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include <stdint.h> //uint8_t

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#include "USBXpress.h" // for Garmin USB1 sticks
#else
#include <termios.h> // unix!!
#include <unistd.h> // unix!!
#include <sys/ioctl.h>
#ifndef N_TTY // for OpenBSD
#define N_TTY 0
#endif
#endif

#if defined GC_HAVE_LIBUSB
#include "LibUsb.h"    // for Garmin USB2 sticks
#endif

#include <QDebug>

#include "Settings.h" // for wheel size config

// timeouts for read/write of serial port in ms
#define ANT_READTIMEOUT    1000
#define ANT_WRITETIMEOUT   2000

class ANTMessage;
class ANTChannel;

typedef struct ant_sensor_type {
  bool user; // can user select this when calibrating ?
  int type;
  int period;
  int device_id;
  int frequency;
  int network;
  const char *descriptive_name;
  char suffix;
  const char *iconname;

} ant_sensor_type_t;

#define DEFAULT_NETWORK_NUMBER 0
#define ANT_SPORT_NETWORK_NUMBER 1
#define RX_BURST_DATA_LEN 128

static inline double get_timestamp( void ) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1.0 + tv.tv_usec * 1.0e-6;
}


struct setChannelAtom {
    setChannelAtom() : channel(0), device_number(0), channel_type(0) {}
    setChannelAtom(int x, int y, int z) : channel(x), device_number(y), channel_type(z) {}

    int channel;
    int device_number;
    int channel_type;
};

//======================================================================
// ANT Constants
//======================================================================

#include "ANTMessages.h"

// ANT constants
#define ANT_MAX_DATA_MESSAGE_SIZE    8

// ANT Sport Power Broadcast message types
#define ANT_STANDARD_POWER     0x10
#define ANT_WHEELTORQUE_POWER  0x11
#define ANT_CRANKTORQUE_POWER  0x12
#define ANT_CRANKSRM_POWER     0x20

// ANT messages
#define ANT_UNASSIGN_CHANNEL   0x41
#define ANT_ASSIGN_CHANNEL     0x42
#define ANT_CHANNEL_ID         0x51
#define ANT_CHANNEL_PERIOD     0x43
#define ANT_SEARCH_TIMEOUT     0x44
#define ANT_CHANNEL_FREQUENCY  0x45
#define ANT_SET_NETWORK        0x46
#define ANT_TX_POWER           0x47
#define ANT_ID_LIST_ADD        0x59
#define ANT_ID_LIST_CONFIG     0x5A
#define ANT_CHANNEL_TX_POWER   0x60
#define ANT_LP_SEARCH_TIMEOUT  0x63
#define ANT_SET_SERIAL_NUMBER  0x65
#define ANT_ENABLE_EXT_MSGS    0x66
#define ANT_ENABLE_LED         0x68
#define ANT_SYSTEM_RESET       0x4A
#define ANT_OPEN_CHANNEL       0x4B
#define ANT_CLOSE_CHANNEL      0x4C
#define ANT_OPEN_RX_SCAN_CH    0x5B
#define ANT_REQ_MESSAGE        0x4D
#define ANT_BROADCAST_DATA     0x4E
#define ANT_ACK_DATA           0x4F
#define ANT_BURST_DATA         0x50
#define ANT_CHANNEL_EVENT      0x40
#define ANT_CHANNEL_STATUS     0x52
#define ANT_CHANNEL_ID         0x51
#define ANT_VERSION            0x3E
#define ANT_CAPABILITIES       0x54
#define ANT_SERIAL_NUMBER      0x61
#define ANT_CW_INIT            0x53
#define ANT_CW_TEST            0x48

#define TRANSITION_START       0x00 // start of transition when opening

// ANT message structure.
#define ANT_OFFSET_SYNC            0
#define ANT_OFFSET_LENGTH          1
#define ANT_OFFSET_ID              2
#define ANT_OFFSET_DATA            3
#define ANT_OFFSET_CHANNEL_NUMBER  3
#define ANT_OFFSET_MESSAGE_ID      4
#define ANT_OFFSET_MESSAGE_CODE    5

// other ANT stuff
#define ANT_SYNC_BYTE        0xA4
#define ANT_MAX_LENGTH       9
#define ANT_KEY_LENGTH       8
#define ANT_MAX_BURST_DATA   8
#define ANT_MAX_MESSAGE_SIZE 12
#define ANT_MAX_CHANNELS     8

// Channel messages
#define RESPONSE_NO_ERROR               0
#define EVENT_RX_SEARCH_TIMEOUT         1
#define EVENT_RX_FAIL                   2
#define EVENT_TX                        3
#define EVENT_TRANSFER_RX_FAILED        4
#define EVENT_TRANSFER_TX_COMPLETED     5
#define EVENT_TRANSFER_TX_FAILED        6
#define EVENT_CHANNEL_CLOSED            7
#define EVENT_RX_BROADCAST             10
#define EVENT_RX_ACKNOWLEDGED          11
#define EVENT_RX_BURST_PACKET          12
#define CHANNEL_IN_WRONG_STATE         21
#define CHANNEL_NOT_OPENED             22
#define CHANNEL_ID_NOT_SET             24
#define TRANSFER_IN_PROGRESS           31
#define TRANSFER_SEQUENCE_NUMBER_ERROR 32
#define INVALID_MESSAGE                40
#define INVALID_NETWORK_NUMBER         41

// ANT+sport
#define ANT_SPORT_HR_PERIOD 8070
#define ANT_SPORT_POWER_PERIOD 8182
#define ANT_SPORT_SPEED_PERIOD 8118
#define ANT_SPORT_CADENCE_PERIOD 8102
#define ANT_SPORT_SandC_PERIOD 8086
#define ANT_SPORT_CONTROL_PERIOD 8192
#define ANT_SPORT_KICKR_PERIOD 2048
#define ANT_SPORT_MOXY_PERIOD 8192
#define ANT_FAST_QUARQ_PERIOD (8182/16)
#define ANT_QUARQ_PERIOD (8182*4)

#define ANT_SPORT_HR_TYPE 0x78
#define ANT_SPORT_POWER_TYPE 11
#define ANT_SPORT_SPEED_TYPE 0x7B
#define ANT_SPORT_CADENCE_TYPE 0x7A
#define ANT_SPORT_SandC_TYPE 0x79
#define ANT_SPORT_MOXY_TYPE 0x1F
#define ANT_SPORT_CONTROL_TYPE 0x10
#define ANT_FAST_QUARQ_TYPE_WAS 11 // before release 1.8
#define ANT_FAST_QUARQ_TYPE 0x60
#define ANT_QUARQ_TYPE 0x60

#define ANT_SPORT_FREQUENCY 57
#define ANT_FAST_QUARQ_FREQUENCY 61
#define ANT_QUARQ_FREQUENCY 61
#define ANT_KICKR_FREQUENCY 52
#define ANT_MOXY_FREQUENCY 57

#define ANT_SPORT_CALIBRATION_MESSAGE                 0x01

// Calibration messages carry a calibration id
#define ANT_SPORT_SRM_CALIBRATIONID                   0x10
#define ANT_SPORT_CALIBRATION_REQUEST_MANUALZERO      0xAA
#define ANT_SPORT_CALIBRATION_REQUEST_AUTOZERO_CONFIG 0xAB
#define ANT_SPORT_ZEROOFFSET_SUCCESS                  0xAC
#define ANT_SPORT_AUTOZERO_SUCCESS                    0xAC
#define ANT_SPORT_ZEROOFFSET_FAIL                     0xAF
#define ANT_SPORT_AUTOZERO_FAIL                       0xAF
#define ANT_SPORT_AUTOZERO_SUPPORT                    0x12

#define ANT_SPORT_AUTOZERO_OFF                        0x00
#define ANT_SPORT_AUTOZERO_ON                         0x01

// kickr
#define KICKR_COMMAND_INTERVAL         60 // every 60 ms
#define KICKR_SET_RESISTANCE_MODE      0x40
#define KICKR_SET_STANDARD_MODE        0x41
#define KICKR_SET_ERG_MODE             0x42
#define KICKR_SET_SIM_MODE             0x43
#define KICKR_SET_CRR                  0x44
#define KICKR_SET_C                    0x45
#define KICKR_SET_GRADE                0x46
#define KICKR_SET_WIND_SPEED           0x47
#define KICKR_SET_WHEEL_CIRCUMFERENCE  0x48
#define KICKR_INIT_SPINDOWN            0x49
#define KICKR_READ_MODE                0x4A
#define KICKR_SET_FTP_MODE             0x4B
// 0x4C-0x4E reserved.
#define KICKR_CONNECT_ANT_SENSOR       0x4F
// 0x51-0x59 reserved.
#define KICKR_SPINDOWN_RESULT          0x5A


//======================================================================
// Worker thread
//======================================================================
class ANT : public QThread
{

   Q_OBJECT
   G_OBJECT


public:
    ANT(QObject *parent = 0, DeviceConfiguration *dc=0);
    ~ANT();

signals:
    void foundDevice(int channel, int device_number, int device_id); // channelInfo
    void lostDevice(int channel);            // dropInfo
    void searchTimeout(int channel);         // searchTimeount
    void searchComplete(int channel);         // searchComplete
    void signalStrength(int channel, double reliability);

    // signal instantly on data receipt for R-R data
    // made a special case to support HRV tool without complication
    void rrData(uint16_t  measurementTime, uint8_t heartrateBeats, uint8_t instantHeartrate);

    void receivedAntMessage(const ANTMessage message, const struct timeval timestamp);

public slots:

    // runtime controls
    int start();                                // Calls QThread to start
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    int quit(int error);                        // called by thread before exiting

    // configuration and channel management
    int setup();                                // reset system, network key and device pairing - moved out of start()
    bool isConfiguring() { return configuring; }
    void setConfigurationMode(bool x) { configuring = x; }
    void setChannel(int channel, int device_number, int channel_type) {
        channelQueue.enqueue(setChannelAtom(channel, device_number, channel_type));
    }
    bool find();                              // find usb device
    bool discover(QString name);              // confirm Server available at portSpec

    int channelCount() { return channels; }   // how many channels we got available?
    void channelInfo(int number, int device_number, int device_id);  // found a device
    void dropInfo(int number, int drops, int received);    // we dropped a connection
    void lostInfo(int number);    // we lost informa
    void staleInfo(int number);   // info is now stale
    void slotSearchTimeout(int number); // search timed out
    void slotSearchComplete(int number); // search completed successfully

    // get telemetry
    void getRealtimeData(RealtimeData &);             // return current realtime data

    // kickr command loading - only ANT device we know about to do this so not generic
    void setLoad(double);
    void setGradient(double);
    void setMode(int);
    void kickrCommand();

public:

    static int interpretSuffix(char c); // utility to convert e.g. 'c' to CHANNEL_TYPE_CADENCE
    static const char *deviceTypeDescription(int type); // utility to convert CHANNEL_TYPE_X to human string
    static char deviceTypeCode(int type); // utility to convert CHANNEL_TYPE_X to 'c', 'p' et al
    static char deviceIdCode(int type); // utility to convert CHANNEL_TYPE_X to 'c', 'p' et al

    // debug enums
    enum { DEBUG_LEVEL_ERRORS=1,
       DEBUG_LEVEL_ANT_CONNECTION=2,
       DEBUG_LEVEL_ANT_MESSAGES=4,
       DEBUG_LEVEL_CONFIG_PARSE=8
    };

    static const unsigned char key[8];
    static const ant_sensor_type_t ant_sensor_types[];
    ANTChannel *antChannel[ANT_MAX_CHANNELS];

    // ANT Devices and Channels
    int addDevice(int device_number, int device_type, int channel_number);
    int removeDevice(int device_number, int channel_type);
    ANTChannel *findDevice(int device_number, int channel_type);
    int startWaitingSearch();

    // transmission
    void sendMessage(ANTMessage);
    void receiveByte(unsigned char byte);
    void handleChannelEvent(void);
    void processMessage(void);


    // serial i/o lifted from Computrainer.cpp
    void setDevice(QString devname);
    void setBaud(int baud);
    int openPort();
    int closePort();
    int rawRead(uint8_t bytes[], int size);
    int rawWrite(uint8_t *bytes, int size);

    // channels update our telemetry
    double channelValue(int channel);
    double channelValue2(int channel);
    void setBPM(float x) {
        telemetry.setHr(x);
    }
    void setCadence(float x) {
        telemetry.setCadence(x);
    }
    void setWheelRpm(float x);
    void setWatts(float x) {
        telemetry.setWatts(x);
    }
    void setAltWatts(float x) {
        telemetry.setAltWatts(x);
    }
    void setHb(double smo2, double thb);

private:

    void run();

    RealtimeData telemetry;
    QMutex pvars;  // lock/unlock access to telemetry data between thread and controller
    int Status;     // what status is the client in?
    bool configuring; // set to true if we're in configuration mode.
    int channels;  // how many 4 or 8 ? depends upon the USB stick...

    // access to device file
    DeviceConfiguration *devConf;
    QString deviceFilename;
    int baud;
#ifdef WIN32
    HANDLE devicePort;              // file descriptor for reading from com3
    DCB deviceSettings;             // serial port settings baud rate et al
#else
    int devicePort;                 // unix!!
    struct termios deviceSettings;  // unix!!
#endif

#if defined GC_HAVE_LIBUSB
    LibUsb *usb2;                   // used for USB2 support
    enum UsbMode { USBNone, USB1, USB2 };
    enum UsbMode usbMode;
#endif

    // telemetry and state
    QStringList antIDs;
#if 0
    QTime elapsedTime;
#endif

    unsigned char rxMessage[ANT_MAX_MESSAGE_SIZE];

    // state machine whilst receiving bytes
    enum States {ST_WAIT_FOR_SYNC, ST_GET_LENGTH, ST_GET_MESSAGE_ID, ST_GET_DATA, ST_VALIDATE_PACKET} state;
    //enum States state;
    int length;
    int bytes;
    int checksum;
    int powerchannels; // how many power channels do we have?

    QQueue<setChannelAtom> channelQueue; // messages for configuring channels from controller

    // generic trainer settings
    double currentLoad, load;
    double currentGradient, gradient;
    int currentMode, mode;

    // now kickr specific
    int kickrDeviceID;
    int kickrChannel;

};

#include "ANTMessage.h"
#include "ANTChannel.h"

#endif
