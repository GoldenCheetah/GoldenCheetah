#ifndef _GC_TacxCafParser_h
#define _GC_TacxCafParser_h

#include "GoldenCheetah.h"

#include <QByteArray>
#include <QStringList>

struct TacxCafBlockHeader
{
    qint16 fingerprint = 0;
    qint32 recordCount = 0;
    qint32 recordSize = 0;
};

qint16 readTacxCafVersion(const QByteArray &fileHeader, QStringList &errors);
bool readTacxCafBlockHeader(const QByteArray &blockHeaderBytes, TacxCafBlockHeader *blockHeader, QStringList *errors = nullptr);
bool validateTacxCafBlocks(const QByteArray &blocks, qint16 version, QStringList &errors);

#endif
