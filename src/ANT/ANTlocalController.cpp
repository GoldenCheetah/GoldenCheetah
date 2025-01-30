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
#include "GarminServiceHelper.h"

ANTlocalController::ANTlocalController(TrainSidebar *parent, DeviceConfiguration *dc) : RealtimeController(parent, dc)
{
    // for Device Pairing the controller is called with parent = NULL
    QString cyclist;
    QString athletePath;

    if (parent) {
        cyclist = parent->context->athlete->cyclist;
        athletePath = parent->context->athlete->home->root().canonicalPath();
    } else {
        cyclist = QString();
        athletePath = QDir::tempPath();
    }

    myANTlocal = new ANT (parent, dc, cyclist);
    logger = new ANTLogger(this, athletePath);

    connect(myANTlocal, SIGNAL(foundDevice(int,int,int)), this, SIGNAL(foundDevice(int,int,int)));
    connect(myANTlocal, SIGNAL(lostDevice(int)), this, SIGNAL(lostDevice(int)));
    connect(myANTlocal, SIGNAL(searchTimeout(int)), this, SIGNAL(searchTimeout(int)));

    // collecting R-R HRV data?
    connect(myANTlocal, SIGNAL(rrData(uint16_t, uint8_t, uint8_t)), this, SIGNAL(rrData(uint16_t, uint8_t, uint8_t)));

    connect(myANTlocal, SIGNAL(posData(uint8_t)), this, SIGNAL(posData(uint8_t)));
    connect(myANTlocal, SIGNAL(tcoreData(float, float, float, int)), this, SIGNAL(tcoreData(float, float, float, int)));

    // connect slot receiving ANT remote control commands & translating to native
    connect(myANTlocal, SIGNAL(antRemoteControl(uint16_t)), this, SLOT(antRemoteControl(uint16_t)));

    // Connect a logger
    connect(myANTlocal, SIGNAL(receivedAntMessage(const unsigned char, const ANTMessage ,const timeval )), logger, SLOT(logRawAntMessage(const unsigned char, const ANTMessage ,const timeval)));
    connect(myANTlocal, SIGNAL(sentAntMessage(const unsigned char, const ANTMessage ,const timeval )), logger, SLOT(logRawAntMessage(const unsigned char, const ANTMessage ,const timeval)));
}

void
ANTlocalController::setDevice(QString device)
{
    myANTlocal->setDevice(device);
}

int
ANTlocalController::start()
{
    // Before we do the low level device opening, check for an active Garmin Service
    if (GarminServiceHelper::isServiceRunning())
    {
        // HACK the event loop gets very angry when we open a dialog in response to a UI event
        QApplication::processEvents();
        int dialogStatus = QMessageBox::warning(parent, tr("Found Garmin Express service"),
                                                tr("The Garmin Express service is active on your computer.\n"
                                                   "This can block GoldenCheetah from accessing your ANT+ stick.\n\n"
                                                   "Do you want to temporarily disable the service?"),
                                                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);

        if (dialogStatus == QMessageBox::Ok)
        {
            if (!GarminServiceHelper::stopService())
            {
                // we failed to stop the service, tell the user
                QMessageBox::critical(parent, tr("Failed stopping Garmin Express service"),
                                      tr("GoldenCheetah failed to stop the Garmin Express service.\n"
                                         "It might be necessary to manually disable it."));
            }
        }
    }

    logger->open();
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
    logger->close();
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
bool ANTlocalController::doesLoad() { return true; }

void
ANTlocalController::setLoad(double x) {
    myANTlocal->setLoad(x);
}

void
ANTlocalController::setGradient(double x) {
    myANTlocal->setGradient(x);
}

void
ANTlocalController::setMode(int mode) {
    myANTlocal->setMode(mode);
}

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
        msgBox.setText(tr("Cannot open ANT+ device"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(1);
        parent->Disconnect();
        logger->close();
        return;
    }
    // get latest telemetry
    myANTlocal->getRealtimeData(rtData);
    processRealtimeData(rtData);
}

uint8_t
ANTlocalController::getCalibrationType()
{
    return myANTlocal->getCalibrationType();
}

double
ANTlocalController::getCalibrationTargetSpeed()
{
    return myANTlocal->getCalibrationTargetSpeed();
}

uint8_t
ANTlocalController::getCalibrationState()
{
    return myANTlocal->getCalibrationState();
}

uint16_t
ANTlocalController::getCalibrationSpindownTime()
{
    return myANTlocal->getCalibrationSpindownTime();
}

uint16_t
ANTlocalController::getCalibrationZeroOffset()
{
    return myANTlocal->getCalibrationZeroOffset();
}

uint16_t
ANTlocalController::getCalibrationSlope()
{
    return myANTlocal->getCalibrationSlope();
}

void
ANTlocalController::setCalibrationState(uint8_t state)
{
    myANTlocal->setCalibrationState(state);
}

void
ANTlocalController::resetCalibrationState()
{
    myANTlocal->resetCalibrationState();
}


void ANTlocalController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

void ANTlocalController::antRemoteControl(uint16_t command)
{
    //qDebug() <<"AntlocalController::antRemoteControl()" << command;

    // When called from pairing dialog the parent is null, do nothing..
    if (parent) {
        // translate the ANT control code to a native command code
        uint16_t nativeCmd = parent->remote->getNativeCmdId(command);

        // pass native command code to train view
        emit remoteControl(nativeCmd);
    }
}
