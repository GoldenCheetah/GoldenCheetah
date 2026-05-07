/*
 * Copyright (c) 2010 Ilja Booij (ibooij@gmail.com)
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

#include "RideFile.h"
#include "TacxCafParser.h"
#include <QtGlobal>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <cstring>

struct TacxCafFileReader : public RideFileReader {
    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*>* = 0) const;
    virtual bool hasWrite() const { return false; }
};

static int tacxCafFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "caf", "Tacx Fortius", new TacxCafFileReader());

/**
  Read all data blocks and return a RideFile.
  */
static RideFile* readBlocks(const QByteArray& blocks, qint16 version, QStringList& errors);

/**
  Read general information about the ride
  */
static bool readRideInformationBlock(RideFile* rideFile, const QByteArray& block, QStringList& errors);

/**
  Read the actual data points
  */
static bool readRideData(RideFile *rideFile, const QByteArray& block, int nrOfRecords, qint32 recordSize, qint16 version, QStringList& errors);

static const int TACX_HEADER_BLOCK_SIZE = 8;
static const int TACX_BLOCK_INFO_HEADER_SIZE = 12;
static const int TACX_RIDE_INFORMATION_BLOCK_SIZE = 20;
static const int TACX_RIDE_DATA_BLOCK_SIZE = 10;

// Block fingerprints
static const int TACX_RIDE_INFORMATION_BLOCK = 3010;
static const int TACX_RIDE_DATA_BLOCK = 3020;

static const QString TACX_FORTIUS_DEVICE_TYPE = "Tacx Fortius";

template <typename T>
static bool readValueFromByteArray(const QByteArray& block, int offset, T *value)
{
    if (value == nullptr || offset < 0 || block.size() < offset + static_cast<int>(sizeof(T))) {
        return false;
    }

    std::memcpy(value, block.constData() + offset, sizeof(T));
    return true;
}

RideFile *TacxCafFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const {
    if (!file.open(QFile::ReadOnly)) {
       errors << ("Could not open ride file: \""
                   + file.fileName() + "\"");
        return NULL;
    }

    QByteArray bytes = file.readAll();
    file.close();

    qint16 version = readTacxCafVersion(bytes.left(TACX_HEADER_BLOCK_SIZE), errors);
    if (version == 0)
        return NULL;

    const QByteArray blocks = bytes.mid(TACX_HEADER_BLOCK_SIZE);
    if (!validateTacxCafBlocks(blocks, version, errors)) {
        return NULL;
    }

    return readBlocks(blocks, version, errors);
}


RideFile* readBlocks(const QByteArray& blocks, const qint16 version, QStringList& errors) {
    RideFile* rideFile = new RideFile();

    rideFile->setDeviceType(TACX_FORTIUS_DEVICE_TYPE);
    rideFile->setFileFormat("Tacx Fortius (caf)");

    QByteArray remainingBytes = blocks;
    while(!remainingBytes.isEmpty()) {
        TacxCafBlockHeader blockHeader;
        if (!readTacxCafBlockHeader(remainingBytes.left(TACX_BLOCK_INFO_HEADER_SIZE), &blockHeader, &errors)) {
            delete rideFile;
            return NULL;
        }

        const qint64 payloadBytes = static_cast<qint64>(blockHeader.recordCount) * blockHeader.recordSize;
        if (payloadBytes < 0 || TACX_BLOCK_INFO_HEADER_SIZE + payloadBytes > remainingBytes.size()) {
            errors << "Tacx CAF block payload is truncated.";
            delete rideFile;
            return NULL;
        }

        const QByteArray blockPayload = remainingBytes.mid(TACX_BLOCK_INFO_HEADER_SIZE, static_cast<int>(payloadBytes));

        switch(blockHeader.fingerprint) {
        case TACX_RIDE_INFORMATION_BLOCK:
            if (!readRideInformationBlock(rideFile, blockPayload, errors)) {
                delete rideFile;
                return NULL;
            }
            break;
        case TACX_RIDE_DATA_BLOCK:
            if (!readRideData(rideFile, blockPayload, blockHeader.recordCount, blockHeader.recordSize, version, errors)) {
                delete rideFile;
                return NULL;
            }
            break;
        }

        remainingBytes = remainingBytes.mid(TACX_BLOCK_INFO_HEADER_SIZE + static_cast<int>(payloadBytes));
    }

    return rideFile;
}

bool readRideInformationBlock(RideFile* rideFile, const QByteArray& block, QStringList& errors) {
    if (block.size() < TACX_RIDE_INFORMATION_BLOCK_SIZE) {
        errors << "Tacx CAF ride information block is truncated.";
        return false;
    }

    // year = block[0-2]
    qint16 year = 0;
    qint16 month = 0;
    qint16 day = 0;
    qint16 hour = 0;
    qint16 minute = 0;
    qint16 second = 0;
    float recordingInterval = 0.0f;
    if (!readValueFromByteArray(block, 0, &year)
        || !readValueFromByteArray(block, 2, &month)
        || !readValueFromByteArray(block, 6, &day)
        || !readValueFromByteArray(block, 8, &hour)
        || !readValueFromByteArray(block, 10, &minute)
        || !readValueFromByteArray(block, 12, &second)
        || !readValueFromByteArray(block, 16, &recordingInterval)) {
        errors << "Tacx CAF ride information block is truncated.";
        return false;
    }

    rideFile->setStartTime(QDateTime(QDate(year, month, day), QTime(hour, minute, second)));
    rideFile->setRecIntSecs(recordingInterval);

    return true;
}

static bool readSinglePoint(const QByteArray& record, const double& timeInSeconds, const double& recordingIntervalInSeconds,
                            const float& startDistance, const float& lastDistance, RideFilePoint *point) {
    if (point == nullptr) {
        return false;
    }

    float distance = 0.0f;
    quint8 heartRate = 0;
    quint8 cadence = 0;
    quint16 powerX10 = 0;
    if (!readValueFromByteArray(record, 0, &distance)
        || !readValueFromByteArray(record, 4, &heartRate)
        || !readValueFromByteArray(record, 5, &cadence)
        || !readValueFromByteArray(record, 6, &powerX10)) {
        return false;
    }

    float relativeDistance = distance - startDistance;
    double power = powerX10 / 10.0;
    double speed = 0.0;
    if (recordingIntervalInSeconds > 0.0) {
        speed = (relativeDistance - lastDistance) / recordingIntervalInSeconds;
    }

    *point = RideFilePoint(timeInSeconds, cadence, heartRate, relativeDistance / 1000.0, speed * 3.6, 0.0, power,
                           0.0, 0.0, 0.0, 0.0, 0.0,
                           RideFile::NA, RideFile::NA,
                           0.0, 0.0, 0.0, 0.0,
                           0.0, 0.0,
                           0.0, 0.0, 0.0, 0.0,
                           0.0, 0.0, 0.0, 0.0,
                           0.0, 0.0,
                           0.0, 0.0, 0.0, 0.0,
                           0);
    return true;
}

bool readRideData(RideFile *rideFile, const QByteArray& block, int nrOfRecords, qint32 recordSize, const qint16 version, QStringList& errors) {
    const int dataRecordSize = (version == 100) ? TACX_RIDE_DATA_BLOCK_SIZE : TACX_RIDE_DATA_BLOCK_SIZE + 8;
    if (nrOfRecords <= 0) {
        return true;
    }

    if (recordSize < dataRecordSize) {
        errors << "Tacx CAF ride data block has an invalid record size.";
        return false;
    }

    const qint64 requiredBytes = static_cast<qint64>(nrOfRecords) * recordSize;
    if (requiredBytes < 0 || requiredBytes > block.size()) {
        errors << "Tacx CAF ride data block is truncated.";
        return false;
    }

    double seconds = rideFile->recIntSecs();
    RideFilePoint firstDataPoint;
    if (!readSinglePoint(block.left(dataRecordSize), seconds, rideFile->recIntSecs(), 0.0f, 0.0f, &firstDataPoint)) {
        errors << "Tacx CAF ride data block is truncated.";
        return false;
    }
    float startDistance = firstDataPoint.km * 1000.0;
    float lastDistance = 0.0f;
    for(int i = 0; i < nrOfRecords; i++, seconds += rideFile->recIntSecs()) {
        const QByteArray record = block.mid(i * recordSize, dataRecordSize);
        RideFilePoint nextDataPoint;
        if (!readSinglePoint(record, seconds, rideFile->recIntSecs(), startDistance, lastDistance, &nextDataPoint)) {
            errors << "Tacx CAF ride data block is truncated.";
            return false;
        }
        lastDistance = nextDataPoint.km * 1000.0;

        rideFile->appendPoint(nextDataPoint.secs, nextDataPoint.cad,
                              nextDataPoint.hr, nextDataPoint.km,
                              nextDataPoint.kph, nextDataPoint.nm,
                              nextDataPoint.watts, nextDataPoint.alt,
                              nextDataPoint.lon, nextDataPoint.lat,
                              nextDataPoint.headwind, 0.0, RideFile::NA, 0.0, 
                              0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                              0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0, nextDataPoint.interval);
    }
    return true;
}
