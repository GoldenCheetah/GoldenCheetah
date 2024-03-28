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

#ifndef _VideoSyncFile_h
#define _VideoSyncFile_h
#include "GoldenCheetah.h"
#include "Context.h"
#include "VideoSyncFileBase.h"

#include <QtGui>
#include <QObject>
#include <QDebug>
#include <QString>
#include <QDate>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QRegExp>

// which section of the file are we in?
#define NOMANSLAND  0
#define SETTINGS 1
#define DATA     2
#define END      3

// is this in .rlv or .gpx format?
#define RLV     1
#define GPX     2

class VideoSyncFilePoint
{
    public:

        VideoSyncFilePoint() : km(0), secs(0) {}

        double km;       // x axis - distance in km
        double secs;     // t axis - time in seconds
        double kph;      // speed in km per hour
};

class VideoSyncCourseInfo
{
    public:

        VideoSyncCourseInfo() : start(0), end(0) {}

        double start, end;     // segment start/end in seconds
        QString SegmentName;
        QString TextFile;
};

class VideoSyncFile : public VideoSyncFileBase
{
    public:
        VideoSyncFile(QString, int&, Context *context);       // constructor uses filename
        VideoSyncFile(Context *context); // no filename, going to use a string

        ~VideoSyncFile();             // delete the contents

        static bool isVideoSync(QString); // is this a supported videosync?

        void reload();                    // reload after messed about
        void parseRLV();                  // its a rlv file
        void parseTTS();                  // its a tts file
        void parseFromRideFileFactory();  // try an skrimp video sync info from a ride file.
        bool isValid() const;             // is the file valid or not?

        bool    valid;          // did it parse ok?

        QVector<VideoSyncFilePoint> Points;    // points in workout

        Context *context;
};

#endif
