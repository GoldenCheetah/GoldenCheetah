/*
 * Copyright (c) 2009 Steve Gribble (gribble [at] cs.washington.edu) and
 *                    Mark Liversedge (liversedge@gmail.com)
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

#include "GoldenClient.h"
#include <QMutexLocker>
#include <QSharedPointer>

bool GoldenClient::connect(QString server_hostname, quint16 server_port,
                           QString raceid, QString ridername, int ftp_watts,
                           float weight_kg) {
  QMutexLocker locker(&client_lock);

  // if the child thread is running, this means we're connected to the
  // server already, so return failure.
  if (running == true) return false;

  race_status.race_finished = false;
  race_status.race_distance_km = 0;
  race_status.membership_changed = false;
  race_status.race_id = raceid;
  race_status.riders_status = QMap<QString,RiderData>();

  // stash away locals.  according to the qt4 docs, this should do the
  // right thing with QString's, even though we're copying across
  // threads.  (yike!)
  remote_host = server_hostname;
  remote_port = server_port;
  rider_raceid = raceid;
  rider_name = ridername;
  rider_ftp_watts = ftp_watts;
  rider_weight_kg = weight_kg;

  // Spawn the child thread.  The child will try to connect to the
  // remote host, and signal us once it's succeeded or failed.
  running = true;
  QThread::start();
  client_cond.wait(&client_lock);

  // Did the child thread succeed?
  if (!connected) {
    // Failure, and the child thread will kill itself off, so wait for
    // it to die before returning.
    QThread::wait();
    running = connected = false;
    return false;
  }

  // Success!
  return true;
}

void GoldenClient::closeAndExit() {
  QMutexLocker locker(&client_lock);

  if (!running)
    return;
  kill_signal = true;
  client_cond.wait(&client_lock);
  kill_signal = false;

  // wait for child thread to actually exit
  QThread::wait();
}

void GoldenClient::sendTelemetry(int power_watts, int cadence_rpm,
                                 float distance_km, int heartrate_bpm,
                                 float speed_kph) {
  QMutexLocker locker(&client_lock);

  QSharedPointer<ProtocolMessage> tm(
     new TelemetryMessage(rider_raceid, rider_id, power_watts,
                          cadence_rpm, distance_km, heartrate_bpm,
                          speed_kph) );
  write_queue.enqueue(tm);
}

RaceStatus GoldenClient::getStandings() {
  QMutexLocker locker(&client_lock);

  RaceStatus ret_status = race_status;
  race_status.membership_changed = false;
  return ret_status;
}



// Here's where the client thread enters.  parent has already set
// "running = true;".
void GoldenClient::run() {
  QMutexLocker locker(&client_lock);
  QTcpSocket   server;

  // First order of business: connect to the remote server and perform
  // the protocol handshake.  On failure, set connected = false, close
  // the socket, and signal the parent.  On success, set connected =
  // true and signal the parent.
  server.connectToHost(remote_host, remote_port);
  if (!(server.waitForConnected(5000))) {
    // handshake failed, so signal the condition variable to
    // wake up the parent to let it know about the failure,
    // then return out of thread.
    running = connected = false;
    client_cond.wakeOne();
    return;
  }

  // Try to exchange the initial handshake with the remote server.
  HelloMessage hm("0.1", rider_raceid, rider_name,
                  rider_ftp_watts, rider_weight_kg);
  write_line(locker, server, hm.toString());
  QString server_response_text;
  if (!read_line(locker, server, true, server_response_text)) {
    server.disconnectFromHost();
    running = connected = false;
    client_cond.wakeOne();
    return;
  }
  QSharedPointer<ProtocolMessage> server_response =
    ProtocolHandler::parseLine(server_response_text);
  if (server_response->message_type == ProtocolMessage::HELLOFAIL) {
    // server didn't recognize our race id, presumably.  in any case,
    // return out of thread after indicating failure.
      QSharedPointer<HelloFailMessage> hfm =
        qSharedPointerDynamicCast<HelloFailMessage>(server_response);
    server.disconnectFromHost();
    running = connected = false;
    client_cond.wakeOne();
    return;
  } else if (server_response->message_type == ProtocolMessage::HELLOSUCCEED) {
    // server completed the handshake!  stash aside our riderid and
    // the race distance.
    QSharedPointer<HelloSucceedMessage> hsm =
      qSharedPointerDynamicCast<HelloSucceedMessage>(server_response);
    rider_id = hsm->riderid;
    race_distance_km = hsm->racedistance_km;
  } else {
    // fail -- unknown message type.  return out of thread after
    // indicating failure.
    server.disconnectFromHost();
    running = connected = false;
    client_cond.wakeOne();
    return;
  }

  // handshake success!  signal parent.
  connected = true;
  client_cond.wakeOne();

  // main loop of the child thread
  while(1) {

    // have I been told to kill myself?
    if (kill_signal) {
      // is the socket connected? if so, disconnect and close it.
      if (server.state() != QAbstractSocket::UnconnectedState) {
        server.disconnectFromHost();
      }
      running = connected = false;
      client_cond.wakeOne();
      return;
    }

    // Try non-blocking read from the network.
    QString nextline;
    if (read_line(locker, server, false, nextline)) {
      QSharedPointer<ProtocolMessage> server_msg =
        ProtocolHandler::parseLine(nextline);
      if (!handle_message(locker, server, server_msg)) {
        server.disconnectFromHost();
        running = connected = false;
        // sneaky race -- if somebody tells me to die *while* I'm
        // learning the server is down, I still have to ack the die
        // signal.
        if (kill_signal)
          client_cond.wakeOne();
        return;
      }

    }

    // Do I have any data to send?
    while(!write_queue.empty()) {
      QSharedPointer<ProtocolMessage> sndmsg = write_queue.dequeue();
      if (!write_line(locker, server,
                      sndmsg->toString())) {
        // write failed, close up shop.
        server.disconnectFromHost();
        running = connected = false;
        // sneaky race -- if somebody tells me to die *while* I'm
        // learning the server is down, I still have to ack the die
        // signal.
        if (kill_signal)
          client_cond.wakeOne();
        return;
      }
    }
  }
}

#define GC_MAX_CHARS_READLINE 256
bool GoldenClient::read_line(QMutexLocker &locker, QTcpSocket &server,
                             bool block, QString &read_into_me) {

  if (!server.canReadLine()) {
    // If "block = false", wait up to 50ms for the socket to be ready
    // for reading.
    //
    // If "block = true", wait forever for the socket to be ready for
    // reading.
    bool done = !block;
    do {
      locker.unlock();
      // wait up to 50ms for the socket to be ready to read,
      // even if we're called with "block = false".
      if (server.waitForReadyRead(50)) {
        done = true;
      }
      locker.relock();
    } while (!done);
  }
  if (!server.canReadLine())
    return false;

  // socket is ready for reading.  prep the buffer to read into.
  char databuf[GC_MAX_CHARS_READLINE+1];
  databuf[GC_MAX_CHARS_READLINE] = '\0';

  // read into the buffer.
  unsigned int readlen = server.readLine(databuf, GC_MAX_CHARS_READLINE);
  // if we didn't read any text, bork out.
  if (!(readlen > 0))
    return false;
  // if the line we read doesn't end in '\n', bork out.
  if (databuf[readlen-1] != '\n')
    return false;

  // win!  deep copy what we read into read_into_me, return true.
  read_into_me = (const char *) databuf;
  return true;
}

bool GoldenClient::write_line(QMutexLocker &locker, QTcpSocket &server,
                              QString line_to_write) {
  unsigned int num_written;

  locker.unlock();
  num_written = server.write(line_to_write.toAscii().constData(),
                             line_to_write.size());
  locker.relock();
  if (num_written != ((unsigned int) line_to_write.size()))
    return false;
  return true;
}

bool GoldenClient::handle_message(QMutexLocker &locker, QTcpSocket &server,
                                  QSharedPointer<ProtocolMessage> msg) {
  if (msg->message_type == ProtocolMessage::CLIENTLIST) {
    return handle_clientlist(
            locker, server, qSharedPointerDynamicCast<ClientListMessage>(msg));
  } else if (msg->message_type == ProtocolMessage::STANDINGS) {
    return handle_standings(
            locker, server, qSharedPointerDynamicCast<StandingsMessage>(msg));
  } else if (msg->message_type == ProtocolMessage::RACECONCLUDED) {
    return handle_raceconcluded(
            locker, server, qSharedPointerDynamicCast<RaceConcludedMessage>(msg));
  } else {
    return false;
  }
}

bool GoldenClient::handle_clientlist(QMutexLocker &locker, QTcpSocket &server,
                                     QSharedPointer<ClientListMessage> msg) {
  int numclients = msg->numclients;
  int i;
  RaceStatus new_status;

  new_status.membership_changed = true;
  new_status.race_finished = race_status.race_finished;
  new_status.race_id = rider_raceid;
  new_status.race_distance_km = race_distance_km;

  for (i=0; i<numclients; i++) {
    QString nextline;

    if (!read_line(locker, server, true, nextline)) {
      printf("readline failed\n");
      return false;
    }
    QSharedPointer<ProtocolMessage> next_msg =
      ProtocolHandler::parseLine(nextline);
    if (next_msg->message_type != ProtocolMessage::CLIENT)
      return false;
    QSharedPointer<ClientMessage> client_msg =
      qSharedPointerDynamicCast<ClientMessage>(next_msg);

    // initialize the rider data struct
    RiderData rider_data = {
      client_msg->ridername, client_msg->riderid, client_msg->ftp_watts,
      client_msg->weight_kg, 0, 0, 0.0, 0, 0.0, 0 };

    // populate telemetry fields from old struct in old map, if exists
    if (race_status.riders_status.contains(client_msg->riderid)) {
      RiderData old_data = race_status.riders_status.value(client_msg->riderid);
      rider_data.power_watts = old_data.power_watts;
      rider_data.cadence_rpm = old_data.cadence_rpm;
      rider_data.distance_km = old_data.distance_km;
      rider_data.heartrate_bpm = old_data.heartrate_bpm;

      rider_data.place = old_data.place;
    }

    // insert into the map
    new_status.riders_status.insert(rider_data.rider_id,
                                    rider_data);
  }

  // if the race has not yet finished, do the swap of old and new (a
  // big copy op).  if the race has finished, don't process any
  // membership changes -- we want the final standings to survive.
  if (!race_status.race_finished) {
    race_status = new_status;
  }
  return true;
}
bool GoldenClient::handle_standings(QMutexLocker &locker, QTcpSocket &server,
                                    QSharedPointer<StandingsMessage> msg) {
  int numclients = msg->numclients;
  int i;
  RaceStatus new_status;

  new_status.membership_changed = race_status.membership_changed;
  new_status.race_finished = race_status.race_finished;
  new_status.race_id = rider_raceid;
  new_status.race_distance_km = race_distance_km;

  for (i=0; i<numclients; i++) {
    QString nextline;

    if (!read_line(locker, server, true, nextline))
      return false;
    QSharedPointer<ProtocolMessage> next_msg =
      ProtocolHandler::parseLine(nextline);
    if (next_msg->message_type != ProtocolMessage::RACER)
      return false;
    QSharedPointer<RacerMessage> racer_msg =
      qSharedPointerDynamicCast<RacerMessage>(next_msg);

    // get old rider data
    if (!race_status.riders_status.contains(racer_msg->riderid)) {
      // big trouble! rider should be known to us..
      return false;
    }
    RiderData old_data = race_status.riders_status.value(racer_msg->riderid);

    // update the old rider data with new telemetry and standings
    old_data.power_watts = racer_msg->power_watts;
    old_data.cadence_rpm = racer_msg->cadence_rpm;
    old_data.distance_km = racer_msg->distance_km;
    old_data.heartrate_bpm = racer_msg->heartrate_bpm;
    old_data.speed_kph = racer_msg->speed_kph;
    old_data.place = racer_msg->place;

    // insert into the map
    new_status.riders_status.remove(old_data.rider_id);
    new_status.riders_status.insert(old_data.rider_id,
                                    old_data);
  }
  // if the race has not yet finished, do the swap of old and new (a
  // big copy op).  if the race has finished, don't process any
  // membership changes -- we want the final standings to survive.
  if (!race_status.race_finished) {
    race_status = new_status;
  }
  return true;
}
bool GoldenClient::handle_raceconcluded(QMutexLocker &locker, QTcpSocket &server,
                                        QSharedPointer<RaceConcludedMessage> msg) {
  int numclients = msg->numclients;
  int i;
  RaceStatus new_status;

  new_status.membership_changed = race_status.membership_changed;
  new_status.race_finished = true;
  new_status.race_id = rider_raceid;
  new_status.race_distance_km = race_distance_km;

  for (i=0; i<numclients; i++) {
    QString nextline;

    if (!read_line(locker, server, true, nextline))
      return false;
    QSharedPointer<ProtocolMessage> next_msg =
      ProtocolHandler::parseLine(nextline);
    if (next_msg->message_type != ProtocolMessage::RESULT)
      return false;
    QSharedPointer<ResultMessage> result_msg =
      qSharedPointerDynamicCast<ResultMessage>(next_msg);

    // update old rider data with new final standings
    RiderData old_data = race_status.riders_status.value(result_msg->riderid);

    // update the old rider data with new telemetry and standings
    old_data.distance_km = result_msg->distance_km;
    old_data.place = result_msg->place;

    // insert into the map
    new_status.riders_status.remove(old_data.rider_id);
    new_status.riders_status.insert(old_data.rider_id,
                                    old_data);
  }
  // do the swap of old and new (a big copy op)
  race_status = new_status;
  return true;
}

