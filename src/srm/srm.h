
#ifndef _srm_h
#define _srm_h

#include <QDataStream>
#include <QDate>
#include <QFile>
#include <QStringList>

struct SrmDataPoint 
{
    int cad, hr, watts, interval;
    double kph, km, secs;
};

struct SrmData 
{
    QDateTime startTime;
    double recint;
    QList<SrmDataPoint*> dataPoints;
    ~SrmData() {
        QListIterator<SrmDataPoint*> i(dataPoints);
        while (i.hasNext()) 
            delete i.next();
    }
};

bool readSrmFile(QFile &file, SrmData &data, QStringList &errorStrings);

#endif // _srm_h

