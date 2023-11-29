/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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
#include "GoldenCheetah.h"

#include "RealtimeController.h"
#include "DeviceConfiguration.h"
#include "ConfigDialog.h"
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include "BT40Device.h"

#ifndef _GC_BT40Controller_h
#define _GC_BT40Controller_h 1

class DeviceInfo;

class BT40Controller : public RealtimeController
{
    Q_OBJECT

public:
    BT40Controller (TrainSidebar *parent =0, DeviceConfiguration *dc =0);
    ~BT40Controller();

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread

    bool find();
    bool discover(QString name);
    void setDevice(QString);
    QList<QBluetoothDeviceInfo> getDeviceInfo();

    void setLoad(double);
    void setGradient(double);
    void setMode(int);
    void setWindSpeed(double);
    void setWeight(double);
    void setRollingResistance(double);
    void setWindResistance(double);
    void setWheelCircumference(double);

    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void pushRealtimeData(RealtimeData &rtData);
    void setBPM(float x) {
	telemetry.setHr(x);
    }
    void setWatts(double watts) {
	telemetry.setWatts(watts);
    }
    void setWheelRpm(double wrpm);
    void setSpeed(double speed) {
        telemetry.setSpeed(speed);
    }
    void setCadence(double cadence) {
	telemetry.setCadence(cadence);
    }
    void setRespiratoryFrequency(double rf) {
        telemetry.setRf(rf);
    }
    void setRespiratoryMinuteVolume(double rmv) {
        telemetry.setRMV(rmv);
    }
    void setVO2_VCO2(double vo2, double vco2) {
        telemetry.setVO2_VCO2(vo2, vco2);
    }
    void setTv(double tv) {
        telemetry.setTv(tv);
    }
    void setFeO2(double feo2) {
        telemetry.setFeO2(feo2);
    }
    void emitVO2Data() {
        emit vo2Data(telemetry.getRf(), telemetry.getRMV(), telemetry.getVO2(), telemetry.getVCO2(), telemetry.getTv(), telemetry.getFeO2());
    }

    // Calibration overrides.
    uint8_t  getCalibrationType();
    uint8_t  getCalibrationState();
    double   getCalibrationTargetSpeed();
    uint16_t getCalibrationSpindownTime();
    uint16_t getCalibrationZeroOffset();
    uint16_t getCalibrationSlope();

signals:
    void vo2Data(double rf, double rmv, double vo2, double vco2, double tv, double feo2);
    void scanFinished(bool foundAnyDevices);

private slots:
    void addDevice(const QBluetoothDeviceInfo&);
    void scanFinished();
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error);

private:
    bool deviceAllowed(const QBluetoothDeviceInfo& info);

private:
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QBluetoothLocalDevice* localDevice;
    RealtimeData telemetry;
    QList<BT40Device*> devices;
    DeviceConfiguration* localDc;
    QList<DeviceInfo> allowedDevices;

    double load;
    double gradient;
    int mode;
    double windSpeed;
    double weight;
    double rollingResistance;
    double windResistance;
    double wheelSize;
};

class DeviceInfo
{
public:
    DeviceInfo(QString data);
    QString getName() const;
    QString getUuid() const;
    QString getAddress() const;
private:
    QString name;
    QString uuid;
    QString address;
};
#endif // _GC_BT40Controller_h
