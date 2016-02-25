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

#include "BT40.h"

BT40::BT40(QObject *parent,  DeviceConfiguration *devConf) : QThread(parent)
{
    this->parent = parent;
    this->devConf = devConf;    
    scanned = false;
    mode = -1;
    load = 100;
    slope = 1.0;

    connect(WFApi::getInstance(), SIGNAL(discoveredDevices(int,bool)), this, SLOT(discoveredDevices(int,bool)));
}

BT40::~BT40()
{
    stop();
}

// not required
void BT40::setDevice(QString) { }

void BT40::setMode(int mode, double load, double gradient)
{
    pvars.lock();
    this->mode = mode;
    this->load = load;
    this->slope = gradient;
    pvars.unlock();
}

void BT40::setLoad(double load)
{
    pvars.lock();
    if (load > 1500) load = 1500;
    if (load < 50) load = 50;
    this->load = load;
    pvars.unlock();
}

void BT40::setGradient(double gradient)
{
    pvars.lock();
    this->slope = gradient;
    pvars.unlock();
}


int BT40::getMode()
{
    int  tmp;
    pvars.lock();
    tmp = mode;
    pvars.unlock();
    return tmp;
}

double BT40::getLoad()
{
    double tmp;
    pvars.lock();
    tmp = load;
    pvars.unlock();
    return tmp;
}

double BT40::getGradient()
{
    double tmp;
    pvars.lock();
    tmp = slope;
    pvars.unlock();
    return tmp;
}

void
BT40::getRealtimeData(RealtimeData &rtData)
{
    pvars.lock();
    rtData = rt;
    pvars.unlock();
}

int
BT40::start()
{
    QThread::start();
    return 0;
}

// does nothing - neither does pause
int BT40::restart() { return 0; }
int BT40::pause() { return 0; }

int BT40::stop()
{
    running = false;
    return 0;
}

// used by thread to set variables and emit event if needed
// on unexpected exit
int BT40::quit(int code)
{
    // event code goes here!
    exit(code);
    return 0; // never gets here obviously but shuts up the compiler!
}

/*----------------------------------------------------------------------
 * MAIN THREAD - READS TELEMETRY AND UPDATES LOAD/GRADIENT ON KICKR
 *----------------------------------------------------------------------*/
void BT40::run()
{
    int currentmode = -1;
    int currentload = -1;
    double currentslope= -1;

    // Connect to the device
    if (connectBT40()) {
        quit(2);
        return; // open failed!
    }

    running = true;
    while(running) {

        // only get busy if we're actually connected
        if (WFApi::getInstance()->isConnected(sd)) {

            // We ALWAYS set load for each loop. This is because
            // even though the device reports as connected we need
            // to wait before it really is. So we just keep on
            // sending the current mode/load. It doesn't cost us
            // anything since all devices are powered.

            // it does generate a few error messages though..
            // and the connection takes about 25 secs to get
            // up to speed.

            // set load - reset it if generated watts don't match .. 
            if (mode == RT_MODE_ERGO) {
                WFApi::getInstance()->setErgoMode(sd);
                WFApi::getInstance()->setLoad(sd, load);
                currentload = load;
                currentmode = mode;
            }

            // set slope
            if (mode == RT_MODE_SLOPE && currentslope) {
                WFApi::getInstance()->setSlopeMode(sd);
                WFApi::getInstance()->setSlope(sd, slope);
                currentslope = slope;
                currentmode = mode;
            }
        }

        if (WFApi::getInstance()->hasData(sd)) {
            pvars.lock();
            WFApi::getInstance()->getRealtimeData(sd, &rt);

            // set speed from wheelRpm and configured wheelsize
            double x = rt.getWheelRpm();
            if (devConf) rt.setSpeed(x * devConf->wheelSize / 1000 * 60 / 1000);
            else rt.setSpeed(x * 2.10 * 60 / 1000);
            pvars.unlock();
        }

        // lets not hog cpu
        msleep(100);
    }

    disconnectBT40();
    quit(0);
}

void
BT40::discoveredDevices(int n, bool finished)
{
    WFApi *w = WFApi::getInstance();

    // need to emit signal with device uuid and type
    // this is used by the add device wizard.
    // but only emit as they are found, not at the end
    // when search times out -- we want them as they
    // arrive.
    if (!finished && w->deviceSubType(n-1) != WFApi::WF_SENSOR_SUBTYPE_BIKE_POWER_KICKR) { 

        for(int i=0; i<n; i++) {
        qDebug()<<this<<"BT40 discovered a bluetooth device.."<<i
                <<w->deviceUUID(i)
                <<w->deviceType(i);
        emit foundDevice(w->deviceUUID(i), w->deviceType(i));
        }
    }
}

bool
BT40::find()
{
    WFApi *w = WFApi::getInstance();

    // do we even have BTLE hardware available?
    if (w->isBTLEEnabled() ==false) {

        // lets try and enable it
        // won't complete for a while but hopefully before next attempt to find is made
        // by user or device scanner in AddDeviceWizard
        w->enableBTLE(true, false);

        return false;
    }

    // can we kick off a search?
    if (w->discoverDevicesOfType(WFApi::WF_SENSORTYPE_NONE) == false) return false;

    // wait for it to return something, anything
    QEventLoop loop;
    connect(w, SIGNAL(discoverFinished()), &loop, SLOT(quit()));
    loop.exec();

    scanned = true;

    // what did we get?
    if (w->deviceCount()) {
        deviceUUID = w->deviceUUID(0);
        return true;
    } else return false;
}

int
BT40::connectBT40()
{
    // get a pool for this thread
    pool = WFApi::getInstance()->getPool();
    sd = WFApi::getInstance()->connectDevice(devConf->portSpec);
    return 0;
}

int
BT40::disconnectBT40()
{
    // disconnect
    WFApi::getInstance()->disconnectDevice(sd);
    connected = false;

    // clear that pool now we're done
    WFApi::getInstance()->freePool(pool);

    return 0;
}

// check to see of there is a port at the device specified
// returns true if the device exists and false if not
bool BT40::discover(QString)
{
    return false;
}
