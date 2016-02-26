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

#include "SimpleNetworkClient.h"
#include "RaceDispatcher.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"

#include <stdio.h>
#include <string.h>

#include <QMutexLocker>

SimpleNetworkClient::SimpleNetworkClient(QObject *parent,
                                         DeviceConfiguration *dc)
  : parent (parent), running(false), connected(false), kill_signal(false)
{
    server_hostname = dc->portSpec.section(':',0,0).toAscii(); // after the colon
    server_port = dc->portSpec.section(':',1,1).toInt(); // after the colon
}

SimpleNetworkClient::~SimpleNetworkClient() {
  closeAndExit();
}

bool SimpleNetworkClient::start() {
  QMutexLocker locker(&client_lock);

  if (running) {
    // already running; fail.
    return false;
  }

  // Fire up socket connector and reader thread
  QThread::start();

  // Wait for child to indicate that it connected to the server
  client_cond.wait(&client_lock);
  if (!connected) {
    QThread::wait();  // second of two ways a thread can exit
    return false;
  }

  printf("Connected to network...\n");
  return true;
}

void SimpleNetworkClient::closeAndExit() {
  QMutexLocker locker(&client_lock);

  if (!running)
    return;

  kill_signal = true;
  client_cond.wait(&client_lock);
  kill_signal = false;
  QThread::wait();  // second of two ways the thread can exit
}

bool SimpleNetworkClient::getRealtimeData(RealtimeData &rtData) {
  QMutexLocker locker(&client_lock);

  if (!connected) {
    return false;
  }
  rtData = read_data_cache;
  return true;
}

bool SimpleNetworkClient::pushRealtimeData(RealtimeData &rtData) {
  QMutexLocker locker(&client_lock);

  if (!connected) {
    return false;
  }


  // need second pair of eyes here: can somebody confirm this pushes a
  // copy of rtData into the queue, not the reference?  (I think
  // it pushes a copy because the type of write_queue is
  // queue<RealtimeData>,  not queue<RealtimeData&>.    )

  // push the realtime data into a queue to be written by the child
// New outbound  telemtry
  write_queue.enqueue(rtData);

rtData.setName((char *)"Me");
DISPATCHER->dispatch(&rtData);
  return true;
}


#define MAX_BYTES_PER_LINE 128

void SimpleNetworkClient::run() {
  QTcpSocket server;
  QMutexLocker locker(&client_lock);

  // signal that I'm running
  running = true;

  /////////////// try to connect to the remote host
  server.connectToHost(server_hostname, server_port);
  // wait up to 5 seconds to connect
  if (server.waitForConnected(5000)) {
    connected = true;
  }
  // let start() invoker know the outcome of connection attempt
  client_cond.wakeOne();
  if (connected == false) {
    server.close();
    running = connected = false;
    while (!write_queue.empty()) write_queue.dequeue();
    return;
  }

  /////////// Loop, reading lines in from the network and writing from the queue.
  while(1) {
    char network_data[MAX_BYTES_PER_LINE];

    // Try up to 1 second to read next line from network.
    if (!read_next_line(locker, server, read_data_cache, network_data)) {
      // read failed, close up shop.
      server.close();
      running = connected = false;
      while (!write_queue.empty()) write_queue.dequeue();
      if (kill_signal)
        client_cond.wakeOne();
      return;
    }

    // If anything in our write queue, push it into the network.
    while(!write_queue.empty()) {
      if (!write_next_line(locker,
                           server,
                           write_queue.dequeue(),
                           network_data)) {
        // write failed, close up shop.
        server.close();
        running = connected = false;
        while (!write_queue.empty()) write_queue.dequeue();
        if (kill_signal)
          client_cond.wakeOne();
        return;
      }
    }
  }
}

bool SimpleNetworkClient::read_next_line(QMutexLocker &locker,
                                         QTcpSocket &server,
                                         RealtimeData &read_into_me,
                                         char *next_line) {
  qint64 read_result;
  qint64 num_read_so_far;
  bool   done;

  next_line[0] = '\0';
  next_line[MAX_BYTES_PER_LINE-1] = '\0';  // to be safe
  num_read_so_far = 0;
  done = false;

  while(!done) {
    // wait up to a second for socket to be ready to read
    locker.unlock();
    server.waitForReadyRead(100);
    locker.relock();

    // re-entered lock; make sure I'm not told to kill myself.
    if (kill_signal) {
      printf("Got kill signal\n");
      return false;
    }

    read_result =
      server.readLine(next_line + num_read_so_far,
                      MAX_BYTES_PER_LINE - num_read_so_far - 1);
    if (read_result == -1) {
      return false;
    }

    if ((read_result == 0) && (num_read_so_far == 0)) {
      // didn't get anything yet, but need to give writing a
      // chance.  so-- return to main loop.
      return true;
    }

    num_read_so_far += read_result;
    if ((num_read_so_far == MAX_BYTES_PER_LINE - 1) ||
        ((num_read_so_far > 0) &&
         next_line[strlen(next_line)-1] == '\n'))
      done = true;
  }

  // Make sure there is a trailing newline.
  if (next_line[strlen(next_line) - 1] != '\n') {
    // nope; quit out.
    return false;
  }

  // Yup; try to parse it.
  {
    float watts, hr, speed, rpm, load;
    long time;
    char namechar[256];
    strcpy(namechar, "empty");
    if (sscanf(next_line, "%s %f %f %ld %f %f %f\n",
               &namechar[0], &watts, &hr, &time, &speed, &rpm, &load) != 7) {
      // couldn't parse, so quit.
      return false;
    }
    read_into_me.setName(namechar);
    read_into_me.setWatts(watts);
    read_into_me.setHr(hr);
    read_into_me.setMsecs(time);
    read_into_me.setSpeed(speed);
    read_into_me.setCadence(rpm);
    read_into_me.setLoad(load);

// New inbound telemtry
DISPATCHER->dispatch(&read_into_me);

    printf("Read from network: %f %f %ld %f %f %f\n",
           read_into_me.getWatts(), read_into_me.getHr(),
           read_into_me.getMsecs(), read_into_me.getSpeed(),
           read_into_me.getCadence(), read_into_me.getLoad());
  }
  return true;
}

bool SimpleNetworkClient::write_next_line(QMutexLocker &locker,
                                          QTcpSocket &server,
                                          RealtimeData record,
                                          char *buffer) {
  int num_written;
  snprintf(buffer, MAX_BYTES_PER_LINE-1,
           "%.2f %.2f %ld %.2f %.2f %.3f\n",
           record.getWatts(), record.getHr(), record.getMsecs(),
           record.getSpeed(), record.getCadence(), record.getLoad());
  locker.unlock();
  num_written = server.write(buffer, strlen(buffer));
  locker.relock();
  if (num_written != (int) strlen(buffer)) {
    return false;
  }
  return true;
}
