/*
 * Copyright (c) 2011 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "SyncRideFile.h"
#include <QSharedPointer>
#include <QMap>
#include <QSet>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>


static int syncFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "osyn", "Macro GoldenCheetah Sync File", new SyncFileReader());


#define TRAINING_DETAIL 0x89 //Packet with details about a specific training


struct SyncFileReaderState
{
    QFile &file;
    QStringList &errors;
    RideFile *rideFile;

    double secs, km;

    int interval;
    double last_interval_secs;

    bool stopped;


    SyncFileReaderState(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), secs(0), km(0),
        interval(0), last_interval_secs(0.0),  stopped(true)
    {

    }

    struct TruncatedRead {};

    int read_bsd_byte(int *count = NULL, int *sum = NULL) {
        char c;
        if (file.read(&c, 1) != 1)
            throw TruncatedRead();
        if (sum)
            *sum += (0xff & c);
        if (count)
            *count += 1;

        return (0xff & c) - (int(c/16)*6);
    }

    int read_bytes(int len, int *count = NULL, int *sum = NULL) {
        char c;
        int res = 0;
        for (int i = 0; i < len; ++i) {
            if (file.read(&c, 1) != 1)
                throw TruncatedRead();
            if (sum)
                *sum += (0xff & c);
            if (count)
                *count += 1;

            res += pow(256,i) * (0xff & c);
        }
        return res;
    }


    QString
    cEscape(char *buf, int len)
    {
        char *result = new char[4 * len + 1];
        char *tmp = result;
        for (int i = 0; i < len; ++i) {
            if (buf[i] == '"')
                tmp += sprintf(tmp, "\\\"");
            else if (isprint(buf[i]))
                *(tmp++) = buf[i];
            else
                tmp += sprintf(tmp, "\\x%02x", 0xff & (unsigned) buf[i]);
        }
        return result;
    }

    void read_graph_data(double *secs, int *count = NULL, int *sum = NULL)
    {
        double alt = 0, cad = 0, grade = 0, hr = 0, kph = 0, watts = 0;

        double nm = 0, headwind = 0.0;
        int lng = 0, lat = 0;

        // Graph data (12 bytes)

        read_bytes(2, count, sum); // Temperature * 100 (in degree C)

        alt = read_bytes(2, count, sum); //Alti (in m)
        grade = read_bytes(1, count, sum); // Gradient %
        cad = read_bytes(1, count, sum); // Cadence
        kph = read_bytes(2, count, sum)/10.0; // Speed * 10 (in Km/h)
        hr = read_bytes(1, count, sum); // Heart Rate
        watts = read_bytes(2, count, sum); // Power (in Watt)

        int record_data_flag = read_bytes(1, count, sum); // Data Flag

        int  intSecs = 1;
        if ((record_data_flag & 0x03) == 0)
            intSecs = 5;
        else if ((record_data_flag & 0x03) == 1)
            intSecs = 10;
        else if ((record_data_flag & 0x03) == 2)
            intSecs = 20;

        km += intSecs * kph / 3600.0;
        rideFile->setRecIntSecs(intSecs);
        rideFile->appendPoint(*secs, cad, hr, km, kph, nm, watts, alt, lng, lat, headwind, interval);

        *secs = *secs + intSecs;

    }

    int read_page()
    {
        int sum = 0;
        int bytes_read = 0;

        char record_command = read_bytes(1, &bytes_read, &sum); // Always 0x89

        if ((0xff & record_command) == 0x89)
        {
            // 1. page # data
            int page_number = read_bytes(2, &bytes_read, &sum); // Page #
            int data_number = read_bytes(1, &bytes_read, &sum); // # of data in page



            if (page_number == 1 || (page_number == 64010 and secs == 0.0)) {
                // 2. Training Summary data (60 bytes)";
                read_bytes(39, &bytes_read, &sum);

                int record_training_flag = read_bytes(1, &bytes_read, &sum); // Training Flag

                if ((record_training_flag & 0x01) == 0) {
                    // Only new lap
                    rideFile->addInterval(last_interval_secs, secs, QString("%1").arg(interval));
                    last_interval_secs = secs;
                    interval ++;
                }

                read_bytes(20, &bytes_read, &sum); // Don't care
            }

            if (page_number == 1 || (page_number == 64010 and secs == 0.0)) {
                // Section Start time and date data (12 byte)

                int sec = read_bsd_byte(&bytes_read, &sum); // Section start time sec
                int min = read_bsd_byte(&bytes_read, &sum); // Section start time min
                int hour = read_bsd_byte(&bytes_read, &sum); // Section start time hour
                int day = read_bsd_byte(&bytes_read, &sum); // Section start time day
                int month = read_bytes(1, &bytes_read, &sum); // Section start time month
                int year = read_bsd_byte(&bytes_read, &sum); // Section start time year

                QDateTime t = QDateTime(QDate(2000+year,month,day), QTime(hour,min,sec));

                if (secs == 0.0 || rideFile->startTime().toTime_t()<0) {
                    rideFile->setStartTime(t);
                }
                else {
                    int gap = (t.toTime_t() - rideFile->startTime().toTime_t()) - secs;
                    secs += gap;
                }

                read_bytes(5, &bytes_read, &sum); // Don't care

                read_bytes(1, &bytes_read, &sum); // Data Flag
                data_number--;
            }

            for (int i = 0; i < data_number; ++i) {
                read_graph_data(&secs, &bytes_read, &sum);
            }

            int finish = 259-bytes_read;

            for (int i = 0; i < finish; i++) {
                read_bytes(1, &bytes_read, &sum); // to finish
            }

            read_bytes(1, &bytes_read, &sum); // Checksum
        }

        return bytes_read;
    }

    RideFile * run() {
        errors.clear();
        rideFile = new RideFile;
        rideFile->setDeviceType("Macro Sync");

        if (!file.open(QIODevice::ReadOnly)) {
            delete rideFile;
            return NULL;
        }
        bool stop = false;

        int data_size = file.size();
        int bytes_read = 0;

        while (!stop && (bytes_read < data_size)) {
            bytes_read += read_page(); // read_page(stop, errors);
        }

        if (stop) {
            delete rideFile;
            return NULL;
        }
        else {
            return rideFile;
        }
    }
};


RideFile *SyncFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    QSharedPointer<SyncFileReaderState> state(new SyncFileReaderState(file, errors));
    return state->run();
}
