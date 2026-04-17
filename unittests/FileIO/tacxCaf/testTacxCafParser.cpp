#include "FileIO/TacxCafParser.h"

#include <QDataStream>
#include <QTest>

namespace {

QByteArray buildHeader(qint16 version)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream << qint16(3000) << version << qint32(0);
    return bytes;
}

QByteArray buildBlock(qint16 fingerprint, qint32 recordCount, qint32 recordSize, const QByteArray &payload)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream << fingerprint << qint16(0) << recordCount << recordSize;
    bytes += payload;
    return bytes;
}

QByteArray buildRideInformationPayload()
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream << qint16(2026) << qint16(4) << qint16(0) << qint16(11)
           << qint16(7) << qint16(30) << qint16(0) << qint16(0) << float(1.0f);
    return bytes;
}

QByteArray buildRideDataPayload()
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream << float(100.0f) << quint8(150) << quint8(85) << quint16(2500) << quint16(0);
    return bytes;
}

} // namespace

class TestTacxCafParser : public QObject
{
    Q_OBJECT

private slots:

    void readVersion_rejectsTruncatedHeader()
    {
        QStringList errors;

        QCOMPARE(readTacxCafVersion(QByteArray(4, '\0'), errors), qint16(0));
        QVERIFY(!errors.isEmpty());
    }

    void validateBlocks_rejectsTruncatedBlockHeader()
    {
        QStringList errors;

        QVERIFY(!validateTacxCafBlocks(QByteArray(6, '\0'), 100, errors));
        QVERIFY(!errors.isEmpty());
    }

    void validateBlocks_rejectsShortRideInformationPayload()
    {
        QStringList errors;
        QByteArray blocks = buildBlock(3010, 1, 8, QByteArray(8, '\0'));

        QVERIFY(!validateTacxCafBlocks(blocks, 100, errors));
        QVERIFY(errors.join('\n').contains("ride information block", Qt::CaseInsensitive));
    }

    void validateBlocks_rejectsShortRideDataPayload()
    {
        QStringList errors;
        QByteArray blocks = buildBlock(3020, 1, 10, QByteArray(6, '\0'));

        QVERIFY(!validateTacxCafBlocks(blocks, 100, errors));
        QVERIFY(errors.join('\n').contains("payload", Qt::CaseInsensitive));
    }

    void validateBlocks_acceptsMinimalValidFile()
    {
        QStringList errors;
        const QByteArray header = buildHeader(100);
        const qint16 version = readTacxCafVersion(header.left(8), errors);

        QByteArray blocks = buildBlock(3010, 1, buildRideInformationPayload().size(), buildRideInformationPayload());
        blocks += buildBlock(3020, 1, 10, buildRideDataPayload());

        QCOMPARE(version, qint16(100));
        QVERIFY(validateTacxCafBlocks(blocks, version, errors));
        QVERIFY(errors.isEmpty());
    }
};

QTEST_MAIN(TestTacxCafParser)
#include "testTacxCafParser.moc"
