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

#include "DaumController.h"
#include "Daum.h"
#include "RealtimeData.h"

#include <QMessageBox>

DaumController::DaumController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
    , daumDevice_(this, dc ? dc->portSpec : "", dc ? dc->deviceProfile : "") {
}

int DaumController::start() {
    return daumDevice_.start();
}

int DaumController::restart() {
    return daumDevice_.restart();
}

int DaumController::pause() {
    return daumDevice_.pause();
}

int DaumController::stop() {
    return daumDevice_.stop();
}

bool DaumController::discover(QString name) {
   return daumDevice_.discover(name);
}

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 */
void DaumController::getRealtimeData(RealtimeData &rtData) {
    if(!daumDevice_.isRunning()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Daum"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(1);
        return;
    }

    rtData.setWatts(daumDevice_.getPower());
    rtData.setHr(daumDevice_.getHeartRate());
    rtData.setCadence(daumDevice_.getCadence());
    rtData.setSpeed(daumDevice_.getSpeed());
}

void DaumController::setLoad(double load) {
    daumDevice_.setLoad(load);
}
