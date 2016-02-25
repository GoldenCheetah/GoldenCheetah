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
#include "BT40Controller.h"
#include "RealtimeData.h"

BT40Controller::BT40Controller(TrainSidebar *parent, DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myBT40 = new BT40(parent, dc);
    connect(myBT40, SIGNAL(foundDevice(QString,int)), this, SIGNAL(foundDevice(QString,int)));
}

BT40Controller::~BT40Controller()
{
    myBT40->stop();
    myBT40->deleteLater();
}

void
BT40Controller::setDevice(QString)
{
    // not required
}

int
BT40Controller::start()
{
    myBT40->start();
    return 0;
}


int
BT40Controller::restart()
{
    return myBT40->restart();
}


int
BT40Controller::pause()
{
    return myBT40->pause();
}


int
BT40Controller::stop()
{
    return myBT40->stop();
}

bool
BT40Controller::find()
{
    return myBT40->find();
}

bool
BT40Controller::discover(QString name)
{
    return myBT40->discover(name);
}


bool BT40Controller::doesPush() { return false; }
bool BT40Controller::doesPull() { return true; }
bool BT40Controller::doesLoad() { return true; }

/*
 * gets called from the GUI to get updated telemetry.
 * so whilst we are at it we check button status too and
 * act accordingly.
 *
 */
void
BT40Controller::getRealtimeData(RealtimeData &rtData)
{
    if(!myBT40->isRunning())
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Cannot Connect to BT40"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(1);
        return;
    }
    // get latest telemetry
    myBT40->getRealtimeData(rtData);
    processRealtimeData(rtData);
}

void BT40Controller::pushRealtimeData(RealtimeData &) { } // update realtime data with current values
