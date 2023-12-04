/*
 * Copyright (c) 2016 Arto Jantunen (viiru@iki.fi)
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

#ifndef gc_BT40Device_h
#define gc_BT40Device_h

#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QQueue>

#include "CalibrationData.h"
#include "Ftms.h"

typedef struct btle_sensor_type {
    const char *descriptive_name;
    const char *iconname;
} btle_sensor_type_t;

class BT40Device: public QObject
{
    Q_OBJECT

public:
    BT40Device(QObject *parent, QBluetoothDeviceInfo devinfo);
    ~BT40Device();
    void connectDevice();
    void disconnectDevice();
    static QMap<QBluetoothUuid, btle_sensor_type_t> supportedServices;
    QBluetoothDeviceInfo deviceInfo() const;

    void setLoad(double);
    void setGradient(double);
    void setMode(int);
    void setWindSpeed(double);
    void setWeight(double);
    void setRollingResistance(double);
    void setWindResistance(double);
    void setWheelCircumference(double);

    uint8_t  getCalibrationType()         { return calibrationData.getType(); }
    uint8_t  getCalibrationState()        { return calibrationData.getState(); }
    uint16_t getCalibrationZeroOffset()   { return calibrationData.getZeroOffset(); }
    uint16_t getCalibrationSpindownTime() { return calibrationData.getSpindownTime(); }
    double   getCalibrationTargetSpeed()  { return calibrationData.getTargetSpeed(); }
    uint16_t getCalibrationSlope()        { return calibrationData.getSlope(); }

private slots:
    void deviceConnected();
    void deviceDisconnected();
    void controllerError(QLowEnergyController::Error error);
    void serviceDiscovered(QBluetoothUuid uuid);
    void serviceScanDone();
    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void updateValue(const QLowEnergyCharacteristic &c,
		     const QByteArray &value);
    void confirmedDescriptorWrite(const QLowEnergyDescriptor &d,
				  const QByteArray &value);
    void confirmedCharacteristicWrite(const QLowEnergyCharacteristic &c, 
                                      const QByteArray &value);
    void serviceError(QLowEnergyService::ServiceError e);

signals:
    void setNotification(QString msg, int timeout);
    void deviceConnected(QString address, QString uuid);
    void deviceDisconnected(QString address, QString uuid);
    void deviceConnectionError(QString address, QString uuid);

private:
    QObject *parent;
    QBluetoothDeviceInfo m_currentDevice;
    QLowEnergyController *m_control;
    QList<QLowEnergyService*> m_services;
    int prevCrankStaleness;
    quint16 prevCrankTime;
    quint16 prevCrankRevs;
    bool prevWheelStaleness;
    quint16 prevWheelTime;
    quint32 prevWheelRevs;
    double load;
    double gradient;
    double prevGradient;
    int mode;
    double windSpeed;
    double weight;
    double rollingResistance;
    double windResistance;
    double wheelSize;
    bool has_power;
    bool has_controllable_service;
    CalibrationData calibrationData;

    // Service and Characteristic to set load
    enum {Load_None, Tacx_UART, Wahoo_Kickr, Kurt_InRide, Kurt_SmartControl, FTMS_Device} loadType;
    QLowEnergyCharacteristic loadCharacteristic;
    QLowEnergyService* loadService;
    QQueue<QByteArray> commandQueue;
    int commandRetry;

    // FTMS Device Configuration
    FtmsDeviceInformation ftmsDeviceInfo;

    bool connected;
    void getCadence(QDataStream& ds);
    void getWheelRpm(QDataStream& ds);
    void setLoadErg(double);
    void setLoadIntensity(double);
    void setLoadLevel(int);
    void setRiderCharacteristics(double weight, double rollingResistance, double windResistance);
    void sendSimulationParameters();
    void commandSend(QByteArray &command);
    void commandWrite(QByteArray &command);
    void commandWriteFailed();
    void commandWritten();
};

#endif
