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

#define tr(s) QObject::tr(s)

static bool srm5Registered =
    Devices::addType("SRM PCV", DevicesPtr(new SrmDevices( 5 )) );
static bool srm7Registered =
    Devices::addType("SRM PCVI/7", DevicesPtr(new SrmDevices( 7 )));

DevicePtr
SrmDevices::newDevice( CommPortPtr dev, Device::StatusCallback cb )
{
    return DevicePtr( new SrmDevice( dev, cb, protoVersion));
}

bool
SrmDevices::supportsPort( CommPortPtr dev )
{
#if defined(SRMIO_HAVE_TERMIOS) || defined(SRMIO_HAVE_WINCOM)
    // XXX: check device name starts with "com" or "/dev"?
    if( dev->type() == "Serial" )
        return true;
#endif

#ifdef SRMIO_HAVE_D2XX
    if( dev->type() == "D2XX" ){
        switch( protoVersion ){
          case 6:
          case 7:
            return true;
        }
    }
#endif

    return false;
}

bool
SrmDevices::exclusivePort( CommPortPtr dev )
{
    switch( protoVersion ){
      case 5:
        // XXX: this has to go, once we have other devices using prolific
        if( dev->type() == "Serial" && dev->name().contains( "PL2303" ) )
            return true;
        break;

      case 6:
      case 7:
        if( dev->type() == "D2XX" && dev->name().startsWith( "POWERCONTROL" ) )
            return true;
        break;
    }

    return false;
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
    srmio_error_t serr;
    unsigned firmware;
    srmio_io_baudrate_t baudrateId;
    unsigned baudrate;

    if( dev->type() == "Serial" ){

        // XXX: check device name starts with "com" or "/dev"?

#ifdef SRMIO_HAVE_WINCOM
        io = srmio_iow32_new( dev->name().toAscii().constData(), &serr );
        if( ! io ){
            err = tr("failed to allocate device handle: %1")
                .arg(serr.message);
            return false;
        }
#elif defined(SRMIO_HAVE_TERMIOS)
        io = srmio_ios_new( dev->name().toAscii().constData(), &serr );
        if( ! io ){
            err = tr("failed to allocate device handle: %1")
                .arg(serr.message);
            return false;
        }
#else
        err = tr("device type %1 is unsupported")
            .arg(dev->type());
        return false;
#endif

    } else if( dev->type() == "D2XX" ){
#ifdef SRMIO_HAVE_D2XX
        io = srmio_d2xx_description_new(
            dev->name().toAscii().constData(), &serr );
        if( ! io ){
            err = tr("failed to allocate device handle: %1")
                .arg(serr.message);
            return false;
        }
#else
        err = tr("device type %1 is unsupported")
            .arg(dev->type());
        return false;
#endif

    } else {
        err = tr("device type %1 is unsupported")
            .arg(dev->type());
        return false;
    }

    switch( protoVersion ){
      case 5:
        statusCallback( tr("opening PCV at %1").arg(dev->name()) );
        pc = srmio_pc5_new( &serr );
        break;

      case 6:
      case 7:
        statusCallback( tr("opening PC6/7 at %1").arg(dev->name()) );
        pc = srmio_pc7_new( &serr );
        break;

      default:
        err = tr("unsupported SRM Protocl version: %1")
            .arg(protoVersion);
        goto fail;
    }
    if( ! pc ){
        err = tr("failed to allocate Powercontrol handle: %1")
            .arg(serr.message);
        goto fail;
    }

    if( ! srmio_io_open( io, &serr ) ){
        err = tr("Couldn't open device %1: %2")
            .arg(dev->name())
            .arg(serr.message);
        goto fail;
    }

    if( ! srmio_pc_set_device( pc, io, &serr ) ){
        err = tr("failed to set Powercontrol io handle: %1")
            .arg(serr.message);
        goto fail;
    }

    if( ! srmio_pc_open( pc, &serr ) ){
        err = tr("failed to initialize Powercontrol communication: %1")
            .arg(serr.message);
        goto fail;
    }

    if( ! srmio_pc_get_version( pc, &firmware, &serr ) ){
        err = tr("failed to get firmware version: %1")
            .arg(serr.message);
        goto fail;
    }

    if( ! srmio_pc_get_baudrate( pc, &baudrateId, &serr ) ){
        err = tr("failed to get baud rate: %1")
            .arg(serr.message);
        goto fail;
    }

    if( ! srmio_io_baud2name( baudrateId, &baudrate ) ){
        err = tr("failed to get baud rate name: %1")
            .arg(serr.message);
    }

    statusCallback(tr("found Powercontrol version 0x%1 using %2 baud")
        .arg(firmware, 4, 16 )
        .arg(baudrate));

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
    rideList.clear();

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
SrmDevice::preview( QString &err )
{
    srmio_error_t serr;
    struct _srmio_pc_xfer_block_t block;

    if( ! is_open ){
        if( ! open( err ) )
            return false;
    }

    rideList.clear();

    if( ! srmio_pc_can_preview( pc ) )
        // nothing to do
        return true;

    if( ! srmio_pc_xfer_start( pc, &serr ) ){
        err = tr("failed to start download: %1")
            .arg(serr.message);
        goto fail;
    }

    while( srmio_pc_xfer_block_next( pc, &block )){
        DeviceRideItemPtr ride( new DeviceRideItem );

        if( block.start )
            ride->startTime.setTime_t( 0.1 * block.start );
        if( block.end )
            ride->startTime.setTime_t( 0.1 * block.end );
        ride->work = block.total;
        rideList.append( ride );

        if( block.athlete )
            free( block.athlete );
        block.athlete = NULL;
    }

    if( ! srmio_pc_xfer_finish( pc, &serr ) ){
        err = tr("download failed: %1")
            .arg(serr.message);
        goto fail;
    }


    return true;

fail:
    rideList.clear();
    return false;
}

bool
SrmDevice::download( const QDir &tmpdir,
                    QList<DeviceDownloadFile> &files,
                    CancelCallback cancelCallback,
                    ProgressCallback progressCallback,
                    QString &err)
{
    srmio_error_t serr;
    struct _srmio_pc_xfer_block_t block;
    srmio_data_t data( NULL );
    srmio_data_t *splitList( NULL );
    srmio_data_t *split;
    int mfirst( -1 );
    size_t block_cnt, block_num( 0 );
    size_t prog_sum( 0 ), prog_prev( 0 );
    size_t chunks_done( 0 );
    srmio_time_t splitGap( 72000 ); // 2h - XXX: make this configurable

    if( ! is_open ){
        if( ! open( err ) )
            return false;
    }

    // fetch preview in case user didn't
    if( srmio_pc_can_preview(pc) && rideList.size() == 0 ){
        if( ! preview( err ) )
            return false;
    }

    data = srmio_data_new( &serr );
    if( ! data ){
        err = tr("failed to allocate data handle: %1")
            .arg(serr.message);
        goto fail;
    }

    if( cancelCallback() ){
        err = tr("download cancelled");
        goto fail;
    }

    if( ! srmio_pc_xfer_start( pc, &serr )){
        err = tr("failed to start download: %1")
            .arg(serr.message);
        goto fail;
    }

    if( ! srmio_pc_xfer_get_blocks( pc, &block_cnt, &serr ) ){
        err = tr("failed to get number of data blocks: %1")
            .arg(serr.message);
        goto fail1;
    }
    statusCallback(tr("found %1 ride blocks").arg(block_cnt));

    for( int i = 0; i < rideList.size(); ++i ){
        if( rideList.at(i)->wanted )
            prog_sum += rideList.at(i)->work;
    }

    while( srmio_pc_xfer_block_next( pc, &block )){
        bool wanted = false;
        struct _srmio_chunk_t chunk;
        bool is_int;
        bool is_first;
        size_t prog_total;

        if( rideList.empty() ){
            wanted = true;

        } else {
            for( int i = 0; i < rideList.size(); ++i ){
                if( rideList.at(i)->startTime.toTime_t() == block.start / 10 ){
                    wanted = rideList.at(i)->wanted;
                    break;
                }
            }
        }

        if( ! wanted ){
            statusCallback(tr("skipping unselected ride block %1")
                .arg(block_num +1));
            continue;
        }
        statusCallback(tr("downloading ride block %1/%2")
            .arg(block_num +1)
            .arg(block_cnt) );

        data->slope = block.slope;
        data->zeropos = block.zeropos;
        data->circum = block.circum;
        if( block.athlete ){
            if( data->athlete )
                free( data->athlete );
            data->athlete = strdup( block.athlete );
        }

        if( ! rideList.empty() ){
            prog_total = prog_sum;

        } else if( block_cnt == 1 ){
            prog_total = block.total;

        } else {
            prog_total = block_cnt * 1000;
        }

        while( srmio_pc_xfer_chunk_next( pc, &chunk, &is_int, &is_first  ) ){
            if( cancelCallback() ){
                err = tr("download cancelled");
                goto fail1;
            }

            if( chunks_done % 16 == 0 ){
                size_t block_done;

                srmio_pc_xfer_block_progress( pc, &block_done );
                if( ! rideList.empty() ){
                    block_done += prog_prev;

                } else if( block_cnt == 1 ){
                    // unchanged

                } else {
                    block_done = (double)block_num * 1000
                        + 1000 * block.total / block_done;
                }

                progressCallback( tr("progress: %1/%2")
                    .arg(block_done)
                    .arg(prog_total));
            }

            if( ! srmio_data_add_chunk( data, &chunk, &serr ) ){
                err = tr("adding chunk failed: %1")
                    .arg(serr.message);
                goto fail1;
            }

            ++chunks_done;

            /* finish previous marker */
            if( mfirst >= 0 && ( ! is_int || is_first ) )
                if( ! srmio_data_add_marker( data, mfirst, data->cused -1, &serr ) ){
                    err = tr("adding marker failed: %1")
                        .arg(serr.message);
                    goto fail1;
                }

            /* start marker */
            if( is_first ){
                mfirst = (int)data->cused;

            } else if( ! is_int ){
                mfirst = -1;

            }
        }

        /* finalize marker at block end */
        if( mfirst >= 0 ){
            if( ! srmio_data_add_marker( data, mfirst, data->cused -1, &serr ) ){;
                err = tr("adding marker failed: %1")
                  .arg(serr.message);
                goto fail1;
            }
            mfirst = -1;
        }

        if( ! rideList.empty() )
            prog_prev += block.total;
        else
            prog_prev += 1000;

        if( block.athlete )
            free( block.athlete );
        block.athlete = NULL;

        ++block_num;
    }

    if( ! srmio_pc_xfer_finish( pc, &serr ) ){
        err = tr( "download failed: %1")
            .arg(serr.message);
        goto fail;
    }

    statusCallback( tr("got %1 records").arg(data->cused) );

    if( cancelCallback() ){
        err = tr("download cancelled");
        goto fail;
    }

    if( ! data->cused ){
        err = tr("no data available");
        goto fail;
    }

    splitList = srmio_data_split( data, splitGap, 1000, &serr );
    if( ! splitList ){
        err = tr("Couldn't split data: %1")
            .arg(serr.message);
        goto fail;
    }

    for( split = splitList; *split; ++split ){
        FILE *fh( NULL );
        srmio_time_t stime;
        srmio_data_t fixed;
        DeviceDownloadFile file;

        file.extension = "srm";

        if (!get_tmpname(tmpdir, file.name, err))
            goto fail;

        fixed = srmio_data_fixup( *split, &serr );
        if( ! fixed ){
            err = tr("Couldn't fixup data: %1")
                .arg(serr.message);
            goto fail;
        }

        if( ! srmio_data_time_start( fixed, &stime, &serr ) ){
            srmio_data_free(fixed);
            err = tr("Couldn't get start time of data: %1")
                .arg(serr.message);
            goto fail;
        }
        file.startTime.setTime_t( 0.1 * stime );

        fh = fopen( file.name.toAscii().constData(), "w" );
        if( ! fh ){
            srmio_data_free(fixed);
            err = tr( "failed to open file %1: %2")
                .arg(file.name)
                .arg(strerror(errno));
            goto fail;
        }

        if( ! srmio_file_srm7_write(fixed, fh, &serr) ){
            srmio_data_free(fixed);
            err = tr("Couldn't write to file %1: %2")
                .arg(file.name)
                .arg(serr.message);
            fclose(fh);
            goto fail;
        }

        files.append(file);

        fclose( fh );
        srmio_data_free(fixed);

    }

    for( split = splitList; *split; ++split )
        srmio_data_free( *split );
    free(splitList);

    srmio_data_free( data );
    return true;

fail1:
    srmio_pc_xfer_finish(pc, NULL);

fail:
    if( data ) srmio_data_free( data );
    if( splitList ){
        for( split = splitList; *split; ++split )
            srmio_data_free( *split );
        free(splitList);
    }
    close();
    return false;
}

bool
SrmDevice::cleanup( QString &err )
{
    srmio_error_t serr;

    if( ! is_open ){
        if( ! open( err ) )
            goto cleanup;
    }

    statusCallback( tr("cleaning device ..."));

    if( ! srmio_pc_cmd_clear( pc, &serr ) ){
        err = tr("failed to clear Powercontrol memory: %1")
            .arg(serr.message);
        goto cleanup;
    }

    return true;

cleanup:
    close();
    return false;
}

