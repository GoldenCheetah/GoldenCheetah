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


#include <float.h>
#include "CalibrationData.h"
#include <QtDebug>

CalibrationData::CalibrationData()
{
    resetCalibrationState();
}

void CalibrationData::resetCalibrationState()
{
    for (uint8_t i=0; i<CALIBRATION_MAX_CHANNELS; i++) {
        type[i] = CALIBRATION_TYPE_NOT_SUPPORTED;
        timestamp[i] = 0;
        requested[i] = false;
    }
    state = CALIBRATION_STATE_IDLE;
    activechannel = targetspeed = spindowntime = zerooffset = slope = 0;
}

uint8_t CalibrationData::getType()
{
    return type[activechannel];
}

void CalibrationData::setType(uint8_t channel, uint8_t type)
{
    if (this->type[channel] != type) {
        qDebug() << "Calibration type changing to" << type << "for channel" << channel;
        this->type[channel] = type;
        setActiveChannel();
    }
}

uint8_t CalibrationData::getState()
{
    return this->state;
}

void CalibrationData::setState(uint8_t state)
{
    if (this->state != state) {
        qDebug() << "Calibration state changing to" << state;
        this->state = state;

        if (state == CALIBRATION_STATE_IDLE)
            setActiveChannel();
    }
}

uint16_t CalibrationData::getSpindownTime()
{
    return this->spindowntime;
}

void CalibrationData::setSpindownTime(uint16_t time)
{
    if (this->spindowntime != time) {
        qDebug() << "Calibration spindown time changing to" << time;
        this->spindowntime = time;
    }
}

uint16_t CalibrationData::getZeroOffset()
{
    return this->zerooffset;
}

void CalibrationData::setZeroOffset(uint16_t offset)
{
    if (this->zerooffset != offset) {
        qDebug() << "Calibration zero offset changing to" << offset;
        this->zerooffset = offset;
    }
}

uint16_t CalibrationData::getSlope()
{
    return this->slope;
}

void CalibrationData::setSlope(uint16_t slope)
{
    if (this->slope != slope) {
        qDebug() << "Calibration slope changing to" << slope;
        this->slope = slope;
    }
}

double CalibrationData::getTargetSpeed()
{
    return this->targetspeed;
}

void CalibrationData::setTargetSpeed(double speed)
{
    if (this->targetspeed != speed) {
        qDebug() << "Calibration target speed changing to" << speed;
        this->targetspeed = speed;
    }
}

void CalibrationData::setRequested(uint8_t channel, bool required)
{
    if (this->requested[channel] != required) {

        if (required)
            qDebug() << "Calibration requested for channel" << channel;
        else
            qDebug() << "Calibration request cleared for channel" << channel;

        this->requested[channel] = required;
        setActiveChannel();
    }
}

void CalibrationData::setTimestamp(uint8_t channel, double time)
{
    if (this->timestamp[channel] != time) {
        qDebug() << "Last calibration timestamp updated for channel" << channel;
        this->timestamp[channel] = time;
    }
}

uint8_t CalibrationData::getActiveChannel()
{
    return activechannel;
}

void CalibrationData::setActiveChannel()
{
    // pick the channel to be used for calibration
    //
    // - if a device has requested calibration - use it
    // - else consider timestamps from last calibration attempt
    // - else pick any device supporting calibration

    uint8_t count=0;

    // Do nothing if calibration in progress
    switch(state) {
    default:
        return;
    case CALIBRATION_STATE_IDLE:
    case CALIBRATION_STATE_SUCCESS:
    case CALIBRATION_STATE_FAILURE:
    case CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_FAST:
    case CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_SLOW:
        break;
    }

    // How many devices support calibration?
    for (int i = 0; i < CALIBRATION_MAX_CHANNELS; i++) {
        if (type[i] != CALIBRATION_TYPE_NOT_SUPPORTED)
            count++;
    }

    switch (count) {
    case 0:             // no devices with calibration support
        break;

    case 1:             // select first/only device with calibration support
        for (int i = 0; i < CALIBRATION_MAX_CHANNELS; i++) {
            if (type[i] != CALIBRATION_TYPE_NOT_SUPPORTED) {
                qDebug() << "Setting active channel for calibration to" << i << "(single)";
                this->activechannel = i;
                break;
            }
        }
        break;

    default:            // multiple devices with calibration support
        // if a device has requested calibration - use it
        for (int i = 0; i < CALIBRATION_MAX_CHANNELS; i++) {
            if ((this->requested[i] == true) && (type[i] != CALIBRATION_TYPE_NOT_SUPPORTED)) {
                qDebug() << "Setting active channel for calibration to" << i << "(multiple/requested)";
                this->activechannel = i;
                return;
            }
        }

        // else pick device with lowest (or no) timestamp from previous calibrations
        double time = DBL_MAX;
        uint8_t chan = 0;
        for (int i = 0; i < CALIBRATION_MAX_CHANNELS; i++) {
            if (type[i] != CALIBRATION_TYPE_NOT_SUPPORTED) {
                //qDebug() << "Channel" << i << "is candidate for calibration with timestamp" << timestamp[i];
                // calibration supported for this channel, check the timestamp
                if (timestamp[i] < time) {
                    time = timestamp[i];
                    chan = i;
                }
            }
        }
        qDebug() << "Setting active channel for calibration to" << chan << "(multiple/timestamp)";
        this->activechannel = chan;
        break;
    }
}
