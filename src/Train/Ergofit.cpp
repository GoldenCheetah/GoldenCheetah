/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net),
 *                    Mark Liversedge (liversedge@gmail.com),
 *               2020 Manfred Mayer (mayerflash@gmx.de)
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


#include "Ergofit.h"

Ergofit::Ergofit(QObject *parent,  QString devname) : QObject(parent),
    m_heartRate(0),
    m_power(0),
    m_cadence(0),
    m_isErgofitConnectionAlive(true)
{
    m_ergofitConnection.setSerialPort(devname);
    if (!connect(&m_ergofitConnection, SIGNAL(power(quint32)), this, SLOT(newPower(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect power signal in Ergofit");
    }
    if (!connect(&m_ergofitConnection, SIGNAL(cadence(quint32)), this, SLOT(newCadence(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect cadence signal in Ergofit");
    }
    if (!connect(&m_ergofitConnection, SIGNAL(pulse(quint32)), this, SLOT(newHeartRate(quint32)), Qt::QueuedConnection)) {
        qFatal("Failed to connect pulse signal in Ergofit");
    }
    if (!connect(&m_ergofitConnection, SIGNAL(finished()), this, SLOT(onErgofitConnectionFinished()), Qt::QueuedConnection)) {
        qFatal("Failed to connect finished signal in Ergofit");
    }
}

Ergofit::~Ergofit()
{
}

int Ergofit::start()
{
    m_ergofitConnection.start();
    return 0;
}

int Ergofit::restart()
{
    return 0;
}

int Ergofit::stop()
{
    return 0;
}

void Ergofit::newCadence(quint32 cadence)
{
    m_cadence = cadence;
}

void Ergofit::newHeartRate(quint32 heartRate)
{
    m_heartRate = heartRate;
}

void Ergofit::newPower(quint32 power)
{
    m_power = power;
}

/**
 * This functions takes a serial port and tries if it can find a Ergofit bike connected
 * to it.
 */
bool Ergofit::discover(QString portName)
{
    bool found = false;
    QSerialPort sp;

    sp.setPortName(portName);

    if (sp.open(QSerialPort::ReadWrite))
    {
        m_ergofitConnection.configurePort(&sp);

        // Read any already existing data on serial line in order to empty it
        QByteArray discover_data = sp.readAll();

        // Check if bike is there
        if (sp.write("\x10\x56\x03\x38\x34\x17") == -1) {
             qWarning("Failed to write to serial port during discovery in Ergofit");
             return found;
        }
        if (!sp.waitForBytesWritten(500)) {
            qWarning("Discover send string timed out");
            return found;
        }

        if (sp.waitForReadyRead(500)) {
            discover_data = sp.readAll();
            while (sp.waitForReadyRead(100)) discover_data += sp.readAll();
        } else {
            qWarning("No response from device during discovery");
            return found;
        }
        if (QString(discover_data).startsWith("\x01\x56\x02\x43\x59\x31\x2e\x30\x30\x03\x53\x17"))
        {
            found = true;
        }
    }

    sp.close();

    return found;
}


void Ergofit::setLoad(double load)
{
    m_ergofitConnection.setLoad((unsigned int)load);
}

bool Ergofit::isConnected()
{
    return m_isErgofitConnectionAlive;
}

void Ergofit::onErgofitConnectionFinished()
{
    m_isErgofitConnectionAlive = false;
}
