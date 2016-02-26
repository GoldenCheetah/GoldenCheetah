/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#ifndef CLIENT_H
#define CLIENT_H
#include "GoldenCheetah.h"

#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include <QObject>
#include <QStringList>
#include <QTime>
#include <QProgressDialog>
#include "RealtimeData.h"
#include "DeviceConfiguration.h"

class QuarqdClient : public QThread
{

   Q_OBJECT
   G_OBJECT


public:
    QuarqdClient(QObject *parent = 0, DeviceConfiguration *dc=0);
    ~QuarqdClient();

    int start();                                // Calls QThread to start
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    int quit(int error);                        // called by thread before exiting
    bool discover(DeviceConfiguration *, QProgressDialog *);              // confirm Server available at portSpec

    RealtimeData getRealtimeData();             // return current realtime data
    // SET
    void setDevice(DeviceConfiguration);       // setup the device filename
    QString getAntID();

private:
    QTcpSocket *tcpSocket;                      // IMPORTANT MUST BE CREATED IN THREAD
    QMutex pvars;  // lock/unlock access to telemetry data between thread and controller
    int Status;     // what status is the client in?

    void run();

    int openPort(), closePort(); // open and close socket

    void parseElement(QString &str); // reads input string and updates current telemetry values
    long getTimeStamp(QString &str);
    bool parsePortSpec(char *, char &, int &); // parse a port spec from string to ip, portnum

    // Current (Last read) Realtime Data
    RealtimeData telemetry;

    // server hostname and TCP port#
    QString deviceHostname;
    int devicePort;
    QStringList antIDs;
    long lastReadWatts;
    long lastReadCadence;
    long lastReadWheelRpm;
    long lastReadSpeed;
    QTime elapsedTime;
    bool sentDual, sentSpeed, sentHR, sentCad, sentPWR;
    void reinitChannel(QString _channel);

private slots:
    void initChannel();

};

#endif
