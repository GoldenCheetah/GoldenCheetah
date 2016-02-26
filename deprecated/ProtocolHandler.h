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

#ifndef _GC_ProtocolHandler_h
#define _GC_ProtocolHandler_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>

// This abstract base class represents a protocol message. There is
// one subclass per message type.
class ProtocolMessage {
 public:
  ProtocolMessage() { }
  virtual ~ProtocolMessage() { }

  enum {
    HELLO, HELLOFAIL, HELLOSUCCEED, CLIENTLIST, CLIENT, TELEMETRY,
    STANDINGS, RACER, RACECONCLUDED, RESULT, GOODBYE, UNKNOWN
  } message_type;

  virtual QString toString() = 0;
};

// This class is used to handle GC/GS protocol messages.  It parses
// protocol messages into nicely defined class instances, and produces
// protocol messages from instances.  In other words, this class does
// message marshaling and unmarshaling.

class ProtocolHandler {
 public:
  ProtocolHandler() { }
  ~ProtocolHandler() { }

  static QSharedPointer<ProtocolMessage> parseLine(QString line);
  static void test();
};

/*
 * This class of message represents a parse failure, i.e.,
 * the message type couldn't be determined.
 */
class UnknownMessage : public ProtocolMessage {
 public:
  UnknownMessage(QString msg) {
    error_message = msg;
    message_type = UNKNOWN;
  }

  QString toString() {
    QString retval = "parse error (\"" + error_message + "\")";
    return retval;
  }

  QString error_message;
};

/*
 * A HelloMessage is sent from a client to a server upon connection.
 */
class HelloMessage : public ProtocolMessage {
 public:
  HelloMessage(QString msg);
  HelloMessage(QString protoversion, QString raceid, QString ridername,
               int ftp_watts, float weight_kg);
  QString toString();

  QString protoversion;
  QString raceid;
  QString ridername;
  int ftp_watts;
  float weight_kg;
};

/*
 * A HelloFail is sent by the server to the client when
 * things go wrong on handshake.
 */
class HelloFailMessage : public ProtocolMessage {
 public:
  HelloFailMessage(QString msg);
  HelloFailMessage(QString protoversion, QString errmessage, QString raceid);
  QString toString();

  QString protoversion;
  QString errmessage;
  QString raceid;
};

/*
 * A HelloSucceedMessage is sent from the server to the client
 * when things go well on handshake.
 */
class HelloSucceedMessage : public ProtocolMessage {
 public:
  HelloSucceedMessage(QString msg);
  HelloSucceedMessage(QString protoversion, QString raceid, QString riderid,
                      float racedistance_km);
  QString toString();

  QString protoversion;
  QString raceid;
  QString riderid;
  float racedistance_km;
};

/*
 * A ClientList is sent from the server to the client
 * to inform it of membership updates.
 */
class ClientListMessage : public ProtocolMessage {
 public:
  ClientListMessage(QString msg);
  ClientListMessage(QString raceid, int numclients);
  QString toString();

  QString raceid;
  int numclients;
};

/*
 * A Client message is sent from the server to the client
 * as part of a ClientList membership update
 */
class ClientMessage : public ProtocolMessage {
 public:
  ClientMessage(QString msg);
  ClientMessage(QString ridername, QString riderid,
                int ftp_watts, float weight_kg);
  QString toString();

  QString ridername;
  QString riderid;
  int ftp_watts;
  float weight_kg;
};

/*
 * A Telemetry message is sent from the client to the server
 * to give it a telemetry update.
 */
class TelemetryMessage : public ProtocolMessage {
 public:
  TelemetryMessage(QString msg);
  TelemetryMessage(QString raceid, QString riderid, int power_watts,
                   int cadence_rpm, float distance_km, int heartrate_bpm,
                   float speed_kph);
  QString toString();

  QString raceid;
  QString riderid;
  int power_watts;
  int cadence_rpm;
  float distance_km;
  int heartrate_bpm;
  float speed_kph;
};

/*
 * A Standings message is sent from the server to the client
 * to inform it of the current standings.
 */
class StandingsMessage : public ProtocolMessage {
 public:
  StandingsMessage(QString msg);
  StandingsMessage(QString raceid, int numclients);
  QString toString();

  QString raceid;
  int numclients;
};

/*
 * A Racer message is sent from the server to the client
 * as part of a standings update.
 */
class RacerMessage : public ProtocolMessage {
 public:
  RacerMessage(QString msg);
  RacerMessage(QString riderid, int power_watts,
               int cadence_rpm, float distance_km, int heartrate_bpm,
               float speed_kph, int place);
  QString toString();

  QString riderid;
  int power_watts;
  int cadence_rpm;
  float distance_km;
  int heartrate_bpm;
  float speed_kph;
  int place;
};

/*
 * A RaceConcluded message is sent from the server to the client
 * to inform that the race has finished and of the final standings.
 */
class RaceConcludedMessage : public ProtocolMessage {
 public:
  RaceConcludedMessage(QString msg);
  RaceConcludedMessage(QString raceid, int numclients);
  QString toString();

  QString raceid;
  int numclients;
};

/*
 * A Result message is sent from the server to the client
 * as part of a raceconcluded message.
 */
class ResultMessage : public ProtocolMessage {
 public:
  ResultMessage(QString msg);
  ResultMessage(QString riderid, float distance_km, int place);
  QString toString();

  QString riderid;
  float distance_km;
  int place;
};

/*
 * A Goodbye message is sent from the client to the server
 * to sign off cleanly.
 */
class GoodbyeMessage : public ProtocolMessage {
 public:
  GoodbyeMessage(QString msg);
  GoodbyeMessage(QString raceid, QString riderid);
  QString toString();

  QString raceid;
  QString riderid;
};

#endif // _GC_ProtocolHandler_h
