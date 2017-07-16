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

#include "KettlerRacerController.h"
#include "KettlerRacer.h"
#include "RealtimeData.h"

#include <QMessageBox>
#include <QSerialPort>

KettlerRacerController::KettlerRacerController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    m_kettlerRacer = new KettlerRacer(this, dc ? dc->portSpec : "");
}

bool KettlerRacerController::find()
{
    return false;
}

int
KettlerRacerController::start()
{
    return m_kettlerRacer->start();
}

int
KettlerRacerController::restart()
{
    return m_kettlerRacer->restart();
}

int
KettlerRacerController::pause()
{
    return 0;
}

int
KettlerRacerController::stop()
{
    return m_kettlerRacer->stop();
}

bool
KettlerRacerController::discover(QString name)
{
   return m_kettlerRacer->discover(name);
}


bool KettlerRacerController::doesPush() { return false; }
bool KettlerRacerController::doesPull() { return true; }
bool KettlerRacerController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
KettlerRacerController::getRealtimeData(RealtimeData &rtData)
{
    if (!m_kettlerRacer->isConnected())
    {
        //QMessageBox msgBox;
        //msgBox.setText(tr("Cannot Connect to KettlerRacer"));
        //msgBox.setIcon(QMessageBox::Critical);
        //msgBox.exec();
        //parent->Stop(0);
        //return;
    }

    rtData.setWatts(m_kettlerRacer->power());
    rtData.setHr(m_kettlerRacer->heartRate());
    rtData.setCadence(m_kettlerRacer->cadence());
    rtData.setSpeed(m_kettlerRacer->speed());
}

void KettlerRacerController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void KettlerRacerController::setLoad(double load)
{
    m_kettlerRacer->setLoad(load);
}
