#include "Computrainer3dpParser.h"

#include <cstring>

namespace {

bool fail(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
    return false;
}

bool skipExact(QDataStream &stream, qint64 count)
{
    return stream.skipRawData(count) == count;
}

} // namespace

namespace Computrainer3dpParser {

bool parseHeader(QDataStream &stream, qint64 fileSize, Header &header, QString *error)
{
    if (fileSize < kHeaderSize) {
        return fail(error, "File is truncated.");
    }

    if (!skipExact(stream, 4)) {
        return fail(error, "File is truncated.");
    }

    char perfStr[5] = {};
    if (stream.readRawData(perfStr, 4) != 4) {
        return fail(error, "File is truncated.");
    }
    if (std::strcmp(perfStr, "perf") != 0) {
        return fail(error, "File is encrypted.");
    }

    if (!skipExact(stream, 0x8)) {
        return fail(error, "File is truncated.");
    }

    char userName[65];
    if (stream.readRawData(userName, sizeof(userName)) != sizeof(userName)) {
        return fail(error, "File is truncated.");
    }

    quint8 age;
    float weight;
    quint32 upperHR;
    quint32 lowerHR;

    stream >> age;
    if (!skipExact(stream, 6)) {
        return fail(error, "File is truncated.");
    }

    stream >> weight;
    stream >> upperHR;
    stream >> lowerHR;
    stream >> header.year;
    stream >> header.month;
    stream >> header.day;
    stream >> header.hour;
    stream >> header.minute;
    stream >> header.numberSamples;

    if (stream.status() != QDataStream::Ok) {
        return fail(error, "File header is truncated.");
    }

    const qint64 sampleBytesAvailable = fileSize - kHeaderSize;
    const quint64 requiredSampleBytes =
        static_cast<quint64>(header.numberSamples) * static_cast<quint64>(kSampleSize);
    if (requiredSampleBytes > static_cast<quint64>(sampleBytesAvailable)) {
        return fail(error, "File is truncated.");
    }

    constexpr qint64 kBytesConsumedBeforeSamples = 112;
    if (!skipExact(stream, kHeaderSize - kBytesConsumedBeforeSamples)) {
        return fail(error, "File is truncated.");
    }

    return true;
}

} // namespace Computrainer3dpParser
