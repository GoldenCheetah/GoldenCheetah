//
// C++ Interface: csv
//
// Description: 
//
//
// Author: Justin F. Knotzke <jknotzke@shampoo.ca>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _csv_h
#define _csv_h

#include <QDate>

class QFile;
class RawFile;

class CsvData
{
public:
    int cad, hr, watts, interval;
    double kph, km, secs;
    QDateTime startTime;
    double recint;
    void readCsvFile(QFile &file, RawFile *rawFile);
};
#endif // _srm_h
