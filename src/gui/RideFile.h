/* 
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _RideFile_h
#define _RideFile_h

#include <QDate>
#include <QDir>
#include <QFile>
#include <QList>
#include <QMap>

struct RideFilePoint 
{
    double secs, cad, hr, km, kph, nm, watts;
    int interval;
    RideFilePoint() : secs(0.0), cad(0.0), hr(0.0), km(0.0), kph(0.0), 
        nm(0.0), watts(0.0), interval(0) {}
    RideFilePoint(double secs, double cad, double hr, double km, double kph, 
                  double nm, double watts, int interval) :
        secs(secs), cad(cad), hr(hr), km(km), kph(kph), nm(nm), 
        watts(watts), interval(interval) {}
};

class RideFile 
{
    private:

        QDateTime startTime_;  // time of day that the ride started
        double recIntSecs_;    // recording interval in seconds
        QList<RideFilePoint*> dataPoints_;

    public:

        RideFile() : recIntSecs_(0.0) {}
        RideFile(const QDateTime &startTime, double recIntSecs) :
            startTime_(startTime), recIntSecs_(recIntSecs) {}

        virtual ~RideFile() {
            QListIterator<RideFilePoint*> i(dataPoints_);
            while (i.hasNext()) 
                delete i.next();
        }

        const QDateTime &startTime() const { return startTime_; }
        double recIntSecs() const { return recIntSecs_; }
        const QList<RideFilePoint*> dataPoints() const { return dataPoints_; }

        void setStartTime(const QDateTime &value) { startTime_ = value; }
        void setRecIntSecs(double value) { recIntSecs_ = value; }
        void appendPoint(double secs, double cad, double hr, double km, 
                         double kph, double nm, double watts, int interval) {
            dataPoints_.append(new RideFilePoint(secs, cad, hr, km, kph, 
                                                 nm, watts, interval));
        }
};

struct RideFileReader {
    virtual ~RideFileReader() {}
    virtual RideFile *openRideFile(QFile &file, QStringList &errors) const = 0;
};

class CombinedFileReader : public RideFileReader {

    private:

        static CombinedFileReader *instance_;
        QMap<QString,RideFileReader*> readFuncs_;

        CombinedFileReader() {}

    public:

        static CombinedFileReader &instance();

        int registerReader(const QString &suffix, RideFileReader *reader);
        virtual RideFile *openRideFile(QFile &file, QStringList &errors) const;
        QStringList listRideFiles(const QDir &dir) const;
};

#endif // _RideFile_h

