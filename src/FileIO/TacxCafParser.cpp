#include "TacxCafParser.h"

#include <cstring>

namespace {

const int TACX_CAF_FILE_FINGERPRINT = 3000;
const int TACX_HEADER_BLOCK_SIZE = 8;
const int TACX_BLOCK_INFO_HEADER_SIZE = 12;
const int TACX_RIDE_INFORMATION_BLOCK = 3010;
const int TACX_RIDE_DATA_BLOCK = 3020;
const int TACX_RIDE_INFORMATION_BLOCK_MIN_SIZE = 20;
const int TACX_RIDE_DATA_BLOCK_SIZE = 10;

template <typename T>
bool readValue(const QByteArray &block, int offset, T *value)
{
    if (value == nullptr || offset < 0 || block.size() < offset + static_cast<int>(sizeof(T))) {
        return false;
    }

    std::memcpy(value, block.constData() + offset, sizeof(T));
    return true;
}

int rideDataRecordSize(qint16 version)
{
    switch (version) {
    case 100:
        return TACX_RIDE_DATA_BLOCK_SIZE;
    case 110:
        return TACX_RIDE_DATA_BLOCK_SIZE + 8;
    default:
        return -1;
    }
}

} // namespace

qint16 readTacxCafVersion(const QByteArray &fileHeader, QStringList &errors)
{
    if (fileHeader.size() != TACX_HEADER_BLOCK_SIZE) {
        errors << "Block size is not 8, is this a valid Tacx run file?";
        return 0;
    }

    qint16 fingerprint = 0;
    qint16 version = 0;
    if (!readValue(fileHeader, 0, &fingerprint) || !readValue(fileHeader, 2, &version)) {
        errors << "Tacx file header is truncated.";
        return 0;
    }

    if (fingerprint != TACX_CAF_FILE_FINGERPRINT) {
        errors << "This is not a Tacx run file";
        return 0;
    }

    if (rideDataRecordSize(version) < 0) {
        errors << QString("Unsupported Tacx CAF file version: %1").arg(version);
        return 0;
    }

    return version;
}

bool readTacxCafBlockHeader(const QByteArray &blockHeaderBytes, TacxCafBlockHeader *blockHeader, QStringList *errors)
{
    if (blockHeader == nullptr) {
        return false;
    }

    if (blockHeaderBytes.size() < TACX_BLOCK_INFO_HEADER_SIZE) {
        if (errors != nullptr) {
            errors->append("Tacx CAF block header is truncated.");
        }
        return false;
    }

    return readValue(blockHeaderBytes, 0, &blockHeader->fingerprint)
        && readValue(blockHeaderBytes, 4, &blockHeader->recordCount)
        && readValue(blockHeaderBytes, 8, &blockHeader->recordSize);
}

bool validateTacxCafBlocks(const QByteArray &blocks, qint16 version, QStringList &errors)
{
    const int dataRecordSize = rideDataRecordSize(version);
    if (dataRecordSize < 0) {
        errors << QString("Unsupported Tacx CAF file version: %1").arg(version);
        return false;
    }

    QByteArray remainingBytes = blocks;
    while (!remainingBytes.isEmpty()) {
        TacxCafBlockHeader blockHeader;
        if (!readTacxCafBlockHeader(remainingBytes.left(TACX_BLOCK_INFO_HEADER_SIZE), &blockHeader, &errors)) {
            return false;
        }

        if (blockHeader.recordCount < 0 || blockHeader.recordSize < 0) {
            errors << "Tacx CAF block header contains a negative size.";
            return false;
        }

        const qint64 payloadBytes = static_cast<qint64>(blockHeader.recordCount) * blockHeader.recordSize;
        const qint64 totalBlockBytes = TACX_BLOCK_INFO_HEADER_SIZE + payloadBytes;
        if (payloadBytes < 0 || totalBlockBytes > remainingBytes.size()) {
            errors << "Tacx CAF block payload is truncated.";
            return false;
        }

        if (blockHeader.fingerprint == TACX_RIDE_INFORMATION_BLOCK
            && payloadBytes < TACX_RIDE_INFORMATION_BLOCK_MIN_SIZE) {
            errors << "Tacx CAF ride information block is truncated.";
            return false;
        }

        if (blockHeader.fingerprint == TACX_RIDE_DATA_BLOCK && blockHeader.recordCount > 0) {
            if (blockHeader.recordSize < dataRecordSize) {
                errors << "Tacx CAF ride data block has an invalid record size.";
                return false;
            }
        }

        remainingBytes = remainingBytes.mid(static_cast<int>(totalBlockBytes));
    }

    return true;
}
