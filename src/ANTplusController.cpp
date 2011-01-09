/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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
#include "ANTplusController.h"
#include "QuarqdClient.h"
#include "RealtimeData.h"

ANTplusController::ANTplusController(RealtimeWindow *parent, DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myANTplus = new QuarqdClient (parent, dc);
}


int
ANTplusController::start()
{
    myANTplus->start();
    return 0;
}


int
ANTplusController::restart()
{
    return myANTplus->restart();
}


int
ANTplusController::pause()
{
    return myANTplus->pause();
}


int
ANTplusController::stop()
{
    return myANTplus->stop();
}


bool
ANTplusController::discover(DeviceConfiguration *dc, QProgressDialog *progress)
{
    return myANTplus->discover(dc, progress);
}


bool ANTplusController::doesPush() { return false; }
bool ANTplusController::doesPull() { return true; }
bool ANTplusController::doesLoad() { return false; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
ANTplusController::getRealtimeData(RealtimeData &rtData)
{
    if(!myANTplus->isRunning())
    {
        QMessageBox msgBox;
        msgBox.setText("Cannot Connect to Quarqd");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(1);
        return;
    }
    // get latest telemetry
    rtData = myANTplus->getRealtimeData();
    processRealtimeData(rtData);
}

void ANTplusController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values
