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

#include "SrmDevice.h"
#include "SrmRideFile.h"
#include "srmio/srmio.h"
#include <boost/scoped_ptr.hpp>
#include <boost/foreach.hpp>
#include <errno.h>

static bool srmRegistered = Device::addDevice("SRM", new SrmDevice());

QString
SrmDevice::downloadInstructions() const
{
    return ""; // no particular instructions for SRM
}

static Device::StatusCallback cb;

static void
logfunc(const char *msg)
{
    // XXX: better log function
    // fprintf(stderr, "%s\n", msg);
    cb(QString(msg).left(40));
}

static bool
get_tmpname(const QDir &tmpdir, QString &tmpname, QString &err)
{
    QString tmpl = tmpdir.absoluteFilePath(".srmdl.XXXXXX.srm");
    QTemporaryFile tmp(tmpl);
    tmp.setAutoRemove(false);
    if (!tmp.open()) {
        err = "Failed to create temporary file "
            + tmpl + ": " + tmp.error();
        return false;
    }
    tmpname = tmp.fileName(); // after close(), tmp.fileName() is ""
    tmp.close();
    return true;
}

bool
SrmDevice::download(CommPortPtr dev, const QDir &tmpdir,
                    QString &tmpname, QString &filename,
                    StatusCallback statusCallback, QString &err)
{
    // Totally ghetto, proof-of-concept integration with srmio.
    cb = statusCallback;
    // Read device path out of device name.  Sketchy.
    QRegExp rx("^Serial: (.+)$");
    if (!rx.exactMatch(dev->name())) {
        err = "SRM download not supported by device " + dev->name();
        return false;
    }
    QString path = rx.cap(1);
    if (!get_tmpname(tmpdir, tmpname, err))
        return false;
    srmpc_conn_t srm;
    int opt_force = 0; // setting this to 1 is potentially dangerous
    srm = srmpc_open(path.toAscii().constData(), opt_force, logfunc);
    if (!srm) {
        err = "Couldn't open device " + path + ": " + strerror(errno);
        return false;
    }
    int opt_all = 0; // XXX: what does this do?
    int opt_fixup = 1; // fix bad data like srmwin.exe does
    srm_data_t srmdata = srmpc_get_data(srm, opt_all, opt_fixup);
    if (!srmdata) {
        err = "srmpc_get_data failed";
        srmpc_close(srm);
        return false;
    }
    if (srm_data_write_srm7(srmdata, tmpname.toAscii().constData()) < 0)
        err = "Couldn't write to file " + tmpname + ": " + strerror(errno);
    srm_data_free(srmdata);
    srmpc_close(srm);
    if (err.size() > 0)
        return false;
    // Read it back in to get the ride start time.
    SrmFileReader reader;
    QStringList errs;
    QFile file(tmpname);
    QStringList errors;
    boost::scoped_ptr<RideFile> ride(
        RideFileFactory::instance().openRideFile(file, errors));
    BOOST_FOREACH(QString err, errors) {
        fprintf(stderr, "error: %s\n", err.toAscii().constData());
    }
    filename = ride->startTime().toString("yyyy_MM_dd_hh_mm_ss") + ".srm";
    return true;
}

