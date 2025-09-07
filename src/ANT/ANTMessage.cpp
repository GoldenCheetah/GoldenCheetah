/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2009 Mark Rages (Quarq)
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
#include <QDebug>

//
// This ANTMessage class is to decode and Encode ANT Messages
//
// DECODING
// To decode a message a pointer to unsigned char passes the message
// data and the contents are decoded into the class variables.
//
//
// ENCODING
// To encode a message 8 bytes of message data are passed and a checksum
// is calculated.
//
// Since many of the message types do not require all data to be encoded
// there are a number of static convenience methods that take the message
// variables and call the underlying constructor to construct the message.
//
// MESSAGE STRUCTURE
// An ANT message is very simple, it is a maximum of 13 bytes of data in
// the following structure:
//
// Byte 0 : Sync byte (always of value 0x44)
// Byte 1 : Message length (between 1 and 9)
// Byte 2 : Message ID (Type) (between 1 and 255, 0 is invalid)
// Byte 3-10 : Data (variable length)
// Last Byte : Checksum byte (XOR of message bytes including sync)
//
// Depending upon the message id (type) the payload in bytes 3-7 vary. The
// different message types and formats can be found at thisisant.com and are
// also available here: http://www.sparkfun.com/datasheets/Wireless/Nordic/ANT-UserGuide.pdf
//
// The majority of the decode/encode routines have been developed from the quarqd sources.
//
// It is important to note that the ANT+ sports protocol is built on top of the ANT
// message format and the ANT+ message formats can be obtained from thisisant.com
// by registering as an ANT adoptor see: http://www.thisisant.com/pages/ant/ant-adopter-zone
//

//======================================================================
// ANT MESSAGE FORMATS
//======================================================================

/*
 * Message Type                   Message Id ...
 * ------------                   ---------------------------------------------------------------------
 * assign_channel                 0x42,channel,channel_type,network_number
 * unassign_channel               0x41,channel
 * open_channel                   0x4b,channel
 * channel_id                     0x51,channel,uint16_le:device_number,device_type_id,transmission_type
 * burst_message                  0x50,chan_seq,data0,data1,data2,data3,data4,data5,data6,data7
 * burst_message7                 0x50,chan_seq,data0,data1,data2,data3,data4,data5,data6
 * burst_message6                 0x50,chan_seq,data0,data1,data2,data3,data4,data5
 * burst_message5                 0x50,chan_seq,data0,data1,data2,data3,data4
 * burst_message4                 0x50,chan_seq,data0,data1,data2,data3
 * burst_message3                 0x50,chan_seq,data0,data1,data2
 * burst_message2                 0x50,chan_seq,data0,data1
 * burst_message1                 0x50,chan_seq,data0
 * burst_message0                 0x50,chan_seq
 * channel_period                 0x43,channel,uint16_le:period
 * search_timeout                 0x44,channel,search_timeout
 * channel_frequency              0x45,channel,rf_frequency
 * set_network                    0x46,channel,key0,key1,key2,key3,key4,key5,key6,key7
 * transmit_power                 0x47,0,tx_power
 * reset_system                   0x4a,None
 * request_message                0x4d,channel,message_requested
 * close_channel                  0x4c,channel
 * response_no_error              0x40,channel,message_id,0x00
 * response_assign_channel_ok     0x40,channel,0x42,0x00
 * response_channel_unassign_ok   0x40,channel,0x41,0x00
 * response_open_channel_ok       0x40,channel,0x4b,0x00
 * response_channel_id_ok         0x40,channel,0x51,0x00
 * response_channel_period_ok     0x40,channel,0x43,0x00
 * response_channel_frequency_ok  0x40,channel,0x45,0x00
 * response_set_network_ok        0x40,channel,0x46,0x00
 * response_transmit_power_ok     0x40,channel,0x47,0x00
 * response_close_channel_ok      0x40,channel,0x4c,0x00
 * response_search_timeout_ok     0x40,channel,0x44,0x00
 * event_rx_search_timeout        0x40,channel,message_id,0x01
 * event_rx_fail                  0x40,channel,message_id,0x02
 * event_tx                       0x40,channel,message_id,0x03
 * event_transfer_rx_failed       0x40,channel,message_id,0x04
 * event_transfer_tx_completed    0x40,channel,message_id,0x05
 * event_transfer_tx_failed       0x40,channel,message_id,0x06
 * event_channel_closed           0x40,channel,message_id,0x07
 * event_rx_fail_go_to_search     0x40,channel,message_id,0x08
 * event_channel_collision        0x40,channel,message_id,0x09
 * event_transfer_tx_start        0x40,channel,message_id,0x0A
 # event_rx_broadcast             0x40,channel,message_id,0x0A
 * event_rx_acknowledged          0x40,channel,message_id,0x0B
 * event_rx_burst_packet          0x40,channel,message_id,0x0C
 * channel_in_wrong_state         0x40,channel,message_id,0x15
 * channel_not_opened             0x40,channel,message_id,0x16
 * channel_id_not_set             0x40,channel,message_id,0x18
 * transfer_in_progress           0x40,channel,message_id,31
 * channel_status                 0x52,channel,status
 * cw_init                        0x53,None
 * cw_test                        0x48,None,power,freq
 * capabilities                   0x54,max_channels,max_networks,standard_options,advanced_options
 * capabilities_extended          0x54,max_channels,max_networks,standard_options,advanced_options,
 *                                                               advanced_options_2,max_data_channels
 * ant_version                    0x3e,data0,data1,data2,data3,data4,data5,data6,data7,data8
 * ant_version_long               0x3e,data0,data1,data2,data3,data4,data5,data6,data7,data8,data9,
 *                                     data10,data11,data12
 * transfer_seq_number_error      0x40,channel,message_id,0x20
 * invalid_message                0x40,channel,message_id,0x28
 * invalid_network_number         0x40,channel,message_id,0x29
 * channel_response               0x40,channel,message_id,message_code
 * extended_broadcast_data        0x5d,channel,uint16le:device_number,device_type,transmission_type,
 *                                                      data0,data1,data2,data3,data4,data5,data6,data7
 * extended_ack_data              0x5e,channel,uint16le:device_number,device_type,transmission_type,
 *                                                      data0,data1,data2,data3,data4,data5,data6,data7
 * extended_burst_data            0x5f,channel,uint16le:device_number,device_type,transmission_type,
 *                                                      data0,data1,data2,data3,data4,data5,data6,data7
 * startup_message                0x6f,start_message
 */

//======================================================================
// ANT SPORT MESSAGE FORMATS
//======================================================================

/* The channel type the message was received upon generally indicates which kind of broadcast message
 * we are dealing with (4e is an ANT broadcast message). So in the ANTMessage constructor we also
 * pass the channel type to help decide how to decode. See interpret_xxx_broadcast() class members
 *
 * Channel       Message
 * type          Type                Message ID ...
 * ---------     -----------         --------------------------------------------------------------------------
 * heartrate     heart_rate          0x4e,channel,None,None,None,None,uint16_le_diff:measurement_time,
 *                                                                    uint8_diff:beats,uint8:instant_heart_rate
 *
 * speed         speed               0x4e,channel,None,None,None,None,uint16_le_diff:measurement_time,
 *                                                                    uint16_le_diff:wheel_revs
 *
 * cadence       cadence             0x4e,channel,None,None,None,None,uint16_le_diff:measurement_time,
 *                                                                    uint16_le_diff:crank_revs
 *
 * speed_cadence speed_cadence       0x4e,channel,uint16_le_diff:cadence_measurement_time,uint16_le_diff:crank_revs,
 *                                                uint16_le_diff:speed_measurement_time,uint16_le_diff:wheel_revs
 *
 * power         calibration_request None,channel,0x01,0xAA,None,None,None,None,None,None
 * power         srm_zero_response   None,channel,0x01,0x10,0x01,None,None,None,uint16_be:offset
 * power         calibration_pass    None,channel,0x01,0xAC,uint8:autozero_status,None,None,None,uint16_le:calibration_data
 * power         calibration_fail    None,channel,0x01,0xAF,uint8:autozero_status,None,None,None,uint16_le:calibration_data
 * power         torque_support      None,channel,0x01,0x12,uint8:sensor_configuration,sint16_le:raw_torque,
 *                                                     sint16_le:offset_torque,None
 * power         standard_power      0x4e,channel,0x10,uint8_diff:event_count,uint8:pedal_power,uint8:instant_cadence,
 *                                                     uint16_le_diff:sum_power,uint16_le:instant_power
 * power         wheel_torque        0x4e,channel,0x11,uint8_diff:event_count,uint8:wheel_rev,uint8:instant_cadence,
 *                                                     uint16_le_diff:wheel_period,uint16_le_diff:torque
 * power         crank_torque        0x4e,channel,0x12,uint8_diff:event_count,uint8:crank_rev,uint8:instant_cadence,
 *                                                     uint16_le_diff:crank_period,uint16_le_diff:torque
 * power         te_and_ps           0x4e,channel,0x13,uint8_diff:event_count,uint8:left_torque_effectivness, uint8:right_torque_effectiveness,
 *                                                     uint8:left_pedal_smoothness, uint8:right_pedal_smoothness
 *                                                     uint16_le_diff:crank_period,uint16_le_diff:torque
 * power         crank_SRM           0x4e,channel,0x20,uint8_diff:event_count,uint16_be:slope,uint16_be_diff:crank_period,
 *                                                     uint16_be_diff:torque
 *
 * any           manufacturer        0x4e,channel,0x50,None,None,hw_rev,uint16_le:manufacturer_id,uint16_le:model_number_id
 * any           product             0x4e,channel,0x51,None,None,sw_rev,uint16_le:serial_number_qpod,
 *                                                                      uint16_le:serial_number_spider
 * any           battery_voltage     0x4e,channel,0x52,None,None,operating_time_lsb,operating_time_1sb,
 *                                                operating_time_msb,voltage_lsb,descriptive_bits
 */

#define bradToDeg(brads) (static_cast<double>(brads)/256.0*360.0)

// construct a null message
ANTMessage::ANTMessage(void) {
    init();
}

// convert message codes to an english string
const char *
ANTMessage::channelEventMessage(unsigned char c)
{
    switch (c) {
    case 0 : return "No error";
    case 1 : return "Search timeout";
    case 2 : return "Message RX fail";
    case 3 : return "Event TX";
    case 4 : return "Receive TX fail";
    case 5 : return "Ack or Burst completed";
    case 6 : return "Event transfer TX failed";
    case 7 : return "Channel closed success";
    case 8 : return "dropped to search after missing too many messages.";
    case 9 : return "Channel collision";
    case 10 : return "Burst starts";
    case 21 : return "Channel in wrong state";
    case 22 : return "Channel not opened";
    case 24 : return "Open without valid id";
    case 25 : return "OpenRXScan when other channels open";
    case 31 : return "Transmit whilst transfer in progress";
    case 32 : return "Sequence number out of order";
    case 33 : return "Burst message past sequence number not transmitted";
    case 40 : return "INVALID PARAMETERS";
    case 41 : return "INVALID NETWORK";
    case 48 : return "ID out of bounds";
    case 49 : return "Transmit during scan mode";
    case 64 : return "NVM for SensRciore mode is full";
    case 65 : return "NVM write failed";
    default: return "UNKNOWN MESSAGE CODE";
    }
}

// construct an ant message based upon a message structure
// the message structure must include the sync byte
ANTMessage::ANTMessage(ANT *parent, const unsigned char *message) {

    // Initialise all fields to invalid
    init();

    // standard fields
    sync = message[0];
    length = message[1];
    type = message[2];

    // snaffle away the data
    memcpy(data, message, ANT_MAX_MESSAGE_SIZE);

    // now lets decode!
    switch(type) {
        case ANT_UNASSIGN_CHANNEL:
            channel = message[3];
            break;
        case ANT_ASSIGN_CHANNEL:
            channel = message[3];
            channelType = message[4];
            networkNumber = message[5];
            break;
        case ANT_CHANNEL_ID:
            channel = message[3];
            deviceNumber = message[4] + (message[5]<<8);
            deviceType = message[6];
            transmissionType = message[7];
            break;
        case ANT_CHANNEL_PERIOD:
            channel = message[3];
            channelPeriod = message[4] + (message[5]<<8);
            break;
        case ANT_SEARCH_TIMEOUT:
            channel = message[3];
            searchTimeout = message[4];
            break;
        case ANT_CHANNEL_FREQUENCY:
            channel = message[3];
            frequency = message[4];
            break;
        case ANT_SET_NETWORK:
            channel = message[3];
            key[0] = message[4];
            key[1] = message[5];
            key[2] = message[6];
            key[3] = message[7];
            key[4] = message[8];
            key[5] = message[9];
            key[6] = message[10];
            key[7] = message[11];
            break;
        case ANT_TX_POWER:
            transmitPower = message[4];
            break;
        case ANT_ID_LIST_ADD:
            break;
        case ANT_ID_LIST_CONFIG:
            break;
        case ANT_CHANNEL_TX_POWER:
            break;
        case ANT_LP_SEARCH_TIMEOUT:
            break;
        case ANT_SET_SERIAL_NUMBER:
            break;
        case ANT_ENABLE_EXT_MSGS:
            break;
        case ANT_ENABLE_LED:
            break;
        case ANT_SYSTEM_RESET:
            break; // nothing to do, this is ok
        case ANT_OPEN_CHANNEL:
            channel = message[3];
            break;
        case ANT_CLOSE_CHANNEL:
            break;
        case ANT_OPEN_RX_SCAN_CH:
            break;
        case ANT_REQ_MESSAGE:
            break;

        //
        // Telemetry received from device, this is channel type
        // dependant so we examine based upon the channel configuration
        //
        case ANT_BROADCAST_DATA:

            // ANT Sport Device profiles
            //
            // Broadcast data comes in lots of flavours
            // the data page identifier tells us what to
            // expect, but USAGE DIFFERS BY DEVICE TYPE:
            //
            //     at present we just extract the basic telemetry
            //     based upon device type, these pages need to be
            //     supported in the next update (possibly v3.1)
            //
            // HEARTRATE (high bit is used to indicate data changed)
            //           (every 65th message is a background data message)
            //           (get page # with mask 0x7F)
            //   ** Note older devices (e.g. GARMIN) do not support
            //   ** multiple data pages (listed below)
            //   0x00 - Heartrate data
            //   0x01 - Background data page (cumulative data)
            //   0x02 - manufacturer ID and Serial Number
            //   0x03 - Product, Model and Software ID
            //   0x04 - Last transmitted heartbeat
            //
            // SPEED AND CADENCE - (high bit used to indicate data changed)
            //                     (get page # with mask 0x7F)
            //   0x01 - Cumulative operating time
            //   0x02 - manufacturer ID
            //   0x03 - product ID
            //
            // POWER
            //   0x01 - calibration data
            //          (Autozero status are sent every 121 messages)
            //          Has sub types;
            //          0xAA - Rx Calibration Request
            //          0xAC - Tx Acknowledge
            //          0xAF - Tx Fail
            //          0xAB - Rx Autozero configuration
            //          0xAC - Tx Acknowledge
            //          0xAF - Tx Fail
            //          0x12 - Autozero status
            //
            //   0x10 - Standard Power Only - sent every 2 seconds, but not SRMs
            //   0x11 - Wheel torque (Powertap)
            //   0x12 - Crank Torque (Quarq)
            //   0x13 - Torque Efficiency and Pedal Smoothness - optional extension to 0x10 Standard Power or 0x11/0x12 Wheel/Crank Torque
            //   0x20 - Crank Torque Frequency (SRM)
            //   0x50 - Manufacturer’s Identification
            //   0x51 - Product Information
            //   0xE0 - Pedal right force angle
            //   0xE1 - Pedal left force angle
            //   0xE2 - Pedal position data
            //   0xFD - Capabilities #1
            //   0xFE - Capabilities #2
            //   0x50 - Manufacturer UD
            //   0x52 - Battery Voltage

            data_page = message[4];

            // we need to handle ant sport messages here
            switch(parent->antChannel[message[3]]->channel_type) {

            case ANTChannel::CHANNEL_TYPE_HR:
            {

                // page no is first 7 bits, page change toggle is bit 8
                bool page_toggle = data_page&128;
                data_page &= 127;

                // Heartrate is fairly simple. Although
                // many older heart rate devices do not support
                // multiple data pages, and provide random values
                // for the data page itself. (E.g. 1st Gen GARMIN)
                // since we do not care hugely about operating time
                // and serial numbers etc, we don't even try for
                // instant heartrate
                instantHeartrate = message[11];
                heartrateBeats =  message[10];

                // but for R-R timing for HRV applications we will
                // insist upon page 0 or page 4 being present
                switch (data_page) {
                case 0:
                    {
                        // Page 0 only contains time of last heartbeat
                        // so propagate that and
                        lastMeasurementTime = message[8] + (message[9]<<8);
                    }
                break;
                case 4:
                    {
                        // Page 4 contains both time of last heartbeat and
                        // the time of the previous one.
                        lastMeasurementTime = message[8] + (message[9]<<8);
                        prevMeasurementTime = message[6] + (message[7]<<8);
                    }
                    break;

                default:
                    // ignore
                    lastMeasurementTime = 0;
                    break;
                }
            }
            break;
/*
 * these are not supported at present:
 * power         calibration_request None,channel,0x01,0xAA,None,None,None,None,None,None
 * power         srm_zero_response   None,channel,0x01,0x10,0x01,None,None,None,uint16_be:offset
 * power         calibration_pass    None,channel,0x01,0xAC,uint8:autozero_status,None,None,None,uint16_le:calibration_data
 * power         calibration_fail    None,channel,0x01,0xAF,uint8:autozero_status,None,None,None,uint16_le:calibration_data
 * power         torque_support      None,channel,0x01,0x12,uint8:sensor_configuration,sint16_le:raw_torque,
 *                                                     sint16_le:offset_torque,None
 */
            case ANTChannel::CHANNEL_TYPE_POWER:

                channel = message[3];
                switch (data_page) {

                case ANT_STANDARD_POWER: // 0x10 - standard power
                    eventCount = message[5];
                    pedalPowerContribution = (( message[6] != 0xFF ) && ( message[6]&0x80) ) ; // left/right is defined if NOT 0xFF (= no Pedal Power) AND BIT 7 is set
                    pedalPower = (message[6]&0x7F); // right pedalPower % - stored in bit 0-6
                    instantCadence = message[7];
                    sumPower = message[8] + (message[9]<<8);
                    instantPower = message[10] + (message[11]<<8);
                    break;

                // subpages 0xE0/E1/E2: cycling dynamics from power device
                case POWER_CYCL_DYN_R_FORCE_ANGLE_PAGE:
                case POWER_CYCL_DYN_L_FORCE_ANGLE_PAGE:
                    eventCount = message[5];
                    if (message[6]==POWER_CYCL_DYN_INVALID  && message[7]==POWER_CYCL_DYN_INVALID)
                    {
                        // change invalid data (0xC0 as per ANT+ specification) to "0"
                        instantStartAngle=instantEndAngle=0.0;
                    } else
                    {
                        instantStartAngle     = bradToDeg(message[6]);
                        instantEndAngle       = bradToDeg(message[7]);
                    }
                    if (message[8]==POWER_CYCL_DYN_INVALID  && message[9]==POWER_CYCL_DYN_INVALID)
                    {
                        instantStartPeakAngle=instantEndPeakAngle=0.0;
                    } else
                    {
                        instantStartPeakAngle = bradToDeg(message[8]);
                        instantEndPeakAngle   = bradToDeg(message[9]);
                    }
                    torque                = message[10] + (message[11]<<8);
                    break;
                case POWER_CYCL_DYN_PEDALPOSITION_PAGE:
                    eventCount = message[5];
                    riderPosition         = message[6] >> 6;
                    rightPCO              = message[8];
                    leftPCO               = message[9];
                    break;

                case ANT_WHEELTORQUE_POWER: // 0x11 - wheel torque (Powertap)
                    eventCount = message[5];
                    wheelRevolutions = message[6];
                    instantCadence = message[7];
                    period = message[8] + (message[9]<<8);
                    torque = message[10] + (message[11]<<8);
                    break;

                case ANT_CRANKTORQUE_POWER: // 0x12 - crank torque (Quarq)
                    eventCount = message[5];
                    crankRevolutions = message[6];
                    instantCadence = message[7];
                    period = message[8] + (message[9]<<8);
                    torque = message[10] + (message[11]<<8);
                    break;

                case ANT_TE_AND_PS_POWER: // 0x13 - torque efficiency and pedal smoothness - extension to standard power

                    eventCount = message[5];
                    leftTorqueEffectiveness = message[6];
                    rightTorqueEffectiveness = message[7];
                    leftOrCombinedPedalSmoothness = message[8];
                    rightPedalSmoothness = message[9];
                    break;

                case ANT_CRANKSRM_POWER: // 0x20 - crank torque (SRM)
                    eventCount = message[5];
                    slope = message[7] + (message[6]<<8); // yes it is bigendian
                    period = message[9] + (message[8]<<8); // yes it is bigendian
                    torque = message[11] + (message[10]<<8); // yes it is bigendian
                    break;

                case ANT_SPORT_CALIBRATION_MESSAGE:

                    calibrationID = message[5];
                    ctfID = message[6];

                    switch (calibrationID) {

                        case ANT_SPORT_SRM_CALIBRATIONID: // 0x01

                            switch(ctfID) { // different types of calibration for SRMs

                            case 0x01 : // srm_offset
                                srmOffset = message[11] + (message[10]<<8); // yes it is bigendian
                                break;

                            case 0x02 : // slope
                                srmSlope = message[11] + (message[10]<<8); // yes it is bigendian
                                break;

                            case 0x03 : //serial number
                                srmSerial = message[11] + (message[10]<<8); // yes it is bigendian
                                break;

                            default:
                            case 0xAC : // ack
                                break;
                            }
                            break;

                        case ANT_SPORT_ZEROOFFSET_SUCCESS: //0xAC
                            // is also ANT_SPORT_AUTOZERO_SUCCESS: // 0xAC
                            calibrationOffset =  message[10] + (message[11]<<8);
                            autoZeroStatus = message[6];
                            break;

                        case ANT_SPORT_ZEROOFFSET_FAIL: //0xAF
                            // is also ANT_SPORT_AUTOZERO_FAIL: // 0xAF
                            calibrationOffset =  message[10] + (message[11]<<8);
                            autoZeroStatus = message[6];
                            break;

                        case ANT_SPORT_AUTOZERO_SUPPORT: //0x12
                            autoZeroEnable = message[6] & 0x01;
                            autoZeroStatus = message[6] & 0x02;
                            break;

                        default:
                            break;

                    }
                    break;

                case POWER_GETSET_PARAM_PAGE:
                {
                    uint8_t data_subpage = message[5];

                    switch(data_subpage) {
                        case POWER_ADV_CAPABILITIES1_SUBPAGE:
                            pwrCapabilities1         = message[8];
                            pwrEnCapabilities1       = message[10];
                            // qDebug()<<channel
                            //     << qPrintable(QString("Received power advanced capabilities page 1, pwrCapabilities1=0x")+QString("%1").arg(pwrCapabilities1, 2, 16, QChar('0')).toUpper()
                            //     +  QString(", enCapabilities1=0x")+QString("%1").arg(pwrEnCapabilities1, 2, 16, QChar('0')).toUpper());
                            break;

                        case POWER_ADV_CAPABILITIES2_SUBPAGE:
                            pwrCapabilities2         = message[8];
                            pwrEnCapabilities2       = message[10];
                            // qDebug()<<channel
                            //     << qPrintable(QString("Received power advanced capabilities page 2, pwrCapabilities2=0x")+QString("%1").arg(pwrCapabilities2, 2, 16, QChar('0')).toUpper()
                            //     +  QString(", enCapabilities2=0x")+QString("%1").arg(pwrEnCapabilities2, 2, 16, QChar('0')).toUpper());
                            break;
                    }
                    break;
                }

                case POWER_MANUFACTURER_ID:
                {
                    // uint16_t manuf_id = (message[8] + (static_cast<uint16_t>(message[9])<<8));
                    // uint16_t product_id = (message[10] + (static_cast<uint16_t>(message[11])<<8));
                    // qDebug()<<channel<<"Received power sensor info: Manufacturer id:" << manuf_id << ", Product id:" << product_id;
                    break;
                }

                case POWER_PRODUCT_INFO:
                    // qDebug()<<channel<<"Received power product info page";
                    break;

                } // data_page
                break;

            case ANTChannel::CHANNEL_TYPE_SPEED:
                channel = message[3];
                wheelMeasurementTime = message[8] + (message[9]<<8);
                wheelRevolutions =  message[10] + (message[11]<<8);
                break;

            case ANTChannel::CHANNEL_TYPE_CADENCE:
                channel = message[3];
                crankMeasurementTime = message[8] + (message[9]<<8);
                crankRevolutions =  message[10] + (message[11]<<8);
                break;

            case ANTChannel::CHANNEL_TYPE_SandC:
                channel = message[3];
                crankMeasurementTime = message[4] + (message[5]<<8);
                crankRevolutions =  message[6] + (message[7]<<8);
                wheelMeasurementTime = message[8] + (message[9]<<8);
                wheelRevolutions =  message[10] + (message[11]<<8);
                break;

            case ANTChannel::CHANNEL_TYPE_MOXY:
                channel = message[3];
                utcTimeRequired = message[6] & 0x01;
                moxyCapabilities = message[7];
                tHb = 0.01f * double(message[8] + ((message[9]&0x0f)<<8));
                oldsmo2 = 0.1f * double (((message[9] & 0xf0)>>4) + ((message[10]&0x3f)<<4));
                newsmo2 = 0.1f * double (((message[10] & 0xc0)>>6) + (message[11]<<2));
                break;

            case ANTChannel::CHANNEL_TYPE_CORETEMP:
            {
                static int tmpQual=0;
                switch (data_page)
                {
                case 0:
                    // Quality only on intermittent messages so need to remember last quality message
                    tmpQual = (message[6]&0x3);
                    break;
                case 1:
                    tempQual = tmpQual;
                    uint16_t val=(message[7]+((message[8] & 0xf0)<<4));
                    if (val>0 && val != 0x800) 
                        skinTemp = val/20.0;
                    val=(message[10]+(message[11]<<8));
                    if (val>0  && val != 0x8000)
                        coreTemp = val/100.0;
                    uint8_t hsi = message[5]; //Heat strain index
                    if (hsi != 0xff)
                        heatStrain = hsi/10.0;
                    break;
                }
                break;
            }

            case ANTChannel::CHANNEL_TYPE_FITNESS_EQUIPMENT:
                switch (data_page)
                {
                case FITNESS_EQUIPMENT_CALIBRATION_REQUEST_PAGE:
                    // response back from trainer at end of calibration
                    fecCalibrationReq = message[5];
                    fecTemperature    = message[7];
                    fecZeroOffset     = message[8];
                    fecZeroOffset    |= (message[9]) << 8;
                    fecSpindownTime   = message[10];
                    fecSpindownTime  |= (message[11]) << 8;
                    break;

                case FITNESS_EQUIPMENT_CALIBRATION_PROGRESS_PAGE:
                    // periodic responses during calibration
                    fecCalibrationStatus     = message[5];
                    fecCalibrationConditions = message[6];
                    fecTemperature           = message[7];
                    fecTargetSpeed           = message[8];
                    fecTargetSpeed          |= (message[9]) << 8;
                    fecTargetSpindownTime    = message[10];
                    fecTargetSpindownTime   |= (message[11]) << 8;
                    break;

                case FITNESS_EQUIPMENT_GENERAL_PAGE:
                     //based on "ANT+ Device Profile Fitness Equipment" rev 4.1 p 42: 6.5.2 Data page 0x10 - General FE Data
                     fecEqtType = message[5] & 0x1F;
                     elapsedTime = message[6];                  // elapsed time since begining of workout ; unit 0.25s ; rollover 64s
                     fecRawDistance = message[7];               // accumulated distance ; unit = m ; rollover = 256m
                     fecSpeed = (message[9] << 8) | message[8]; // instantaneous speed ; unit=0.001 m/s ; 0xFFFF=invalid
                     instantHeartrate = message[10];            // instantaneous heartrate ; unit = bpm ; 0xFF=invalid
                     fecHRSource = message[11] & 0x03;
                     fecDistanceCapability = (message[11] & 0x04) == 0x04;
                     fecSpeedIsVirtual     = (message[11] & 0x08) == 0x08;
                     fecState = (message[11] & 0xF0) >> 4;
                     break;

                case FITNESS_EQUIPMENT_TRAINER_SPECIFIC_PAGE:
                     //based on "ANT+ Device Profile Fitness Equipment" rev 4.1 p 58: 6.6.7 Data page 0x19 - Specific trainer data
                     fecPage0x19EventCount = message[5];
                     fecCadence = message[6];
                     fecAccumulatedPower = message[7];
                     fecAccumulatedPower |= message[8] << 8;
                     fecInstantPower = message[9];
                     fecInstantPower |= (message[10] & 0x0F) << 8;
                     fecPowerCalibRequired = ((message[10] & 0xF0) >> 4) & FITNESS_EQUIPMENT_POWERCALIB_REQU;
                     fecResisCalibRequired = ((message[10] & 0xF0) >> 4) & FITNESS_EQUIPMENT_RESISCALIB_REQU;
                     fecUserConfigRequired = ((message[10] & 0xF0) >> 4) & FITNESS_EQUIPMENT_USERCONFIG_REQU;
                     fecPowerOverLimits = message[11] & 0x0F;
                     fecState = (message[11] & 0xF0) >> 4;
                     break;

                case FITNESS_EQUIPMENT_STATIONARY_SPECIFIC_PAGE:
                    //based on "ANT+ Device Profile Fitness Equipment" rev 4.1 p 58: 6.6.3 Data page 0x15 - Specific stationary bike data
                    fecCadence = message[8];
                    fecInstantPower = message[9];
                    fecInstantPower |= message[10] << 8;
                    fecState = (message[11] & 0xF0) >> 4;
                    break;

                case FITNESS_EQUIPMENT_TRAINER_TORQUE_PAGE:
                     //based on "ANT+ Device Profile Fitness Equipment" rev 4.1 p 61: 6.6.8 Data page 0x20 - Specific trainer torque data
                     fecPage0x20EventCount = message[5];
                     wheelRevolutions = message[6];                 // increments with each wheel revolution, rollover 256 revolutions
                     wheelAccumulatedPeriod = message[7];           // accumulated wheel period ; unit: 1/2048s ; rollover 32s
                     wheelAccumulatedPeriod |= message[8] << 8;
                     accumulatedTorque = message[9];                // accumulated torque ; unit: 1/32Nm ; rollover 2048Nm
                     accumulatedTorque |= message[10] << 8;
                     fecState = (message[11] & 0xF0) >> 4;
                     break;

                case FITNESS_EQUIPMENT_TRAINER_CAPABILITIES_PAGE:
                     fecMaxResistance         = message[9];
                     fecMaxResistance        |= (message[10]) << 8;
                     fecCapabilities          = message[11];
                     break;

                case FITNESS_EQUIPMENT_COMMAND_STATUS_PAGE:
                    // Acknowledge from device
                    fecLastCommandReceived = message[5];
                    fecLastCommandSeq      = message[6];
                    fecLastCommandStatus   = message[7];
                    switch (fecLastCommandReceived)
                    {
                    case FITNESS_EQUIPMENT_BASIC_RESISTANCE_ID:
                        fecSetResistanceAck = (double) message[11] * 0.5;
                        break;
                    case FITNESS_EQUIPMENT_TARGET_POWER_ID:
                        fecSetTargetPowerAck   = message[10] >> 2;   // device power step is 0.25W, GC consider 1W accuracy only
                        fecSetTargetPowerAck  |= (message[11]) << 6;
                        break;
                    case FITNESS_EQUIPMENT_WIND_RESISTANCE_ID:
                        fecSetWindResistanceAck = (double) message[9] * 0.01;
                        fecSetWindSpeedAck      = message[10];
                        fecSetDraftingFactorAck = message[11];
                        break;
                    case FITNESS_EQUIPMENT_TRACK_RESISTANCE_ID:
                        fecSetGradeAck          = (double) ((int16_t) message[9] | ((int16_t) message[10] << 8)) * 0.01;
                        fecSetRollResistanceAck = (double) message[11] * 0.00005;
                        break;
                    }
                    break;

                case ANT_MANUFACTURER_ID_PAGE: // 0x50
                    break;

                case ANT_PRODUCT_INFO_PAGE: // 0x51
                    break;

                default:
                    break;
                }
                break;

            case ANTChannel::CHANNEL_TYPE_TACX_VORTEX:
            {
                const uint8_t* const payload = message + 4;
                vortexPage = payload[0];

                switch (vortexPage)
                {
                case TACX_VORTEX_DATA_SPEED:
                    vortexUsingVirtualSpeed = (payload[1] >> 7) == 1;
                    vortexPower = payload[2] | ((payload[1] & 7) << 8); // watts
                    vortexSpeed = payload[4] | ((payload[3] & 3) << 8); // cm/s
                    // 0, 1, 2, 3 = none, running, new, failed
                    vortexCalibrationState = (payload[1] >> 5) & 3;
                    // unclear if this is set to anything
                    vortexCadence = payload[7];
                    break;

                case TACX_VORTEX_DATA_SERIAL:
                    // unk0 .. unk2 make up the serial number of the trainer
                    //uint8_t unk0 = payload[1];
                    //uint8_t unk1 = payload[2];
                    //uint32_t unk2 = payload[3] << 16 | payload[4] << 8 || payload[5];
                    // various flags, only known one is for virtual speed used
                    //uint8_t alarmStatus = payload[6] << 8 | payload[7];
                    break;

                case TACX_VORTEX_DATA_VERSION:
                {
                    //uint8_t major = payload[4];
                    //uint8_t minor = payload[5];
                    //uint8_t build = payload[6] << 8 | payload[7];
                    break;
                }

                case TACX_VORTEX_DATA_CALIBRATION:
                    // one byte for calibration, tacx treats this as signed
                    vortexCalibration = payload[5];
                    // duplicate of ANT deviceId, I think, necessary for issuing commands
                    vortexId = payload[6] << 8 | payload[7];
                    break;
                }

                break;
            }

            case ANTChannel::CHANNEL_TYPE_FOOTPOD:
            {
                channel = message[3];
                data_page = message[4];

                switch(data_page) {

                case FOOTPOD_MAIN_PAGE:
                    {
                        // SDM main format - strides loop at 255 should be x2
                        fpodInstant=false;
                        fpodStrides = message[10];
                        fpodSpeed = double(message[8]&0x0f) + (double(message[9]/256.00f));
                        fpodCadence = 0;
                    }
                    break;

                case FOOTPOD_SPEED_CADENCE:
                    {
                        // SDM instantaneous speed and cadence
                        // we ignore these
                        fpodInstant = true;
                        fpodStrides = 0;
                        fpodSpeed = double(message[7]&0x0f) + (double(message[8]/256.00f));
                        fpodCadence = double(message[6]) + (double(message[7] << 4) / 16.00f);
                    }

                }
            }
            break;

            case ANTChannel::CHANNEL_TYPE_TEMPE:
            {
                if (data_page == 1){
                    uint16_t val = message[10] | (message[11]<<8);
                    if (val != 0x8000)
                        tempValid = true;
                    temp = val;
                }
            }
            break;

            default:
                break;
            }
            break;

        case ANT_ACK_DATA:
            //qDebug()<<"ack data on channel" << channel;
            channel = message[3];
            data_page = message[4];

            switch(parent->antChannel[message[3]]->channel_type)
            {
            case ANTChannel::CHANNEL_TYPE_CONTROL:
                switch (data_page) {
                case ANT_CONTROL_GENERIC_CMD_PAGE:
                    controlSerial = (message[6] << 8) | message[5];
                    controlVendor = (message[8] << 8) | message[7];
                    controlSeq = message[9];
                    controlCmd = (message[11] << 8) | message[10];
                    break;

                default:
                    qDebug()<<"Unhandled remote control command page on channel" << channel;

                }
                break;

            default:
                qDebug()<<"Unhandled acknowledged ANT message on channel " << channel;
                break;

            }
            break;

        case ANT_BURST_DATA:
            break;
        case ANT_CHANNEL_EVENT:
            break;
        case ANT_CHANNEL_STATUS:
            break;
        case ANT_VERSION:
            break;
        case ANT_CAPABILITIES:
            break;
        case ANT_SERIAL_NUMBER:
            break;
        case ANT_CW_INIT:
            break;
        case ANT_CW_TEST:
            break;
        default:
            break; // shouldn't get here!
    }
}

// construct a message with all data passed (except sync and checksum)
ANTMessage::ANTMessage(const unsigned char len,
                       const unsigned char type,
                       const unsigned char b3,
                       const unsigned char b4,
                       const unsigned char b5,
                       const unsigned char b6,
                       const unsigned char b7,
                       const unsigned char b8,
                       const unsigned char b9,
                       const unsigned char b10,
                       const unsigned char b11)
{
    timestamp = get_timestamp();

    // encode the message
    data[0] = ANT_SYNC_BYTE;
    data[1] = len; // message payload length
    data[2] = type; // message type
    data[3] = b3;
    data[4] = b4;
    data[5] = b5;
    data[6] = b6;
    data[7] = b7;
    data[8] = b8;
    data[9] = b9;
    data[10] = b10;
    data[11] = b11;

    // compute the checksum and place after data
    unsigned char crc = 0;
    int i=0;
    for (; i< (len+3); i++) crc ^= data[i];
    data[i] = crc;

    length = i+1;
}

void ANTMessage::init()
{
    timestamp = get_timestamp();
    data_page = frequency = deviceType = transmitPower = searchTimeout = 0;
    transmissionType = networkNumber = channelType = channel = 0;
    channelPeriod = deviceNumber = 0;
    wheelMeasurementTime = crankMeasurementTime = lastMeasurementTime = 0;
    prevMeasurementTime = 0;
    instantHeartrate = heartrateBeats = 0;
    eventCount = 0;
    wheelRevolutions = crankRevolutions = 0;
    slope = period = torque = 0;
    sync = length = type = 0;
    srmOffset = srmSlope = srmSerial = 0;
    calibrationID = ctfID = 0;
    autoZeroStatus = autoZeroEnable = 0;
    pedalPowerContribution = false;
    pedalPower = 0;
    instantStartAngle = instantEndAngle = instantStartPeakAngle = instantEndPeakAngle = 0;
    riderPosition = 0;
    rightPCO = leftPCO = 0;
    leftTorqueEffectiveness = rightTorqueEffectiveness = 0;
    leftOrCombinedPedalSmoothness = rightPedalSmoothness = 0;
    fecSpeed = fecInstantPower = fecAccumulatedPower = 0;
    fecRawDistance = fecCadence = fecPage0x19EventCount = fecPage0x20EventCount = 0;
    fecPowerCalibRequired = fecResisCalibRequired = fecUserConfigRequired = false;
    fecPowerOverLimits = fecState = fecHRSource = 0;
    fecDistanceCapability = fecSpeedIsVirtual = false;
    fecEqtType = 0;
    fpodInstant = false;
    fpodSpeed=fpodCadence=0;
    fpodStrides=0;
    tempValid = false;
    temp=0;
    coreTemp = skinTemp = heatStrain = 0.0;
    tempQual = 0;
}

ANTMessage ANTMessage::resetSystem()
{
    return ANTMessage(1, ANT_SYSTEM_RESET);
}

ANTMessage ANTMessage::assignChannel(const unsigned char channel,
                                     const unsigned char type,
                                     const unsigned char network)
{
    return ANTMessage(3, ANT_ASSIGN_CHANNEL, channel, type, network);
}

ANTMessage ANTMessage::unassignChannel(const unsigned char channel)
{
    return ANTMessage(1, ANT_UNASSIGN_CHANNEL, channel);
}

ANTMessage ANTMessage::setLPSearchTimeout(const unsigned char channel,
                                        const unsigned char timeout)
{
    return ANTMessage(2, ANT_LP_SEARCH_TIMEOUT, channel, timeout);
}

ANTMessage ANTMessage::setHPSearchTimeout(const unsigned char channel,
                                        const unsigned char timeout)
{
    return ANTMessage(2, ANT_SEARCH_TIMEOUT, channel, timeout);
}

ANTMessage ANTMessage::requestMessage(const unsigned char channel,
                                      const unsigned char request)
{
    return ANTMessage(2, ANT_REQ_MESSAGE, channel, request);
}

ANTMessage ANTMessage::setChannelID(const unsigned char channel,
                                    const unsigned short device,
                                    const unsigned char devicetype,
                                    const unsigned char txtype)
{
    return ANTMessage(5, ANT_CHANNEL_ID, channel, device&0xff, (device>>8)&0xff, devicetype, txtype);
}

ANTMessage ANTMessage::setChannelPeriod(const unsigned char channel,
                                        const unsigned short period)
{
    return ANTMessage(3, ANT_CHANNEL_PERIOD, channel, period&0xff, (period>>8)&0xff);
}

ANTMessage ANTMessage::setChannelFreq(const unsigned char channel,
                                      const unsigned char frequency)
{
    // Channel Frequency = 2400 MHz + Channel RF Frequency Number * 1.0 MHz
    // The range is 0-124 with a default value of 66
    // ANT_SPORT_FREQ = 57
    // ANT_KICKR_FREQ = 52
    return ANTMessage(2, ANT_CHANNEL_FREQUENCY, channel, frequency);
}

ANTMessage ANTMessage::setNetworkKey(const unsigned char net,
                                     const unsigned char *key)
{
    return ANTMessage(9, ANT_SET_NETWORK, net, key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7]);
}

ANTMessage ANTMessage::setAutoCalibrate(const unsigned char channel,
                                        bool autozero)
{
    return ANTMessage(4, ANT_ACK_DATA, channel, ANT_SPORT_CALIBRATION_MESSAGE,
                                       ANT_SPORT_CALIBRATION_REQUEST_AUTOZERO_CONFIG,
                                       autozero ? ANT_SPORT_AUTOZERO_ON : ANT_SPORT_AUTOZERO_OFF);
}

ANTMessage ANTMessage::requestCalibrate(const unsigned char channel)
{
    return ANTMessage(4, ANT_ACK_DATA, channel, ANT_SPORT_CALIBRATION_MESSAGE,
                                       ANT_SPORT_CALIBRATION_REQUEST_MANUALZERO);
}

// see http://www.thisisant.com/images/Resources/PDF/ap2_rf_transceiver_module_errata.pdf
ANTMessage ANTMessage::ANTMessage::boostSignal(const unsigned char channel)
{
    // [A4][02][6A][XX][57][9B]
    return ANTMessage(2, 0x6A, channel, 0x57);
}

ANTMessage ANTMessage::open(const unsigned char channel)
{
    return ANTMessage(1, ANT_OPEN_CHANNEL, channel);
}

ANTMessage ANTMessage::close(const unsigned char channel)
{
    return ANTMessage(1, ANT_CLOSE_CHANNEL, channel);
}

ANTMessage ANTMessage::tacxVortexSetFCSerial(const uint8_t channel, const uint16_t setVortexId)
{
    return ANTMessage(9, ANT_BROADCAST_DATA, channel, 0x10, setVortexId >> 8, setVortexId & 0xFF,
                      0x55, // coupling request
                      0x7F, 0, 0, 0);
}

ANTMessage ANTMessage::tacxVortexStartCalibration(const uint8_t channel, const uint16_t vortexId)
{
    return ANTMessage(9, ANT_BROADCAST_DATA, channel, 0x10, vortexId >> 8, vortexId & 0xFF,
                      0, 0xFF /* signals calibration start */, 0, 0, 0);
}

ANTMessage ANTMessage::tacxVortexStopCalibration(const uint8_t channel, const uint16_t vortexId)
{
    return ANTMessage(9, ANT_BROADCAST_DATA, channel, 0x10, vortexId >> 8, vortexId & 0xFF,
                      0, 0x7F /* signals calibration stop */, 0, 0, 0);
}

ANTMessage ANTMessage::tacxVortexSetCalibrationValue(const uint8_t channel, const uint16_t vortexId, const uint8_t calibrationValue)
{
    return ANTMessage(9, ANT_BROADCAST_DATA, channel, 0x10, vortexId >> 8, vortexId & 0xFF,
                      0, 0x7F, calibrationValue, 0, 0);
}

ANTMessage ANTMessage::tacxVortexSetPower(const uint8_t channel, const uint16_t vortexId, const uint16_t power)
{
    return ANTMessage(9, ANT_BROADCAST_DATA, channel, 0x10, vortexId >> 8, vortexId & 0xFF,
                      0xAA, // power request
                      0, 0, // no calibration related data
                      power >> 8, power & 0xFF);
}

// kickr broadcast commands, lifted largely from the Wahoo SDK example: KICKRDemo/WFAntBikePowerCodec.cs
ANTMessage ANTMessage::kickrErgMode(const unsigned char channel, ushort usDeviceId, ushort usWatts, bool bSimSpeed)
{
    return ANTMessage(9, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_ERG_MODE, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8),
                                           (unsigned char)usWatts, (unsigned char)(usWatts>>8),
                                           (unsigned char)(bSimSpeed) ? 1 : 0);
}

ANTMessage ANTMessage::kickrSlopeMode(const unsigned char channel, ushort usDeviceId, ushort scale)
{
    return ANTMessage(7, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_RESISTANCE_MODE, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)scale, (unsigned char)(scale>>8));
}

ANTMessage ANTMessage::kickrSimMode(const unsigned char channel, ushort usDeviceId, float fWeight)
{
    // weight encoding
    ushort usWeight = (ushort)(fWeight * 100.0);

    return ANTMessage(7, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_SIM_MODE, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)usWeight, (unsigned char)(usWeight>>8));
}

ANTMessage ANTMessage::kickrStdMode(const unsigned char channel, ushort usDeviceId, ushort eLevel)
{
    return ANTMessage(7, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_STANDARD_MODE, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)eLevel, (unsigned char)(eLevel>>8));
}

ANTMessage ANTMessage::kickrFtpMode(const unsigned char channel, ushort usDeviceId, ushort usFtp, ushort usPercent)
{
    return ANTMessage(9, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_FTP_MODE, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)usFtp, (unsigned char)(usFtp>>8),
                                    (unsigned char)usPercent, (unsigned char)(usPercent>>8));
}

ANTMessage ANTMessage::kickrRollingResistance(const unsigned char channel, ushort usDeviceId, float fCrr)
{
    // crr encoding
    ushort usCrr = (ushort)(fCrr * 10000.0);

    return ANTMessage(7, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_CRR, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)usCrr, (unsigned char)(usCrr>>8));
}

ANTMessage ANTMessage::kickrWindResistance(const unsigned char channel, ushort usDeviceId, float fC)
{
    // wind resistance encoding
    ushort usC = (ushort)(fC * 1000.0);

    return ANTMessage(7, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_C, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)usC, (unsigned char)(usC>>8));
}

ANTMessage ANTMessage::kickrGrade(const unsigned char channel, ushort usDeviceId, float fGrade)
{

    // grade encoding
    if (fGrade > 1) fGrade = 1;
    if (fGrade < -1) fGrade = -1;

    fGrade = fGrade + 1;
    ushort usGrade = (ushort)(fGrade * 65536 / 2);

    return ANTMessage(7, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_GRADE, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)usGrade, (unsigned char)(usGrade>>8));
}

ANTMessage ANTMessage::kickrWindSpeed(const unsigned char channel, ushort usDeviceId, float mpsWindSpeed)
{
    // windspeed encoding
    if (mpsWindSpeed > 32.768f) mpsWindSpeed = 32.768f;
    if (mpsWindSpeed < -32.768f) mpsWindSpeed = -32.768f;

    // the wind speed is transmitted in 1/1000 mps resolution.
    mpsWindSpeed = mpsWindSpeed + 32.768f;
    ushort usSpeed = (ushort)(mpsWindSpeed * 1000);

    return ANTMessage(7, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_WIND_SPEED, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)usSpeed, (unsigned char)(usSpeed>>8));
}

ANTMessage ANTMessage::kickrWheelCircumference(const unsigned char channel, ushort usDeviceId, float mmCircumference)
{
    // circumference encoding
    ushort usCirc = (ushort)(mmCircumference * 10.0);

    return ANTMessage(7, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_SET_WHEEL_CIRCUMFERENCE, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8), // preamble
                                    (unsigned char)usCirc, (unsigned char)(usCirc>>8));
}

ANTMessage ANTMessage::kickrReadMode(const unsigned char channel, ushort usDeviceId)
{
    return ANTMessage(5, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_INIT_SPINDOWN, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8)); // preamble
}

ANTMessage ANTMessage::kickrInitSpindown(const unsigned char channel, ushort usDeviceId)
{
    return ANTMessage(5, ANT_BROADCAST_DATA, channel, // broadcast
           KICKR_READ_MODE, (unsigned char)usDeviceId, (unsigned char)(usDeviceId>>8)); // preamble
}

ANTMessage ANTMessage::fecSetResistance(const uint8_t channel, const uint8_t resistance) // unit is 0.5% => 200 = 100%
{
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      FITNESS_EQUIPMENT_BASIC_RESISTANCE_ID,
                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, resistance);
}

ANTMessage ANTMessage::fecSetTargetPower(const uint8_t channel, const uint16_t targetPower)
{
    // unit is 0.25W, but targetPower are full watts and theres no trainer with that precision anyway
    uint16_t powerValue = targetPower * 4;
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      FITNESS_EQUIPMENT_TARGET_POWER_ID,
                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, (uint8_t)(powerValue & 0xFF), (uint8_t)(powerValue >> 8));
}

ANTMessage ANTMessage::fecSetWindResistance(const uint8_t channel, const double windResistance, const uint8_t windSpeed, const uint8_t draftingFactor)
                                                                    //     0.00 < kg/m < 1.86         -127 < kph < + 127               0 < % < 100%
{
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      FITNESS_EQUIPMENT_WIND_RESISTANCE_ID,
                      0xFF, 0xFF, 0xFF, 0xFF, (uint8_t) (windResistance * 100), windSpeed, draftingFactor);
}

ANTMessage ANTMessage::fecSetTrackResistance(const uint8_t channel, const double grade, const double rollingResistance)
                                                             //     -200 < % < +200%                   0.0 <  < 0.0127
{
    uint16_t rawGradeValue = (uint16_t) ((grade+200.0) * 100.0);
    uint8_t  rollingResistanceValue = rollingResistance / 0.00005;
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      FITNESS_EQUIPMENT_TRACK_RESISTANCE_ID,
                      0xFF, 0xFF, 0xFF, 0xFF, (uint8_t)(rawGradeValue & 0xFF), (uint8_t)(rawGradeValue >> 8), rollingResistanceValue);
}

ANTMessage ANTMessage::fecRequestCapabilities(const uint8_t channel)
{
    // based on ANT+ Common Pages, Rev 2.4 p 14: 6.2  Common Data Page 70: Request Data Page
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      FITNESS_EQUIPMENT_REQUEST_DATA_PAGE,        // data page request
                      0xFF, 0xFF,  // reserved
                      0xFF, 0xFF,  // descriptors
                      0x04,        // requested transmission response
                      FITNESS_EQUIPMENT_TRAINER_CAPABILITIES_PAGE,        // requested page
                      0x01);       // request data page
}

ANTMessage ANTMessage::fecRequestCommandStatus(const uint8_t channel, const uint8_t page)
{
    // based on ANT+ Common Pages, Rev 2.4 p 14: 6.2  Common Data Page 70: Request Data Page
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      FITNESS_EQUIPMENT_REQUEST_DATA_PAGE,        // data page request
                      0xFF, 0xFF,  // reserved
                      0xFF, 0xFF,  // descriptors
                      0x01,        // requested transmission response (1 time only)
                      page,        // requested page (0x30 = resistance, 0x31 = power, 0x32 = wind, 0x33 = slope/track)
                      0x01);       // request data page
}

ANTMessage ANTMessage::fecRequestCalibration(const uint8_t channel, const uint8_t type)
{
    qDebug() << "Sending ANT_SPORT_CALIBRATION_MESSAGE (FE-C) on channel" << channel;

    // based on ANT+ Device Profile Fitness Equipment, Rev 4.1 p 38: 6.4.1  Data Page 1 – Calibration Request and Response Page
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      ANT_SPORT_CALIBRATION_MESSAGE,        // data page request
                      type,                                 // spindown, zero offset, or none (does not seem to cancel an ongoing calibration?)
                      0x00,                                 // reserved
                      0xFF,                                 // temperature - set to invalid in request
                      0xFF, 0xFF,                           // zero offset - set to invalid in request
                      0xFF, 0xFF);                          // spin down time - set to invalid in request
}

ANTMessage ANTMessage::requestPwrCalibration(const uint8_t channel, const uint8_t type)
{
    qDebug() << "Sending ANT_SPORT_CALIBRATION_MESSAGE (Power) on channel" << channel;

    // based on ANT+ Device Profile Bicycle Power, Rev 4.0 p 38: 6.4.1  Data Page 1 – Calibration Request and Response Page
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      ANT_SPORT_CALIBRATION_MESSAGE,        // data page request
                      type,                                 // calibration request type
                      0xFF, 0xFF,                           // reserved
                      0xFF, 0xFF,                           // reserved
                      0xFF, 0xFF);                          // reserved
}

ANTMessage ANTMessage::requestPwrCapabilities1(const uint8_t channel)
{
    // qDebug()<<channel<<"Requesting Capabilities sub-page 1 from power device";

    // based on ANT+ Common Pages, Rev 2.4 p 14: 6.2  Common Data Page 70: Request Data Page
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      ANT_REQUEST_DATA_PAGE,              // data page request
                      0xFF, 0xFF,                         // reserved
                      POWER_ADV_CAPABILITIES1_SUBPAGE,    // descriptor 1 sub page
                      0xFF,                               // descriptor 2
                      POWER_CAPABILITIES_RQST_NBR,        // requested transmission response
                      POWER_GETSET_PARAM_PAGE,            // requested page
                      ANT_RQST_MST_DATA_PAGE);            // request data page from master
}

ANTMessage ANTMessage::requestPwrCapabilities2(const uint8_t channel)
{
    // qDebug()<<channel<<"Requesting Capabilities sub-page 2 from power device";

    // based on ANT+ Common Pages, Rev 2.4 p 14: 6.2  Common Data Page 70: Request Data Page
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      ANT_REQUEST_DATA_PAGE,              // data page request
                      0xFF, 0xFF,                         // reserved
                      POWER_ADV_CAPABILITIES2_SUBPAGE,    // descriptor 1 sub page
                      0xFF,                               // descriptor 2
                      POWER_CAPABILITIES_RQST_NBR,        // requested transmission response
                      POWER_GETSET_PARAM_PAGE,            // requested page
                      ANT_RQST_MST_DATA_PAGE);            // request data page from master
}

ANTMessage ANTMessage::enablePwrCapabilities1(const uint8_t channel, const uint8_t capabilitiesMask, const uint8_t capabilitiesSetup)
{
    // qDebug()<<channel<<"Setup power sensor capabilities from sub-page 1 to"
    //     << qPrintable(QString("0x")+QString("%1").arg(capabilitiesSetup, 2, 16, QChar('0')).toUpper());

    // based on ANT+ Device Profile - Bicycle Power Rev 5.1
    // page 21: 4.5.2 Enabling Cycling Dynamics
    // page 71: 15.2.3 Subpage 0x04 – Rider Position Configuration
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      POWER_GETSET_PARAM_PAGE,              // data page
                      POWER_ADV_CAPABILITIES1_SUBPAGE,      // subpage
                      0xFF, 0xFF,                           // reserved
                      capabilitiesMask,                     // capabilities mask
                      0xFF,                                 // reserved
                      capabilitiesSetup,                    // capabilities details
                      0xFF);                                // reserved
}

ANTMessage ANTMessage::enablePwrCapabilities2(const uint8_t channel, const uint8_t capabilitiesMask, const uint8_t capabilitiesSetup)
{
    // qDebug()<<channel<<"Setup power sensor capabilities from sub-page 2 to"
    //     << qPrintable(QString("0x")+QString("%1").arg(capabilitiesSetup, 2, 16, QChar('0')).toUpper());

    // based on ANT+ Device Profile - Bicycle Power Rev 5.1
    // page 21: 4.5.2 Enabling Cycling Dynamics
    // page 71: 15.2.3 Subpage 0x04 – Rider Position Configuration
    return ANTMessage(9, ANT_ACK_DATA, channel,
                      POWER_GETSET_PARAM_PAGE,              // data page
                      POWER_ADV_CAPABILITIES2_SUBPAGE,      // subpage
                      0xFF, 0xFF,                           // reserved
                      capabilitiesMask,                     // capabilities mask
                      0xFF,                                 // reserved
                      capabilitiesSetup,                    // capabilities details
                      0xFF);                                // reserved
}

ANTMessage ANTMessage::controlDeviceAvailability(const uint8_t channel)
{
    // based on ANT+ Managed Network Document – Controls Device Profile, Rev 2.0 p 26: 8.2 Data Page 2 – Control Device Availability
    return ANTMessage(9, ANT_BROADCAST_DATA, channel,
                      ANT_CONTROL_DEVICE_AVAILABILITY_PAGE, // data page request
                      0x00, 0x00,                           // reserved
                      0x00, 0x00,                           // reserved
                      0x00, 0x00,                           // reserved
                      ANT_CONTROL_DEVICE_SUPPORT_GENERIC);  // support for generic remote controls
}



