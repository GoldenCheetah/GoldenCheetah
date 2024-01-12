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

#include "KettlerConnection.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

KettlerConnection::KettlerConnection() :
    m_serial(0),
    m_pollInterval(1000),
    m_timer(0),
    m_load(0),
    m_loadToWrite(0),
    m_shouldWriteLoad(false)
{
}

void KettlerConnection::setSerialPort(const QString serialPortName)
{
    if (! this->isRunning())
    {
        m_serialPortName = serialPortName;
    } else {
        qWarning() << "KettlerConnection: Cannot set serialPortName while running";
    }
}

void KettlerConnection::setPollInterval(int interval)
{
    if (interval != m_pollInterval)
    {
        m_pollInterval = interval;
        m_timer->setInterval(m_pollInterval);
    }
}

int KettlerConnection::pollInterval()
{
    return m_pollInterval;
}


/**
 * QThread::run()
 *
 * Open the serial port and set it up, then starts polling.
 *
 */
void KettlerConnection::run()
{
    // Open and configure serial port
    m_serial = new QSerialPort();
    m_serial->setPortName(m_serialPortName);

    m_timer = new QTimer();
    QTimer *startupTimer = new QTimer();

    if (!m_serial->open(QSerialPort::ReadWrite))
    {
        qDebug() << "Error opening serial";
        this->exit(-1);
    } else {
        configurePort(m_serial);

        // Discard any existing data
        QByteArray data = m_serial->readAll();

        // Set up polling
        connect(m_timer, SIGNAL(timeout()), this, SLOT(requestAll()),Qt::DirectConnection);

        // Set up initial model detection
        connect(startupTimer, SIGNAL(timeout()), this, SLOT(initializePcConnection()),Qt::DirectConnection);
    }

    m_timer->setInterval(1000);
    m_timer->start();

    startupTimer->setSingleShot(true);
    startupTimer->setInterval(0);
    startupTimer->start();

    exec();
}

void KettlerConnection::requestAll()
{
    // If something else is blocking mutex, don't start another round of requests
    if (! m_mutex.tryLock())
        return;

    // Discard any existing data
    QByteArray discarded = m_serial->readAll();

    m_serial->write("ST\r\n");
    m_serial->waitForBytesWritten(1000);

    QByteArray data;
    bool completeReplyRead = false;
    bool failed = false;
    int maxRetries = 3;
    do
    {
        if (m_serial->waitForReadyRead(500))
        {
            data.append(m_serial->readAll());
        } else {
            failed = true;
        }

        QString dataString = QString(data);
        QStringList splits = dataString.split(QRegularExpression("\\s"));

        // We need to make sure the last split is 3 chars long, otherwise we
        // might have read a partial power value

        if (splits.size() >= 8 && (splits.at(7).length() >= 3))
        {
            completeReplyRead = true;
            failed = false;
            bool ok;

            quint32 newHeartrate = splits.at(0).toUInt(&ok);
            if (ok)
            {
                emit pulse(newHeartrate);
            }

            quint32 newCadence = splits.at(1).toUInt(&ok);
            if (ok)
            {
                emit cadence(newCadence);
            }

            quint32 newSpeed = splits.at(2).toUInt(&ok);
            if (ok)
            {
                emit speed(newSpeed/10);
            }

            quint32 newPower = splits.at(7).toUInt(&ok);

            if (ok)
            {
                emit power(newPower);
            }
        } else if (splits.size() > 8) {
            qDebug() << "Kettler: Faulty sample, larger than 8 splits.";
            failed = true;
        }

        if (--maxRetries == 0)
        {
            failed = true;
        }
    } while ((!completeReplyRead) || failed);

    if ((m_loadToWrite != m_load))
    {
        QString cmd = QString("pw %1\r\n").arg(m_loadToWrite);
        m_serial->write(cmd.toStdString().c_str());
        if (!m_serial->waitForBytesWritten(500))
        {
            // failure to write to device, bail out
            this->exit(-1);
        }
        m_load = m_loadToWrite;

        // Ignore reply
        QByteArray data = m_serial->readAll();

        data.append('\0');
    }

    m_mutex.unlock();
}

void KettlerConnection::initializePcConnection()
{
    int maxRetries = 3;
    bool keepTrying = false;

    do
    {
        keepTrying = (--maxRetries != 0);

        // Set kettler into PC-mode, reply should be ACK or RUN
        m_serial->write("cd\r\n");

        if (!m_serial->waitForBytesWritten(500))
        {
            // failure to write to device, bail out
            this->exit(-1);
        }

        QByteArray data;

        if (m_serial->waitForReadyRead(500))
        {
            data = m_serial->readAll();
            if (QString(data).contains("ACK") || QString(data).contains("RUN"))
            {
                keepTrying = false;
            }
        }


    } while (keepTrying);

    setLoad(100);
}

void KettlerConnection::setLoad(unsigned int load)
{
    m_loadToWrite = load;
    m_shouldWriteLoad = true;
}

/*
 * Configures a serialport for communicating with a Kettler bike.
 */
void KettlerConnection::configurePort(QSerialPort *serialPort)
{
    if (!serialPort)
    {
        qFatal("Trying to configure null port, start debugging.");
    }
    serialPort->setBaudRate(QSerialPort::Baud9600);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->setParity(QSerialPort::NoParity);
}
