/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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
#include "KickrController.h"
#include "RealtimeData.h"

KickrController::KickrController(TrainSidebar *parent, DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myKickr = new Kickr(parent, dc);
    connect(myKickr, SIGNAL(foundDevice(QString,int)), this, SIGNAL(foundDevice(QString,int)));
}

void
KickrController::setDevice(QString)
{
    // not required
}

int
KickrController::start()
{
    myKickr->start();
    return 0;
}


int
KickrController::restart()
{
    return myKickr->restart();
}


int
KickrController::pause()
{
    return myKickr->pause();
}


int
KickrController::stop()
{
    return myKickr->stop();
}

bool
KickrController::find()
{
    return myKickr->find();
}

bool
KickrController::discover(QString name)
{
    return myKickr->discover(name);
}


bool KickrController::doesPush() { return false; }
bool KickrController::doesPull() { return true; }
bool KickrController::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
KickrController::getRealtimeData(RealtimeData &rtData)
{
    if(!myKickr->isRunning())
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to Kickr"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(1);
        return;
    }
    // get latest telemetry
    myKickr->getRealtimeData(rtData);
    processRealtimeData(rtData);
}

void KickrController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values
