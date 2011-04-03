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
#include <QStringList>
#include <QTime>
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

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#include "USBXpress.h" // for Garmin USB1 sticks
#include "LibUsb.h"    // for Garmin USB2 sticks
#else
#include <termios.h> // unix!!
#include <unistd.h> // unix!!
#include <sys/ioctl.h>
#endif

// timeouts for read/write of serial port in ms
#define ANT_READTIMEOUT    1000
#define ANT_WRITETIMEOUT   2000

class ANTMessage;
class ANTChannel;

typedef struct ant_sensor_type {
  int type;
  int period;
  int device_id;
  int frequency;
  int network;
  const char *descriptive_name;
  char suffix;

} ant_sensor_type_t;

#define DEFAULT_NETWORK_NUMBER 0
#define ANT_SPORT_NETWORK_NUMBER 1
#define RX_BURST_DATA_LEN 128

static inline double get_timestamp( void ) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1.0 + tv.tv_usec * 1.0e-6;
}


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
#define ANT_MAX_CHANNELS     4

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
#define ANT_FAST_QUARQ_PERIOD (8182/16)
#define ANT_QUARQ_PERIOD (8182*4)

#define ANT_SPORT_HR_TYPE 0x78
#define ANT_SPORT_POWER_TYPE 11
#define ANT_SPORT_SPEED_TYPE 0x7B
#define ANT_SPORT_CADENCE_TYPE 0x7A
#define ANT_SPORT_SandC_TYPE 0x79
#define ANT_FAST_QUARQ_TYPE_WAS 11 // before release 1.8
#define ANT_FAST_QUARQ_TYPE 0x60
#define ANT_QUARQ_TYPE 0x60

#define ANT_SPORT_FREQUENCY 57
#define ANT_FAST_QUARQ_FREQUENCY 61
#define ANT_QUARQ_FREQUENCY 61

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

public slots:

    // runtime controls
    int start();                                // Calls QThread to start
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    int quit(int error);                        // called by thread before exiting

    // channel management
    bool discover(DeviceConfiguration *, QProgressDialog *);              // confirm Server available at portSpec
    void channelInfo(int number, int device_number, int device_id);  // found a device
    void dropInfo(int number);    // we dropped a connection
    void lostInfo(int number);    // we lost informa
    void staleInfo(int number);   // info is now stale
    void slotSearchTimeout(int number); // search timed out
    void searchComplete(int number); // search completed successfully

    // get telemetry
    RealtimeData getRealtimeData();             // return current realtime data

public:

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
    void report();
    void associateControlChannels();

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
    void setBPM(float x) {
        telemetry.setHr(x);
    }
    void setCadence(float x) {
        telemetry.setCadence(x);
    }
    void setWheelRpm(float x) {
        telemetry.setWheelRpm(x);
        telemetry.setSpeed(x * 2.1 * 60 / 1000); // XXX fixed wheel size needs fixing
    }
    void setWatts(float x) {
        telemetry.setWatts(x);
    }

private:

    void run();
    static int interpretSuffix(char c); // utility to convert e.g. 'c' to CHANNEL_TYPE_CADENCE
    static const char *deviceTypeDescription(int type); // utility to convert CHANNEL_TYPE_XXX to human string
    static char deviceTypeCode(int type); // utility to convert CHANNEL_TYPE_XXX to 'c', 'p' et al

    RealtimeData telemetry;
    QMutex pvars;  // lock/unlock access to telemetry data between thread and controller
    int Status;     // what status is the client in?

    // access to device file
    QString deviceFilename;
    int baud;
#ifdef WIN32
    HANDLE devicePort;              // file descriptor for reading from com3
    DCB deviceSettings;             // serial port settings baud rate et al
    LibUsb *usb2;                   // used for USB2 support
    enum UsbMode { USBNone, USB1, USB2 };
    enum UsbMode usbMode;
#else
    int devicePort;                 // unix!!
    struct termios deviceSettings;  // unix!!
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

    // antlog.bin ant message stream
    QFile antlog;
};

#endif
