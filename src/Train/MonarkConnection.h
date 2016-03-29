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
    static void configurePort(QSerialPort * serialPort);
    static bool discover(QString portName);
    quint32 power();
    quint32 cadence();
    quint32 pulse();

public slots:
    void requestAll();
    void requestPower();
    void requestPulse();
    void requestCadence();
    int readConfiguredLoad();
    void identifyModel();
    void setLoad(unsigned int load);
    void setKp(double kp);

private:
    QString m_serialPortName;
    QSerialPort *m_serial;
    int m_pollInterval;
    QString m_id;
    void run();
    QTimer *m_timer;
    QByteArray readAnswer(int timeoutMs = -1);
    QMutex m_mutex;
    QMutex m_readMutex;
    unsigned int m_load;
    unsigned int m_loadToWrite;
    double m_kp;
    double m_kpToWrite;
    bool m_shouldWriteLoad;
    bool m_shouldWriteKp;
    enum MonarkType { MONARK_UNKNOWN, MONARK_LT2, MONARK_LC, MONARK_LC_NOVO } m_type;
    bool canDoLoad();
    bool canDoKp();
    quint32 m_power;
    quint32 m_cadence;
    quint32 m_pulse;

signals:
    void pulse(quint32);
    void cadence(quint32);
    void power(quint32);
};

#endif // _GC_MonarkConnection_h
