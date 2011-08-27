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
#include <QtEndian>
#include <QDebug>
#include <stdio.h>
#include <stdint.h>
#include <limits>

#define RECORD_TYPE 20

static int fitFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "fit", "Garmin FIT", new FitFileReader());

static const QDateTime qbase_time(QDate(1989, 12, 31), QTime(0, 0, 0), Qt::UTC);

struct FitField {
    int num;
    int type; // FIT base_type
    int size; // in bytes
};

struct FitDefinition {
    int global_msg_num;
    bool is_big_endian;
    std::vector<FitField> fields;
};

/* FIT has uint32 as largest integer type. So qint64 is large enough to
 * store all integer types - no matter if they're signed or not */
// XXX this needs to get changed to support non-integer values
typedef qint64 fit_value_t;
#define NA_VALUE std::numeric_limits<fit_value_t>::max()


struct FitFileReaderState
{
    QFile &file;
    QStringList &errors;
    RideFile *rideFile;
    time_t start_time;
    time_t last_time;
    QMap<int, FitDefinition> local_msg_types;
    QSet<int> unknown_record_fields, unknown_global_msg_nums, unknown_base_type;
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
    }

    struct TruncatedRead {};
    struct BadDelta {};

    void read_unknown( int size, int *count = NULL ){
        char c[size+1];

        // XXX: just seek instead of read?
        if (file.read(c, size ) != size)
            throw TruncatedRead();
        if (count)
            (*count) += size;
    }

    fit_value_t read_int8(int *count = NULL) {
        qint8 i;
        if (file.read(reinterpret_cast<char*>( &i), 1) != 1)
            throw TruncatedRead();
        if (count)
            (*count) += 1;

        return i == 0x7f ? NA_VALUE : i;
    }

    fit_value_t read_uint8(int *count = NULL) {
        quint8 i;
        if (file.read(reinterpret_cast<char*>( &i), 1) != 1)
            throw TruncatedRead();
        if (count)
            (*count) += 1;

        return i == 0xff ? NA_VALUE : i;
    }

    fit_value_t read_uint8z(int *count = NULL) {
        quint8 i;
        if (file.read(reinterpret_cast<char*>( &i), 1) != 1)
            throw TruncatedRead();
        if (count)
            (*count) += 1;

        return i == 0x00 ? NA_VALUE : i;
    }

    fit_value_t read_int16(bool is_big_endian, int *count = NULL) {
        qint16 i;
        if (file.read(reinterpret_cast<char*>(&i), 2) != 2)
            throw TruncatedRead();
        if (count)
            (*count) += 2;

        i = is_big_endian
            ? qFromBigEndian<qint16>( i )
            : qFromLittleEndian<qint16>( i );

        return i == 0x7fff ? NA_VALUE : i;
    }

    fit_value_t read_uint16(bool is_big_endian, int *count = NULL) {
        quint16 i;
        if (file.read(reinterpret_cast<char*>(&i), 2) != 2)
            throw TruncatedRead();
        if (count)
            (*count) += 2;

        i = is_big_endian
            ? qFromBigEndian<quint16>( i )
            : qFromLittleEndian<quint16>( i );

        return i == 0xffff ? NA_VALUE : i;
    }

    fit_value_t read_uint16z(bool is_big_endian, int *count = NULL) {
        quint16 i;
        if (file.read(reinterpret_cast<char*>(&i), 2) != 2)
            throw TruncatedRead();
        if (count)
            (*count) += 2;

        i = is_big_endian
            ? qFromBigEndian<quint16>( i )
            : qFromLittleEndian<quint16>( i );

        return i == 0x0000 ? NA_VALUE : i;
    }

    fit_value_t read_int32(bool is_big_endian, int *count = NULL) {
        qint32 i;
        if (file.read(reinterpret_cast<char*>(&i), 4) != 4)
            throw TruncatedRead();
        if (count)
            (*count) += 4;

        i = is_big_endian
            ? qFromBigEndian<qint32>( i )
            : qFromLittleEndian<qint32>( i );

        return i == 0x7fffffff ? NA_VALUE : i;
    }

    fit_value_t read_uint32(bool is_big_endian, int *count = NULL) {
        quint32 i;
        if (file.read(reinterpret_cast<char*>(&i), 4) != 4)
            throw TruncatedRead();
        if (count)
            (*count) += 4;

        i = is_big_endian
            ? qFromBigEndian<quint32>( i )
            : qFromLittleEndian<quint32>( i );

        return i == 0xffffffff ? NA_VALUE : i;
    }

    fit_value_t read_uint32z(bool is_big_endian, int *count = NULL) {
        quint32 i;
        if (file.read(reinterpret_cast<char*>(&i), 4) != 4)
            throw TruncatedRead();
        if (count)
            (*count) += 4;

        i = is_big_endian
            ? qFromBigEndian<quint32>( i )
            : qFromLittleEndian<quint32>( i );

        return i == 0x00000000 ? NA_VALUE : i;
    }

    void decodeFileId(const FitDefinition &def, int, const std::vector<fit_value_t> values) {
        int i = 0;
        int manu = -1, prod = -1;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++];

            if( value == NA_VALUE )
                continue;

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

    void decodeEvent(const FitDefinition &def, int, const std::vector<fit_value_t> values) {
        time_t time = 0;
        int event = -1;
        int event_type = -1;
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++];

            if( value == NA_VALUE )
                continue;

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

    void decodeLap(const FitDefinition &def, int time_offset, const std::vector<fit_value_t> values) {
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        int i = 0;
        time_t this_start_time = 0;
        ++interval;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++];

            if( value == NA_VALUE )
                continue;

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

    void decodeRecord(const FitDefinition &def, int time_offset, const std::vector<fit_value_t> values) {
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        double alt = 0, cad = 0, km = 0, grade = 0, hr = 0, lat = 0, lng = 0, badgps = 0;
        double resistance = 0, kph = 0, temperature = 0, time_from_course = 0, watts = 0;
        fit_value_t lati = NA_VALUE, lngi = NA_VALUE;
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++];

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: time = value + qbase_time.toTime_t();
                          // Time MUST NOT go backwards
                          // You canny break the laws of physics, Jim
                          if (time < last_time) time = last_time;
                          break;
                case 0: lati = value; break;
                case 1: lngi = value; break;
                case 2: alt = value / 5.0 - 500.0; break;
                case 3: hr = value; break;
                case 4: cad = value; break;
                case 5: km = value / 100000.0; break;
                case 6: kph = value * 3.6 / 1000.0; break;
                case 7: watts = value; break;
                case 8: break; // XXX packed speed/dist
                case 9: grade = value / 100.0; break;
                case 10: resistance = value; break;
                case 11: time_from_course = value / 1000.0; break;
                case 12: break; // XXX "cycle_length"
                case 13: temperature = value; break;
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
        if (lati != NA_VALUE && lngi != NA_VALUE) {
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

        //printf( "point time=%d lat=%.2lf lon=%.2lf alt=%.1lf hr=%.0lf "
        //    "cad=%.0lf km=%.1lf kph=%.1lf watts=%.0lf grade=%.1lf "
        //    "resist=%.1lf off=%.1lf temp=%.1lf\n",
        //    time, lat, lng, alt, hr,
        //    cad, km, kph, watts, grade,
        //    resistance, time_from_course, temperature );

        double secs = time - start_time;
        double nm = 0;
        double headwind = 0.0;
        int interval = 0;
        if ((last_msg_type == RECORD_TYPE) && (last_time != 0) && (time > last_time + 1)) {
            // Evil smart recording.  Linearly interpolate missing points.
            RideFilePoint *prevPoint = rideFile->dataPoints().back();
            int deltaSecs = (int) (secs - prevPoint->secs);
            if(deltaSecs != secs - prevPoint->secs)
                throw BadDelta(); // no fractional part
            // This is only true if the previous record was of type record:
            if(deltaSecs != time - last_time)
                throw BadDelta();
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
        int header_byte = read_uint8(&count);
        if (!(header_byte & 0x80) && (header_byte & 0x40)) {
            // Definition record
            int local_msg_type = header_byte & 0xf;

            local_msg_types.insert(local_msg_type, FitDefinition());
            FitDefinition &def = local_msg_types[local_msg_type];

            int reserved = read_uint8(&count); (void) reserved; // unused
            def.is_big_endian = read_uint8(&count);
            def.global_msg_num = read_uint16(def.is_big_endian, &count);
            int num_fields = read_uint8(&count);
            //printf("definition: local type=%d global=%d arch=%d fields=%d\n",
            //       local_msg_type, def.global_msg_num, def.is_big_endian,
            //       num_fields );

            for (int i = 0; i < num_fields; ++i) {
                def.fields.push_back(FitField());
                FitField &field = def.fields.back();

                field.num = read_uint8(&count);
                field.size = read_uint8(&count);
                int base_type = read_uint8(&count);
                field.type = base_type & 0x1f;
                //printf("  field %d: %d bytes, num %d, type %d\n",
                //       i, field.size, field.num, field.type );
            }
        }
        else {
            // Data record
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
                errors << QString("local type %1 without previous definition").arg(local_msg_type);
                stop = true;
                return count;
            }
            const FitDefinition &def = local_msg_types[local_msg_type];
            //printf( "message local=%d global=%d\n", local_msg_type,
            //    def.global_msg_num );

            std::vector<fit_value_t> values;
            foreach(const FitField &field, def.fields) {
                fit_value_t v;
                switch (field.type) {
                    case 0: v = read_uint8(&count); break;
                    case 1: v = read_int8(&count); break;
                    case 2: v = read_uint8(&count); break;
                    case 3: v = read_int16(def.is_big_endian, &count); break;
                    case 4: v = read_uint16(def.is_big_endian, &count); break;
                    case 5: v = read_int32(def.is_big_endian, &count); break;
                    case 6: v = read_uint32(def.is_big_endian, &count); break;
                    case 10: v = read_uint8z(&count); break;
                    case 11: v = read_uint16z(def.is_big_endian, &count); break;
                    case 12: v = read_uint32z(def.is_big_endian, &count); break;
                    //XXX: support float, string + byte base types
                    default:
                        read_unknown( field.size, &count );
                        v = NA_VALUE;
                        unknown_base_type.insert(field.num);
                }
                values.push_back(v);
                //printf( " field: type=%d num=%d value=%lld\n",
                //    field.type, field.num, v );
            }
            // Most of the record types in the FIT format aren't actually all
            // that useful.  FileId, Lap, and Record clearly are.  The one
            // other one that might be useful is DeviceInfo, but it doesn't
            // seem to be filled in properly.  Sean's Cinqo, for example,
            // shows up as manufacturer #65535, even though it should be #7.
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
                case 79: /* unknown */
                    break;
                default:
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
        int header_size = read_uint8();
        if (header_size != 12 && header_size != 14) {
            errors << QString("bad header size: %1").arg(header_size);
            delete rideFile;
            return NULL;
        }
        int protocol_version = read_uint8();
        (void) protocol_version;

        // if the header size is 14 we have profile minor then profile major
        // version. We still don't do anything with this information
        int profile_version = read_uint16(false); // always littleEndian
        (void) profile_version; // not sure what to do with this

        int data_size = read_uint32(false); // always littleEndian
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

        // read the rest of the header
        if (header_size == 14) read_uint16(false);

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
        catch (BadDelta &e) {
            errors << "Unsupported smart recording interval found";
            delete rideFile;
            return NULL;
        }
        if (stop) {
            delete rideFile;
            return NULL;
        }
        else {
            int crc = read_uint16( false ); // always littleEndian
            (void) crc;
            foreach(int num, unknown_global_msg_nums)
                qDebug() << QString("FitRideFile: unknown global message number %1; ignoring it").arg(num);
            foreach(int num, unknown_record_fields)
                qDebug() << QString("FitRideFile: unknown record field %1; ignoring it").arg(num);
            foreach(int num, unknown_base_type)
                qDebug() << QString("FitRideFile: unknown base type %1; skipped").arg(num);

            return rideFile;
        }
    }
};

RideFile *FitFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    QSharedPointer<FitFileReaderState> state(new FitFileReaderState(file, errors));
    return state->run();
}

// vi:expandtab tabstop=4 shiftwidth=4
