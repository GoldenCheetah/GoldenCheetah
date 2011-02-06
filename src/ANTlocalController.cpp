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
#include "ANTlocalController.h"
#include "ANT.h"
#include "RealtimeData.h"

ANTlocalController::ANTlocalController(RealtimeWindow *parent, DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myANTlocal = new ANT (parent, dc);
}


int
ANTlocalController::start()
{
    myANTlocal->start();
    return 0;
}


int
ANTlocalController::restart()
{
    return myANTlocal->restart();
}


int
ANTlocalController::pause()
{
    return myANTlocal->pause();
}


int
ANTlocalController::stop()
{
    return myANTlocal->stop();
}


bool
ANTlocalController::discover(DeviceConfiguration *dc, QProgressDialog *progress)
{
    return myANTlocal->discover(dc, progress);
}


bool ANTlocalController::doesPush() { return false; }
bool ANTlocalController::doesPull() { return true; }
bool ANTlocalController::doesLoad() { return false; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
ANTlocalController::getRealtimeData(RealtimeData &rtData)
{
    if(!myANTlocal->isRunning())
    {
        QMessageBox msgBox;
        msgBox.setText("Cannot open ANT+ device");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(1);
        return;
    }
    // get latest telemetry
    rtData = myANTlocal->getRealtimeData();
    processRealtimeData(rtData);
}

void ANTlocalController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values
