//
// Decodes .srm files.  Adapted from (buggy) description by Stephan Mantler
// (http://www.stephanmantler.com/?page_id=86).
//

#include <QDataStream>
#include <QDate>
#include <QFile>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <arpa/inet.h>

static quint8 readByte(QDataStream &in) 
{
    quint8 value;
    in >> value;
    return value;
}

static quint16 readShort(QDataStream &in) 
{
    quint16 value;
    in >> value;
    return htons(value); // SRM uses big endian
}

static quint32 readLong(QDataStream &in) 
{
    quint32 value;
    in >> value;
    return htonl(value); // SRM uses big endian
}

struct blockhdr 
{
    QDateTime dt;
    quint16 chunkcnt;
};

int main(int argc, char *argv[])
{
    assert(argc > 1);
    QFile file(argv[1]);
    if (!file.open(QFile::ReadOnly))
       assert(false); 
    QDataStream in(&file);

    char magic[5];
    in.readRawData(magic, sizeof(magic) - 1);
    magic[sizeof(magic) - 1] = '\0';
    if (strncmp(magic, "SRM6", sizeof(magic)) != 0)
        assert(false);

    quint16 dayssince1880 = readShort(in);
    quint16 wheelcirc = readShort(in);
    quint8 recint1 = readByte(in);
    quint8 recint2 = readByte(in);
    quint16 blockcnt = readShort(in);
    quint16 markercnt = readShort(in);
    readByte(in); // padding
    quint8 commentlen = readByte(in);

    char comment[71];
    in.readRawData(comment, sizeof(comment) - 1);
    comment[commentlen - 1] = '\0';

    double recint = ((double) recint1) / recint2;
    unsigned recintms = (unsigned) round(recint * 1000.0);

    printf("magic=%s\n", magic);
    printf("wheelcirc=%d\n", wheelcirc);
    printf("dayssince1880=%d\n", dayssince1880);
    printf("recint=%f sec\n", recint);
    printf("blockcnt=%d\n", blockcnt);
    printf("markercnt=%d\n", markercnt);
    printf("commentlen=%d\n", commentlen);
    printf("comment=%s\n", comment);

    QDate date(1880, 1, 1);
    date = date.addDays(dayssince1880);

    printf("date=%s\n", date.toString().toAscii().constData());

    for (int i = 0; i <= markercnt; ++i) {
        char mcomment[256];
        in.readRawData(mcomment, sizeof(mcomment) - 1);
        mcomment[sizeof(mcomment) - 1] = '\0';

        quint8 active = readByte(in);
        quint16 start = readShort(in);
        quint16 end = readShort(in);
        quint16 avgwatts = readShort(in);
        quint16 avghr = readShort(in);
        quint16 avgcad = readShort(in);
        quint16 avgspeed = readShort(in);
        quint16 pwc150 = readShort(in);

        printf("marker %d:\n", i);
        printf("  mcomment=%s\n", mcomment);
        printf("  active=%d\n", active);
        printf("  start=%d\n", start);
        printf("  end=%d\n", end);
        printf("  avgwatts=%0.1f\n", avgwatts / 8.0);
        printf("  avghr=%0.1f\n", avghr / 64.0);
        printf("  avgcad=%0.1f\n", avgcad / 32.0);
        printf("  avgspeed=%0.1f km/h\n", avgspeed / 2500.0 * 9.0);
        printf("  pwc150=%d\n", pwc150);
    }

    blockhdr *blockhdrs = new blockhdr[blockcnt];
    for (int i = 0; i < blockcnt; ++i) {
        quint32 hsecsincemidn = readLong(in);
        blockhdrs[i].chunkcnt = readShort(in);
        blockhdrs[i].dt = QDateTime(date);
        blockhdrs[i].dt = blockhdrs[i].dt.addMSecs(hsecsincemidn * 10);
        printf("block %d:\n", i);
        printf("  start=%s\n", 
               blockhdrs[i].dt.toString().toAscii().constData());
        printf("  chunkcnt=%d\n", blockhdrs[i].chunkcnt);
    }

    quint16 zero = readShort(in);
    quint16 slope = readShort(in);
    quint16 datacnt = readShort(in);
    readByte(in); // padding

    printf("zero=%d\n", zero);
    printf("slope=%d\n", slope);
    printf("datacnt=%d\n", datacnt);

    for (int i = 0, blknum = 0, blkidx = 0; i < datacnt; ++i) {
        quint8 ps[3];
        in.readRawData((char*) ps, sizeof(ps));
        quint8 cad = readByte(in);
        quint8 hr = readByte(in);
        double kph = (((((unsigned) ps[1]) & 0xf0) << 3) 
                      | (ps[0] & 0x7f)) * 3.0 / 26.0;
        unsigned watts = (ps[1] & 0x0f) | (ps[2] << 0x4);
        QDateTime dt = blockhdrs[blknum].dt.addMSecs(recintms * blkidx);
        printf("  %s %5.1f %4d %3d %3d\n", 
               dt.toString().toAscii().constData(), kph, watts, hr, cad);
        ++blkidx;
        if (blkidx == blockhdrs[blknum].chunkcnt) {
            ++blknum;
            blkidx = 0;
        }
    }

    file.close();
    return 0;
}

