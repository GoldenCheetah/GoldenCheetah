/* 
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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
#include "pt.h"
#include <assert.h>
#include <math.h>

#define MILES_TO_KM 1.609344
        
static int rawFileReaderRegistered = 
    CombinedFileReader::instance().registerReader("raw", new RawFileReader());
 
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

