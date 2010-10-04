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
#include <QtGlobal>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <assert.h>

struct TacxCafFileReader : public RideFileReader {
    virtual RideFile *openRideFile(QFile &file, QStringList &errors) const;
};

static int tacxCafFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "caf", "Tacx Fortius", new TacxCafFileReader());

/** a simple struct which describes the header of a block */
typedef struct header {
    /** fingerprint of the block, see the list of block fingerprints */
    qint16 fingerprint;
    /** nr of records in the block */
    qint32 recordCount;
    /** size of each record */
    qint32 recordSize;
} header_t;

/**
  Read header block, containing general information about file, among which the
  fingerprint of the file. Return the version of the file, 100 or 110.
  */
static qint16 readHeaderBlock(const QByteArray& block, QStringList& errors);

/**
  Read all data blocks and return a RideFile.
  */
static RideFile* readBlocks(const QByteArray& blocks, qint16 version, QStringList& errors);

/**
  read the header of one block
  */
static header_t readBlockHeader(const QByteArray& blockHeader);

/**
  Read general information about the ride
  */
static bool readRideInformationBlock(RideFile* rideFile, const QByteArray& block);

/**
  Read the actual data points
  */
static bool readRideData(RideFile *rideFile, const QByteArray& block, const int nrOfRecords, const qint16 version);

static qint8 readByteFromByteArray(const QByteArray& block);
static qint16 readShortFromByteArray(const QByteArray& block);
static quint16 readUnsignedShortFromByteArray(const QByteArray& block);
static qint32 readIntFromByteArray(const QByteArray& block);
static float readFloatFromByteArray(const QByteArray& block);

static const int TACX_CAF_FILE_FINGERPRINT = 3000;
static const int TACX_HEADER_BLOCK_SIZE = 8;
static const int TACX_BLOCK_INFO_HEADER_SIZE = 12;
static const int TACX_RIDE_DATA_BLOCK_SIZE = 10;

// Block fingerprints
static const int TACX_RIDE_INFORMATION_BLOCK = 3010;
static const int TACX_RIDE_DATA_BLOCK = 3020;

static const QString TACX_FORTIUS_DEVICE_TYPE = "Tacx Fortius";

RideFile *TacxCafFileReader::openRideFile(QFile &file, QStringList &errors) const {
    if (!file.open(QFile::ReadOnly)) {
       errors << ("Could not open ride file: \""
                   + file.fileName() + "\"");
        return NULL;
    }

    QByteArray bytes = file.readAll();
    file.close();

    qint16 version = readHeaderBlock(bytes.left(TACX_HEADER_BLOCK_SIZE), errors);
    if (version == 0)
        return NULL;

    return readBlocks(bytes.mid(TACX_HEADER_BLOCK_SIZE), version, errors);
}

qint16 readHeaderBlock(const QByteArray& block, QStringList& errors) {
    assert(block.size() == 8);

    qint16 fingerprint = readShortFromByteArray(block);

    if (fingerprint != TACX_CAF_FILE_FINGERPRINT) {
        errors << ("This is not a Tacx run file");
        return 0;
    }
    qint16 version = readShortFromByteArray(block.mid(2));
    return version;
}


RideFile* readBlocks(const QByteArray& blocks, const qint16 version, QStringList& /*errors*/) {
    RideFile* rideFile = new RideFile();

    rideFile->setDeviceType(TACX_FORTIUS_DEVICE_TYPE);

    QByteArray remainingBytes = blocks;
    while(!remainingBytes.isEmpty()) {
        const QByteArray blockHeaderBytes = remainingBytes.left(12);
        header_t blockHeader = readBlockHeader(blockHeaderBytes);

        switch(blockHeader.fingerprint) {
        case TACX_RIDE_INFORMATION_BLOCK:
            readRideInformationBlock(rideFile, remainingBytes.mid(12));
            break;
        case TACX_RIDE_DATA_BLOCK:
            readRideData(rideFile, remainingBytes.mid(12), blockHeader.recordCount, version);
            break;
        }

        int skipBytes = 12 + blockHeader.recordCount * blockHeader.recordSize;

        remainingBytes = remainingBytes.mid(skipBytes);
    }

    return rideFile;
}

header_t readBlockHeader(const QByteArray& blockHeader) {
    Q_ASSERT(blockHeader.size() >= 12);
    header_t header;
    header.fingerprint = readShortFromByteArray(blockHeader);
    header.recordCount = readIntFromByteArray(blockHeader.mid(4, 4));
    header.recordSize = readIntFromByteArray(blockHeader.right(4));

    return header;
}

bool readRideInformationBlock(RideFile* rideFile, const QByteArray& block) {
    // year = block[0-2]
    qint16 year = readShortFromByteArray(block);
    qint16 month = readShortFromByteArray(block.mid(2));
    qint16 day = readShortFromByteArray(block.mid(6));
    qint16 hour = readShortFromByteArray(block.mid(8));
    qint16 minute = readShortFromByteArray(block.mid(10));
    qint16 second = readShortFromByteArray(block.mid(12));

    rideFile->setStartTime(QDateTime(QDate(year, month, day), QTime(hour, minute, second)));
    rideFile->setRecIntSecs(readFloatFromByteArray(block.mid(16)));

    return true;
}

bool readRideData(RideFile *rideFile, const QByteArray& block, const int nrOfRecords, const qint16 version) {
    const int dataRecordSize = (version == 100) ? TACX_RIDE_DATA_BLOCK_SIZE : TACX_RIDE_DATA_BLOCK_SIZE + 8;

    double seconds = rideFile->recIntSecs();
    float lastDistance = 0.0f;
    for(int i = 0; i < nrOfRecords; i++, seconds += rideFile->recIntSecs()) {
        const QByteArray& record = block.mid(i * dataRecordSize);

        float distance = readFloatFromByteArray(record);
        quint8 heartRate = readByteFromByteArray(record.mid(4));
        quint8 cadence = readByteFromByteArray(record.mid(5));
        quint16 powerX10 = readUnsignedShortFromByteArray(record.mid(6));
        double power = powerX10 / 10.0;

        double speed = (distance - lastDistance) / rideFile->recIntSecs();

        rideFile->appendPoint(seconds, cadence, heartRate, distance / 1000.0, speed * 3.6, 0.0, power, 0.0, 0.0, 0.0, 0.0, 0);
        lastDistance = distance;
    }

    return true;
}

qint8 readByteFromByteArray(const QByteArray& block) {
    Q_ASSERT(block.size() >= 1);
    qint8 value = 0;
    memcpy(&value, block, 1);
    return value;
}

quint16 readUnsignedShortFromByteArray(const QByteArray& block) {
    Q_ASSERT(block.size() >= 2);
    quint16 value = 0;
    memcpy(&value, block, 2);
    return value;
}

qint16 readShortFromByteArray(const QByteArray& block) {
    Q_ASSERT(block.size() >= 2);
    qint16 value = 0;
    memcpy(&value, block, 2);
    return value;
}

qint32 readIntFromByteArray(const QByteArray& block) {
    Q_ASSERT(block.size() >= 4);
    qint32 value = 0;
    memcpy(&value, block, 4);
    return value;
}

float readFloatFromByteArray(const QByteArray& block) {
    Q_ASSERT(block.size() >= 4);
    float value = 0;
    memcpy(&value, block, 4);
    return value;
}
