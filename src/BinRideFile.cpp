/*
 * Copyright (c) 2010 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "BinRideFile.h"
#include <QSharedPointer>
#include <QMap>
#include <QSet>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define RECORD_TYPE__META 0
#define RECORD_TYPE__RIDE_DATA 1
#define RECORD_TYPE__RAW_DATA 2
#define RECORD_TYPE__SPARSE_DATA 3
#define RECORD_TYPE__INTERVAL_DATA 4
#define RECORD_TYPE__DATA_ERROR 5
#define RECORD_TYPE__HISTORY 6

#define FORMAT_ID__RIDE_START 0
#define FORMAT_ID__RIDE_SAMPLE_RATE 1
#define FORMAT_ID__FIRMWARE_VERSION 2
#define FORMAT_ID__LAST_UPDATE 3
#define FORMAT_ID__ODOMETER 4
#define FORMAT_ID__PRIMARY_POWER_ID 5
#define FORMAT_ID__SECONDARY_POWER_ID 6
#define FORMAT_ID__CHEST_STRAP_ID 7
#define FORMAT_ID__CADENCE_ID 8
#define FORMAT_ID__SPEED_ID 9
#define FORMAT_ID__RESISTANCE_UNITID 10
#define FORMAT_ID__WORKOUT_ID 11
#define FORMAT_ID__USER_WEIGHT 12
#define FORMAT_ID__USER_CATEGORY 13
#define FORMAT_ID__USER_HR_ZONE_1 14
#define FORMAT_ID__USER_HR_ZONE_2 15
#define FORMAT_ID__USER_HR_ZONE_3 16
#define FORMAT_ID__USER_HR_ZONE_4 17
#define FORMAT_ID__USER_POWER_ZONE_1 18
#define FORMAT_ID__USER_POWER_ZONE_2 19
#define FORMAT_ID__USER_POWER_ZONE_3 20
#define FORMAT_ID__USER_POWER_ZONE_4 21
#define FORMAT_ID__USER_POWER_ZONE_5 22
#define FORMAT_ID__WHEEL_CIRC 23
#define FORMAT_ID__RIDE_DISTANCE 24
#define FORMAT_ID__RIDE_TIME 25
#define FORMAT_ID__POWER 26
#define FORMAT_ID__TORQUE 27
#define FORMAT_ID__SPEED 28
#define FORMAT_ID__CADENCE 29
#define FORMAT_ID__HEART_RATE 30
#define FORMAT_ID__GRADE 31
#define FORMAT_ID__ALTITUDE_OLD 32
#define FORMAT_ID__RAW_DATA 33
#define FORMAT_ID__TEMPERATURE 34
#define FORMAT_ID__INTERVAL_NUM 35
#define FORMAT_ID__DROPOUT_FLAGS 36
#define FORMAT_ID__RAW_DATA_FORMAT 37
#define FORMAT_ID__RAW_BARO_SENSOR 38
#define FORMAT_ID__ALTITUDE 39
#define FORMAT_ID__THRESHOLD_POWER 40


static int binFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "bin", "Joule Bin File", new BinFileReader());

struct BinField {
    int num;
    int id;
    int size; // in bytes
};

struct BinDefinition {
    int format_identifier;
    std::vector<BinField> fields;
};

struct BinFileReaderState
{
    static QMap<int,QString> global_record_types;
    static QMap<int,QString> global_format_identifiers;

    QMap<int, BinDefinition> local_format_identifiers;

    QSet<int> unknown_record_types, unknown_format_identifiers;
    QSet<int> unused_record_types;
    QSet<int> unexpected_record_types;

    QMap<int, QSet<int> > unknown_format_identifiers_for_record_types;
    QMap<int, QSet<int> > unused_format_identifiers_for_record_types;
    QMap<int, QSet<int> > unexpected_format_identifiers_for_record_types;

    QFile &file;
    QStringList &errors;
    RideFile *rideFile;
    time_t start_time;

    int interval;
    double last_interval_secs;

    bool stopped;


    BinFileReaderState(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), start_time(0),
        interval(0), last_interval_secs(0.0),  stopped(true)
    {
        if (global_record_types.isEmpty()) {
            global_record_types.insert(RECORD_TYPE__META,  "Meta Data");
            global_record_types.insert(RECORD_TYPE__RIDE_DATA,  "1 sec detail ride data");
            global_record_types.insert(RECORD_TYPE__RAW_DATA,  "Raw data packet");
            global_record_types.insert(RECORD_TYPE__SPARSE_DATA,  "Sparse ride data");
            global_record_types.insert(RECORD_TYPE__INTERVAL_DATA,  "Interval");
            global_record_types.insert(RECORD_TYPE__DATA_ERROR,  "Data error");
            global_record_types.insert(RECORD_TYPE__HISTORY,  "History summary record");
        }

        if (global_format_identifiers.isEmpty()) {
            global_format_identifiers.insert(FORMAT_ID__RIDE_START,  "Ride start");
            global_format_identifiers.insert(FORMAT_ID__RIDE_SAMPLE_RATE,  "Ride sample rate");
            global_format_identifiers.insert(FORMAT_ID__FIRMWARE_VERSION,  "Firmware Version");
            global_format_identifiers.insert(FORMAT_ID__LAST_UPDATE,  "Last update");
            global_format_identifiers.insert(FORMAT_ID__ODOMETER,  "Odometer");
            global_format_identifiers.insert(FORMAT_ID__PRIMARY_POWER_ID,  "Primary Power Radio ID");
            global_format_identifiers.insert(FORMAT_ID__SECONDARY_POWER_ID,  "Secondary Power Radio ID");
            global_format_identifiers.insert(FORMAT_ID__CHEST_STRAP_ID,  "Chest strap Radio ID");
            global_format_identifiers.insert(FORMAT_ID__CADENCE_ID,  "Cadense sensor Radio ID");
            global_format_identifiers.insert(FORMAT_ID__SPEED_ID,  "Speed sensor Radio ID");
            global_format_identifiers.insert(FORMAT_ID__RESISTANCE_UNITID,  "Resistance Unit Radio ID");
            global_format_identifiers.insert(FORMAT_ID__WORKOUT_ID,  "Workout ID");
            global_format_identifiers.insert(FORMAT_ID__USER_WEIGHT,  "User weight");
            global_format_identifiers.insert(FORMAT_ID__USER_CATEGORY,  "User training category");
            global_format_identifiers.insert(FORMAT_ID__USER_HR_ZONE_1,  "User HR Zone 1");
            global_format_identifiers.insert(FORMAT_ID__USER_HR_ZONE_2,  "User HR Zone 2");
            global_format_identifiers.insert(FORMAT_ID__USER_HR_ZONE_3,  "User HR Zone 3");
            global_format_identifiers.insert(FORMAT_ID__USER_HR_ZONE_4,  "User HR Zone 4");
            global_format_identifiers.insert(FORMAT_ID__USER_POWER_ZONE_1,  "User Power Zone 1");
            global_format_identifiers.insert(FORMAT_ID__USER_POWER_ZONE_2,  "User Power Zone 2");
            global_format_identifiers.insert(FORMAT_ID__USER_POWER_ZONE_3,  "User Power Zone 3");
            global_format_identifiers.insert(FORMAT_ID__USER_POWER_ZONE_4,  "User Power Zone 4");
            global_format_identifiers.insert(FORMAT_ID__USER_POWER_ZONE_5,  "User Power Zone 5");
            global_format_identifiers.insert(FORMAT_ID__WHEEL_CIRC,  "Wheel circumference");
            global_format_identifiers.insert(FORMAT_ID__RIDE_DISTANCE,  "Ride distance");
            global_format_identifiers.insert(FORMAT_ID__RIDE_TIME,  "Ride time");
            global_format_identifiers.insert(FORMAT_ID__POWER,  "Power");
            global_format_identifiers.insert(FORMAT_ID__TORQUE,  "Torque");
            global_format_identifiers.insert(FORMAT_ID__SPEED,  "Speed");
            global_format_identifiers.insert(FORMAT_ID__CADENCE,  "Cadence");
            global_format_identifiers.insert(FORMAT_ID__HEART_RATE,  "Heart rate");
            global_format_identifiers.insert(FORMAT_ID__GRADE,  "Grade");
            global_format_identifiers.insert(FORMAT_ID__ALTITUDE_OLD,  "Altitude");
            global_format_identifiers.insert(FORMAT_ID__RAW_DATA,  "Raw Data");
            global_format_identifiers.insert(FORMAT_ID__TEMPERATURE,  "Temperature");
            global_format_identifiers.insert(FORMAT_ID__INTERVAL_NUM,  "Interval number");
            global_format_identifiers.insert(FORMAT_ID__DROPOUT_FLAGS,  "Dropout flags");
            global_format_identifiers.insert(FORMAT_ID__RAW_DATA_FORMAT,  "Raw Data Format ID");
            global_format_identifiers.insert(FORMAT_ID__RAW_BARO_SENSOR,  "Raw Baro Sensor Value");
            global_format_identifiers.insert(FORMAT_ID__ALTITUDE,  "Altitude");
            global_format_identifiers.insert(FORMAT_ID__THRESHOLD_POWER,  "Threshold power");
        }
    }

    int read_byte(int *count = NULL, int *sum = NULL) {
        char c;
        if (file.read(&c, 1) != 1)
            return -1;
        if (sum)
            *sum += (0xff & c);
        if (count)
            *count += 1;
        return 0xff & c;
    }

    int read_double_byte(int *count = NULL, int *sum = NULL) {
        char c1,c2;
        if (file.read(&c1, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c2, 1) != 1)
            return -1;
        if (count)
            *count += 1;

        if (sum)
            *sum += (0xff & c1) + (0xff & c2);

        
        return  256*(0xff & c1) + (0xff & c2);
    }

    int read_four_byte(int *count = NULL, int *sum = NULL) {
        char c1,c2,c3,c4;
        if (file.read(&c1, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c2, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c3, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c4, 1) != 1)
            return -1;
        if (count)
            *count += 1;

        if (sum)
            *sum += (0xff & c1) + (0xff & c2) + (0xff & c3) + (0xff & c4);


        return 256*256*256*(0xff & c1) + 256*256*(0xff & c2) + 256*(0xff & c3) + (0xff & c4);
    }

    int read_date(int *count = NULL, int *sum = NULL) {
        char c1,c2,c3,c4,c5,c6,c7;
        if (file.read(&c1, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c2, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c3, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c4, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c5, 1) != 1)
            return -1;
        if (count)
            *count += 1;
        if (file.read(&c6, 1) != 1)
           return -1;
        if (count)
            *count += 1;
        if (file.read(&c7, 1) != 1)
            return -1;
        if (count)
            *count += 1;

        if (sum)
            *sum += (0xff & c1) + (0xff & c2) + (0xff & c3) + (0xff & c4) + (0xff & c5) + (0xff & c6) + (0xff & c7);

        QDateTime dateTime(QDate((0xff & c1)*256+(0xff & c2), (0xff & c3), (0xff & c4)), QTime((0xff & c5), (0xff & c6), (0xff & c7)), Qt::LocalTime);

        return dateTime.toTime_t();
    }


    void decodeMetaData(const BinDefinition &def, const std::vector<int> values) {
        int i = 0;
        QString deviceInfo = "";
        QDateTime t;
        
        foreach(const BinField &field, def.fields) {
            if (!global_format_identifiers.contains(field.id)) {
                unknown_format_identifiers.insert(field.id);
            } else {
                int value = values[i++];
                switch (field.id) {
                    case FORMAT_ID__RIDE_START : {
                        start_time = value;
                        t.setTime_t(value);
                        rideFile->setStartTime(t);
                        break;
                    }
                    case FORMAT_ID__RIDE_SAMPLE_RATE :
                        rideFile->setRecIntSecs(value/1000.0);
                        break;
                    case FORMAT_ID__FIRMWARE_VERSION :
                        //rideFile->setDeviceType(rideFile->deviceType()+ QString(" (%1)").arg(value));
                        deviceInfo += rideFile->deviceType()+QString(" Version %1\n").arg(value);
                        break;
                    case FORMAT_ID__LAST_UPDATE :
                        //t.setTime_t(value);
                        //deviceInfo += QString("Last update %1\n").arg(t.toString());
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__ODOMETER :
                        if (value>0)
                            deviceInfo += QString("Odometer %1km\n").arg(value/10.0);
                        break;
                    case FORMAT_ID__PRIMARY_POWER_ID :
                        if (value>0)
                            deviceInfo += QString("Primary Power Id %1\n").arg(value);
                        break;
                    case FORMAT_ID__SECONDARY_POWER_ID :
                        if (value>0)
                            deviceInfo += QString("Secondary Power Id %1\n").arg(value);
                        break;
                    case FORMAT_ID__CHEST_STRAP_ID :
                        if (value>0)
                            deviceInfo += QString("Chest strap Id %1\n").arg(value);
                        break;
                    case FORMAT_ID__CADENCE_ID :
                        if (value>0)
                            deviceInfo += QString("Cadence Id %1\n").arg(value);
                        break;
                    case FORMAT_ID__SPEED_ID :
                        if (value>0)
                            deviceInfo += QString("Speed Id %1\n").arg(value);
                        break;
                    case FORMAT_ID__RESISTANCE_UNITID :
                        if (value>0)
                            deviceInfo += QString("Resistance Unit Id %1\n").arg(value);
                        break;
                    case FORMAT_ID__WORKOUT_ID :
                        rideFile->setTag("Workout Code", QString(value));
                        break;
                    case FORMAT_ID__USER_WEIGHT :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_CATEGORY :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_HR_ZONE_1 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_HR_ZONE_2 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_HR_ZONE_3 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_HR_ZONE_4 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_POWER_ZONE_1 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_POWER_ZONE_2 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_POWER_ZONE_3 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_POWER_ZONE_4 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__USER_POWER_ZONE_5 :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__WHEEL_CIRC :
                        deviceInfo += QString("Wheel Circ. %1mm\n").arg(value);
                        break;
                    case FORMAT_ID__THRESHOLD_POWER :
                        deviceInfo += QString("Threshold Power %1W\n").arg(value);
                        break;

                    default:
                        unexpected_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                }
            }
        }
        rideFile->setTag("Device Info", deviceInfo);
    }

    void decodeRideData(const BinDefinition &def, const std::vector<int> values) {
        int i = 0;
        double secs = 0, alt = 0, cad = 0, km = 0, grade = 0, hr = 0;
        double nm = 0, kph = 0, watts = 0;

        foreach(const BinField &field, def.fields) {
            if (!global_format_identifiers.contains(field.id)) {
                unknown_format_identifiers.insert(field.id);
            } else {

                int value = values[i++];

                switch (field.id) {
                    case FORMAT_ID__RIDE_DISTANCE :
                        km = value/10000.0;
                    case FORMAT_ID__RIDE_TIME :
                        secs = value / 1000.0;
                        break;
                    case FORMAT_ID__POWER :
                        if (value <= 2999) // Limit from definition
                            watts = value;
                        break;
                    case FORMAT_ID__TORQUE :
                        nm = value;
                        break;
                    case FORMAT_ID__SPEED :
                        value = value*3.6/100.0;
                        if (value < 145) // Limit for data error
                            kph = value;
                        break;
                    case FORMAT_ID__CADENCE :
                        if (value < 255) // Limit for data error
                            cad = value;
                        break;
                    case FORMAT_ID__HEART_RATE :
                        if (value < 255) // Limit for data error
                            hr = value;
                        break;
                    case FORMAT_ID__GRADE :
                        grade = value;
                        break;
                    case FORMAT_ID__ALTITUDE :
                        alt = value/10.0;
                           break;
                    case FORMAT_ID__ALTITUDE_OLD :
                        alt = value/10.0;
                        break;
                    default:
                        unexpected_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                }
            }
        }

        double headwind = 0.0;
        int interval = 0;
        int lng = 0;
        int lat = 0;

        rideFile->appendPoint(secs, cad, hr, km, kph, nm, watts, alt, lng, lat, headwind, interval);
        //printf("addPoint time %f hr %f speed %f dist %f alt %f\n", secs, hr, kph, km, alt);
    }

    void decodeSparseData(const BinDefinition &def, const std::vector<int> values) {
        int i = 0;
        int temperature_count = 0;
        double temperature = 0.0;
        
        foreach(const BinField &field, def.fields) {
            if (!global_format_identifiers.contains(field.id)) {
                unknown_format_identifiers.insert(field.id);
            } else {
                int value = values[i++];
                switch (field.id) {
                    case FORMAT_ID__TEMPERATURE :
                        // use for average
                        temperature += value/10.0;
                        temperature_count ++;
                        break;
                    case FORMAT_ID__RIDE_TIME :
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;

                    default:
                        unexpected_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                }
            }
        }
        rideFile->setTag("Temperature", QString("%1").arg(temperature/temperature_count));
    }

    void decodeIntervalData(const BinDefinition &def, const std::vector<int> values) {
        int i = 0;
        double secs = 0;

        foreach(const BinField &field, def.fields) {
            if (!global_format_identifiers.contains(field.id)) {
                unknown_format_identifiers.insert(field.id);
            } else {
                int value = values[i++];
                switch (field.id) {
                    case FORMAT_ID__INTERVAL_NUM :
                        interval = value;
                        break;
                    case FORMAT_ID__RIDE_TIME :
                        secs = value / 1000.0;;
                        break;

                    default:
                        unexpected_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                }
            }
        }
        if (interval>1) {
            rideFile->addInterval(last_interval_secs, secs, QString("%1").arg(interval-1));
        }
        last_interval_secs = secs;

    }

    void decodeDataError(const BinDefinition &def, const std::vector<int> values) {
        int i = 0;
        foreach(const BinField &field, def.fields) {
            if (!global_format_identifiers.contains(field.id)) {
                unknown_format_identifiers.insert(field.id);
            } else {
                int value = values[i++];
           
                bool b0 = (value % 2);
                bool b1 = (value / 2>1);
                bool b2 = (value / 4>1);
                bool b3 = (value / 8>1);
                bool b4 = (value / 16>1);
                bool b5 = (value / 32>1);
                bool b6 = (value / 64>1);
                bool b7 = (value / 128>1);
                
                QString b = QString("DataError : %1 %2 %3 %4 %5 %6 %7 %8").arg(b0?"0":"").arg(b1?"1":"").arg(b2?"2":"").arg(b3?"3":"").arg(b4?"4":"").arg(b5?"5":"").arg(b6?"6":"").arg(b7?"7":"");
                
                switch (field.id) {
                    case FORMAT_ID__DROPOUT_FLAGS :
                        
                        //errors << QString("DataError field.id %1 value %2").arg(field.id).arg(b);
                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;
                    case FORMAT_ID__RIDE_TIME :
                        //errors << QString("DataError time field.id %1 value %2").arg(field.id).arg(value/1000);

                        unused_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                        break;

                    default:
                        unexpected_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                }
            }
        }
    }

    void decodeHistoryData(const BinDefinition &def, const std::vector<int> /*values*/) {
        //int i = 0;
        foreach(const BinField &field, def.fields) {
            if (!global_format_identifiers.contains(field.id)) {
                unknown_format_identifiers.insert(field.id);
            } else {
                //int value = values[i++];
                switch (field.id) {
                    default:
                        unexpected_format_identifiers_for_record_types[def.format_identifier].insert(field.id);
                }
            }
        }
    }



    int read_record(bool &stop, QStringList &errors) {
        int sum = 0;
        int bytes_read = 0;

        int record_type = read_byte(&bytes_read, &sum); // Always 0xFF

        if (record_type == -1) {
           errors << QString("Truncated file");
           //bytes_read++;
           return bytes_read;
        } else if (record_type == 255) {
            int format_identifier = read_byte(&bytes_read, &sum);

            if (format_identifier == -1) {
               errors << QString("Truncated file");
               //bytes_read++;
               return bytes_read;
            } else if (!global_record_types.contains(format_identifier)) {
                errors << QString("unknown format_identifier %1").arg(format_identifier);
                stop = true;
                return bytes_read;
            }
            int nb_meta = read_double_byte(&bytes_read, &sum);

            local_format_identifiers.insert(format_identifier, BinDefinition());
             //printf("- format_identifier : %d\n", format_identifier);
            BinDefinition &def = local_format_identifiers[format_identifier];
            def.format_identifier = format_identifier;

            for (int i = 0; i < nb_meta; ++i) {
               def.fields.push_back(BinField());
               BinField &field = def.fields.back();
               int field_id = read_double_byte(&bytes_read,&sum);
               field.id = field_id;
               int field_size = read_double_byte(&bytes_read,&sum);
               field.size = field_size;
               //printf("- field %d : %d\n", field_id, field_size);
            }

            int checksum = read_double_byte(&bytes_read);
            //printf("- checksum %d : %d\n", checksum, sum);

            if (checksum == -1) {
               errors << QString("Truncated file");
               return bytes_read;
            } else if (checksum != sum) {
                errors << QString("bad checksum: %1").arg(sum);
                stop = true;
                return bytes_read;
            }
        }
        else {
            int format_identifier = record_type;
            //printf("- data for format identifier : %d\n", format_identifier);

            if (!local_format_identifiers.contains(format_identifier)) {
                errors << QString("undefined format_identifier: %1").arg(format_identifier);
                stop = true;
                return bytes_read;
            } else {
                const BinDefinition &def = local_format_identifiers[format_identifier];
                //printf("- fields type : %d\n", def.format_identifier);
                std::vector<int> values;
                foreach(const BinField &field, def.fields) {
                    //printf("- field : %d \n", field.id);
                    int v;
                    switch (field.size) {
                        case 1: v = read_byte(&bytes_read,&sum); break;
                        case 2: v = read_double_byte(&bytes_read,&sum); break;
                        case 4: v = read_four_byte(&bytes_read,&sum); break;
                        case 7: v = read_date(&bytes_read,&sum); break;
                        default:
                            for (int i = 0; i < field.size; ++i)
                                read_byte(&bytes_read,&sum);
                                errors << QString("unsupported field size %1").arg(field.size);
                     }
                     values.push_back(v);
                     //printf("- %d : %d\n", field.id, v);
                }
                 int checksum = read_double_byte(&bytes_read);
                 //printf("- checksum %d : %d\n", checksum, sum);

                 if (checksum == -1) {
                    errors << QString("Truncated file");
                    return bytes_read;
                 } else if (checksum != sum) {
                   errors << QString("bad checksum: %1").arg(sum);
                   stop = true;
                   return bytes_read;
                }

                switch (def.format_identifier) {
                       case RECORD_TYPE__META:          decodeMetaData(def, values); break;
                       case RECORD_TYPE__RIDE_DATA:     decodeRideData(def, values); break;
                       case RECORD_TYPE__RAW_DATA:
                           unused_record_types.insert(def.format_identifier);
                           break;
                       case RECORD_TYPE__SPARSE_DATA:   decodeSparseData(def, values); break;
                       case RECORD_TYPE__INTERVAL_DATA: decodeIntervalData(def, values); break;
                       case RECORD_TYPE__DATA_ERROR:    decodeDataError(def, values); break;
                       case RECORD_TYPE__HISTORY:       decodeHistoryData(def, values); break;

                       default:
                           unexpected_record_types.insert(def.format_identifier);
                   }
             }
         }
         return bytes_read;
    }

    RideFile * run() {
        errors.clear();
        rideFile = new RideFile;
        rideFile->setDeviceType("Joule");

        if (!file.open(QIODevice::ReadOnly)) {
            delete rideFile;
            return NULL;
        }

        //
        bool stop = false;

        int data_size = file.size();
        int bytes_read = 0;

        while (!stop && (bytes_read < data_size)) {
            bytes_read += read_record(stop, errors);
        }

        if (last_interval_secs>0) {
            rideFile->addInterval(last_interval_secs, rideFile->dataPoints().last()->secs, QString("%1").arg(interval));
        }
        if (stop) {
            delete rideFile;
            return NULL;
        }
        else {
            foreach(int num, unknown_record_types) {
                errors << QString("unknow record type %1; ignoring it").arg(num);
            }
            foreach(int num, unknown_format_identifiers) {
                errors << QString("unknow format identifier %1; ignoring it").arg(num);
            }
            /*foreach(int num, unused_record_types) {
                errors << QString("unused record type \"%1\" (%2)\n").arg(global_record_types[num].toAscii().constData())
                                                                     .arg(num);
            }
            foreach(QSet<int> set, unused_format_identifiers_for_record_types) {
                foreach(int num, set) {
                    int record_type = unused_format_identifiers_for_record_types.keys(set).takeFirst();
                    errors << QString("unused format identifier \"%1\" (%2) in \"%3\" (%4)\n")
                           .arg(global_format_identifiers[num].toAscii().constData())
                           .arg(num)
                           .arg(global_record_types[record_type].toAscii().constData())
                           .arg(record_type);
                }
            }*/
            foreach(int num, unexpected_record_types) {
                errors << QString("unexpected record type %1 (%2)\n").arg(global_record_types[num]).arg(num);
            }
            foreach(QSet<int> set, unexpected_format_identifiers_for_record_types) {
                foreach(int num, set) {
                    int record_type = unexpected_format_identifiers_for_record_types.keys(set).takeFirst();
                    errors << QString("unexpected format identifier \"%1\" (%2) in \"%3\" (%4)\n")
                            .arg(global_format_identifiers[num].toAscii().constData())
                            .arg(num)
                            .arg(global_record_types[record_type].toAscii().constData())
                            .arg(record_type);
                }
            }
            return rideFile;
        }
    }
};

QMap<int,QString> BinFileReaderState::global_record_types;
QMap<int,QString> BinFileReaderState::global_format_identifiers;

RideFile *BinFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    QSharedPointer<BinFileReaderState> state(new BinFileReaderState(file, errors));
    return state->run();
}

