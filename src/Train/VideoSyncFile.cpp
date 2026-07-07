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
    ::supported << ".json";
    ::supported << ".gpx";

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
VideoSyncFile::VideoSyncFile(QString filename, int& /*mode*/, Context *context)
: context(context)
{
    this->filename(filename);
    reload();
}

VideoSyncFile::VideoSyncFile(Context *context) : context(context)
{
    filename("");
}

void VideoSyncFile::reload()
{
    // These types are enabled using ::supported list at top of this file.
    QRegExp fact(".+[.](gpx|json)$", Qt::CaseInsensitive);

    // which parser to call?
    if      (filename().endsWith(".rlv", Qt::CaseInsensitive)) parseRLV();
    else if (filename().endsWith(".tts", Qt::CaseInsensitive)) parseTTS();
    else if (fact.exactMatch(filename()))                      parseFromRideFileFactory();
}

void VideoSyncFile::parseTTS()
{
    // Initialise
    version("");
    units("");
    originalFilename("");
    name("");
    duration(-1);
    valid = false;  // did it parse ok as sync file?
    format(VideoSyncFileFormat::rlv);
    Points.clear();

    QFile ttsFile(filename());
    if (ttsFile.open(QIODevice::ReadOnly)) {

        QStringList errors_;

        QDataStream qttsStream(&ttsFile);
        qttsStream.setByteOrder(QDataStream::LittleEndian);

        NS_TTSReader::TTSReader ttsReader;
        bool success = ttsReader.parseFile(qttsStream);
        if (success) {

            // -----------------------------------------------------------------
            // VideoFrameRate
            videoFrameRate(ttsReader.getFrameRate());

            // -----------------------------------------------------------------
            // VideoSyncFilePoint
            if (ttsReader.hasFrameMapping()) {

                const std::vector<NS_TTSReader::Point>& ttsPoints = ttsReader.getPoints();
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

                duration(Points.last().secs * 1000.0);
                distance(Points.last().km);

                valid = true;

            } // hasFrameMapping

        } // parseFile

        ttsFile.close();

    } // ttsFile.open
}

void VideoSyncFile::parseRLV()
{
    // Initialise
    version("");
    units("");
    originalFilename("");
    name("");
    duration(-1);
    valid = false;             // did it parse ok?
    format(VideoSyncFileFormat::rlv); // default to rlv until we know
    Points.clear();

    // running totals
    double rdist = 0; // running total for distance
    double rend = 0;  // Length of entire course

    // open the file for binary reading and open a datastream
    QFile RLVFile(filename());
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

                    videoFrameRate(videoInfo.frameRate);
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

                        if (format() == VideoSyncFileFormat::rlv) {
                            // Calculate average distance per video frame between this sync point and the last
                            // using the formula (2a+b)/3 where a is the point of lower speed and b is the higher.
                            double distance = framemapping.distance;
                            double avgDistance = (qMin(distance, PreviousDistance)*2 + qMax(distance, PreviousDistance)) / 3;

                            rdist += avgDistance * (double) (framemapping.frameNbr - PreviousFrameNbr);
                            add.km = rdist / 1000.0;

                            // time
                            add.secs = (float) framemapping.frameNbr / videoFrameRate(); //FIXME : are we sure that video frame rate will be given prior to mapping?

                            // speed
                            add.kph = (distance / 1000.0) * (3600.0 * videoFrameRate());

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
        duration(Points.last().secs * 1000.0);      // last is the end point in msecs
        distance(Points.last().km);
    }
}

void VideoSyncFile::parseFromRideFileFactory()
{
    // Initialise
    version("");
    units("");
    originalFilename("");
    name("");
    duration(-1);
    valid = false;  // did it parse ok as sync file?
    format(VideoSyncFileFormat::rlv);
    Points.clear();

    // static double km = 0;

    QFile rideFile(filename());

    // Check file exists
    if (!rideFile.exists())
        return;

    // Instantiate RideFile
    QStringList errors_;
    RideFile* ride = RideFileFactory::instance().openRideFile(context, rideFile, errors_);
    if (ride == NULL)
        return;

    // Enumerate the data types that are available.
    bool fHasKm   = ride->areDataPresent()->km;
    bool fHasTime = ride->areDataPresent()->secs;
    bool fHasKph  = ride->areDataPresent()->kph;

    // These files have no frame rate. Let use determine
    videoFrameRate(0);

    // Video sync needs distance and time.
    if (!(fHasKm && fHasTime))
        return;

    double d0 = ride->dataPoints()[1]->km   - ride->dataPoints()[0]->km;
    double t0 = ride->dataPoints()[1]->secs - ride->dataPoints()[0]->secs;

    double v0 = (d0 / t0) * 60. * 60.; // initial kph, use second kph

    // If out of range set kph to zero.
    if (v0 > 1000. || v0 < 0.) v0 = 0.;

    double initialSecs = ride->dataPoints()[0]->secs;

    int pointCount = ride->dataPoints().count();
    for (int i = 0; i < pointCount; i++) {

        VideoSyncFilePoint add;

        const RideFilePoint& point = *ride->dataPoints()[i];

        // distance
        add.km = point.km;

        // time (first entry in file is 0.)
        add.secs = point.secs - initialSecs;

        // speed
        double kph = 0.;
        if (fHasKph) {
            kph = point.kph;
        } else if (i == 0) {
            kph = v0;
        } else {
            const RideFilePoint& prevPoint = *ride->dataPoints()[i-1];

            double distDelta = point.km   - prevPoint.km;
            double timeDelta = point.secs - prevPoint.secs;

            double kph = 0.;
            if (timeDelta) {
                kph = (distDelta / timeDelta) * 60. * 60.; // initial kph, use second kph
            } else {
                // Propagate previous speed if time is unchanged.
                kph = Points.last().kph;
            }
            // If out of range set kph to zero.
            if (kph > 1000. || kph < 0.) v0 = 0.;
        }

        add.kph = kph;

        Points.append(add);
    }

    // set RLVFile duration
    duration(Points.last().secs * 1000.0);      // last is the end point in msecs
    distance(Points.last().km);

    rideFile.close();

    valid = true;
}

VideoSyncFile::~VideoSyncFile()
{
    Points.clear();
}


bool
VideoSyncFile::isValid() const
{
    return valid;
}
