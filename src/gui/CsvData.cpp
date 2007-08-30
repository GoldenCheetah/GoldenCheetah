//
// C++ Implementation: csv
//
// Description: 
//
//
// Author: Justin F. Knotzke <jknotzke@shampoo.ca>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "CsvData.h"
#include <QTextStream>
#include "RawFile.h"
#include <iostream>
#include <algorithm>
#include <vector>

void CsvData::readCsvFile(QFile &file, RawFile *rawFile) 
{

    QTextStream stream( &file );
    QString line;
    std::vector<double> vec;
    double previousSecs = 0.0;
    //Flush the first line.
    line = stream.readLine();
    while ( !stream.atEnd() )
    {
        line = stream.readLine();
        //Parse the line here
        secs = line.section(',', 0, 0).toDouble();
        kph = line.section(',', 2, 2).toDouble();
        watts = line.section(',', 3, 3).toInt();
        km = line.section(',', 4, 4).toDouble();
        cad = line.section(',', 5, 5).toInt();
        hr = line.section(',', 6, 6).toInt();
        interval = line.section(',', 7, 7).toInt();

        //To figure out the mean recording interval
        vec.push_back(secs - previousSecs);
        previousSecs = secs;

        //For the power histogram
        if (rawFile->powerHist.contains(watts))
            rawFile->powerHist[watts] +=secs;
        else
            rawFile->powerHist[watts] = secs;

        //Assume that all CSV files have km/h
        RawFilePoint *p1 = new RawFilePoint(
            secs, 0, kph * 0.621371192 , watts,
            km * 0.621371192, cad, hr, interval);

        rawFile->points.append(p1);
    }
    //Sort it
    std::sort(vec.begin(), vec.end());
    int size = vec.size();
    //Then grab the middle value (the mean) and covert to milliseconds
    rawFile->rec_int_ms = int(vec[size/2] * 60 * 1000);
}

