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

// C, C++ includes
#include <stdio.h>
#include <stdexcept>

// QT includes
#include <QRegExp>

// GoldenCheetah includes
#include "ProtocolHandler.h"

//////// Main parser; does bulk of parsing in constructor of subclasses.
QSharedPointer<ProtocolMessage> ProtocolHandler::parseLine(QString line) {
  try {
    if (line.startsWith("hello ") == true) {
      QSharedPointer<ProtocolMessage> hm(new HelloMessage(line));
      return hm;
    } else if (line.startsWith("hellofail ") == true) {
      QSharedPointer<ProtocolMessage> hfm(new HelloFailMessage(line));
      return hfm;
    } else if (line.startsWith("hellosucceed ") == true) {
      QSharedPointer<ProtocolMessage> hsm(new HelloSucceedMessage(line));
      return hsm;
    } else if (line.startsWith("clientlist ") == true) {
      QSharedPointer<ProtocolMessage> clm(new ClientListMessage(line));
      return clm;
    } else if (line.startsWith("client ") == true) {
      QSharedPointer<ProtocolMessage> cm(new ClientMessage(line));
      return cm;
    } else if (line.startsWith("telemetry ") == true) {
      QSharedPointer<ProtocolMessage> tm(new TelemetryMessage(line));
      return tm;
    } else if (line.startsWith("standings ") == true) {
      QSharedPointer<ProtocolMessage> sm(new StandingsMessage(line));
      return sm;
    } else if (line.startsWith("racer ") == true) {
      QSharedPointer<ProtocolMessage> rm(new RacerMessage(line));
      return rm;
    } else if (line.startsWith("raceconcluded ") == true) {
      QSharedPointer<ProtocolMessage> rcm(new RaceConcludedMessage(line));
      return rcm;
    } else if (line.startsWith("result ") == true) {
      QSharedPointer<ProtocolMessage> resm(new ResultMessage(line));
      return resm;
    } else if (line.startsWith("goodbye ") == true) {
      QSharedPointer<ProtocolMessage> gm(new GoodbyeMessage(line));
      return gm;
    }
  } catch (std::invalid_argument& ia) {
    // couldn't parse, so return an unknownmessage.  pass along
    // the error message embedded in the exception.
    QSharedPointer<ProtocolMessage> um(new UnknownMessage(ia.what()));
    return um;
  }

  // couldn't figure out what kind of message this is, so return
  // an UnknownMessage.
  QSharedPointer<ProtocolMessage> um(new UnknownMessage("unknown protocol message"));
  return um;
}


/////// HelloMessage methods
HelloMessage::HelloMessage(QString line) {
  static QRegExp regexp("hello\\s+(\\d+\\.\\d+)\\s+raceid='([0-9a-fA-F]+)'\\s+ridername='([a-zA-Z0-9 ]+)'\\s+ftp='([0-9]+)'\\s+weight='([0-9.]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse hello message");
  }
  this->message_type = ProtocolMessage::HELLO;
  this->protoversion = regexp.cap(1);
  this->raceid = regexp.cap(2).toLower();
  this->ridername = regexp.cap(3);
  this->ftp_watts = regexp.cap(4).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse hello message -- bogus ftp_watts");
  }
  this->weight_kg = regexp.cap(5).toFloat(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse hello message -- bogus weight_kg");
  }
}

HelloMessage::HelloMessage(QString protoversion, QString raceid, QString ridername,
                           int ftp_watts, float weight_kg) {
  this->message_type = ProtocolMessage::HELLO;
  this->protoversion = protoversion;
  this->raceid = raceid;
  this->ridername = ridername;
  this->ftp_watts = ftp_watts;
  this->weight_kg = weight_kg;
}

QString HelloMessage::toString() {
  QString retstr = QString("hello %1 raceid='%2' ridername='%3' ftp='%4' weight='%5'\n")
    .arg(this->protoversion).arg(this->raceid).arg(this->ridername)
    .arg(this->ftp_watts).arg(this->weight_kg);
  return retstr;

}

//////// HelloFail methods
HelloFailMessage::HelloFailMessage(QString line) {
  static QRegExp regexp("hellofail\\s+(\\d+\\.\\d+)\\s+(\\S+)\\s+raceid='([0-9a-fA-F]+)'");

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse hellofail message");
  }
  this->message_type = ProtocolMessage::HELLOFAIL;
  this->protoversion = regexp.cap(1);
  this->errmessage = regexp.cap(2);
  this->raceid = regexp.cap(3).toLower();
}

HelloFailMessage::HelloFailMessage(QString protoversion, QString errmessage,
                                   QString raceid) {
  this->message_type = ProtocolMessage::HELLOFAIL;
  this->protoversion = protoversion;
  this->errmessage = errmessage;
  this->raceid = raceid;
}

QString HelloFailMessage::toString() {
  QString retstr = QString("hellofail %1 %2 raceid='%3'\n")
    .arg(this->protoversion).arg(this->errmessage).arg(this->raceid);
  return retstr;
}

/////// HelloSucceedMessage methods
HelloSucceedMessage::HelloSucceedMessage(QString line) {
  static QRegExp regexp("hellosucceed\\s+(\\d+\\.\\d+)\\s+raceid='([0-9a-fA-F]+)'\\s+riderid='([0-9a-fA-F]+)'\\s+racedistance='([0-9.]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse hellosucceed message");
  }
  this->message_type = ProtocolMessage::HELLOSUCCEED;
  this->protoversion = regexp.cap(1);
  this->raceid = regexp.cap(2).toLower();
  this->riderid = regexp.cap(3).toLower();
  this->racedistance_km = regexp.cap(4).toFloat(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse hellosucceed message -- bogus racedistance_km");
  }
}

HelloSucceedMessage::HelloSucceedMessage(QString protoversion, QString raceid,
                                         QString riderid, float racedistance_km) {
  this->message_type = ProtocolMessage::HELLOSUCCEED;
  this->protoversion = protoversion;
  this->raceid = raceid;
  this->riderid = riderid;
  this->racedistance_km = racedistance_km;
}

QString HelloSucceedMessage::toString() {
  QString retstr = QString("hellosucceed %1 raceid='%2' riderid='%3' racedistance='%4'\n")
    .arg(this->protoversion).arg(this->raceid).arg(this->riderid)
    .arg(this->racedistance_km);
  return retstr;
}

/////// ClientListMessage methods
ClientListMessage::ClientListMessage(QString line) {
  static QRegExp regexp("clientlist\\s+raceid='([0-9a-fA-F]+)'\\s+numclients='([0-9]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse clientlist message");
  }
  this->message_type = ProtocolMessage::CLIENTLIST;
  this->raceid = regexp.cap(1).toLower();
  this->numclients = regexp.cap(2).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse clientlist message -- bogus numclients");
  }
}

ClientListMessage::ClientListMessage(QString raceid, int numclients) {
  this->message_type = ProtocolMessage::CLIENTLIST;
  this->raceid = raceid;
  this->numclients = numclients;
}

QString ClientListMessage::toString() {
  QString retstr = QString("clientlist raceid='%1' numclients='%2'\n")
    .arg(this->raceid).arg(this->numclients);
  return retstr;
}

/////// ClientMessage methods
ClientMessage::ClientMessage(QString line) {
  static QRegExp regexp("client\\s+ridername='([a-zA-Z0-9 ]+)'\\s+riderid='([0-9a-fA-F]+)'\\s+ftp='([0-9]+)'\\s+weight='([0-9.]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse client message");
  }
  this->message_type = ProtocolMessage::CLIENT;
  this->ridername = regexp.cap(1);
  this->riderid = regexp.cap(2).toLower();
  this->ftp_watts = regexp.cap(3).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse client message -- bogus ftp_watts");
  }
  this->weight_kg = regexp.cap(4).toFloat(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse client message -- bogus weight_kg");
  }
}

ClientMessage::ClientMessage(QString ridername, QString riderid,
                             int ftp_watts, float weight_kg) {
  this->message_type = ProtocolMessage::CLIENT;
  this->ridername = ridername;
  this->riderid = riderid;
  this->ftp_watts = ftp_watts;
  this->weight_kg = weight_kg;
}

QString ClientMessage::toString() {
  QString retstr = QString("client ridername='%1' riderid='%2' ftp='%3' weight='%4'\n")
    .arg(this->ridername).arg(this->riderid)
    .arg(this->ftp_watts).arg(this->weight_kg);
  return retstr;
}

/////// TelemetryMessage methods
TelemetryMessage::TelemetryMessage(QString line) {
  static QRegExp regexp("telemetry\\s+raceid='([0-9a-fA-F]+)'\\s+riderid='([0-9a-fA-F]+)'\\s+power='([0-9]+)'\\s+cadence='([0-9]+)'\\s+distance='([0-9.]+)'\\s+heartrate='([0-9]+)'\\s+speed='([0-9.]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse telemetry message");
  }
  this->message_type = ProtocolMessage::TELEMETRY;
  this->raceid = regexp.cap(1).toLower();
  this->riderid = regexp.cap(2).toLower();
  this->power_watts = regexp.cap(3).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse telemetry message -- bogus power_watts");
  }
  this->cadence_rpm = regexp.cap(4).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse telemetry message -- bogus cadence_rpm");
  }
  this->distance_km = regexp.cap(5).toFloat(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse telemetry message -- bogus distance_km");
  }
  this->heartrate_bpm = regexp.cap(6).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse telemetry message -- bogus heartrate_bpm");
  }
  this->speed_kph = regexp.cap(7).toFloat(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse telemetry message -- bogus speed_kph");
  }
}

TelemetryMessage::TelemetryMessage(QString raceid, QString riderid, int power_watts,
                                   int cadence_rpm, float distance_km, int heartrate_bpm,
                                   float speed_kph) {
  this->message_type = ProtocolMessage::TELEMETRY;
  this->raceid = raceid;
  this->riderid = riderid;
  this->power_watts = power_watts;
  this->cadence_rpm = cadence_rpm;
  this->distance_km = distance_km;
  this->heartrate_bpm = heartrate_bpm;
  this->speed_kph = speed_kph;
}

QString TelemetryMessage::toString() {
  QString retstr =
    QString("telemetry raceid='%1' riderid='%2' power='%3' cadence='%4' distance='%5' heartrate='%6' speed='%7'\n")
    .arg(this->raceid).arg(this->riderid).arg(this->power_watts)
    .arg(this->cadence_rpm).arg(this->distance_km).arg(this->heartrate_bpm)
    .arg(this->speed_kph);
  return retstr;
}

/////// StandingsMessage methods
StandingsMessage::StandingsMessage(QString line) {
  static QRegExp regexp("standings\\s+raceid='([0-9a-fA-F]+)'\\s+numclients='([0-9]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse standings message");
  }
  this->message_type = ProtocolMessage::STANDINGS;
  this->raceid = regexp.cap(1).toLower();
  this->numclients = regexp.cap(2).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse standings message -- bogus numclients");
  }
}

StandingsMessage::StandingsMessage(QString raceid, int numclients) {
  this->message_type = ProtocolMessage::STANDINGS;
  this->raceid = raceid;
  this->numclients = numclients;
}

QString StandingsMessage::toString() {
  QString retstr = QString("standings raceid='%1' numclients='%2'\n")
    .arg(this->raceid).arg(this->numclients);
  return retstr;
}

/////// TelemetryMessage methods
RacerMessage::RacerMessage(QString line) {
  static QRegExp regexp("racer\\s+riderid='([0-9a-fA-F]+)'\\s+power='([0-9]+)'\\s+cadence='([0-9]+)'\\s+distance='([0-9.]+)'\\s+heartrate='([0-9]+)'\\s+speed='([0-9.]+)'\\s+place='([0-9]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse racer message");
  }
  this->message_type = ProtocolMessage::RACER;
  this->riderid = regexp.cap(1).toLower();
  this->power_watts = regexp.cap(2).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse racer message -- bogus power_watts");
  }
  this->cadence_rpm = regexp.cap(3).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse racer message -- bogus cadence_rpm");
  }
  this->distance_km = regexp.cap(4).toFloat(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse racer message -- bogus distance_km");
  }
  this->heartrate_bpm = regexp.cap(5).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse racer message -- bogus heartrate_bpm");
  }
  this->speed_kph = regexp.cap(6).toFloat(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse racer message -- bogus speed_kph");
  }
  this->place = regexp.cap(7).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse racer message -- bogus place");
  }
}

RacerMessage::RacerMessage(QString riderid, int power_watts,
                           int cadence_rpm, float distance_km, int heartrate_bpm,
                           float speed_kph, int place) {
  this->message_type = ProtocolMessage::RACER;
  this->riderid = riderid;
  this->power_watts = power_watts;
  this->cadence_rpm = cadence_rpm;
  this->distance_km = distance_km;
  this->heartrate_bpm = heartrate_bpm;
  this->speed_kph = speed_kph;
  this->place = place;
}

QString RacerMessage::toString() {
  QString retstr =
    QString("racer riderid='%1' power='%2' cadence='%3' distance='%4' heartrate='%5' speed='%6' place='%7'\n")
    .arg(this->riderid).arg(this->power_watts)
    .arg(this->cadence_rpm).arg(this->distance_km).arg(this->heartrate_bpm)
    .arg(this->speed_kph).arg(this->place);
  return retstr;
}

/////// RaceConcludedMessage methods
RaceConcludedMessage::RaceConcludedMessage(QString line) {
  static QRegExp regexp("raceconcluded\\s+raceid='([0-9a-fA-F]+)'\\s+numclients='([0-9]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse raceconcluded message");
  }
  this->message_type = ProtocolMessage::RACECONCLUDED;
  this->raceid = regexp.cap(1).toLower();
  this->numclients = regexp.cap(2).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse raceconcluded message -- bogus numclients");
  }
}

RaceConcludedMessage::RaceConcludedMessage(QString raceid, int numclients) {
  this->message_type = ProtocolMessage::RACECONCLUDED;
  this->raceid = raceid;
  this->numclients = numclients;
}

QString RaceConcludedMessage::toString() {
  QString retstr = QString("raceconcluded raceid='%1' numclients='%2'\n")
    .arg(this->raceid).arg(this->numclients);
  return retstr;
}

/////// ResultMessage methods
ResultMessage::ResultMessage(QString line) {
  static QRegExp regexp("result\\s+riderid='([0-9a-fA-F]+)'\\s+distance='([0-9.]+)'\\s+place='([0-9]+)'");
  bool ok;

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse result message");
  }
  this->message_type = ProtocolMessage::RESULT;
  this->riderid = regexp.cap(1).toLower();
  this->distance_km = regexp.cap(2).toFloat(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse result message -- bogus distance");
  }
  this->place = regexp.cap(3).toInt(&ok);
  if (!ok) {
    throw std::invalid_argument("couldn't parse result message -- bogus place");
  }
}

ResultMessage::ResultMessage(QString riderid, float distance_km, int place) {
  this->message_type = ProtocolMessage::RESULT;
  this->riderid = riderid;
  this->distance_km = distance_km;
  this->place = place;
}

QString ResultMessage::toString() {
  QString retstr = QString("result riderid='%1' distance='%2' place='%3'\n")
    .arg(this->riderid).arg(this->distance_km).arg(this->place);
  return retstr;
}

//////// Goodbye methods
GoodbyeMessage::GoodbyeMessage(QString line) {
  static QRegExp regexp("goodbye\\s+raceid='([0-9a-fA-F]+)'\\s+riderid='([0-9a-fA-F]+)'");

  if (regexp.indexIn(line) != 0) {
    throw std::invalid_argument("couldn't parse goodbye message");
  }
  this->message_type = ProtocolMessage::GOODBYE;
  this->raceid = regexp.cap(1).toLower();
  this->riderid = regexp.cap(2).toLower();
}

GoodbyeMessage::GoodbyeMessage(QString raceid, QString riderid) {
  this->message_type = ProtocolMessage::GOODBYE;
  this->raceid = raceid;
  this->riderid = riderid;
}

QString GoodbyeMessage::toString() {
  QString retstr = QString("goodbye raceid='%1' riderid='%2'\n")
    .arg(this->raceid).arg(this->riderid);
  return retstr;
}


////////////// Test code; assumes you've compiled so stdout appears at a terminal.
void ProtocolHandler::test() {
  // Test "Unknown" message type
  QSharedPointer<ProtocolMessage> msg = ProtocolHandler::parseLine("blah");
  if (msg->message_type == ProtocolMessage::UNKNOWN) {
    QSharedPointer<UnknownMessage> um =
      qSharedPointerDynamicCast<UnknownMessage>(msg);
    printf("UNKNOWN -- %s\n", um->toString().toAscii().constData());
  } else {
    printf("!! expected UNKNOWN, but didn't get it...\n");
  }

  // Test broken "Hello" message
  QSharedPointer<ProtocolMessage> msg2 = ProtocolHandler::parseLine("hello there\n");
  if (msg2->message_type == ProtocolMessage::UNKNOWN) {
    QSharedPointer<UnknownMessage> um =
      qSharedPointerDynamicCast<UnknownMessage>(msg2);
    printf("broken hello --> UNKNOWN -- %s", um->toString().toAscii().constData());
  } else {
    printf("!! expected broken hello --> UNKNOWN, but didn't get it...\n");
  }

  // Test valid "Hello" message
  QSharedPointer<ProtocolMessage> msg3 = ProtocolHandler::parseLine(
     "hello 0.1 raceid='18d1a1bcd104ee116a772310bbc61211' ridername='Steve G' ftp='213' weight='74.8'\n"
                                                                       );
  if (msg3->message_type == ProtocolMessage::HELLO) {
    QSharedPointer<HelloMessage> hm =
      qSharedPointerDynamicCast<HelloMessage>(msg3);
    printf("HELLO:  %s", hm->toString().toAscii().constData());
  } else {
    printf("!! expected hello, but didn't get it...\n");
  }
  HelloMessage hm2("0.1", "deadbeef01234567cafebabe76543210", "Steve G",
                   213, 74.8);
  printf("cons'ed up hellomessage: %s", hm2.toString().toAscii().constData());

  // Test valid HelloFail message
  QSharedPointer<ProtocolMessage> msg4 = ProtocolHandler::parseLine(
     "hellofail 0.1 nosuchrace raceid='18d1a1bcd104ee116a772310bbc61211'\n"
                                                                       );
  if (msg4->message_type == ProtocolMessage::HELLOFAIL) {
    QSharedPointer<HelloFailMessage> hfm =
      qSharedPointerDynamicCast<HelloFailMessage>(msg4);
    printf("HELLOFAIL:  %s", hfm->toString().toAscii().constData());
  } else {
    printf("!! expected hellofail, but didn't get it...\n");
  }

  // Test valid HelloSucceed message
  QSharedPointer<ProtocolMessage> msg5 = ProtocolHandler::parseLine(
     "hellosucceed 0.1 raceid='18d1a1bcd104ee116a772310bbc61211' riderid='123212321232123a' racedistance='180.0'\n"
                                                                       );
  if (msg5->message_type == ProtocolMessage::HELLOSUCCEED) {
    QSharedPointer<HelloSucceedMessage> hsm =
      qSharedPointerDynamicCast<HelloSucceedMessage>(msg5);
    printf("HELLOSUCCEED:  %s", hsm->toString().toAscii().constData());
  } else {
    printf("!! expected hellosucceed, but didn't get it...\n");
  }

  // Test valid ClientList message
  QSharedPointer<ProtocolMessage> msg6 = ProtocolHandler::parseLine(
     "clientlist raceid='18d1a1bcd104ee116a772310bbc61211' numclients='5'\n"
                                                                       );
  if (msg6->message_type == ProtocolMessage::CLIENTLIST) {
    QSharedPointer<ClientListMessage> clm =
      qSharedPointerDynamicCast<ClientListMessage>(msg6);
    printf("CLIENTLIST:  %s", clm->toString().toAscii().constData());
  } else {
    printf("!! expected clientlist, but didn't get it...\n");
  }

  // Test valid Client message
  QSharedPointer<ProtocolMessage> msg7 = ProtocolHandler::parseLine(
     "client ridername='Steve G' riderid='123212321232123a' ftp='213' weight='75.8'"
                                                                       );
  if (msg7->message_type == ProtocolMessage::CLIENT) {
    QSharedPointer<ClientMessage> cm =
      qSharedPointerDynamicCast<ClientMessage>(msg7);
    printf("CLIENT:  %s", cm->toString().toAscii().constData());
  } else {
    printf("!! expected client, but didn't get it...\n");
  }

  // Test valid Telemetry message
  QSharedPointer<ProtocolMessage> msg8 = ProtocolHandler::parseLine(
     "telemetry raceid='18d1a1bcd104ee116a772310bbc61211' riderid='123212321232123a' power='250' cadence='85' distance='5.41' heartrate='155' speed='31.5'\n"
                                                                       );
  if (msg8->message_type == ProtocolMessage::TELEMETRY) {
    QSharedPointer<TelemetryMessage> tm =
      qSharedPointerDynamicCast<TelemetryMessage>(msg8);
    printf("TELEMETRY:  %s", tm->toString().toAscii().constData());
  } else {
    printf("!! expected telemetry, but didn't get it...\n");
  }

  // Test valid Standings message
  QSharedPointer<ProtocolMessage> msg9 = ProtocolHandler::parseLine(
     "standings raceid='18d1a1bcd104ee116a772310bbc61211' numclients='5'\n"
                                                                       );
  if (msg9->message_type == ProtocolMessage::STANDINGS) {
    QSharedPointer<StandingsMessage> sm =
      qSharedPointerDynamicCast<StandingsMessage>(msg9);
    printf("STANDINGS:  %s", sm->toString().toAscii().constData());
  } else {
    printf("!! expected standings, but didn't get it...\n");
  }

  // Test valid Racer message
  QSharedPointer<ProtocolMessage> msg10 = ProtocolHandler::parseLine(
     "racer riderid='123212321232123a' power='250' cadence='85' distance='5.41' heartrate='155' speed='31.5' place='1'\n"
                                                                       );
  if (msg10->message_type == ProtocolMessage::RACER) {
    QSharedPointer<RacerMessage> rm =
      qSharedPointerDynamicCast<RacerMessage>(msg10);
    printf("RACER:  %s", rm->toString().toAscii().constData());
  } else {
    printf("!! expected racer, but didn't get it...\n");
  }

  // Test valid RaceConcluded message
  QSharedPointer<ProtocolMessage> msg11 = ProtocolHandler::parseLine(
     "raceconcluded raceid='18d1a1bcd104ee116a772310bbc61211' numclients='5'\n"
                                                                       );
  if (msg11->message_type == ProtocolMessage::RACECONCLUDED) {
    QSharedPointer<RaceConcludedMessage> rcm =
      qSharedPointerDynamicCast<RaceConcludedMessage>(msg11);
    printf("RACECONCLUDED:  %s", rcm->toString().toAscii().constData());
  } else {
    printf("!! expected raceconclded, but didn't get it...\n");
  }

  // Test valid Result message
  QSharedPointer<ProtocolMessage> msg12 = ProtocolHandler::parseLine(
     "result riderid='123212321232123a' distance='5.41' place='1'\n"
                                                                       );
  if (msg12->message_type == ProtocolMessage::RESULT) {
    QSharedPointer<ResultMessage> resm =
      qSharedPointerDynamicCast<ResultMessage>(msg12);
    printf("RESULT:  %s", resm->toString().toAscii().constData());
  } else {
    printf("!! expected result, but didn't get it...\n");
  }

  // Test valid Goodbye message
  QSharedPointer<ProtocolMessage> msg13 = ProtocolHandler::parseLine(
     "goodbye raceid='18d1a1bcd104ee116a772310bbc61211' riderid='123212321232123a'\n"
                                                                       );
  if (msg13->message_type == ProtocolMessage::GOODBYE) {
    QSharedPointer<GoodbyeMessage> gbm =
      qSharedPointerDynamicCast<GoodbyeMessage>(msg13);
    printf("GOODBYE:  %s", gbm->toString().toAscii().constData());
  } else {
    printf("!! expected goodbye, but didn't get it...\n");
  }

}
