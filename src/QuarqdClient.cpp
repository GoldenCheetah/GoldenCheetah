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

#include <QtNetwork>
#include <QMessageBox>
#include <QTime>
#include <QProgressDialog>
#include <QtDebug>
#include "QuarqdClient.h"
#include "RealtimeData.h"


// Strings as received from quarqd
// tested with a Garmin ANT+ stick
//
static QString powerStr       = "<Power ";
static QString cadenceStr     = "<Cadence ";
static QString speedStr       = "<Speed ";
static QString heartRateStr   = "<HeartRate ";

static QString sensorStaleStr = "<SensorStale ";
static QString sensorFoundStr = "<SensorFound ";
static QString sensorDropStr  = "<SensorDrop ";
static QString sensorLostStr  = "<SensorLost ";

/* Control status */
#define ANT_RUNNING  0x01
#define ANT_PAUSED   0x02

QuarqdClient::QuarqdClient(QObject *parent, DeviceConfiguration *devConf) : QThread(parent)
{
    Status=0;
    // server hostname and TCP port#
    deviceHostname = devConf->portSpec.section(':',0,0).toAscii(); // after the colon
    devicePort = (int)QString(devConf->portSpec).section(':',1,1).toInt(); // after the colon
    antIDs = devConf->deviceProfile.split(",");
    lastReadWatts = 0;
    lastReadCadence = 0;
    lastReadSpeed = 0;
}

QuarqdClient::~QuarqdClient()
{
}

void QuarqdClient::run()
{
    qDebug() << "Starting thread...";
    int status; // control commands from controller
    bool isPortOpen = false;

    Status = ANT_RUNNING;
    QString strBuf;

    openPort();
    isPortOpen = true;
    elapsedTime.start();

    while(1)
    {
        if(tcpSocket->isValid())
        {
            if(!tcpSocket->waitForReadyRead(-1))
            {
                return;
            }
           QByteArray array = tcpSocket->readAll();
           strBuf = array;
           parseElement(strBuf); // updates local telemetry
        }


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

void
QuarqdClient::parseElement(QString &strBuf) // updates QuarqdClient::telemetry
{
    QStringList qList = strBuf.split("\n");

    // qDebug("%s",strBuf.toAscii().data());

    //Loop for all the elements.
    for(int i=0; i<qList.size(); i++)
    {
        QString str = qList.at(i);
        bool ok;
        double value;
        int start, end;
        QString mid;

        if(str.startsWith(powerStr))
        {
            start = str.indexOf("watts='");
            start += 7;
            end = str.indexOf("'", start);

            mid = str.mid(start, end - start);
            value = mid.toDouble(&ok);

            if (ok && mid != "nan" && mid != "inf") {
                telemetry.setWatts(value);
                telemetry.setTime(getTimeStamp(str));
                lastReadWatts = elapsedTime.elapsed();
            }
        } else if(str.startsWith(cadenceStr))
        {
            start = str.indexOf("RPM='");
            start += 5;
            end = str.indexOf("'", start);

            mid = str.mid(start, end - start);
            value = mid.toDouble(&ok);

            if (ok && mid != "nan" && mid != "inf") {
                telemetry.setCadence(value);
                telemetry.setTime(getTimeStamp(str));
                lastReadCadence = elapsedTime.elapsed();
            }
        } else if(str.startsWith(speedStr))
        {
            // This is not the speed it is the revolution of the wheel
            // at least when it comes from PowerTap.
            start = str.indexOf("RPM='");
            start += 5;
            end = str.indexOf("'", start);

            mid = str.mid(start, end - start);
            value = mid.toDouble(&ok);

            if (ok && mid != "nan" && mid != "inf") {
                if (value > 0) {
                    // TODO: let wheel size be a configurable, default now to 2101 mm
                    telemetry.setSpeed((value*2101/1000*60)/1000); // meter/minute -> meter/hour -> km/hour
                    telemetry.setWheelRpm(value);
                    lastReadSpeed = elapsedTime.elapsed();
                }
                telemetry.setTime(getTimeStamp(str));
            }
        } else if(str.startsWith(heartRateStr))
        {
            start = str.indexOf("BPM");
            start += 5;
            end = str.indexOf("'", start);

            mid = str.mid(start, end - start);
            value = mid.toDouble(&ok);

            if (ok && mid != "nan" && mid != "inf") {
                telemetry.setHr(value);
                telemetry.setTime(getTimeStamp(str));
            }
        } else if(str.startsWith(sensorDropStr) || str.startsWith(sensorStaleStr) || str.startsWith(sensorLostStr))//Try and save
        {
            int start = str.indexOf("id");
            start += 4;
            int end = str.indexOf("'", start);
            reinitChannel(str.mid(start, end - start));
        }

        if(elapsedTime.elapsed() - lastReadCadence > 5000)
            telemetry.setCadence(0);

        if(elapsedTime.elapsed() - lastReadWatts > 5000)
            telemetry.setWatts(0);

        if(elapsedTime.elapsed() - lastReadSpeed > 5000)
            telemetry.setSpeed(0);

    }
}

long QuarqdClient::getTimeStamp(QString &str)
{
    //Get the timestamp
    int start = str.indexOf("timestamp='");
    start += 11;
    int end = str.indexOf(".", start);
    //    qDebug() << str.mid(start, end - start);
    return str.mid(start, end - start).toLong();
}

int
QuarqdClient::openPort()
{
    tcpSocket = new QTcpSocket();
    tcpSocket->connectToHost(deviceHostname, devicePort);
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(initChannel()));
    return 0;
}

int
QuarqdClient::closePort()
{
    tcpSocket->close();
    delete tcpSocket;
    return 0;
}

//
// All stuff below here is to support startup / shutdown commands and
// interaction with the ANTplusController class between the QuarqdClient
// and the GUI
//
int
QuarqdClient::start()
{
    QThread::start();
    return 0;
}

int
QuarqdClient::restart()
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
QuarqdClient::pause()
{
    int status;

    // get current status
    pvars.lock();
    status = this->Status;
    pvars.unlock();

    if (status&ANT_PAUSED) return 2; // already paused you muppet!
    else if (!(status&ANT_RUNNING)) return 4; // not running anyway, fool!
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
QuarqdClient::stop()
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
QuarqdClient::quit(int code)
{
    // event code goes here!
    closePort();
    exit(code);
    return 0; // never gets here obviously but shuts up the compiler!
}

bool
QuarqdClient::discover(DeviceConfiguration *config, QProgressDialog *progress)
{
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
}

RealtimeData
QuarqdClient::getRealtimeData()
{
    return telemetry;
}

void QuarqdClient::reinitChannel(QString _channel)
{
    if(!tcpSocket->isValid())
        return;
    qDebug() << "Reinit: " << _channel;

    QByteArray channel("X-set-channel: ");
    channel.append(_channel);
    tcpSocket->write(channel);
}


void
QuarqdClient::initChannel()
{

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
}
