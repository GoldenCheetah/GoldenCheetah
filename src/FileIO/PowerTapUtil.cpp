/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#include "PowerTapUtil.h"
#include "RideFile.h"
#include "Units.h"
#include <QString>
#include <cmath>
#include <stdio.h>

bool
PowerTapUtil::is_ignore_record(unsigned char *buf, bool bVer81)
{
    if (bVer81)
        return buf[0]==0 && buf[1]==0 && buf[2]==0;
    else
        return buf[0]==0;
}

bool
PowerTapUtil::is_Ver81(unsigned char *buf)
{
    return buf[3] == 0x81;
}

int
PowerTapUtil::is_time(unsigned char *buf, bool bVer81)
{
    return (bVer81 && buf[0] == 0x10) || (!bVer81 && buf[0] == 0x60);
}

time_t
PowerTapUtil::unpack_time(unsigned char *buf, struct tm *time, bool bVer81)
{
    (void) bVer81; // unused
    memset(time, 0, sizeof(*time));
    time->tm_year = 2000 + buf[1] - GC_EPOCH_YEAR;
    time->tm_mon = buf[2] - 1;
    time->tm_mday = buf[3] & 0x1f;
    time->tm_hour = buf[4] & 0x1f;
    time->tm_min = buf[5] & 0x3f;
    time->tm_sec = ((buf[3] >> 5) << 3) | (buf[4] >> 5);
    time->tm_isdst = -1;
    return mktime(time);
}

int
PowerTapUtil::is_config(unsigned char *buf, bool bVer81)
{
    return (bVer81 && buf[0] == 0x00) || (!bVer81 && buf[0] == 0x40);
}

const double TIME_UNIT_SEC = 0.021*60.0;
const double TIME_UNIT_SEC_V81 = 0.01;

int
PowerTapUtil::unpack_config(unsigned char *buf, unsigned *interval,
                            unsigned *last_interval, double *rec_int_secs,
                            unsigned *wheel_sz_mm, bool bVer81)
{
    *wheel_sz_mm = (buf[1] << 8) | buf[2];
    /* Data from device wraps interval after 9... */
    if (buf[3] != *last_interval) {
        *last_interval = buf[3];
        ++*interval;
    }

    *rec_int_secs = buf[4];
    if (bVer81)
    {
        *rec_int_secs *= TIME_UNIT_SEC_V81;
    }
    else
    {
        *rec_int_secs += 1;
        *rec_int_secs *= TIME_UNIT_SEC;
    }
    return 0;
}

int
PowerTapUtil::is_data(unsigned char *buf, bool bVer81)
{
    if (bVer81)
        return (buf[0] & 0x40) == 0x40;
    else
        return (buf[0] & 0x80) == 0x80;
}

#define MAGIC_CONSTANT 147375.0
#define PI M_PI
#define LBFIN_TO_NM 0.11298483

void
PowerTapUtil::unpack_data(unsigned char *buf, double rec_int_secs,
                          unsigned wheel_sz_mm, double *time_secs,
                          double *torque_Nm, double *mph, double *watts,
                          double *dist_m, unsigned *cad, unsigned *hr,
                          bool bVer81)
{
    if (bVer81)
    {
        const double CLOCK_TICK_TIME = 0.000512;
        const double METERS_PER_SEC_TO_MPH = 2.23693629;

        *time_secs += rec_int_secs;
        int rotations = buf[0] & 0x0f;
        int ticks_for_1_rotation = (buf[1]<<4) | (buf[2]>>4);

        if (ticks_for_1_rotation==0xff0 || ticks_for_1_rotation==0)
        {
            *watts = 0;
            *cad = 0;
            *mph = 0;
            *torque_Nm = 0;
        }
        else
        {
            *watts = ((buf[2] & 0x0f) << 8) | buf[3];

            // clear out of bounds power values
            if(*watts > RideFile::maximumFor(RideFile::watts)) *watts = 0.0;
            
            *cad = buf[4];
            if (*cad == 0xff)
                *cad = 0;

            double wheel_sz_meters = wheel_sz_mm / 1000.0;
            *dist_m += rotations * wheel_sz_meters;
            double seconds_for_1_rotation = ticks_for_1_rotation * CLOCK_TICK_TIME;
            double meters_per_sec = wheel_sz_meters / seconds_for_1_rotation;
            *mph = meters_per_sec * METERS_PER_SEC_TO_MPH;

            *torque_Nm = (*watts * seconds_for_1_rotation)/(2.0*PI);
        }
        *hr = buf[5];
        if (*hr == 0xff)
            *hr = 0;

    }
    else
    {
        double kph10;
        unsigned speed;
        unsigned torque_inlbs;

        *time_secs += rec_int_secs;
        torque_inlbs = ((buf[1] & 0xf0) << 4) | buf[2];
        if (torque_inlbs == 0xfff)
            torque_inlbs = 0;
        speed = ((buf[1] & 0x0f) << 8) | buf[3];
        if ((speed < 100) || (speed == 0xfff)) {
            if ((speed != 0) && (speed < 1000)) {
                fprintf(stderr, "possible error: speed=%.1f; ignoring it\n",
                        MAGIC_CONSTANT / speed / 10.0);
            }
            *mph = -1.0;
            *watts = -1.0;
        }
        else {
            *torque_Nm = torque_inlbs * LBFIN_TO_NM;
            kph10 = MAGIC_CONSTANT / speed;
            *mph = kph10 / 10.0 * MILES_PER_KM;

            // from http://en.wikipedia.org/wiki/Torque#Conversion_to_other_units
            double dMetersPerMinute = (kph10 / 10.0) * 1000.0 / 60.0;
            double dWheelSizeMeters = wheel_sz_mm / 1000.0;
            double rpm = dMetersPerMinute/dWheelSizeMeters;
            *watts = round(*torque_Nm * rpm * 2.0 * PI / 60.0);
        }
        *dist_m += (buf[0] & 0x7f) * wheel_sz_mm / 1000.0;
        *cad = buf[4];
        if (*cad == 0xff)
            *cad = 0;
        *hr = buf[5];
        if (*hr == 0xff)
            *hr = 0;
    }
}


