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

#ifndef _GC_MonarkConnection_h
#define _GC_MonarkConnection_h 1

#include <QtSerialPort/QSerialPort>
#include <QThread>
#include <QTimer>
#include <QMutex>

class MonarkConnection : public QThread
{
    Q_OBJECT

public:
    MonarkConnection();
    void setPollInterval(int interval);
    int pollInterval();
    void setSerialPort(const QString serialPortName);
    void configurePort(QSerialPort * serialPort);

public slots:
    void requestAll();
    void requestPower();
    void requestPulse();
    void requestCadence();
    void identifyModel();
    void setLoad(unsigned int load);

private:
    QString m_serialPortName;
    QSerialPort *m_serial;
    int m_pollInterval;
    QString m_id;
    void run();
    QTimer *m_timer;
    QByteArray readAnswer(int timeoutMs = -1);
    QMutex m_mutex;
    bool m_canControlPower;
    unsigned int m_load;
    unsigned int m_loadToWrite;
    bool m_shouldWriteLoad;

signals:
    void pulse(quint32);
    void cadence(quint32);
    void power(quint32);
};

#endif // _GC_MonarkConnection_h
