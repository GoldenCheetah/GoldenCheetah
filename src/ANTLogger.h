#include <sys/time.h>
#include <QObject>
#include "ANTMessage.h"

#ifndef ANTLOGGER_H
#define ANTLOGGER_H

class ANTLogger : public QObject
{
    Q_OBJECT
public:
    explicit ANTLogger(QObject *parent = 0);
    
signals:
    
public slots:
    void logRawAntMessage(const ANTMessage message, const struct timeval timestamp);
    void open();
    void close();

private:
    // antlog.bin ant message stream
    QFile antlog;
    bool isLogging;

};

#endif // ANTLOGGER_H
