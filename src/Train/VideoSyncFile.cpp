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

#include "VideoSyncFile.h"

#include <stdint.h>
#include "Units.h"

#include "TTSReader.h"

// Supported file types
static QStringList supported;
static bool setSupported()
{
    ::supported << ".rlv";
    ::supported << ".tts";
//TODO    ::supported << ".gpx";
    return true;
}
static bool isinit = setSupported();
bool VideoSyncFile::isVideoSync(QString name)
{
    foreach(QString extension, supported) {
        if (name.endsWith(extension, Qt::CaseInsensitive))
            return true;
    }
    return false;
}
VideoSyncFile::VideoSyncFile(QString filename, int& /*mode*/, Context *context) :
    filename(filename), context(context)
{
    reload();
}

VideoSyncFile::VideoSyncFile(Context *context) : context(context)
{
    filename ="";
}

void VideoSyncFile::reload()
{
    // which parser to call?
    if (filename.endsWith(".rlv", Qt::CaseInsensitive)) parseRLV();
    if (filename.endsWith(".tts", Qt::CaseInsensitive)) parseTTS();
//TODO    else if (filename.endsWith(".gpx", Qt::CaseInsensitive)) parseGPX();
}

void VideoSyncFile::parseTTS()
{
    // Initialise
    manualOffset = 0;
    Version = "";
    Units = "";
    Filename = "";
    Name = "";
    Duration = -1;
    valid = false;  // did it parse ok as sync file?
    format = RLV;
    Points.clear();

    QFile ttsFile(filename);
    if (ttsFile.open(QIODevice::ReadOnly)) {

        QStringList errors_;

        QDataStream qttsStream(&ttsFile);
        qttsStream.setByteOrder(QDataStream::LittleEndian);

        NS_TTSReader::TTSReader ttsReader;
        bool success = ttsReader.parseFile(qttsStream);
        if (success) {

            // -----------------------------------------------------------------
            // VideoFrameRate
            VideoFrameRate = ttsReader.getFrameRate();

            // -----------------------------------------------------------------
            // VideoSyncFilePoint
            const std::vector<NS_TTSReader::Point> &ttsPoints = ttsReader.getPoints();
            if (ttsReader.hasFrameMapping()) {

                NS_TTSReader::Point fakeFirstPoint;

                fakeFirstPoint = ttsPoints[1];
                fakeFirstPoint.setDistanceFromStart(0);
                fakeFirstPoint.setTime(0);

                size_t pointCount = ttsPoints.size();
                for (size_t i = 0; i < pointCount; i++) {

                    const NS_TTSReader::Point &point = ttsPoints[i];

                    VideoSyncFilePoint add;

                    // distance
                    add.km = point.getDistanceFromStart() / 1000.0;

                    // time
                    add.secs = point.getTime() / 1000.;

                    // speed
                    add.kph = point.getSpeed();

                    Points.append(add);
                }

                Duration = Points.last().secs * 1000.0;
                Distance = Points.last().km;

                valid = true;

            } // hasFrameMapping

        } // parseFile

        ttsFile.close();

    } // ttsFile.open
}

void VideoSyncFile::parseRLV()
{
    // Initialise
    manualOffset = 0;
    Version = "";
    Units = "";
    Filename = "";
    Name = "";
    Duration = -1;
    valid = false;             // did it parse ok?
    format = RLV; // default to rlv until we know
    Points.clear();

    // running totals
    double rdist = 0; // running total for distance
    double rend = 0;  // Length of entire course

    // open the file for binary reading and open a datastream
    QFile RLVFile(filename);
    if (RLVFile.open(QIODevice::ReadOnly) == false) return;
    QDataStream input(&RLVFile);
    input.setByteOrder(QDataStream::LittleEndian);
    input.setVersion(QDataStream::Qt_4_0); // 32 bit floats not 64 bit.

    bool happy = true; // are we ok to continue reading?

    //
    // BASIC DATA STRUCTURES
    //
    struct {
        uint16_t fingerprint;
        uint16_t version;
        uint32_t blocks;
    } header; // file header

    struct {
        uint16_t type;
        uint16_t version;
        uint32_t records;
        uint32_t recordSize;
    } info; // tells us what to read

    struct {
        quint8 videoFileName[522];
        float  frameRate;
        uint32_t orgrunWeight;
        uint32_t frameOffset;
    } videoInfo; // type 2010

    struct {
        uint32_t frameNbr;
        float distance;
    } framemapping; // type 2020

    struct {
        uint32_t frame;
        uint32_t cmd;
    } blockInfo; // type 2030

    struct {
        float  start;
        float  end;
        quint8 segmentName[66];
        quint8 textFile[522];
    } courseInfo; // type 2040


    //
    // FILE HEADER
    //
    int rc = input.readRawData((char*)&header, sizeof(header));
    if (rc == sizeof(header)) {
        if (header.fingerprint == 2000) happy = true; //FIXME  && header.version == ???
        else happy = false;
    } else happy = false;

    unsigned int block = 0; // keep track of how many blocks we have read

    //
    // READ THE BLOCKS INSIDE THE FILE
    //
    while (happy && block < header.blocks) {

        // read the info for this block
        rc = input.readRawData((char*)&info, sizeof(info));
        if (rc == sizeof(info)) {

            // okay now read that block
            switch (info.type) {

            case 2010 : // videoInfo
                {
                    // we read member by member to avoid struct word alignment problem caused
                    // by the string length
// FIXME ?                    input>>general.checksum;
                    input.readRawData((char*)&videoInfo.videoFileName[0], 522);
                    input>>videoInfo.frameRate;
                    input>>videoInfo.orgrunWeight;
                    input>>videoInfo.frameOffset;

                    VideoFrameRate = videoInfo.frameRate;
                }
                break;

            case 2020 : // frame mapping
                {
                    // read in the mapping records
                    uint32_t PreviousFrameNbr=0;
                    double PreviousDistance=0;

                    for (unsigned int record=0; record < info.records; record++) {
                        // get the next record
                        if (sizeof(framemapping) != input.readRawData((char*)&framemapping, sizeof(framemapping))) {
                            happy = false;
                            break;
                        }

                        if (record == 0) PreviousDistance = framemapping.distance;
                        VideoSyncFilePoint add;

                        if (format == RLV) {
                            // Calculate average distance per video frame between this sync point and the last
                            // using the formula (2a+b)/3 where a is the point of lower speed and b is the higher.
                            double distance = framemapping.distance;
                            double avgDistance = (qMin(distance, PreviousDistance)*2 + qMax(distance, PreviousDistance)) / 3;

                            rdist += avgDistance * (double) (framemapping.frameNbr - PreviousFrameNbr);
                            add.km = rdist / 1000.0;

                            // time
                            add.secs = (float) framemapping.frameNbr / VideoFrameRate; //FIXME : are we sure that video frame rate will be given prior to mapping?

                            // speed
                            add.kph = (distance / 1000.0) * (3600.0 * VideoFrameRate);

                            // FIXME : do we have to consider video offset and weight ?
                            PreviousFrameNbr = framemapping.frameNbr;
                            PreviousDistance = distance;
                        }

                        Points.append(add);

                    }
                }
                break;


            case 2030 : // rlv info
                {
                    // read in the mapping records
                    for (unsigned int record=0; record < info.records; record++) {

                        // get the next record
                        if (sizeof(blockInfo) != input.readRawData((char*)&blockInfo, sizeof(blockInfo))) {
                            happy = false;
                            break;
                        }
                    }
                }
                break;


            case 2040 : // course info
                {
                    // read in the mapping records
                    for (unsigned int record=0; record < info.records; record++) {

                        // get the next record
                        if (sizeof(courseInfo) != input.readRawData((char*)&courseInfo, sizeof(courseInfo))) {
                            happy = false;
                            break;
                        }
                        if (courseInfo.end > rend) rend = courseInfo.end;
                    }
                    // FIXME : add those data to training messages
                }
                break;


            default: // unexpected block type
                happy = false;
                break;
            }

            block++;

        } else happy = false;
    }

    // Make sure we have a sync point to represent the end of the course
    // Some earlier rlvs don't supply an end point frame reference,
    // so we need to assume the speed from the last ref point continues to the end
    if ((!Points.isEmpty()) && (rend > Points.last().km * 1000) && Points.last().kph > 0) {
        VideoSyncFilePoint add;
        add.km = rend / 1000;
        add.kph = Points.last().kph;
        add.secs = Points.last().secs + ((add.km - Points.last().km) * 3600) / add.kph;
        Points.append(add);
    }

    // done
    RLVFile.close();

    // if we got here and are still happy then it
    // must have been a valid file.
    if (happy) {
        valid = true;

        // set RLVFile duration
        Duration = Points.last().secs * 1000.0;      // last is the end point in msecs
        Distance = Points.last().km;
    }
}


VideoSyncFile::~VideoSyncFile()
{
    Points.clear();
}


bool
VideoSyncFile::isValid()
{
    return valid;
}
