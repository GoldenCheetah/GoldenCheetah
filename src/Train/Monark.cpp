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


#include "Monark.h"

Monark::Monark(QObject *parent,  QString devname) : QObject(parent),
    m_heartRate(0),
    m_power(0),
    m_cadence(0),
    m_isMonarkConnectionAlive(true)
{
    m_monarkConnection.setSerialPort(devname);
    connect(&m_monarkConnection, SIGNAL(power(quint32)), this, SLOT(newPower(quint32)), Qt::QueuedConnection);
    connect(&m_monarkConnection, SIGNAL(cadence(quint32)), this, SLOT(newCadence(quint32)), Qt::QueuedConnection);
    connect(&m_monarkConnection, SIGNAL(pulse(quint32)), this, SLOT(newHeartRate(quint32)), Qt::QueuedConnection);
    connect(&m_monarkConnection, SIGNAL(finished()), this, SLOT(onMonarkConnectionFinished()), Qt::QueuedConnection);
}

Monark::~Monark()
{
}

int Monark::start()
{
    m_monarkConnection.start();
    return 0;
}

int Monark::restart()
{
    return 0;
}

int Monark::stop()
{
    return 0;
}

void Monark::newCadence(quint32 cadence)
{
    m_cadence = cadence;
}

void Monark::newHeartRate(quint32 heartRate)
{
    m_heartRate = heartRate;
}

void Monark::newPower(quint32 power)
{
    m_power = power;
}

/**
 * This functions takes a serial port and tries if it can find a Monark bike connected
 * to it.
 */
bool Monark::discover(QString portName)
{
    bool found = false;
    QSerialPort sp;

    sp.setPortName(portName);

    if (sp.open(QSerialPort::ReadWrite))
    {
        m_monarkConnection.configurePort(&sp);

        // Discard any existing data
        QByteArray data = sp.readAll();

        // Read id from bike
        sp.write("id\r");
        sp.waitForBytesWritten(-1);

        QByteArray id;
        do
        {
            bool readyToRead = sp.waitForReadyRead(1000);
            if (readyToRead)
            {
                id.append(sp.readAll());
            } else {
                id.append('\r');
            }
        } while ((id.indexOf('\r') == -1));

        id.replace("\r", "\0");

        // Should check for all bike ids known to use this protocol
        if (QString(id).toLower().contains("lt") ||
            QString(id).toLower().contains("lc") ||
            QString(id).toLower().contains("novo")) {
            found = true;
        }
    }

    sp.close();

    return found;
}


void Monark::setLoad(double load)
{
    m_monarkConnection.setLoad((unsigned int)load);
}

bool Monark::isConnected()
{
    return m_isMonarkConnectionAlive;
}

void Monark::onMonarkConnectionFinished()
{
    m_isMonarkConnectionAlive = false;
}
