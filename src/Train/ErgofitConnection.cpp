/*
 * Copyright (c) 2020 Manfred Mayer (mayerflash@gmx.de)
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

#include "ErgofitConnection.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QTime>

ErgofitConnection::ErgofitConnection() :
    m_serial(0),
    m_pollInterval(1000),
    m_timer(0),
    m_load(0),
    m_loadToWrite(0),
    m_shouldWriteLoad(false)
{
}

void ErgofitConnection::setSerialPort(const QString serialPortName)
{
    if (! this->isRunning())
    {
        m_serialPortName = serialPortName;
    } else {
        qWarning() << "ErgofitConnection: Cannot set serialPortName while running";
    }
}

void ErgofitConnection::setPollInterval(int interval)
{
    if (interval != m_pollInterval)
    {
        m_pollInterval = interval;
        m_timer->setInterval(m_pollInterval);
    }
}

int ErgofitConnection::pollInterval()
{
    return m_pollInterval;
}


/**
 * QThread::run()
 *
 * Open the serial port and set it up, then starts polling.
 *
 */
void ErgofitConnection::run()
{
    // Open and configure serial port
    m_serial = new QSerialPort();
    m_serial->setPortName(m_serialPortName);

    m_timer = new QTimer();
    QTimer *startupTimer = new QTimer();

    if (!m_serial->open(QSerialPort::ReadWrite))
    {
        qWarning("Error opening serial");
        this->exit(-1);
    } else {
        configurePort(m_serial);

        // Read any already existing data on serial line in order to empty it
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

void ErgofitConnection::requestAll()
{
    // If something else is blocking mutex, don't start another round of requests
    if (! m_mutex.tryLock())
        return;

    // Read any already existing data on serial line in order to empty it
    QByteArray discarded = m_serial->readAll();

    // Request pulse
    m_serial->write("\x01\x50\x03\x38\x32\x17");
    if (!m_serial->waitForBytesWritten(500)) {
        qWarning() << tr("Send request pulse timed out. Time: %1").arg(QTime::currentTime().toString());
    }
    if (m_serial->waitForReadyRead(100)) {
        QByteArray pulse_data = m_serial->readAll();
        while (m_serial->waitForReadyRead(80)) pulse_data += m_serial->readAll();
        if (QString(pulse_data).startsWith("\x01\x50\x02")) {
            QString pulse_string = pulse_data.mid(3, 3);
            quint32 newHeartrate = pulse_string.toUInt();
            emit pulse(newHeartrate);
        } else {
            qWarning() << tr("Wrong pulse received. Time: %1").arg(QTime::currentTime().toString());
        }
    } else {
        qWarning() << tr("Receive of request pulse timed out. Time: %1").arg(QTime::currentTime().toString());
    }

    // Request cadence
    m_serial->write("\x01\x44\x03\x37\x30\x17");
    if (!m_serial->waitForBytesWritten(500)) {
        qWarning() << tr("Send request cadence timed out. Time %1").arg(QTime::currentTime().toString());
    }
    if (m_serial->waitForReadyRead(100)) {
        QByteArray cadence_data = m_serial->readAll();
        while (m_serial->waitForReadyRead(80)) cadence_data += m_serial->readAll();
        if (QString(cadence_data).startsWith("\x01\x44\x02")) {
            QString cadence_string = cadence_data.mid(3, 3);
            quint32 newCadence = cadence_string.toUInt();
            emit cadence(newCadence);
        } else {
            qWarning() << tr("Wrong cadence received. Time: %1").arg(QTime::currentTime().toString());
        }
    } else {
        qWarning() << tr("Receive of request cadence timed out. Time: %1").arg(QTime::currentTime().toString());
    }

    // Request power
    m_serial->write("\x01\x4c\x03\x37\x38\x17");
    if (!m_serial->waitForBytesWritten(500)) {
        qWarning() << tr("Send request power timed out. Time: %1").arg(QTime::currentTime().toString());
    }
    if (m_serial->waitForReadyRead(100)) {
        QByteArray power_data = m_serial->readAll();
        while (m_serial->waitForReadyRead(80)) power_data += m_serial->readAll();
        if (QString(power_data).startsWith("\x01\x4c\x02")) {
            QString power_string = power_data.mid(3, 3);
            quint32 newPower = power_string.toUInt();
            emit power(newPower);
        } else {
            qWarning() << tr("Wrong power received. Time: %1").arg(QTime::currentTime().toString());
        }
    } else {
        qWarning() << tr("Receive of request power timed out. Time: %1").arg(QTime::currentTime().toString());
    }

    if ((m_loadToWrite != m_load)) {
        QString cmd = QString("\x01\x57\x02%1\x03\x50\x17").arg(m_loadToWrite);
        m_serial->write(cmd.toStdString().c_str());
        if (!m_serial->waitForBytesWritten(500)) {
            qWarning() << tr("Send set load timed out. Time: %1").arg(QTime::currentTime().toString());
            this->exit(-1);
        }
        m_load = m_loadToWrite;
        if (m_serial->waitForReadyRead(100)) {
            QByteArray data = m_serial->readAll();
            while (m_serial->waitForReadyRead(80)) data += m_serial->readAll();
            if (!QString(data).startsWith("\x01\x57\x02")) {
                qWarning() << tr("Wrong load acknowledge received. Time: %1").arg(QTime::currentTime().toString());
            }
        } else {
            qWarning() << tr("Receive of acknowledge set load timed out. Time: %1").arg(QTime::currentTime().toString());
        }

    }

    m_mutex.unlock();
}

void ErgofitConnection::initializePcConnection()
{
    // Lock mutex in order to proper initialize bike mode. (Otherwise polling could disturb initialization)
    m_mutex.lock();

    // Read any already existing data on serial line in order to empty it
    QByteArray init_data = m_serial->readAll();

    // Set ergofit into PC-mode, and disable +/- Buttons.
    m_serial->write("\x01\x4f\x03\x4d\x17");
    if (!m_serial->waitForBytesWritten(500)) {
        qWarning("Send set pc-mode timed out");
        this->exit(-1);            // failure to write to device, bail out
    }
    if (m_serial->waitForReadyRead(100)) {
        init_data = m_serial->readAll();
        while (m_serial->waitForReadyRead(80)) init_data += m_serial->readAll();
        if (QString(init_data).startsWith("\x01\x4f\x03\x4d\x17")) {
        } else {
        qWarning("Did not get right acknowledge of pc-mode set");
        this->exit(-1);            // failure to write to device, bail out
        }
    } else {
        qWarning("Receive acknowledge of pc-mode set timed out");
        this->exit(-1);            // failure to write to device, bail out
    }

    // Set Standardbetrieb-Mode on ergofit
    m_serial->write("\x01\x52\x02\x30\x03\x62\x17");
    if (!m_serial->waitForBytesWritten(500)) {
        qWarning("Send set default-mode timed out");
        this->exit(-1);            // failure to write to device, bail out
    }
    if (m_serial->waitForReadyRead(100)) {
        init_data = m_serial->readAll();
        while (m_serial->waitForReadyRead(80)) init_data += m_serial->readAll();
        if (QString(init_data).startsWith("\x01\x52\x02\x30\x03\x62\x17")) {
        } else {
        qWarning("Did not get right acknowledge of default-mode set");
        this->exit(-1);            // failure to write to device, bail out
        }
    } else {
        qWarning("Receive acknowledge of default-mode set timed out");
        this->exit(-1);            // failure to write to device, bail out
    }

    setLoad(100);     // Load in GoldenCheetah is by default 100W
    m_mutex.unlock();      // Unlock mutex after initialization
}

void ErgofitConnection::setLoad(unsigned int load)
{
    m_loadToWrite = load;
    m_shouldWriteLoad = true;
}

/*
 * Configures a serialport for communicating with a Ergofit bike.
 */
void ErgofitConnection::configurePort(QSerialPort *serialPort)
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
