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
    is_kickr=false;
    is_moxy=false;
    is_cinqo=0;
    is_old_cinqo=0;
    is_alt=0;
    manufacturer_id=0;
    product_id=0;
    product_version=0;
    device_number=0;
    device_id=0;
    state=ANT_UNASSIGN_CHANNEL;
    blanked=1;
    messages_received=0;
    messages_dropped=0;
    setId();
    srm_offset=400; // default relatively arbitrary, but matches common 'indoors' values
    burstInit();
    value2=value=0;
    status = Closed;
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

    // lets open the channel
    qDebug()<<"** OPENING CHANNEL"<<number<<"**";
    status = Opening;

    // start the transition process 
    attemptTransition(TRANSITION_START);
}

// close an ant channel assignment
void ANTChannel::close()
{
    emit lostInfo(number);
    lastMessage = ANTMessage();

    // lets shutdown
    qDebug()<<"** CLOSING CHANNEL"<<number<<"**";
    status = Closing;

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
        //qDebug()<<"broadcast event:"<<number;
        break;
    case ANT_ACK_DATA:
        ackEvent(ant_message);
        //qDebug()<<"ack data:"<<number;
        break;
    case ANT_CHANNEL_ID:
        channelId(ant_message);
        //qDebug()<<"channel id:"<<number;
        break;
    case ANT_BURST_DATA:
        burstData(ant_message);
        break;
    default:
        //qDebug()<<"dunno?"<<number;
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

    // we should reimplement this via ANTMessage at some point
    unsigned char *message=ant_message+2;

    //qDebug()<<number<<"channel event:"<< ANTMessage::channelEventMessage(*(message+1));
    if (MESSAGE_IS_RESPONSE_NO_ERROR(message)) {

        //qDebug()<<number<<"channel event response no error";
        attemptTransition(RESPONSE_NO_ERROR_MESSAGE_ID(message));

    } else if (MESSAGE_IS_EVENT_CHANNEL_CLOSED(message)) {

        //qDebug()<<number<<"channel event channel closed";
        if (status != Closing) {
            //qDebug()<<"we got closed!! re-open !!";
            status = Opening;
            attemptTransition(TRANSITION_START);
        } else {
            parent->sendMessage(ANTMessage::unassignChannel(number));
        }

    } else if (MESSAGE_IS_EVENT_RX_SEARCH_TIMEOUT(message)) {

        //qDebug()<<number<<"channel event rx timeout";
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

        //qDebug()<<number<<"channel event rx fail";
        messages_dropped++;
        double t=get_timestamp();

        if (t > (last_message_timestamp + timeout_drop)) {
            if (channel_type != CHANNEL_TYPE_UNUSED) emit dropInfo(number, messages_dropped, messages_received);
            // this is a hacky way to prevent the drop message from sending multiple times
            last_message_timestamp+=2*timeout_drop;
        }

    } else if (MESSAGE_IS_EVENT_RX_ACKNOWLEDGED(message)) {

        //this cannot possibly be correct >> exit(-10);

    } else if (MESSAGE_IS_EVENT_TRANSFER_TX_COMPLETED(message)) {

        // do nothing

    }  else {

        // usually a response event, so lets get a debug going
        //qDebug()<<number<<"channel event other ....."<<QString("0x%1 0x%2 %3 %4")
        //.arg(message[0], 1, 16).arg(message[1], 1, 16).arg(message[2]).arg(ANTMessage::channelEventMessage(message[2]));
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

// are we a moxy ?
void ANTChannel::checkMoxy()
{
    // we are a moxy !
    if (!is_moxy && manufacturer_id==76)
        is_moxy=1;
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
    //qDebug()<<number<<"broadcast event !";
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
        // pretty critical) -- because the USB stick needed a USB reset which we now
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
        checkMoxy();

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

                        // ignore the occasional spikes (reed switch)
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
                   nullCount = 0;
                   float cadence = 1024*60*revs / time;
                   parent->setCadence(cadence);
                   value2 = value = cadence;
               } else {
                   nullCount++;
                   if (nullCount >= 12) { parent->setCadence(0);
                                          value = 0;
                   }
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

                   if (is_moxy) /* do nothing for now */ ; //XXX fixme when moxy arrives XXX
                   else parent->setCadence(cadence);
                   value = cadence;

               } else {

                   nullCount++;
                   if (!is_moxy && nullCount >= 12) { parent->setCadence(0);
                        value = 0;
                   }
               }

               // now speed ...
               time = antMessage.wheelMeasurementTime - lastMessage.wheelMeasurementTime;
               revs = antMessage.wheelRevolutions - lastMessage.wheelRevolutions;
               if (time) {
                   dualNullCount = 0;

                   float rpm = 1024*60*revs / time;
                   if (is_moxy) /* do nothing for now */ ; //XXX fixme when moxy arrives XXX
                   else parent->setWheelRpm(rpm);
                   value2 = rpm;

               } else {

                    dualNullCount++;
                    if (!is_moxy && dualNullCount >= 12) {
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

            //moxy
            case CHANNEL_TYPE_MOXY:
            {
                value = antMessage.tHb;
                value2 = antMessage.newsmo2;
                parent->setHb(value2, value);
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

    if (is_kickr) {
        qDebug()<<number<<"KICKR DETECTED VIA CHANNEL ID EVENT";
    }

    // tell controller we got a new channel id
    setId();
    emit channelInfo(number, device_number, device_id);

    // if we were searching,
    if (channel_type_flags&CHANNEL_TYPE_QUICK_SEARCH) {
        //qDebug()<<number<<"change timeout setting";
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

// TRANSITIONING FROM NOT OPEN TO OPEN AND BACK
// this is basically a transition from an unassigned channel
// to one with a device open on it and works from top to bottom
// it is first called by open() to unassign channel and then
// after each message response from the device we get called here
// to perform the next action in the sequence
void ANTChannel::attemptTransition(int message_id)
{
    // ignore all the nonsense if we are not trying to transition !
    if (status != Closing && status != Opening) {
        //qDebug()<<number<<"ignore transition"<<message_id;
        return;
    }

    const ant_sensor_type_t *st;
    int previous_state=state;
    st=&(parent->ant_sensor_types[channel_type]);

    //qDebug()<<number<<"type="<<channel_type<<"device type="<<device_id<<"freq="<<st->frequency;

    // update state
    state=message_id;

    // do transitions from top status to bottom status !
    switch (state) {

    case TRANSITION_START:
        //qDebug()<<number<<"TRANSITION start";

        // unassign regardless of status
        parent->sendMessage(ANTMessage::unassignChannel(number)); // unassign whatever we had before

        // drops through into assign channel because if the channel currently has no
        // assignment the unassign channel message will generate an error response not
        // an unassign channel response. But we don't really know what state the device
        // might be in, so we unassign then assign at the beginning of a transition

    case ANT_UNASSIGN_CHANNEL:
        //qDebug()<<number<<"TRANSITION from unassigned";

        // assign and set channel id all in one
        parent->sendMessage(ANTMessage::assignChannel(number, CHANNEL_TYPE_RX, st->network)); // receive channel on network 1

        device_id=st->device_id;
        setId();
        break;

    case ANT_ASSIGN_CHANNEL:

        //qDebug()<<number<<"TRANSITION from assign channel";

        parent->sendMessage(ANTMessage::setChannelID(number, 0, device_id, 0)); // lets go back to allowing anything
        break;

    case ANT_CHANNEL_ID:
        //qDebug()<<number<<"TRANSITION from channel id";
        //qDebug()<<number<<"**** adjust timeout";
        if (channel_type & CHANNEL_TYPE_QUICK_SEARCH) {
            parent->sendMessage(ANTMessage::setSearchTimeout(number, (int)(timeout_scan/2.5)));
        } else {
            parent->sendMessage(ANTMessage::setSearchTimeout(number, (int)(timeout_lost/2.5)));
        }
        break;

    case ANT_SEARCH_TIMEOUT:
        //qDebug()<<number<<"TRANSITION from search timeout";
        if (previous_state==ANT_CHANNEL_ID) {
            // continue down the initialization chain
            //qDebug()<<"set channel period in timeout"<<st->period;
            parent->sendMessage(ANTMessage::setChannelPeriod(number, st->period));
        } else {
            // we are setting the ant_search timeout after connected
            // we'll just pretend this never happened
            state=previous_state;
        }
        break;

    case ANT_CHANNEL_PERIOD:
        //qDebug()<<number<<"TRANSITION from channel period";
        //qDebug()<<"set channel freq in timeout"<<st->frequency;
        parent->sendMessage(ANTMessage::setChannelFreq(number, st->frequency));
        break;

    case ANT_CHANNEL_FREQUENCY:
        //qDebug()<<number<<"TRANSITION from channel freq";
        parent->sendMessage(ANTMessage::open(number));
        mi.initialise();
        break;

    case ANT_OPEN_CHANNEL:
        //qDebug()<<number<<"TRANSITION from open (do nothing)";
        status = Open;
        //qDebug()<<"** CHANNEL"<<number<<"NOW OPEN **";
        //parent->sendMessage(ANTMessage::open(number));
        break;

    case ANT_CLOSE_CHANNEL:
        // next step is unassign and start over
        // but we must wait until event_channel_closed
        // which is its own channel event
        state=MESSAGE_RECEIVED;
        //qDebug()<<number<<"TRANSITION from closed";
        status = Closed;
        //qDebug()<<"** CHANNEL"<<number<<"NOW CLOSED **";
        break;

    default:
        break;
    }
}

// Calibrate... needs fixing in version 3.1
// request the device on this channel calibrates itself
void ANTChannel::requestCalibrate() {
  parent->sendMessage(ANTMessage::requestCalibrate(number));
}
