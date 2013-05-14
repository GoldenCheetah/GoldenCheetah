/*
 * Copyright (c) 2009 Mark Rages
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
#include "ANTLogger.h"
#include "RealtimeData.h"

ANTlocalController::ANTlocalController(TrainTool *parent, DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    myANTlocal = new ANT (parent, dc);
    connect(myANTlocal, SIGNAL(foundDevice(int,int,int)), this, SIGNAL(foundDevice(int,int,int)));
    connect(myANTlocal, SIGNAL(lostDevice(int)), this, SIGNAL(lostDevice(int)));
    connect(myANTlocal, SIGNAL(searchTimeout(int)), this, SIGNAL(searchTimeout(int)));

    // Connect a logger
    connect(myANTlocal, SIGNAL(receivedAntMessage(const ANTMessage ,const timeval )), &logger, SLOT(logRawAntMessage(const ANTMessage ,const timeval)));
}

void
ANTlocalController::setDevice(QString device)
{
    myANTlocal->setDevice(device);
}

int
ANTlocalController::start()
{
    logger.open();
    myANTlocal->start();
    myANTlocal->setup();
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
    int rc =  myANTlocal->stop();
    logger.close();
    return rc;
}

bool
ANTlocalController::find()
{
    return myANTlocal->find();
}

bool
ANTlocalController::discover(QString name)
{
    return myANTlocal->discover(name);
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
        logger.close();
        return;
    }
    // get latest telemetry
    myANTlocal->getRealtimeData(rtData);
    processRealtimeData(rtData);
}

void ANTlocalController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values
