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


#ifndef _GC_KettlerRacerConnection_h
#define _GC_KettlerRacerConnection_h 1

#include <QtSerialPort/QSerialPort>
//#include <QBluetoothSocket>
#include <QIODevice>

#include <QThread>
#include <QTimer>
//#include <QMutex>

class KettlerRacerConnection : public QThread
{
    Q_OBJECT

public:
    KettlerRacerConnection();
    ~KettlerRacerConnection();
    void setWeight(double m){m_athleteWeight=m;};
    void setPollInterval(int interval,int m);
    int pollInterval();
    void setSerialPort(const QString serialPortName);
    bool configurePort(QSerialPort *serialPort);
    void deleteSerialPort();
    void writeCommand(const QByteArray &cmd);
    void resetRead();
    void setGradient(double slope);
    int m_mode;//slope 2 or erg 1
    enum GearAction { fdUp, fdDown, rdUp, rdDown };
    void changeGearByAction(GearAction action);


    //bool m_useBluetooth = false; // serial or bluetooth-rfconn

public slots:
    void requestAll(); //timer send request
    void initializePcConnection();
    void setLoad(unsigned int load);
    void onReadyRead(); //read requestAll
public slots:
    void handleKeyPress(int key);
    //void setGradient(double slope);

private:
    QString m_serialPortName;

    //QBluetoothSocket *m_btSocket;   // bluetooth or serial
    QIODevice *m_device = nullptr;
    QByteArray m_rxBuffer;
    QByteArray data;
    QByteArray cmd;
    QByteArray line;
    QByteArrayList m_splits;
    QByteArrayList m_gears;
    int m_gearsKeyPressed;
    void parseES();
    void parseST();
    void parseGear();
    void parseALL();
    int m_pollInterval;
    void run();
    QTimer *m_timer;
    QMutex m_mutex;
    QSerialPort *m_serial;
    bool pauseRead; // use instead m_mutex
    unsigned int m_load;
    unsigned int m_loadToWrite;
    bool m_shouldWriteLoad;
    bool m_parseALLok;
    bool m_parseESok;
    bool m_parseSTok;
    bool m_parseGearsok;
    int m_shiftGear;
    int m_shiftFront;
    int m_shiftRear;
    double m_gradient;
    double m_gradient_old;
    double m_internalSpeed; // slope mode: computed equivalent road speed (km/h)
    double m_pignoni[12] = {36, 33, 30, 27, 25, 23, 21, 19, 17, 15, 13, 12};
    double m_pignone;
    double m_rapp;
    int conta=0;
    int idx;
    //double m_realPowerFiltered;
    void updateInternalSpeed();    // slope mode: compute speed from load/gradient/gear
    qint32 m_power;
    qint32 m_cadence;
    int m_countKR;
    double m_athleteWeight;

signals:
    void pulse(quint32);
    void cadence(quint32);
    void power(quint32);
    void speed(double);
    void timerIntervalChanged(int interval);
    void gearChanged(int front, int rear, double rapp);
};

#endif // _GC_KettlerRacerConnection_h
