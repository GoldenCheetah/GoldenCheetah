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
#include <QDebug>

static float timeout_blanking=2.0;  // time before reporting stale data, seconds
static float timeout_drop=2.0; // time before reporting dropped message
static float timeout_scan=10.0; // time to do initial scan
static float timeout_lost=30.0; // time to do more thorough scan

ANTChannel::ANTChannel(int number, ANT *parent) : parent(parent), number(number)
{
    init();
}

void
ANTChannel::init()
{
    channel_type=CHANNEL_TYPE_UNUSED;
    channel_type_flags=0;
    is_cinqo=0;
    is_old_cinqo=0;
    is_alt=0;
    control_channel=NULL;
    manufacturer_id=0;
    product_id=0;
    product_version=0;
    device_number=0;
    device_id=0;
    channel_assigned=0;
    state=ANT_UNASSIGN_CHANNEL;
    blanked=1;
    messages_received=0;
    messages_dropped=0;
    setId();
    srm_offset=400; // default relatively arbitrary, but matches common 'indoors' values
    burstInit();
    value2=value=0;
}

//
// channel id is in the form nnnnx where nnnn is the device number
// and x is the channel type (p)ower, (c) adence etc the full list
// can be found in ANT.cpp when initialising ant_sensor_types
//
void ANTChannel::setId()
{
    if (channel_type==CHANNEL_TYPE_UNUSED) {
        strcpy(id, "none");
    } else {
        snprintf(id, 10, "%d%c", device_number, parent->ant_sensor_types[channel_type].suffix);
    }
}

// The description of the device
const char * ANTChannel::getDescription()
{
    return parent->ant_sensor_types[channel_type].descriptive_name;
}

// Get device type
int ANTChannel::interpretDescription(char *description)
{
    const ant_sensor_type_t *st=parent->ant_sensor_types;

    do {
        if (0==strcmp(st->descriptive_name,description))
            return st->type;
    } while (++st, st->type != CHANNEL_TYPE_GUARD);
    return -1;
}

// Open an ant channel assignment.
void ANTChannel::open(int device, int chan_type)
{
    channel_type=chan_type;
    channel_type_flags = CHANNEL_TYPE_QUICK_SEARCH ;
    device_number=device;
    setId();

    attemptTransition(ANT_UNASSIGN_CHANNEL);
}

// close an ant channel assignment
void ANTChannel::close()
{
    emit lostInfo(number);
    lastMessage = ANTMessage();
    parent->sendMessage(ANTMessage::close(number));
    init();
}

//
// The main read loop is in ANT.cpp, it will pass us
// the inbound message received for our channel.
void ANTChannel::receiveMessage(unsigned char *ant_message)
{
    switch (ant_message[2]) {
    case ANT_CHANNEL_EVENT:
        channelEvent(ant_message);
        break;
    case ANT_BROADCAST_DATA:
        broadcastEvent(ant_message);
        break;
    case ANT_ACK_DATA:
        ackEvent(ant_message);
        break;
    case ANT_CHANNEL_ID:
        channelId(ant_message);
        break;
    case ANT_BURST_DATA:
        burstData(ant_message);
        break;
    default:
        break; //errors silently ignored for now, would indicate hardware fault.
    }

    if (get_timestamp() > blanking_timestamp + timeout_blanking) {
        if (!blanked) {
            blanked=1;
            value2=value=0;
            emit staleInfo(number);
        }
    } else blanked=0;
}


// process a channel event message
// would be good to refactor to use ANTMessage at some point
// but not compelling reason to do so at this point and might
// break existing code.
void ANTChannel::channelEvent(unsigned char *ant_message) {

    unsigned char *message=ant_message+2;

//qDebug()<<"channel event:"<< ANTMessage::channelEventMessage(*(message+1));

    if (MESSAGE_IS_RESPONSE_NO_ERROR(message)) {

        attemptTransition(RESPONSE_NO_ERROR_MESSAGE_ID(message));

    } else if (MESSAGE_IS_EVENT_CHANNEL_CLOSED(message)) {

        parent->sendMessage(ANTMessage::unassignChannel(number));

    } else if (MESSAGE_IS_EVENT_RX_SEARCH_TIMEOUT(message)) {

        // timeouts are normal for search channel
        if (channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {

            channel_type_flags &= ~CHANNEL_TYPE_QUICK_SEARCH;
            channel_type_flags |= CHANNEL_TYPE_WAITING;

            emit searchTimeout(number);

        } else {

            emit lostInfo(number);

            // Don't wipe out the channel settings when the search times out,
            // else can not reconnect to the device once back in range..
            //channel_type=CHANNEL_TYPE_UNUSED;
            //channel_type_flags=0;
            //device_number=0;
            //value2=value=0;
            //setId();

            parent->sendMessage(ANTMessage::unassignChannel(number));
        }

    } else if (MESSAGE_IS_EVENT_RX_FAIL(message)) {

        messages_dropped++;
        double t=get_timestamp();

        if (t > (last_message_timestamp + timeout_drop)) {
            if (channel_type != CHANNEL_TYPE_UNUSED) emit dropInfo(number, messages_dropped, messages_received);
            // this is a hacky way to prevent the drop message from sending multiple times
            last_message_timestamp+=2*timeout_drop;
        }

    } else if (MESSAGE_IS_EVENT_RX_ACKNOWLEDGED(message)) {

        exit(-10);

    } else if (MESSAGE_IS_EVENT_TRANSFER_TX_COMPLETED(message)) {
        // do nothing
    } 
}

// if this is a quarq cinqo then record that fact
// was probably interesting to quarqd, but less so for us!
void ANTChannel::checkCinqo()
{

    int version_hi, version_lo;
    version_hi=(product_version >> 8) &0xff;
    version_lo=(product_version & 0xff);

    if (!(mi.first_time_manufacturer || mi.first_time_product)) {
        if ((product_id == 1) && (manufacturer_id==7)) {
            // we are a cinqo, were we aware of this?
            is_cinqo=1;

            // are we an old-version or new-version cinqo?
            is_old_cinqo = ((version_hi <= 17) && (version_lo==10));
        }
    }
}

// notify we have a cinqo, does nothing
void ANTChannel::sendCinqoSuccess() {}

//
// We got a broadcast event -- this is where inbound
// telemetry gets processed, and for many message types
// we need to remember previous messages to look at the
// deltas during the period
//
void ANTChannel::broadcastEvent(unsigned char *ant_message)
{
    ANTMessage antMessage(parent, ant_message);
    bool savemessage = true; // flag to stop lastmessage being
                             // overwritten for standard power
                             // messages

    unsigned char *message=ant_message+2;
    double timestamp=get_timestamp();

    messages_received++;
    last_message_timestamp=timestamp;

    if (messages_received <= 1) {

        // this is mega important! -- when we get broadcast data from a device
        // we ask it to identify itself, then when the channel id message is
        // received we set our channel id to that received. So, if the message
        // below is not sent, we will never set channel properly.

        // The recent bug with not being able to "pair" intermittently, was caused
        // by the write below failing (and any write really, but the one below being
        // pretty critical) -- because the USB stick needed a USB reset which we know
        // do every time we open the USB device
        parent->sendMessage(ANTMessage::requestMessage(number, ANT_CHANNEL_ID));
        blanking_timestamp=get_timestamp();
        blanked=0;
        return; // because we can't associate a channel id with the message yet
    }

    // for automatically opening quarq channel on early cinqo
    if (MESSAGE_IS_PRODUCT(message)) {

        mi.first_time_product= false;
        product_version&=0x00ff;
        product_version|=(PRODUCT_SW_REV(message))<<8;
        checkCinqo();

    } else if (MESSAGE_IS_MANUFACTURER(message)) {

        mi.first_time_manufacturer= false;
        product_version&=0xff00;
        product_version|=MANUFACTURER_HW_REV(message);
        manufacturer_id=MANUFACTURER_MANUFACTURER_ID(message);
        product_id=MANUFACTURER_MODEL_NUMBER_ID(message);
        checkCinqo();

    } else {
        //
        // We got some telemetry on this channel
        //
        if (lastMessage.type != 0) {

           switch (channel_type) {

           // Power
           case CHANNEL_TYPE_POWER:
           case CHANNEL_TYPE_QUARQ:
           case CHANNEL_TYPE_FAST_QUARQ:
           case CHANNEL_TYPE_FAST_QUARQ_NEW:

                // what kind of power device is this?
                switch(antMessage.data_page) {

                case ANT_SPORT_CALIBRATION_MESSAGE:
                {
                    // Always ack calibs unless they are acks too!
                    if (antMessage.data[6] != 0xAC) {
                        antMessage.data[6] = 0xAC;
                        parent->sendMessage(antMessage);
                    }

                    // each device sends a different type
                    // of calibration message...
                    switch (antMessage.calibrationID) {

                    case ANT_SPORT_SRM_CALIBRATIONID:

                        switch (antMessage.ctfID) {

                            case 0x01: // offset
                                // if we're getting calibration messages then
                                // we should be coasting, so power and cadence
                                // will be zero
                                srm_offset = antMessage.srmOffset;
                                is_alt ? parent->setAltWatts(0) : parent->setWatts(0);
                                parent->setCadence(0);
                                value2=value=0;
                                break;

                            case 0x02: // slope
                                break;

                            case 0x03: // serial number
                                break;

                            default:
                                break;

                            }
                            break;

                        default: 
                            break;
                    }

                } // ANT_SPORT_CALIBRATION
                savemessage = false; // we don't want to overwrite other messages
                break;

                //
                // SRM - crank torque frequency
                //
                case ANT_CRANKSRM_POWER: // 0x20 - crank torque (SRM)
                {
                    uint16_t period = antMessage.period - lastMessage.period;
                    uint16_t torque = antMessage.torque - lastMessage.torque;
                    float time = (float)period / (float)2000.00;

                    if (time && antMessage.slope && period) {

                        nullCount = 0;
                        float torque_freq = torque / time - 420/*srm_offset*/;
                        float nm_torque = 10.0 * torque_freq / antMessage.slope;
                        float cadence = 2000.0 * 60 * (antMessage.eventCount - lastMessage.eventCount) / period;
                        float power = 3.14159 * nm_torque * cadence / 30;

                        // ignore the occassional spikes (reed switch)
                        if (power >= 0 && power < 2501 && cadence >=0 && cadence < 256) {
                            value2 = value = power;
                            is_alt ? parent->setAltWatts(power) : parent->setWatts(power);
                            parent->setCadence(cadence);
                        }

                    } else {

                        nullCount++;
                        if (nullCount >= 4) { // 4 messages on an SRM
                            value2 = value = 0;
                            is_alt ? parent->setAltWatts(0) : parent->setWatts(0);
                            parent->setCadence(0);
                        }
                    }
                }
                break;

                //
                // Powertap - wheel torque
                //
                case ANT_WHEELTORQUE_POWER: // 0x11 - wheel torque (Powertap)
                {
                    uint8_t events = antMessage.eventCount - lastMessage.eventCount;
                    uint16_t period = antMessage.period - lastMessage.period;
                    uint16_t torque = antMessage.torque - lastMessage.torque;

                    if (events && period) {

                        nullCount = 0;

                        float nm_torque = torque / (32.0 * events);
                        float wheelRPM = 2048.0 * 60.0 * events / period;
                        float power = 3.14159 * nm_torque * wheelRPM / 30;

                        value2 = value = power;
                        parent->setWheelRpm(wheelRPM);
                        is_alt ? parent->setAltWatts(power) : parent->setWatts(power);

                    } else {
                        nullCount++;

                        if (nullCount >= 4) { // 4 messages on Powertap according to specs
                            parent->setWheelRpm(0);
                            value2 = value = 0;
                            is_alt ? parent->setAltWatts(0) : parent->setWatts(0);
                        }
                    }
                }
                break;

                //
                // Standard Power - interleaved with other messages 1 per second
                //                  NOTE: Standard power messages are interleaved
                //                        with other power broadcast messages and so
                //                        we need to make sure lastmessage is NOT
                //                        updated with this message and instead we
                //                        store in a special lastStdPwrMessage
                //
                case ANT_STANDARD_POWER: // 0x10 - standard power
                {
                    uint8_t events = antMessage.eventCount - lastStdPwrMessage.eventCount;
                    if (lastStdPwrMessage.type && events) {
                        stdNullCount = 0;
                        is_alt ? parent->setAltWatts(antMessage.instantPower) : parent->setWatts(antMessage.instantPower);
                        value2 = value = antMessage.instantPower;
                        parent->setCadence(antMessage.instantCadence); // cadence
                    } else {
                       stdNullCount++;
                       if (stdNullCount >= 6) { //6 for standard power according to specs
                           parent->setCadence(0);
                           is_alt ? parent->setAltWatts(0) : parent->setWatts(0);
                           value2 = value = 0;
                       }
                    }
                    lastStdPwrMessage = antMessage;
                    savemessage = false;
                }
                break;


                //
                // Quarq - Crank torque
                //
                case ANT_CRANKTORQUE_POWER: // 0x12 - crank torque (Quarq)
                {
                    uint8_t events = antMessage.eventCount - lastMessage.eventCount;
                    uint16_t period = antMessage.period - lastMessage.period;
                    uint16_t torque = antMessage.torque - lastMessage.torque;

                    if (events && period && lastMessage.period) {
                        nullCount = 0;

                        float nm_torque = torque / (32.0 * events);
                        float cadence = 2048.0 * 60.0 * events / period;
                        float power = 3.14159 * nm_torque * cadence / 30;

                        parent->setCadence(cadence);
                        is_alt ? parent->setAltWatts(power) : parent->setWatts(power);
                        value2 = value = power;

                    } else {
                        nullCount++;
                        if (nullCount >= 4) { // 4 on a quarq according to specs
                            parent->setCadence(0);
                            is_alt ? parent->setAltWatts(0) : parent->setWatts(0);
                            value2 = value = 0;
                        }
                    }
                }
                break;

                default: // UNKNOWN
                    break;
            }
            break;

           // HR
           case CHANNEL_TYPE_HR:
           {
               // cadence first...
               uint16_t time = antMessage.measurementTime - lastMessage.measurementTime;
               if (time) {
                   nullCount = 0;
                   parent->setBPM(antMessage.instantHeartrate);
                   value2 = value = antMessage.instantHeartrate;

                    // lets emit a signal for collected HR R-R data
                    emit rrData(antMessage.measurementTime, antMessage.heartrateBeats, antMessage.instantHeartrate);

               } else {
                   nullCount++;
                   if (nullCount >= 12) {
                        parent->setBPM(0); // 12 according to the docs
                        value2 = value = 0;
                    }
               }
           }
           break;

           // Cadence
           case CHANNEL_TYPE_CADENCE:
           {
               uint16_t time = antMessage.crankMeasurementTime - lastMessage.crankMeasurementTime;
               uint16_t revs = antMessage.crankRevolutions - lastMessage.crankRevolutions;
               if (time) {
                   float cadence = 1024*60*revs / time;
                   parent->setCadence(cadence);
                   value2 = value = cadence;
               }
           }
           break;

           // Speed and Cadence
           case CHANNEL_TYPE_SandC:
           {
               // cadence first...
               uint16_t time = antMessage.crankMeasurementTime - lastMessage.crankMeasurementTime;
               uint16_t revs = antMessage.crankRevolutions - lastMessage.crankRevolutions;
               if (time) {
                   nullCount = 0;
                   float cadence = 1024*60*revs / time;
                   parent->setCadence(cadence);
                   value = cadence;
               } else {
                   nullCount++;
                   if (nullCount >= 12) { parent->setCadence(0);
                                          value = 0;
                    }
               }

               // now speed ...
               time = antMessage.wheelMeasurementTime - lastMessage.wheelMeasurementTime;
               revs = antMessage.wheelRevolutions - lastMessage.wheelRevolutions;
               if (time) {
                   dualNullCount = 0;

                   float rpm = 1024*60*revs / time;
                   parent->setWheelRpm(rpm);
                   value2 = rpm;
               } else {

                    dualNullCount++;
                    if (dualNullCount >= 12) {
                        parent->setWheelRpm(0);
                        value2 = 0;
                    }
               }
           }
           break;

           // Speed
           case CHANNEL_TYPE_SPEED:
           {
               uint16_t time = antMessage.wheelMeasurementTime - lastMessage.wheelMeasurementTime;
               uint16_t revs = antMessage.wheelRevolutions - lastMessage.wheelRevolutions;
               if (time) {
                   nullCount=0;
                   float rpm = 1024*60*revs / time;
                   parent->setWheelRpm(rpm);
                   value2=value=rpm;
               } else {
                   nullCount++;
                   if (nullCount >= 12) parent->setWheelRpm(0);
                   value2=value=0;
               }
           }
           break;

           default:
             break; // unknown?
           }

        } else {

            // reset nullCount if receiving first telemetry update
            stdNullCount = dualNullCount = nullCount = 0;
            value2 = value = 0;
        }

        // we don't overwrite for Standard Power messages
        // these are maintained separately in lastStdPwrMessage
        if (savemessage) lastMessage = antMessage;
    }
}

// we got an acknowledgement, so lets process it
// does nothing for now
void ANTChannel::ackEvent(unsigned char * /*ant_message*/) { }


// we got a channel ID notification
void ANTChannel::channelId(unsigned char *ant_message) {

    unsigned char *message=ant_message+2;

    // channel id data
    device_number=CHANNEL_ID_DEVICE_NUMBER(message);
    device_id=CHANNEL_ID_DEVICE_TYPE_ID(message);
    state=MESSAGE_RECEIVED;

    // high nibble of transmission type used to indicate
    // it is a kick, A0 gives the game away :)
    is_kickr = (device_id == ANT_SPORT_POWER_TYPE) && ((CHANNEL_ID_TRANSMISSION_TYPE(message)&0xF0) == 0xA0);

    emit channelInfo(number, device_number, device_id);
    setId();

    // if we were searching,
    if (channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
        parent->sendMessage(ANTMessage::setSearchTimeout(number, (int)(timeout_lost/2.5)));
    }
    channel_type_flags &= ~CHANNEL_TYPE_QUICK_SEARCH;
}

// get ready to burst
void ANTChannel::burstInit() {
    rx_burst_data_index=0;
    rx_burst_next_sequence=0;
    rx_burst_disposition=NULL;
}

// are we in the middle of a search?
int ANTChannel::isSearching() {
    return ((channel_type_flags & (CHANNEL_TYPE_WAITING | CHANNEL_TYPE_QUICK_SEARCH)) || (state != MESSAGE_RECEIVED));
}


// receive burst data
void ANTChannel::burstData(unsigned char *ant_message) {

    unsigned char *message=ant_message+2;
    char seq=(message[1]>>5)&0x3;
    char last=(message[1]>>7)&0x1;
    const unsigned char next_sequence[4]={1,2,3,1};

    if (seq!=rx_burst_next_sequence) {
        // we don't handle burst data at present.
    } else {

        int len=ant_message[ANT_OFFSET_LENGTH]-3;

        if ((rx_burst_data_index + len)>(RX_BURST_DATA_LEN)) {
            len = RX_BURST_DATA_LEN-rx_burst_data_index;
        }

        rx_burst_next_sequence=next_sequence[(int)seq];
        memcpy(rx_burst_data+rx_burst_data_index, message+2, len);
        rx_burst_data_index+=len;
    }

    if (last) {
        if (rx_burst_disposition) {
          // we don't handle burst data at present.
        }
        burstInit();
    }
}

// choose this one..
void ANTChannel::setChannelID(int device_number, int device_id, int /*txtype*/)
{
    parent->sendMessage(ANTMessage::setChannelID(number, device_number, device_id, 0)); // lets go back to allowing anything
    parent->sendMessage(ANTMessage::open(number)); // lets go back to allowing anything
}

void ANTChannel::attemptTransition(int message_id)
{

    const ant_sensor_type_t *st;
    int previous_state=state;
    st=&(parent->ant_sensor_types[channel_type]);

    // update state
    state=message_id;

    // do transitions from top status to bottom status !
    switch (state) {

    case ANT_CLOSE_CHANNEL:
        // next step is unassign and start over
        // but we must wait until event_channel_closed
        // which is its own channel event
        state=MESSAGE_RECEIVED;
        break;

    case ANT_UNASSIGN_CHANNEL:
        channel_assigned=0;

        // lets make sure this channel is assigned to our network
        // regardless of its current state.
        parent->sendMessage(ANTMessage::unassignChannel(number)); // unassign whatever we had before

        // reassign to whatever we need!
        parent->sendMessage(ANTMessage::assignChannel(number, 0, st->network)); // recieve channel on network 1
        device_id=st->device_id;
        parent->sendMessage(ANTMessage::setChannelID(number, 0, device_id, 0)); // lets go back to allowing anything
        setId();
        break;

    case ANT_ASSIGN_CHANNEL:
        channel_assigned=1;
        parent->sendMessage(ANTMessage::setChannelID(number, device_number, device_id, 0));
        break;

    case ANT_CHANNEL_ID:
        if (channel_type & CHANNEL_TYPE_QUICK_SEARCH) {
            parent->sendMessage(ANTMessage::setSearchTimeout(number, (int)(timeout_scan/2.5)));
        } else {
            parent->sendMessage(ANTMessage::setSearchTimeout(number, (int)(timeout_lost/2.5)));
        }
        break;

    case ANT_SEARCH_TIMEOUT:
        if (previous_state==ANT_CHANNEL_ID) {
            // continue down the intialization chain
            parent->sendMessage(ANTMessage::setChannelPeriod(number, st->period));
        } else {
            // we are setting the ant_search timeout after connected
            // we'll just pretend this never happened
            state=previous_state;
        }
        break;

    case ANT_CHANNEL_PERIOD:
        parent->sendMessage(ANTMessage::setChannelFreq(number, st->frequency));
        break;

    case ANT_CHANNEL_FREQUENCY:
        parent->sendMessage(ANTMessage::open(number));
        mi.initialise();
        break;

    case ANT_OPEN_CHANNEL:
        parent->sendMessage(ANTMessage::open(number));
        break;

    default:
        break;
    }
}

// set channel timeout
void ANTChannel::setTimeout(int seconds)
{
    parent->sendMessage(ANTMessage::setSearchTimeout(number, seconds/2.5));
}

// Calibrate... needs fixing in version 3.1
// request the device on this channel calibrates itselt
void ANTChannel::requestCalibrate() {
  parent->sendMessage(ANTMessage::requestCalibrate(number));
}
