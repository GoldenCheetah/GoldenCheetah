/*
 * Copyright (c) 2015 Jon Escombe (jone@dresco.co.uk)
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

#ifndef SRC_CALIBRATIONDATA_H_
#define SRC_CALIBRATIONDATA_H_

#include <stdint.h> // uint8_t etc

#define CALIBRATION_TYPE_NOT_SUPPORTED               0x00
#define CALIBRATION_TYPE_COMPUTRAINER                0x01
#define CALIBRATION_TYPE_ZERO_OFFSET                 0x02
#define CALIBRATION_TYPE_ZERO_OFFSET_SRM             0x04
#define CALIBRATION_TYPE_SPINDOWN                    0x08
#define CALIBRATION_TYPE_FORTIUS                     0x10

#define CALIBRATION_STATE_IDLE                       0x00
#define CALIBRATION_STATE_PENDING                    0x01
#define CALIBRATION_STATE_REQUESTED                  0x02
#define CALIBRATION_STATE_STARTING                   0x03
#define CALIBRATION_STATE_STARTED                    0x04
#define CALIBRATION_STATE_POWER                      0x05
#define CALIBRATION_STATE_COAST                      0x06
#define CALIBRATION_STATE_SUCCESS                    0x07
#define CALIBRATION_STATE_FAILURE                    0x08
#define CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_SLOW  0x09
#define CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_FAST  0x0a

#define CALIBRATION_MAX_CHANNELS        8

class CalibrationData
{
public:

    CalibrationData();

    uint8_t getType();
    void setType(uint8_t channel, uint8_t type);

    uint8_t getState();
    void setState(uint8_t state);

    uint16_t getZeroOffset();
    void setZeroOffset(uint16_t offset);

    uint16_t getSpindownTime();
    void setSpindownTime(uint16_t time);

    double getTargetSpeed();
    void setTargetSpeed(double speed);

    uint16_t getSlope();
    void setSlope(uint16_t slope);

    void    resetCalibrationState(void);
    void    setRequested(uint8_t channel, bool required);
    void    setTimestamp(uint8_t channel, double time);
    uint8_t getActiveChannel(void);

private:

    void setActiveChannel(void);

    // support multiple channels with calibration capability
    uint8_t  type[CALIBRATION_MAX_CHANNELS];
    double   timestamp[CALIBRATION_MAX_CHANNELS];
    bool     requested[CALIBRATION_MAX_CHANNELS];

    uint8_t  activechannel;
    uint8_t  state;
    uint16_t zerooffset;
    uint16_t spindowntime;
    uint16_t slope;
    double   targetspeed;
};

#endif /* SRC_CALIBRATIONDATA_H_ */
