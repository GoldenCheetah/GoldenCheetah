#include "ANTLogger.h"

ANTLogger::ANTLogger(QObject *parent) : QObject(parent)
{
    isLogging=false;
}

void
ANTLogger::open()
{
    if (isLogging) return;

    antlog.setFileName("antlog.bin");
    antlog.open(QIODevice::WriteOnly | QIODevice::Truncate);
    isLogging=true;
}

void
ANTLogger::close()
{
    if (!isLogging) return;

    antlog.close();
    isLogging=false;
}

void ANTLogger::logRawAntMessage(const ANTMessage message, const struct timeval timestamp)
{
    Q_UNUSED(timestamp); // not used at present
    if (isLogging) {
        QDataStream out(&antlog);

        for (int i=0; i<ANT_MAX_MESSAGE_SIZE; i++)
            out<<message.data[i];
    }
}
