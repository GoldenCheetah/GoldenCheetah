/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "Imagic.h"

/* ----------------------------------------------------------------------
 * CONSTRUCTOR/DESRTUCTOR
 * ---------------------------------------------------------------------- */
Imagic::Imagic(QObject *parent) : QThread(parent)
{

    devicePower = deviceHeartRate = deviceCadence = deviceSpeed = 0.00;
    mode = IM_IDLE;
    load = DEFAULT_IM_LOAD;
    gradient = DEFAULT_IM_GRADIENT;
    weight = DEFAULT_IM_WEIGHT;
    powerScaleFactor = DEFAULT_IM_SCALING;
    deviceStatus=0;
    brakeCalibrationFactor = 1;
    this->parent = parent;

    /*
     * Initialise the 2 byte command sequence
     * This is modified as brake resistance is set
     */
    Imagic_Command[0] = 0x80;
    Imagic_Command[1] = 0x00;

    // for interacting over the USB port
    usb2 = new LibUsb(TYPE_IMAGIC);
}

Imagic::~Imagic()
{
    delete usb2;
}

/* ----------------------------------------------------------------------
 * SET
 * ---------------------------------------------------------------------- */
void Imagic::setMode(int mode)
{
    pvars.lock();
    this->mode = mode;
    pvars.unlock();
}

// output power adjusted by this value so user can compare with hub or crank based readings
void Imagic::setPowerScaleFactor(double powerScaleFactor)
{
    pvars.lock();
    this->powerScaleFactor = powerScaleFactor;
    pvars.unlock();
}

// User weight used by brake in slope mode
void Imagic::setWeight(double weight)
{
    pvars.lock();
    this->weight = weight;
    pvars.unlock();
}

// Load in watts when in power mode
void Imagic::setLoad(double load)
{
    // Not sure if we should limit this further?
    // You might do 1000W, but would mean cycling at >80kph at max resistance!
    if (load > 1000) load = 1000;
    if (load < 0) load = 0;

    pvars.lock();
    this->load = load;
    pvars.unlock();
}

// Load as slope % when in slope mode
void Imagic::setGradient(double gradient)
{
    // We could limit gradient here as IMagic can only simulate up to 5% at best.
    // But this way, at least the training screen shows the actual gradient rather than the limited one
    // and this corresponds to the expected gradient on RLVs etc
    //if (gradient > 5) gradient = 5;
    //if (gradient < -5) gradient = -5;

    pvars.lock();
    this->gradient = gradient;
    pvars.unlock();
}


/* ----------------------------------------------------------------------
 * GET
 * ---------------------------------------------------------------------- */
void Imagic::getTelemetry(double &power, double &heartrate, double &cadence, double &speed, double &distance, int &buttons, int &steering, int &status)
{

    pvars.lock();
    power = devicePower;
    heartrate = deviceHeartRate;
    cadence = deviceCadence;
    speed = deviceSpeed;
    distance = deviceDistance;
    buttons = deviceButtons;
    steering = deviceSteering;
    status = deviceStatus;

    // work around to ensure controller doesn't miss button press.
    // The run thread will only set the button bits, they don't get
    // reset until the ui reads the device state
    deviceButtons = 0xf0;
    pvars.unlock();
}

int Imagic::getMode()
{
    int  tmp;
    pvars.lock();
    tmp = mode;
    pvars.unlock();
    return tmp;
}

double Imagic::getLoad()
{
    double tmp;
    pvars.lock();
    tmp = load;
    pvars.unlock();
    return tmp;
}

double Imagic::getGradient()
{
    double tmp;
    pvars.lock();
    tmp = gradient;
    pvars.unlock();
    return tmp;
}

double Imagic::getWeight()
{
    double tmp;
    pvars.lock();
    tmp = weight;
    pvars.unlock();
    return tmp;
}

double Imagic::getPowerScaleFactor()
{
    double tmp;
    pvars.lock();
    tmp = powerScaleFactor;
    pvars.unlock();
    return tmp;
}

/* ----------------------------------------------------------------------
 * EXECUTIVE FUNCTIONS
 *
 * start() - start/re-start reading telemetry in a thread
 * stop() - stop reading telemetry and terminates thread
 * pause() - discards inbound telemetry (ignores it)
 *
 *
 * THE MEAT OF THE CODE IS IN RUN() IT IS A WHILE LOOP CONSTANTLY
 * READING TELEMETRY AND ISSUING CONTROL COMMANDS WHILST UPDATING
 * MEMBER VARIABLES AS TELEMETRY CHANGES ARE FOUND.
 *
 * run() - bg thread continuosly reading/writing the device port
 *         it is kicked off by start and then examines status to check
 *         when it is time to pause or stop altogether.
 * ---------------------------------------------------------------------- */
int Imagic::start()
{
    pvars.lock();
    this->deviceStatus = IM_RUNNING;
    pvars.unlock();

    QThread::start();
    return 0;
}

int Imagic::restart()
{
    int status;

    // get current status
    pvars.lock();
    status = this->deviceStatus;
    pvars.unlock();
    // what state are we in anyway?
    if (status&IM_RUNNING && status&IM_PAUSED) {
        status &= ~IM_PAUSED;
        pvars.lock();
        this->deviceStatus = status;
        pvars.unlock();
        return 0; // ok its running again!
    }
    return 2;
}

int Imagic::stop()
{
    // what state are we in anyway?
    pvars.lock();
    deviceStatus = 0; // Terminate it!
    pvars.unlock();
    return 0;
}

int Imagic::pause()
{
    int status;

    // get current status
    pvars.lock();
    status = this->deviceStatus;
    pvars.unlock();

    if (status&IM_PAUSED) return 2;
    else if (!(status&IM_RUNNING)) return 4;
    else {

        // ok we're running and not paused so lets pause
        status |= IM_PAUSED;
        pvars.lock();
        this->deviceStatus = status;
        pvars.unlock();
        return 0;
    }
}

// used by thread to set variables and emit event if needed
// on unexpected exit
int Imagic::quit(int code)
{
    // event code goes here!
    exit(code);
    return 0;
}

/*------------------------------------------------------------------------------
 * THREADED CODE - READS TELEMETRY AND SENDS COMMANDS TO SET REQUIRED RESISTANCE
 *----------------------------------------------------------------------------*/
void Imagic::run()
{

    int error_count = 0;
    int clean_count = 0;
    bool isDeviceOpen = false;

    // ui controller state
    int curstatus;

    // variables for telemetry, copied to fields on each brake update
    double curPower;                      // current output power in Watts
    double curHeartRate;                  // current heartrate in BPM
    double curCadence;                    // current cadence in RPM
    double curSpeed;                      // current speed in KPH
    int curButtons;                       // Button status
    uint8_t curSteering;                  // Angle of steering controller

    // Local copies of control variables copied from class members on each update
    int mode;
    double gradient;
    double load;
    double weight;
    float brakeCalibrationFactor;

    // Internal variables
    double curResistance = 0;             // Resistance level currently applied by brake
    double curSpeedInternal = 0;          // Current speed in internal units
    double lastGradient = 0;              // Previous gradient value set on imagic

    // we need to average out power for the last second
    // since we get updates every 10ms (100hz)
    int powerhist[10];     // last 10 values received
    int powertot=0;        // running total
    int powerindex=0;      // index into the powerhist array
    for (int i=0; i<10; i++) powerhist[i]=0;

    // initialise local cache & main vars
    pvars.lock();
    curPower = this->devicePower = 0;
    curHeartRate = this->deviceHeartRate = 0;
    curCadence = this->deviceCadence = 0;
    curSpeed = this->deviceSpeed = 0;
    curSteering = this->deviceSteering = 0;
    curButtons = this->deviceButtons = 0xf0;
    pvars.unlock();


    // open the device
    if (openPort()) {
        // open failed!
        quit(2);
        return;
    } else {
        isDeviceOpen = true;
        sendOpenCommand();
    }

    /* This is the main imagic commamd / status loop.
     * Every 1/10 second we wake up and send a command to imagic, then retrieve status
     *
     * Outbound control message has the format:
     * Byte          Value / Meaning
     * 0             Brake load (0x80 = idling? 0x1e seems to be min value - 0xe2 seems to be max)
     * 1             Seems to be a flag of some sort. We always set it to zero
     *
     * Inbound status packet has the format:
     * Byte          Value / Meaning
     * 0             Bits 0-3 are button flags (0xf0 = nothing pressed, 4-7 are status (0x0e = brake connected)
     * 1-2           Speed (Units not known but seems to be km/h * 1.19)
     * 3             Cadence
     * 4             Heartrate
     * 5-8           Integer counter - time in 1/100th seconds since last reset via 0x800000000000 command
     * 9             Current operating load
     * 10            ??? Flag byte - usually 0x00, but seen occasional 0x01 ???
     * 11            0xC3 -- CONSTANT (but not really - sometimes seen 0xc2 or 0xc4
     * 12            Steering angle
     * 13            0x00 - CONSTANT?
     * 14            0x00 - CONSTANT?
     * 15            ??? - Seems to count from 0x00 to 0xff then wrap - occasionally misses a value
     * 16            ??? - Varies in relation to speed - seem to correspond to integer km/h ???
     * 17-20         0x02086f1c - CONSTANT?
    */

    while(1) {
        if (isDeviceOpen == true) {

            /**************************************************************
             * Work out the appropriate command to send to the imagic
             * For ergo or slope mode, this will set the brake resistance
             *************************************************************/
            double setResistance = 0;
            int rc = 0;

            pvars.lock();
            mode = this->mode;
            gradient = this->gradient;
            load = this->load;
            weight = this->weight;
            brakeCalibrationFactor = this->brakeCalibrationFactor;
            pvars.unlock();

            if (mode == IM_ERGOMODE)
            {
                // If we're not moving yet, just set minimum resistance
                // so its easy to set off
                if (curSpeedInternal <=0) {
                    setResistance = 30;
                }
                else {
                      // Calculate resistance reqd based on requested load and current speed
                      setResistance = (((load / curSpeedInternal) - 0.2f) / 0.0036f);

                      // Add in brake calibration factor
                      // The +1 is a cheap way to ensure we round up on conversion to integer
                      setResistance = (setResistance * brakeCalibrationFactor) + 1;

                      // Check bounds
                      if (setResistance < 30) setResistance = 30;
                      else if (setResistance > 226) setResistance = 226;
                }

                // Update the imagic resistance setting
                rc = sendRunCommand(setResistance);
            }
            else if (mode == IM_SSMODE)
            {
                /*
                 * Calculate resistance required for current speed and slope
                 * We use as a base 40kph ~ 300W.
                 * The brake resistance value implies a watts per speed value.
                 * Watts seem to increase linearly with resistance for a given speed and
                 * linearly with speed for a given resistance. As real world watts increase
                 * with the square of speed, we need to increase resiatance in the same proportion as speed.
                 * Note also that the linear relationship between watts and speed breaks down at very low
                 * or very high speeds, but as yet we don't adjust for this.
                 */

                // If we're not moving yet, just set minimum resistance
                if (curSpeedInternal <= 0) {
                    setResistance = 30;
                }
                else {
                      // Calculate the required brake resistance based on current speed and gradient
                      // We make no allowance for acceleration as we assume the flywheel
                      // takes care of that

                      // Reference watts per speed unit value for 40kph on a flat road
                      double refSpeedWatts = 300.0f/480.0f;

                      // Calculate our current speed as a ratio to 40kph
                      // which is represented in internal speed units as 480
                      double speedFactor = curSpeedInternal/480.0f;
                      // Apply this ratio to watts per unit speed figure
                      refSpeedWatts *= speedFactor;

                      // Smooth gradient transitions
                      double gradientDiff = gradient - lastGradient;
                      if (gradientDiff > 0.1) {
                          if (gradientDiff > 0.5) gradient = lastGradient + (gradientDiff / 5);
                          else gradient = lastGradient + 0.1;
                      } else if (gradientDiff < -0.1) {
                          if (gradientDiff < -0.5) gradient = lastGradient + (gradientDiff / 5);
                          else gradient = lastGradient - 0.1;
                      }
                      lastGradient = gradient;

                      // Calculate vertical speed in internal units
                      // We should really not assume that our reported speed is horizontal rather than
                      // on the hypotenuese, but it makes the calculation quicker and simpler and as
                      // the imagic maxes out at just under 4% for an 82kg rider anyway, the difference
                      // is minimal. Strictly speaking though, it does mean we set the resistance marginally
                      // higher than we should.
                      double vertSpeed = (curSpeedInternal * gradient) / 100;


                      vertSpeed = vertSpeed / (11.9f * 3.6f);
                      double vertWatts = 9.8f * vertSpeed * weight;
                      refSpeedWatts += (vertWatts / curSpeedInternal);


                      // Now apply the formula to convert watts/speed unit to a resistance setting
                      setResistance = (refSpeedWatts - 0.2f) / 0.0036f;

                      // Add in brake calibration factor
                      setResistance *= brakeCalibrationFactor;

                      // Check bounds
                      if (setResistance < 30) setResistance = 30;
                      else if (setResistance > 226) setResistance = 226;
                }

                // Update the imagic resistance setting
                rc = sendRunCommand(setResistance);

            }
            else if (mode == IM_IDLE)
            {
                rc = sendOpenCommand();
            }
            else if (mode == IM_CALIBRATE)
            {
                // TODO
            }

            if (rc < 0) {
                qDebug() << "usb write error " << rc;
                if (error_count++ > 20) {
                    // Too many errors - time to give up
                    closePort(); // need to release that file handle!!
                    quit(4);
                    return;
                }
                // Allow up to 20 retries for every 100 successes
                msleep(100);
                continue;
            }

            /**************************************************************
             * Read the status of the imagic and update telemetry
             *************************************************************/
            int actualLength = readMessage();
            if (actualLength < 0) {
                qDebug() << "usb read error " << actualLength;
                if (error_count++ > 20) {
                    closePort();
                    quit(8);
                    return;
                }
                msleep(100);
                continue;
            }

            if (actualLength != 21) {
                qDebug() << "Unexpected imagic status length " << actualLength;
                if (error_count++ > 20) {
                    closePort();
                    quit(16);
                    return;
                }
                msleep(100);
                continue;
            }

            // buttons
            curButtons = buf[0] & 0xf0;

            //--------------------------------------------------------------
            // HR, Cadence and steering angle are reported as 8-bit integers
            //--------------------------------------------------------------
            curSteering = buf[12];
            curCadence = buf[3];
            curHeartRate = buf[4];

            // ----------------------------------------------------------------
            // Speed seems to be in some mysterious internal units
            // Divide by 11.9 to convert to km/h
            // ----------------------------------------------------------------
            curSpeedInternal = (double)qFromLittleEndian<quint16>(&buf[1]);
            curSpeed = curSpeedInternal / (1.19f * 10.0f);

            // Power
            // Need to calculate this based on resistance setting and speed.
            // From measurements, it seems that Watts = (0.0036 * reported resistance setting + 0.2) * reported speed
            curResistance = buf[9];
            curPower = ((curResistance * 0.0036f) + 0.2f) * curSpeedInternal;
            if (curPower < 0) curPower = 0;

            // average power over last second (ish)
            powertot += curPower;
            powertot -= powerhist[powerindex];
            powerhist[powerindex] = curPower;

            curPower = powertot / 10;
            powerindex = (powerindex == 9) ? 0 : powerindex+1;

            curPower *= powerScaleFactor; // apply scale factor

            // update public fields
            pvars.lock();
            deviceSpeed = curSpeed;
            deviceCadence = curCadence;
            deviceHeartRate = curHeartRate;
            devicePower = curPower;
            deviceButtons &= curButtons;    // Accumulate all button pushes since last read
            deviceSteering = curSteering;
            pvars.unlock();
        }

        //----------------------------------------------------------------
        // LISTEN TO GUI CONTROL COMMANDS
        //----------------------------------------------------------------
        pvars.lock();
        curstatus = this->deviceStatus;
        pvars.unlock();

        // time to shut up shop?
        if (!(curstatus&IM_RUNNING)) {
            sendCloseCommand();
            closePort(); // need to release that file handle!!
            quit(0);
            return;
        }

        if ((curstatus&IM_PAUSED) && isDeviceOpen == true) {
            closePort();
            isDeviceOpen = false;
        }
        else if (!(curstatus&IM_PAUSED) && (curstatus&IM_RUNNING) && isDeviceOpen == false) {

            if (openPort()) {
                quit(2);
                return; // open failed!
            }
            isDeviceOpen = true;
            sendOpenCommand();
        }

        // Reset error counter after 100 successful cycles
        if (clean_count++ > 100) clean_count = error_count = 0;
        // Interrogate imagic every 100ms
        msleep(100);
    }

}

/* ----------------------------------------------------------------------
 * HIGH LEVEL DEVICE IO ROUTINES
 *
 * sendOpenCommand() - initialises training session
 * sendCloseCommand() - finalises training session
 * sendRunCommand(double, double) - update brake resistance
 *
 * ---------------------------------------------------------------------- */
int Imagic::sendOpenCommand()
{
    uint8_t open_command[] = {0x80,0x00,0x00,0x00,0x00,0x00};

    // This command resets all imagic timers and counters
    int retCode = rawWrite(open_command, 6);
    //qDebug() << "usb status " << retCode;
    return retCode;
}

int Imagic::sendCloseCommand()
{
    // Not required at the moment for imagic
    return(0);
}

int Imagic::sendRunCommand(int resistance)
{
    Imagic_Command[0] = resistance;
    Imagic_Command[1] = 0x00;
    int retCode = rawWrite(Imagic_Command, 2);
    //qDebug() << "usb status " << retCode;
    return retCode;
}

/* ----------------------------------------------------------------------
 * LOW LEVEL DEVICE IO ROUTINES - PORT TO QIODEVICE REQUIRED BEFORE COMMIT
 *
 *
 * readMessage()        - reads an inbound message
 * openPort() - opens serial device and configures it
 * closePort() - closes serial device and releases resources
 * rawRead() - non-blocking read of inbound data
 * rawWrite() - non-blocking write of outbound data
 * discover() - check if a ct is attached to the port specified
 * ---------------------------------------------------------------------- */
int Imagic::readMessage()
{
    int rc;
    rc = rawRead(buf, 21);
    //qDebug() << "usb status " << rc;
    return rc;
}

int Imagic::closePort()
{
    usb2->close();
    return 0;
}

bool Imagic::find()
{
    int rc;
    rc = usb2->find();
    //qDebug() << "usb status " << rc;
    return rc;
}

int Imagic::openPort()
{
    int rc;
    rc = usb2->open();
    //qDebug() << "usb status " << rc;
    return rc;
}

int Imagic::rawWrite(uint8_t *bytes, int size) // unix!!
{
    return usb2->write((char *)bytes, size, IM_USB_TIMEOUT);
}

int Imagic::rawRead(uint8_t bytes[], int size)
{
    return usb2->read((char *)bytes, size, IM_USB_TIMEOUT);
}

// check to see of there is a port at the device specified
// returns true if the device exists and false if not
bool Imagic::discover(QString)
{
    return true;
}
