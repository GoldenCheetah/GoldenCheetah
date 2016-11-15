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


#ifndef _GC_SimpleNetworkClient_h
#define _GC_SimpleNetworkClient_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>
#include <QQueue>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "RealtimeController.h"
#include "RealtimeData.h"

// This class is used by SimpleNetworkController to actually
// connect to the remote server, and do the reads/writes and
// marshalling/unmarshalling of data over the network.
// The class spawns a thread to do reads, and steals the
// caller's thread to do writes.

class SimpleNetworkClient : public QThread
{
 public:

  QObject *parent;

  // hostname and port are the hostname/port of the server to which
  // this SimpleNetworkClient should connect.
  SimpleNetworkClient(QObject *parent, DeviceConfiguration *dc);
  ~SimpleNetworkClient();

  // Forge the connection to the remote host, and start the
  // thread running.  Returns false on failure (in which case
  // the thread will not be running).
  bool start();

  // If the TCP connection is open, closes it.  If the thread
  // is running, terminates it and waits for it to exit.
  void closeAndExit();

  // When called, this method will fill in rtData with the latest
  // realtime data from the remote peer.
  //
  // Returns true on success, false if the remote peer has been
  // disconnected.
  bool getRealtimeData(RealtimeData &rtData);

  // When called, this method will push the realtime data in
  // rtData to the server.
  //
  // Returns true on success, false if the remote peer has been
  // disconnected.
  bool pushRealtimeData(RealtimeData &rtData);

 private:
  // When SimpleNetworkClient.start() is called, the new thread
  // will begin executing here.
  void run();

  // The thread uses this to pull from the network.
  bool read_next_line(QMutexLocker &locker,
                      QTcpSocket &server,
                      RealtimeData &read_into_me,
                      char *next_line);

  // The thread uses this to push into the network.
  bool write_next_line(QMutexLocker &locker,
                       QTcpSocket &server,
                       RealtimeData record,
                       char *buffer);

  // Hostname and port number we connect to.
  QString server_hostname;
  quint16 server_port;

  // For coordinating between GUI and the thread.
  QMutex client_lock;
  QWaitCondition client_cond;
  bool running;     // controlled by child thread
  bool connected;   // controlled by child thread
  bool kill_signal; // controlled by caller; child thread will signal

  // The latest data read from the network.
  RealtimeData read_data_cache;

  // A Queue of data to write to the network.
  QQueue<RealtimeData> write_queue;
};

#endif // _GC_SimpleNetworkClient_h
