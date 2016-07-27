/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2015 Erik Botö (erik.boto@gmail.com)
 * Copyright (c) 2016 Chang Spivey (chang.spivey@gmail.com)
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

#include "kdri.h"
#include "KettlerBluetoothController.h"
#include "RealtimeData.h"

#include <QMessageBox>
#include <QSerialPort>

KettlerBluetoothController::KettlerBluetoothController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    if(dc != NULL) {
        std::string utf8_text = dc->name.toUtf8().constData();
        this->devConf = *dc;
        this->devConfPtr = &this->devConf;
    } else {
        this->devConfPtr = NULL;
    }

    m_connection = NULL;
}

KettlerBluetoothController::~KettlerBluetoothController()
{
    if (m_connection) kdri_connection_close(m_connection);
    m_connection = NULL;
}

bool KettlerBluetoothController::ensureConnection(bool tryConnect) {
    if(isConnected()) return true;
    if(tryConnect && this->devConfPtr) { return connectToDevice(); }
    return false;
}

bool
KettlerBluetoothController::canDoInquiry() {
    return true;
}


QList<DeviceConfiguration>
KettlerBluetoothController::doInquiry() {
    if(m_connection != NULL) return QList<DeviceConfiguration>();

    auto deviceConfList = QList<DeviceConfiguration>();


    // auto deviceConf = DeviceConfiguration();
    // char macAddr[19] = "12:34:56:78:9A:BC";
    // deviceConf.name = QString((const char*) "RUN 7");
    // deviceConf.portSpec = QString("BT@%1").arg(macAddr);
    // for(int j = 0; j < 6; j++) deviceConf.mac[j] = ((uint8_t[]){0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC})[j];
    // deviceConfList.append(deviceConf);
    // return deviceConfList;

    KdriDevice devices[32] = { };
    int32_t result = kdri_scan_devices(devices, 32);
    if(result < 0) { printf("kdri_scan_devices(): Error code %d\n", result); return QList<DeviceConfiguration>(); }
    else if(result == 0) { printf("kdri_scan_devices(): No Kettler devices in range\n"); return QList<DeviceConfiguration>(); }


    for(int i = 0; i < result; i++) {
        auto deviceConf = DeviceConfiguration();
        char macAddr[19] = {};
        kdri_addr_to_str(&devices[i].addr, (uint8_t*)macAddr);
        deviceConf.name = QString((const char*) devices[i].name);
        deviceConf.portSpec = QString("BT@%1").arg(macAddr);
        for(int j = 0; j < 6; j++) deviceConf.mac[j] = devices[i].addr.b[j];
        deviceConfList.append(deviceConf);
    }

    return deviceConfList;
}

bool
KettlerBluetoothController::connectToDevice() {
    if(m_connection != NULL) return false;
    if(this->devConfPtr == NULL) return false;

    KdriAddr addr;
    for(int j = 0; j < 6; j++) addr.b[j] = this->devConfPtr->mac[j];
    m_connection = kdri_connect(&addr);
    return m_connection != NULL;
}

bool
KettlerBluetoothController::isConnected() {
    if(m_connection == NULL) return false;
    return true;
}

int
KettlerBluetoothController::start()
{
    if(!ensureConnection(true)) {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Kettler"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(0);
        return false;
    }
    return true;
}

int
KettlerBluetoothController::restart()
{
    if(isConnected()) { kdri_set_online(m_connection, true); }
    return 0;
}

int
KettlerBluetoothController::pause()
{
    if(isConnected()) { kdri_set_online(m_connection, false); }
    return 0;
}

int
KettlerBluetoothController::stop()
{
    if(isConnected()) { kdri_set_online(m_connection, false); }
    return 0;
}

bool KettlerBluetoothController::doesPush() { return false; }
bool KettlerBluetoothController::doesPull() { return true; }
bool KettlerBluetoothController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
KettlerBluetoothController::getRealtimeData(RealtimeData &rtData)
{
    if(!isConnected()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Kettler"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(0);
        return;
    }

    uint16_t v16;
    v16 = 0; if(kdri_get_speed(m_connection, &v16) == KdriOk) rtData.setSpeed(v16 / 10.0);
    v16 = 0; if(kdri_get_incline(m_connection, &v16) == KdriOk) rtData.setSlope(v16 / 10.0);
    v16 = 0; if(kdri_get_pulse(m_connection, &v16) == KdriOk) rtData.setHr(v16);

    // following statements are untested, because the right device was not available
    v16 = 0; if(kdri_get_rpm(m_connection, &v16) == KdriOk) rtData.setWheelRpm(v16);
    v16 = 0; if(kdri_get_energy(m_connection, &v16) == KdriOk) rtData.setWatts(v16);
}

void KettlerBluetoothController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void KettlerBluetoothController::setLoad(double load)
{
    //m_kettler->setLoad(load);
}

void KettlerBluetoothController::setMode(int mode) {
    if(!isConnected()) return;

    switch(mode) {
        case RT_MODE_ERGO: kdri_set_online(m_connection, true); break;
        default: break;
    }
}
