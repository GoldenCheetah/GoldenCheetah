/*
 * Copyright (c) 2015 Erik Botö (erik.boto@gmail.com)
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

#include "MonarkConnection.h"

#include <QByteArray>
#include <QDebug>

MonarkConnection::MonarkConnection() :
    m_serial(0),
    m_pollInterval(1000),
    m_timer(0),
    m_canControlPower(false),
    m_load(0),
    m_loadToWrite(0),
    m_shouldWriteLoad(false)
{
}

void MonarkConnection::setSerialPort(const QString serialPortName)
{
    if (! this->isRunning())
    {
        m_serialPortName = serialPortName;
    } else {
        qWarning() << "MonarkConnection: Cannot set serialPortName while running";
    }
}

void MonarkConnection::setPollInterval(int interval)
{
    if (interval != m_pollInterval)
    {
        m_pollInterval = interval;
        m_timer->setInterval(m_pollInterval);
    }
}

int MonarkConnection::pollInterval()
{
    return m_pollInterval;
}

/**
 * Private function that reads a complete reply and prepares if for
 * processing by replacing \r with \0
 */
QByteArray MonarkConnection::readAnswer(int timeoutMs)
{
    QByteArray data;

    do
    {
        if (m_serial->waitForReadyRead(timeoutMs))
        {
            data.append(m_serial->readAll());
        } else {
            data.append('\r');
        }
    } while (data.indexOf('\r') == -1);

    data.replace("\r", "\0");
    return data;
}

/**
 * QThread::run()
 *
 * Open the serial port and set it up, then starts polling.
 *
 */
void MonarkConnection::run()
{
    // Open and configure serial port
    m_serial = new QSerialPort();
    m_serial->setPortName(m_serialPortName);

    m_timer = new QTimer();

    if (!m_serial->open(QSerialPort::ReadWrite))
    {
        qDebug() << "Error opening serial";
    } else {
        configurePort(m_serial);
        // Discard any existing data
        QByteArray data = m_serial->readAll();

        // Read id from bike
        m_serial->write("id\r");
        m_serial->waitForBytesWritten(-1);
        QByteArray id = readAnswer(1000);
        m_id = QString(id);

        qDebug() << "Connected to bike: " << m_id;

        if (m_id.toLower().startsWith("lc"))
        {
            m_canControlPower = true;
            setLoad(100);
        }

        // Set up polling
        connect(m_timer, SIGNAL(timeout()), this, SLOT(requestAll()),Qt::DirectConnection);
    }

    m_timer->setInterval(1000);
    m_timer->start();

    exec();
}

void MonarkConnection::requestAll()
{
    // If something else is blocking mutex, don't start another round of requests
    if (! m_mutex.tryLock())
        return;

    requestPower();
    requestPulse();
    requestCadence();

    if ((m_loadToWrite != m_load) && m_canControlPower)
    {
        QString cmd = QString("power %1\r").arg(m_loadToWrite);
        m_serial->write(cmd.toStdString().c_str());
        if (m_serial->waitForBytesWritten(100))
            m_load = m_loadToWrite;
        QByteArray data = m_serial->readAll();
    }

    m_mutex.unlock();
}

void MonarkConnection::requestPower()
{
    m_serial->write("power\r");
    m_serial->waitForBytesWritten(-1);
    QByteArray data = readAnswer();
    quint32 p = data.toInt();
    emit power(p);
}

void MonarkConnection::requestPulse()
{
    m_serial->write("pulse\r");
    m_serial->waitForBytesWritten(-1);
    QByteArray data = readAnswer();
    quint32 p = data.toInt();
    emit pulse(p);
}

void MonarkConnection::requestCadence()
{
    m_serial->write("pedal\r");
    m_serial->waitForBytesWritten(-1);
    QByteArray data = readAnswer();
    quint32 c = data.toInt();
    emit cadence(c);
}

void MonarkConnection::setLoad(unsigned int load)
{
    m_loadToWrite = load;
    m_shouldWriteLoad = true;
}

/*
 * Configures a serialport for communicating with a Monark bike.
 */
void MonarkConnection::configurePort(QSerialPort *serialPort)
{
    if (!serialPort)
    {
        qFatal("Trying to configure null port, start debugging.");
    }
    serialPort->setBaudRate(QSerialPort::Baud4800);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::SoftwareControl);
    serialPort->setParity(QSerialPort::NoParity);

    // Send empty \r after configuring port, otherwise first command might not
    // be interpreted correctly
    serialPort->write("\r");
}
