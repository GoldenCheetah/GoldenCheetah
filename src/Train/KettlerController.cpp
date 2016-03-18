/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "KettlerController.h"
#include "Kettler.h"
#include "RealtimeData.h"

#include <QMessageBox>
#include <QSerialPort>

KettlerController::KettlerController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    m_kettler = new Kettler(this, dc ? dc->portSpec : "");
}

bool KettlerController::find()
{
    return false;
}

int
KettlerController::start()
{
    return m_kettler->start();
}

int
KettlerController::restart()
{
    return m_kettler->restart();
}

int
KettlerController::pause()
{
    return 0;
}

int
KettlerController::stop()
{
    return m_kettler->stop();
}

bool
KettlerController::discover(QString name)
{
   return m_kettler->discover(name);
}


bool KettlerController::doesPush() { return false; }
bool KettlerController::doesPull() { return true; }
bool KettlerController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
KettlerController::getRealtimeData(RealtimeData &rtData)
{
    if (!m_kettler->isConnected())
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Kettler"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(0);
        return;
    }

    rtData.setWatts(m_kettler->power());
    rtData.setHr(m_kettler->heartRate());
    rtData.setCadence(m_kettler->cadence());
    rtData.setSpeed(m_kettler->speed());
}

void KettlerController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void KettlerController::setLoad(double load)
{
    m_kettler->setLoad(load);
}
