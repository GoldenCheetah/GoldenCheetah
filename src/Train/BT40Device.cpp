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

QList<QBluetoothUuid> BT40Device::supportedServiceUuids = QList<QBluetoothUuid>();

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
