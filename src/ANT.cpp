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
#include "TrainSidebar.h" // for RT_MODE_{ERGO,SPIN,CALIBRATE}
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

// !!! THE INITIALISATION OF ant_sensor_types BELOW MUST MATCH THE
// !!! ENUM FOR CHANNELTYPE IN ANTChannel.h (SORRY, ITS HORRIBLE)

// supported sensor types
const ant_sensor_type_t ANT::ant_sensor_types[] = {
  { true, ANTChannel::CHANNEL_TYPE_UNUSED, 0, 0, 0, 0, "Unused", '?', "" },
  { true, ANTChannel::CHANNEL_TYPE_HR, ANT_SPORT_HR_PERIOD, ANT_SPORT_HR_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Heartrate", 'h', ":images/IconHR.png" },
  { true, ANTChannel::CHANNEL_TYPE_POWER, ANT_SPORT_POWER_PERIOD, ANT_SPORT_POWER_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Power", 'p', ":images/IconPower.png" },
  { true, ANTChannel::CHANNEL_TYPE_SPEED, ANT_SPORT_SPEED_PERIOD, ANT_SPORT_SPEED_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Speed", 's', ":images/IconSpeed.png" },
  { true, ANTChannel::CHANNEL_TYPE_CADENCE, ANT_SPORT_CADENCE_PERIOD, ANT_SPORT_CADENCE_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Cadence", 'c', ":images/IconCadence.png" },
  { true, ANTChannel::CHANNEL_TYPE_SandC, ANT_SPORT_SandC_PERIOD, ANT_SPORT_SandC_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Speed + Cadence", 'd', ":images/IconCadence.png" },
  { true, ANTChannel::CHANNEL_TYPE_MOXY, ANT_SPORT_MOXY_PERIOD, ANT_SPORT_MOXY_TYPE,
                ANT_MOXY_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Moxy", 'm', ":images/IconMoxy.png" },
  { true, ANTChannel::CHANNEL_TYPE_CONTROL, ANT_SPORT_CONTROL_PERIOD, ANT_SPORT_CONTROL_TYPE,
                ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Remote Control", 'r', ":images/IconCadence.png" },
  { true, ANTChannel::CHANNEL_TYPE_TACX_VORTEX, ANT_SPORT_TACX_VORTEX_PERIOD, ANT_SPORT_TACX_VORTEX_TYPE,
                ANT_TACX_VORTEX_FREQUENCY, DEFAULT_NETWORK_NUMBER, "Tacx Vortex Smart", 'v', ":images/IconPower.png" },
  { true, ANTChannel::CHANNEL_TYPE_FITNESS_EQUIPMENT, ANT_SPORT_FITNESS_EQUIPMENT_PERIOD, ANT_SPORT_FITNESS_EQUIPMENT_TYPE,
                ANT_FITNESS_EQUIPMENT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Fitness Equipment Control (FE-C)", 'f', ":images/IconPower.png" },
  { false, ANTChannel::CHANNEL_TYPE_GUARD, 0, 0, 0, 0, "", '\0', "" }
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
ANT::ANT(QObject *parent, DeviceConfiguration *devConf, QString cyclist) : QThread(parent), devConf(devConf)
{
    qRegisterMetaType<ANTMessage>("ANTMessage");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<struct timeval>("struct timeval");

    //remember the cylist for wheelsize Settings
    trainCyclist = cyclist;

    // device status and settings
    Status=0;
    deviceFilename = devConf ? devConf->portSpec : "";
    baud=115200;
    powerchannels=0;
    configuring = false;

    // kickr
    kickrDeviceID = 0;
    kickrChannel = -1;

    // vortex
    vortexID = vortexChannel = -1;

    fecChannel = -1;

    // current and desired modes/load/gradients
    // set so first time through current != desired
    currentMode = 0;
    mode = RT_MODE_ERGO;
    currentLoad = 0;
    load = 100; // always set to something
    currentGradient = 0;
    currentRollingResistance = rollingResistance = 0.004; // typical for road
    gradient = 0.1;

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

bool ANT::modeERGO(void) const
{
    return mode==RT_MODE_ERGO; 
}

bool ANT::modeSLOPE(void) const
{
    return mode==RT_MODE_SLOPE;
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
    else telemetry.setSpeed(x * appsettings->cvalue(trainCyclist, GC_WHEELSIZE, 2100).toInt() / 1000 * 60 / 1000);
}

void ANT::setHb(double smo2, double thb)
{
    telemetry.setHb(smo2, thb);
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
        int rc = rawRead(&byte, 1);
        if (rc > 0)
          receiveByte(byte);
        else {
#if defined GC_HAVE_LIBUSB
          if (rc == LIBUSB_ERROR_NO_DEVICE) {
            // Can happen when the USB stick is pulled.
            stop();
          } else if (rc == LIBUSB_ERROR_PIPE) {
            // Endpoint halted.
            restart();
          } else {
            // Typically a timeout (error -110).
            msleep(5);
          }
#else
          msleep(5);
#endif
        }
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

void
ANT::setLoad(double load)
{
    if (this->load == load) return;

    // load has changed
    this->load = load;

    // if we have a vortex trainer connected, relay the change in target power to the brake
    if (vortexChannel != -1)
    {
        sendMessage(ANTMessage::tacxVortexSetPower(vortexChannel, vortexID, (int)load));
    }

    // if we have a FE-C trainer connected, relay the change in target power to the brake
    if ((fecChannel != -1) && (antChannel[fecChannel]->capabilities() & FITNESS_EQUIPMENT_POWER_MODE_CAPABILITY))
    {
        sendMessage(ANTMessage::fecSetTargetPower(fecChannel, (int)load));
    }
}

void ANT::refreshFecLoad()
{
    if (fecChannel == -1)
        return;

    if (antChannel[fecChannel]->capabilities() & FITNESS_EQUIPMENT_POWER_MODE_CAPABILITY)
        sendMessage(ANTMessage::fecSetTargetPower(fecChannel, (int)load));
}

void ANT::refreshFecGradient()
{
    if (fecChannel == -1)
        return;

    if (antChannel[fecChannel]->capabilities() & FITNESS_EQUIPMENT_SIMUL_MODE_CAPABILITY)
        sendMessage(ANTMessage::fecSetTrackResistance(fecChannel, gradient, currentRollingResistance));
}

void ANT::requestFecCapabilities()
{
    sendMessage(ANTMessage::fecRequestCapabilities(fecChannel));
}

void ANT::refreshVortexLoad()
{
    if (vortexChannel == -1)
        return;

    sendMessage(ANTMessage::tacxVortexSetPower(vortexChannel, vortexID, (int)load));
}

void
ANT::setGradient(double gradient)
{
//    if (fecChannel != -1)
//        qDebug() << "We have fec trainer connected, simulation capabilities=" << antChannel[fecChannel]->capabilities();

    if (this->gradient == gradient) return;

    // gradient changed
    this->gradient = gradient;

    // if we have a FE-C trainer connected, relay the change in simulated slope of trainer electronic
    if ((fecChannel != -1) && (antChannel[fecChannel]->capabilities() & FITNESS_EQUIPMENT_SIMUL_MODE_CAPABILITY))
    {
        //set fitness equipment target gradient
        sendMessage(ANTMessage::fecSetTrackResistance(fecChannel, gradient, currentRollingResistance));
        currentGradient = gradient;
        // TODO : obtain acknowledge / confirm value using fecRequestCommandStatus
        // TODO : if trainer does not have simulation capabilities, use power mode & let GC calculate
        //        the desired load based on gradient, wind, rolling resistance...
    }
}

void
ANT::setMode(int mode)
{
    if (this->mode == mode) return;

    // mode changed
    this->mode = mode;
}

void
ANT::kickrCommand()
{
    // we don't have a kickr !
    if (kickrChannel < 0) return;

    // mode changed ?
    if (currentMode != mode) {

        switch(mode) {
        case RT_MODE_ERGO : // do nothing, just start sending ergo commands below
            {
            }
           break;

        case RT_MODE_SPIN : // need to setup for "sim" mode, so sending lots of
                            // config to overcome the default values
            {
            }
            break;

        case RT_MODE_CALIBRATE : // ??? maybe ???
            //qDebug()<<"A: setup calib mode";
            break;
        }

        currentMode = mode;
        currentLoad = load;
        currentGradient = gradient;
    }

    // load has changed ?
    if (mode == RT_MODE_ERGO && load != currentLoad) {
        currentLoad = load;
    }

    // slope has changed in slope mode
    if (mode == RT_MODE_SPIN && gradient != currentGradient) {
        currentGradient = gradient;
    }

    // create message to send
    ANTMessage toSend;

    switch (mode) {

    default:
    case RT_MODE_ERGO:
        toSend = ANTMessage::kickrErgMode(kickrChannel, kickrDeviceID, load, false);
        break;

    case RT_MODE_SPIN: // gradient is between -1.0 and +1.0 so needs conversion from gradient
        toSend = ANTMessage::kickrGrade(kickrChannel, kickrDeviceID, gradient/100.00f);
        break;
    }

    sendMessage(toSend);
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

            if (channels > 4) {
                addDevice(0, ANTChannel::CHANNEL_TYPE_SandC, 4);
                addDevice(0, ANTChannel::CHANNEL_TYPE_MOXY, 5);
                addDevice(0, ANTChannel::CHANNEL_TYPE_FITNESS_EQUIPMENT, 6);
            }
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

    // and close any channels that are open
    for (int i=0; i<channels; i++)
        if (antChannel[i]->status != ANTChannel::Closed)
            antChannel[i]->close();

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
    rtData = telemetry;
    rtData.mode = mode;
    rtData.setLoad(load);
    rtData.setSlope(gradient);
}

/*======================================================================
 * Channel management
 *====================================================================*/

// returns -1 for fail otherwise channel number used
int
ANT::addDevice(int device_number, int device_type, int channel_number)
{
    // if we're given a channel number, then use that one
    if (channel_number>-1) {
        //antChannel[channel_number]->close();
        antChannel[channel_number]->open(device_number, device_type);
        return channel_number;
    }

    // if we already have the device, then return.
    // but only if the device number is given since
    // we may choose to scan for multiple devices
    // on separate channels (e.g. 0p on channel 0
    // and 0p on channel 1
    if (device_number != 0) {
        for (int i=0; i<channels; i++) {
            if ((antChannel[i]->channel_type == device_type) &&
                (antChannel[i]->device_number == device_number)) {
                // send the channel found...
                return i;
            }
        }
    }

    // look for an unused channel and use on that one
    for (int i=0; i<channels; i++) {
        if (antChannel[i]->channel_type == ANTChannel::CHANNEL_TYPE_UNUSED) {

            //antChannel[i]->close();
            antChannel[i]->open(device_number, device_type);

            // this is an alternate channel for power
            if ((device_type == ANTChannel::CHANNEL_TYPE_POWER) ||
                (device_type == ANTChannel::CHANNEL_TYPE_FITNESS_EQUIPMENT)) {

                // if we are not the first power channel then set to update
                // the alternate power channel
                if (powerchannels)
                    antChannel[i]->setAlt(true);

                // increment the number of power channels
                powerchannels++;
            }
            return i;
        }
    }

    // there are no unused channels.  fail.
    return -1;
}

// returns 1 for successfully removed, 0 for none found.
int
ANT::removeDevice(int device_number, int channel_type)
{
    int i;

    for (i=0; i<channels; i++) {
        ANTChannel *ac = antChannel[i];

        if ((ac->channel_type == channel_type) && (ac->device_number == device_number)) {
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
    QString sysFileSerial = QString("/sys/dev/char/%1:%2/device/driver/module/drivers/usb-serial:cp210x").arg(maj).arg(min);
    if (QFileInfo(sysFile).exists() || QFileInfo(sysFileSerial).exists()) return true;
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
    qDebug()<<"** CONNECTION ESTABLISHED channel:"<<channel<<"device:"<<device_number<<"**";
    emit foundDevice(channel, device_number, device_id);

    // KICKR DETECTED - ACT ACCORDINGLY !
    // if we just got a power device and its recognised as being a kickr
    // trainer then we will need to open up the comms channel, unless it
    // has already been done
    if (!configuring && antChannel[channel]->is_kickr && kickrDeviceID != device_number) {

        kickrDeviceID = device_number;
        kickrChannel = channel;

        // lets go ?

        // need to find a way to communicate back on error
        qDebug()<<"kickr found."<<kickrDeviceID<<"on channel"<<kickrChannel;
    }

    // ANT FE-C DEVICE DETECTED - ACT ACCORDINGLY !
    // if we just got an ANT FE-C trainer, request the capabilities
    if (!configuring && antChannel[channel]->is_fec) {
        antChannel[channel]->capabilities();
        qDebug()<<"ANT FE-C device found."<<device_number<<"on channel"<<channel;
    }

    //qDebug()<<"found device number"<<device_number<<"type"<<device_id<<"on channel"<<channel
    //<< "is a "<<deviceTypeDescription(device_id) << "with code"<<deviceTypeCode(device_id);
}

void
ANT::dropInfo(int channel, int drops, int received)    // we dropped a message
{
    double reliability = 100.00f - (100.00f * double(drops) / double(received));
    qDebug()<<"Channel"<<channel<<"reliability is"<< (int)(reliability)<<"%";
    emit signalStrength(channel, reliability);
    return;
}

void
ANT::lostInfo(int number)    // we lost the connection
{
    if (number < 0 || number >= channels) return; // ignore out of bound

    emit lostDevice(number);
    qDebug()<<"lost info for channel"<<number;
}

void
ANT::staleInfo(int number)   // info is now stale - set to zero
{
    if (number < 0 || number >= channels) return; // ignore out of bound

    qDebug()<<"stale info for channel"<<number;
}

void
ANT::slotSearchTimeout(int number) // search timed out
{
    if (number < 0 || number >= channels) return; // ignore out of bound

    emit searchTimeout(number);
    qDebug()<<"search timeout on channel"<<number;
}

void
ANT::slotSearchComplete(int number) // search completed successfully
{
    if (number < 0 || number >= channels) return; // ignore out of bound

    emit searchComplete(number);
    qDebug()<<"search completed on channel"<<number;
}

/*----------------------------------------------------------------------
 * Message I/O
 *--------------------------------------------------------------------*/
void
ANT::sendMessage(ANTMessage m) {

  //fprintf(stderr, ">> send: ");
  //for(int i=0; i<m.length+3; i++) fprintf(stderr, "%02x ", m.data[i]);
  //fprintf(stderr, "\n");
  if (m.length > 12) {
    rawWrite((uint8_t*)m.data, m.length);
  } else {
    // Messages less than 12 bytes in length must be padded to a
    // minimum of 12 bytes.
    uint8_t msg[16];
    memset(msg, 0, sizeof(msg));
    memcpy(msg, m.data, m.length);
    rawWrite(msg, 12);
  }
}

void
ANT::receiveByte(uint8_t byte) {

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

//fprintf(stderr, "<< receive %i: ", rxMessage[ANT_OFFSET_CHANNEL_NUMBER]);
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
#ifdef GC_HAVE_LIBUSB
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
    return -1;
#endif
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
#ifdef GC_HAVE_LIBUSB
    if (usb2->find() == true) return true;
#if defined(WIN32) && defined(GC_HAVE_USBXPRESS)
    if (USBXpress::find() == true) return true;
#endif
#endif
    return false;
}

int ANT::openPort()
{
#ifdef WIN32
#ifdef GC_HAVE_LIBUSB
    int rc;

    // on windows we try on USB2 then on USB1 then fail...
    if ((rc=usb2->open()) != -1) {
        usbMode = USB2;
        channels = 8;
        return rc;
    }
#if defined(WIN32) && defined(GC_HAVE_USBXPRESS)
    else if ((rc= USBXpress::open(&devicePort)) != -1) {
        usbMode = USB1;
        channels = 4;
        return rc;
    }
#endif
    else {
        usbMode = USBNone;
        channels = 0;
        return -1;
    }
#else
    return -1;
#endif
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
#ifdef GC_HAVE_LIBUSB
    switch (usbMode) {
#ifdef GC_HAVE_USBXPRESS
    case USB1:
        rc = USBXpress::write(&devicePort, bytes, size);
        break;
#endif
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
    return rc=-1;
#endif
#else

#ifdef GC_HAVE_LIBUSB
    if (usbMode == USB2) {
        return usb2->write(bytes, size);
    }

    if (usbMode == USB1) {
        int ibytes;

        ioctl(devicePort, FIONREAD, &ibytes);

        // timeouts are less critical for writing, since vols are low
        rc= write(devicePort, bytes, size);

        if (rc != -1) tcdrain(devicePort); // wait till its gone.

        ioctl(devicePort, FIONREAD, &ibytes);
        return rc;
    }
#endif
#endif
    return -1;

}

int ANT::rawRead(uint8_t bytes[], int size)
{
#ifdef WIN32
#ifdef GC_HAVE_LIBUSB
    switch (usbMode) {
#ifdef GC_HAVE_USBXPRESS
    case USB1:
        return USBXpress::read(&devicePort, bytes, size);
        break;
#endif
    case USB2:
        return usb2->read((char *)bytes, size);
        break;
    default:
        break;
    }

#else
    return -1;
#endif
#else

#ifdef GC_HAVE_LIBUSB
    if (usbMode == USB2) {
        return usb2->read(bytes, size);
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

void ANT::setVortexData(int channel, int id)
{
    vortexChannel = channel;
    vortexID = id;
}

void ANT::setFecChannel(int channel)
{
    fecChannel = channel;
}
