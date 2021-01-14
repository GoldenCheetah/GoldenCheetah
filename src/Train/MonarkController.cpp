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
#include "MonarkConnection.h"
#include "RealtimeData.h"

#include <QMessageBox>
#include <QSerialPort>

MonarkController::MonarkController(TrainSidebar *parent,  DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    m_monark = new MonarkConnection();
    m_monark->setSerialPort(dc ? dc->portSpec : "");
}

bool MonarkController::find()
{
    return false;
}

int
MonarkController::start()
{
    m_monark->start();
    return 0;
}


int
MonarkController::restart()
{
    return 0;
}


int
MonarkController::pause()
{
    return 0;
}


int
MonarkController::stop()
{
    return 0;
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
    if (m_monark->isFinished())
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Monark"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(0);
        parent->Disconnect();
        return;
    }

    rtData.setWatts(m_monark->power());
    rtData.setHr(m_monark->pulse());
    rtData.setCadence(m_monark->cadence());
}

void MonarkController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void MonarkController::setLoad(double load)
{
    m_monark->setLoad(load);
}

void MonarkController::setGradientWithSimState(double, double targetForce_N, double simSpeedKph)
{
    // Set Kp second to keep monark in Kp mode.
    static const double s_KilopondsPerNewton = 9.80665;                // acceleration due to gravity
    m_monark->setKp(s_KilopondsPerNewton * targetForce_N);

    // Some monarks dont support kp so set load also.
    // Do not change device mode.
    m_monark->setLoad(simSpeedKph * targetForce_N * (1 / 3.6), false); // speed and force to watts
}
