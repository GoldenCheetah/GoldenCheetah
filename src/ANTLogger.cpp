#include "ANTLogger.h"

ANTLogger::ANTLogger(QObject *parent) : QObject(parent)
{
    isLogging=false;
}

void ANTLogger::logRawAntMessage(const ANTMessage *message, const struct timeval *timestamp)
{
    if (message==NULL && timestamp==NULL) {
        if (isLogging) {
            // close debug file
            antlog.close();
            isLogging=false;
        }
        return;
    }

    if (!isLogging) {
        antlog.setFileName("antlog.bin");
        antlog.open(QIODevice::WriteOnly | QIODevice::Truncate);
        isLogging = true;
    }

    QDataStream out(&antlog);

    for (int i=0; i<ANT_MAX_MESSAGE_SIZE; i++)
        out<<message->data[i];


}

