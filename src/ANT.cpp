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


/* Control status */
#define ANT_RUNNING  0x01
#define ANT_PAUSED   0x02

// network key
const unsigned char ANT::key[8] = { 0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45 };

// supported sensor types
const ant_sensor_type_t ANT::ant_sensor_types[] = {
  { ANTChannel::CHANNEL_TYPE_UNUSED, 0, 0, 0, 0, "Unused", '?' },
  { ANTChannel::CHANNEL_TYPE_HR, ANT_SPORT_HR_PERIOD, ANT_SPORT_HR_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Heartrate", 'h' },
  { ANTChannel::CHANNEL_TYPE_POWER, ANT_SPORT_POWER_PERIOD, ANT_SPORT_POWER_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Power", 'p' },
  { ANTChannel::CHANNEL_TYPE_SPEED, ANT_SPORT_SPEED_PERIOD, ANT_SPORT_SPEED_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Speed", 's' },
  { ANTChannel::CHANNEL_TYPE_CADENCE, ANT_SPORT_CADENCE_PERIOD, ANT_SPORT_CADENCE_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Cadence", 'c' },
  { ANTChannel::CHANNEL_TYPE_SandC, ANT_SPORT_SandC_PERIOD, ANT_SPORT_SandC_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Speed + Cadence", 'd' },
  { ANTChannel::CHANNEL_TYPE_QUARQ, ANT_QUARQ_PERIOD, ANT_QUARQ_TYPE,
                ANT_QUARQ_FREQUENCY, DEFAULT_NETWORK_NUMBER, "Quarq Channel", 'Q' },
  { ANTChannel::CHANNEL_TYPE_FAST_QUARQ, ANT_FAST_QUARQ_PERIOD, ANT_FAST_QUARQ_TYPE,
                ANT_FAST_QUARQ_FREQUENCY, DEFAULT_NETWORK_NUMBER, "Fast Quarq", 'q' },
  { ANTChannel::CHANNEL_TYPE_FAST_QUARQ_NEW, ANT_FAST_QUARQ_PERIOD, ANT_FAST_QUARQ_TYPE_WAS,
                ANT_FAST_QUARQ_FREQUENCY, DEFAULT_NETWORK_NUMBER, "Fast Quarq New", 'n' },
  { ANTChannel::CHANNEL_TYPE_GUARD, 0, 0, 0, 0, "", '\0' }
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
ANT::ANT(QObject *parent, DeviceConfiguration *devConf) : QThread(parent)
{
    // device status and settings
    Status=0;
    deviceFilename = devConf->portSpec;
    baud=115200;

    // state machine
    state = ST_WAIT_FOR_SYNC;
    length = bytes = 0;
    checksum = ANT_SYNC_BYTE;

    // ant ids - may not be configured of course
    if (devConf->deviceProfile.length())
        antIDs = devConf->deviceProfile.split(",");
    else
        antIDs.clear();

    // setup the channels
    for (int i=0; i<ANT_MAX_CHANNELS; i++) antChannel[i] = new ANTChannel(i, this);
}

ANT::~ANT()
{
}

void ANT::setDevice(QString x)
{
    deviceFilename = x;
}

void ANT::setBaud(int x)
{
    baud = x;
}


/*======================================================================
 * Main thread functions; start, stop etc
 *====================================================================*/

void ANT::run()
{
    //qDebug() << "Starting ANT thread...";
    int status; // control commands from controller
    bool isPortOpen = false;

    Status = ANT_RUNNING;
    QString strBuf;

    if (openPort() == 0) {

        isPortOpen = true;
        sendMessage(ANTMessage::resetSystem());
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

            // not configured, just pair with whatever you can find
            addDevice(0, ANTChannel::CHANNEL_TYPE_POWER, 0);
            addDevice(0, ANTChannel::CHANNEL_TYPE_SandC, 1);
            addDevice(0, ANTChannel::CHANNEL_TYPE_CADENCE, 2);
            addDevice(0, ANTChannel::CHANNEL_TYPE_HR, 3);
        }

    } else {
        quit(0);
        return;
    }

    while(1)
    {
        // read more bytes from the device
        uint8_t byte;
        if (rawRead(&byte, 1) > 0) receiveByte((unsigned char)byte);
        //else msleep(5);

        //----------------------------------------------------------------------
        // LISTEN TO CONTROLLER FOR COMMANDS
        //----------------------------------------------------------------------
        pvars.lock();
        status = this->Status;
        pvars.unlock();

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
    int status;

    // get current status
    pvars.lock();
    status = this->Status;
    pvars.unlock();

    // what state are we in anyway?
    pvars.lock();
    Status = 0; // Terminate it!
    pvars.unlock();
    return 0;
}

int
ANT::quit(int code)
{
//qDebug()<<"Stopping ANT thread...";
    // event code goes here!
    closePort();
    exit(code);
    return 0;
}

RealtimeData
ANT::getRealtimeData()
{
    return telemetry;
}

/*======================================================================
 * Channel management
 *====================================================================*/

// returns 1 for success, 0 for fail.
int
ANT::addDevice(int device_number, int device_type, int channel_number)
{
    // if we're given a device number, then use that one
    if (channel_number>-1) {
        antChannel[channel_number]->close();
        antChannel[channel_number]->open(device_number, device_type);
        return 1;
    }

    // if we already have the device, then return.
    for (int i=0; i<ANT_MAX_CHANNELS; i++) {
        if (((antChannel[i]->channel_type & 0xf ) == device_type) &&
            (antChannel[i]->device_number == device_number)) {
            // send the channel found...
            antChannel[i]->channelInfo();
            return 1;
        }
    }

    // look for an unused channel and use on that one
    for (int i=0; i<ANT_MAX_CHANNELS; i++) {
        if (antChannel[i]->channel_type == ANTChannel::CHANNEL_TYPE_UNUSED) {
            antChannel[i]->open(device_number, device_type);
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

    for (i=0; i<ANT_MAX_CHANNELS; i++) {
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

    for (i=0; i<ANT_MAX_CHANNELS; i++) {
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
    for (i=0; i<ANT_MAX_CHANNELS; i++) {
        if (antChannel[i]->channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
            return 0;
        }
    }

    // start the first slow search
    for (i=0; i<ANT_MAX_CHANNELS; i++) {
        if (antChannel[i]->channel_type_flags & CHANNEL_TYPE_WAITING) {
            antChannel[i]->channel_type_flags &= ~CHANNEL_TYPE_WAITING;
            sendMessage(ANTMessage::unassignChannel(i));
            return 1;
        }
    }

    return 0;
}

void
ANT::report()
{
    for (int i=0; i<ANT_MAX_CHANNELS; i++)
        antChannel[i]->channelInfo();
}

void
ANT::associateControlChannels() {

    // first, unassociate all control channels
    for (int i=0; i<ANT_MAX_CHANNELS; i++) antChannel[i]->control_channel=NULL;

    // then, associate cinqos:
    //    new cinqos get their own selves for control
    //    old cinqos, look for an open control channel
    //       if found and open, associate
    //       elif found and not open yet, nop
    //       elif not found, open one
    for (int i=0; i<ANT_MAX_CHANNELS; i++) {
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

// XXX device discovery for pairing to do... need to
// think about a cool way to do this.
bool
ANT::discover(DeviceConfiguration *, QProgressDialog *)
{
    return false;
}

/*----------------------------------------------------------------------
 * Message I/O
 *--------------------------------------------------------------------*/
void
ANT::sendMessage(ANTMessage m) {
    static const unsigned char padding[5] = { '\0', '\0', '\0', '\0', '\0' };

    rawWrite((uint8_t*)m.data, m.length);

    // this padding is important, for some reason XXX find out why?
    rawWrite((uint8_t*)padding, 5);
}

void
ANT::receiveByte(unsigned char byte) {

    switch (state) {
        case ST_WAIT_FOR_SYNC:
            if (byte == ANT_SYNC_BYTE) {
                state = ST_GET_LENGTH;
                checksum = ANT_SYNC_BYTE;
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
    if(channel >= 0 && channel < 4) {

    // handle a channel event here!
//qDebug()<<"channel event on channel:"<<channel;

        antChannel[channel]->receiveMessage(rxMessage);
    }
}

void
ANT::processMessage(void) {

//qDebug()<<"processing ant message"<<rxMessage[ANT_OFFSET_ID];
    ANTMessage(this, rxMessage); // for debug!

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
            //XXX remember last message ... ANT_SendAckMessage();
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
    return (int)!CloseHandle(devicePort);
#else
    tcflush(devicePort, TCIOFLUSH); // clear out the garbage
    return close(devicePort);
#endif
}

int ANT::openPort()
{
#ifndef WIN32

    // LINUX AND MAC USES TERMIO / IOCTL / STDIO

#if defined(Q_OS_MACX)
    int ldisc=TTYDISC;
#else
    int ldisc=N_TTY; // LINUX
#endif

    if ((devicePort=open(deviceFilename.toAscii(),O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1)
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

#else
#ifdef GC_HAVE_USBXPRESS
    return USBXpress::open(&devicePort);
#else
    // WINDOWS USES SET/GETCOMMSTATE AND READ/WRITEFILE

    COMMTIMEOUTS timeouts; // timeout settings on serial ports

    // if deviceFilename references a port above COM9
    // then we need to open "\\.\COMX" not "COMX"
    QString portSpec;
    int portnum = deviceFilename.midRef(3).toString().toInt();
    if (portnum < 10)
       portSpec = deviceFilename;
    else
       portSpec = "\\\\.\\" + deviceFilename;
    wchar_t deviceFilenameW[32]; // \\.\COM32 needs 9 characters, 32 should be enough?
    MultiByteToWideChar(CP_ACP, 0, portSpec.toAscii(), -1, (LPWSTR)deviceFilenameW,
                    sizeof(deviceFilenameW));

    // win32 commport API
    devicePort = CreateFile (deviceFilenameW, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (devicePort == INVALID_HANDLE_VALUE) return -1;

    if (GetCommState (devicePort, &deviceSettings) == false) return -1;

    // so we've opened the comm port lets set it up for
    deviceSettings.BaudRate = CBR_2400;
    deviceSettings.fParity = NOPARITY;
    deviceSettings.ByteSize = 8;
    deviceSettings.StopBits = ONESTOPBIT;
    deviceSettings.EofChar = 0x0;
    deviceSettings.ErrorChar = 0x0;
    deviceSettings.EvtChar = 0x0;
    deviceSettings.fBinary = true;
    deviceSettings.fRtsControl = RTS_CONTROL_HANDSHAKE;
    deviceSettings.fOutxCtsFlow = TRUE;


    if (SetCommState(devicePort, &deviceSettings) == false) {
        CloseHandle(devicePort);
        return -1;
    }

    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 1000;
    timeouts.ReadTotalTimeoutMultiplier = 50;
    timeouts.WriteTotalTimeoutConstant = 2000;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(devicePort, &timeouts);
#endif
#endif

    // success
    return 0;
}

int ANT::rawWrite(uint8_t *bytes, int size) // unix!!
{
    int rc=0;

#ifdef WIN32
#ifdef GC_HAVE_USBXPRESS
    rc = USBXpress::write(&devicePort, bytes, size);
#else
    DWORD cBytes;
    rc = WriteFile(devicePort, bytes, size, &cBytes, NULL);
#endif
    if (!rc) return -1;
    return rc;

#else
    int ibytes;

    ioctl(devicePort, FIONREAD, &ibytes);

    // timeouts are less critical for writing, since vols are low
    rc= write(devicePort, bytes, size);

    if (rc != -1) tcdrain(devicePort); // wait till its gone.

    ioctl(devicePort, FIONREAD, &ibytes);
#endif

    return rc;

}

int ANT::rawRead(uint8_t bytes[], int size)
{
    int rc=0;

#ifdef WIN32
#ifdef GC_HAVE_USBXPRESS
    return USBXpress::read(&devicePort, bytes, size);
#else

    // Readfile deals with timeouts and readyread issues
    DWORD cBytes;
    rc = ReadFile(devicePort, bytes, size, &cBytes, NULL);
    if (rc) return (int)cBytes;
    else return (-1);
#endif
#else

    int timeout=0, i=0;
    uint8_t byte;

    // read one byte at a time sleeping when no data ready
    // until we timeout waiting then return error
    for (i=0; i<size; i++) {
        timeout =0;
            rc = read(devicePort, &byte, 1);
            if (rc == -1 || rc == 0) return -1; // error!
            else bytes[i] = byte;
    }
    return i;

#endif
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
