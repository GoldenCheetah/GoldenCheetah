/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2015 Vianney Boyer   (vlcvboyer@gmail.com)
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

#include "VideoSyncFileBase.h"


VideoSyncFileBase::VideoSyncFileBase
() : _VideoFrameRate(0), _format(VideoSyncFileFormat::unknown), _Version(), _Units(), _OriginalFilename(), _filename(), _Name(), _Source(),
     _Duration(0), _Distance(0)
{
}


VideoSyncFileBase::~VideoSyncFileBase
()
{
}


void
VideoSyncFileBase::videoFrameRate
(double videoFrameRate)
{
    _VideoFrameRate = videoFrameRate;
}


double
VideoSyncFileBase::videoFrameRate
() const
{
    return _VideoFrameRate;
}


void
VideoSyncFileBase::format
(VideoSyncFileFormat format)
{
    _format = format;
}


VideoSyncFileFormat
VideoSyncFileBase::format
() const
{
    return _format;
}


void
VideoSyncFileBase::version
(QString version)
{
    _Version = version;
}


QString
VideoSyncFileBase::version
() const
{
    return _Version;
}


void
VideoSyncFileBase::units
(QString units)
{
    _Units = units;
}


QString
VideoSyncFileBase::units
() const
{
    return _Units;
}


void
VideoSyncFileBase::filename
(QString filename)
{
    _filename = filename;
}


QString
VideoSyncFileBase::filename
() const
{
    return _filename;
}


void
VideoSyncFileBase::originalFilename
(QString originalFilename)
{
    _OriginalFilename = originalFilename;
}


QString
VideoSyncFileBase::originalFilename
() const
{
    return _OriginalFilename;
}


void
VideoSyncFileBase::name
(QString name)
{
    _Name = name;
}


QString
VideoSyncFileBase::name
() const
{
    return _Name;
}


void
VideoSyncFileBase::source
(QString source)
{
    _Source = source;
}


QString
VideoSyncFileBase::source
() const
{
    return _Source;
}


void
VideoSyncFileBase::duration
(long duration)
{
    _Duration = duration;
}


long
VideoSyncFileBase::duration
() const
{
    return _Duration;
}


void
VideoSyncFileBase::distance
(double distance)
{
    _Distance = distance;
}


double
VideoSyncFileBase::distance
() const
{
    return _Distance;
}
