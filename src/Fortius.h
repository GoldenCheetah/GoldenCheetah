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


// I have consciously avoided putting things like data logging, lap marking,
// intervals or any load management functions in this class. It is restricted
// to controlling an reading telemetry from the device
//
// I expect higher order classes to implement such functions whilst
// other devices (e.g. ANT+ devices) may be implemented with the same basic
// interface
//
// I have avoided a base abstract class at this stage since I am uncertain
// what core methods would be required by say, ANT+ or Tacx devices


#ifndef _GC_Fortius_h
#define _GC_Fortius_h 1

#include <QObject>

#include "LibUsb.h"
#include "RunningAverage.h"

/* Device operation mode */
#define FT_ERGOMODE    0x01
#define FT_SSMODE      0x02
#define FT_CALIBRATE   0x04

// forward declarations
class QTimer;

const int FORTIUS_RECEIVE_MESSAGE_SIZE = 48;

class Fortius : public QObject
{
    Q_OBJECT
public:
	enum OperationMode {
		ErgoMode,
		SlopeMode,
		Calibration
	};

	Fortius(OperationMode mode, double startLoad, double startGradient,
			QObject *parent=0);
    ~Fortius();

    static bool find();                      // find fortius device

public slots:
    // HIGH-LEVEL FUNCTIONS
    void start();
    void restart();                              // restart after paused
    void pause();                                // pauses data collection, inbound telemetry is discarded
    void stop();                                 // stops data collection thread

    // SET
    void changeLoad(double load);                  // set the load to generate in ERGOMODE
    void changeGradient(double gradient);          // set the load to generate in SSMODE
    void useErgoMode();
    void useSlopeMode();

Q_SIGNALS:
    // GET TELEMETRY AND STATUS
    // direct access to class variables is not allowed because we need to use wait conditions
    // to sync data read/writes between the run() thread and the main gui thread
    void signalTelemetry(double power, double heartrate, double cadence,
		      double speed);

    void upButtonPushed();

    void downButtonPushed();

    void cancelButtonPushed();

    void enterButtonPushed();

    void error(int errorNr);

private Q_SLOTS:
    /** Perform read from usb */
    void performRead();
    /** Send signal with telemetry */
    void emitTelemetry();
private:

    /** set all values to 0 */
    void resetValues();

    // 56 bytes comprise of 8 7byte command messages, where
    // the last is the set load / gradient respectively
    uint8_t ERGO_Command[12],
            SLOPE_Command[12];


    // Protocol encoding
    void prepareSlopeCommand();
    void prepareErgoCommand();
    void sendCommand();      // writes a command to the device

    // Read message from Fortius
    int readMessage();

    QTimer *_readTimer;

	// OUTBOUND COMMANDS
	int mode;
	double load;
	double gradient;

    // INBOUND TELEMETRY
    RunningAverage power;
    RunningAverage heartRate;
    RunningAverage cadence;
    RunningAverage speed;

    // i/o message holder
    uint8_t buf[FORTIUS_RECEIVE_MESSAGE_SIZE];

    // device port
    QScopedPointer<LibUsb> usb2;                   // used for USB2 support

    void handleButtons(int buttons);

    // Handling of USB port
    int openPort();
    int closePort();

    // raw device utils
    int rawWrite(uint8_t *bytes, int size);
    int rawRead(uint8_t *bytes, int size);
};

#endif // _GC_Fortius_h
