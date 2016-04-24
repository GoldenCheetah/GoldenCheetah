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

QList<QBluetoothUuid> BT40Device::supportedServiceUuids = QList<QBluetoothUuid>()
    << QBluetoothUuid(QBluetoothUuid::HeartRate)
       << QBluetoothUuid(QBluetoothUuid::CyclingPower)
	  << QBluetoothUuid(QBluetoothUuid::CyclingSpeedAndCadence);

BT40Device::BT40Device(QObject *parent, QBluetoothDeviceInfo devinfo) : parent(parent), m_currentDevice(devinfo)

{
    m_control = new QLowEnergyController(m_currentDevice, this);
    connect(m_control, SIGNAL(connected()),
	    this, SLOT(deviceConnected()));
    connect(m_control, SIGNAL(error(QLowEnergyController::Error)),
	    this, SLOT(controllerError(QLowEnergyController::Error)));
    connect(m_control, SIGNAL(disconnected()),
	    this, SLOT(deviceDisconnected()));
    connect(m_control, SIGNAL(serviceDiscovered(QBluetoothUuid)),
	    this, SLOT(serviceDiscovered(QBluetoothUuid)));
    connect(m_control, SIGNAL(discoveryFinished()),
	    this, SLOT(serviceScanDone()));
    prevWheelTime = 0;
    prevWheelRevs = 0;
    prevCrankTime = 0;
    prevCrankRevs = 0;
}

BT40Device::~BT40Device() {
    this->disconnectDevice();
    delete m_control;
}

void BT40Device::connectDevice() {
    qDebug() << "Connecting to device" << m_currentDevice.name();
    m_control->setRemoteAddressType(QLowEnergyController::RandomAddress);
    m_control->connectToDevice();
}

void BT40Device::disconnectDevice() {
    qDebug() << "Disconnecting from device" << m_currentDevice.name();
    m_control->disconnectFromDevice();
}

void BT40Device::deviceConnected() {
    qDebug() << "Connected to device" << m_currentDevice.name();
    m_control->discoverServices();
}

void BT40Device::controllerError(QLowEnergyController::Error error)
{
    qWarning() << "Controller Error:" << error << "for device" << m_currentDevice.name();
}

void BT40Device::deviceDisconnected() {
    qDebug() << "Lost connection to" << m_currentDevice.name();
}

void BT40Device::serviceDiscovered(QBluetoothUuid uuid) {
    if (supportedServiceUuids.contains(uuid)) {
	qDebug() << "Discovering details for service" << uuid << "for device" << m_currentDevice.name();
	QLowEnergyService *service = m_control->createServiceObject(uuid, this);
	connect(service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)),
		this, SLOT(serviceStateChanged(QLowEnergyService::ServiceState)));
	connect(service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
		this, SLOT(updateValue(QLowEnergyCharacteristic,QByteArray)));
	connect(service, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)),
		this, SLOT(confirmedDescriptorWrite(QLowEnergyDescriptor,QByteArray)));
	connect(service, SIGNAL(error(QLowEnergyService::ServiceError)),
		this, SLOT(serviceError(QLowEnergyService::ServiceError)));
	service->discoverDetails();
	m_services.append(service);
    }
}

void BT40Device::serviceScanDone()
{
    qDebug() << "Service scan done" << "for device" << m_currentDevice.name();
}

void BT40Device::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    qDebug() << "service state changed" << s << "for device" << m_currentDevice.name();
    if (s == QLowEnergyService::ServiceDiscovered) {
       foreach (QLowEnergyService* const &service, m_services) {
           if (service->state() == s) {
               QLowEnergyCharacteristic characteristic;
	       if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::HeartRate)) {
		   characteristic = service->characteristic(
		       QBluetoothUuid(QBluetoothUuid::HeartRateMeasurement));
	       }
	       else if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::CyclingPower)) {
		   characteristic = service->characteristic(
		       QBluetoothUuid(QBluetoothUuid::CyclingPowerMeasurement));
	       }
	       else if (service->serviceUuid() == QBluetoothUuid(QBluetoothUuid::CyclingSpeedAndCadence)) {
		   characteristic = service->characteristic(
		       QBluetoothUuid(QBluetoothUuid::CSCMeasurement));
	       }
               if (characteristic.isValid()) {
                   const QLowEnergyDescriptor notificationDesc = characteristic.descriptor(
                       QBluetoothUuid::ClientCharacteristicConfiguration);
                   if (notificationDesc.isValid()) {
                       service->writeDescriptor(notificationDesc,
                                                QByteArray::fromHex("0100"));
                   }
               }
           }
       }
    }
}

void BT40Device::updateValue(const QLowEnergyCharacteristic &c,
			     const QByteArray &value)
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
	}
	else {
	    quint8 heartRate;
	    ds >> heartRate;
	    hr = (float) heartRate;
	}
	dynamic_cast<BT40Controller*>(parent)->setBPM(hr);
    }
    else if (c.uuid() == QBluetoothUuid(QBluetoothUuid::CyclingPowerMeasurement)) {
	quint16 flags;
	ds >> flags;
	qint16 tmp_pwr;
	ds >> tmp_pwr;
	double power = (double) tmp_pwr;
	dynamic_cast<BT40Controller*>(parent)->setWatts(power);
    }
    else if (c.uuid() == QBluetoothUuid(QBluetoothUuid::CSCMeasurement)) {
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

void BT40Device::serviceError(QLowEnergyService::ServiceError e)
{
    switch (e) {
    case QLowEnergyService::DescriptorWriteError:
    {
	qWarning() << "Failed to enable BTLE notifications" << "for device" << m_currentDevice.name();;
	break;
    }
    default:
    {
	qWarning() << "BTLE service error" << e << "for device" << m_currentDevice.name();;
    }
    }
}

void BT40Device::confirmedDescriptorWrite(const QLowEnergyDescriptor &d,
					  const QByteArray &value)
{
    if (d.isValid() && value == QByteArray("0000")) {
	qWarning() << "disabled BTLE notifications" << "for device" << m_currentDevice.name();;
	this->disconnectDevice();
    }
}

void BT40Device::getCadence(QDataStream& ds) {
    quint16 cur_revs;
    quint16 cur_time;
    ds >> cur_revs;
    ds >> cur_time;
    double rpm = 0.0;
    quint16 time = cur_time - prevCrankTime;
    quint16 revs = cur_revs - prevCrankRevs;
    if (time) {
	rpm = 1024*60*revs / time;
    }
    prevCrankRevs = cur_revs;
    prevCrankTime = cur_time;
    dynamic_cast<BT40Controller*>(parent)->setCadence(rpm);
}

void BT40Device::getWheelRpm(QDataStream& ds) {
    quint32 wheelrevs;
    quint16 wheeltime;
    ds >> wheelrevs;
    ds >> wheeltime;
    double rpm = 0.0;
    quint16 time = wheeltime - prevWheelTime;
    quint32 revs = wheelrevs - prevWheelRevs;
    if (time) {
	rpm = 1024*60*revs / time;
    }
    prevWheelRevs = wheelrevs;
    prevWheelTime = wheeltime;
    dynamic_cast<BT40Controller*>(parent)->setWheelRpm(rpm);
}
