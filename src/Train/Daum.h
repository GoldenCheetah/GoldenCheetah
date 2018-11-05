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

    Daum(QObject *parent = 0, QString device = "", QString profile = "");
    virtual ~Daum();

    QObject *parent;

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

    virtual bool ResetDevice();
    virtual bool StartProgram(unsigned int prog);
    virtual bool StopProgram(unsigned int prog);
    virtual int GetAddress();
    virtual int CheckCockpit();
    virtual int GetDeviceVersion();
    virtual bool SetProgram(unsigned int prog);
    virtual bool SetDate();
    virtual bool SetTime();
    virtual void PlaySound();

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

private slots:
    void requestRealtimeData();
};

#endif // _GC_Daum_h

