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


#include "Kettler.h"

Kettler::Kettler(QObject *parent,  QString devname) : QObject(parent),
    m_heartRate(0),
    m_power(0),
    m_cadence(0),
    m_isKettlerConnectionAlive(true)
{
    m_kettlerConnection.setSerialPort(devname);
    if (!connect(&m_kettlerConnection, SIGNAL(power(quint32)), this, SLOT(newPower(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect power signal in Kettler");
    }
    if (!connect(&m_kettlerConnection, SIGNAL(cadence(quint32)), this, SLOT(newCadence(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect cadence signal in Kettler");
    }
    if (!connect(&m_kettlerConnection, SIGNAL(pulse(quint32)), this, SLOT(newHeartRate(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect pulse signal in Kettler");
    }
    if (!connect(&m_kettlerConnection, SIGNAL(speed(double)), this, SLOT(newSpeed(double)), Qt::QueuedConnection)) {
        qFatal("Failed to connect speed signal in Kettler");
    }
    if (!connect(&m_kettlerConnection, SIGNAL(finished()), this, SLOT(onKettlerConnectionFinished()), Qt::QueuedConnection)) {
        qFatal("Failed to connect finished signal in Kettler");
    }
}

Kettler::~Kettler()
{
}

int Kettler::start()
{
    m_kettlerConnection.start();
    return 0;
}

int Kettler::restart()
{
    return 0;
}

int Kettler::stop()
{
    return 0;
}

void Kettler::newCadence(quint32 cadence)
{
    m_cadence = cadence;
}

void Kettler::newHeartRate(quint32 heartRate)
{
    m_heartRate = heartRate;
}

void Kettler::newPower(quint32 power)
{
    m_power = power;
}

void Kettler::newSpeed(double speed)
{
    m_speed = speed;
}

/**
 * This functions takes a serial port and tries if it can find a Kettler bike connected
 * to it.
 */
bool Kettler::discover(QString portName)
{
    bool found = false;
    QSerialPort sp;

    sp.setPortName(portName);

    if (sp.open(QSerialPort::ReadWrite))
    {
        m_kettlerConnection.configurePort(&sp);

        // Discard any existing data
        QByteArray data = sp.readAll();

        // Read id from bike
        if (sp.write("ID\r\n") == -1) {
             qWarning("Failed to write to serial port during discovery in Kettler");
        }
        sp.waitForBytesWritten(500);

        QByteArray reply = sp.readAll();

        reply.append('\0');

        QString replyString(reply);
        if (replyString.startsWith("ACK") || replyString.startsWith("RUN"))
        {
            found = true;
        }
    }

    sp.close();

    return found;
}


void Kettler::setLoad(double load)
{
    m_kettlerConnection.setLoad((unsigned int)load);
}

bool Kettler::isConnected()
{
    return m_isKettlerConnectionAlive;
}

void Kettler::onKettlerConnectionFinished()
{
    m_isKettlerConnectionAlive = false;
}
