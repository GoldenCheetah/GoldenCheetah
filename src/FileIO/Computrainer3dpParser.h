#ifndef _GC_COMPUTRAINER3DPPARSER_H
#define _GC_COMPUTRAINER3DPPARSER_H

#include <QDataStream>
#include <QString>
#include <QtGlobal>

namespace Computrainer3dpParser {

inline constexpr qint64 kHeaderSize = 0xf8;
inline constexpr qint64 kSampleSize = 48;

struct Header {
    quint32 year = 0;
    quint8 month = 0;
    quint8 day = 0;
    quint8 hour = 0;
    quint8 minute = 0;
    quint32 numberSamples = 0;
};

bool parseHeader(QDataStream &stream, qint64 fileSize, Header &header, QString *error);

} // namespace Computrainer3dpParser

#endif
