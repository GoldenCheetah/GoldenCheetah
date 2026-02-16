/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net),
 *                    Mark Liversedge (liversedge@gmail.com),
 *               2015 Erik Botö (erik.boto@gmail.com)
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


#include "KettlerRacer.h"

KettlerRacer::KettlerRacer(QObject *parent,  QString devname) : QObject(parent),
    m_heartRate(0),
    m_power(0),
    m_cadence(0),
    m_isKettlerRacerConnectionAlive(true)
{
    m_kettlerRacerConnection = new KettlerRacerConnection;
    m_kettlerRacerConnection->setSerialPort(devname);
    if (!connect(m_kettlerRacerConnection, SIGNAL(power(quint32)), this, SLOT(newPower(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect power signal in KettlerRacer");
    }
    if (!connect(m_kettlerRacerConnection, SIGNAL(cadence(quint32)), this, SLOT(newCadence(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect cadence signal in KettlerRacer");
    }
    if (!connect(m_kettlerRacerConnection, SIGNAL(pulse(quint32)), this, SLOT(newHeartRate(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect pulse signal in KettlerRacer");
    }
    if (!connect(m_kettlerRacerConnection, SIGNAL(speed(double)), this, SLOT(newSpeed(double)), Qt::QueuedConnection)) {
        qFatal("Failed to connect speed signal in KettlerRacer");
    }
    if (!connect(m_kettlerRacerConnection, SIGNAL(finished()), this, SLOT(onKettlerRacerConnectionFinished()), Qt::QueuedConnection)) {
        qFatal("Failed to connect finished signal in KettlerRacer");
    }
}

KettlerRacer::~KettlerRacer(){
   m_kettlerRacerConnection->quit();
   m_kettlerRacerConnection->deleteSerialPort();
   delete m_kettlerRacerConnection;
}

int KettlerRacer::start()
{
    m_kettlerRacerConnection->start();
    return 0;
}

int KettlerRacer::restart()
{
    return 0;
}

int KettlerRacer::stop()
{
    m_kettlerRacerConnection->setLoad(25);
    return 0;
}

void KettlerRacer::newCadence(quint32 cadence)
{
    m_cadence = cadence;
}

void KettlerRacer::newHeartRate(quint32 heartRate)
{
    m_heartRate = heartRate;
}

void KettlerRacer::newPower(quint32 power)
{
    m_power = power;
}

void KettlerRacer::newSpeed(double speed)
{
    m_speed = speed;
}

/**
 * This functions takes a serial port and tries if it can find a Kettler bike connected
 * to it.
 */
bool KettlerRacer::discover(QString portName)
{
    bool found = false;
    QSerialPort sp;
    QLabel *label = new QLabel();
    label->setWordWrap(true);
    label->show();
    label->setText(label->text() +"---"+ portName);
    label->setFixedSize(300,100);
    sp.setPortName(portName);

    if (sp.open(QSerialPort::ReadWrite))
    {
        m_kettlerRacerConnection->configurePort(&sp);
        // Discard any existing data
        QByteArray reply = sp.readAll();

        for(int i=0; i<4; i++)
        {

            // Read id from bike
            reply.clear();
            if (sp.write("KI\r\n") == -1) {
                // handle write error? In discovery loop, maybe just continue or log?
                qWarning("Failed to write to serial port during discovery in KettlerRacer");
            }
            if (sp.waitForBytesWritten(500))
            {}
            for(int i=0; i<15; i++)
            {
                if (sp.waitForReadyRead(500))
                {
                    reply.append(sp.readAll());
                    if(reply.contains("\r\n"))
                    {
                        found=true;
                        break;
                    }
                }
            }
            if(found)
            {
                break;
            } else {
                switch (i) {
                case 0:
                    sp.setBaudRate(QSerialPort::Baud9600);
                    break;
                case 1:
                    sp.setBaudRate(QSerialPort::Baud19200);
                    break;
                case 2:
                    sp.setBaudRate(QSerialPort::Baud38400);
                    break;
                case 3:
                    sp.setBaudRate(QSerialPort::Baud115200);
                    break;
                default:
                    break;
                }
            }
        }
        //found=false;
        reply.append('\0');
        QString replyString(reply);
        qDebug() << "find" << reply;
        label->setText(label->text() +"---"+ replyString);
        if (replyString.contains("RACER"))
        {
            found = true;
        }
    }

    sp.close();

    return found;
}


void KettlerRacer::setLoad(double load)
{
    m_kettlerRacerConnection->setLoad((unsigned int)load);
}

bool KettlerRacer::isConnected()
{
    return m_isKettlerRacerConnectionAlive;
}

void KettlerRacer::onKettlerRacerConnectionFinished()
{
    m_isKettlerRacerConnectionAlive = false;
}
