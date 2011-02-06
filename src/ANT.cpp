/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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
  { CHANNEL_TYPE_UNUSED, 0, 0, 0, 0, "Unused", '?' },
  { CHANNEL_TYPE_HR, ANT_SPORT_HR_PERIOD, ANT_SPORT_HR_TYPE, ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Heartrate", 'h' },
  { CHANNEL_TYPE_POWER, ANT_SPORT_POWER_PERIOD, ANT_SPORT_POWER_TYPE, ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Power", 'p' },
  { CHANNEL_TYPE_SPEED, ANT_SPORT_SPEED_PERIOD, ANT_SPORT_SPEED_TYPE, ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Speed", 's' },
  { CHANNEL_TYPE_CADENCE, ANT_SPORT_CADENCE_PERIOD, ANT_SPORT_CADENCE_TYPE, ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Cadence", 'c' },
  { CHANNEL_TYPE_SandC, ANT_SPORT_SandC_PERIOD, ANT_SPORT_SandC_TYPE, ANT_SPORT_FREQUENCY, ANT_SPORT_NETWORK_NUMBER, "Speed + Cadence", 'd' },
  { CHANNEL_TYPE_QUARQ, ANT_QUARQ_PERIOD, ANT_QUARQ_TYPE, ANT_QUARQ_FREQUENCY, DEFAULT_NETWORK_NUMBER, "Quarq Channel", 'Q' },
  { CHANNEL_TYPE_FAST_QUARQ, ANT_FAST_QUARQ_PERIOD, ANT_FAST_QUARQ_TYPE, ANT_FAST_QUARQ_FREQUENCY, DEFAULT_NETWORK_NUMBER, "Fast Quarq", 'q' },
  { CHANNEL_TYPE_FAST_QUARQ_NEW, ANT_FAST_QUARQ_PERIOD, ANT_FAST_QUARQ_TYPE_WAS, ANT_FAST_QUARQ_FREQUENCY, DEFAULT_NETWORK_NUMBER, "Fast Quarq New", 'n' },
  { CHANNEL_TYPE_GUARD, 0, 0, 0, 0, "", '\0' }
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

    // ant ids
    antIDs = devConf->deviceProfile.split(",");
    lastReadHR = lastReadWatts = lastReadCadence = lastReadSpeed = 0;
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
    qDebug() << "Starting ANT thread...";
    int status; // control commands from controller
    bool isPortOpen = false;

    Status = ANT_RUNNING;
    QString strBuf;

    if (openPort() == 0) {
        isPortOpen = true;
        ANT_ResetSystem();
        ANT_SetNetworkKey(1, key);
        elapsedTime.start();
    } else {
        quit(0);
        return;
    }

    while(1)
    {
        // read more bytes from the device
        uint8_t byte;
        if (rawRead(&byte, 1) > 0) ANT_ReceiveByte((unsigned char)byte);

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
qDebug()<<"Stopping ANT thread...";
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

void
ANT::channel_manager_init(channel_manager_t *self) {
  int i;
  
  for (i=0; i<ANT_CHANNEL_COUNT; i++)
    ant_channel_init(self->channels+i, i, self);  

#if 0 // XXX need to understand this better, not used obviously!
  if (0) {
    channel_manager_remember_device(self, 17, CHANNEL_TYPE_POWER);
    channel_manager_remember_device(self, 17, CHANNEL_TYPE_HR);
    channel_manager_remember_device(self, 17, CHANNEL_TYPE_SandC);
    channel_manager_remember_device(self, 17, CHANNEL_TYPE_SPEED);
  
    return;
  }
#endif
  
}

// returns 1 for success, 0 for fail.
int
ANT::channel_manager_add_device(channel_manager_t *self, int device_number, int device_type, int channel_number) {
  int i;  

  // if we're given a device number, then use that one
  if (channel_number>-1) {
    ant_channel_close(self->channels+channel_number);
    ant_channel_open(self->channels+channel_number, device_number, device_type);
    return 1;
  }

  // if we already have the device, then return.
  for (i=0; i<ANT_CHANNEL_COUNT; i++)
    if (((self->channels[i].channel_type & 0xf ) == device_type) &&
	(self->channels[i].device_number == device_number)) {
      // send the channel found...
      ant_channel_channel_info(self->channels+i);
      return 1; 
    }
  
  // look for an unused channel and use on that one
  for (i=0; i<ANT_CHANNEL_COUNT; i++)
    if (self->channels[i].channel_type == CHANNEL_TYPE_UNUSED) {
      
      fprintf(stderr, "opening channel #%d\n",i);

      ant_channel_open(self->channels+i, device_number, device_type);
      return 1;
    }

  // there are no unused channels.  fail.
  return 0;  
}

// returns 1 for successfully removed, 0 for none found.
int
ANT::channel_manager_remove_device(channel_manager_t *self, int device_number, int channel_type) {

  int i;

  for (i=0; i<ANT_CHANNEL_COUNT; i++) {
    ant_channel_t *ac = self->channels+i;

    if ((ac->channel_type == channel_type) && 
	(ac->device_number == device_number)) {
      
      if ((ac->control_channel!=ac) && ac->control_channel) {
      
	channel_manager_remove_device(self, device_number, ac->control_channel->channel_type);
      }
      
      ant_channel_close(ac);

      ac->channel_type=CHANNEL_TYPE_UNUSED;
      ac->device_number=0;
      ant_channel_set_id(ac);
      return 1;
    }
  }

  // device not found.
  return 0;
}

ant_channel_t *
ANT::channel_manager_find_device(channel_manager_t *self, int device_number, int channel_type) {

  int i;

  for (i=0; i<ANT_CHANNEL_COUNT; i++)
    if (((self->channels[i].channel_type) == channel_type) && 
	(self->channels[i].device_number==device_number)) {      
      return self->channels+i;
    }

  // device not found.
  return NULL;

}

int
ANT::channel_manager_start_waiting_search(channel_manager_t *self) {
  
  int i;
  // are any fast searches in progress?  if so, then bail
  for (i=0; i<ANT_CHANNEL_COUNT; i++) {
    if (self->channels[i].channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
      return 0;
    }
  }

  // start the first slow search
  for (i=0; i<ANT_CHANNEL_COUNT; i++) {
    if (self->channels[i].channel_type_flags & CHANNEL_TYPE_WAITING) {
      self->channels[i].channel_type_flags &= ~CHANNEL_TYPE_WAITING;
      ANT_UnassignChannel(i);
      return 1;
    }
  }
  
  return 0;
}

void
ANT::channel_manager_report(channel_manager_t *self) {
  int i;

  for (i=0; i<ANT_CHANNEL_COUNT; i++) 
    ant_channel_channel_info(self->channels+i);

}

void
ANT::channel_manager_associate_control_channels(channel_manager_t *self) {
  int i;

  // first, unassociate all control channels
  for (i=0; i<ANT_CHANNEL_COUNT; i++) 
    self->channels[i].control_channel=NULL;

  // then, associate cinqos:
  //    new cinqos get their own selves for control
  //    old cinqos, look for an open control channel
  //       if found and open, associate
  //       elif found and not open yet, nop
  //       elif not found, open one

  for (i=0; i<ANT_CHANNEL_COUNT; i++) {
    ant_channel_t *ac=self->channels+i;

    switch (ac->channel_type) {
    case CHANNEL_TYPE_POWER:
      if (ac->is_cinqo) {
	if (ac->is_old_cinqo) {
	  ant_channel_t * my_ant_channel;

	  my_ant_channel=channel_manager_find_device(self,
						     ac->device_number,
						     CHANNEL_TYPE_QUARQ);
	  if (!my_ant_channel) {
	    my_ant_channel=channel_manager_find_device(self,
						       ac->device_number,
						       CHANNEL_TYPE_FAST_QUARQ);
	  }
	  if (!my_ant_channel) {
	    my_ant_channel=channel_manager_find_device(self,
						       ac->device_number,
						       CHANNEL_TYPE_FAST_QUARQ_NEW);
	  }

	  if (my_ant_channel) {
	    if (ant_channel_is_searching(my_ant_channel)) {
	      ; // searching, just wait around 
	    } else {
	      ac->control_channel=my_ant_channel;
	      ant_channel_send_cinqo_success(ac);
	    }
	  } else { // no ant channel, let's start one
	    channel_manager_add_device(self, ac->device_number,
				       CHANNEL_TYPE_QUARQ, -1);
	  }
    
	} else { // new cinqo
	  ac->control_channel=ac;
	  ant_channel_send_cinqo_success(ac);
	}
      } // is_cinqo
      break;

    case CHANNEL_TYPE_FAST_QUARQ:
    case CHANNEL_TYPE_FAST_QUARQ_NEW:
    case CHANNEL_TYPE_QUARQ:
      ac->is_cinqo=1;
      ac->control_channel=ac;
    default:
      ;
    } // channel_type case
  } // for-loop

}

bool
ANT::discover(DeviceConfiguration *, QProgressDialog *)
{
#if 0
    QString strBuf;
    QStringList strList;
    sentDual = false;
    sentSpeed = false;
    sentHR = false;
    sentCad = false;
    sentPWR = false;

    openPort();

    QByteArray strPwr("X-set-channel: 0p"); //Power
    QByteArray strHR("X-set-channel: 0h"); //Heart Rate
    QByteArray strSpeed("X-set-channel: 0s"); //Speed
    QByteArray strCad("X-set-channel: 0c"); //Cadence
    QByteArray strDual("X-set-channel: 0d"); //Dual (Speed/Cadence)

    // tell quarqd to start scanning....
    if (tcpSocket->isOpen()) {
        tcpSocket->write(strPwr); //Power

    }

    QTime start;
    start.start();
    progress->setMaximum(50000);

    while(start.elapsed() <= 50000) //Scan for 50 seconds.
    {
        if (progress->wasCanceled())
        {
            tcpSocket->close();
            return false;
        }

        progress->setValue(start.elapsed());

        if(start.elapsed() >= 40000 && sentSpeed == false)
        {
            sentSpeed = true;
            tcpSocket->write(strSpeed); //Speed
        } else if(start.elapsed() >= 30000 && sentDual == false)
        {
            sentDual = true;
            tcpSocket->write(strDual); //Dual
        } else if(start.elapsed() >= 20000 && sentHR == false)
        {
            sentHR = true;
            tcpSocket->write(strHR); //HR
        } else if(start.elapsed() >= 10000 && sentCad == false)
        {
            sentCad = true;
            tcpSocket->write(strCad); //Cadence
        } else if(start.elapsed() >= 0 && sentPWR == false)
        {
            sentPWR = true;
            tcpSocket->write(strPwr);
        }

        if (tcpSocket->bytesAvailable() > 0) {
            QByteArray array = tcpSocket->readAll();
            strBuf = array;
            //            qDebug() << strBuf;
            QStringList qList = strBuf.split("\n");

            //Loop for all the elements.
            for(int i=0; i<qList.size(); i++)
            {
                progress->setValue(start.elapsed());
                QString str = qList.at(i);
                //                qDebug() << str;
                if(str.contains("id"))
                {
                    int start = str.indexOf("id");
                    start += 4;
                    int end = str.indexOf("'", start);
                    QString id = str.mid(start, end - start);
                    if(!strList.contains(id))
                    {
                        if(id != "0p" && id != "0h" && id != "0s" && id != "0c" && id != "0d")
                            strList.append(id);
                    }
                }
            }
        }
    }
    progress->setValue(50000);//We are done.
    //Now return a comma delimited string.
    for(int i=0; i < strList.size(); i++)
    {
        config->deviceProfile.append(strList.at(i));
        if(i < strList.size() -1)
            config->deviceProfile.append(',');
    }

    return config;
#endif
    return false;
}

void ANT::reinitChannel(QString /*_channel*/)
{
#if 0
    if(!tcpSocket->isValid())
        return;
    qDebug() << "Reinit: " << _channel;

    QByteArray channel("X-set-channel: ");
    channel.append(_channel);
    tcpSocket->write(channel);
#endif
}


void
ANT::initChannel()
{
#if 0
    qDebug() << "Init channel..";
    if(!tcpSocket->isValid())
        return;

    QByteArray setChannel("X-set-channel: ");
    QByteArray channel;
    for(int i=0; i < antIDs.size(); i++)
    {
        if(tcpSocket->isValid())
        {
            channel.clear();
            channel = setChannel;
            channel.append(antIDs.at(i));
            tcpSocket->write(channel);
            tcpSocket->flush();
            qDebug() << channel;
            sleep(2);
        }
    }
#endif
}


/*======================================================================
 * ANT Channels
 *====================================================================*/

static float timeout_blanking=2.0;  // time before reporting stale data, seconds
static float timeout_drop=2.0; // time before reporting dropped message
static float timeout_scan=10.0; // time to do initial scan
static float timeout_lost=30.0; // time to do more thorough scan 

void
ANT::ant_channel_set_id(ant_channel_t *self) {
  if ((self->channel_type)==CHANNEL_TYPE_UNUSED) {
    strcpy(self->id, "none");
  } else {
    snprintf(self->id, 10, "%d%c", self->device_number, ant_sensor_types[self->channel_type].suffix);
  }
}

void
ANT::ant_channel_init(ant_channel_t *self, int number, channel_manager_t *parent) {
  
  self->parent=parent;
  
  self->channel_type=CHANNEL_TYPE_UNUSED;
  self->channel_type_flags=0;
  self->number=number;
  

  self->is_cinqo=0;
  self->is_old_cinqo=0;
  self->control_channel=NULL;  
  self->manufacturer_id=0;
  self->product_id=0;
  self->product_version=0;

  self->device_number=0;
  self->channel_assigned=0;

  ant_channel_set_id(self);

// XXX fixme - channel event function setup...
#if 0
  ANT_AssignChannelEventFunction(self->number, 
				 (void (*)(void *, unsigned char*)) ant_channel_receive_message,
				 (void *)self);
#endif

  self->state=ANT_UNASSIGN_CHANNEL;
  self->blanked=1;

  self->messages_received=0;
  self->messages_dropped=0;

  ant_channel_burst_init(self);  

  INITIALIZE_MESSAGES_INITIALIZATION(self->mi);
}

const char *
ANT::ant_channel_get_description(ant_channel_t *self) {  
  return ant_sensor_types[self->channel_type].descriptive_name;
}

int
ANT::ant_channel_interpret_description(char *description) {
  const ant_sensor_type_t *st=ant_sensor_types;

  do {
    if (0==strcmp(st->descriptive_name,description)) 
      return st->type;
  } while (++st, st->type != CHANNEL_TYPE_GUARD);
  
  return -1;
}

int
ANT::ant_channel_interpret_suffix(char c) {
  const ant_sensor_type_t *st=ant_sensor_types;

  do {
    if (st->suffix==c)
      return st->type;
  } while (++st, st->type != CHANNEL_TYPE_GUARD);
  
  return -1;
}

void
ANT::ant_channel_open(ant_channel_t *self, int device_number, int channel_type) {
  self->channel_type=channel_type;
  self->channel_type_flags = CHANNEL_TYPE_QUICK_SEARCH ; 
  self->device_number=device_number;

  ant_channel_set_id(self);

  if (self->channel_assigned)
    ANT_UnassignChannel(self->number);        
  else 
    ant_channel_attempt_transition(self,ANT_UNASSIGN_CHANNEL);      
}


void
ANT::ant_channel_close(ant_channel_t *self) {  
  ant_channel_lost_info(self);

    ANT_Close(self->number); 

}

void
ANT::ant_channel_receive_message(ant_channel_t *self, unsigned char *ant_message) {

  unsigned char *message=ant_message+2;

#ifdef DEBUG
  int i;
  if (quarqd_config.debug_level & DEBUG_LEVEL_ANT_MESSAGES) {
    fprintf(stderr, "Got a message\n");
    for (i=0; i<ant_message[ANT_OFFSET_LENGTH]; i++) {
      fprintf(stderr, "0x%02x ",message[i]);
    }
    fprintf(stderr, "\n");

    ant_message_print_debug(message);
  }
#endif

  switch (message[0]) {
  case ANT_CHANNEL_EVENT:
    ant_channel_channel_event(self, ant_message);
    break;
  case ANT_BROADCAST_DATA:
    ant_channel_broadcast_event(self, ant_message);   
    break;
  case ANT_ACK_DATA:
    ant_channel_ack_event(self, ant_message);   
    break;
  case ANT_CHANNEL_ID:
    ant_channel_channel_id(self, ant_message);
    break;
  case ANT_BURST_DATA:
    ant_channel_burst_data(self, ant_message);
    break;
  default:
    break; //XXX should trap error here, but silently ignored for now
  }  

  if (get_timestamp() > self->blanking_timestamp + timeout_blanking) {
    if (!self->blanked) {
      self->blanked=1;
      ant_channel_stale_info(self);
    }
  } else self->blanked=0;
}


void
ANT::ant_channel_channel_event(ant_channel_t *self, unsigned char *ant_message) { 
  unsigned char *message=ant_message+2;

  if (MESSAGE_IS_RESPONSE_NO_ERROR(message)) {
    ant_channel_attempt_transition(self,RESPONSE_NO_ERROR_MESSAGE_ID(message));
  } else if (MESSAGE_IS_EVENT_CHANNEL_CLOSED(message)) {
    ANT_UnassignChannel(self->number);
  } else if (MESSAGE_IS_EVENT_RX_SEARCH_TIMEOUT(message)) {
    // timeouts are normal for search channel, so don't send xml for those
    if (self->channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
      DEBUG_ANT_CONNECTION("Got timeout on channel %d.  Turning off search.\n", self->number);
      self->channel_type_flags &= ~CHANNEL_TYPE_QUICK_SEARCH;		
      self->channel_type_flags |= CHANNEL_TYPE_WAITING;

    } else {
      DEBUG_ANT_CONNECTION("Got timeout on channel %d.  search is off.\n", self->number);

      ant_channel_lost_info(self);

      self->channel_type=CHANNEL_TYPE_UNUSED;
      self->channel_type_flags=0;
      self->device_number=0;
      ant_channel_set_id(self);

      ANT_UnassignChannel(self->number);      
    } 

    DEBUG_ANT_CONNECTION("Rx search timeout on %d\n",self->number);
    channel_manager_start_waiting_search(self->parent);
  
  } else if (MESSAGE_IS_EVENT_RX_FAIL(message)) {
    //ant_message_print_debug(message);        

    self->messages_dropped++;

    double t=get_timestamp();

    //fprintf(stderr, "timediff %f\n",self->last_message_timestamp-t);

    if (t > (self->last_message_timestamp + timeout_drop)) {
      if (self->channel_type != CHANNEL_TYPE_UNUSED)
	ant_channel_drop_info(self);      
      // this is a hacky way to prevent the drop message from sending multiple times
      self->last_message_timestamp+=2*timeout_drop;
    }

  } else if (MESSAGE_IS_EVENT_RX_ACKNOWLEDGED(message)) {
    exit(-10);
  } else if (MESSAGE_IS_EVENT_TRANSFER_TX_COMPLETED(message)) {
    if (self->tx_ack_disposition) {
      self->tx_ack_disposition(self);
    }
  } else {
      ant_message_print_debug(message);    
    ; // default
  }
}

void
ANT::ant_channel_send_cinqo_error(ant_channel_t * /*self*/) {
  //XmlPrintf("<CinQoError id='%s' />\n", self->id); 
}

void
ANT::ant_channel_send_cinqo_success(ant_channel_t * /*self*/) {
  //XmlPrintf("<CinQoConnected id='%s' />\n", self->id); 
}

void
ANT::ant_channel_check_cinqo(ant_channel_t *self) {
  int version_hi, version_lo, swab_version;

  version_hi=(self->product_version >> 8) &0xff;
  version_lo=(self->product_version & 0xff);

  swab_version=version_lo | (version_hi<<8);

  fprintf(stderr, "check cinqo\n");
  fprintf(stderr, "Product id %x\n",self->product_id);
  fprintf(stderr, "Manu id %x\n",self->manufacturer_id);

  fprintf(stderr, "Product version %x\n",self->product_version);
  fprintf(stderr, "Product version cvs rev#%d\n",swab_version);
  fprintf(stderr, "Product version %d | %d\n",version_hi, version_lo);

  if (!(self->mi.first_time_manufacturer || self->mi.first_time_product)) {
    if ((self->product_id == 1) && (self->manufacturer_id==7)) { 
      // we are a cinqo, were we aware of this?
      self->is_cinqo=1;

      // are we an old-version or new-version cinqo?
      self->is_old_cinqo = ((version_hi <= 17) && (version_lo==10));

      fprintf(stderr, "I'm a cinqo %d!\n",self->is_old_cinqo);

      channel_manager_associate_control_channels(self->parent);
    } // cinqo check
  }
}

void
ANT::ant_channel_broadcast_event(ant_channel_t *self, unsigned char *ant_message) { 

  unsigned char *message=ant_message+2;
  static char last_message[ANT_MAX_MESSAGE_SIZE];
  timestamp=get_timestamp();

  self->messages_received++;
  self->last_message_timestamp=timestamp;

  if (self->state != MESSAGE_RECEIVED) {
    // first message! who are we talking to?
    ANT_RequestMessage(self->number, ANT_CHANNEL_ID);
    self->blanking_timestamp=get_timestamp();
    self->blanked=0;
    return; // because we can't associate a channel id with the message yet
  } 

  if (0==memcmp(message, last_message, ANT_MAX_MESSAGE_SIZE)) {
    //fprintf(stderr, "No change\n");
    return; // no change
  }

  {
    // for automatically opening quarq channel on early cinqo
    if (MESSAGE_IS_PRODUCT(message)) {
      self->mi.first_time_product=0;
      self->product_version&=0x00ff; 
      self->product_version|=(PRODUCT_SW_REV(message))<<8;
      ant_channel_check_cinqo(self);
    } else if (MESSAGE_IS_MANUFACTURER(message)) {
      self->mi.first_time_manufacturer=0;
      self->product_version&=0xff00;
      self->product_version|=MANUFACTURER_HW_REV(message);
      self->manufacturer_id=MANUFACTURER_MANUFACTURER_ID(message);
      self->product_id=MANUFACTURER_MODEL_NUMBER_ID(message);
      ant_channel_check_cinqo(self);
    }            
  }

// XXX need to decide what to do with these messages
#if 0
  {
    int matched=0;
    
    switch(self->channel_type) {
    case CHANNEL_TYPE_HR:
      ant_message_print_debug(message);
      matched=xml_message_interpret_heartrate_broadcast(self, message);
      break;
    case CHANNEL_TYPE_POWER:    
      matched=xml_message_interpret_power_broadcast(self, message);
      break;
    case CHANNEL_TYPE_SPEED:
      matched=xml_message_interpret_speed_broadcast(self, message);
      break;
    case CHANNEL_TYPE_CADENCE:
      matched=xml_message_interpret_cadence_broadcast(self, message);
      break;
    case CHANNEL_TYPE_SandC:
      matched=xml_message_interpret_speed_cadence_broadcast(self, message);
      break;
    case CHANNEL_TYPE_QUARQ:
    case CHANNEL_TYPE_FAST_QUARQ:
    case CHANNEL_TYPE_FAST_QUARQ_NEW:
#ifdef QUARQ_BUILD
      matched=xml_message_interpret_quarq_broadcast(self, message);
#else
      matched=xml_message_interpret_power_broadcast(self, message);
#endif
      break;
    default:      
      break;
    }
   
    if ((!matched) && (quarqd_config.debug_level & DEBUG_LEVEL_ERRORS)) {
      int i;
      fprintf(stderr, "Unknown Message!\n");
      for (i=0; i<ant_message[ANT_OFFSET_LENGTH]; i++) {
	fprintf(stderr, "0x%02x ",message[i]);
      }
      fprintf(stderr, "\n");

      ant_message_print_debug(message);
      
      //exit(-1); // for testing
    } 
  }
#endif
}

void
ANT::ant_channel_ack_event(ant_channel_t * /*self*/, unsigned char * /*ant_message*/) { 

#if 0
  unsigned char *message=ant_message+2;
  {
    int matched=0;
    
    switch(self->channel_type) {
    case CHANNEL_TYPE_POWER:    
      matched=xml_message_interpret_power_broadcast(self, message);
      break;  
    }

    if ((!matched) && (quarqd_config.debug_level & DEBUG_LEVEL_ERRORS)) {
      int i;
      fprintf(stderr, "Unknown Message!\n");
      for (i=0; i<ant_message[ANT_OFFSET_LENGTH]; i++) {
	fprintf(stderr, "0x%02x ",message[i]);
      }
      fprintf(stderr, "\n");

      ant_message_print_debug(message);
      
      exit(-1); // for testing
    } 
  }
#endif
}


void
ANT::ant_channel_channel_id(ant_channel_t *self, unsigned char *ant_message) { 
  
  unsigned char *message=ant_message+2;
  
  self->device_number=CHANNEL_ID_DEVICE_NUMBER(message);
  self->device_id=CHANNEL_ID_DEVICE_TYPE_ID(message);
  
  ant_channel_set_id(self);

  self->state=MESSAGE_RECEIVED;

  DEBUG_ANT_CONNECTION("%d: Connected to %s (0x%x) device number %d\n",
		       self->number,
		       ant_channel_get_description(self),
		       self->device_id,
		       self->device_number);
  
  ant_channel_channel_info(self);

  // if we were searching, 
  if (self->channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
    ANT_SetChannelSearchTimeout(self->number, 
				(int)(timeout_lost/2.5));
  }  
  self->channel_type_flags &= ~CHANNEL_TYPE_QUICK_SEARCH;

  channel_manager_start_waiting_search(self->parent);

  // if we are quarq channel, hook up with the ant+ channel we are connected to
  channel_manager_associate_control_channels(self->parent);
  
}

void
ANT::ant_channel_burst_init(ant_channel_t *self) {
  self->rx_burst_data_index=0;
  self->rx_burst_next_sequence=0;
  self->rx_burst_disposition=NULL;
}

int
ANT::ant_channel_is_searching(ant_channel_t *self) {
  return ((self->channel_type_flags & (CHANNEL_TYPE_WAITING | CHANNEL_TYPE_QUICK_SEARCH)) || (self->state != MESSAGE_RECEIVED)); 
}


void
ANT::ant_channel_burst_data(ant_channel_t *self, unsigned char *ant_message) {

  unsigned char *message=ant_message+2;
  char seq=(message[1]>>5)&0x3;
  char last=(message[1]>>7)&0x1;
  const unsigned char next_sequence[4]={1,2,3,1};

  if (seq!=self->rx_burst_next_sequence) {
    DEBUG_ERRORS("Bad sequence %d not %d\n",seq,self->rx_burst_next_sequence); 
   // burst problem!
  } else {
    int len=ant_message[ANT_OFFSET_LENGTH]-3;
    
    if ((self->rx_burst_data_index + len)>(RX_BURST_DATA_LEN)) {
      len = RX_BURST_DATA_LEN-self->rx_burst_data_index;
    }

    self->rx_burst_next_sequence=next_sequence[(int)seq];
    memcpy(self->rx_burst_data+self->rx_burst_data_index, message+2, len);
    self->rx_burst_data_index+=len; 
    
    //fprintf(stderr, "Copying %d bytes.\n",len);
  }

  if (last) {
    if (self->rx_burst_disposition) {
      self->rx_burst_disposition(self);
    }
    ant_channel_burst_init(self);
  }
}

void
ANT::ant_channel_request_calibrate(ant_channel_t *self) {  
  ANT_RequestCalibrate(self->number);
}

void
ANT::ant_channel_attempt_transition(ant_channel_t *self, int message_id) {
  
  const ant_sensor_type_t *st;

  int previous_state=self->state;

  st=ant_sensor_types+(self->channel_type);


  // update state
  self->state=message_id;

  // do transitions
  switch (self->state) {
  case ANT_CLOSE_CHANNEL:
    // next step is unassign and start over
    // but we must wait until event_channel_closed
    // which is its own channel event
    self->state=MESSAGE_RECEIVED; 
    break;
  case ANT_UNASSIGN_CHANNEL:
    self->channel_assigned=0;
    if (st->type==CHANNEL_TYPE_UNUSED) {
      // we're shutting the channel down

    } else {

      self->device_id=st->device_id;
	
      if (self->channel_type & CHANNEL_TYPE_PAIR) {
	self->device_id |= 0x80;
      }
      
      ant_channel_set_id(self);
    
      DEBUG_ANT_CONNECTION("Opening for %s\n",ant_channel_get_description(self));
      ANT_AssignChannel(self->number, 0, st->network); // recieve channel on network 1
    }
    break;
  case ANT_ASSIGN_CHANNEL:
    self->channel_assigned=1;
    ANT_SetChannelID(self->number, self->device_number, self->device_id, 0);
    break;
  case ANT_CHANNEL_ID:
    if (self->channel_type & CHANNEL_TYPE_QUICK_SEARCH) {
      DEBUG_ANT_CONNECTION("search\n");
      ANT_SetChannelSearchTimeout(self->number, 
				  (int)(timeout_scan/2.5));
    } else {
      DEBUG_ANT_CONNECTION("nosearch\n");
      ANT_SetChannelSearchTimeout(self->number, 
				  (int)(timeout_lost/2.5));
    }    
    break;
  case ANT_SEARCH_TIMEOUT:
    if (previous_state==ANT_CHANNEL_ID) {
      // continue down the intialization chain
      ANT_SetChannelPeriod(self->number, st->period); 
    } else {
      // we are setting the ant_search timeout after connected
      // we'll just pretend this never happened
      DEBUG_ANT_CONNECTION("resetting ant_search timeout.\n");
      self->state=previous_state; 
    }
    break;
  case ANT_CHANNEL_PERIOD:
    ANT_SetChannelFreq(self->number, st->frequency);
    break;
  case ANT_CHANNEL_FREQUENCY:
    ANT_Open(self->number);
    INITIALIZE_MESSAGES_INITIALIZATION(self->mi);
    break;
  case ANT_OPEN_CHANNEL:
    DEBUG_ANT_CONNECTION("Channel %d opened for %s\n",self->number,
			 ant_channel_get_description(self));
    break;
  default:
    DEBUG_ERRORS("unknown channel event 0x%x\n",message_id);
  }  
}

#define BXmlPrintf(format, args...) \
  self->blanking_timestamp=timestamp; \
  XmlPrintf(format, ##args)

#define RememberMessage(message_len, if_changed)		      \
  static unsigned char last_messages[ANT_CHANNEL_COUNT][message_len]; \
  unsigned char * last_message=last_messages[self->number]; \
  \
  if (!first_time) { \
    if (0!=memcmp(message,last_message,message_len)) {	\
      if_changed;					\
    } \
  } else {	  \
    first_time=0; \
  } \
  memcpy(last_message,message,message_len)

#define RememberXmlPrintf(message_len, format, args...)	\
  RememberMessage(message_len, XmlPrintf(format, ##args))

#define BlankingXmlPrintf(message_len, format, args...)	\
  RememberMessage(message_len, BXmlPrintf(format, ##args))

void
ANT::ant_channel_channel_info(ant_channel_t * /*self*/) {
#if 0
  XmlPrintf("<SensorFound id='%s' device_number='%d' type='%s%s'%s ant_channel='%d' messages_received='%d' messages_dropped='%d' drop_percent='%d'/>\n",
	    self->id,
	    self->device_number,
	    ant_channel_get_description(self),
	    ((self->state==MESSAGE_RECEIVED || 
	      (ant_sensor_types[self->channel_type]).type==CHANNEL_TYPE_UNUSED)?
	     "":" (searching)"),
	    (self->channel_type_flags&CHANNEL_TYPE_PAIR)?" paired":"",
	    self->number,
	    self->messages_received,
	    self->messages_dropped,
	    self->messages_received? \
	    100*self->messages_dropped/self->messages_received : 0);
#endif
}

void
ANT::ant_channel_drop_info(ant_channel_t * /*self*/) {  
#if 0
  XmlPrintf("<SensorDrop id='%s' device_number='%d' type='%s' timeout='%.1f' ant_channel='%d'/>\n",
	    self->id,
	    self->device_number,
	    ant_channel_get_description(self),
	    timeout_drop,
	    self->number);
#endif
}

void
ANT::ant_channel_lost_info(ant_channel_t * /*self*/) {  
#if 0
  XmlPrintf("<SensorLost id='%s' device_number='%d' type='%s' timeout='%.1f' ant_channel='%d'/>\n",
	    self->id,
	    self->device_number,
	    ant_channel_get_description(self),	    
	    timeout_lost,
	    self->number);
#endif
}

void
ANT::ant_channel_stale_info(ant_channel_t * /*self*/) {  
#if 0
  XmlPrintf("<SensorStale id='%s' device_number='%d' type='%s' timeout='%.1f' ant_channel='%d'/>\n",
	    self->id,
	    self->device_number,
	    ant_channel_get_description(self),	    
	    timeout_blanking,
	    self->number);
#endif
}

void
ANT::ant_channel_report_timeouts( void ) {
#if 0
  XmlPrintf("<Timeout type='blanking' value='%.2f'>\n"
	    "<Timeout type='drop' value='%.2f'>\n"
	    "<Timeout type='scan' value='%.2f'>\n"
	    "<Timeout type='lost' value='%.2f'>\n",
	    timeout_blanking,
	    timeout_drop,
	    timeout_scan,
	    timeout_lost);
#endif
}

int
ANT::ant_channel_set_timeout( char *type, float value, int /*connection*/) {

  if (0==strcmp(type,"blanking")) timeout_blanking=value;
  else if (0==strcmp(type,"drop")) timeout_drop=value;
  else if (0==strcmp(type,"scan")) timeout_scan=value;
  else if (0==strcmp(type,"lost")) timeout_lost=value;
  else {
    ant_channel_report_timeouts();
    return 0;
  } 
  ant_channel_report_timeouts();
  return 1;
}

// this is in the wrong place here for the convenience of the 
// XmlPrintf macro
void
ANT::ant_channel_search_complete( void ) {
#if 0
  XmlPrintf("<SearchFinished />\n");  
#endif
}

// XXX decide how to manage this
#if 0
float
ANT::get_srm_offset(int device_id) {
  float ret=0.0;
  read_offset_log(device_id, &ret);
  return ret;
}

void
ANT::set_srm_offset(int device_id, float value) {
  update_offset_log(device_id, value);
}
#endif

/*======================================================================
 * ANT Messaging
 *====================================================================*/

void
ANT::ANT_ResetSystem(void){

	txMessage[ANT_OFFSET_LENGTH] = 1;
	txMessage[ANT_OFFSET_ID] = ANT_SYSTEM_RESET;
	ANT_SendMessage();
}

void
ANT::ANT_AssignChannel(const unsigned char channel, const unsigned char type, const unsigned char network) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;

	txMessage[ANT_OFFSET_LENGTH] = 3;
	txMessage[ANT_OFFSET_ID] = ANT_ASSIGN_CHANNEL;
	*data++ = channel;
	*data++ = type;
	*data++ = network;

	ANT_SendMessage();

}

void
ANT::ANT_UnassignChannel(const unsigned char channel) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;

	txMessage[ANT_OFFSET_LENGTH] = 1;
	txMessage[ANT_OFFSET_ID] = ANT_UNASSIGN_CHANNEL;
	*data = channel;

	ANT_SendMessage();
}

void
ANT::ANT_SetChannelSearchTimeout(const unsigned char channel, const unsigned char search_timeout) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;	

	txMessage[ANT_OFFSET_LENGTH] = 2;
	txMessage[ANT_OFFSET_ID] = ANT_SEARCH_TIMEOUT;
	*data++ = channel;
	*data++ = search_timeout;

	ANT_SendMessage();
}

void
ANT::ANT_RequestMessage(const unsigned char channel, const unsigned char request) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;

	txMessage[ANT_OFFSET_LENGTH] = 2;
	txMessage[ANT_OFFSET_ID] = ANT_REQ_MESSAGE;
	*data++ = channel;
	*data++ = request;

	ANT_SendMessage();
}

void
ANT::ANT_SetChannelID(const unsigned char channel, const unsigned short device,
                      const unsigned char deviceType, const unsigned char txType) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;

	txMessage[ANT_OFFSET_LENGTH] = 5;
	txMessage[ANT_OFFSET_ID] = ANT_CHANNEL_ID;
	*data++ = channel;
	*data++ = device & 0xff;
	*data++ = (device >> 8) & 0xff;
	*data++ = deviceType;
	*data++ = txType;

	ANT_SendMessage();
}

void
ANT::ANT_SetChannelPeriod(const unsigned char channel, const unsigned short period) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;

	txMessage[ANT_OFFSET_LENGTH] = 3;
	txMessage[ANT_OFFSET_ID] = ANT_CHANNEL_PERIOD;
	*data++ = channel;
	*data++ = period & 0xff;
	*data++ = (period >> 8) & 0xff;

	ANT_SendMessage();
}

void
ANT::ANT_SetChannelFreq(const unsigned char channel, const unsigned char frequency) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;

	txMessage[ANT_OFFSET_LENGTH] = 2;
	txMessage[ANT_OFFSET_ID] = ANT_CHANNEL_FREQUENCY;
	*data++ = channel;
	*data++ = frequency;

	ANT_SendMessage();
}

void
ANT::ANT_SetNetworkKey(const unsigned char net, const unsigned char *key) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;
	int i;

	txMessage[ANT_OFFSET_LENGTH] = 9;
	txMessage[ANT_OFFSET_ID] = ANT_SET_NETWORK;
	*data++ = net;
	for (i = 0; i < ANT_KEY_LENGTH; i++)
		*data++ = key[i];

	ANT_SendMessage();

}

void
ANT::ANT_SendAckMessage( void ) {

    memcpy(txMessage, txAckMessage, ANT_MAX_MESSAGE_SIZE);
	ANT_SendMessage();

}

void
ANT::ANT_SetAutoCalibrate(const unsigned char channel, const int autozero) { 
	unsigned char *data = txAckMessage + ANT_OFFSET_DATA;

	txAckMessage[ANT_OFFSET_LENGTH] = 4;
	txAckMessage[ANT_OFFSET_ID] = ANT_ACK_DATA;
	*data++ = channel;
	*data++ = ANT_SPORT_CALIBRATION_MESSAGE;
	*data++ = ANT_SPORT_CALIBRATION_REQUEST_AUTOZERO_CONFIG;
	*data++ = autozero ? ANT_SPORT_AUTOZERO_ON : ANT_SPORT_AUTOZERO_OFF;

	ANT_SendAckMessage();
}

void
ANT::ANT_RequestCalibrate(const unsigned char channel) { 
	unsigned char *data = txAckMessage + ANT_OFFSET_DATA;

	txAckMessage[ANT_OFFSET_LENGTH] = 4;
	txAckMessage[ANT_OFFSET_ID] = ANT_ACK_DATA;
	*data++ = channel;
	*data++ = ANT_SPORT_CALIBRATION_MESSAGE;
	*data++ = ANT_SPORT_CALIBRATION_REQUEST_MANUALZERO;
	*data++ = 0;

	ANT_SendAckMessage();

}

void
ANT::ANT_Open(const unsigned char channel) {
	unsigned char *data = txMessage + ANT_OFFSET_DATA;

	txMessage[ANT_OFFSET_LENGTH] = 1;
	txMessage[ANT_OFFSET_ID] = ANT_OPEN_CHANNEL;
	*data++ = channel;

	ANT_SendMessage();
}

void
ANT::ANT_Close(const unsigned char channel) {

	unsigned char *data = txMessage + ANT_OFFSET_DATA;

	txMessage[ANT_OFFSET_LENGTH] = 1;
	txMessage[ANT_OFFSET_ID] = ANT_CLOSE_CHANNEL;
	*data++ = channel;

	ANT_SendMessage();
}

void
ANT::ANT_SendMessage(void) {
    int i;
	const int length = txMessage[ANT_OFFSET_LENGTH] + ANT_OFFSET_DATA;
	unsigned char checksum = ANT_SYNC_BYTE;
	
	txMessage[ANT_OFFSET_SYNC] = ANT_SYNC_BYTE;

	for (i = ANT_OFFSET_LENGTH; i < length; i++)
		checksum ^= txMessage[i];
	
	txMessage[i++] = checksum;

	txMessage[i++] = 0x0;
	txMessage[i++] = 0x0;
	txMessage[i++] = 0x0;
	txMessage[i++] = 0x0;
	txMessage[i++] = 0x0;

	ANT_Transmit(txMessage, i);
}

void 
ANT::ANT_Transmit(const unsigned char *msg, int length) {
    if (rawWrite((uint8_t*)msg, length) < 0) {
        //XXX error somewhere;
    }
}

void
ANT::ANT_ReceiveByte(unsigned char byte) {

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
				ANT_ProcessMessage();
			}
			state = ST_WAIT_FOR_SYNC;
			break;
	}
}


void
ANT::ANT_HandleChannelEvent(void) {
	int channel = rxMessage[ANT_OFFSET_DATA] & 0x7;
	if(channel >= 0 && channel < 4) {

    // handle a channel event here!
qDebug()<<"channel event on channel:"<<channel;
#if 0
	  if(eventFuncs[channel] != NULL) {	    
	    eventFuncs[channel](privateData[channel],rxMessage);
	  }
#endif
	}
}

void
ANT::ANT_ProcessMessage(void) {

qDebug()<<"processing ant message"<<rxMessage[ANT_OFFSET_ID];

	switch (rxMessage[ANT_OFFSET_ID]) {
		case ANT_ACK_DATA:
		case ANT_BROADCAST_DATA:
		case ANT_CHANNEL_STATUS:
		case ANT_CHANNEL_ID:
		case ANT_BURST_DATA:
			ANT_HandleChannelEvent();
			break;

		case ANT_CHANNEL_EVENT:
		  switch (rxMessage[ANT_OFFSET_MESSAGE_CODE]) {
		  case EVENT_TRANSFER_TX_FAILED:
		    fprintf(stderr, "\nResending ack.\n");
		    ANT_SendAckMessage();
		    break;
		  case EVENT_TRANSFER_TX_COMPLETED:		    
		    fprintf(stderr, "\nAck sent.\n");
		    // fall through
		  default:
		    ANT_HandleChannelEvent();
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
 * Serial Port I/O
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

    if ((devicePort=open(deviceFilename.toAscii(),O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) return errno;

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

    // success
    return 0;
}

int ANT::rawWrite(uint8_t *bytes, int size) // unix!!
{
    int rc=0,ibytes;

#ifdef WIN32
    DWORD cBytes;
    rc = WriteFile(devicePort, bytes, size, &cBytes, NULL);
    if (!rc) return -1;
    return rc;

#else

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

    // Readfile deals with timeouts and readyread issues
    DWORD cBytes;
    rc = ReadFile(devicePort, bytes, size, &cBytes, NULL);
    if (rc) return (int)cBytes;
    else return (-1);

#else

    int timeout=0, i=0;
    uint8_t byte;

    // read one byte at a time sleeping when no data ready
    // until we timeout waiting then return error
    for (i=0; i<size; i++) {
        timeout =0;
        rc=0;
        while (rc==0 && timeout < ANT_READTIMEOUT) {
            rc = read(devicePort, &byte, 1);
            if (rc == -1) return -1; // error!
            else if (rc == 0) {
                msleep(50); // sleep for 1/20th of a second
                timeout += 50;
            } else {
                bytes[i] = byte;
            }
        }
        if (timeout >= ANT_READTIMEOUT) return -1; // we timed out!
    }

    return i;

#endif
}

//======================================================================
// Quarqd style debug message output
//======================================================================

// note: this was imported from quarqd_dist/quarqd/src/generated-debug.c

void
ANT::ant_message_print_debug(unsigned char * message) {
// assign_channel
if (( message[0]==0x42 )) {
	unsigned char channel = message[0];
	unsigned char channel_type = message[0];
	unsigned char network_number = message[0];

	fprintf(stderr,"assign_channel:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tchannel_type: %d (0x%02X)\n",channel_type,channel_type);
	fprintf(stderr,"\tnetwork_number: %d (0x%02X)\n",network_number,network_number);
} else
// unassign_channel
if (( message[0]==0x41 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"unassign_channel:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// open_channel
if (( message[0]==0x4b )) {
	unsigned char channel = message[0];

	fprintf(stderr,"open_channel:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// channel_id
if (( message[0]==0x51 )) {
	unsigned char channel = message[0];
	unsigned short device_number = (message[1]<<8)+(message[0]);
	unsigned char device_type_id = message[0];
	unsigned char transmission_type = message[0];

	fprintf(stderr,"channel_id:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tdevice_number: %d\n",device_number);
	fprintf(stderr,"\tdevice_type_id: %d (0x%02X)\n",device_type_id,device_type_id);
	fprintf(stderr,"\ttransmission_type: %d (0x%02X)\n",transmission_type,transmission_type);
} else
// burst_message
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];
	unsigned char data5 = message[0];
	unsigned char data6 = message[0];
	unsigned char data7 = message[0];

	fprintf(stderr,"burst_message:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
	fprintf(stderr,"\tdata5: %d (0x%02X)\n",data5,data5);
	fprintf(stderr,"\tdata6: %d (0x%02X)\n",data6,data6);
	fprintf(stderr,"\tdata7: %d (0x%02X)\n",data7,data7);
} else
// burst_message7
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];
	unsigned char data5 = message[0];
	unsigned char data6 = message[0];

	fprintf(stderr,"burst_message7:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
	fprintf(stderr,"\tdata5: %d (0x%02X)\n",data5,data5);
	fprintf(stderr,"\tdata6: %d (0x%02X)\n",data6,data6);
} else
// burst_message6
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];
	unsigned char data5 = message[0];

	fprintf(stderr,"burst_message6:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
	fprintf(stderr,"\tdata5: %d (0x%02X)\n",data5,data5);
} else
// burst_message5
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];

	fprintf(stderr,"burst_message5:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
} else
// burst_message4
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];

	fprintf(stderr,"burst_message4:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
} else
// burst_message3
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];

	fprintf(stderr,"burst_message3:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
} else
// burst_message2
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];

	fprintf(stderr,"burst_message2:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
} else
// burst_message1
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];
	unsigned char data0 = message[0];

	fprintf(stderr,"burst_message1:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
} else
// burst_message0
if (( message[0]==0x50 )) {
	unsigned char chan_seq = message[0];

	fprintf(stderr,"burst_message0:\n");
	fprintf(stderr,"\tchan_seq: %d (0x%02X)\n",chan_seq,chan_seq);
} else
// channel_period
if (( message[0]==0x43 )) {
	unsigned char channel = message[0];
	unsigned short period = (message[1]<<8)+(message[0]);

	fprintf(stderr,"channel_period:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tperiod: %d\n",period);
} else
// search_timeout
if (( message[0]==0x44 )) {
	unsigned char channel = message[0];
	unsigned char search_timeout = message[0];

	fprintf(stderr,"search_timeout:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tsearch_timeout: %d (0x%02X)\n",search_timeout,search_timeout);
} else
// channel_frequency
if (( message[0]==0x45 )) {
	unsigned char channel = message[0];
	unsigned char rf_frequency = message[0];

	fprintf(stderr,"channel_frequency:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\trf_frequency: %d (0x%02X)\n",rf_frequency,rf_frequency);
} else
// set_network
if (( message[0]==0x46 )) {
	unsigned char channel = message[0];
	unsigned char key0 = message[0];
	unsigned char key1 = message[0];
	unsigned char key2 = message[0];
	unsigned char key3 = message[0];
	unsigned char key4 = message[0];
	unsigned char key5 = message[0];
	unsigned char key6 = message[0];
	unsigned char key7 = message[0];

	fprintf(stderr,"set_network:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tkey0: %d (0x%02X)\n",key0,key0);
	fprintf(stderr,"\tkey1: %d (0x%02X)\n",key1,key1);
	fprintf(stderr,"\tkey2: %d (0x%02X)\n",key2,key2);
	fprintf(stderr,"\tkey3: %d (0x%02X)\n",key3,key3);
	fprintf(stderr,"\tkey4: %d (0x%02X)\n",key4,key4);
	fprintf(stderr,"\tkey5: %d (0x%02X)\n",key5,key5);
	fprintf(stderr,"\tkey6: %d (0x%02X)\n",key6,key6);
	fprintf(stderr,"\tkey7: %d (0x%02X)\n",key7,key7);
} else
// transmit_power
if (( message[0]==0x47 ) && ( message[0]==0x00 )) {
	unsigned char tx_power = message[0];

	fprintf(stderr,"transmit_power:\n");
	fprintf(stderr,"\ttx_power: %d (0x%02X)\n",tx_power,tx_power);
} else
// reset_system
if (( message[0]==0x4a )) {


	fprintf(stderr,"reset_system:\n");
} else
// request_message
if (( message[0]==0x4d )) {
	unsigned char channel = message[0];
	unsigned char message_requested = message[0];

	fprintf(stderr,"request_message:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_requested: %d (0x%02X)\n",message_requested,message_requested);
} else
// close_channel
if (( message[0]==0x4c )) {
	unsigned char channel = message[0];

	fprintf(stderr,"close_channel:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_no_error
if (( message[0]==0x40 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"response_no_error:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// response_assign_channel_ok
if (( message[0]==0x40 ) && ( message[0]==0x42 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_assign_channel_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_channel_unassign_ok
if (( message[0]==0x40 ) && ( message[0]==0x41 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_channel_unassign_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_open_channel_ok
if (( message[0]==0x40 ) && ( message[0]==0x4b ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_open_channel_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_channel_id_ok
if (( message[0]==0x40 ) && ( message[0]==0x51 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_channel_id_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_channel_period_ok
if (( message[0]==0x40 ) && ( message[0]==0x43 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_channel_period_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_channel_frequency_ok
if (( message[0]==0x40 ) && ( message[0]==0x45 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_channel_frequency_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_set_network_ok
if (( message[0]==0x40 ) && ( message[0]==0x46 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_set_network_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_transmit_power_ok
if (( message[0]==0x40 ) && ( message[0]==0x47 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_transmit_power_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_close_channel_ok
if (( message[0]==0x40 ) && ( message[0]==0x4c ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_close_channel_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// response_search_timeout_ok
if (( message[0]==0x40 ) && ( message[0]==0x44 ) && ( message[0]==0x00 )) {
	unsigned char channel = message[0];

	fprintf(stderr,"response_search_timeout_ok:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// event_rx_search_timeout
if (( message[0]==0x40 ) && ( message[0]==0x01 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_rx_search_timeout:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_rx_fail
if (( message[0]==0x40 ) && ( message[0]==0x02 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_rx_fail:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_tx
if (( message[0]==0x40 ) && ( message[0]==0x03 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_tx:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_transfer_rx_failed
if (( message[0]==0x40 ) && ( message[0]==0x04 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_transfer_rx_failed:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_transfer_tx_completed
if (( message[0]==0x40 ) && ( message[0]==0x05 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_transfer_tx_completed:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_transfer_tx_failed
if (( message[0]==0x40 ) && ( message[0]==0x06 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_transfer_tx_failed:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_channel_closed
if (( message[0]==0x40 ) && ( message[0]==0x07 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_channel_closed:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_rx_fail_go_to_search
if (( message[0]==0x40 ) && ( message[0]==0x08 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_rx_fail_go_to_search:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_channel_collision
if (( message[0]==0x40 ) && ( message[0]==0x09 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_channel_collision:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_transfer_tx_start
if (( message[0]==0x40 ) && ( message[0]==0x0a )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_transfer_tx_start:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_rx_acknowledged
if (( message[0]==0x40 ) && ( message[0]==0x0b )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_rx_acknowledged:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// event_rx_burst_packet
if (( message[0]==0x40 ) && ( message[0]==0x0c )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"event_rx_burst_packet:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// channel_in_wrong_state
if (( message[0]==0x40 ) && ( message[0]==0x15 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"channel_in_wrong_state:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// channel_not_opened
if (( message[0]==0x40 ) && ( message[0]==0x16 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"channel_not_opened:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// channel_id_not_set
if (( message[0]==0x40 ) && ( message[0]==0x18 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"channel_id_not_set:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// transfer_in_progress
if (( message[0]==0x40 ) && ( message[0]==0x1f )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"transfer_in_progress:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// channel_status
if (( message[0]==0x52 )) {
	unsigned char channel = message[0];
	unsigned char status = message[0];

	fprintf(stderr,"channel_status:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tstatus: %d (0x%02X)\n",status,status);
} else
// cw_init
if (( message[0]==0x53 )) {


	fprintf(stderr,"cw_init:\n");
} else
// cw_test
if (( message[0]==0x48 )) {
	unsigned char power = message[0];
	unsigned char freq = message[0];

	fprintf(stderr,"cw_test:\n");
	fprintf(stderr,"\tpower: %d (0x%02X)\n",power,power);
	fprintf(stderr,"\tfreq: %d (0x%02X)\n",freq,freq);
} else
// capabilities
if (( message[0]==0x54 )) {
	unsigned char max_channels = message[0];
	unsigned char max_networks = message[0];
	unsigned char standard_options = message[0];
	unsigned char advanced_options = message[0];

	fprintf(stderr,"capabilities:\n");
	fprintf(stderr,"\tmax_channels: %d (0x%02X)\n",max_channels,max_channels);
	fprintf(stderr,"\tmax_networks: %d (0x%02X)\n",max_networks,max_networks);
	fprintf(stderr,"\tstandard_options: %d (0x%02X)\n",standard_options,standard_options);
	fprintf(stderr,"\tadvanced_options: %d (0x%02X)\n",advanced_options,advanced_options);
} else
// capabilities_extended
if (( message[0]==0x54 )) {
	unsigned char max_channels = message[0];
	unsigned char max_networks = message[0];
	unsigned char standard_options = message[0];
	unsigned char advanced_options = message[0];
	unsigned char advanced_options_2 = message[0];
	unsigned char max_data_channels = message[0];

	fprintf(stderr,"capabilities_extended:\n");
	fprintf(stderr,"\tmax_channels: %d (0x%02X)\n",max_channels,max_channels);
	fprintf(stderr,"\tmax_networks: %d (0x%02X)\n",max_networks,max_networks);
	fprintf(stderr,"\tstandard_options: %d (0x%02X)\n",standard_options,standard_options);
	fprintf(stderr,"\tadvanced_options: %d (0x%02X)\n",advanced_options,advanced_options);
	fprintf(stderr,"\tadvanced_options_2: %d (0x%02X)\n",advanced_options_2,advanced_options_2);
	fprintf(stderr,"\tmax_data_channels: %d (0x%02X)\n",max_data_channels,max_data_channels);
} else
// ant_version
if (( message[0]==0x3e )) {
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];
	unsigned char data5 = message[0];
	unsigned char data6 = message[0];
	unsigned char data7 = message[0];
	unsigned char data8 = message[0];

	fprintf(stderr,"ant_version:\n");
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
	fprintf(stderr,"\tdata5: %d (0x%02X)\n",data5,data5);
	fprintf(stderr,"\tdata6: %d (0x%02X)\n",data6,data6);
	fprintf(stderr,"\tdata7: %d (0x%02X)\n",data7,data7);
	fprintf(stderr,"\tdata8: %d (0x%02X)\n",data8,data8);
} else
// ant_version_long
if (( message[0]==0x3e )) {
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];
	unsigned char data5 = message[0];
	unsigned char data6 = message[0];
	unsigned char data7 = message[0];
	unsigned char data8 = message[0];
	unsigned char data9 = message[0];
	unsigned char data10 = message[0];
	unsigned char data11 = message[0];
	unsigned char data12 = message[0];

	fprintf(stderr,"ant_version_long:\n");
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
	fprintf(stderr,"\tdata5: %d (0x%02X)\n",data5,data5);
	fprintf(stderr,"\tdata6: %d (0x%02X)\n",data6,data6);
	fprintf(stderr,"\tdata7: %d (0x%02X)\n",data7,data7);
	fprintf(stderr,"\tdata8: %d (0x%02X)\n",data8,data8);
	fprintf(stderr,"\tdata9: %d (0x%02X)\n",data9,data9);
	fprintf(stderr,"\tdata10: %d (0x%02X)\n",data10,data10);
	fprintf(stderr,"\tdata11: %d (0x%02X)\n",data11,data11);
	fprintf(stderr,"\tdata12: %d (0x%02X)\n",data12,data12);
} else
// transfer_seq_number_error
if (( message[0]==0x40 ) && ( message[0]==0x20 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"transfer_seq_number_error:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// invalid_message
if (( message[0]==0x40 ) && ( message[0]==0x28 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"invalid_message:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// invalid_network_number
if (( message[0]==0x40 ) && ( message[0]==0x29 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];

	fprintf(stderr,"invalid_network_number:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
} else
// channel_response
if (( message[0]==0x40 )) {
	unsigned char channel = message[0];
	unsigned char message_id = message[0];
	unsigned char message_code = message[0];

	fprintf(stderr,"channel_response:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmessage_id: %d (0x%02X)\n",message_id,message_id);
	fprintf(stderr,"\tmessage_code: %d (0x%02X)\n",message_code,message_code);
} else
// extended_broadcast_data
if (( message[0]==0x5d )) {
	unsigned char channel = message[0];
	unsigned short device_number = (message[1]<<8)+(message[0]);
	unsigned char device_type = message[0];
	unsigned char transmission_type = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];
	unsigned char data5 = message[0];
	unsigned char data6 = message[0];
	unsigned char data7 = message[0];

	fprintf(stderr,"extended_broadcast_data:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tdevice_number: %d\n",device_number);
	fprintf(stderr,"\tdevice_type: %d (0x%02X)\n",device_type,device_type);
	fprintf(stderr,"\ttransmission_type: %d (0x%02X)\n",transmission_type,transmission_type);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
	fprintf(stderr,"\tdata5: %d (0x%02X)\n",data5,data5);
	fprintf(stderr,"\tdata6: %d (0x%02X)\n",data6,data6);
	fprintf(stderr,"\tdata7: %d (0x%02X)\n",data7,data7);
} else
// extended_ack_data
if (( message[0]==0x5e )) {
	unsigned char channel = message[0];
	unsigned short device_number = (message[1]<<8)+(message[0]);
	unsigned char device_type = message[0];
	unsigned char transmission_type = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];
	unsigned char data5 = message[0];
	unsigned char data6 = message[0];
	unsigned char data7 = message[0];

	fprintf(stderr,"extended_ack_data:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tdevice_number: %d\n",device_number);
	fprintf(stderr,"\tdevice_type: %d (0x%02X)\n",device_type,device_type);
	fprintf(stderr,"\ttransmission_type: %d (0x%02X)\n",transmission_type,transmission_type);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
	fprintf(stderr,"\tdata5: %d (0x%02X)\n",data5,data5);
	fprintf(stderr,"\tdata6: %d (0x%02X)\n",data6,data6);
	fprintf(stderr,"\tdata7: %d (0x%02X)\n",data7,data7);
} else
// extended_burst_data
if (( message[0]==0x5f )) {
	unsigned char channel = message[0];
	unsigned short device_number = (message[1]<<8)+(message[0]);
	unsigned char device_type = message[0];
	unsigned char transmission_type = message[0];
	unsigned char data0 = message[0];
	unsigned char data1 = message[0];
	unsigned char data2 = message[0];
	unsigned char data3 = message[0];
	unsigned char data4 = message[0];
	unsigned char data5 = message[0];
	unsigned char data6 = message[0];
	unsigned char data7 = message[0];

	fprintf(stderr,"extended_burst_data:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tdevice_number: %d\n",device_number);
	fprintf(stderr,"\tdevice_type: %d (0x%02X)\n",device_type,device_type);
	fprintf(stderr,"\ttransmission_type: %d (0x%02X)\n",transmission_type,transmission_type);
	fprintf(stderr,"\tdata0: %d (0x%02X)\n",data0,data0);
	fprintf(stderr,"\tdata1: %d (0x%02X)\n",data1,data1);
	fprintf(stderr,"\tdata2: %d (0x%02X)\n",data2,data2);
	fprintf(stderr,"\tdata3: %d (0x%02X)\n",data3,data3);
	fprintf(stderr,"\tdata4: %d (0x%02X)\n",data4,data4);
	fprintf(stderr,"\tdata5: %d (0x%02X)\n",data5,data5);
	fprintf(stderr,"\tdata6: %d (0x%02X)\n",data6,data6);
	fprintf(stderr,"\tdata7: %d (0x%02X)\n",data7,data7);
} else
// startup_message
if (( message[0]==0x6f )) {
	unsigned char start_message = message[0];

	fprintf(stderr,"startup_message:\n");
	fprintf(stderr,"\tstart_message: %d (0x%02X)\n",start_message,start_message);
} else
// calibration_request
if (( message[0]==0x01 ) && ( message[0]==0xaa )) {
	unsigned char channel = message[0];

	fprintf(stderr,"calibration_request:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
} else
// srm_zero_response
if (( message[0]==0x01 ) && ( message[0]==0x10 ) && ( message[0]==0x01 )) {
	unsigned char channel = message[0];
	unsigned short offset = (message[0]<<8)+(message[1]);

	fprintf(stderr,"srm_zero_response:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\toffset: %d\n",offset);
} else
// calibration_pass
if (( message[0]==0x01 ) && ( message[0]==0xac )) {
	unsigned char channel = message[0];
	unsigned char autozero_status = message[0];
	unsigned short calibration_data = (message[1]<<8)+(message[0]);

	fprintf(stderr,"calibration_pass:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tautozero_status: %d (0x%02X)\n",autozero_status,autozero_status);
	fprintf(stderr,"\tcalibration_data: %d\n",calibration_data);
} else
// calibration_fail
if (( message[0]==0x01 ) && ( message[0]==0xaf )) {
	unsigned char channel = message[0];
	unsigned char autozero_status = message[0];
	unsigned short calibration_data = (message[1]<<8)+(message[0]);

	fprintf(stderr,"calibration_fail:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tautozero_status: %d (0x%02X)\n",autozero_status,autozero_status);
	fprintf(stderr,"\tcalibration_data: %d\n",calibration_data);
} else
// torque_support
if (( message[0]==0x01 ) && ( message[0]==0x12 )) {
	unsigned char channel = message[0];
	unsigned char sensor_configuration = message[0];
	signed short raw_torque = (message[1]<<8)+(message[0]);
	signed short offset_torque = (message[1]<<8)+(message[0]);

	fprintf(stderr,"torque_support:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tsensor_configuration: %d (0x%02X)\n",sensor_configuration,sensor_configuration);
	fprintf(stderr,"\traw_torque: %d\n",raw_torque);
	fprintf(stderr,"\toffset_torque: %d\n",offset_torque);
} else
// standard_power
if (( message[0]==0x4e ) && ( message[0]==0x10 )) {
	unsigned char channel = message[0];
	unsigned char event_count = message[0];
	unsigned char instant_cadence = message[0];
	unsigned short sum_power = (message[1]<<8)+(message[0]);
	unsigned short instant_power = (message[1]<<8)+(message[0]);

	fprintf(stderr,"standard_power:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tevent_count: %d (0x%02X)\n",event_count,event_count);
	fprintf(stderr,"\tinstant_cadence: %d (0x%02X)\n",instant_cadence,instant_cadence);
	fprintf(stderr,"\tsum_power: %d\n",sum_power);
	fprintf(stderr,"\tinstant_power: %d\n",instant_power);
} else
// wheel_torque
if (( message[0]==0x4e ) && ( message[0]==0x11 )) {
	unsigned char channel = message[0];
	unsigned char event_count = message[0];
	unsigned char wheel_rev = message[0];
	unsigned char instant_cadence = message[0];
	unsigned short wheel_period = (message[1]<<8)+(message[0]);
	unsigned short torque = (message[1]<<8)+(message[0]);

	fprintf(stderr,"wheel_torque:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tevent_count: %d (0x%02X)\n",event_count,event_count);
	fprintf(stderr,"\twheel_rev: %d (0x%02X)\n",wheel_rev,wheel_rev);
	fprintf(stderr,"\tinstant_cadence: %d (0x%02X)\n",instant_cadence,instant_cadence);
	fprintf(stderr,"\twheel_period: %d\n",wheel_period);
	fprintf(stderr,"\ttorque: %d\n",torque);
} else
// crank_torque
if (( message[0]==0x4e ) && ( message[0]==0x12 )) {
	unsigned char channel = message[0];
	unsigned char event_count = message[0];
	unsigned char crank_rev = message[0];
	unsigned char instant_cadence = message[0];
	unsigned short crank_period = (message[1]<<8)+(message[0]);
	unsigned short torque = (message[1]<<8)+(message[0]);

	fprintf(stderr,"crank_torque:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tevent_count: %d (0x%02X)\n",event_count,event_count);
	fprintf(stderr,"\tcrank_rev: %d (0x%02X)\n",crank_rev,crank_rev);
	fprintf(stderr,"\tinstant_cadence: %d (0x%02X)\n",instant_cadence,instant_cadence);
	fprintf(stderr,"\tcrank_period: %d\n",crank_period);
	fprintf(stderr,"\ttorque: %d\n",torque);
} else
// crank_SRM
if (( message[0]==0x4e ) && ( message[0]==0x20 )) {
	unsigned char channel = message[0];
	unsigned char event_count = message[0];
	unsigned short slope = (message[0]<<8)+(message[1]);
	unsigned short crank_period = (message[0]<<8)+(message[1]);
	unsigned short torque = (message[0]<<8)+(message[1]);

	fprintf(stderr,"crank_SRM:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tevent_count: %d (0x%02X)\n",event_count,event_count);
	fprintf(stderr,"\tslope: %d\n",slope);
	fprintf(stderr,"\tcrank_period: %d\n",crank_period);
	fprintf(stderr,"\ttorque: %d\n",torque);
} else
// manufacturer
if (( message[0]==0x4e ) && ( message[0]==0x50 )) {
	unsigned char channel = message[0];
	unsigned char hw_rev = message[0];
	unsigned short manufacturer_id = (message[1]<<8)+(message[0]);
	unsigned short model_number_id = (message[1]<<8)+(message[0]);

	fprintf(stderr,"manufacturer:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\thw_rev: %d (0x%02X)\n",hw_rev,hw_rev);
	fprintf(stderr,"\tmanufacturer_id: %d\n",manufacturer_id);
	fprintf(stderr,"\tmodel_number_id: %d\n",model_number_id);
} else
// product
if (( message[0]==0x4e ) && ( message[0]==0x51 )) {
	unsigned char channel = message[0];
	unsigned char sw_rev = message[0];
	unsigned short serial_number_qpod = (message[1]<<8)+(message[0]);
	unsigned short serial_number_spider = (message[1]<<8)+(message[0]);

	fprintf(stderr,"product:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tsw_rev: %d (0x%02X)\n",sw_rev,sw_rev);
	fprintf(stderr,"\tserial_number_qpod: %d\n",serial_number_qpod);
	fprintf(stderr,"\tserial_number_spider: %d\n",serial_number_spider);
} else
// battery_voltage
if (( message[0]==0x4e ) && ( message[0]==0x52 )) {
	unsigned char channel = message[0];
	unsigned char operating_time_lsb = message[0];
	unsigned char operating_time_1sb = message[0];
	unsigned char operating_time_msb = message[0];
	unsigned char voltage_lsb = message[0];
	unsigned char descriptive_bits = message[0];

	fprintf(stderr,"battery_voltage:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\toperating_time_lsb: %d (0x%02X)\n",operating_time_lsb,operating_time_lsb);
	fprintf(stderr,"\toperating_time_1sb: %d (0x%02X)\n",operating_time_1sb,operating_time_1sb);
	fprintf(stderr,"\toperating_time_msb: %d (0x%02X)\n",operating_time_msb,operating_time_msb);
	fprintf(stderr,"\tvoltage_lsb: %d (0x%02X)\n",voltage_lsb,voltage_lsb);
	fprintf(stderr,"\tdescriptive_bits: %d (0x%02X)\n",descriptive_bits,descriptive_bits);
} else
// heart_rate
if (( message[0]==0x4e )) {
	unsigned char channel = message[0];
	unsigned short measurement_time = (message[1]<<8)+(message[0]);
	unsigned char beats = message[0];
	unsigned char instant_heart_rate = message[0];

	fprintf(stderr,"heart_rate:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmeasurement_time: %d\n",measurement_time);
	fprintf(stderr,"\tbeats: %d (0x%02X)\n",beats,beats);
	fprintf(stderr,"\tinstant_heart_rate: %d (0x%02X)\n",instant_heart_rate,instant_heart_rate);
} else
// speed
if (( message[0]==0x4e )) {
	unsigned char channel = message[0];
	unsigned short measurement_time = (message[1]<<8)+(message[0]);
	unsigned short wheel_revs = (message[1]<<8)+(message[0]);

	fprintf(stderr,"speed:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmeasurement_time: %d\n",measurement_time);
	fprintf(stderr,"\twheel_revs: %d\n",wheel_revs);
} else
// cadence
if (( message[0]==0x4e )) {
	unsigned char channel = message[0];
	unsigned short measurement_time = (message[1]<<8)+(message[0]);
	unsigned short crank_revs = (message[1]<<8)+(message[0]);

	fprintf(stderr,"cadence:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tmeasurement_time: %d\n",measurement_time);
	fprintf(stderr,"\tcrank_revs: %d\n",crank_revs);
} else
// speed_cadence
if (( message[0]==0x4e )) {
	unsigned char channel = message[0];
	unsigned short cadence_measurement_time = (message[1]<<8)+(message[0]);
	unsigned short crank_revs = (message[1]<<8)+(message[0]);
	unsigned short speed_measurement_time = (message[1]<<8)+(message[0]);
	unsigned short wheel_revs = (message[1]<<8)+(message[0]);

	fprintf(stderr,"speed_cadence:\n");
	fprintf(stderr,"\tchannel: %d (0x%02X)\n",channel,channel);
	fprintf(stderr,"\tcadence_measurement_time: %d\n",cadence_measurement_time);
	fprintf(stderr,"\tcrank_revs: %d\n",crank_revs);
	fprintf(stderr,"\tspeed_measurement_time: %d\n",speed_measurement_time);
	fprintf(stderr,"\twheel_revs: %d\n",wheel_revs);
} else
 { ; } 
} 
