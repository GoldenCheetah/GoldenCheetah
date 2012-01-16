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

#include "Fortius.h"

#include <QTimer>
#include <QtDebug>
#include <QtEndian>

//
// Outbound control message has the format:
// Byte          Value / Meaning
// 0             0x01 CONSTANT
// 1             0x08 CONSTANT
// 2             0x01 CONSTANT
// 3             0x00 CONSTANT
// 4             Brake Value - Lo Byte
// 5             Brake Value - Hi Byte
// 6             Echo cadence sensor
// 7             0x00 -- UNKNOWN
// 8             0x02 -- 0 - idle 2 - Active
// 9             0x52 -- Mode (?) 52 = idle, 0a = ergo, 48 = slope. 
// 10            0x10 -- UNKNOWN - CONSTANT?
// 11            0x04 -- UNKNOWN - CONSTANT?

const static uint8_t ergo_command[12] = {
     // 0     1     2     3     4     5     6     7    8      9     10    11
        0x01, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0a, 0x10, 0x04
};

const static uint8_t slope_command[12] = {
     // 0     1     2     3     4     5     6     7    8      9     10    11
        0x01, 0x08, 0x01, 0x00, 0x6c, 0x01, 0x00, 0x00, 0x02, 0x48, 0x10, 0x04
};

static const int READ_DELAY = 70; // read delay in miliseconds.

enum Buttons {
    ButtonEnter = 0x01,
    ButtonMinus = 0x02,
    ButtonPlus = 0x04,
    ButtonCancel = 0x08
};

Fortius::Fortius(OperationMode mode, double startLoad, double startGradient,
				 QObject *parent) : QObject(parent),
    _readTimer(new QTimer(this)),
	mode(mode), load(startLoad), gradient(startGradient),
    power(16), heartRate(16), cadence(16), speed(16),
    usb2(new LibUsb(TYPE_FORTIUS))
{
    connect(_readTimer, SIGNAL(timeout()), SLOT(performRead()));

    /* 56 byte control sequence, composed of 8 command packets
     * where the last packet sets the load. The first byte
     * is a CRC for the value being issued (e.g. Load in WATTS)
     *
     * these members are modified as load / gradient are set
     */
    memcpy(ERGO_Command, ergo_command, 12);
    memcpy(SLOPE_Command, slope_command, 12);
}

Fortius::~Fortius()
{
    if (_readTimer->isActive())
	qCritical() << "Fortius destructor called, but Fortius is not stopped.."    ;
}

void Fortius::start()
{
    resetValues();

    if (openPort())
	emit error(10);
    else
	sendCommand();
}

void Fortius::restart() {
    resetValues();
    sendCommand();
}

void Fortius::pause() {
    if (_readTimer->isActive())
	_readTimer->stop();
}

void Fortius::stop() {
    if (_readTimer->isActive()) {
        _readTimer->stop();
        closePort();
    }
}

// Read current values from Fortius device.
void Fortius::performRead()
{
    int nrReceivedBytes = readMessage();

    if (nrReceivedBytes < 0) {
	    qDebug() << "Error receiving message from Fortius";
	    emit error(1);
    } else if (nrReceivedBytes < FORTIUS_RECEIVE_MESSAGE_SIZE) {
#if 0
	qDebug() << "Short read. Nr of bytes received = " << nrReceivedBytes;
#endif
    }

    //----------------------------------------------------------------
    // UPDATE BASIC TELEMETRY (HR, CAD, SPD et al)
    // The data structure is very simple, no bit twiddling needed here
    //----------------------------------------------------------------

    int buttons = static_cast<int>(buf[13]);
    handleButtons(buttons);

    // brake status status&0x04 == stopping wheel
    //              status&0x01 == brake on
    // buf[42];

    // cadence
    cadence.add(static_cast<double>(buf[44]));

    // speed
    double currentSpeed = qFromLittleEndian<quint16>(&buf[32]) / 1000.0;
    speed.add(currentSpeed);

    // power
    qint16 force = qFromLittleEndian<qint16>(&buf[38]) / 100.0;
    power.add(force * currentSpeed);

    // heartrate
    quint8 receivedHeartRate = buf[12];
    heartRate.add(static_cast<double>(receivedHeartRate));

    // send update
    emitTelemetry();

    // send new command to fortius
    sendCommand();
}

/* ----------------------------------------------------------------------
 * SET
 * ---------------------------------------------------------------------- */
void Fortius::changeMode(OperationMode mode) {
    this->mode = mode;
}

void Fortius::changeLoad(double load) {
    this->load = qMax(50.0, qMin(1000.0, load));
}

void Fortius::changeGradient(double gradient)
{
    this->gradient = gradient;
}


/* ----------------------------------------------------------------------
 * signals
 * ---------------------------------------------------------------------- */
void Fortius::emitTelemetry()
{
    emit signalTelemetry(power.average(), heartRate.average(), cadence.average(),
			 speed.average() * 3.6 /* m/s to km/h */);
}

void Fortius::prepareSlopeCommand() {
    qint16 value;

    if (speed.average() < 3) // m/s
	value = 0;
    else
	value = 650 * gradient;

    qToLittleEndian<qint16>(value, &SLOPE_Command[4]);
}

void Fortius::prepareErgoCommand() {
    qint16 value;

    if (speed.average() < 3) // m/s
	value = 0;
    else
	value = static_cast<qint16>((load / speed.average()) * 100);

    qToLittleEndian<qint16>(value, &ERGO_Command[4]);
}

void Fortius::resetValues() {
    memset(buf, 0, FORTIUS_RECEIVE_MESSAGE_SIZE);

    power.reset();
    heartRate.reset();
    cadence.reset();
    speed.reset();
}

/* ----------------------------------------------------------------------
 * LOW LEVEL DEVICE IO ROUTINES - PORT TO QIODEVICE REQUIRED BEFORE COMMIT
 *
 *
 * HIGH LEVEL IO
 * int sendCommand()        - writes a command to the device
 * int readMessage()        - reads an inbound message
 *
 * LOW LEVEL IO
 * openPort() - opens serial device and configures it
 * closePort() - closes serial device and releases resources
 * rawRead() - non-blocking read of inbound data
 * rawWrite() - non-blocking write of outbound data
 * discover() - check if a ct is attached to the port specified
 * ---------------------------------------------------------------------- */

/**
  * Send command to Fortius.
  * If sending succeeds, a single-shot timer is started which will call
  * performRead() when data should be available for reading.
  * If sending fails, an error is emitted and no further reading or sending will
  * be done.
  */
void Fortius::sendCommand()      // writes a command to the device
{
    int result;
    switch (mode) {
	case ErgoMode:
		prepareErgoCommand();
		result = rawWrite(ERGO_Command, 12);
		break;

	case SlopeMode:
		prepareSlopeCommand();
		result = rawWrite(SLOPE_Command, 12);
		break;

	default:
		result = -1;
		break;
    }

    if (result < 0)
		emit error(1);
    else
		_readTimer->singleShot(READ_DELAY, this, SLOT(performRead()));
}

int Fortius::readMessage()
{
    int rc;

    rc = rawRead(buf, FORTIUS_RECEIVE_MESSAGE_SIZE);
    return rc;
}

int Fortius::closePort()
{
    usb2->close();
    return 0;
}

bool Fortius::find()
{
    return usb2->find();
}

int Fortius::openPort()
{
    // on windows we try on USB2 then on USB1 then fail...
    return usb2->open();
}

int Fortius::rawWrite(uint8_t *bytes, int size) // unix!!
{
    int rc=0;

    rc = usb2->write((char *)bytes, size);

    if (!rc) rc = -1; // return -1 if nothing written
    return rc;
}

int Fortius::rawRead(uint8_t bytes[], int size)
{
    return usb2->read((char *)bytes, size);
}

void Fortius::handleButtons(int buttons)
{
    // ADJUST LOAD
    if (buttons&ButtonPlus) emit upButtonPushed();
    if (buttons&ButtonMinus) emit downButtonPushed();

    // LAP/INTERVAL
    if (buttons&ButtonEnter) emit enterButtonPushed();

    // CANCEL
    if (buttons&ButtonCancel) emit cancelButtonPushed();
}

