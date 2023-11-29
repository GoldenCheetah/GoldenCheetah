/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#include <QProgressDialog>
#include "BT40Controller.h"
#include "RealtimeData.h"

BT40Controller::BT40Controller(TrainSidebar *parent, DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    localDevice = new QBluetoothLocalDevice(this);
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    localDc = dc;
    load = 0;
    gradient = 0;
    mode = RT_MODE_SLOPE;
    windSpeed = 0;
    weight = 80;
    rollingResistance = 0.0033;
    windResistance = 0.6;
    wheelSize = 2100;

    if (localDc && !localDc->deviceProfile.isEmpty())
    {
        foreach (QString deviceInfoString, localDc->deviceProfile.split(","))
        {
            allowedDevices.append(DeviceInfo(deviceInfoString));
        }
    }

    connect(discoveryAgent, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)),
	    this, SLOT(addDevice(const QBluetoothDeviceInfo&)));
    connect(discoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),
	    this, SLOT(deviceScanError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(discoveryAgent, SIGNAL(finished()), this, SLOT(scanFinished()));
}

BT40Controller::~BT40Controller()
{
    delete localDevice;
    delete discoveryAgent;
}

void
BT40Controller::setDevice(QString)
{
    // not required
}

QList<QBluetoothDeviceInfo>
BT40Controller::getDeviceInfo()
{
    QList<QBluetoothDeviceInfo> deviceInfo;
    foreach(BT40Device* dev, devices)
    {
        deviceInfo.append(dev->deviceInfo());
    }

    return deviceInfo;
}

int
BT40Controller::start()
{
    if (localDevice->isValid()) {
        discoveryAgent->start();
    }
    return 0;
}


int
BT40Controller::restart()
{
    return 0;
}


int
BT40Controller::pause()
{
    return 0;
}


int
BT40Controller::stop()
{
    foreach (BT40Device* const &device, devices) {
        device->disconnectDevice();
        delete device;
    }
    devices.clear();
    return 0;
}

bool
BT40Controller::find()
{
    return localDevice->isValid();
}

bool
BT40Controller::discover(QString)
{
    return true;
}


bool BT40Controller::doesPush() { return false; }
bool BT40Controller::doesPull() { return true; }
bool BT40Controller::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
BT40Controller::getRealtimeData(RealtimeData &rtData)
{
    rtData = telemetry;
    processRealtimeData(rtData);
}

void BT40Controller::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void
BT40Controller::addDevice(const QBluetoothDeviceInfo &info)
{
    if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        // Check if device is already created for this uuid/address
        // At least on MacOS the deviceDiscovered signal can/will be sent multiple times
        // for the same device during discovery.
        foreach(BT40Device* dev, devices)
        {
            if (info.address().isNull())
            {
                // On MacOS there's no address, so check deviceUuid
                if (dev->deviceInfo().deviceUuid() == info.deviceUuid())
                {
                    return;
                }
            } else {
                if (dev->deviceInfo().address() == info.address())
                {
                    return;
                }
            }
        }

        // Check for device configuration and only
        // connect to configured sensors.
        //
        // We can still connect to all available devices
        // is the device profile is empty
        foreach (const DeviceInfo deviceInfo, allowedDevices)
        {
            if (info.address().isNull())
            {
                // macOS
                if (info.deviceUuid().toString() != deviceInfo.getUuid())
                {
                    return;
                }
            }
            else
            {
                if (info.address().toString() != deviceInfo.getAddress())
                {
                    return;
                }
            }
        }

        BT40Device* dev = new BT40Device(this, info);
        devices.append(dev);

        // Only connect to device if we really want
        // to use them for a workout
        if(localDc)
        {
            // When start() is called, it initiates the device scan and returns immediately.
            // Then, commands like setWeight() may come before any device is discovered.
            // In that case, the weight is stored but is sent to an empty list of devices.
            // However, when devices are added, the stored parameters are sent.
            dev->setWheelCircumference(wheelSize);
            dev->setRollingResistance(rollingResistance);
            dev->setWindResistance(windResistance);
            dev->setWeight(weight);
            dev->setWindSpeed(windSpeed);
            dev->setMode(mode);
            if (mode == RT_MODE_ERGO) dev->setLoad(load);
            else dev->setGradient(gradient);

            dev->connectDevice();
            connect(dev, &BT40Device::setNotification, this, &BT40Controller::setNotification);
        }
    }
}

void
BT40Controller::scanFinished()
{    
    emit setNotification(tr("Bluetooth scan finished"), 2);
    emit scanFinished(devices.count() > 0);
    qDebug() << "BT scan finished";
}


void
BT40Controller::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qWarning() << "Error while scanning BT devices:" << error;
}

uint8_t
BT40Controller::getCalibrationType() {
    for (auto* dev : devices) {
        uint8_t caltype = dev->getCalibrationType();
        if (caltype != CALIBRATION_TYPE_NOT_SUPPORTED) {
            return caltype;
        }
    }
    return CALIBRATION_TYPE_NOT_SUPPORTED;
}

uint8_t
BT40Controller::getCalibrationState() {
    for (auto* dev : devices) {
        uint8_t caltype = dev->getCalibrationType();
        if (caltype != CALIBRATION_TYPE_NOT_SUPPORTED) {
            return dev->getCalibrationState();
        }
    }
    return CALIBRATION_STATE_IDLE;
}

double
BT40Controller::getCalibrationTargetSpeed() {
    for (auto* dev : devices) {
        uint8_t caltype = dev->getCalibrationType();
        if (caltype != CALIBRATION_TYPE_NOT_SUPPORTED) {
            return dev->getCalibrationTargetSpeed();
        }
    }
    return 0;
}

uint16_t
BT40Controller::getCalibrationSpindownTime() {
    for (auto* dev : devices) {
        uint8_t caltype = dev->getCalibrationType();
        if (caltype != CALIBRATION_TYPE_NOT_SUPPORTED) {
            return dev->getCalibrationSpindownTime();
        }
    }
    return 0;
}

uint16_t
BT40Controller::getCalibrationZeroOffset() {
    for (auto* dev : devices) {
        uint8_t caltype = dev->getCalibrationType();
        if (caltype != CALIBRATION_TYPE_NOT_SUPPORTED) {
            return dev->getCalibrationZeroOffset();
        }
    }
    return 0;
}

uint16_t
BT40Controller::getCalibrationSlope() {
    for (auto* dev : devices) {
        uint8_t caltype = dev->getCalibrationType();
        if (caltype != CALIBRATION_TYPE_NOT_SUPPORTED) {
            return dev->getCalibrationSlope();
        }
    }
    return 0;
}

void
BT40Controller::setWheelRpm(double wrpm) {
    telemetry.setWheelRpm(wrpm, true); // record time sample for new rpm data
    telemetry.setSpeed(wrpm * wheelSize / 1000.0 * 60.0 / 1000.0);
}

void BT40Controller::setLoad(double l)
{
  load = l;
  for (auto* dev: devices) {
    dev->setLoad(l);
  }
}

void BT40Controller::setGradient(double g) 
{
  gradient = g;
  for (auto* dev: devices) {
    dev->setGradient(g);
  }
}

void BT40Controller::setMode(int m)
{
  mode = m;
  for (auto* dev: devices) {
    dev->setMode(m);
  }
}

void BT40Controller::setWindSpeed(double s)
{
  windSpeed = s;
  for (auto* dev: devices) {
    dev->setWindSpeed(s);
  }
}

void BT40Controller::setWeight(double w)
{
  weight = w;
  for (auto* dev: devices) {
    dev->setWeight(w);
  }
}

void BT40Controller::setRollingResistance(double rr)
{
  rollingResistance = rr;
  for (auto* dev: devices) {
    dev->setRollingResistance(rr);
  }
}

void BT40Controller::setWindResistance(double wr)
{
  windResistance = wr;
  for (auto* dev: devices) {
    dev->setWindResistance(wr);
  }
}

void BT40Controller::setWheelCircumference(double wc)
{
  wheelSize = wc;
  for (auto* dev: devices) {
    dev->setWheelCircumference(wc);
  }
}


DeviceInfo::DeviceInfo(QString data)
{
    QStringList deviceInfo = data.split(";");
    if (deviceInfo.size() == 3)
    {
        name = deviceInfo[0];
        address = deviceInfo[1];
        uuid = deviceInfo[2];
    }
}

QString DeviceInfo::getName() const
{
    return name;
}

QString DeviceInfo::getUuid() const
{
    return uuid;
}

QString DeviceInfo::getAddress() const
{
    return address;
}
