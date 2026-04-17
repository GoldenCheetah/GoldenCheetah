#include "FileIO/Computrainer3dpParser.h"

#include <QBuffer>
#include <QDate>
#include <QTest>

#include <limits>

namespace {

QByteArray build3dpFile(quint32 sampleCount, int sampleBytes)
{
    QByteArray bytes(Computrainer3dpParser::kHeaderSize + sampleBytes, '\0');
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);

    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_4_0);
    stream.setByteOrder(QDataStream::LittleEndian);

    buffer.seek(4);
    stream.writeRawData("perf", 4);

    buffer.seek(88);
    stream << 75.0f;             // weight
    stream << quint32(180);      // upperHR
    stream << quint32(60);       // lowerHR
    stream << quint32(2024);     // year
    stream << quint8(1);         // month
    stream << quint8(2);         // day
    stream << quint8(3);         // hour
    stream << quint8(4);         // minute
    stream << sampleCount;

    return bytes;
}

bool parseHeader(const QByteArray &bytes, Computrainer3dpParser::Header &header, QString *error)
{
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);

    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_4_0);
    stream.setByteOrder(QDataStream::LittleEndian);

    return Computrainer3dpParser::parseHeader(stream, bytes.size(), header, error);
}

} // namespace

class TestComputrainer3dpFile : public QObject
{
    Q_OBJECT

private slots:
    void openRideFile_acceptsZeroSampleFile()
    {
        Computrainer3dpParser::Header header;
        QString error;

        QVERIFY2(parseHeader(build3dpFile(0, 0), header, &error), qPrintable(error));
        QVERIFY(error.isEmpty());
        QCOMPARE(header.year, quint32(2024));
        QCOMPARE(header.month, quint8(1));
        QCOMPARE(header.day, quint8(2));
        QCOMPARE(header.hour, quint8(3));
        QCOMPARE(header.minute, quint8(4));
        QCOMPARE(header.numberSamples, quint32(0));
    }

    void openRideFile_rejectsTruncatedSamplePayload()
    {
        Computrainer3dpParser::Header header;
        QString error;

        QVERIFY(!parseHeader(build3dpFile(1, Computrainer3dpParser::kSampleSize - 1), header, &error));
        QCOMPARE(error, QString("File is truncated."));
    }

    void openRideFile_rejectsAbsurdSampleCount()
    {
        Computrainer3dpParser::Header header;
        QString error;

        QVERIFY(!parseHeader(build3dpFile(std::numeric_limits<quint32>::max(), 0), header, &error));
        QCOMPARE(error, QString("File is truncated."));
    }
};

QTEST_MAIN(TestComputrainer3dpFile)
#include "testComputrainer3dpFile.moc"
