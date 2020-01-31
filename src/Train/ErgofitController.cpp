/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2020 Manfred Mayer (mayerflash@gmx.de)
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

#include "ErgofitController.h"
#include "Ergofit.h"
#include "RealtimeData.h"

#include <QMessageBox>
#include <QSerialPort>

ErgofitController::ErgofitController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    m_ergofit = new Ergofit(this, dc ? dc->portSpec : "");
}

bool ErgofitController::find()
{
    return false;
}

int
ErgofitController::start()
{
    return m_ergofit->start();
}

int
ErgofitController::restart()
{
    return m_ergofit->restart();
}

int
ErgofitController::pause()
{
    return 0;
}

int
ErgofitController::stop()
{
    return m_ergofit->stop();
}

bool
ErgofitController::discover(QString name)
{
   return m_ergofit->discover(name);
}


bool ErgofitController::doesPush() { return false; }
bool ErgofitController::doesPull() { return true; }
bool ErgofitController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
ErgofitController::getRealtimeData(RealtimeData &rtData)
{
    if (!m_ergofit->isConnected())
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Ergofit"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(0);
        return;
    }

    rtData.setWatts(m_ergofit->power());
    rtData.setHr(m_ergofit->heartRate());
    rtData.setCadence(m_ergofit->cadence());
}

void ErgofitController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void ErgofitController::setLoad(double load)
{
    m_ergofit->setLoad(load);
}
