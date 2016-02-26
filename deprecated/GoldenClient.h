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

#ifndef _GC_GoldenClient_h
#define _GC_GoldenClient_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>
#include <QMap>
#include <QQueue>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "ProtocolHandler.h"

typedef struct {
  QString rider_name;
  QString rider_id;
  int     ftp_watts;
  float   weight_kg;
  int     power_watts;
  int     cadence_rpm;
  float   distance_km;
  int     heartrate_bpm;
  float   speed_kph;
  int     place;
} RiderData;

typedef struct {
  QString race_id;
  float   race_distance_km;
  bool    race_finished;
  bool    membership_changed;
  QMap<QString,RiderData> riders_status;
} RaceStatus;

// This class manages the binding of the GoldenCheetah client with
// a GoldenServer.  When connected, an instance of this class spawns
// a thread to manage reading and writing data from/to the server.
class GoldenClient : public QThread {
 public:
 GoldenClient() : running(false), connected(false), kill_signal(false) { }
  ~GoldenClient() { }

  // Forge the connection to the GoldenServer and perform the initial
  // handshake.  If the handshake works, set up the cache of rider
  // performance (initially empty).  Returns true on success, in which
  // case a thread has been spawned to manage the connection.  Returns
  // false on failure.
  //
  // raceid is the raceid to connect to, ridername is the name of
  // the local rider.  the rest should be self-explanatory. :)
  bool connect(QString server_hostname, quint16 server_port,
               QString raceid, QString ridername, int ftp_watts,
               float weight_kg);

  // Sever the connection with the server and kill off the thread that
  // was spawned to manage it.  Note that this MUST be called before
  // the instance is destroyed, or the thread will be orphaned.
  void closeAndExit();

  // Send a telemetry update to the server.
  //
  // Note that we haven't yet figured out which telemetry data the
  // server should use when calculating race updates.  For example,
  // one possibility is that the server just uses watts to calculate
  // each rider's speed normalized to FTP, as a way of having a handicapped
  // race.  As another example, the server could use reported speed
  // or reported distance.  [For now, the server just uses the
  // reported speed of the client, and ignores watts and distance.]
  void sendTelemetry(int power_watts, int cadence_rpm,
                     float distance_km, int heartrate_bpm,
                     float speed_kph);

  // Get a copy of the current race standings.  Note that
  // race_finished is set to true if the race is over (for any
  // reason).  Also note that membership_changed is set to true if the
  // race membership has been updated since the last time anybody
  // invoked getStandings().
  RaceStatus getStandings();

  // The client's particulars, exchanged with the server on session
  // establishment. [Is just a cached copy of what's passed to me in
  // connect().]
  QString remote_host;
  quint16 remote_port;
  QString rider_raceid;
  QString rider_name;
  int     rider_ftp_watts;
  float   rider_weight_kg;

  // The race metadata, returned during the server handshake.
  QString rider_id;
  float   race_distance_km;

 private:
  // For coordinating between the caller and the embedded thread.
  QMutex client_lock;
  QWaitCondition client_cond;

  bool running;       // thread running? controlled by the child thread
  bool connected;     // socket connected? controlled by the child thread
  bool kill_signal;   // controlled by caller via closeAndExit()

  // The current race status/standings
  RaceStatus race_status;

  // The client can enqueue a message to be sent to the server
  // on here by calling sendTelemetry().
  QQueue<QSharedPointer<ProtocolMessage> > write_queue;

  // connect() will spawn a child thread when it successfully
  // connects to a server.  The child thread will enter this
  // private method.
  void run();

  // read a line of text from the socket; returns true on success,
  // and includes the newline at the end of the string.  since
  // a read might block, we'll pass in a locker to release.
  // "block" says whether to loop forever waiting for data.
  bool read_line(QMutexLocker &locker, QTcpSocket &server,
                 bool block, QString &read_into_me);

  // write a line of text to the socket; returns true on success,
  // false if the write failed.  since a write might block, we'll
  // pass in a locker to release.
  bool write_line(QMutexLocker &locker, QTcpSocket &server,
                  QString line_to_write);

  // handle various messages from the server.  returns false if things
  // go wrong, in which case caller should bork out.
  bool handle_message(QMutexLocker &locker, QTcpSocket &server,
                      QSharedPointer<ProtocolMessage> msg);
  bool handle_clientlist(QMutexLocker &locker, QTcpSocket &server,
                         QSharedPointer<ClientListMessage> msg);
  bool handle_standings(QMutexLocker &locker, QTcpSocket &server,
                        QSharedPointer<StandingsMessage> msg);
  bool handle_raceconcluded(QMutexLocker &locker, QTcpSocket &server,
                            QSharedPointer<RaceConcludedMessage> msg);
};

#endif // _GC_GoldenClient_h
