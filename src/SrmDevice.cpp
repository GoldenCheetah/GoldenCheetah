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
#include <srmio.h>
#include <QMessageBox>
#include <boost/scoped_ptr.hpp>
#include <boost/foreach.hpp>
#include <errno.h>
#include <stdio.h>

#define tr(s) QObject::tr(s)

static bool srm5Registered =
    Devices::addType("SRM PCV", DevicesPtr(new SrmDevices( 5 )) );
static bool srm7Registered =
    Devices::addType("SRM PCVI/7", DevicesPtr(new SrmDevices( 7 )));

static Device::StatusCallback cb;

DevicePtr
SrmDevices::newDevice( CommPortPtr dev )
{
    return DevicePtr( new SrmDevice( dev, protoVersion));
}

static void progress( size_t total, size_t done, void *user_data )
{
    (void)user_data;

    // XXX: better way to pass statusCallback - that's what we have
    // "user_data" for...
    cb( tr("progress: %1/%1").arg(done).arg(total) );
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

SrmDevice::~SrmDevice()
{
    close();
}

bool
SrmDevice::open( QString &err )
{
    if( dev->type() == "Serial" ){
        io = srmio_ios_new( dev->name().toAscii().constData() );
        if( ! io ){
            err = tr("failed to allocate device handle: %1")
                .arg(strerror(errno));
            return false;
        }

    } else {
        err = tr("device type %1 is unsupported")
            .arg(dev->type());
        return false;
    }

    switch( protoVersion ){
      case 5:
        pc = srmio_pc5_new();
        break;

      case 6:
      case 7:
        pc = srmio_pc7_new();
        break;

      default:
        err = tr("unsupported SRM Protocl version: %1")
            .arg(protoVersion);
        goto fail;
    }
    if( ! pc ){
        err = tr("failed to allocate Powercontrol handle: %1")
            .arg(strerror(errno));
        goto fail;
    }

    if( ! srmio_io_open(io ) ){
        err = tr("Couldn't open device %1: %2")
            .arg(dev->name())
            .arg(strerror(errno));
        goto fail;
    }

    if( ! srmio_pc_set_device( pc, io ) ){
        err = tr("failed to set Powercontrol io handle: %1")
            .arg(strerror(errno));
        goto fail;
    }

    if( ! srmio_pc_open( pc ) ){
        err = tr("failed to initialize Powercontrol communication: %1")
            .arg(strerror(errno));
        goto fail;
    }

    is_open = true;
    return true;

fail:
    if( pc ){
        srmio_pc_free( pc );
        pc = NULL;
    }

    if( io ){
        srmio_io_free( io );
        io = NULL;
    }

    return false;
}

bool
SrmDevice::close( void )
{
    if( pc ){
        srmio_pc_free( pc );
        pc = NULL;
    }

    if( io ){
        srmio_io_free( io );
        io = NULL;
    }

    is_open = false;
    return true;
}

bool
SrmDevice::download(const QDir &tmpdir,
                    QString &tmpname, QString &filename,
                    StatusCallback statusCallback, QString &err)
{
    srmio_data_t data( NULL );
    FILE *fh( NULL );
    QDateTime startTime;

    cb = statusCallback;

    if (!get_tmpname(tmpdir, tmpname, err))
        return false;

    if( ! is_open ){
        statusCallback( tr("opening device %1").arg(dev->name()) );
        if( ! open( err ) )
            return false;
    }

    data = srmio_data_new();
    if( ! data ){
        err = tr("failed to allocate data handle: %1")
            .arg(strerror(errno));
        goto fail;
    }

    if( ! srmio_pc_xfer_all( pc, data, progress, NULL ) ){
        err = tr( "failed to download data from Powercontrol: %1")
            .arg(strerror(errno));
        goto fail;
    }

    statusCallback( tr("got %1 records").arg(data->cused) );

    if( ! data->cused ){
        err = tr("no data available");
        goto fail;
    }

    fh = fopen( tmpname.toAscii().constData(), "w" );
    if( ! fh ){
        err = tr( "failed to open file %1: %2")
            .arg(tmpname)
            .arg(strerror(errno));
        goto fail;
    }

    if( ! srmio_file_srm7_write(data, fh) ){
        err = tr("Couldn't write to file %1: %2")
            .arg(tmpname)
            .arg(strerror(errno));
        goto fail;
    }

    startTime.setTime_t( 0.1 * data->chunks[0]->time );
    filename = startTime.toString("yyyy_MM_dd_hh_mm_ss") + ".srm";

    srmio_data_free( data );
    fclose( fh );
    return true;

fail:
    if( data ) srmio_data_free( data );
    if( fh ) fclose( fh );
    close();
    return false;
}

void
SrmDevice::cleanup()
{
    QString err;

    if (QMessageBox::question(0, "Powercontrol",
                              "Erase ride from device memory?",
                              "&Erase", "&Cancel", "", 1, 1) != 0)
        return;


    if( ! is_open ){
        if( ! open( err ) )
            goto cleanup;
    }

    if( ! srmio_pc_cmd_clear( pc ) ){
        err = tr("failed to clear Powercontrol memory: %1")
            .arg(strerror(errno));
        goto cleanup;
    }

    return;

cleanup:
    if( err.length() ){
        QMessageBox::warning(0, "Error", err );
        close();
    }
}

