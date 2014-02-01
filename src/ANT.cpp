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

//------------------------------------------------------------------------
// This code has been created by folding the ANT.cpp source with
// the Quarqd source provided by Mark Rages and the Serial device
// code from Computrainer.cpp
//------------------------------------------------------------------------

#include "ANT.h"
#include "ANTMessage.h"
#include <QMessageBox>
#include <QTime>
#include <QProgressDialog>
#include <QtDebug>
#include "RealtimeData.h"

#ifdef Q_OS_LINUX // to get stat /dev/xxx for major/minor
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

/* Control status */
#define ANT_RUNNING  0x01
#define ANT_PAUSED   0x02

// network key
const unsigned char ANT::key[8] = { 0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45 };

// supported sensor types
const ant_sensor_type_t ANT::ant_sensor_types[] = {
  { ANTChannel::CHANNEL_TYPE_UNUSED, 0, 0, 0, 0, "Unused", '?', "" },
  { ANTChannel::CHANNEL_TYPE_HR, ANT_SPORT_HR_PERIOD, ANT_SPORT_HR_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Heartrate", 'h', ":images/IconHR.png" },
  { ANTChannel::CHANNEL_TYPE_POWER, ANT_SPORT_POWER_PERIOD, ANT_SPORT_POWER_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Power", 'p', ":images/IconPower.png" },
  { ANTChannel::CHANNEL_TYPE_SPEED, ANT_SPORT_SPEED_PERIOD, ANT_SPORT_SPEED_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Speed", 's', ":images/IconSpeed.png" },
  { ANTChannel::CHANNEL_TYPE_CADENCE, ANT_SPORT_CADENCE_PERIOD, ANT_SPORT_CADENCE_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Cadence", 'c', ":images/IconCadence.png" },
  { ANTChannel::CHANNEL_TYPE_SandC, ANT_SPORT_SandC_PERIOD, ANT_SPORT_SandC_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Speed + Cadence", 'd', ":images/IconCadence.png" },
  { ANTChannel::CHANNEL_TYPE_GUARD, 0, 0, 0, 0, "", '\0', "" }
};

//
// The ANT class is a worker thread, reading/writing to a local
// Garmin ANT+ serial device. It maintains local state and telemetry.
// It is controlled by an ANTController, which starts/stops and will
// request telemetry and send commands to assign channels etc
//
// ANTController sits between the RealtimeWindow and the ANT worker
// thread and is part of the GC architecture NOT related to the
// hardware controller.
//
ANT::ANT(QObject *parent, DeviceConfiguration *devConf) : QThread(parent), devConf(devConf)
{
    qRegisterMetaType<ANTMessage>("ANTMessage");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<struct timeval>("struct timeval");

    // device status and settings
    Status=0;
    deviceFilename = devConf ? devConf->portSpec : "";
    baud=115200;
    powerchannels=0;
    configuring = false;

    // state machine
    state = ST_WAIT_FOR_SYNC;
    length = bytes = 0;
    checksum = ANT_SYNC_BYTE;

    // ant ids - may not be configured of course
    if (devConf && devConf->deviceProfile.length())
        antIDs = devConf->deviceProfile.split(",");
    else
        antIDs.clear();

    // setup the channels
    for (int i=0; i<ANT_MAX_CHANNELS; i++) {

        // create the channel
        antChannel[i] = new ANTChannel(i, this);

        // connect up its signals
        connect(antChannel[i], SIGNAL(channelInfo(int,int,int)), this, SLOT(channelInfo(int,int,int)));
        connect(antChannel[i], SIGNAL(dropInfo(int,int,int)), this, SLOT(dropInfo(int,int,int)));
        connect(antChannel[i], SIGNAL(lostInfo(int)), this, SLOT(lostInfo(int)));
        connect(antChannel[i], SIGNAL(staleInfo(int)), this, SLOT(staleInfo(int)));
        connect(antChannel[i], SIGNAL(searchTimeout(int)), this, SLOT(slotSearchTimeout(int)));
        connect(antChannel[i], SIGNAL(searchComplete(int)), this, SLOT(slotSearchComplete(int)));

        // R-R data
        connect(antChannel[i], SIGNAL(rrData(uint16_t, uint8_t, uint8_t)), this, SIGNAL(rrData(uint16_t, uint8_t, uint8_t)));
    }

    // on windows and linux we use libusb to read from USB2
    // sticks, if it is not available we use stubs
#if defined GC_HAVE_LIBUSB
    usbMode = USBNone;
    usb2 = new LibUsb(TYPE_ANT);
#endif
    channels = 0;
}

ANT::~ANT()
{
#if defined GC_HAVE_LIBUSB
    delete usb2;
#endif
}

void ANT::setDevice(QString x)
{
    deviceFilename = x;
}

void ANT::setBaud(int x)
{
    baud = x;
}

double ANT::channelValue2(int channel)
{
    return antChannel[channel]->channelValue2();
}
double ANT::channelValue(int channel)
{
    return antChannel[channel]->channelValue();
}

void ANT::setWheelRpm(float x) {
    telemetry.setWheelRpm(x);

    // devConf will be NULL if we are are running the add device wizard
    // we can default to the global setting
    if (devConf) telemetry.setSpeed(x * devConf->wheelSize / 1000 * 60 / 1000);
    else telemetry.setSpeed(x * appsettings->value(NULL, GC_WHEELSIZE, 2100).toInt() / 1000 * 60 / 1000);
}

/*======================================================================
 * Main thread functions; start, stop etc
 *====================================================================*/

void ANT::run()
{
    int status; // control commands from controller
    powerchannels = 0;

    Status = ANT_RUNNING;
    QString strBuf;
#if defined GC_HAVE_LIBUSB
    usbMode = USBNone;
#endif
    channels = 0;

    for (int i=0; i<ANT_MAX_CHANNELS; i++) antChannel[i]->init();

    state = ST_WAIT_FOR_SYNC;
    length = bytes = 0;
    checksum = ANT_SYNC_BYTE;

    if (openPort() == 0) {

        // Moved early setup code (reset, network key, device pairing) to ANT::setup() so that
        // the receive loop is already running when these early messages are transmitted. This
        // will enable us to check responses to these messages in the future.

    } else {
        quit(0);
        return;
    }

    while(1)
    {
        // read more bytes from the device
        uint8_t byte;
        if (rawRead(&byte, 1) > 0) receiveByte((unsigned char)byte);
        else msleep(5);

        //----------------------------------------------------------------------
        // LISTEN TO CONTROLLER FOR COMMANDS
        //----------------------------------------------------------------------
        pvars.lock();
        status = this->Status;
        pvars.unlock();

        // do we have a channel to search / stop
        if (!channelQueue.isEmpty()) {
            setChannelAtom x = channelQueue.dequeue();
            if (x.device_number == -1) antChannel[x.channel]->close(); // unassign
            else addDevice(x.device_number, x.channel_type, x.channel); // assign
        }

        /* time to shut up shop */
        if (!(status&ANT_RUNNING)) {
            // time to stop!
            quit(0);
            return;
        }
    }
}

int
ANT::start()
{
    QThread::start();
    return 0;
}

int
ANT::setup()
{
    // Give the thread a chance to start.
    // fixme: better synchronisation?
    msleep(500);

    sendMessage(ANTMessage::resetSystem());

    // specs say wait 500ms after reset before sending any more host commands
    msleep(500);

    sendMessage(ANTMessage::setNetworkKey(1, key));

    // pair with specified devices on next available channel
    if (antIDs.count()) {

        foreach(QString antid, antIDs) {

            if (antid.length()) {
                unsigned char c = antid.at(antid.length()-1).toLatin1();
                int ch_type = interpretSuffix(c);
                int device_number = antid.mid(0, antid.length()-1).toInt();

                addDevice(device_number, ch_type, -1);
            }
        }

    } else {

        if (!configuring) {
            // not configured, just pair with whatever you can find
            addDevice(0, ANTChannel::CHANNEL_TYPE_SPEED, 0);
            addDevice(0, ANTChannel::CHANNEL_TYPE_POWER, 1);
            addDevice(0, ANTChannel::CHANNEL_TYPE_CADENCE, 2);
            addDevice(0, ANTChannel::CHANNEL_TYPE_HR, 3);

            if (channels > 4) addDevice(0, ANTChannel::CHANNEL_TYPE_SandC, 4);
        }
    }

    return 0;
}

int
ANT::restart()
{
    int status;

    // get current status
    pvars.lock();
    status = this->Status;
    pvars.unlock();

    // what state are we in anyway?
    if (status&ANT_RUNNING && status&ANT_PAUSED) {
            status &= ~ANT_PAUSED;
            pvars.lock();
            this->Status = status;
            pvars.unlock();
            return 0; // ok its running again!
    }
    return 2;
}

int
ANT::pause()
{
    int status;

    // get current status
    pvars.lock();
    status = this->Status;
    pvars.unlock();

    if (status&ANT_PAUSED) return 2;
    else if (!(status&ANT_RUNNING)) return 4;
    else {
            // ok we're running and not paused so lets pause
            status |= ANT_PAUSED;
            pvars.lock();
            this->Status = status;
            pvars.unlock();

            return 0;
    }
}

int
ANT::stop()
{
    // Close the connections to ANT devices before we stop. Sending the
    // "close channel" ANT message seems to resolve an intermittent
    // issue of unresponsive USB2 stick on subsequent opens.

    if (antIDs.count()) {
        foreach(QString antid, antIDs) {
            if (antid.length()) {
                unsigned char c = antid.at(antid.length()-1).toLatin1();
                int ch_type = interpretSuffix(c);
                int device_number = antid.mid(0, antid.length()-1).toInt();

                removeDevice(device_number, ch_type);
            }
        }
    }

    // what state are we in anyway?
    pvars.lock();
    Status = 0; // Terminate it!
    pvars.unlock();

    return 0;
}

int
ANT::quit(int code)
{
    // event code goes here!
    closePort();

    // Signal to stop logging. Moved to the end of the reading thread to
    // ensure no more messages can arrive and re-open the log file.
    exit(code);
    return 0;
}

void
ANT::getRealtimeData(RealtimeData &rtData)
{
    int mode = rtData.mode;
    long load = rtData.getLoad();
    double slope = rtData.getSlope();

    rtData = telemetry;
    rtData.mode = mode;
    rtData.setLoad(load);
    rtData.setSlope(slope);
}

/*======================================================================
 * Channel management
 *====================================================================*/

// returns 1 for success, 0 for fail.
int
ANT::addDevice(int device_number, int device_type, int channel_number)
{
    // if we're given a channel number, then use that one
    if (channel_number>-1) {
        //antChannel[channel_number]->close();
        antChannel[channel_number]->open(device_number, device_type);
        return 1;
    }

    // if we already have the device, then return.
    // but only if the device number is given since
    // we may choose to scan for multiple devices
    // on separate channels (e.g. 0p on channel 0
    // and 0p on channel 1
    if (device_number != 0) {
        for (int i=0; i<channels; i++) {
            if (((antChannel[i]->channel_type & 0xf ) == device_type) &&
                (antChannel[i]->device_number == device_number)) {
                // send the channel found...
                return 1;
            }
        }
    }

    // look for an unused channel and use on that one
    for (int i=0; i<channels; i++) {
        if (antChannel[i]->channel_type == ANTChannel::CHANNEL_TYPE_UNUSED) {

            //antChannel[i]->close();
            antChannel[i]->open(device_number, device_type);

            // this is an alternate channel for power
            if (device_type == ANTChannel::CHANNEL_TYPE_POWER) {

                // if we are not the first power channel then set to update
                // the alternate power channel
                if (powerchannels) antChannel[i]->setAlt(true);

                // increment the number of power channels
                powerchannels++;
            }
            return 1;
        }
    }

    // there are no unused channels.  fail.
    return 0;
}

// returns 1 for successfully removed, 0 for none found.
int
ANT::removeDevice(int device_number, int channel_type)
{
    int i;

    for (i=0; i<channels; i++) {
        ANTChannel *ac = antChannel[i];

        if ((ac->channel_type == channel_type) && (ac->device_number == device_number)) {

            if ((ac->control_channel!=ac) && ac->control_channel)
                removeDevice(device_number, ac->control_channel->channel_type);

            ac->close();
            ac->channel_type=ANTChannel::CHANNEL_TYPE_UNUSED;
            ac->device_number=0;
            ac->setId();
            return 1;
       }
  }

  // device not found.
  return 0;
}

ANTChannel *
ANT::findDevice(int device_number, int channel_type)
{

    int i;

    for (i=0; i<channels; i++) {
        if (((antChannel[i]->channel_type) == channel_type) &&
            (antChannel[i]->device_number==device_number)) {
            return antChannel[i];
        }
    }

    // device not found.
    return NULL;
}

int
ANT::startWaitingSearch()
{
    int i;

    // are any fast searches in progress?  if so, then bail
    for (i=0; i<channels; i++) {
        if (antChannel[i]->channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
            return 0;
        }
    }

    // start the first slow search
    for (i=0; i<channels; i++) {
        if (antChannel[i]->channel_type_flags & CHANNEL_TYPE_WAITING) {
            antChannel[i]->channel_type_flags &= ~CHANNEL_TYPE_WAITING;
            sendMessage(ANTMessage::unassignChannel(i));
            return 1;
        }
    }

    return 0;
}

void
ANT::associateControlChannels() {

    // first, unassociate all control channels
    for (int i=0; i<channels; i++) antChannel[i]->control_channel=NULL;

    // then, associate cinqos:
    //    new cinqos get their own selves for control
    //    old cinqos, look for an open control channel
    //       if found and open, associate
    //       elif found and not open yet, nop
    //       elif not found, open one
    for (int i=0; i<channels; i++) {
        ANTChannel *ac=antChannel[i];

        switch (ac->channel_type) {
            case ANTChannel::CHANNEL_TYPE_POWER:
                if (ac->is_cinqo) {
                    if (ac->is_old_cinqo) {
                        ANTChannel *my_ant_channel;

                        my_ant_channel=findDevice(ac->device_number, ANTChannel::CHANNEL_TYPE_QUARQ);
                        if (!my_ant_channel) my_ant_channel=findDevice(ac->device_number, ANTChannel::CHANNEL_TYPE_FAST_QUARQ);
                        if (!my_ant_channel) my_ant_channel=findDevice(ac->device_number, ANTChannel::CHANNEL_TYPE_FAST_QUARQ_NEW);

                        if (my_ant_channel) {
                            if (my_ant_channel->isSearching()) {
                                // ignore if searching
                            } else {
                                ac->control_channel=my_ant_channel;
                                ac->sendCinqoSuccess();
                            }
                        } else { // no ant channel, let's start one
                            addDevice(ac->device_number, ANTChannel::CHANNEL_TYPE_QUARQ, -1);
                        }
                    } else { // new cinqo
                        ac->control_channel=ac;
                        ac->sendCinqoSuccess();
                    }
                } // is_cinqo
                break;

              case ANTChannel::CHANNEL_TYPE_FAST_QUARQ:
              case ANTChannel::CHANNEL_TYPE_FAST_QUARQ_NEW:
              case ANTChannel::CHANNEL_TYPE_QUARQ:
                  ac->is_cinqo=1;
                  ac->control_channel=ac;
                  break;
              default:
                      ;
        } // channel_type case
    } // for-loop
}

// For serial device discovery
bool
ANT::discover(QString name)
{
#ifdef WIN32
Q_UNUSED(name);
#endif

#ifdef Q_OS_LINUX

    // All we can do for USB1 sticks is see if the cp210x driver module
    // is loaded for this device, and if it is, we will use the device
    // they are getting rarer and rarer these days (no longer sold by
    // Garmin anyway) so no need to expend to much energy extending this
    // especially since the Linux user community is relatively small.
    struct stat s;
    if (stat(name.toLatin1(), &s) == -1) return false;
    int maj = major(s.st_rdev);
    int min = minor(s.st_rdev);
    QString sysFile = QString("/sys/dev/char/%1:%2/device/driver/module/drivers/usb:cp210x").arg(maj).arg(min);
    if (QFileInfo(sysFile).exists()) return true;
#endif

#ifdef Q_OS_MAC
    // On MAC we only support the SILabs Virtual Com Port Drivers
    // which will leave a device file /dev/cu.ANTUSBStick.slabvcp
    if (name == "/dev/cu.ANTUSBStick.slabvcp") return true;
#endif
    return false;
}

void
ANT::channelInfo(int channel, int device_number, int device_id)
{
    emit foundDevice(channel, device_number, device_id);
    //qDebug()<<"found device number"<<device_number<<"type"<<device_id<<"on channel"<<channel
    //<< "is a "<<deviceTypeDescription(device_id) << "with code"<<deviceTypeCode(device_id);
}

void
ANT::dropInfo(int channel, int drops, int received)    // we dropped a message
{
    double reliability = 100.00f - (100.00f * double(drops) / double(received));
    //qDebug()<<"Channel"<<channel<<"reliability is"<< (int)(reliability)<<"%";
    emit signalStrength(channel, reliability);
    return;
}

void
ANT::lostInfo(int number)    // we lost the connection
{
    if (number < 0 || number >= channels) return; // ignore out of bound

    emit lostDevice(number);
    //qDebug()<<"lost info for channel"<<number;
}

void
ANT::staleInfo(int number)   // info is now stale - set to zero
{
    if (number < 0 || number >= channels) return; // ignore out of bound

    //qDebug()<<"stale info for channel"<<number;
}

void
ANT::slotSearchTimeout(int number) // search timed out
{
    if (number < 0 || number >= channels) return; // ignore out of bound

    emit searchTimeout(number);
    //qDebug()<<"search timeout on channel"<<number;
}

void
ANT::slotSearchComplete(int number) // search completed successfully
{
    if (number < 0 || number >= channels) return; // ignore out of bound

    emit searchComplete(number);
    //qDebug()<<"search completed on channel"<<number;
}

/*----------------------------------------------------------------------
 * Message I/O
 *--------------------------------------------------------------------*/
void
ANT::sendMessage(ANTMessage m) {
    static const unsigned char padding[5] = { '\0', '\0', '\0', '\0', '\0' };

//fprintf(stderr, ">> send: ");
//for(int i=0; i<m.length+3; i++) fprintf(stderr, "%02x ", m.data[i]);
//fprintf(stderr, "\n");

    rawWrite((uint8_t*)m.data, m.length);

    // this padding is important - do not remove it
    // we need to be sure the message is at least 12 bytes
    rawWrite((uint8_t*)padding, 5);
}

void
ANT::receiveByte(unsigned char byte) {

    switch (state) {
        case ST_WAIT_FOR_SYNC:
            if (byte == ANT_SYNC_BYTE) {
                state = ST_GET_LENGTH;
                checksum = ANT_SYNC_BYTE;
                rxMessage[0] = byte;
            }
            break;

        case ST_GET_LENGTH:
            if ((byte == 0) || (byte > ANT_MAX_LENGTH)) {
                state = ST_WAIT_FOR_SYNC;
            }
            else {
              rxMessage[ANT_OFFSET_LENGTH] = byte;
                checksum ^= byte;
                length = byte;
                bytes = 0;
                state = ST_GET_MESSAGE_ID;
            }
            break;

        case ST_GET_MESSAGE_ID:
            rxMessage[ANT_OFFSET_ID] = byte;
            checksum ^= byte;
            state = ST_GET_DATA;
            break;

        case ST_GET_DATA:
            rxMessage[ANT_OFFSET_DATA + bytes] = byte;
            checksum ^= byte;
            if (++bytes >= length){
                state = ST_VALIDATE_PACKET;
            }
            break;

        case ST_VALIDATE_PACKET:
            if (checksum == byte){
                processMessage();
            }
            state = ST_WAIT_FOR_SYNC;
            break;
    }
}


//
// Pass inbound message to channel for handling
//
void
ANT::handleChannelEvent(void) {
    int channel = rxMessage[ANT_OFFSET_DATA] & 0x7;
    if(channel >= 0 && channel < channels) {

        // handle a channel event here!
        antChannel[channel]->receiveMessage(rxMessage);
    }
}

void
ANT::processMessage(void) {

    ANTMessage m(this, rxMessage); // for debug!

//fprintf(stderr, "<< receive: ");
//for(int i=0; i<m.length+3; i++) fprintf(stderr, "%02x ", m.data[i]);
//fprintf(stderr, "\n");

    struct timeval timestamp;
    gettimeofday (&timestamp, NULL);
    emit receivedAntMessage(m, timestamp);

    switch (rxMessage[ANT_OFFSET_ID]) {
        case ANT_ACK_DATA:
        case ANT_BROADCAST_DATA:
        case ANT_CHANNEL_STATUS:
        case ANT_CHANNEL_ID:
        case ANT_BURST_DATA:
            handleChannelEvent();
            break;

        case ANT_CHANNEL_EVENT:
          switch (rxMessage[ANT_OFFSET_MESSAGE_CODE]) {
          case EVENT_TRANSFER_TX_FAILED:
            break;
          case EVENT_TRANSFER_TX_COMPLETED:
            // fall through
          default:
            handleChannelEvent();
          }
          break;

        case ANT_VERSION:
            break;

        case ANT_CAPABILITIES:
            break;

        case ANT_SERIAL_NUMBER:
            break;

        default:
            break;
    }
}

/*======================================================================
 * Serial I/O
 *====================================================================*/

int ANT::closePort()
{
#ifdef WIN32
    switch (usbMode) {
    case USB2 :
        usb2->close();
        return 0;
        break;
    case USB1 :
        return (int)!CloseHandle(devicePort);
        break;
    default :
        return -1;
        break;
    }
#else

#ifdef GC_HAVE_LIBUSB
    if (usbMode == USB2) {
        usb2->close();
        return 0;
    }
#endif
    tcflush(devicePort, TCIOFLUSH); // clear out the garbage
    return close(devicePort);
#endif
}

bool ANT::find()
{
#if defined WIN32 || defined GC_HAVE_LIBUSB
    if (usb2->find() == true) return true;
#endif
#ifdef WIN32
    if (USBXpress::find() == true) return true;
#endif
    return false;
}

int ANT::openPort()
{
#ifdef WIN32
    int rc;

    // on windows we try on USB2 then on USB1 then fail...
    if ((rc=usb2->open()) != -1) {
        usbMode = USB2;
        channels = 8;
        return rc;
    } else if ((rc= USBXpress::open(&devicePort)) != -1) {
        usbMode = USB1;
        channels = 4;
        return rc;
    } else {
        usbMode = USBNone;
        channels = 0;
        return -1;
    }

#else
    // LINUX AND MAC USES TERMIO / IOCTL / STDIO

#if defined(Q_OS_MACX)
    int ldisc=TTYDISC;
#else
    int ldisc=N_TTY; // LINUX
#endif

#ifdef GC_HAVE_LIBUSB
    int rc;
    if ((rc=usb2->open()) != -1) {
        usbMode = USB2;
        channels = 8;
        return rc;
    }
    usbMode = USB1;
#endif

    // if usb2 failed / not compiled in, we must be using
    // a USB1 stick so default to 4 channels
    channels = 4;

    if ((devicePort=open(deviceFilename.toLatin1(),O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1)
        return errno;

    tcflush(devicePort, TCIOFLUSH); // clear out the garbage

    if (ioctl(devicePort, TIOCSETD, &ldisc) == -1) return errno;

    // get current settings for the port
    tcgetattr(devicePort, &deviceSettings);

    // set raw mode i.e. ignbrk, brkint, parmrk, istrip, inlcr, igncr, icrnl, ixon
    //                   noopost, cs8, noecho, noechonl, noicanon, noisig, noiexn
    cfmakeraw(&deviceSettings);
    cfsetspeed(&deviceSettings, B115200);

    // further attributes
    deviceSettings.c_iflag= IGNPAR;
    deviceSettings.c_oflag=0;
    deviceSettings.c_cflag &= (~CSIZE & ~CSTOPB);
#if defined(Q_OS_MACX)
    deviceSettings.c_cflag |= (CS8 | CREAD | HUPCL | CCTS_OFLOW | CRTS_IFLOW);
#else
    deviceSettings.c_cflag |= (CS8 | CREAD | HUPCL | CRTSCTS);
#endif
    deviceSettings.c_lflag=0;
    deviceSettings.c_cc[VMIN]=0;
    deviceSettings.c_cc[VTIME]=0;

    // set those attributes
    if(tcsetattr(devicePort, TCSANOW, &deviceSettings) == -1) return errno;
    tcgetattr(devicePort, &deviceSettings);

#endif

    // success
    return 0;
}

int ANT::rawWrite(uint8_t *bytes, int size) // unix!!
{
    int rc=0;

#ifdef WIN32
    switch (usbMode) {
    case USB1:
        rc = USBXpress::write(&devicePort, bytes, size);
        break;
    case USB2:
        rc = usb2->write((char *)bytes, size);
        break;
    default:
        rc = 0;
        break;
    }

    if (!rc) rc = -1; // return -1 if nothing written
    return rc;

#else

#ifdef GC_HAVE_LIBUSB
    if (usbMode == USB2) {
        return usb2->write((char *)bytes, size);
    }
#endif

    int ibytes;

    ioctl(devicePort, FIONREAD, &ibytes);

    // timeouts are less critical for writing, since vols are low
    rc= write(devicePort, bytes, size);

    if (rc != -1) tcdrain(devicePort); // wait till its gone.

    ioctl(devicePort, FIONREAD, &ibytes);
    return rc;
#endif


}

int ANT::rawRead(uint8_t bytes[], int size)
{
#ifdef WIN32
    switch (usbMode) {
    case USB1:
        return USBXpress::read(&devicePort, bytes, size);
        break;
    case USB2:
        return usb2->read((char *)bytes, size);
        break;
    default:
        break;
    }

#else

#ifdef GC_HAVE_LIBUSB
    if (usbMode == USB2) {
        return usb2->read((char *)bytes, size);
    }
#endif
    int i=0;
    uint8_t byte;

    // read one byte at a time sleeping when no data ready
    // until we timeout waiting then return error
    for (i=0; i<size; i++) {
        int rc = read(devicePort, &byte, 1);
        if (rc == -1 || rc == 0) return -1; // error!
        else bytes[i] = byte;
    }
    return i;

#endif
    return -1; // keep compiler happy.
}

// convert 'p' 'c' etc into ANT values for device type
int ANT::interpretSuffix(char c)
{
    const ant_sensor_type_t *st=ant_sensor_types;

    do {
        if (st->suffix==c) return st->type;
    } while (++st, st->type != ANTChannel::CHANNEL_TYPE_GUARD);

    return -1;
}

// convert ANT value to 'p' 'c' values
char ANT::deviceIdCode(int type)
{
    const ant_sensor_type_t *st=ant_sensor_types;

    do {
        if (st->type==type) return st->suffix;
    } while (++st, st->type != ANTChannel::CHANNEL_TYPE_GUARD);
    return '-';
}

// convert ANT value to 'p' 'c' values
char ANT::deviceTypeCode(int type)
{
    const ant_sensor_type_t *st=ant_sensor_types;

    do {
        if (st->device_id==type) return st->suffix;
    } while (++st, st->type != ANTChannel::CHANNEL_TYPE_GUARD);
    return '-';
}

// convert ANT value to human string
const char * ANT::deviceTypeDescription(int type)
{
    const ant_sensor_type_t *st=ant_sensor_types;

    do {
        if (st->device_id==type) return st->descriptive_name;
    } while (++st, st->type != ANTChannel::CHANNEL_TYPE_GUARD);
    return "Unknown device type";
}
