/*
 * Copyright (c) 2007-2008 Sean C. Rhea (srhea@srhea.net)
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

#include "FitRideFile.h"
#include <QSharedPointer>
#include <QMap>
#include <QSet>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define RECORD_TYPE 20

static int fitFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "fit", "Garmin FIT", new FitFileReader());

/*static const char *base_type_names[] = {
    "enum",    // 0
    "int8",    // 1
    "uint8",   // 2
    "int16",   // 3
    "uint16",  // 4
    "int32",   // 5
    "uint32",  // 6
    "string",  // 7
    "float32", // 8
    "float64", // 9
    "uint8z",  // 10
    "uint16z", // 11
    "uint32z", // 12
    "byte"     // 13
}; */
static const int base_type_names_size(14);

static const QDateTime qbase_time(QDate(1989, 12, 31), QTime(0, 0, 0), Qt::UTC);

struct FitField {
    int num;
    int type;
    int size; // in bytes
};

struct FitDefinition {
    int global_msg_num;
    std::vector<FitField> fields;
};

struct FitFileReaderState
{
    static QMap<int,QString> global_msg_names;

    QFile &file;
    QStringList &errors;
    RideFile *rideFile;
    time_t start_time;
    time_t last_time;
    QMap<int, FitDefinition> local_msg_types;
    QSet<int> unknown_record_fields, unknown_global_msg_nums;
    int interval;
    int devices;
    bool stopped;
    int last_event_type;
    int last_event;
    int last_msg_type;

    FitFileReaderState(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), start_time(0),
        last_time(0), interval(0), devices(0), stopped(true),
        last_event_type(-1), last_event(-1), last_msg_type(-1)
    {
        if (global_msg_names.isEmpty()) {
            global_msg_names.insert(0,  "file_id");
            global_msg_names.insert(18, "session");
            global_msg_names.insert(19, "lap");
            global_msg_names.insert(RECORD_TYPE, "record");
            global_msg_names.insert(21, "event");
            global_msg_names.insert(22, "undocumented");
            global_msg_names.insert(23, "device_info");
            global_msg_names.insert(34, "activity");
            global_msg_names.insert(49, "file_creator");
            global_msg_names.insert(72, "undocumented_2"); // New for Garmin 800
        }
    }

    struct TruncatedRead {};

    int read_byte(int *count = NULL) {
        char c;
        if (file.read(&c, 1) != 1)
            throw TruncatedRead();
        if (count)
            (*count) += 1;
        return 0xff & c;
    }

    int read_short(int *count = NULL) {
        uint16_t s;
        if (file.read(reinterpret_cast<char*>(&s), 2) != 2)
            throw TruncatedRead();
        if (count)
            (*count) += 2;
        return 0xffff & s;
    }

    int read_long(int *count = NULL) {
        uint32_t l;
        if (file.read(reinterpret_cast<char*>(&l), 4) != 4)
            throw TruncatedRead();
        if (count)
            (*count) += 4;
        return l;
    }

    void decodeFileId(const FitDefinition &def, int, const std::vector<int> values) {
        int i = 0;
        int manu = -1, prod = -1;
        foreach(const FitField &field, def.fields) {
            int value = values[i++];
            switch (field.num) {
                case 1: manu = value; break;
                case 2: prod = value; break;
                default: ; // do nothing
            }
        }
        if (manu == 1) {
            switch (prod) {
                case 717: rideFile->setDeviceType("Garmin FR405"); break;
                case 782: rideFile->setDeviceType("Garmin FR50"); break;
                case 988: rideFile->setDeviceType("Garmin FR60"); break;
                case 1018: rideFile->setDeviceType("Garmin FR310XT"); break;
                case 1036: rideFile->setDeviceType("Garmin Edge 500"); break;
                case 1169: rideFile->setDeviceType("Garmin Edge 800"); break;
                default: rideFile->setDeviceType(QString("Unknown Garmin Device %1").arg(prod));
            }
        }
        else {
            rideFile->setDeviceType(QString("Unknown FIT Device %1:%2").arg(manu).arg(prod));
        }
    }

    void decodeEvent(const FitDefinition &def, int, const std::vector<int> values) {
        time_t time = 0;
        int event = -1;
        int event_type = -1;
        int i = 0;
        foreach(const FitField &field, def.fields) {
            int value = values[i++];
            switch (field.num) {
                case 253: time = value + qbase_time.toTime_t(); break;
                case 0: event = value; break;
                case 1: event_type = value; break;
                default: ; // do nothing
            }
        }
        if (event == 0) { // Timer event
            switch (event_type) {
                case 0: // start
                    stopped = false;
                    break;
                case 1: // stop
                    stopped = true;
                    break;
                case 2: // consecutive_depreciated
                case 3: // marker
                    break;
                case 4: // stop all
                    stopped = true;
                    break;
                case 5: // begin_depreciated
                case 6: // end_depreciated
                case 7: // end_all_depreciated
                case 8: // stop_disable
                    stopped = true;
                    break;
                case 9: // stop_disable_all
                    stopped = true;
                    break;
                default:
                    errors << QString("Unknown event type %1").arg(event_type);
            }
        }
        // printf("event type %d\n", event_type);
        last_event = event;
        last_event_type = event_type;
    }

    void decodeLap(const FitDefinition &def, int time_offset, const std::vector<int> values) {
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        int i = 0;
        time_t this_start_time = 0;
        ++interval;
        foreach(const FitField &field, def.fields) {
            int value = values[i++];
            switch (field.num) {
                case 253: time = value + qbase_time.toTime_t(); break;
                case 2: this_start_time = value + qbase_time.toTime_t(); break;
                default: ; // ignore it
            }
        }
        if (this_start_time == 0)
            errors << QString("lap %1 has no start time").arg(interval);
        else {
            rideFile->addInterval(this_start_time - start_time, time - start_time,
                                  QString("%1").arg(interval));
        }
    }

    void decodeRecord(const FitDefinition &def, int time_offset, const std::vector<int> values) {
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        double alt = 0, cad = 0, km = 0, grade = 0, hr = 0, lat = 0, lng = 0, badgps = 0;
        double resistance = 0, kph = 0, temperature = 0, time_from_course = 0, watts = 0;
        int lati = 0x7fffffff, lngi = 0x7fffffff;
        int i = 0;
        foreach(const FitField &field, def.fields) {
            int value = values[i++];
            switch (field.num) {
                case 253: time = value + qbase_time.toTime_t(); break;
                case 0: lati = value; break;
                case 1: lngi = value; break;
                case 2: alt = (value == 0xffff) ? 0 : (value / 5.0 - 500.0); break;
                case 3: hr = (value == 0xff) ? 0 : value; break;
                case 4: cad = (value == 0xff) ? 0 : value; break;
                case 5: km = ((uint32_t) value == 0xffffffff) ? 0 : value / 100000.0; break;
                case 6: kph = (value == 0xffff) ? 0 : value * 3.6 / 1000.0; break;
                case 7: watts = (value == 0xffff) ? 0 : value; break;
                case 9: grade = (value == 0x7fff) ? 0 : value / 100.0; break;
                case 10: resistance = (value == 0xff) ? 0 : value; break;
                case 11: time_from_course = (value == 0x7fffffff) ? 0 : value / 1000.0; break;
                case 13: temperature = (value == 0x7f) ? 0 : value; break;
                default: unknown_record_fields.insert(field.num);
            }
        }
        if (time == last_time)
            return; // Sketchy, but some FIT files do this.
        if (stopped) {
            // As it turns out, this happens all the time in some FIT files.
            // Since we don't really understand the meaning, don't make noise.
            /*
            errors << QString("At %1 seconds, time is stopped, but got record "
                              "anyway.  Ignoring it.  Last event type was "
                              "%2 for event %3.").arg(time-start_time).arg(last_event_type).arg(last_event);
            return;
            */
        }
        if (lati != 0x7fffffff && lngi != 0x7fffffff) {
            lat = lati * 180.0 / 0x7fffffff;
            lng = lngi * 180.0 / 0x7fffffff;
        } else
        {
            // If lat/lng are missng, set to 0/0 and fill point from last point as 0/0)
            lat = 0;
            lng = 0;
            badgps = 1;
        }
        if (start_time == 0) {
            start_time = time - 1; // XXX: recording interval?
            QDateTime t;
            t.setTime_t(start_time);
            rideFile->setStartTime(t);
        }
        double secs = time - start_time;
        double nm = 0;
        double headwind = 0.0;
        int interval = 0;
        if ((last_msg_type == RECORD_TYPE) && (last_time != 0) && (time > last_time + 1)) {
            // Evil smart recording.  Linearly interpolate missing points.
            RideFilePoint *prevPoint = rideFile->dataPoints().back();
            int deltaSecs = (int) (secs - prevPoint->secs);
            assert(deltaSecs == secs - prevPoint->secs); // no fractional part
            // This is only true if the previous record was of type record:
            assert(deltaSecs == time - last_time);
            // If the last lat/lng was missing (0/0) then all points up to lat/lng are marked as 0/0.
            if (prevPoint->lat == 0 && prevPoint->lon == 0 ) {
                badgps = 1;
            }
            double deltaCad = cad - prevPoint->cad;
            double deltaHr = hr - prevPoint->hr;
            double deltaDist = km - prevPoint->km;
            double deltaSpeed = kph - prevPoint->kph;
            double deltaTorque = nm - prevPoint->nm;
            double deltaPower = watts - prevPoint->watts;
            double deltaAlt = alt - prevPoint->alt;
            double deltaLon = lng - prevPoint->lon;
            double deltaLat = lat - prevPoint->lat;
            double deltaHeadwind = headwind - prevPoint->headwind;
            for (int i = 1; i < deltaSecs; i++) {
                double weight = 1.0 * i / deltaSecs;
                rideFile->appendPoint(
                    prevPoint->secs + (deltaSecs * weight),
                    prevPoint->cad + (deltaCad * weight),
                    prevPoint->hr + (deltaHr * weight),
                    prevPoint->km + (deltaDist * weight),
                    prevPoint->kph + (deltaSpeed * weight),
                    prevPoint->nm + (deltaTorque * weight),
                    prevPoint->watts + (deltaPower * weight),
                    prevPoint->alt + (deltaAlt * weight),
                    (badgps == 1) ? 0 : prevPoint->lon + (deltaLon * weight),
                    (badgps == 1) ? 0 : prevPoint->lat + (deltaLat * weight),
                    prevPoint->headwind + (deltaHeadwind * weight),
                    interval);
            }
            prevPoint = rideFile->dataPoints().back();
        }
        rideFile->appendPoint(secs, cad, hr, km, kph, nm, watts, alt, lng, lat, headwind, interval);
        last_time = time;
    }

    int read_record(bool &stop, QStringList &errors) {
        stop = false;
        int count = 0;
        int header_byte = read_byte(&count);
        if (!(header_byte & 0x80) && (header_byte & 0x40)) {
            // Definition record
            int local_msg_type = header_byte & 0xf;
            int reserved = read_byte(&count); (void) reserved; // unused
            int architecture = read_byte(&count);
            int global_msg_num = read_short(&count);
            if (!global_msg_names.contains(global_msg_num)) {
                errors << QString("unknown global_msg_num %1").arg(global_msg_num);
                stop = true;
                return count;
            }
            /*printf("definition: local type %d is %s\n",
                   local_msg_type,
                   global_msg_names[global_msg_num].toAscii().constData());*/
            if (architecture != 0) {
                errors << QString("unsupported architecture %1").arg(architecture);
                stop = true;
                return count;
            }
            int num_fields = read_byte(&count);
            // printf(", %d num_fields:\n", num_fields);
            local_msg_types.insert(local_msg_type, FitDefinition());
            FitDefinition &def = local_msg_types[local_msg_type];
            def.global_msg_num = global_msg_num;
            for (int i = 0; i < num_fields; ++i) {
                def.fields.push_back(FitField());
                FitField &field = def.fields.back();
                int field_def_num = read_byte(&count);
                int field_size = read_byte(&count);
                field.size = field_size;
                int base_type = read_byte(&count);
                int base_type_num = base_type & 0x1f;
                if (base_type_num >= base_type_names_size) {
                    errors << QString("unknown base type %1").arg(base_type_num);
                    stop = true;
                    return count;
                }
                field.type = base_type_num;
                field.num = field_def_num;
                /*printf("  field %d: %d bytes, num %d, type %s, %s endianness\n",
                       i, field_size, field_def_num, base_type_names[base_type_num],
                       (base_type & 0x80) ? "with" : "without");*/
            }
        }
        else {
            int local_msg_type = 0;
            int time_offset = 0;
            if (header_byte & 0x80) {
                // compressed time record
                local_msg_type = (header_byte >> 5) & 0x3;
                time_offset = header_byte & 0x1f;
            }
            else {
                local_msg_type = header_byte & 0xf;
            }

            if (!local_msg_types.contains(local_msg_type)) {
                errors << QString("unrecognized local type %1").arg(local_msg_type);
                stop = true;
                return count;
            }
            const FitDefinition &def = local_msg_types[local_msg_type];
            std::vector<int> values;
            foreach(const FitField &field, def.fields) {
                int v;
                switch (field.size) {
                    case 1: v = read_byte(&count); break;
                    case 2: v = read_short(&count); break;
                    case 4: v = read_long(&count); break;
                    default:
                        errors << QString("unsupported field size %1").arg(field.size);
                        stop = true;
                        return count;
                }
                values.push_back(v);
            }
            // Most of the record types in the FIT format aren't actually all
            // that useful.  FileId, Lap, and Record clearly are.  The one
            // other one that might be useful is DeviceInfo, but it doesn't
            // seem to be filled in properly.  Sean's Cinqo, for example,
            // shows up as manufacturer #65535, even though it should be #7.
            // printf("msg: %s\n", global_msg_names[def.global_msg_num].toAscii().constData());
            switch (def.global_msg_num) {
                case 0:  decodeFileId(def, time_offset, values); break;
                case 19: decodeLap(def, time_offset, values); break;
                case RECORD_TYPE: decodeRecord(def, time_offset, values); break;
                case 21: decodeEvent(def, time_offset, values); break;
                case 23: /* device info */
                case 18: /* session */
                case 22: /* undocumented */
                case 72: /* undocumented  - new for garmin 800*/
                case 34: /* activity */
                case 49: /* file creator */
                    break;
                default:
                    /*
                    int i = 0;
                    printf("----------------\n");
                    foreach(const FitField &field, def.fields) {
                        int value = values[i++];
                        printf("msg %d: field %d = %d\n", def.global_msg_num, field.num, value);
                    }
                    */
                    unknown_global_msg_nums.insert(def.global_msg_num);
            }
            last_msg_type = def.global_msg_num;
        }
        return count;
    }

    RideFile * run() {
        rideFile = new RideFile;
        rideFile->setDeviceType("Garmin FIT"); // XXX: read from device msg?
        rideFile->setRecIntSecs(1.0); // XXX: always?
        if (!file.open(QIODevice::ReadOnly)) {
            delete rideFile;
            return NULL;
        }
        int header_size = read_byte();
        if (header_size != 12) {
            errors << QString("bad header size: %1").arg(header_size);
            delete rideFile;
            return NULL;
        }
        int protocol_version = read_byte();
        (void) protocol_version;
        int profile_version = read_short(); // not sure what to do with this
        (void) profile_version;
        int data_size = read_long(); // always little endian
        char fit_str[5];
        if (file.read(fit_str, 4) != 4) {
            errors << "truncated header";
            delete rideFile;
            return NULL;
        }
        fit_str[4] = '\0';
        if (strcmp(fit_str, ".FIT") != 0) {
            errors << QString("bad header, expected \".FIT\" but got \"%1\"").arg(fit_str);
            delete rideFile;
            return NULL;
        }
        int bytes_read = 0;
        bool stop = false;
        try {
            while (!stop && (bytes_read < data_size))
                bytes_read += read_record(stop, errors);
        }
        catch (TruncatedRead &e) {
            errors << "truncated file body";
            delete rideFile;
            return NULL;
        }
        if (stop) {
            delete rideFile;
            return NULL;
        }
        else {
            int crc = read_short();
            (void) crc;
            foreach(int num, unknown_global_msg_nums) {
                if (global_msg_names.contains(num)) {
                    errors << ("unsupported global message \"" + global_msg_names[num] +
                               "\" (%1); ignoring it").arg(num);
                }
                else {
                    errors << QString("unsupported global message number %1; ignoring it").arg(num);
                }
            }
            foreach(int num, unknown_record_fields)
                errors << QString("unknown record field %1; ignoring it").arg(num);
            return rideFile;
        }
    }
};

QMap<int,QString> FitFileReaderState::global_msg_names;

RideFile *FitFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    QSharedPointer<FitFileReaderState> state(new FitFileReaderState(file, errors));
    return state->run();
}

