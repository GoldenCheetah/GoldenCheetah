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

#ifndef _VideoSyncFileBase_h
#define _VideoSyncFileBase_h

#include <QString>


enum class VideoSyncFileFormat {
    unknown = 0,
    rlv,
    gpx
};


class VideoSyncFileBase
{
    public:
        VideoSyncFileBase();
        ~VideoSyncFileBase();

        void videoFrameRate(double videoFrameRate);
        double videoFrameRate() const;

        void format(VideoSyncFileFormat format);
        VideoSyncFileFormat format() const;

        void version(QString version);
        QString version() const;

        void units(QString units);
        QString units() const;

        void filename(QString filename);
        QString filename() const;

        void originalFilename(QString originalFilename);
        QString originalFilename() const;

        void name(QString name);
        QString name() const;

        void source(QString source);
        QString source() const;

        void duration(long duration);
        long duration() const;

        void distance(double distance);
        double distance() const;

    private:
        double _VideoFrameRate;

        VideoSyncFileFormat _format;    // RLV or GPX currently supported

        QString _Version;       // version number / identifer
        QString _Units;         // units used
        QString _OriginalFilename;      // filename from inside file
        QString _filename;      // filename on disk
        QString _Name;          // description in file
        QString _Source;        // where did this come from

        long _Duration;         // Duration of this workout in msecs
        double _Distance;       // Distance of this workout in km
};

#endif
