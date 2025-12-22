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
    m_load(0),
    m_loadToWrite(0),
    m_kp(0),
    m_kpToWrite(0),
    m_shouldWriteLoad(false),
    m_shouldWriteKp(false),
    m_type(MONARK_UNKNOWN),
    m_mode(MONARK_MODE_WATT),
    m_power(0),
    m_cadence(0),
    m_pulse(0)
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
QString MonarkConnection::readAnswer(int timeoutMs)
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
    return QString(data);
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
        (void)connect(m_timer, SIGNAL(timeout()), this, SLOT(requestAll()),Qt::DirectConnection);

        // Set up initial model detection
        (void)connect(startupTimer, SIGNAL(timeout()), this, SLOT(identifyModel()),Qt::DirectConnection);
    }

    m_timer->setInterval(1000);
    m_timer->start();

    startupTimer->setSingleShot(true);
    startupTimer->setInterval(0);
    startupTimer->start();

    exec();
}

void MonarkConnection::requestAll()
{
    // If something else is blocking mutex, don't start another round of requests
    if (! m_mutex.tryLock())
        return;

    // Discard any previous data, useful if e.g a restart of the bike caused us to get
    // out of sync
    QByteArray data = m_serial->readAll();
    Q_UNUSED(data)

    requestPower();
    requestPulse();
    requestCadence();

    if ((m_loadToWrite != m_load) && m_mode == MONARK_MODE_WATT && canDoLoad())
    {
        QString cmd = QString("power %1\r").arg(m_loadToWrite);
        (void)m_serial->write(cmd.toStdString().c_str());
        if (!m_serial->waitForBytesWritten(500))
        {
            // failure to write to device, bail out
            this->exit(-1);
        }
        m_load = m_loadToWrite;
        QByteArray data = m_serial->readAll();
    }

    if ((m_kpToWrite != m_kp) && m_mode == MONARK_MODE_KP && canDoKp())
    {
        QString cmd = QString("kp %1\r").arg(QString::number(m_kpToWrite, 'f', 1 ));
        (void)m_serial->write(cmd.toStdString().c_str());
        if (!m_serial->waitForBytesWritten(500))
        {
            // failure to write to device, bail out
            this->exit(-1);
        }
        m_kp = m_kpToWrite;
        QByteArray data = m_serial->readAll();
    }

    if ((m_mode == MONARK_MODE_KP) && canDoLoad() && !canDoKp())
    {
        // Calculate what wattage to request to simulate selected kp
        // watt = kp * cadence * 0.98
        unsigned int load = (m_kpToWrite * m_cadence) * 0.98;

        QString cmd = QString("power %1\r").arg(load);
        (void)m_serial->write(cmd.toStdString().c_str());
        if (!m_serial->waitForBytesWritten(500))
        {
            // failure to write to device, bail out
            this->exit(-1);
        }
        QByteArray data = m_serial->readAll();
    }

    m_mutex.unlock();
}

void MonarkConnection::requestPower()
{
    // Discard any existing data
    m_serial->readAll();

    (void)m_serial->write("power\r");
    if (!m_serial->waitForBytesWritten(500))
    {
        // failure to write to device, bail out
        this->exit(-1);
    }
    QString data = readAnswer(500);
    m_readMutex.lock();
    m_power = data.toInt();
    m_readMutex.unlock();
    emit power(m_power);
}

void MonarkConnection::requestPulse()
{
    // Discard any existing data
    m_serial->readAll();

    (void)m_serial->write("pulse\r");
    if (!m_serial->waitForBytesWritten(500))
    {
        // failure to write to device, bail out
        this->exit(-1);
    }
    QString data = readAnswer(500);
    m_readMutex.lock();
    m_pulse = data.toInt();
    m_readMutex.unlock();
    emit pulse(m_pulse);
}

void MonarkConnection::requestCadence()
{
    // Discard any existing data
    m_serial->readAll();

    (void)m_serial->write("pedal\r");
    if (!m_serial->waitForBytesWritten(500))
    {
        // failure to write to device, bail out
        this->exit(-1);
    }
    QString data = readAnswer(500);
    m_readMutex.lock();
    m_cadence = data.toInt();
    m_readMutex.unlock();
    emit cadence(m_cadence);
}

int MonarkConnection::readConfiguredLoad()
{
    // Discard any existing data
    m_serial->readAll();

    (void)m_serial->write("B\r");
    if (!m_serial->waitForBytesWritten(500))
    {
        // failure to write to device, bail out
        this->exit(-1);
    }
    QString data = readAnswer(500);
    //data.remove(0,1);
    qDebug() << "Current configure load: " << data.toInt();
    return data.toInt();
}

void MonarkConnection::identifyModel()
{
    // Discard any existing data
    m_serial->readAll();

    QString servo = "";

    (void)m_serial->write("id\r");
    if (!m_serial->waitForBytesWritten(500))
    {
        // failure to write to device, bail out
        this->exit(-1);
    }
    m_id = readAnswer(500);

    if (m_id.toLower().startsWith("novo"))
    {
        (void)m_serial->write("servo\r");
        if (!m_serial->waitForBytesWritten(500))
        {
            // failure to write to device, bail out
            this->exit(-1);
        }
        servo = readAnswer(500);
    }


    qDebug() << "Connected to bike: " << m_id;
    qDebug() << "Servo: : " << servo;

    if (m_id.toLower().startsWith("lc"))
    {
        m_type = MONARK_LC;
        setLoad(100);
    } else if (m_id.toLower().startsWith("novo") && servo != "manual") {
        m_type = MONARK_LC_NOVO;
        setLoad(100);
    } else if (m_id.toLower().startsWith("mec")) {
        m_type = MONARK_839E;
        setLoad(100);
    }

}

void MonarkConnection::setLoad(unsigned int load)
{
    m_loadToWrite = load;
    m_mode = MONARK_MODE_WATT;
}

void MonarkConnection::setKp(double kp)
{
    if (kp < 0)
        kp = 0;

    if (kp > 7)
        kp = 7;

    m_kpToWrite = kp;
    m_mode = MONARK_MODE_KP;
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
    (void)serialPort->write("\r");
}

bool MonarkConnection::canDoLoad()
{
    bool result = false;

    switch (m_type)
    {
    case MONARK_LC: // fall through
    case MONARK_LC_NOVO: // fall through
    case MONARK_839E:
        result = true;
        break;
    case MONARK_LT2: // fall through
    default:
        result = false;
        break;
    }

    return result;
}

bool MonarkConnection::canDoKp()
{
    bool result = false;

    switch (m_type)
    {
    case MONARK_LC_NOVO:
        result = true;
        break;
    case MONARK_LC: // fall through
    case MONARK_LT2: // fall through
    case MONARK_839E: // fall through
    default:
        result = false;
        break;
    }

    return result;
}

/**
 * This functions takes a serial port and tries if it can find a Monark bike connected
 * to it.
 */
bool MonarkConnection::discover(QString portName)
{
    bool found = false;
    QSerialPort sp;

    sp.setPortName(portName);

    if (sp.open(QSerialPort::ReadWrite))
    {
        configurePort(&sp);

        // Discard any existing data
        QByteArray data = sp.readAll();

        // Read id from bike
        (void)sp.write("id\r");
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
            QString(id).toLower().contains("mec") ||
            QString(id).toLower().contains("novo")) {
            found = true;
        }
    }

    sp.close();

    return found;
}

quint32 MonarkConnection::power()
{
    QMutexLocker mlock(&m_readMutex);
    return m_power;
}

quint32 MonarkConnection::cadence()
{
    QMutexLocker mlock(&m_readMutex);
    return m_cadence;
}

quint32 MonarkConnection::pulse()
{
    QMutexLocker mlock(&m_readMutex);
    return m_pulse;
}

double MonarkConnection::kp()
{
    return m_kp;
}
