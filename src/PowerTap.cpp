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

#include "PowerTap.h"
#include <math.h>

#define PT_DEBUG false

static bool
hasNewline(const char *buf, int len)
{
    static char newline[] = { 0x0d, 0x0a };
    if (len < 2)
        return false;
    for (int i = 0; i < len; ++i) {
        bool success = true;
        for (int j = 0; j < 2; ++j) {
            if (buf[i+j] != newline[j]) {
                success = false;
                break;
            }
        }
        if (success)
            return true;
    }
    return false;
}

static QString
cEscape(const char *buf, int len)
{
    char *result = new char[4 * len + 1];
    char *tmp = result;
    for (int i = 0; i < len; ++i) {
        if (buf[i] == '"')
            tmp += sprintf(tmp, "\\\"");
        else if (isprint(buf[i]))
            *(tmp++) = buf[i];
        else
            tmp += sprintf(tmp, "\\x%02x", 0xff & (unsigned) buf[i]);
    }
    return result;
}

static bool
doWrite(DevicePtr dev, char c, bool hwecho, QString &err)
{
    if (PT_DEBUG) printf("writing '%c' to device\n", c);
    int n = dev->write(&c, 1, err);
    if (n != 1) {
        if (n < 0)
            err = QString("failed to write %1 to device: %2").arg(c).arg(err);
        else
            err = QString("timeout writing %1 to device").arg(c);
        return false;
    }
    if (hwecho) {
        char c;
        int n = dev->read(&c, 1, err);
        if (n != 1) {
            if (n < 0)
                err = QString("failed to read back hardware echo: %2").arg(err);
            else
                err = "timeout reading back hardware echo";
            return false;
        }
    }
    return true;
}

static int
readUntilNewline(DevicePtr dev, char *buf, int len, QString &err)
{
    int sofar = 0;
    while (!hasNewline(buf, sofar)) {
        assert(sofar < len);
        // Read one byte at a time to avoid waiting for timeout.
        int n = dev->read(buf + sofar, 1, err);
        if (n <= 0) {
            err = (n < 0) ? ("read error: " + err) : "read timeout";
            err += QString(", read %1 bytes so far: \"%2\"")
                .arg(sofar).arg(cEscape(buf, sofar));
            return -1;
        }
        sofar += n;
    }
    return sofar;
}

bool
PowerTap::download(DevicePtr dev, QByteArray &version,
                   QVector<unsigned char> &records,
                   StatusCallback statusCallback, QString &err)
{
    if (!dev->open(err)) {
        err = "ERROR: open failed: " + err;
        return false;
    } 

    if (!doWrite(dev, 0x56, false, err)) // 'V'
        return false;
    if (!statusCallback(STATE_READING_VERSION)) {
        err = "download cancelled";
        return false;
    }
    char vbuf[256];
    int version_len = readUntilNewline(dev, vbuf, sizeof(vbuf), err);
    if (version_len < 0) {
        err = "Error reading version: " + err;
        return false;
    }
    if (PT_DEBUG) {
        printf("read version \"%s\"\n",
               cEscape(vbuf, version_len).toAscii().constData());
    }
    version = QByteArray(vbuf, version_len);

    // We expect the version string to be something like 
    // "VER 02.21 PRO...", so if we see two V's, it's probably 
    // because there's a hardware echo going on.

    int veridx = version.indexOf("VER");
    if (veridx < 0) {
        err = QString("Unrecognized version \"%1\"")
                .arg(cEscape(vbuf, version_len));
        return false;
    }
    bool hwecho = version.indexOf('V') < veridx;
    if (PT_DEBUG) printf("hwecho=%s\n", hwecho ? "true" : "false");

    if (!statusCallback(STATE_READING_HEADER)) {
        err = "download cancelled";
        return false;
    }

    if (!doWrite(dev, 0x44, hwecho, err)) // 'D'
        return false;
    unsigned char header[6];
    int header_len = dev->read(header, sizeof(header), err);
    if (header_len != 6) {
        if (header_len < 0)
            err = "ERROR: reading header: " + err;
        else
            err = "ERROR: timeout reading header";
        return false;
    }
    if (PT_DEBUG) {
        printf("read header \"%s\"\n",
               cEscape((char*) header, 
                       sizeof(header)).toAscii().constData());
    }
    for (size_t i = 0; i < sizeof(header); ++i)
        records.append(header[i]);

    if (!statusCallback(STATE_READING_DATA)) {
        err = "download cancelled";
        return false;
    }

    fflush(stdout);
    while (true) {
        if (PT_DEBUG) printf("reading block\n");
        unsigned char buf[256 * 6 + 1];
        int n = dev->read(buf, 2, err);
        if (n < 2) {
            if (n < 0)
                err = "ERROR: reading first two: " + err;
            else
                err = "ERROR: timeout reading first two";
            return false;
        }
        if (PT_DEBUG) {
            printf("read 2 bytes: \"%s\"\n", 
                   cEscape((char*) buf, 2).toAscii().constData());
        }
        if (hasNewline((char*) buf, 2))
            break;
        unsigned count = 2;
        while (count < sizeof(buf)) {
            n = dev->read(buf + count, sizeof(buf) - count, err);
            if (n < 0) {
                err = "ERROR: reading block: " + err;
                return false;
            }
            if (n == 0) {
                err = "ERROR: timeout reading block";
                return false;
            }
            if (PT_DEBUG) {
                printf("read %d bytes: \"%s\"\n", n, 
                       cEscape((char*) buf + count, n).toAscii().constData());
            }
            count += n;
        }
        unsigned csum = 0;
        for (int i = 0; i < ((int) sizeof(buf)) - 1; ++i) 
            csum += buf[i];
        if ((csum % 256) != buf[sizeof(buf) - 1]) {
            err = "ERROR: bad checksum";
            return false;
        }
        if (PT_DEBUG) printf("good checksum\n");
        for (size_t i = 0; i < sizeof(buf) - 1; ++i)
            records.append(buf[i]);
        if (!statusCallback(STATE_DATA_AVAILABLE)) {
            err = "download cancelled";
            return false;
        }
        if (!doWrite(dev, 0x71, hwecho, err)) // 'q'
            return false;
    }
    return true;
}

bool 
PowerTap::is_ignore_record(unsigned char *buf, bool bVer81)
{
    if (bVer81)
        return buf[0]==0 && buf[1]==0 && buf[2]==0;
    else
        return buf[0]==0;
}

bool
PowerTap::is_Ver81(unsigned char *buf)
{
    return buf[3] == 0x81;
}

int
PowerTap::is_time(unsigned char *buf, bool bVer81)
{
    return (bVer81 && buf[0] == 0x10) || (!bVer81 && buf[0] == 0x60);
}

time_t 
PowerTap::unpack_time(unsigned char *buf, struct tm *time, bool bVer81)
{
    (void) bVer81; // unused
    memset(time, 0, sizeof(*time));
    time->tm_year = 2000 + buf[1] - 1900;
    time->tm_mon = buf[2] - 1;
    time->tm_mday = buf[3] & 0x1f;
    time->tm_hour = buf[4] & 0x1f;
    time->tm_min = buf[5] & 0x3f;
    time->tm_sec = ((buf[3] >> 5) << 3) | (buf[4] >> 5);
    time->tm_isdst = -1;
    return mktime(time);
}

int
PowerTap::is_config(unsigned char *buf, bool bVer81)
{
    return (bVer81 && buf[0] == 0x00) || (!bVer81 && buf[0] == 0x40);
}

const double TIME_UNIT_SEC = 0.021*60.0;
const double TIME_UNIT_SEC_V81 = 0.01;

int 
PowerTap::unpack_config(unsigned char *buf, unsigned *interval, 
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
PowerTap::is_data(unsigned char *buf, bool bVer81)
{
    if (bVer81)
        return (buf[0] & 0x40) == 0x40;
    else
        return (buf[0] & 0x80) == 0x80;
}

static double 
my_round(double x)
{
    int i = (int) x;
    double z = x - i;
    /* For some unknown reason, the PowerTap software rounds 196.5 down... */
    if ((z > 0.5) || ((z == 0.5) && (i != 196)))
        ++i;
    return i;
}

#define MAGIC_CONSTANT 147375.0
#define PI M_PI

#define LBFIN_TO_NM 0.11298483
#define KM_TO_MI 0.62137119

#define BAD_LBFIN_TO_NM_1 0.112984
#define BAD_LBFIN_TO_NM_2 0.1129824
#define BAD_KM_TO_MI 0.62

void
PowerTap::unpack_data(unsigned char *buf, int compat, double rec_int_secs,
                      unsigned wheel_sz_mm, double *time_secs,
                      double *torque_Nm, double *mph, double *watts,
                      double *dist_m, unsigned *cad, unsigned *hr, bool bVer81)
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
            if (compat)
                *torque_Nm = torque_inlbs * BAD_LBFIN_TO_NM_2;
            else
                *torque_Nm = torque_inlbs * LBFIN_TO_NM;
            kph10 = MAGIC_CONSTANT / speed;
            if (compat)
                *mph = my_round(kph10) / 10.0 * BAD_KM_TO_MI;
            else
                *mph = kph10 / 10.0 * KM_TO_MI;

            // from http://en.wikipedia.org/wiki/Torque#Conversion_to_other_units
            double dMetersPerMinute = (kph10 / 10.0) * 1000.0 / 60.0;
            double dWheelSizeMeters = wheel_sz_mm / 1000.0;
            double rpm = dMetersPerMinute/dWheelSizeMeters;
            *watts = *torque_Nm * rpm * 2.0 * PI /60.0;

            if (compat)
                *watts = my_round(*watts);
            else
                *watts = round(*watts);
        }
        if (compat)
            *torque_Nm = torque_inlbs * BAD_LBFIN_TO_NM_1;
        *dist_m += (buf[0] & 0x7f) * wheel_sz_mm / 1000.0;
        *cad = buf[4];
        if (*cad == 0xff)
            *cad = 0;
        *hr = buf[5];
        if (*hr == 0xff)
            *hr = 0;
    }
}


