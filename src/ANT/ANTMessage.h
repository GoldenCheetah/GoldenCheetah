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
        static ANTMessage setLPSearchTimeout(const unsigned char channel,
                                             const unsigned char timeout);
        static ANTMessage setHPSearchTimeout(const unsigned char channel,
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

        // tacx vortex command message is a single broadcast ant message
        static ANTMessage tacxVortexSetFCSerial(const uint8_t channel, const uint16_t setVortexId);
        static ANTMessage tacxVortexStartCalibration(const uint8_t channel, const uint16_t vortexId);
        static ANTMessage tacxVortexStopCalibration(const uint8_t channel, const uint16_t vortexId);
        static ANTMessage tacxVortexSetCalibrationValue(const uint8_t channel, const uint16_t vortexId, const uint8_t calibrationValue);
        static ANTMessage tacxVortexSetPower(const uint8_t channel, const uint16_t vortexId, const uint16_t power);

        // fitness equipment control messages
        static ANTMessage fecSetResistance(const uint8_t channel, const uint8_t resistance);
        static ANTMessage fecSetTargetPower(const uint8_t channel, const uint16_t targetPower);
        static ANTMessage fecSetWindResistance(const uint8_t channel, const double windResistance, const uint8_t windSpeed, const uint8_t draftingFactor);
        static ANTMessage fecSetTrackResistance(const uint8_t channel, const double grade, const double rollingResistance);
        static ANTMessage fecRequestCapabilities(const uint8_t channel);
        static ANTMessage fecRequestCommandStatus(const uint8_t channel, const uint8_t page);
        static ANTMessage fecRequestCalibration(const uint8_t channel, const uint8_t type);

        // Power meter calibration
        static ANTMessage requestPwrCalibration(const uint8_t channel, const uint8_t type);

        // Power meter capabilities
        static ANTMessage requestPwrCapabilities1(const uint8_t channel);
        static ANTMessage requestPwrCapabilities2(const uint8_t channel);
        static ANTMessage enablePwrCapabilities1(const uint8_t channel, const uint8_t capabilitiesMask, const uint8_t capabilitiesSetup);
        static ANTMessage enablePwrCapabilities2(const uint8_t channel, const uint8_t capabilitiesMask, const uint8_t capabilitiesSetup);

        // remote control
        static ANTMessage controlDeviceAvailability(const uint8_t channel);

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
        bool pedalPowerContribution; // power - if true, right pedal power % of contribution is filled in "pedalPower"
        uint8_t pedalPower; // power - if pedalPowerContribution is true, % contribution of right pedal
        uint16_t lastMeasurementTime, prevMeasurementTime, wheelMeasurementTime, crankMeasurementTime;
        uint8_t heartrateBeats, instantHeartrate; // heartrate
        uint16_t slope, period, torque; // power
        uint16_t sumPower, instantPower; // power
        uint16_t wheelRevolutions, crankRevolutions; // speed and power and cadence
        uint16_t wheelAccumulatedPeriod;
        uint16_t accumulatedTorque;
        uint8_t  elapsedTime;
        uint8_t instantCadence; // cadence
        double  instantStartAngle;
        double  instantEndAngle;
        double  instantStartPeakAngle;
        double  instantEndPeakAngle;
        uint8_t riderPosition;
        uint8_t rightPCO;
        uint8_t leftPCO;

        uint8_t autoZeroStatus, autoZeroEnable;
        bool utcTimeRequired; // moxy
        uint8_t moxyCapabilities; //moxy
        double tHb, oldsmo2, newsmo2; //moxy
        double coreTemp, skinTemp;
        uint8_t tempQual;
        uint8_t leftTorqueEffectiveness, rightTorqueEffectiveness; // power - TE&PS
        uint8_t leftOrCombinedPedalSmoothness, rightPedalSmoothness; // power - TE&PS
        // tacx vortex fields - only what we care about now, for more check decoding
        uint16_t vortexId, vortexSpeed, vortexPower, vortexCadence;
        uint8_t vortexCalibration, vortexCalibrationState, vortexPage;
        uint8_t vortexUsingVirtualSpeed;
        uint8_t  pwrCapabilities1, pwrEnCapabilities1, pwrCapabilities2, pwrEnCapabilities2;

        // fitness equipment data fields
        uint16_t fecSpeed, fecInstantPower, fecAccumulatedPower;
        uint8_t  fecRawDistance, fecCadence, fecPage0x19EventCount, fecPage0x20EventCount;
        bool     fecPowerCalibRequired, fecResisCalibRequired, fecUserConfigRequired;
        uint8_t  fecPowerOverLimits;
        uint8_t  fecState;
        uint8_t  fecHRSource;
        bool     fecDistanceCapability;
        bool     fecSpeedIsVirtual;

        uint8_t  fecEqtType, fecCapabilities;
        bool     fecResistModeCapability, fecPowerModeCapability, fecSimulModeCapability;
        uint16_t fecMaxResistance;

        // for details and equations see ANT+ Fitness Equipment Device Profile, Rev 4.1 p 66... "6.8  Control Data Pages"
        uint8_t  fecLastCommandReceived, fecLastCommandSeq, fecLastCommandStatus;
        double   fecSetResistanceAck;        //    0  /   +100 %
        uint16_t fecSetTargetPowerAck;       //    0  /  +4000 W
        double   fecSetGradeAck;             // -200  /   +200 %
        double   fecSetRollResistanceAck;    //    0  / 0.0127
        double   fecSetWindResistanceAck;    //    0  /   1.86 kg/m
        int8_t   fecSetWindSpeedAck;         // -127  /   +127 km/h
        uint8_t  fecSetDraftingFactorAck;    //    0  /    100 %
        uint8_t  fecCalibrationReq, fecCalibrationStatus, fecTemperature, fecCalibrationConditions;
        uint16_t fecZeroOffset, fecSpindownTime, fecTargetSpeed, fecTargetSpindownTime;

        uint16_t calibrationOffset;

        // remote control
        uint8_t  controlSeq;
        uint16_t controlSerial, controlVendor, controlCmd;

        // footpod
        bool fpodInstant;
        uint8_t  fpodStrides;
        double   fpodSpeed, fpodCadence;

        // tempe
        bool     tempValid;
        uint16_t temp;
        
    private:
        void init();
};
#endif
