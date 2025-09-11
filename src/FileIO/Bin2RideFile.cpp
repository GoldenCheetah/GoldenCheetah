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
#include <cmath>

#define START 0x210
#define UNIT_VERSION 0x2000
#define SYSTEM_INFO 0x2003

#define BIN2_DEBUG false // debug traces

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
    int interval_offset;

    bool jouleGPS;
    bool jouleGPS_GPSPlus;

    bool stopped;

    QString deviceInfo;

    Bin2FileReaderState(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), data_version(0),
        secs(0), km(0),
        interval(0), last_interval_secs(0.0), interval_offset(0),
        jouleGPS(true), jouleGPS_GPSPlus(true),
        stopped(true)
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

            if (c != 0)
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

        return QDateTime(QDate(GC_RIDE_FILE_YEAR+year,month,day), QTime(hour,min,sec));
    }

    QDateTime read_RTC_mark(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        QDateTime date = read_date(bytes_read, sum);

        if (jouleGPS_GPSPlus) {
            read_bytes(1, bytes_read, sum); // dummy
            read_bytes(4, bytes_read, sum); // time_moving

            // time
            double newsecs = double(read_bytes(4, bytes_read, sum));
            if (BIN2_DEBUG)
                qDebug() << "read_RTC_mark new ridetime" << newsecs << "secs (old" << *secs <<"secs)";

            *secs = newsecs;
            read_bytes(16, bytes_read, sum);  // dummy
        } else {
            read_bytes(13, bytes_read, sum);  // dummy
        }

        return date;
    }

    int read_interval_mark(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        int intervalNumber = read_bytes(1, bytes_read, sum);

        read_bytes(2, bytes_read, sum); // dummy
        double rideTime = double(read_bytes(4, bytes_read, sum));
        if (BIN2_DEBUG)
            qDebug() << "read_interval_mark" << rideTime << "at" << *secs <<"secs";

        if (jouleGPS_GPSPlus)
            read_bytes(24, bytes_read, sum); // dummy
        else
            read_bytes(12, bytes_read, sum); // dummy

        return intervalNumber;
    }

    void read_detail_record(double *secs, int *bytes_read = NULL, int *sum = NULL)
    {
        int cad = 0;
        int lrbal = 0;
        int hr = 0;
        int watts = 0;
        int nm = 0;
        double kph = 0.0;
        int alt = 0;
        double temp = RideFile::NA;
        double lat = 0.0;
        double lng = 0.0;
        double km = 0.0;

        if (jouleGPS_GPSPlus) {
            cad = read_bytes(1, bytes_read, sum);

            read_bytes(1, bytes_read, sum); // pedal_smoothness

            lrbal = read_bytes(1, bytes_read, sum);
            hr = read_bytes(1, bytes_read, sum);

            read_bytes(1, bytes_read, sum); // dummy

            watts = read_bytes(2, bytes_read, sum);
            nm = read_bytes(2, bytes_read, sum);
            kph = read_bytes(2, bytes_read, sum);
            alt = read_bytes(2, bytes_read, sum); // todo this value is signed
            if (data_version>=9) {
                alt = (alt-5000)/10.0; // todo this value is signed
            }
            temp = read_bytes(2, bytes_read, sum); // °C × 10 todo this value is signed
            lat = read_bytes(4, bytes_read, sum); // todo this value is signed
            lng = read_bytes(4, bytes_read, sum); // todo this value is signed
            km = read_bytes(8, bytes_read, sum)/1000.0/1000.0;
        } else {
            read_bytes(1, bytes_read, sum); // dropout flag

            cad = read_bytes(1, bytes_read, sum);
            hr = read_bytes(1, bytes_read, sum);
            kph = read_bytes(2, bytes_read, sum);
            watts = read_bytes(2, bytes_read, sum);
            nm = read_bytes(2, bytes_read, sum);
            alt = read_bytes(2, bytes_read, sum); // todo this value is signed
            temp = read_bytes(2, bytes_read, sum); // °C × 10 todo this value is signed

            read_bytes(2, bytes_read, sum); // dummy

            km = read_bytes(4, bytes_read, sum)/1000.0/1000.0;
        }

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
            temp = RideFile::NA;
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
        rideFile->appendPoint(*secs, cad, hr, km, kph, nm, watts, alt, lng, lat, 0.0, 0, temp, lrbal, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, interval);
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

        if (jouleGPS_GPSPlus)
        {
            read_bytes(148, bytes_read, sum);

            if (data_version >= 4)
                read_bytes(8, bytes_read, sum);

            if (data_version >= 6)
                read_bytes(8, bytes_read, sum);
       } else
       {
            read_bytes(84, bytes_read, sum);
       }
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
        if (BIN2_DEBUG)
            qDebug() << "User" << name;
        //deviceInfo += QString("User : %1\n").arg(name);
    }

    void read_user_info_record(int *bytes_read = NULL, int *sum = NULL)
    {
        read_bytes(64, bytes_read, sum);

        int smartbelt_A = read_bytes(2, bytes_read, sum);
        int smartbelt_B = read_bytes(2, bytes_read, sum);
        int smartbelt_C = read_bytes(2, bytes_read, sum);

        if (BIN2_DEBUG)
            qDebug() << "Smartbelt" << smartbelt_A << smartbelt_B << smartbelt_C;
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

                if (device_type == 8) // No 8 in FIT specification
                    device_type_str = "Power Id";
                else if (device_type == 11)
                    device_type_str = "Primary Power Id";
                else if (device_type == 120)
                    device_type_str = "Chest strap Id";
                else if (device_type == 121)
                    device_type_str = "Speed/Cadence Id";
                else if (device_type == 122)
                    device_type_str = "Cadence Id";
                else if (device_type == 123)
                    device_type_str = "Speed Id";

                else
                    device_type_str = QString("ANT %1 Id").arg(device_type);

                if (BIN2_DEBUG)
                    qDebug() << "ANT" << device_type_str << id << text;
                deviceInfo += QString("%1 %2 %3\n").arg(device_type_str).arg(id).arg(text);
            }
        }
    }

    // pages
    int read_summary_page()
    {
        if (BIN2_DEBUG)
            qDebug() << "read_summary_page()";

        int sum = 0;
        int bytes_read = 0;

        char header1 = read_bytes(1, &bytes_read, &sum); // Always 0x10
        char header2 = read_bytes(1, &bytes_read, &sum); // Always 0x02
        uint16_t command = read_bytes(2, &bytes_read, &sum);


        if (header1 == 0x10 && header2 == 0x02 && command == 0x2022)
        {
            uint16_t length = read_bytes(2, &bytes_read, &sum);
            uint16_t page_number = read_bytes(2, &bytes_read, &sum); // Page #

            if (BIN2_DEBUG)
                qDebug() << "page_number" << page_number;

            if (page_number == 0) {
                // Page #0
                if (jouleGPS_GPSPlus) {
                    read_ride_summary(&bytes_read, &sum);
                    read_interval_summary(&bytes_read, &sum);
                    read_username(&bytes_read, &sum);
                    read_user_info_record(&bytes_read, &sum);
                    read_ant_info_record(&bytes_read, &sum);
                } else  {
                    read_ride_summary(&bytes_read, &sum);
                    // page still contains 47 interval_summary
                }


                int finish = length+6-bytes_read;

                for (int i = 0; i < finish; i++) {
                    read_bytes(1, &bytes_read, &sum); // to finish
                }

                read_bytes(1, &bytes_read, &sum); // checksum

            } else {
                if (page_number == 1 && !jouleGPS_GPSPlus)  {
                    //page still contains 48 interval_summary
                    int finish = length+6-bytes_read;

                    for (int i = 0; i < finish; i++) {
                        read_bytes(1, &bytes_read, &sum); // to finish
                    }

                    read_bytes(1, &bytes_read, &sum); // checksum
                }
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

            if (page_number == 2 && !jouleGPS_GPSPlus) {
                //page still contains 5 interval_summary in 2 blocks of 256k
                read_bytes(512, &bytes_read, &sum);

            }

            if (page_number > 0) {
                if (BIN2_DEBUG)
                    qDebug() << "page_number" << page_number;
                int nb_record = 128;

                // Page # >0
                // Joule GPS : 128 x record (32k)

                if (!jouleGPS_GPSPlus) {
                    // Page # = 2
                    // Joule  : 168 x record (20k)
                    // Page # > 1
                    // Joule  : 192 x record (20k)

                    if (page_number == 2)
                        nb_record = 168;
                    else
                        nb_record = 192;
                }

                QDateTime t;


                for (int i = 0; i < nb_record; i++) {
                    int flag = read_bytes(1, &bytes_read, &sum);
                    //b0..b1: "00" Detail Record
                    //b0..b1: "01" RTC Mark Record
                    //b0..b1: "10" Session Mark (Joule GPS)
                    //b0..b1: "10" Interval Record (Joule 1.0)
                    //b0..b1: "11" Interval Record (Joule GPS)
                    //b0..b1: "11" Invalid ( 0xFF)


                    //b3: Power was calculated
                    //b4: HR packet missing
                    //b5: CAD packet missing
                    //b6: PWR packet missing
                    //b7: Power data = old (No new power calculated)

                    if (BIN2_DEBUG && (flag & 0x03) != 0) {
                        qDebug() << "flag B0..B1" << (flag & 0x03) << "flag" << flag;
                        if (flag == 0xff) {
                            qDebug() << "Invalid record";
                        }
                        else {
                            if ((flag & 0x08) == 0x08) {
                                qDebug() << "Power was calculated";
                            }
                            if ((flag & 0x10) == 0x10) {
                                qDebug() << "HPR packet missing";
                            }
                            if ((flag & 0x20) == 0x20) {
                                qDebug() << "CAD packet missing";
                            }
                            if ((flag & 0x40) == 0x40) {
                                qDebug() << "PWR packet missing";
                            }
                            if ((flag & 0x80) == 0x80) {
                                qDebug() << "Power data = old";
                            }
                        }
                    }

                    if (flag == 0xff) {
                        // means invalid entry
                        if (BIN2_DEBUG)
                            qDebug() << "invalid at" << secs << "secs";
                        if (jouleGPS_GPSPlus)
                            read_bytes(31, &bytes_read, &sum); // dummy
                        else
                            read_bytes(19, &bytes_read, &sum); // dummy
                        //increase seconds
                        (secs)++;
                    }
                    else if ((flag & 0x03) == 0x01){
                        // The RTC Mark is written when there is a gap in the ride data,
                        // or when the internal Real Time Clock is adjusted.
                        t= read_RTC_mark(&secs, &bytes_read, &sum);
                    }
                    else if ((jouleGPS_GPSPlus && (flag & 0x03) == 0x03) || (!jouleGPS_GPSPlus && (flag & 0x03) == 0x02)) {
                        // The Interval Mark immediately precedes a Detail Record at the same RTC time.
                        int t = read_interval_mark(&secs, &bytes_read, &sum);

                        if (interval_offset == 0 && t==interval)
                            interval_offset++;

                        if (t + interval_offset == interval) // end in interval mode
                            interval=0;
                        else
                            interval = t + interval_offset;
                    }
                    else if ((flag & 0x03) == 0x00 ){
                        // The detail record contains current telemetry data.
                        read_detail_record(&secs, &bytes_read, &sum);
                    } else {
                        if (BIN2_DEBUG)
                            qDebug() << "UNKNOW: flag" << flag << "at" << secs << "secs";

                        if (jouleGPS_GPSPlus)
                            read_bytes(31, &bytes_read, &sum); // dummy
                        else
                            read_bytes(19, &bytes_read, &sum); // dummy
                    }

                    if (!jouleGPS_GPSPlus && (i+1)%12 == 0)
                        read_bytes(16, &bytes_read, &sum); // unused

                }
                read_bytes(1, &bytes_read, &sum); // checksum

            }


        }

        return bytes_read;
    }

    int read_version()
    {
        if (BIN2_DEBUG)
            qDebug() << "read_version()";

        int sum = 0;
        int bytes_read = 0;

        uint16_t header = read_bytes(2, &bytes_read, &sum); // Always 0x210 (0x10-0x02)
        uint16_t command = read_bytes(2, &bytes_read, &sum);

        if (header == START && command == UNIT_VERSION)
        {
            int length = read_bytes(2, &bytes_read, &sum); // length
            if (BIN2_DEBUG)
                qDebug() << "length" << length;

            int major_version = read_bytes(1, &bytes_read, &sum);
            int minor_version = read_bytes(2, &bytes_read, &sum);

            int data_version = 1;

            if (major_version == 18)
            {
                jouleGPS_GPSPlus = false;
                jouleGPS = false;
            } else
            {
                jouleGPS_GPSPlus = true;
                if (major_version == 22)
                    jouleGPS = false;
            }

            if (jouleGPS_GPSPlus)
                data_version = read_bytes(2, &bytes_read, &sum);

            QString version = QString(minor_version<100?"%1.0%2 (%3)":"%1.%2 (%3)").arg(major_version).arg(minor_version).arg(data_version);
            if (jouleGPS)
                deviceInfo += "Joule GPS";
            else if (jouleGPS_GPSPlus)
                deviceInfo += "Joule GPS+";
            else
                deviceInfo += "Joule";

            if (BIN2_DEBUG)
                qDebug() << "Version" << version;

            deviceInfo += QString(" Version %1\n").arg(version);

            if (jouleGPS_GPSPlus && !jouleGPS)
                read_bytes(5, &bytes_read, &sum);

            read_bytes(1, &bytes_read, &sum); // checksum
        }
        return bytes_read;
    }

    int read_system_info()
    {
        if (BIN2_DEBUG)
            qDebug() << "read_system_info()";

        int sum = 0;
        int bytes_read = 0;

        uint16_t header = read_bytes(2, &bytes_read, &sum); // Always (0x10-0x02)
        uint16_t command = read_bytes(2, &bytes_read, &sum);

        if (BIN2_DEBUG)
            qDebug() << "header" << header << "command" << command;

        if (header == START && command == SYSTEM_INFO)
        {
            int length = read_bytes(2, &bytes_read, &sum); // length
            if (BIN2_DEBUG)
                qDebug() << "length" << length;

            if (jouleGPS_GPSPlus)
                read_bytes(52, &bytes_read, &sum);
            else
                read_bytes(50, &bytes_read, &sum);

            uint16_t odometer = read_bytes(8, &bytes_read, &sum);
            deviceInfo += QString("Odometer %1km\n").arg(odometer/1000.0);

            if (!jouleGPS_GPSPlus)
                read_bytes(34, &bytes_read, &sum);

            read_bytes(1, &bytes_read, &sum); // checksum
        } else
        {
            if (BIN2_DEBUG)
                qDebug() << "unknown header or command";
        }
        return bytes_read;
    }



    RideFile * run() {
        errors.clear();
        rideFile = new RideFile;
        rideFile->setFileFormat("CycleOps Joule (bin2)");
        rideFile->setRecIntSecs(1);

        if (!file.open(QIODevice::ReadOnly)) {
            delete rideFile;
            return NULL;
        }
        bool stop = false;

        qint64 data_size = file.size();
        qint64 bytes_read = 0;

        bytes_read += read_version();

        if (jouleGPS)
            rideFile->setDeviceType("Joule GPS");
        else if (jouleGPS_GPSPlus)
            rideFile->setDeviceType("Joule GPS+");
        else
            rideFile->setDeviceType("Joule");

        bytes_read += read_system_info();
        bytes_read += read_summary_page();
        if (!jouleGPS_GPSPlus)
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
