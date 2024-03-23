/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com),
 *               2018 Florian Nairz (nairz.florian@gmail.com)
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

#ifndef _GC_Daum_h
#define _GC_Daum_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QSerialPort>
#include "RealtimeController.h"

/*
 * This class add support for some Daum devices that are connected
 * to the PC via serial port.
 * This work is based on the documents found on the daum-electronic site:
 * http://www.daum-electronic.de/de/support/supp02.html
 * and the implementation details are inspired by the Fortius class.
*/

class Daum : public QThread
{
    Q_OBJECT
public:
    // default load
    const int kDefaultLoad = 100;
    const int kQueryIntervalMS = 1000;

    Daum(QObject *parent, QString device, QString profile);

    int start();
    int restart();
    int pause();
    int stop();
    int quit();

    bool discover(QString dev);

    void setLoad(double load);
    double getPower() const;
    double getSpeed() const;
    double getCadence() const;
    double getHeartRate() const;

private:
    void run();

    bool openPort(QString dev);
    bool closePort();
    void initializeConnection();
    bool configureForCockpitType(int cockpitType);
    bool configureFromProfile();

    bool ResetDevice();
    bool StartProgram(unsigned int prog);
    bool StopProgram(unsigned int prog);
    int GetAddress();
    int CheckCockpit();
    int GetDeviceVersion();
    bool SetProgram(unsigned int prog);
    bool SetDate();
    bool SetTime();
    void PlaySound();

    QByteArray WriteDataAndGetAnswer(QByteArray const& dat, int response_bytes);
    char MapLoadToByte(unsigned int load) const;
    bool isPaused() const;


    // a lock for our private vars
    mutable QMutex pvars;

    QTimer *timer_;
    QString serialDeviceName_;
    QSerialPort *serial_dev_;

    char deviceAddress_;
    unsigned int maxDeviceLoad_;
    unsigned int serialWriteDelay_;
    bool playSound_;

    // state
    bool paused_;

    // inbound
    volatile int devicePower_;
    volatile double deviceHeartRate_;
    volatile double deviceCadence_;
    volatile double deviceSpeed_;

    // outbound
    volatile int load_, loadToWrite_;
    const bool forceUpdate_;
    const QString profile_;

    enum CockpitType {
        COCKPIT_CARDIO = 0x10,
        COCKPIT_FITNESS = 0x20,
        COCKPIT_8008_TRS = 0x2a,
        COCKPIT_VITA_DE_LUXE = 0x30,
        COCKPIT_8008 = 0x40,
        COCKPIT_8080 = 0x50,
        COCKPIT_UNKNOWN = 0x55,
        COCKPIT_THERAPIE = 0x60,
        COCKPIT_8008_TRS_PRO = 0x64
    };

private slots:
    void requestRealtimeData();
};

#endif // _GC_Daum_h

