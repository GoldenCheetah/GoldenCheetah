#include "FileIO/TTSReader.h"

#include <QBuffer>
#include <QDataStream>
#include <QTest>

namespace {

QByteArray buildHeader(quint16 headerType, quint16 blockType, quint16 version, quint32 width, quint32 height)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << headerType << blockType << version << width << height;
    return bytes;
}

bool parseTts(const QByteArray &bytes)
{
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);

    QDataStream stream(&buffer);
    stream.setByteOrder(QDataStream::LittleEndian);

    NS_TTSReader::TTSReader reader;
    return reader.parseFile(stream);
}

} // namespace

class TestTtsReader : public QObject
{
    Q_OBJECT

private slots:

    void parseFile_rejectsMissingPendingBlockData()
    {
        const QByteArray bytes = buildHeader(1, 500, 1, 2, 2);

        QVERIFY(!parseTts(bytes));
    }

    void parseFile_rejectsOversizedBlockPayload()
    {
        const QByteArray bytes = buildHeader(1, 500, 1, 65536, 65536);

        QVERIFY(!parseTts(bytes));
    }
};

QTEST_MAIN(TestTtsReader)
#include "testTtsReader.moc"
