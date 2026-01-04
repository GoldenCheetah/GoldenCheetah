/*
 * Copyright (c) 2007-2008 Sean C. Rhea (srhea@srhea.net)
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

#include "Context.h"
#include "RawRideFile.h"
#include "PowerTapUtil.h"
#include "Units.h"
#include <cmath>

#ifdef Q_OS_WIN
#include <io.h>
#define DUP(fd) _dup(fd)
#else
#include <unistd.h>
#define DUP(fd) dup(fd)
#endif

#ifdef Q_OS_WIN
// 'fscanf': This function or variable may be unsafe.
// 'sprintf': This function or variable may be unsafe.
// 'fdopen': The POSIX name for this item is deprecated.
#pragma warning(disable:4996)
#endif

static int rawFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "raw", "GoldenCheetah Raw PowerTap Format", new RawFileReader());

struct ReadState
{
    RideFile *rideFile;
    QStringList &errors;
    double last_secs, last_miles;
    unsigned last_interval;
    time_t start_since_epoch;
    // this seems to not be used
    //unsigned rec_int;
    ReadState(RideFile *rideFile,
                     QStringList &errors) :
        rideFile(rideFile), errors(errors), last_secs(0.0),
        last_miles(0.0), last_interval(0), start_since_epoch(0)/*, rec_int(0)*/ {}
};

static void
config_cb(unsigned interval, double rec_int_secs,
          unsigned wheel_sz_mm, void *context)
{
    (void) interval;
    (void) wheel_sz_mm;
    ReadState *state = (ReadState*) context;
    // Assume once set, rec_int should never change.
    //double recIntSecs = rec_int * 1.26;
    // ACTUALLY LETS NOT, IF IT CHANGES ITS NOT A REASON TO FUCKING CRASH.
    //assert((state->rideFile->recIntSecs() == 0.0) || (state->rideFile->recIntSecs() == rec_int_secs));
    state->rideFile->setRecIntSecs(rec_int_secs);
}

static void
time_cb(struct tm *, time_t since_epoch, void *context)
{
    ReadState *state = (ReadState*) context;
    if (state->rideFile->startTime().isNull())
    {
        QDateTime t;
        t.setSecsSinceEpoch(since_epoch);
        state->rideFile->setStartTime(t);
    }
    if (state->start_since_epoch == 0)
        state->start_since_epoch = since_epoch;
    double secs = since_epoch - state->start_since_epoch;
    state->rideFile->appendPoint(secs, 0.0, 0.0,
                                 state->last_miles * KM_PER_MILE, 0.0,
                                 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                 RideFile::NA, RideFile::NA,
                                 0.0, 0.0, 0.0, 0.0,
                                 0.0,0.0,
                                 0.0, 0.0, 0.0, 0.0,
                                 0.0, 0.0, 0.0, 0.0,
                                 0.0,0.0,
                                 0.0,0.0,0.0, // running dynamic
                                 0.0, //tcore
                                 state->last_interval);
    state->last_secs = secs;
}

static void
data_cb(double secs, double nm, double mph, double watts, double miles, double alt,
        unsigned cad, unsigned hr, unsigned interval, void *context)
{
    if (nm < 0.0)    nm = 0.0;
    if (mph < 0.0)   mph = 0.0;
    if (watts < 0.0) watts = 0.0;

    ReadState *state = (ReadState*) context;
    state->rideFile->appendPoint(secs, cad, hr, miles * KM_PER_MILE,
                                 mph * KM_PER_MILE, nm, watts, alt, 0.0, 0.0, 0.0, 0.0, RideFile::NA, 0.0, 
                                 0.0,0.0,0.0,0.0, // pedal smooth/te
                                 0.0,0.0,
                                 0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,
                                 0.0,0.0,
                                 0.0,0.0,0.0,
                                 0.0, //tcore
                                 interval);
    state->last_secs = secs;
    state->last_miles = miles;
    state->last_interval = interval;
}

static void
error_cb(const char *msg, void *context)
{
    ReadState *state = (ReadState*) context;
    state->errors.append(QString(msg));
}

static void
pt_read_raw(FILE *in, void *context,
            void (*config_cb)(unsigned interval, double rec_int_secs,
                              unsigned wheel_sz_mm, void *context),
            void (*time_cb)(struct tm *time, time_t since_epoch, void *context),
            void (*data_cb)(double secs, double nm, double mph, double watts,
                            double miles, double alt, unsigned cad, unsigned hr,
                            unsigned interval, void *context),
            void (*error_cb)(const char *msg, void *context))
{
    unsigned interval = 0;
    unsigned last_interval = 0;
    unsigned wheel_sz_mm = 0;
    double rec_int_secs = 0.0;
    int i, n, row = 0;
    unsigned char buf[6];
    unsigned sbuf[6];
    double meters = 0.0;
    double secs = 0.0, start_secs = 0.0;
    double miles;
    double mph;
    double nm;
    double watts;
    double alt = 0;
    unsigned cad;
    unsigned hr;
    struct tm time;
    time_t since_epoch;
    char ebuf[256];
    bool bIsVer81 = false;

    while ((n = fscanf(in, "%x %x %x %x %x %x\n",
            sbuf, sbuf+1, sbuf+2, sbuf+3, sbuf+4, sbuf+5)) == 6) {
        ++row;
        for (i = 0; i < 6; ++i) {
            if (sbuf[i] > 0xff) { n = 1; break; }
            buf[i] = sbuf[i];
        }
        if (row == 1)
        {
            /* Serial number? */
            bIsVer81 = PowerTapUtil::is_Ver81(buf);
        }
        else if (PowerTapUtil::is_ignore_record(buf, bIsVer81)) {
            // do nothing
        }
        else if (PowerTapUtil::is_config(buf, bIsVer81)) {
            double new_rec_int_secs = 0.0;
            if (PowerTapUtil::unpack_config(buf, &interval, &last_interval,
                                        &new_rec_int_secs, &wheel_sz_mm, bIsVer81) < 0) {
                sprintf(ebuf, "Couldn't unpack config record.");
                if (error_cb) error_cb(ebuf, context);
                return;
            }
            if ((rec_int_secs != 0.0) && (rec_int_secs != new_rec_int_secs)) {
                sprintf(ebuf, "Ride has multiple recording intervals, which "
                        "is not yet supported.<br>(Recording interval changes "
                        "from %0.2f to %0.2f after %0.2f minutes of ride data.)\n",
                        rec_int_secs, new_rec_int_secs, secs / 60.0);
                if (error_cb) error_cb(ebuf, context);
                return;
            }
            rec_int_secs = new_rec_int_secs;
            if (config_cb) config_cb(interval, rec_int_secs, wheel_sz_mm, context);
        }
        else if (PowerTapUtil::is_time(buf, bIsVer81)) {
            since_epoch = PowerTapUtil::unpack_time(buf, &time, bIsVer81);
            bool ignore = false;
            if (start_secs == 0.0)
                start_secs = since_epoch;
            else if (since_epoch - start_secs > secs)
                secs = since_epoch - start_secs;
            else {
                sprintf(ebuf, "Warning: %0.3f minutes into the ride, "
                        "time jumps backwards by %0.3f minutes; ignoring it.",
                        secs / 60.0, (secs - since_epoch + start_secs) / 60.0);
                if (error_cb) error_cb(ebuf, context);
                ignore = true;
            }
            if (time_cb && !ignore) time_cb(&time, since_epoch, context);
        }
        else if (PowerTapUtil::is_data(buf, bIsVer81)) {
            if (wheel_sz_mm == 0) {
                sprintf(ebuf, "Read data row before wheel size set.");
                if (error_cb) error_cb(ebuf, context);
                return;
            }
            PowerTapUtil::unpack_data(buf, rec_int_secs, wheel_sz_mm, &secs,
                                      &nm, &mph, &watts, &meters, &cad, &hr, bIsVer81);
            miles = meters / 1000.0 * MILES_PER_KM;
            if (data_cb)
                data_cb(secs, nm, mph, watts, miles, alt, cad,
                        hr, interval, context);
        }
        else {
            sprintf(ebuf, "Unknown record type 0x%x on row %d.", buf[0], row);
            if (error_cb) error_cb(ebuf, context);
            return;
        }
    }
    if (n != -1) {
        sprintf(ebuf, "Parse error on row %d.", row);
        if (error_cb) error_cb(ebuf, context);
        return;
    }
}

RideFile *RawFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    RideFile *rideFile = new RideFile;
    rideFile->setDeviceType("PowerTap");
    rideFile->setFileFormat("GoldenCheetah Raw PowerTap (raw)");
    if (!file.open(QIODevice::ReadOnly)) {
        delete rideFile;
        return NULL;
    }
    // DUP added to avoid ocational crashs due to fclose on closed fd
    FILE *f = fdopen(DUP(file.handle()), "r");

    // failed to associate a stream!
    if (f==NULL) {
        file.close();
        delete rideFile;
        return NULL;
    }

    ReadState state(rideFile, errors);
    pt_read_raw(f, &state, config_cb, time_cb, data_cb, error_cb);
    file.close();
    // fclose can handle the file being closed already, QFile crashes on Windows
    // we need to do both because fclose needs to release its stream buffers and
    // fclose maintains local state that causes a crash when out of sync on Windows
    fclose(f);
    return rideFile;
}

