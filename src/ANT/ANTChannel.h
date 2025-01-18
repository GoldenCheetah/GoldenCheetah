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

#ifndef gc_ANTChannel_h
#define gc_ANTChannel_h

#include "ANT.h"
#include "ANTMessage.h"
#include <QObject>
#include <QTime>
#include <QTimer>

#define CHANNEL_TYPE_QUICK_SEARCH 0x10 // or'ed with current channel type
/* after fast search, wait for slow search.  Otherwise, starting slow
   search might postpone the fast search on another channel. */
#define CHANNEL_TYPE_WAITING 0x20
#define CHANNEL_TYPE_RX 0x0
#define CHANNEL_TYPE_TX 0x10
#define CHANNEL_TYPE_PAIR   0x40 // to do an Ant pair
#define MESSAGE_RECEIVED -1

// note: this struct is from quarqd_dist/quarqd/src/generated-headers.h
class ANTChannelInitialisation {

    public:
    ANTChannelInitialisation() {
        initialise();
    }
    void initialise() {
        first_time_crank_torque=
        first_time_crank_SRM=
        first_time_wheel_torque=
        first_time_standard_power=
        first_time_torque_support=
        first_time_calibration_pass=
        first_time_calibration_fail=
        first_time_heart_rate=
        first_time_speed=
        first_time_cadence=
        first_time_speed_cadence=
        first_time_manufacturer=
        first_time_product=
        first_time_battery_voltage= true;
    }

    bool first_time_crank_torque;
    bool first_time_crank_SRM;
    bool first_time_wheel_torque;
    bool first_time_standard_power;
    bool first_time_torque_support;
    bool first_time_calibration_pass;
    bool first_time_calibration_fail;
    bool first_time_heart_rate;
    bool first_time_speed;
    bool first_time_cadence;
    bool first_time_speed_cadence;
    bool first_time_manufacturer;
    bool first_time_product;
    bool first_time_battery_voltage;
};


class ANTChannel : public QObject {
    private:

        Q_OBJECT

        ANT *parent;

        // Stores the last message.
        ANTMessage lastMessage;
        // Stores last message for ANT_STANDARD_POWER.
        ANTMessage lastStdPwrMessage;
        // Stores last message for ANT_CRANKTORQUE_POWER.
        ANTMessage lastCrankTorquePwrMessage;
        // Stores latest ANT_STANDARD_POWER or ANT_CRANKTORQUE_POWER
        // for use by ANT_TE_AND_PS_POWER.
        ANTMessage lastPwrForTePsMessage;
        int dualNullCount, nullCount, stdNullCount;
        double last_message_timestamp;
        uint8_t fecPrevRawDistance;
        uint8_t  fecCapabilities;

        // sensor capabilities, requested capabilities and enabled capabilities
        uint16_t  pwrCapabilities;
        uint16_t  pwrReqCapabilities;
        uint16_t  pwrEnCapabilities;
        uint8_t   pwrCapabilitiesMsgDelay;
        bool      pwrSkipCapability1Rqst;
        bool      pwrSkipCapability1Setup;
        bool      pwrCapabilitiesSetupComplete;

        double blanking_timestamp;
        int blanked;
        char id[10]; // short identifier
        ANTChannelInitialisation mi;

        bool sc_speed_active, sc_cadence_active; // for S&C sensor with single magnet

        int    messages_received; // for signal strength metric
        int    messages_dropped;
        qint64 lastMessageTimestamp; // for time comparisons
        qint64 lastMessageTimestamp2;
        uint16_t lastHRmeasurement;
        uint8_t  lastPositionData;

        unsigned char rx_burst_data[RX_BURST_DATA_LEN];
        int           rx_burst_data_index;
        unsigned char rx_burst_next_sequence;
        void (*rx_burst_disposition)(struct ant_channel *);
        void (*tx_ack_disposition)(struct ant_channel *);

        // what we got
        int product_id;
        int product_version;
        int manufacturer_id;

    public:

        // !!! THIS ENUM LIST MUST MATCH THE ORDER THAT ant_sensor_type_t
        // !!! IS INITIALISED IN ANT.cpp (SORRY, ITS HORRIBLE)
        enum channeltype {
            CHANNEL_TYPE_UNUSED,
            CHANNEL_TYPE_HR,
            CHANNEL_TYPE_POWER,
            CHANNEL_TYPE_SPEED,
            CHANNEL_TYPE_CADENCE,
            CHANNEL_TYPE_SandC,
            CHANNEL_TYPE_FOOTPOD,
            CHANNEL_TYPE_MOXY,
            CHANNEL_TYPE_CONTROL,
            CHANNEL_TYPE_TACX_VORTEX,
            CHANNEL_TYPE_FITNESS_EQUIPMENT,
            CHANNEL_TYPE_TEMPE,
            CHANNEL_TYPE_GUARD
        };
        typedef enum channeltype ChannelType;

        // lets track the status as we open and close a channel
        enum channelstatus {
            Closed,
            Opening,
            Open,
            Closing
        } status;

        // Channel Information - to save tedious set/getters made public
        int number; // Channel number within Ant chip
        int state;
        int channel_type;
        int device_number;
        int channel_type_flags;
        int device_id;
        int blacklisted;

        // special cases
        bool is_kickr;
        bool is_moxy; // bool
        bool is_cinqo; // bool
        bool is_old_cinqo; // bool, set for cinqo needing separate control channel
        bool is_srm;
        bool is_fec;
        bool is_power;
        bool is_alt; // is alternative channel for power
        bool is_master; // is a master channel (for remote control)

        int search_type;

        int      srm_offset;
        int      srm_offset_instant;
        double   srm_calibration_timestamp;
        uint16_t srm_slope;

        ANTChannel(int number, ANT *parent);

        // What kind of channel
        const char *getDescription();
        int interpretDescription(char *description);

        // channel open/close
        void init();
        void open(int device_number, int channel_type);
        void close();

        // handle inbound data
        void receiveMessage(unsigned char *message);
        void channelEvent(unsigned char *message);
        void burstInit();
        void burstData(unsigned char *message);
        void broadcastEvent(unsigned char *message);
        void ackEvent(unsigned char *message);
        void channelId(unsigned char *message);
        void setId();
        void requestCalibrate();
        void attemptTransition(int message_code);

        // telemetry for this channel
        double channelValue() { return value; }
        double channelValue2() { return value2; }
        double value,value2; // used during config, rather than rtData
        uint16_t capabilities();

        // search
        int isSearching();

        QTimer *channelTimer; // timer for master channel broadcast events

        // timer for sensors setup
        QTimer *sensorSetupTimer;

        // Cinqo support
        void sendCinqoError();
        void sendCinqoSuccess();
        void checkCinqo();
        void checkMoxy();
        void checkSRM();

        void setAlt(bool value) { is_alt = value; }

    public slots:
       // setup sensors
        void slotStartSensorSetupTimer();
        void slotStopSensorSetupTimer();
        void slotSensorSetupTimerEvent();

    signals:
        void channelInfo(int number, int device_number, int device_id); // we got a channel info message
        void dropInfo(int number, int dropped, int received);    // we dropped a packet
        void lostInfo(int number);    // we lost a connection
        void staleInfo(int number);   // the connection is stale
        void searchTimeout(int number); // search timed out
        void searchComplete(int number); // search completed successfully
        void broadcastTimerStart(int number);
        void broadcastTimerStop(int number);
        void sensorSetupTimerStart(); // setup sensors timer
        void sensorSetupTimerStop();

        // signal instantly on data receipt for R-R data
        void rrData(uint16_t  rrtime, uint8_t heartrateBeats, uint8_t instantHeartrate);

        void posData(uint8_t position);

        // signal for passing remote control commands
        void antRemoteControl(uint16_t command);
};
#endif
