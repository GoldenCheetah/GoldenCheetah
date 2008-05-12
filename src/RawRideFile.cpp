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

#include "RawRideFile.h"
#include "PowerTap.h"
#include <assert.h>
#include <math.h>

#define MILES_TO_KM 1.609344
#define KM_TO_MI 0.62137119
#define BAD_KM_TO_MI 0.62

static int rawFileReaderRegistered = 
    RideFileFactory::instance().registerReader("raw", new RawFileReader());
 
struct ReadState
{
    RideFile *rideFile;
    QStringList &errors;
    double last_secs, last_miles;
    unsigned last_interval;
    time_t start_since_epoch;
    unsigned rec_int;
    ReadState(RideFile *rideFile,
                     QStringList &errors) : 
        rideFile(rideFile), errors(errors), last_secs(0.0), 
        last_miles(0.0), last_interval(0), start_since_epoch(0), rec_int(0) {}
};

static void 
config_cb(unsigned interval, unsigned rec_int, 
          unsigned wheel_sz_mm, void *context) 
{
    (void) interval;
    (void) wheel_sz_mm;
    ReadState *state = (ReadState*) context;
    // Assume once set, rec_int should never change.
    double recIntSecs = rec_int * 1.26;
    assert((state->rideFile->recIntSecs() == 0.0)
           || (state->rideFile->recIntSecs() == recIntSecs));
    state->rideFile->setRecIntSecs(recIntSecs);
}

static void
time_cb(struct tm *, time_t since_epoch, void *context)
{
    ReadState *state = (ReadState*) context;
    if (state->start_since_epoch == 0)
        state->start_since_epoch = since_epoch;
    double secs = since_epoch - state->start_since_epoch;
    state->rideFile->appendPoint(secs, 0.0, 0.0, 
                                 state->last_miles * MILES_TO_KM, 0.0, 
                                 0.0, 0.0, state->last_interval);
    state->last_secs = secs;
}

static void
data_cb(double secs, double nm, double mph, double watts, double miles, 
        unsigned cad, unsigned hr, unsigned interval, void *context)
{
    if (nm < 0.0)    nm = 0.0;
    if (mph < 0.0)   mph = 0.0;
    if (watts < 0.0) watts = 0.0;

    ReadState *state = (ReadState*) context;
    state->rideFile->appendPoint(secs, cad, hr, miles * MILES_TO_KM, 
                                 mph * MILES_TO_KM, nm, watts, interval);
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
pt_read_raw(FILE *in, int compat, void *context,
            void (*config_cb)(unsigned interval, unsigned rec_int, 
                              unsigned wheel_sz_mm, void *context),
            void (*time_cb)(struct tm *time, time_t since_epoch, void *context),
            void (*data_cb)(double secs, double nm, double mph, 
                            double watts, double miles, unsigned cad, 
                            unsigned hr, unsigned interval, void *context),
            void (*error_cb)(const char *msg, void *context))
{
    unsigned interval = 0;
    unsigned last_interval = 0;
    unsigned wheel_sz_mm = 0;
    unsigned rec_int = 0;
    int i, n, row = 0;
    unsigned char buf[6];
    unsigned sbuf[6];
    double meters = 0.0;
    double secs = 0.0, start_secs = 0.0;
    double miles;
    double mph;
    double nm;
    double watts;
    unsigned cad;
    unsigned hr;
    struct tm time;
    time_t since_epoch;
    char ebuf[256];

    while ((n = fscanf(in, "%x %x %x %x %x %x\n", 
            sbuf, sbuf+1, sbuf+2, sbuf+3, sbuf+4, sbuf+5)) == 6) {
        ++row;
        for (i = 0; i < 6; ++i) {
            if (sbuf[i] > 0xff) { n = 1; break; }
            buf[i] = sbuf[i];
        }
        if (row == 1) {
            /* Serial number? */
        }
        else if (PowerTap::is_config(buf)) {
            if (PowerTap::unpack_config(buf, &interval, &last_interval, 
                                        &rec_int, &wheel_sz_mm) < 0) {
                sprintf(ebuf, "Couldn't unpack config record.");
                if (error_cb) error_cb(ebuf, context);
                return;
            }
            if (config_cb) config_cb(interval, rec_int, wheel_sz_mm, context);
        }
        else if (PowerTap::is_time(buf)) {
            since_epoch = PowerTap::unpack_time(buf, &time);
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
        else if (PowerTap::is_data(buf)) {
            if (wheel_sz_mm == 0) {
                sprintf(ebuf, "Read data row before wheel size set.");
                if (error_cb) error_cb(ebuf, context);
                return;
            }
            PowerTap::unpack_data(buf, compat, rec_int, wheel_sz_mm, &secs, 
                                  &nm, &mph, &watts, &meters, &cad, &hr);
            if (compat)
                miles = round(meters) / 1000.0 * BAD_KM_TO_MI;
            else 
                miles = meters / 1000.0 * KM_TO_MI;
            if (data_cb) 
                data_cb(secs, nm, mph, watts, miles, cad, 
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

RideFile *RawFileReader::openRideFile(QFile &file, QStringList &errors) const 
{
    RideFile *rideFile = new RideFile;
    if (!file.open(QIODevice::ReadOnly)) {
        delete rideFile;
        return NULL;
    }
    FILE *f = fdopen(file.handle(), "r");
    assert(f);
    ReadState state(rideFile, errors);
    pt_read_raw(f, 0 /* not compat */, &state, config_cb, 
                time_cb, data_cb, error_cb);
    return rideFile;
}

