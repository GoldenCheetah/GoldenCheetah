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

#include "ANT.h"
#include "ANTChannel.h"

#ifndef gc_ANTMessage_h
#define gc_ANTMessage_h

class ANTMessage {

    public:

        ANTMessage(); // null message
        ANTMessage(ANT *parent, const unsigned char *data); // decode from parameters
        ANTMessage(unsigned char b1,
                   unsigned char b2 = '\0',
                   unsigned char b3 = '\0',
                   unsigned char b4 = '\0',
                   unsigned char b5 = '\0',
                   unsigned char b6 = '\0',
                   unsigned char b7 = '\0',
                   unsigned char b8 = '\0',
                   unsigned char b9 = '\0',
                   unsigned char b10 = '\0',
                   unsigned char b11 = '\0'); // encode with values (at least one value must be passed though)

        // convenience functions for encoding messages
        static ANTMessage resetSystem();
        static ANTMessage assignChannel(const unsigned char channel,
                                        const unsigned char type,
                                        const unsigned char network);
        static ANTMessage boostSignal(const unsigned char channel);
        static ANTMessage unassignChannel(const unsigned char channel);
        static ANTMessage setSearchTimeout(const unsigned char channel,
                                           const unsigned char timeout);
        static ANTMessage requestMessage(const unsigned char channel,
                                         const unsigned char request);
        static ANTMessage setChannelID(const unsigned char channel,
                                       const unsigned short device,
                                       const unsigned char devicetype,
                                       const unsigned char txtype);
        static ANTMessage setChannelPeriod(const unsigned char channel,
                                           const unsigned short period);
        static ANTMessage setChannelFreq(const unsigned char channel,
                                         const unsigned char frequency);
        static ANTMessage setNetworkKey(const unsigned char net,
                                        const unsigned char *key);
        static ANTMessage setAutoCalibrate(const unsigned char channel,
                                           bool autozero);
        static ANTMessage requestCalibrate(const unsigned char channel);
        static ANTMessage open(const unsigned char channel);
        static ANTMessage close(const unsigned char channel);

        // kickr command channel messages all sent as broadcast data
        // over the command channel as type 0x4E
        static ANTMessage kickrErgMode(const unsigned char channel, ushort usDeviceId, ushort usWatts, bool bSimSpeed);
        static ANTMessage kickrFtpMode(const unsigned char channel, ushort usDeviceId, ushort usFtp, ushort usPercent);
        static ANTMessage kickrSlopeMode(const unsigned char channel, ushort usDeviceId, ushort scale);
        static ANTMessage kickrStdMode(const unsigned char channel, ushort usDeviceId, ushort eLevel);
        static ANTMessage kickrSimMode(const unsigned char channel, ushort usDeviceId, float fWeight);
        static ANTMessage kickrWindResistance(const unsigned char channel, ushort usDeviceId, float fC) ;
        static ANTMessage kickrRollingResistance(const unsigned char channel, ushort usDeviceId, float fCrr);
        static ANTMessage kickrGrade(const unsigned char channel, ushort usDeviceId, float fGrade);
        static ANTMessage kickrWindSpeed(const unsigned char channel, ushort usDeviceId, float mpsWindSpeed);
        static ANTMessage kickrWheelCircumference(const unsigned char channel, ushort usDeviceId, float mmCircumference);
        static ANTMessage kickrReadMode(const unsigned char channel, ushort usDeviceId);
        static ANTMessage kickrInitSpindown(const unsigned char channel, ushort usDeviceId);

        // convert a channel event message id to human readable string
        static const char * channelEventMessage(unsigned char c);

        // to avoid a myriad of tedious set/getters the data fields
        // are plain public members. This is unlikely to change in the
        // future since the whole purpose of this class is to encode
        // and decode ANT messages into the member variables below.

        // BEAR IN MIND THAT ONLY A HANDFUL OF THE VARS WILL BE SET
        // DURING DECODING, SO YOU *MUST* EXAMINE THE MESSAGE TYPE
        // TO DECIDE WHICH FIELDS TO REFERENCE

        // Standard ANT values
        int length;
        unsigned char data[ANT_MAX_MESSAGE_SIZE+1]; // include sync byte at front
        double timestamp;
        uint8_t sync, type;
        uint8_t transmitPower, searchTimeout, transmissionType, networkNumber,
                channelType, channel, deviceType, frequency;
        uint16_t channelPeriod, deviceNumber;
        uint8_t key[8];

        // ANT Sport values
        uint8_t data_page, calibrationID, ctfID;
        uint16_t srmOffset, srmSlope, srmSerial;
        uint8_t eventCount;
        uint8_t pedalPower;
        uint16_t measurementTime, wheelMeasurementTime, crankMeasurementTime;
        uint8_t heartrateBeats, instantHeartrate; // heartrate
        uint16_t slope, period, torque; // power
        uint16_t sumPower, instantPower; // power
        uint16_t wheelRevolutions, crankRevolutions; // speed and power and cadence
        uint8_t instantCadence; // cadence
        uint8_t autoZeroStatus, autoZeroEnable;
        bool utcTimeRequired; // moxy
        uint8_t moxyCapabilities; //moxy
        double tHb, oldsmo2, newsmo2; //moxy

    private:
        void init();
};
#endif
