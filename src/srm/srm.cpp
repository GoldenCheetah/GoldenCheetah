//
// Decodes .srm files.  Adapted from (buggy) description by Stephan Mantler
// (http://www.stephanmantler.com/?page_id=86).
//

#include "srm.h"
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

struct marker 
{
    int start, end;
};

struct blockhdr 
{
    QDateTime dt;
    quint16 chunkcnt;
};

bool readSrmFile(QFile &file, SrmData &data, QStringList &errorStrings) 
{
    if (!file.open(QFile::ReadOnly)) {
        errorStrings << QString("can't open file %1").arg(file.fileName());
        return false;
    }
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

    data.recint = ((double) recint1) / recint2;
    unsigned recintms = (unsigned) round(data.recint * 1000.0);

    // printf("magic=%s\n", magic);
    // printf("wheelcirc=%d\n", wheelcirc);
    // printf("dayssince1880=%d\n", dayssince1880);
    // printf("recint=%f sec\n", data.recint);
    // printf("blockcnt=%d\n", blockcnt);
    // printf("markercnt=%d\n", markercnt);
    // printf("commentlen=%d\n", commentlen);
    // printf("comment=%s\n", comment);

    QDate date(1880, 1, 1);
    date = date.addDays(dayssince1880);

    // printf("date=%s\n", date.toString().toAscii().constData());

    marker *markers = new marker[markercnt + 1];
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
        markers[i].start = start;
        markers[i].end = end;

        (void) active;
        (void) avgwatts;
        (void) avghr;
        (void) avgcad;
        (void) avgspeed;
        (void) pwc150;
        (void) wheelcirc;

        // printf("marker %d:\n", i);
        // printf("  mcomment=%s\n", mcomment);
        // printf("  active=%d\n", active);
        // printf("  start=%d\n", start);
        // printf("  end=%d\n", end);
        // printf("  avgwatts=%0.1f\n", avgwatts / 8.0);
        // printf("  avghr=%0.1f\n", avghr / 64.0);
        // printf("  avgcad=%0.1f\n", avgcad / 32.0);
        // printf("  avgspeed=%0.1f km/h\n", avgspeed / 2500.0 * 9.0);
        // printf("  pwc150=%d\n", pwc150);
    }

    blockhdr *blockhdrs = new blockhdr[blockcnt];
    for (int i = 0; i < blockcnt; ++i) {
        quint32 hsecsincemidn = readLong(in);
        blockhdrs[i].chunkcnt = readShort(in);
        blockhdrs[i].dt = QDateTime(date);
        blockhdrs[i].dt = blockhdrs[i].dt.addMSecs(hsecsincemidn * 10);
        // printf("block %d:\n", i);
        // printf("  start=%s\n", 
        //        blockhdrs[i].dt.toString().toAscii().constData());
        // printf("  chunkcnt=%d\n", blockhdrs[i].chunkcnt);
    }

    quint16 zero = readShort(in);
    quint16 slope = readShort(in);
    quint16 datacnt = readShort(in);
    readByte(in); // padding

    (void) zero;
    (void) slope;

    // printf("zero=%d\n", zero);
    // printf("slope=%d\n", slope);
    // printf("datacnt=%d\n", datacnt);

    assert(blockcnt > 0);

    int blknum = 0, blkidx = 0, mrknum = 0, interval = 0;
    double km = 0.0, secs = 0.0;

    if (markercnt > 0)
        mrknum = 1;

    for (int i = 0; i < datacnt; ++i) {
        quint8 ps[3];
        in.readRawData((char*) ps, sizeof(ps));
        quint8 cad = readByte(in);
        quint8 hr = readByte(in);
        double kph = (((((unsigned) ps[1]) & 0xf0) << 3) 
                      | (ps[0] & 0x7f)) * 3.0 / 26.0;
        unsigned watts = (ps[1] & 0x0f) | (ps[2] << 0x4);
        if (i == 0) {
            data.startTime = blockhdrs[blknum].dt;
            // printf("startTime=%s\n", 
            //        data.startTime.toString().toAscii().constData());
        }
        if (i == markers[mrknum].end) {
            ++interval;
            ++mrknum;
        }

        if ((i > 0) && (i == markers[mrknum].start - 1)) // markers count from 1
            ++interval;

        km += data.recint * kph / 3600.0;

        SrmDataPoint *point = new SrmDataPoint;
        point->cad = cad;
        point->hr = hr;
        point->watts = watts;
        point->interval = interval;
        point->kph = kph;
        point->km = km;
        point->secs = secs;
        data.dataPoints.append(point);

        // printf("%5.1f %5.1f %5.1f %4d %3d %3d %2d\n", 
        //        secs, km, kph, watts, hr, cad, interval);

        ++blkidx;
        if ((blkidx == blockhdrs[blknum].chunkcnt) && (blknum + 1 < blockcnt)) {
            QDateTime end = blockhdrs[blknum].dt.addMSecs(
                recintms * blockhdrs[blknum].chunkcnt);
            ++blknum;
            blkidx = 0;
            QDateTime start = blockhdrs[blknum].dt;
            qint64 endms = 
                ((qint64) end.toTime_t()) * 1000 + end.time().msec();
            qint64 startms = 
                ((qint64) start.toTime_t()) * 1000 + start.time().msec();
            qint64 diff = startms - endms;
            if (diff < 0) {
                errorStrings << QString("ERROR: time goes backwards by %1 ms"
                                        " on trans " "to block %2"
                                        ).arg(diff).arg(blknum);
                secs += data.recint; // for lack of a better option
            }
            else {
                // printf("jumping forward %lld ms\n", diff);
                secs += diff / 1000.0;
            }
        }
        else {
            secs += data.recint;
        }
    }

    file.close();
    return true;
}

/*
int main(int argc, char *argv[]) {
    assert(argc > 1);
    QFile file(argv[1]);
    QStringList errorStrings;
    SrmData data;
    if (readSrmFile(file, data, errorStrings))
        return 0;
    else 
        return -1;
}
*/

