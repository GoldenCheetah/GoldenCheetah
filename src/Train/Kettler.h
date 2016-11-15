/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_Kettler_h
#define _GC_Kettler_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QFile>
#include "RealtimeController.h"
#include "KettlerConnection.h"

class Kettler : public QObject
{
    Q_OBJECT
public:
    Kettler(QObject *parent=0, QString deviceFilename=0);       // pass device
    ~Kettler();

    int start();
    int stop();
    int restart();

    quint32 heartRate() {return m_heartRate;}
    quint32 power() {return m_power;}
    quint32 cadence() {return m_cadence;}
    double speed() {return m_speed;}
    bool discover(QString portName);

    void setLoad(double load);
    bool isConnected();

private:
    KettlerConnection m_kettlerConnection;
    quint32 m_heartRate;
    quint32 m_power;
    quint32 m_cadence;
    double m_speed; // in km/h
    bool m_isKettlerConnectionAlive;

private slots:
    void newHeartRate(quint32);
    void newPower(quint32);
    void newCadence(quint32 cadence);
    void newSpeed(double speed);
    void onKettlerConnectionFinished();
};

#endif // _GC_Kettler_h

