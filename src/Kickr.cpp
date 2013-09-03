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

#include "Kickr.h"

Kickr::Kickr(QObject *parent,  DeviceConfiguration *devConf) : QThread(parent)
{
    this->parent = parent;
    this->devConf = devConf;    
    scanned = false;
    mode = -1;
    load = 100;
    slope = 1.0;

    connect(WFApi::getInstance(), SIGNAL(discoveredDevices(int,bool)), this, SLOT(discoveredDevices(int,bool)));
}

Kickr::~Kickr()
{
}

// not required
void Kickr::setDevice(QString) { }

void Kickr::setMode(int mode, double load, double gradient)
{
    pvars.lock();
    this->mode = mode;
    this->load = load;
    this->slope = gradient;
    pvars.unlock();
}

void Kickr::setLoad(double load)
{
    pvars.lock();
    if (load > 1500) load = 1500;
    if (load < 50) load = 50;
    this->load = load;
    pvars.unlock();
}

void Kickr::setGradient(double gradient)
{
    pvars.lock();
    this->slope = gradient;
    pvars.unlock();
}


int Kickr::getMode()
{
    int  tmp;
    pvars.lock();
    tmp = mode;
    pvars.unlock();
    return tmp;
}

double Kickr::getLoad()
{
    double tmp;
    pvars.lock();
    tmp = load;
    pvars.unlock();
    return tmp;
}

double Kickr::getGradient()
{
    double tmp;
    pvars.lock();
    tmp = slope;
    pvars.unlock();
    return tmp;
}

void
Kickr::getRealtimeData(RealtimeData &rtData)
{
    pvars.lock();
    rtData = rt;
    pvars.unlock();
}

int
Kickr::start()
{
    QThread::start();
    return 0;
}

// does nothing - neither does pause
int Kickr::restart() { return 0; }
int Kickr::pause() { return 0; }

int Kickr::stop()
{
    running = false;
    return 0;
}

// used by thread to set variables and emit event if needed
// on unexpected exit
int Kickr::quit(int code)
{
    // event code goes here!
    exit(code);
    return 0; // never gets here obviously but shuts up the compiler!
}

/*----------------------------------------------------------------------
 * MAIN THREAD - READS TELEMETRY AND UPDATES LOAD/GRADIENT ON KICKR
 *----------------------------------------------------------------------*/
void Kickr::run()
{
    int currentmode = -1;
    int currentload = -1;
    double currentslope= -1;

    // Connect to the device
    if (connectKickr()) {
        quit(2);
        return; // open failed!
    }

    int connectionloops =0;

    running = true;
    while(running) {

        // only get busy if we're actually connected
        if (WFApi::getInstance()->isConnected(sd)) {

            connectionloops = 0;

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

            // get telemetry
            if (WFApi::getInstance()->hasData(sd)) {
                pvars.lock();
                WFApi::getInstance()->getRealtimeData(sd, &rt);

                // set speed from wheelRpm and configured wheelsize
                double x = rt.getWheelRpm();
                if (devConf) rt.setSpeed(x * devConf->wheelSize / 1000 * 60 / 1000);
                else rt.setSpeed(x * 2.10 * 60 / 1000);
                pvars.unlock();
            }

        } else {
            // 100ms in each loop, 30secs is 300 loops
            if (++connectionloops > 300) {
                // give up waiting for connection
                quit(-1);
            }
        }
        // lets not hog cpu
        msleep(100);
    }

    disconnectKickr();
    quit(0);
}

void
Kickr::discoveredDevices(int n, bool finished)
{
    WFApi *w = WFApi::getInstance();

    // need to emit signal with device uuid and type
    // this is used by the add device wizard.
    // but only emit as they are found, not at the end
    // when search times out -- we want them as they
    // arrive.
    if (!finished && w->deviceSubType(n-1) == WFApi::WF_SENSOR_SUBTYPE_BIKE_POWER_KICKR) { 
        qDebug()<<"KIKCR? discovered a device.."
                <<w->deviceUUID(n-1)
                <<w->deviceType(n-1);
        emit foundDevice(w->deviceUUID(n-1), w->deviceType(n-1));
    }
}

bool
Kickr::find()
{
    WFApi *w = WFApi::getInstance();

    if (w->discoverDevicesOfType(WFApi::WF_SENSORTYPE_BIKE_POWER) == false) return false;

    QEventLoop loop;
    connect(w, SIGNAL(discoveredDevices(int,bool)), &loop, SLOT(quit()));
    loop.exec();

    scanned = true;

    if (w->deviceCount()) {
        deviceUUID = w->deviceUUID(0);
        return true;
    } else return false;
}

int
Kickr::connectKickr()
{
    // is BTLE even enabled?
    if (WFApi::getInstance()->isBTLEEnabled() == false) return -1;

    // get a pool for this thread
    pool = WFApi::getInstance()->getPool();

    // kick off connection and assume it is gonna be ok
    sd = WFApi::getInstance()->connectDevice(devConf->portSpec);
    return 0;
}

int
Kickr::disconnectKickr()
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
bool Kickr::discover(QString)
{
    return false;
}
