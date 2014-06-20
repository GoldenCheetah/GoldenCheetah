/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "Bin2RideFile.h"
#include <math.h>

#define START 0x210
#define UNIT_VERSION 0x2000
#define SYSTEM_INFO 0x2003

static int bin2FileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "bin2", "Joule GPS File", new Bin2FileReader());


struct Bin2FileReaderState
{
    QFile &file;
    QStringList &errors;
    RideFile *rideFile;
    int data_version;

    double secs, km;

    int interval;
    double last_interval_secs;

    bool stopped;

    QString deviceInfo;

    Bin2FileReaderState(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), secs(0), km(0),
        interval(0), last_interval_secs(0.0),  stopped(true)
    {

    }

    struct TruncatedRead {};

    int bcd2Int(char c)
    {
        return (0xff & c) - (((0xff & c)/16)*6);
    }

    double read_bytes(int len, int *count = NULL, int *sum = NULL)
    {
        char c;
        double res = 0;
        for (int i = 0; i < len; ++i) {
            if (file.read(&c, 1) != 1)
                throw TruncatedRead();
            if (sum)
                *sum += (0xff & c);
            if (count)
                *count += 1;

            res += pow(256,i) * (0xff & (unsigned) c) ;
        }
        return res;
    }

    QString read_text(int len, int *count = NULL, int *sum = NULL)
    {
        char c;
        QString res = "";
        for (int i = 0; i < len; ++i) {
            if (file.read(&c, 1) != 1)
                throw TruncatedRead();
            if (sum)
                *sum += (0xff & c);
            if (count)
                *count += 1;

            res += c;
        }
        return res;
    }

    QDateTime read_date(int *bytes_read = NULL, int *sum = NULL)
    {
        int sec = bcd2Int(read_bytes(1, bytes_read, sum));
        int min = bcd2Int(read_bytes(1, bytes_read, sum));
        int hour = bcd2Int(read_bytes(1, bytes_read, sum));
        int day = bcd2Int(read_bytes(1, bytes_read, sum));
        int month = bcd2Int(read_bytes(1, bytes_read, sum));
        int year = bcd2Int(read_bytes(1, bytes_read, sum));

        return QDateTime(QDate(2000+year,month,day), QTime(hour,min,sec));
    }

    QDateTime read_RTC_mark(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        QDateTime date = read_date(bytes_read, sum);

        read_bytes(1, bytes_read, sum); // dummy
        read_bytes(4, bytes_read, sum); // time_moving
        *secs = double(read_bytes(4, bytes_read, sum));
        read_bytes(16, bytes_read, sum);  // dummy

        return date;
    }

    int read_interval_mark(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        int intervalNumber = read_bytes(1, bytes_read, sum);;

        read_bytes(2, bytes_read, sum); // dummy
        *secs = double(read_bytes(4, bytes_read, sum));
        read_bytes(24, bytes_read, sum); // dummy

        return intervalNumber;
    }

    void read_detail_record(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        int cad = read_bytes(1, bytes_read, sum);
        read_bytes(1, bytes_read, sum); // pedal_smoothness
        int lrbal = read_bytes(1, bytes_read, sum);
        int hr = read_bytes(1, bytes_read, sum);
        read_bytes(1, bytes_read, sum); // dummy
        int watts = read_bytes(2, bytes_read, sum);
        int nm = read_bytes(2, bytes_read, sum);
        double kph = read_bytes(2, bytes_read, sum);
        int alt = read_bytes(2, bytes_read, sum); // todo this value is signed
        double temp = read_bytes(2, bytes_read, sum); // °C × 10 todo this value is signed
        double lat = read_bytes(4, bytes_read, sum); // todo this value is signed
        double lng = read_bytes(4, bytes_read, sum); // todo this value is signed
        double km = read_bytes(8, bytes_read, sum)/1000.0/1000.0;

        // Validations
        if (lrbal == 0xFF)
            lrbal = 0;
        else if ((lrbal & 0x200) == 0x200)
            lrbal = 100-(lrbal & 0x3F);
        else
            lrbal = lrbal & 0x3F;

        if (cad == 0xFF)
            cad = 0;

        if (hr == 0xFF)
            hr = 0;

        if (watts == 0xFFFF) // 65535
            watts = 0;

        if (kph == 0xFFFF) // 65535
            kph = 0;
        else
            kph = kph/10.0;

        if (temp == 0x8000) //0x8000 = invalid
            temp = RideFile::NoTemp;
        else if (temp > 0x7FFF) // Negative
            temp = (temp-0xFFFF)/10.0;
        else
            temp = temp/10.0;

        if (alt == 0x8000)
            alt = 0;
        else if (alt > 0x7FFF) // Negative
            alt = (alt-0xFFFF);

        if (lat == 0x80000000) //0x80000000 = invalid
            lat = 0;
        else if (lat > 0x7FFFFFFF) // Negative
            lat = (lat-0xFFFFFFFF)/10000000.0;
        else
            lat = lat/10000000.0;

        if (lng == 0x80000000) //0x80000000 = invalid
            lng = 0;
        else if (lng > 0x7FFFFFFF) // Negative
            lng = (lng-0xFFFFFFFF)/10000000.0;
        else
            lng = lng/10000000.0;

        // 0.0 values at end are for garmin vector torque efficiency/pedal smoothness which are not available
        rideFile->appendPoint(*secs, cad, hr, km, kph, nm, watts, alt, lng, lat, 0.0, 0, temp, lrbal, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, interval);
        (*secs)++;
    }

    void read_header(uint16_t &header, uint16_t &command, uint16_t &length, int *bytes_read = NULL, int *sum = NULL)
    {
        header = read_bytes(2, bytes_read, sum);
        command = read_bytes(2, bytes_read, sum);
        length = read_bytes(2, bytes_read, sum);
    }

    void read_ride_summary(int *bytes_read = NULL, int *sum = NULL)
    {
        data_version = read_bytes(1, bytes_read, sum); // data_version
        read_bytes(1, bytes_read, sum); // firmware_minor_version

        QDateTime t = read_date(bytes_read, sum);

        rideFile->setStartTime(t);

        read_bytes(148, bytes_read, sum);

        if (data_version >= 4)
            read_bytes(8, bytes_read, sum);

        if (data_version >= 6)
            read_bytes(8, bytes_read, sum);
    }

    void read_interval_summary(int *bytes_read = NULL, int *sum = NULL)
    {
        int interval_summary_size = 32;
        if (data_version>=4)
            interval_summary_size = 36;
        read_bytes(interval_summary_size*100, bytes_read, sum);
    }

    void read_username(int *bytes_read = NULL, int *sum = NULL)
    {
        QString name = read_text(20, bytes_read, sum);
        //deviceInfo += QString("User : %1\n").arg(name);
    }

    void read_user_info_record(int *bytes_read = NULL, int *sum = NULL)
    {
        read_bytes(64, bytes_read, sum);

        int smartbelt_A = read_bytes(2, bytes_read, sum);
        int smartbelt_B = read_bytes(2, bytes_read, sum);
        int smartbelt_C = read_bytes(2, bytes_read, sum);
        deviceInfo += QString("Smartbelt %1-%2-%3\n").arg(smartbelt_A).arg(smartbelt_B).arg(smartbelt_C);

        read_bytes(42, bytes_read, sum);

        if (data_version>=6)
            read_bytes(4, bytes_read, sum);

        if (data_version>=8)
            read_bytes(20, bytes_read, sum);
    }

    void read_ant_info_record(int *bytes_read = NULL, int *sum = NULL)
    {
        for (int i = 0; i < 6; i++) {
            int device_type = read_bytes(1, bytes_read, sum);
            if (device_type < 255)  {
                QString text = read_text(20, bytes_read, sum);

                for(int i=0; i < text.length(); i++)
                {
                    if (text.at(i) == QChar(0))
                        text = text.left(i);
                }

                //while (text.endsWith( QChar(0) )) text.chop(1);

                read_bytes(1, bytes_read, sum); // flag
                uint16_t id = read_bytes(2, bytes_read, sum);
                read_bytes(2, bytes_read, sum);
                read_bytes(2, bytes_read, sum);
                QString device_type_str;
                if (device_type == 11)
                    device_type_str = "Primary Power Id";
                else if (device_type == 120)
                    device_type_str = "Chest strap Id";
                else
                    device_type_str = QString("ANT %1 Id").arg(device_type);

                deviceInfo += QString("%1 %2 %3\n").arg(device_type_str).arg(id).arg(text);
            }
        }
    }

    // pages
    int read_summary_page()
    {
        int sum = 0;
        int bytes_read = 0;

        char header1 = read_bytes(1, &bytes_read, &sum); // Always 0x10
        char header2 = read_bytes(1, &bytes_read, &sum); // Always 0x02
        uint16_t command = read_bytes(2, &bytes_read, &sum);


        if (header1 == 0x10 && header2 == 0x02 && command == 0x2022)
        {
            uint16_t length = read_bytes(2, &bytes_read, &sum);
            uint16_t page_number = read_bytes(2, &bytes_read, &sum); // Page #

            if (page_number == 0) {
                // Page #0
                read_ride_summary(&bytes_read, &sum);
                read_interval_summary(&bytes_read, &sum);
                read_username(&bytes_read, &sum);
                read_user_info_record(&bytes_read, &sum);
                read_ant_info_record(&bytes_read, &sum);

                int finish = length+6-bytes_read;

                for (int i = 0; i < finish; i++) {
                    read_bytes(1, &bytes_read, &sum); // to finish
                }

                read_bytes(1, &bytes_read, &sum); // checksum

            } else {
               // not a summary page !
            }


        }

        return bytes_read;
    }

    int read_detail_page()
    {
        int sum = 0;
        int bytes_read = 0;

        char header1 = read_bytes(1, &bytes_read, &sum); // Always 0x10
        char header2 = read_bytes(1, &bytes_read, &sum); // Always 0x02
        uint16_t command = read_bytes(2, &bytes_read, &sum);

        if (header1 == 0x10 && header2 == 0x02 && command == 0x2022)
        {
            read_bytes(2, &bytes_read, &sum); // length
            uint16_t page_number = read_bytes(2, &bytes_read, &sum); // Page #

            if (page_number > 0) {
                // Page # >0
                // 128 x 32k
                QDateTime t;


                for (int i = 0; i < 128; i++) {
                    int flag = read_bytes(1, &bytes_read, &sum);
                    //b0..b1: "00" Detail Record
                    //b0..b1: "01" RTC Mark Record
                    //b0..b1: "00" Interval Record

                    //b2: reserved
                    //b3: Power was calculated
                    //b4: HPR packet missing
                    //b5: CAD packet missing
                    //b6: PWR packet missing
                    //b7: Power data = old (No new power calculated)

                    if (flag == 0xff) {
                        // means invalid entry
                        read_bytes(31, &bytes_read, &sum);
                    }
                    else if ((flag & 0x03) == 0x01){
                        t= read_RTC_mark(&secs, &bytes_read, &sum);
                    }
                    else if ((flag & 0x03) == 0x03){
                        int t = read_interval_mark(&secs, &bytes_read, &sum);
                        interval = t;
                    }
                    else if ((flag & 0x03) == 0x00 ){
                        read_detail_record(&secs, &bytes_read, &sum);
                    }

                }
                read_bytes(1, &bytes_read, &sum); // checksum

            }


        }

        return bytes_read;
    }

    int read_version()
    {
        int sum = 0;
        int bytes_read = 0;

        uint16_t header = read_bytes(2, &bytes_read, &sum); // Always 0x210 (0x10-0x02)
        uint16_t command = read_bytes(2, &bytes_read, &sum);

        if (header == START && command == UNIT_VERSION)
        {
            read_bytes(2, &bytes_read, &sum); // length

            int major_version = read_bytes(1, &bytes_read, &sum);
            int minor_version = read_bytes(2, &bytes_read, &sum);
            int data_version = read_bytes(2, &bytes_read, &sum);

            QString version = QString(minor_version<100?"%1.0%2 (%3)":"%1.%2 (%3)").arg(major_version).arg(minor_version).arg(data_version);
            deviceInfo += rideFile->deviceType()+QString(" Version %1\n").arg(version);

            read_bytes(1, &bytes_read, &sum); // checksum
        }
        return bytes_read;
    }

    int read_system_info()
    {
        int sum = 0;
        int bytes_read = 0;

        uint16_t header = read_bytes(2, &bytes_read, &sum); // Always (0x10-0x02)
        uint16_t command = read_bytes(2, &bytes_read, &sum);


        if (header == START && command == SYSTEM_INFO)
        {
            read_bytes(2, &bytes_read, &sum); // length

            read_bytes(52, &bytes_read, &sum);
            uint16_t odometer = read_bytes(8, &bytes_read, &sum);
            deviceInfo += QString("Odometer %1km\n").arg(odometer/1000.0);

            read_bytes(1, &bytes_read, &sum); // checksum
        }
        return bytes_read;
    }



    RideFile * run() {
        errors.clear();
        rideFile = new RideFile;
        rideFile->setDeviceType("Joule GPS");
        rideFile->setFileFormat("CycleOps Joule (bin2)");
        rideFile->setRecIntSecs(1);

        if (!file.open(QIODevice::ReadOnly)) {
            delete rideFile;
            return NULL;
        }
        bool stop = false;

        int data_size = file.size();
        int bytes_read = 0;

        bytes_read += read_version();
        bytes_read += read_system_info();
        bytes_read += read_summary_page();

        while (!stop && (bytes_read < data_size)) {
            bytes_read += read_detail_page(); // read_page(stop, errors);
        }

        rideFile->setTag("Device Info", deviceInfo);

        if (stop) {
            file.close();
            delete rideFile;
            return NULL;

        } else {
            file.close();
            return rideFile;
        }
    }
};

RideFile *Bin2FileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    QSharedPointer<Bin2FileReaderState> state(new Bin2FileReaderState(file, errors));
    return state->run();
}
