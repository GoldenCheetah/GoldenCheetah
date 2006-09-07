/* 
 * $Id: RawFile.cpp,v 1.2 2006/08/11 19:58:07 srhea Exp $
 *
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "RawFile.h"
#include <QMessageBox>
extern "C" {
#include "pt.h"
}

struct RawFileReadState
{
    QList<RawFilePoint*> &points;
    QStringList &errors;
    double last_secs, last_miles;
    unsigned last_interval;
    time_t start_since_epoch;
    unsigned rec_int;
    RawFileReadState(QList<RawFilePoint*> &points, QStringList &errors) : 
        points(points), errors(errors), last_secs(0.0), last_miles(0.0), 
        last_interval(0), start_since_epoch(0), rec_int(0) {}
};

static void 
config_cb(unsigned /*interval*/, unsigned rec_int, 
          unsigned /*wheel_sz_mm*/, void *context) 
{
    RawFileReadState *state = (RawFileReadState*) context;
    // Assume once set, rec_int should never change.
    assert((state->rec_int == 0) || (state->rec_int == rec_int));
    state->rec_int = rec_int;
}

static void
time_cb(struct tm *, time_t since_epoch, void *context)
{
    RawFileReadState *state = (RawFileReadState*) context;
    if (state->start_since_epoch == 0)
        state->start_since_epoch = since_epoch;
    double secs = since_epoch - state->start_since_epoch;
    RawFilePoint *point = new RawFilePoint(secs, -1.0, -1.0, -1.0, 
                                           state->last_miles, 0, 0, 
                                           state->last_interval);
    state->points.append(point);
    state->last_secs = secs;
}

static void
data_cb(double secs, double nm, double mph, double watts, double miles, 
        unsigned cad, unsigned hr, unsigned interval, void *context)
{
    RawFileReadState *state = (RawFileReadState*) context;
    RawFilePoint *point = new RawFilePoint(secs, nm, mph, watts, miles,
                                           cad, hr, interval);
    state->points.append(point);
    state->last_secs = secs;
    state->last_miles = miles;
    state->last_interval = interval;
}

static void
error_cb(const char *msg, void *context) 
{
    RawFileReadState *state = (RawFileReadState*) context;
    state->errors.append(QString(msg));
}

RawFile *RawFile::readFile(const QFile &file, QStringList &errors)
{
    RawFile *result = new RawFile(file.fileName());
    if (!result->file.open(QIODevice::ReadOnly)) {
        delete result;
        return NULL;
    }
    FILE *f = fdopen(result->file.handle(), "r");
    assert(f);
    RawFileReadState state(result->points, errors);
    pt_read_raw(f, 0 /* not compat */, &state, config_cb, 
                time_cb, data_cb, error_cb);
    result->rec_int = state.rec_int;
    return result;
}


