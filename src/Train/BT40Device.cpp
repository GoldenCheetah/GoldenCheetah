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

#include "BT40Device.h"
#include <QDebug>
#include "BT40Controller.h"

QMap<QBluetoothUuid, btle_sensor_type_t> BT40Device::supportedServices = {
    { QBluetoothUuid(QBluetoothUuid::HeartRate), { "Heartrate", ":images/IconHR.png" }},
    { QBluetoothUuid(QBluetoothUuid::CyclingPower), { "Power", ":images/IconPower.png" }},
    { QBluetoothUuid(QBluetoothUuid::CyclingSpeedAndCadence), { "Speed + Cadence", ":images/IconCadence.png" }},
};

BT40Device::BT40Device(QObject *parent, QBluetoothDeviceInfo devinfo) : parent(parent), m_currentDevice(devinfo)
{
    m_control = new QLowEnergyController(m_currentDevice, this);
    connect(m_control, SIGNAL(connected()), this, SLOT(deviceConnected()));
    connect(m_control, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(controllerError(QLowEnergyController::Error)));
    connect(m_control, SIGNAL(disconnected()), this, SLOT(deviceDisconnected()));
    connect(m_control, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(serviceDiscovered(QBluetoothUuid)));
    connect(m_control, SIGNAL(discoveryFinished()), this, SLOT(serviceScanDone()));

    connected = false;
    prevWheelTime = 0;
    prevWheelRevs = 0;
    prevCrankTime = 0;
    prevCrankRevs = 0;
    prevCrankStaleness = -1; // indicates prev crank data values aren't measured values
}

BT40Device::~BT40Device()
{
    this->disconnectDevice();
    delete m_control;
}

void
BT40Device::connectDevice()
{
    qDebug() << "Connecting to device" << m_currentDevice.name();
    m_control->setRemoteAddressType(QLowEnergyController::RandomAddress);
    m_control->connectToDevice();
    connected = true;
}

void
BT40Device::disconnectDevice()
{
    qDebug() << "Disconnecting from device" << m_currentDevice.name();
    connected = false;
    m_control->disconnectFromDevice();
}

void
BT40Device::deviceConnected()
{
    qDebug() << "Connected to device" << m_currentDevice.name();
    m_control->discoverServices();
}

void
BT40Device::controllerError(QLowEnergyController::Error error)
{
    qWarning() << "Controller Error:" << error << "for device" << m_currentDevice.name();
}

void
BT40Device::deviceDisconnected()
{
    qDebug() << "Lost connection to" << m_currentDevice.name();

    // Zero any readings provided by this device
    foreach (QLowEnergyService* const &service, m_services) {

        if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::HeartRate)) {
            dynamic_cast<BT40Controller*>(parent)->setBPM(0.0);
        } else if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::CyclingPower)) {
            dynamic_cast<BT40Controller*>(parent)->setWatts(0.0);
        } else if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::CyclingSpeedAndCadence)) {
            dynamic_cast<BT40Controller*>(parent)->setWheelRpm(0.0);
        }
    }

    // Try to reconnect if the connection was lost inadvertently
    if (connected) {
        this->connectDevice();
    }
}

void
BT40Device::serviceDiscovered(QBluetoothUuid uuid)
{

    if (supportedServices.contains(uuid)) {
        QLowEnergyService *service = m_control->createServiceObject(uuid, this);
        m_services.append(service);
    }

}

void
BT40Device::serviceScanDone()
{
    qDebug() << "Service scan done for device" << m_currentDevice.name();
    bool has_power = false;
    bool has_csc = false;
    QLowEnergyService* csc_service=NULL;
    foreach (QLowEnergyService* const &service, m_services) {

        qDebug() << "Discovering details for service" << service->serviceUuid() << "for device" << m_currentDevice.name();

        connect(service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(serviceStateChanged(QLowEnergyService::ServiceState)));
        connect(service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(updateValue(QLowEnergyCharacteristic,QByteArray)));
        connect(service, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(confirmedDescriptorWrite(QLowEnergyDescriptor,QByteArray)));
        connect(service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(serviceError(QLowEnergyService::ServiceError)));

        if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::CyclingPower)) {

            has_power = true;
            service->discoverDetails();

        } else if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::CyclingSpeedAndCadence)) {

            has_csc = true;
            csc_service = service;

        } else {

            service->discoverDetails();
        }
    }

    if (csc_service && has_csc && !has_power) {

        // Only connect to CSC service if the same device doesn't provide a power service
        // since the power service also provides the same readings.
        qDebug() << "Connecting to the CSC service for device" << m_currentDevice.name();
        csc_service->discoverDetails();

    } else {

        qDebug() << "Ignoring the CSC service for device" << m_currentDevice.name();
    }
}

void
BT40Device::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    qDebug() << "service state changed" << s << "for device" << m_currentDevice.name();

    if (s == QLowEnergyService::ServiceDiscovered) {

        foreach (QLowEnergyService* const &service, m_services) {

            if (service->state() == s) {

                QLowEnergyCharacteristic characteristic;
                if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::HeartRate)) {

                    characteristic = service->characteristic(
                    QBluetoothUuid(QBluetoothUuid::HeartRateMeasurement));

                } else if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::CyclingPower)) {

                    characteristic = service->characteristic(
                    QBluetoothUuid(QBluetoothUuid::CyclingPowerMeasurement));

                } else if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::CyclingSpeedAndCadence)) {

                    characteristic = service->characteristic(
                    QBluetoothUuid(QBluetoothUuid::CSCMeasurement));
                }

                if (characteristic.isValid()) {

                    const QLowEnergyDescriptor notificationDesc = characteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                    if (notificationDesc.isValid()) {
                        service->writeDescriptor(notificationDesc, QByteArray::fromHex("0100"));
                    }
                }
            }
        }
    }
}

void
BT40Device::updateValue(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    QDataStream ds(value);
    ds.setByteOrder(QDataStream::LittleEndian); // Bluetooth data is always LE

    if (c.uuid() == QBluetoothUuid(QBluetoothUuid::HeartRateMeasurement)) {
        quint8 flags;
        ds >> flags;
        float hr;
        if (flags & 0x1) { // HR 16 bit? otherwise 8 bit
            quint16 heartRate;
            ds >> heartRate;
            hr = (float) heartRate;
        } else {
            quint8 heartRate;
            ds >> heartRate;
            hr = (float) heartRate;
        }
        dynamic_cast<BT40Controller*>(parent)->setBPM(hr);

    } else if (c.uuid() == QBluetoothUuid(QBluetoothUuid::CyclingPowerMeasurement)) {

        quint16 flags;
        ds >> flags;
        qint16 tmp_pwr;
        ds >> tmp_pwr;
        double power = (double) tmp_pwr;
        dynamic_cast<BT40Controller*>(parent)->setWatts(power);

        if (flags & 0x01) { // power balance present
            qint8 byte;
            ds >> byte;
        }

        if (flags & 0x04) { // accumulated torque data present
            qint16 word;
            ds >> word;
        }

        if (flags & 0x10) { // wheel revolutions data present
            getWheelRpm(ds);
        }

        if (flags & 0x20) { // crank data present
            // If this power meter reports crank revolutions, it is
            // likely a crank-based meter (e.g. Stages)
            getCadence(ds);
        }

    } else if (c.uuid() == QBluetoothUuid(QBluetoothUuid::CSCMeasurement)) {

        quint8 flags;
        ds >> flags;

        if (flags & 0x1) { // Wheel Revolution Data Present
            getWheelRpm(ds);
        }

        if (flags & 0x2) { // Crank Revolution Data Present
          getCadence(ds);
        }
    }
}

void
BT40Device::serviceError(QLowEnergyService::ServiceError e)
{
    switch (e) {
    case QLowEnergyService::DescriptorWriteError:
        {
            qWarning() << "Failed to enable BTLE notifications" << "for device" << m_currentDevice.name();;
        }
        break;

    default:
        {
            qWarning() << "BTLE service error" << e << "for device" << m_currentDevice.name();;
        }
        break;
    }
}

void
BT40Device::confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    if (d.isValid() && value == QByteArray("0000")) {
        qWarning() << "disabled BTLE notifications" << "for device" << m_currentDevice.name();;
        this->disconnectDevice();
    }
}

void
BT40Device::getCadence(QDataStream& ds)
{
    quint16 cur_revs;
    quint16 cur_time;
    ds >> cur_revs;
    ds >> cur_time;

    // figure wether to update cadence and with what value
    //
    // If we have a new crank event (new time) we push a new RPM, but
    // only if the previous data is valid (fixes glitch on first
    // update)
    //
    // If we don't have new crank data, push a zero for RPM, unless
    // previous data is only 1 or 2 notifications old. This lets us
    // report RPMs lower than 60 (assuming notification period is 1s)
    // but still report a zero fairly quickly (2 notification periods)
    if (cur_time != prevCrankTime) {

        if (prevCrankStaleness >= 0) {

            const int time = cur_time + (cur_time < prevCrankTime ? 0x10000:0) - prevCrankTime;
            const int revs = cur_revs + (cur_revs < prevCrankRevs ? 0x10000:0) - prevCrankRevs;
            const double rpm = 1024*60*revs / time;
            dynamic_cast<BT40Controller*>(parent)->setCadence(rpm);
        }

    } else if (prevCrankStaleness < 0 || prevCrankStaleness >= 2) {
      dynamic_cast<BT40Controller*>(parent)->setCadence(0.0);

    }

    // update the staleness of the previous crank data
    if (cur_time != prevCrankTime) {
        prevCrankStaleness = 0;
    } else if (prevCrankStaleness < 2) {
        prevCrankStaleness += 1;
    }

    // update the previous crank data
    prevCrankRevs = cur_revs;
    prevCrankTime = cur_time;
}

void
BT40Device::getWheelRpm(QDataStream& ds)
{
    quint32 wheelrevs;
    quint16 wheeltime;
    ds >> wheelrevs;
    ds >> wheeltime;

    double rpm = 0.0;
    quint16 time = wheeltime - prevWheelTime;
    quint32 revs = wheelrevs - prevWheelRevs;
    if (time) rpm = 1024*60*revs / time;

    prevWheelRevs = wheelrevs;
    prevWheelTime = wheeltime;
    dynamic_cast<BT40Controller*>(parent)->setWheelRpm(rpm);
}
