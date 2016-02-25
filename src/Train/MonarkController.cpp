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

#include "MonarkController.h"
#include "Monark.h"
#include "RealtimeData.h"

#include <QMessageBox>
#include <QSerialPort>

MonarkController::MonarkController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    m_monark = new Monark(this, dc ? dc->portSpec : "");
}

bool MonarkController::find()
{
    return false;
}

int
MonarkController::start()
{
    return m_monark->start();
}


int
MonarkController::restart()
{
    return m_monark->restart();
}


int
MonarkController::pause()
{
    return 0;
}


int
MonarkController::stop()
{
    return m_monark->stop();
}


bool
MonarkController::discover(QString name)
{
   return m_monark->discover(name);
}


bool MonarkController::doesPush() { return false; }
bool MonarkController::doesPull() { return true; }
bool MonarkController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
MonarkController::getRealtimeData(RealtimeData &rtData)
{
    if (!m_monark->isConnected())
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Monark"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(0);
        return;
    }

    rtData.setWatts(m_monark->power());
    rtData.setHr(m_monark->heartRate());
    rtData.setCadence(m_monark->cadence());
}

void MonarkController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void MonarkController::setLoad(double load)
{
    m_monark->setLoad(load);
}
