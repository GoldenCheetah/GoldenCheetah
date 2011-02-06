/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

//
// Time
//
#include <sys/time.h>

//
// Serial i/o stuff
//
#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#else
#include <termios.h> // unix!!
#include <unistd.h> // unix!!
#include <sys/ioctl.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

// timeouts for read/write of serial port in ms
#define ANT_READTIMEOUT    1000
#define ANT_WRITETIMEOUT   2000

//======================================================================
// Debug output, set DEBUGLEVEL using Quarqd settings
//======================================================================
#define DEBUGLEVEL 0

#define DEBUG_PRINTF(mask, format, args...) \
  if (DEBUGLEVEL & mask) \
    fprintf(stderr, format, ##args)

#define DEBUG_ERRORS(format, args...) \
  DEBUG_PRINTF(DEBUG_LEVEL_ERRORS, format, ##args)

#define DEBUG_ANT_CONNECTION(format, args...)  \
  DEBUG_PRINTF(DEBUG_LEVEL_ANT_CONNECTION, format, ##args)

#define DEBUG_ANT_MESSAGES(format, args...)  \
  DEBUG_PRINTF(DEBUG_LEVEL_ANT_MESSAGES, format, ##args)

#define DEBUG_CONFIG_PARSE(format, args...)  \
  DEBUG_PRINTF(DEBUG_LEVEL_CONFIG_PARSE, format, ##args)

//======================================================================
// ANT channels
//======================================================================


#define CHANNEL_TYPE_QUICK_SEARCH 0x10 // or'ed with current channel type
/* after fast search, wait for slow search.  Otherwise, starting slow
   search might postpone the fast search on another channel. */
#define CHANNEL_TYPE_WAITING 0x20 
#define CHANNEL_TYPE_PAIR   0x40 // to do an Ant pair
#define MESSAGE_RECEIVED -1

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

// note: this struct is from quarqd_dist/quarqd/src/generated-headers.h
typedef struct {
	int first_time_crank_torque;
	int first_time_crank_SRM;
	int first_time_wheel_torque;
	int first_time_standard_power;
	int first_time_torque_support;
	int first_time_calibration_pass;
	int first_time_calibration_fail;
	int first_time_heart_rate;
	int first_time_speed;
	int first_time_cadence;
	int first_time_speed_cadence;
	int first_time_manufacturer;
	int first_time_product;
	int first_time_battery_voltage;
} messages_initialization_t;

#define INITIALIZE_MESSAGES_INITIALIZATION(x) \
	x.first_time_crank_torque=1; \
	x.first_time_crank_SRM=1; \
	x.first_time_wheel_torque=1; \
	x.first_time_standard_power=1; \
	x.first_time_torque_support=1; \
	x.first_time_calibration_pass=1; \
	x.first_time_calibration_fail=1; \
	x.first_time_heart_rate=1; \
	x.first_time_speed=1; \
	x.first_time_cadence=1; \
	x.first_time_speed_cadence=1; \
	x.first_time_manufacturer=1; \
	x.first_time_product=1; \
	x.first_time_battery_voltage=1; \

typedef struct ant_channel {
  int number; // Channel number within Ant chip  
  int state;  
  int channel_type;
  int channel_type_flags;
  int device_number;
  int device_id;

  int search_type;

  struct channel_manager *parent;

  double last_message_timestamp;
  double blanking_timestamp;
  int blanked;

  char id[10]; // short identifier

  int channel_assigned;
 
  messages_initialization_t mi;

  int messages_received; // for signal strength metric
  int messages_dropped;  

  unsigned char rx_burst_data[RX_BURST_DATA_LEN];
  int           rx_burst_data_index;
  unsigned char rx_burst_next_sequence;

  void (*rx_burst_disposition)(struct ant_channel *);
  void (*tx_ack_disposition)(struct ant_channel *);

  int is_cinqo; // bool
  int is_old_cinqo; // bool, set for cinqo needing separate control channel
  struct ant_channel *control_channel;
  /* cinqo-specific, which channel should control
     messages be sent to?   Open a channel for older
     cinqos */
  int manufacturer_id;
  int product_id;
  int product_version;

} ant_channel_t;

static inline double get_timestamp( void ) {
  struct timeval tv;
  gettimeofday(&tv, NULL); 
  return tv.tv_sec * 1.0 + tv.tv_usec * 1.0e-6;
}

//======================================================================
// ANT Channel Management
//======================================================================

#define ANT_CHANNEL_COUNT 4
typedef struct channel_manager {
  struct ant_channel channels[ANT_CHANNEL_COUNT];
} channel_manager_t;

//======================================================================
// ANT Messaging
//======================================================================

#include "ANTMessages.h"

// ANT constants
#define POWER_MSG_ID         	0x10
#define TORQUE_AT_WHEEL_MSG_ID	0x11
#define TORQUE_AT_CRANK_MSG_ID	0x12
#define ANT_MAX_DATA_MESSAGE_SIZE	8

// ANT messages
#define ANT_UNASSIGN_CHANNEL		0x41
#define ANT_ASSIGN_CHANNEL		0x42
#define ANT_CHANNEL_ID			0x51
#define ANT_CHANNEL_PERIOD		0x43
#define ANT_SEARCH_TIMEOUT		0x44
#define ANT_CHANNEL_FREQUENCY		0x45
#define ANT_SET_NETWORK			0x46
#define ANT_TX_POWER			0x47
#define ANT_ID_LIST_ADD			0x59
#define ANT_ID_LIST_CONFIG		0x5A
#define ANT_CHANNEL_TX_POWER		0x60
#define ANT_LP_SEARCH_TIMEOUT		0x63
#define ANT_SET_SERIAL_NUMBER		0x65
#define ANT_ENABLE_EXT_MSGS		0x66
#define ANT_ENABLE_LED			0x68
#define ANT_SYSTEM_RESET		0x4A
#define ANT_OPEN_CHANNEL		0x4B
#define ANT_CLOSE_CHANNEL		0x4C
#define ANT_OPEN_RX_SCAN_CH		0x5B
#define ANT_REQ_MESSAGE			0x4D
#define ANT_BROADCAST_DATA		0x4E
#define ANT_ACK_DATA			0x4F
#define ANT_BURST_DATA			0x50
#define ANT_CHANNEL_EVENT		0x40
#define ANT_CHANNEL_STATUS		0x52
#define ANT_CHANNEL_ID			0x51
#define ANT_VERSION			0x3E
#define ANT_CAPABILITIES		0x54
#define ANT_SERIAL_NUMBER		0x61
#define ANT_CW_INIT			0x53
#define ANT_CW_TEST			0x48

// ANT message structure.
#define ANT_OFFSET_SYNC			0
#define ANT_OFFSET_LENGTH		1
#define ANT_OFFSET_ID			2
#define ANT_OFFSET_DATA			3
#define ANT_OFFSET_CHANNEL_NUMBER       3
#define ANT_OFFSET_MESSAGE_ID           4
#define ANT_OFFSET_MESSAGE_CODE         5

// other ANT stuff
#define ANT_SYNC_BYTE		0xA4
#define ANT_MAX_LENGTH		9
#define ANT_KEY_LENGTH		8
#define ANT_MAX_BURST_DATA	8
#define ANT_MAX_MESSAGE_SIZE    12
#define ANT_MAX_CHANNELS	4

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
#define ANT_SPORT_AUTOZERO_OFF                        0x00
#define ANT_SPORT_AUTOZERO_ON                         0x01
#define ANT_SPORT_CALIBRATION_REQUEST_MANUALZERO      0xAA
#define ANT_SPORT_CALIBRATION_REQUEST_AUTOZERO_CONFIG 0xAB

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

public slots:

    // runtime controls
    int start();                                // Calls QThread to start
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    int quit(int error);                        // called by thread before exiting

    // channel management
    void initChannel();
    bool discover(DeviceConfiguration *, QProgressDialog *);              // confirm Server available at portSpec
    void reinitChannel(QString _channel);

    // get telemetry
    RealtimeData getRealtimeData();             // return current realtime data

public:

    // ANT Channel Management
    void channel_manager_init(channel_manager_t *self);
    int channel_manager_add_device(channel_manager_t *self, int device_number, int device_type, int channel_number);
    int channel_manager_remove_device(channel_manager_t *self, int device_number, int channel_type);
    ant_channel_t *channel_manager_find_device(channel_manager_t *self, int device_number, int channel_type);
    int channel_manager_start_waiting_search(channel_manager_t *self);
    void channel_manager_report(channel_manager_t *self);
    void channel_manager_associate_control_channels(channel_manager_t *self);

    // ANT channel functions
    void ant_channel_init(ant_channel_t *self, int number, struct channel_manager *parent);
    const char *ant_channel_get_description(ant_channel_t *self);
    int ant_channel_interpret_description(char *description);
    int ant_channel_interpret_suffix(char c);
    void ant_channel_open(ant_channel_t *self, int device_number, int channel_type);
    void ant_channel_close(ant_channel_t *self);

    void ant_channel_receive_message(ant_channel_t *self, unsigned char *message);
    void ant_channel_channel_event(ant_channel_t *self, unsigned char *message);
    void ant_channel_burst_data(ant_channel_t *self, unsigned char *message);
    void ant_channel_broadcast_event(ant_channel_t *self, unsigned char *message);
    void ant_channel_ack_event(ant_channel_t *self, unsigned char *message);
    void ant_channel_channel_id(ant_channel_t *self, unsigned char *message);
    void ant_channel_request_calibrate(ant_channel_t *self);
    void ant_channel_attempt_transition(ant_channel_t *self, int message_code);
    void ant_channel_channel_info(ant_channel_t *self);
    void ant_channel_drop_info(ant_channel_t *self);
    void ant_channel_lost_info(ant_channel_t *self);
    void ant_channel_stale_info(ant_channel_t *self);
    void ant_channel_report_timeouts( void );
    int ant_channel_set_timeout( char *type, float value, int connection );
    void ant_channel_search_complete(void);
    //void ant_channel_send_error( char * message ); // XXX  missing in quarqd src (unused)
    void ant_channel_set_id(ant_channel_t *self);
    int ant_channel_is_searching(ant_channel_t *self);
    void ant_channel_burst_init(ant_channel_t *self);
    void ant_channel_send_cinqo_error(ant_channel_t *self);
    void ant_channel_send_cinqo_success(ant_channel_t *self);
    void ant_message_print_debug(unsigned char *);

    // device specific
    void ant_channel_check_cinqo(ant_channel_t *self);

    // XXX decide how to manage this
#if 0
    float get_srm_offset(int);
    void set_srm_offset(int, float);
#endif

    // low-level ANT functions (from quarqd_dist/src/ant.c)
    void ANT_ResetSystem(void);
    void ANT_AssignChannel(const unsigned char channel, const unsigned char type, const unsigned char network);
    void ANT_UnassignChannel(const unsigned char channel);
    void ANT_SetChannelSearchTimeout(const unsigned char channel, const unsigned char search_timeout);
    void ANT_RequestMessage(const unsigned char channel, const unsigned char request);
    void ANT_SetChannelID(const unsigned char channel, const unsigned short device,
                      const unsigned char deviceType, const unsigned char txType);
    void ANT_SetChannelPeriod(const unsigned char channel, const unsigned short period);
    void ANT_SetChannelFreq(const unsigned char channel, const unsigned char frequency);
    void ANT_SetNetworkKey(const unsigned char net, const unsigned char *key);
    void ANT_SendAckMessage( void );
    void ANT_SetAutoCalibrate(const unsigned char channel, const int autozero);
    void ANT_RequestCalibrate(const unsigned char channel);
    void ANT_Open(const unsigned char channel);
    void ANT_Close(const unsigned char channel);
    void ANT_SendMessage(void);
    void ANT_Transmit(const unsigned char *msg, int length);
    void ANT_ReceiveByte(unsigned char byte);
    void ANT_HandleChannelEvent(void);
    void ANT_ProcessMessage(void);


    // serial i/o lifted from Computrainer.cpp
    void setDevice(QString devname);
    void setBaud(int baud);
    int openPort();
    int closePort();
    int rawRead(uint8_t bytes[], int size);
    int rawWrite(uint8_t *bytes, int size);

private:
    RealtimeData telemetry;
    double timestamp;

    QMutex pvars;  // lock/unlock access to telemetry data between thread and controller
    int Status;     // what status is the client in?

    // thread process
    void run();

    // access to device file
    QString deviceFilename;
    int baud;
#ifdef WIN32
    HANDLE devicePort;              // file descriptor for reading from com3
    DCB deviceSettings;             // serial port settings baud rate et al
#else
    int devicePort;                 // unix!!
    struct termios deviceSettings;  // unix!!
#endif

    // telemetry and state
    QStringList antIDs;
    long lastReadHR;
    long lastReadWatts;
    long lastReadCadence;
    long lastReadWheelRpm;
    long lastReadSpeed;
    QTime elapsedTime;
    bool sentDual, sentSpeed, sentHR, sentCad, sentPWR;

    // ANT low-level routines state and buffers
    // from quarqd_dist/src/ant.c
    unsigned char rxMessage[ANT_MAX_MESSAGE_SIZE]; 
    unsigned char txMessage[ANT_MAX_MESSAGE_SIZE+10];
    unsigned char txAckMessage[ANT_MAX_MESSAGE_SIZE];
    static const unsigned char key[8];
    static const ant_sensor_type_t ant_sensor_types[];

    // state machine whilst receiving bytes
	enum States {ST_WAIT_FOR_SYNC, ST_GET_LENGTH, ST_GET_MESSAGE_ID, ST_GET_DATA, ST_VALIDATE_PACKET};
	enum States state;
	int length;
	int bytes;
	int checksum;

    // debug enums
    enum { DEBUG_LEVEL_ERRORS=1,
       DEBUG_LEVEL_ANT_CONNECTION=2,
       DEBUG_LEVEL_ANT_MESSAGES=4,
       DEBUG_LEVEL_CONFIG_PARSE=8
    };

    enum { 
        CHANNEL_TYPE_UNUSED,
        CHANNEL_TYPE_HR,
        CHANNEL_TYPE_POWER, 
        CHANNEL_TYPE_SPEED,
        CHANNEL_TYPE_CADENCE,
        CHANNEL_TYPE_SandC,
        CHANNEL_TYPE_QUARQ,
        CHANNEL_TYPE_FAST_QUARQ,
        CHANNEL_TYPE_FAST_QUARQ_NEW,
        CHANNEL_TYPE_GUARD
    };
};

#endif
