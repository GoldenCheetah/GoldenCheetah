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
#include "Settings.h"
#include "Units.h"
#include <QSharedPointer>
#include <QMap>
#include <QSet>
#include <QtEndian>
#include <QDebug>
#include <QTime>
#include <cstdio>
#include <stdint.h>
#include <time.h>
#include <limits>
#include <cmath>

#define FIT_DEBUG     false // debug traces
#define LAPSWIM_DEBUG false

#ifndef MATHCONST_PI
#define MATHCONST_PI 		    3.141592653589793238462643383279502884L /* pi */
#endif

#define LAP_TYPE     19
#define RECORD_TYPE  20
#define SEGMENT_TYPE 142

static int fitFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "fit", "Garmin FIT", new FitFileReader());

static const QDateTime qbase_time(QDate(1989, 12, 31), QTime(0, 0, 0), Qt::UTC);

static double bearing = 0; // used to compute headwind depending on wind/cyclist bearing difference

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

// this will need to change if float or other non-integer values are 
// introduced into the file format
typedef qint64 fit_value_t;
#define NA_VALUE std::numeric_limits<fit_value_t>::max()
typedef std::string fit_string_value;

enum fitValueType { SingleValue, DoubleValue, StringValue };
typedef enum fitValueType FitValueType;

struct FitValue
{
    FitValueType type;
    fit_value_t v;
    fit_value_t v2;
    fit_string_value s;
};

struct FitFileReaderState
{
    QFile &file;
    QStringList &errors;
    RideFile *rideFile;
    time_t start_time;
    time_t last_time;
    double last_distance;
    QMap<int, FitDefinition> local_msg_types;
    QSet<int> unknown_record_fields, unknown_global_msg_nums, unknown_base_type;
    int interval;
    int calibration;
    int devices;
    bool stopped;
    bool isLapSwim;
    double pool_length;
    int last_event_type;
    int last_event;
    int last_msg_type;
    QVariant isGarminSmartRecording;
    QVariant GarminHWM;

    FitFileReaderState(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), start_time(0),
        last_time(0), last_distance(0.00f), interval(0), calibration(0),
        devices(0), stopped(true), isLapSwim(false), pool_length(0.0),
        last_event_type(-1), last_event(-1), last_msg_type(-1)
    {}

    struct TruncatedRead {};

    void read_unknown( int size, int *count = NULL ) {
        if (!file.seek(file.pos() + size))
            throw TruncatedRead();
        if (count)
            (*count) += size;
    }

    fit_string_value read_text(int len, int *count = NULL) {
        char c;
        fit_string_value res = "";
        for (int i = 0; i < len; ++i) {
            if (file.read(&c, 1) != 1)
                throw TruncatedRead();
            if (count)
                *count += 1;

            if (c != 0)
                res += c;
        }
        return res;
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

    void DumpFitValue(const FitValue& v) {
        printf("type: %d %llx %llx %s\n", v.type, v.v, v.v2, v.s.c_str());
    }    

    void decodeFileId(const FitDefinition &def, int,
                      const std::vector<FitValue>& values) {
        int i = 0;
        int manu = -1, prod = -1;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 1: manu = value; break;
                case 2: prod = value; break;

                // other are ignored at present:
                case 0: // file type:
                    // 4:  activity log
                    // 6:  Itinary
                    // 34: segment
                    break;
                case 3: //serial number
                case 4: //timestamp
                case 5: //number
                default: ; // do nothing
            }
        }
        if (manu == 1) {
            // Garmin
            // Product IDs can be found in c/fit_example.h in the FIT SDK.
            // Multiple product IDs refer to different regions e.g. China, Japan etc.
            switch (prod) {
                case 473: case 474: case 475: case 494: rideFile->setDeviceType("Garmin FR301"); break;
                case 717: case 987: rideFile->setDeviceType("Garmin FR405"); break;
                case 782: rideFile->setDeviceType("Garmin FR50"); break;
                case 988: rideFile->setDeviceType("Garmin FR60"); break;
                case 1018: rideFile->setDeviceType("Garmin FR310XT"); break;
                case 1036: case 1199: case 1213: case 1387: rideFile->setDeviceType("Garmin Edge 500"); break;
                case 1124: case 1274: rideFile->setDeviceType("Garmin FR110"); break;
                case 1169: case 1333: case 1334: case 1386: rideFile->setDeviceType("Garmin Edge 800"); break;
                case 1325: rideFile->setDeviceType("Garmin Edge 200"); break;
                case 1328: rideFile->setDeviceType("Garmin FR910XT"); break;
                case 1345: case 1410: rideFile->setDeviceType("Garmin FR610"); break;
                case 1360: rideFile->setDeviceType("Garmin FR210"); break;
                case 1436: rideFile->setDeviceType("Garmin FR70"); break;
                case 1446: rideFile->setDeviceType("Garmin FR310XT 4T"); break;
                case 1482: case 1688: rideFile->setDeviceType("Garmin FR10"); break;
                case 1499: rideFile->setDeviceType("Garmin Swim"); break;
                case 1551: rideFile->setDeviceType("Garmin Fenix"); break;
                case 1561: case 1742: case 1821: rideFile->setDeviceType("Garmin Edge 510"); break;
                case 1567: rideFile->setDeviceType("Garmin Edge 810"); break;
                case 1623: rideFile->setDeviceType("Garmin FR620"); break;
                case 1632: rideFile->setDeviceType("Garmin FR220"); break;
                case 1765: case 2130: case 2131: case 2132: rideFile->setDeviceType("Garmin FR920XT"); break;
                case 1836: case 2052: case 2053: case 2070: case 2100: rideFile->setDeviceType("Garmin Edge 1000"); break;
                case 1903: rideFile->setDeviceType("Garmin FR15"); break;
                case 1967: rideFile->setDeviceType("Garmin Fenix2"); break;
                case 2050: case 2188: case 2189: rideFile->setDeviceType("Garmin Fenix3"); break;
                case 2067: case 2260: rideFile->setDeviceType("Garmin Edge 520"); break;
                case 2147: rideFile->setDeviceType("Garmin Edge 25"); break;
                case 2153: rideFile->setDeviceType("Garmin FR225"); break;
                case 2238: rideFile->setDeviceType("Garmin Edge 20"); break;
                case 20119: rideFile->setDeviceType("Garmin Training Center"); break;
                case 65532: rideFile->setDeviceType("Android ANT+ Plugin"); break;
                case 65534: rideFile->setDeviceType("Garmin Connect Website"); break;
                default: rideFile->setDeviceType(QString("Garmin %1").arg(prod));
            }
        } else if (manu == 6 ) {
            // SRM
            // powercontrol now uses FIT files from PC8
            switch (prod) {

            case 6: rideFile->setDeviceType("SRM PC6");break;
            case 7: rideFile->setDeviceType("SRM PC7");break;
            case 8: rideFile->setDeviceType("SRM PC8");break;
            default: rideFile->setDeviceType("SRM Powercontrol");break;
            }
        } else if (manu == 9 ) {
            // Powertap
            switch (prod) {
                case 14: rideFile->setDeviceType("Joule 2.0");break;
                case 18: rideFile->setDeviceType("Joule");break;
                case 19: rideFile->setDeviceType("Joule GPS");break;
                case 22: rideFile->setDeviceType("Joule GPS+");break;

                default: rideFile->setDeviceType(QString("Powertap Device %1").arg(prod));break;
            }
        } else  if (manu == 38) {
            // o_synce
            switch (prod) {
                case 1: rideFile->setDeviceType("o_synce navi2coach"); break;
                default: rideFile->setDeviceType(QString("o_synce %1").arg(prod));
            }
        } else if (manu == 70) {
            // does not set product at this point
           rideFile->setDeviceType("Sigmasport ROX");

        } else if (manu == 76) {
            // Moxy
            rideFile->setDeviceType("Moxy Monitor");

        } else if (manu == 260) {
            // Zwift!
            rideFile->setDeviceType("Zwift");

        } else {
            rideFile->setDeviceType(QString("Unknown FIT Device %1:%2").arg(manu).arg(prod));
        }
        rideFile->setFileFormat("FIT (*.fit)");
    }

    void decodeSession(const FitDefinition &def, int,
                       const std::vector<FitValue>& values) {
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 5: // sport field
                    switch (value) {
                        case 1: // running:
                            rideFile->setTag("Sport","Run");
                            break;
                        default: // if we can't work it out, assume bike
                            // but only if not already set to another sport,
                            // Garmin Swim send 2 tags for example
                            if (rideFile->getTag("Sport", "Bike") != "Bike") break;
                        case 2: // cycling
                            rideFile->setTag("Sport","Bike");
                            break;
                        case 5: // swimming
                            rideFile->setTag("Sport","Swim");
                            break;
/*
                        // other sports are ignored at present:
                        case 0:    // generic
                        case 3:    // transition
                        case 4:    // fitness_equipment
                        case 6:    // basketball
                        case 7:    // soccer
                        case 8:    // tennis
                        case 9:    // american_football
                        case 10:    // training
                        case 11:    // walking
                        case 12:    // cross_country_skiing
                        case 13:    // alpine_skiing
                        case 14:    // snowboarding
                        case 15:    // rowing
                        case 16:    // mountaineering
                        case 17:    // hiking
                        case 18:    // multisport
                        case 19:    // paddling
                        case 254:    // all
*/
                    }
                    break;
                case 6: // sub sport (ignored at present)
                    switch (value) {
                        case 0:    // generic
                        case 1:    // treadmill
                        case 2:    // street
                        case 3:    // trail
                        case 4:    // track
                        case 5:    // spin
                        case 6:    // home trainer
                        case 7:    // route
                        case 8:    // mountain
                        case 9:    // downhill
                        case 10:    // recumbent
                        case 11:    // cyclocross
                        case 12:    // hand_cycling
                        case 13:    // piste
                        case 14:    // indoor_rowing
                        case 15:    // elliptical
                        case 254:    // all
                        default:
                            break;
                    }
                    break;
                case 44: // pool_length
                    pool_length = value / 100000.0;
                    if (LAPSWIM_DEBUG) qDebug() << "Pool length" << pool_length;
                    break;

                // other fields are ignored at present
                case 253: //timestamp
                case 254: //index
                case 0:   //event
                case 1:    /* event_type */
                case 2:    /* start_time */
                case 3:    /* start_position_lat */
                case 4:    /* start_position_long */
                case 7:    /* total elapsed time */
                case 8:    /* total timer time */
                case 9:    /* total distance */
                case 10:    /* total_cycles */
                case 11:    /* total calories */
                case 13:    /* total fat calories */
                case 14:    /* avg_speed */
                case 15:    /* max_speed */
                case 16:    /* avg_HR */
                case 17:    /* max_HR */
                case 18:    /* avg_cad */
                case 19:    /* max_cad */
                case 20:    /* avg_pwr */
                case 21:    /* max_pwr */
                case 22:    /* total ascent */
                case 23:    /* total descent */
                case 25:    /* first lap index */
                case 26:    /* num lap */
                case 29:    /* north-east lat = bounding box */
                case 30:    /* north-east lon = bounding box */
                case 31:    /* south west lat = bounding box */
                case 32:    /* south west lon = bounding box */
                case 34:    /* normalized power */
                case 48:    /* total work (J) */
                case 49:    /* avg altitude */
                case 50:    /* max altitude */
                case 52:    /* avg grade */
                case 53:    /* avg positive grade */
                case 54:    /* avg negative grade */
                case 55:    /* max pos grade */
                case 56:    /* max neg grade */
                case 57:    /* avg temperature (Celsius. deg) */
                case 58:    /* max temp */
                case 59:    /* total_moving_time */
                case 60:    /* avg_pos_vertical_speed (m/s) */
                case 61:    /* avg_neg_vertical_speed */
                case 62:    /* max_pos_vertical_speed */
                case 63:    /* max neg_vertical_speed */
                case 64:    /* min HR bpm */
                case 69:    /* avg lap time */
                case 70:    /* best lap index */
                case 71:    /* min altitude */
                case 92:    /* fractional avg cadence (rpm) */
                case 93:    /* fractional max cadence */
                default: ; // do nothing
            }

            if (FIT_DEBUG) {
                printf("decodeSession  field %d: %d bytes, num %d, type %d\n", i, field.size, field.num, field.type );
            }
        }
    }

    void decodeDeviceInfo(const FitDefinition &def, int,
                          const std::vector<FitValue>& values) {
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {

                // all fields are ignored at present
                case 253: //timestamp
                case 3:   // serial number
                case 2:   // manufacturer
                case 4:   // product
                case 5:   // software version
                case 10:  // battery voltage
                case 0:   // device index
                case 1:   // ANT+ device type
                          // details: 0x78 = HRM, 0x79 = Spd&Cad, 0x7A = Cad, 0x7B = Speed
                case 6:   // hardware version
                case 11:  // battery status
                case 22:  // ANT network
                case 25:  // source type
                case 24:  // equipment ID
                default: ; // do nothing
            }

            if (FIT_DEBUG) {
                printf("decodeDeviceInfo  field %d: %d bytes, num %d, type %d\n", i, field.size, field.num, field.type );
            }
        }
    }

    void decodeEvent(const FitDefinition &def, int,
                     const std::vector<FitValue>& values) {
        int time = -1;
        int event = -1;
        int event_type = -1;
        qint16 data16 = -1;
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: // timestamp field (s)
                    time = value + qbase_time.toTime_t();
                          break;
                case 0: // event field
                    event = value; break;
                case 1: // event_type field
                    event_type = value; break;
                case 2: // data16 field
                    data16 = value; break;

                // additional values (ignored at present):
                case 3: //data
                case 4: // event group
                default: ; // do nothing
            }
        }

        switch (event) {
            case 0: // Timer event
                {
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
                            errors << QString("Unknown timer event type %1").arg(event_type);
                    }
                }
                break;

            case 36: // Calibration event
                {
                    int secs = (start_time==0?0:time-start_time);
                    switch (event_type) {
                        case 3: // marker
                            ++calibration;
                            rideFile->addCalibration(secs, data16, QString("Calibration %1 (%2)").arg(calibration).arg(data16));
                            //qDebug() << "marker" << secs << data16;
                            break;
                        default:
                            errors << QString("Unknown calibration event type %1").arg(event_type);
                            break;
                    }
                }
                break;

            case 3: /* workout */
            case 4: /* workout_step */
            case 5: /* power_down */
            case 6: /* power_up */
            case 7: /* off_course */
            case 8: /* session */
            case 9: /* lap */
            case 10: /* course_point */
            case 11: /* battery */
            case 12: /* virtual_partner_pace */
            case 13: /* hr_high_alert */
            case 14: /* hr_low_alert */
            case 15: /* speed_high_alert */
            case 16: /* speed_low_alert */
            case 17: /* cad_high_alert */
            case 18: /* cad_low_alert */
            case 19: /* power_high_alert */
            case 20: /* power_low_alert */
            case 21: /* recovery_hr */
            case 22: /* battery_low */
            case 23: /* time_duration_alert */
            case 24: /* distance_duration_alert */
            case 25: /* calorie_duration_alert */
            case 26: /* activity */
            case 27: /* fitness_equipment */
            case 28: /* length */
            case 32: /* user_marker */
            case 33: /* sport_point */
            case 42: /* front_gear_change */
            case 43: /* rear_gear_change */

            default: ;
        }

        if (FIT_DEBUG) {
            printf("event type %d\n", event_type);
        }
        last_event = event;
        last_event_type = event_type;
    }

    void decodeLap(const FitDefinition &def, int time_offset,
                   const std::vector<FitValue>& values) {
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        else
            time = last_time;
        int i = 0;
        time_t this_start_time = 0;
        ++interval;
        double total_elapsed_time = 0.0;
        double total_distance = 0.0;
        if (FIT_DEBUG)  {
            printf( " FIT decode lap \n");
        }

        foreach(const FitField &field, def.fields) {
            const FitValue& value = values[i++];

            if( value.v == NA_VALUE )
                continue;

            if (FIT_DEBUG) {
                printf ("\tfield: num: %d ", field.num);
                DumpFitValue(value);
            }

            switch (field.num) {
                case 253:
                    time = value.v + qbase_time.toTime_t();
                    break;
                case 2:
                    this_start_time = value.v + qbase_time.toTime_t();
                    break;
                case 7:
                    total_elapsed_time = round(value.v / 1000.0);
                    break;
                case 9:
                    total_distance = value.v / 100000.0;
                    break;

                // other data (ignored at present):
                case 254: // lap nbr
                case 3: // start_position_lat
                case 4: // start_position_lon
                case 5: // end_position_lat
                case 6: // end_position_lon
                case 8: // total_timer_time
                case 10: // total_cycles
                case 11: // total calories
                case 12: // total fat calories
                case 13: // avg_speed
                case 14: // max_speed
                case 15: // avg HR (bpm)
                case 16: // Max HR
                case 17: // AvCad
                case 18: // MaxCad
                case 21: // total ascent
                case 22: // total descent
                case 27: // north-east lat (bounding box)
                case 28: // north-east lon
                case 29: // south west lat
                case 30: // south west lon
                    break;
                default: ; // ignore it
            }
        }
        if (LAPSWIM_DEBUG) qDebug() << "Lap" << interval << this_start_time - start_time << total_elapsed_time
                                    << time - this_start_time << total_distance;
        if (this_start_time == 0 || this_start_time-start_time < 0) {
            //errors << QString("lap %1 has invalid start time").arg(interval);
            this_start_time = start_time; // time was corrected after lap start

            if (time == 0 || time-start_time < 0) {
                errors << QString("lap %1 is ignored (invalid end time)").arg(interval);
                return;
            }
        }
        if (rideFile->dataPoints().count()) { // no samples means no laps..
            if (isLapSwim && total_elapsed_time > 0.0) {
                rideFile->addInterval(RideFileInterval::DEVICE, this_start_time - start_time,
                                      this_start_time - start_time + total_elapsed_time,
                                      QObject::tr("Lap %1").arg(interval));
            } else {
                rideFile->addInterval(RideFileInterval::DEVICE, this_start_time - start_time, time - start_time,
                                      QObject::tr("Lap %1").arg(interval));
            }
        }
    }

    void decodeRecord(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        if (isLapSwim) return; // We use the length message for Lap Swimming
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        double alt = 0, cad = 0, km = 0, hr = 0, lat = 0, lng = 0, badgps = 0, lrbalance = RideFile::NA;
        double kph = 0, temperature = RideFile::NA, watts = 0, slope = 0;
        double leftTorqueEff = 0, rightTorqueEff = 0, leftPedalSmooth = 0, rightPedalSmooth = 0;

        double leftPedalCenterOffset = 0;
        double rightPedalCenterOffset = 0;
        double leftTopDeathCenter = 0;
        double rightTopDeathCenter = 0;
        double leftBottomDeathCenter = 0;
        double rightBottomDeathCenter = 0;
        double leftTopPeakPowerPhase = 0;
        double rightTopPeakPowerPhase = 0;
        double leftBottomPeakPowerPhase = 0;
        double rightBottomPeakPowerPhase = 0;

        double rvert = 0, rcad = 0, rcontact = 0;
        double smO2 = 0, tHb = 0;
        //bool run=false;

        fit_value_t lati = NA_VALUE, lngi = NA_VALUE;
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i].v;
            fit_value_t value2 = values[i++].v2;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: // TIMESTAMP
                          time = value + qbase_time.toTime_t();
                          // Time MUST NOT go backwards
                          // You canny break the laws of physics, Jim
                          if (time < last_time)
                              time = last_time;
                          break;
                case 0: // POSITION_LAT
                        lati = value;
                        break;
                case 1: // POSITION_LONG
                        lngi = value;
                        break;
                case 2: // ALTITUDE
                        alt = value / 5.0 - 500.0;
                        break;
                case 3: // HEART_RATE
                        hr = value;
                        break;
                case 4: // CADENCE
                        if (rideFile->getTag("Sport", "Bike") == "Run")
                            rcad = value;
                        else
                            cad = value;
                        break;

                case 5: // DISTANCE
                        km = value / 100000.0;
                        break;
                case 6: // SPEED
                        kph = value * 3.6 / 1000.0;
                        break;
                case 7: // POWER
                        watts = value;
                        break;
                case 8: break; // packed speed/dist
                case 9: // GRADE
                        slope = value / 100.0;
                        break;
                case 10: //resistance = value;
                         break;
                case 11: //time_from_course = value / 1000.0;
                         break;
                case 12: break; // "cycle_length"
                case 13: // TEMPERATURE
                         temperature = value;
                         break;
                case 29: // ACCUMULATED_POWER
                         break;
                case 30: //LEFT_RIGHT_BALANCE
                         lrbalance = (value & 0x80 ? 100 - (value & 0x7F) : value & 0x7F);
                         break;
                case 31: // GPS Accuracy
                         break;

                case 39: // VERTICAL OSCILLATION
                         rvert = value / 100.0f;
                         break;

                //case 40: // ACTIVITY_TYPE
                //         // TODO We should know/test value for run
                //         run = true;
                //         break;

                case 41: // GROUND CONTACT TIME
                         rcontact = value / 10.0f;
                         break;

                case 43: // LEFT_TORQUE_EFFECTIVENESS
                         leftTorqueEff = value / 2.0;
                         break;
                case 44: // RIGHT_TORQUE_EFFECTIVENESS
                         rightTorqueEff = value / 2.0;
                         break;
                case 45: // LEFT_PEDAL_SMOOTHNESS
                         leftPedalSmooth = value / 2.0;
                         break;
                case 46: // RIGHT_PEDAL_SMOOTHNESS
                         rightPedalSmooth = value / 2.0;
                         break;
                case 47: // COMBINED_PEDAL_SMOOTHNES
                         //qDebug() << "COMBINED_PEDAL_SMOOTHNES" << value;
                         break;
                case 53: // RUNNING CADENCE FRACTIONAL VALUE
                         break;
                case 54: // tHb
                        tHb= value/100.0f;
                        break;
                case 57: // SMO2
                        smO2= value/10.0f;
                        break;
                case 61: // ? GPS Altitude ? or atmospheric pressure ?
                        break;
                case 66: // ??
                        break;
                case 67: // ? Left Platform Center Offset ?
                        leftPedalCenterOffset = value;
                        break;
                case 68: // ? Right Platform Center Offset ?
                        rightPedalCenterOffset = value;
                        break;
                case 69: // ? Left Power Phase ?
                        leftTopDeathCenter = round(value * 360.0/256);
                        leftBottomDeathCenter = round(value2 * 360.0/256);
                        break;
                case 70: // ? Left Peak Phase  ?
                        leftTopPeakPowerPhase = round(value * 360.0/256);
                        leftBottomPeakPowerPhase = round(value2 * 360.0/256);
                        break;
                case 71: // ? Right Power Phase ?
                        rightTopDeathCenter = round(value * 360.0/256);
                        rightBottomDeathCenter = round(value2 * 360.0/256);
                        break;
                case 72: // ? Right Peak Phase  ?
                        rightTopPeakPowerPhase = round(value * 360.0/256);
                        rightBottomPeakPowerPhase = round(value2 * 360.0/256);
                        break;


                default: 
                         unknown_record_fields.insert(field.num);
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
            start_time = time - 1; // recording interval?
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

        // compute bearing in order to calculate headwind
        if ((!rideFile->dataPoints().empty()) && (last_time != 0))
        {
            RideFilePoint *prevPoint = rideFile->dataPoints().back();
            // ensure a movement occurred and valid lat/lon in order to compute cyclist direction
            if (  (prevPoint->lat != lat || prevPoint->lon != lng )
               && (prevPoint->lat != 0 || prevPoint->lon != 0 )
               && (lat != 0 || lng != 0 ) )
                        bearing = atan2(cos(lat)*sin(lng - prevPoint->lon),
                                        cos(prevPoint->lat)*sin(lat)-sin(prevPoint->lat)*cos(lat)*cos(lng - prevPoint->lon));
        }
        // else keep previous bearing or 0 at beginning

        double headwind = cos(bearing - rideFile->windHeading()) * rideFile->windSpeed() + kph;

        int interval = 0;
        // if there are data points && a time difference > 1sec && smartRecording processing is requested at all
        if ((!rideFile->dataPoints().empty()) && (last_time != 0) &&
             (time > last_time + 1) && (isGarminSmartRecording.toInt() != 0)) {
            // Handle smart recording if configured in preferences.  Linearly interpolate missing points.
            RideFilePoint *prevPoint = rideFile->dataPoints().back();
            double deltaSecs = (secs - prevPoint->secs);
            //assert(deltaSecs == secs - prevPoint->secs); // no fractional part -- don't CRASH FFS, be graceful
            // This is only true if the previous record was of type record:
            //assert(deltaSecs == time - last_time); -- don't CRASH FFS, be graceful
            // If the last lat/lng was missing (0/0) then all points up to lat/lng are marked as 0/0.
            if (prevPoint->lat == 0 && prevPoint->lon == 0 ) {
                badgps = 1;
            }
            double deltaCad = cad - prevPoint->cad;
            double deltaHr = hr - prevPoint->hr;
            double deltaDist = km - prevPoint->km;
            if (km < 0.00001) deltaDist = 0.000f; // effectively zero distance
            double deltaSpeed = kph - prevPoint->kph;
            double deltaTorque = nm - prevPoint->nm;
            double deltaPower = watts - prevPoint->watts;
            double deltaAlt = alt - prevPoint->alt;
            double deltaLon = lng - prevPoint->lon;
            double deltaLat = lat - prevPoint->lat;
            double deltaHeadwind = headwind - prevPoint->headwind;
            double deltaSlope = slope - prevPoint->slope;
            double deltaLeftRightBalance = lrbalance - prevPoint->lrbalance;
            double deltaLeftTE = leftTorqueEff - prevPoint->lte;
            double deltaRightTE = rightTorqueEff - prevPoint->rte;
            double deltaLeftPS = leftPedalSmooth - prevPoint->lps;
            double deltaRightPS = rightPedalSmooth - prevPoint->rps;
            double deltaLeftPedalCenterOffset = leftPedalCenterOffset - prevPoint->lpco;
            double deltaRightPedalCenterOffset = rightPedalCenterOffset - prevPoint->rpco;
            double deltaLeftTopDeathCenter = leftTopDeathCenter - prevPoint->lppb;
            double deltaRightTopDeathCenter = rightTopDeathCenter - prevPoint->rppb;
            double deltaLeftBottomDeathCenter = leftBottomDeathCenter - prevPoint->lppe;
            double deltaRightBottomDeathCenter = rightBottomDeathCenter - prevPoint->rppe;
            double deltaLeftTopPeakPowerPhase = leftTopPeakPowerPhase - prevPoint->lpppb;
            double deltaRightTopPeakPowerPhase = rightTopPeakPowerPhase - prevPoint->rpppb;
            double deltaLeftBottomPeakPowerPhase = leftBottomPeakPowerPhase - prevPoint->lpppe;
            double deltaRightBottomPeakPowerPhase = rightBottomPeakPowerPhase - prevPoint->rpppe;
            double deltaSmO2 = smO2 - prevPoint->smo2;
            double deltaTHb = tHb - prevPoint->thb;
            double deltarvert = rvert - prevPoint->rvert;
            double deltarcad = rcad - prevPoint->rcad;
            double deltarcontact = rcontact - prevPoint->rcontact;

            // only smooth the maximal smart recording gap defined in preferences - we don't want to crash / stall on bad
            // or corrupt files
            if (deltaSecs > 0 && deltaSecs < GarminHWM.toInt()) {

                for (int i = 1; i < deltaSecs; i++) {
                    double weight = i /deltaSecs;
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
                        prevPoint->slope + (deltaSlope * weight),
                        temperature,
                        prevPoint->lrbalance + (deltaLeftRightBalance * weight),
                        prevPoint->lte + (deltaLeftTE * weight),
                        prevPoint->rte + (deltaRightTE * weight),
                        prevPoint->lps + (deltaLeftPS * weight),
                        prevPoint->rps + (deltaRightPS * weight),
                        prevPoint->lpco + (deltaLeftPedalCenterOffset * weight),
                        prevPoint->rpco + (deltaRightPedalCenterOffset * weight),
                        prevPoint->lppb + (deltaLeftTopDeathCenter * weight),
                        prevPoint->rppb + (deltaRightTopDeathCenter * weight),
                        prevPoint->lppe + (deltaLeftBottomDeathCenter * weight),
                        prevPoint->rppe + (deltaRightBottomDeathCenter * weight),
                        prevPoint->lpppb + (deltaLeftTopPeakPowerPhase * weight),
                        prevPoint->rpppb + (deltaRightTopPeakPowerPhase * weight),
                        prevPoint->lpppe + (deltaLeftBottomPeakPowerPhase * weight),
                        prevPoint->rpppe + (deltaRightBottomPeakPowerPhase * weight),
                        prevPoint->smo2 + (deltaSmO2 * weight),
                        prevPoint->thb + (deltaTHb * weight),
                        prevPoint->rvert + (deltarvert * weight),
                        prevPoint->rcad + (deltarcad * weight),
                        prevPoint->rcontact + (deltarcontact * weight),
                        0.0, // tcore
                        interval);
                }
            }
        }

        if (km < 0.00001f) km = last_distance;
        rideFile->appendPoint(secs, cad, hr, km, kph, nm, watts, alt, lng, lat, headwind, slope, temperature,
                     lrbalance, leftTorqueEff, rightTorqueEff, leftPedalSmooth, rightPedalSmooth,
                     leftPedalCenterOffset, rightPedalCenterOffset,
                     leftTopDeathCenter, rightTopDeathCenter, leftBottomDeathCenter, rightBottomDeathCenter,
                     leftTopPeakPowerPhase, rightTopPeakPowerPhase, leftBottomPeakPowerPhase, rightBottomPeakPowerPhase,
                     smO2, tHb, rvert, rcad, rcontact, 0.0, interval);
        last_time = time;
        last_distance = km;
    }

    void decodeLength(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        if (!isLapSwim) {
            isLapSwim = true;
            // reset rideFile if not empty
            if (!rideFile->dataPoints().empty()) {
                start_time = 0;
                last_time = 0;
                last_distance = 0.00f;
                interval = 0;
                QString deviceType = rideFile->deviceType();
                delete rideFile;
                rideFile = new RideFile;
                rideFile->setDeviceType(deviceType);
                rideFile->setRecIntSecs(1.0);
             }
        }
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        double cad = 0, km = 0, kph = 0;

        bool length_type = false;
        double length_duration = 0.0;

        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 0: // event
                        if (FIT_DEBUG) qDebug() << " event:" << value;
                        break;
                case 1: // event type
                        if (FIT_DEBUG) qDebug() << " event_type:" << value;
                        break;
                case 2: // start time
                        time = value + qbase_time.toTime_t();
                        // Time MUST NOT go backwards
                        // You canny break the laws of physics, Jim
                        if (time < last_time)
                            time = last_time;
                        break;
                case 3: // total elapsed time
                        length_duration = value / 1000;
                        break;
                case 4: // total timer time
                        if (FIT_DEBUG) qDebug() << " total_timer_time:" << value;
                        break;
                case 5: // total strokes
                        if (FIT_DEBUG) qDebug() << " total_strokes:" << value;
                        break;
                case 6: // avg speed
                        kph = value * 3.6 / 1000.0;
                        break;
                case 7: // swim stroke
                        if (FIT_DEBUG) qDebug() << " swim_stroke:" << value;
                        break;
                case 9: // cadence
                        cad = value;
                        break;
                case 11: // total_calories
                        if (FIT_DEBUG) qDebug() << " total_calories:" << value;
                        break;
                case 12: // length type: 0-rest, 1-strokes
                        length_type = value;
                        break;
                case 254: // message_index
                        if (FIT_DEBUG) qDebug() << " message_index:" << value;
                        break;
                default:
                         unknown_record_fields.insert(field.num);
            }
        }

        // Rest interval
        if (!length_type) {
            kph = 0.0;
            cad = 0.0;
        }
        if (time == last_time)
            return; // Sketchy, but some FIT files do this.
        if (start_time == 0) {
            start_time = time - 1; // recording interval?
            QDateTime t;
            t.setTime_t(start_time);
            rideFile->setStartTime(t);
        }

        double secs = time - start_time;

        // Normalize distance for the most common pool lengths,
        // this is a hack to avoid the need for a double pass when
        // pool_length comes in Session message at the end of the file.
        if (pool_length == 0.0) {
            pool_length = kph*length_duration/3600;
            if (fabs(pool_length - 0.050) < 0.004) pool_length = 0.050;
            else if (fabs(pool_length - 0.033) < 0.003) pool_length = 0.033;
            else if (fabs(pool_length - 0.025) < 0.002) pool_length = 0.025;
            else if (fabs(pool_length - 0.025*METERS_PER_YARD) < 0.002) pool_length = 0.025*METERS_PER_YARD;
            else if (fabs(pool_length - 0.020) < 0.002) pool_length = 0.020;
        }

        // another pool length or pause
        km = last_distance + (length_type ? pool_length : 0.0);

        if ((secs > last_time + 1) && (isGarminSmartRecording.toInt() != 0) && (secs - last_time < 10*GarminHWM.toInt())) {
            double deltaSecs = secs - last_time;
            if (LAPSWIM_DEBUG) qDebug() << "Pause" << last_time+1 << deltaSecs;
            for (int i = 1; i <= deltaSecs; i++) {
                rideFile->appendPoint(
                    last_time+i, 0.0, 0.0,
                    last_distance,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, RideFile::NA, RideFile::NA,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0);
            }
            last_time += deltaSecs;
        }

        // only fill 10x the maximal smart recording gap defined
        // in preferences - we don't want to crash / stall on bad
        // or corrupt files
        if ((isGarminSmartRecording.toInt() != 0) && length_duration > 0 && length_duration < 10*GarminHWM.toInt()) {
            double deltaSecs = length_duration;
            double deltaDist = km - last_distance;
            kph = 3600.0 * deltaDist / deltaSecs;
            if (LAPSWIM_DEBUG) qDebug() << "Length" << last_time+1 << deltaSecs << deltaDist << "type" << length_type;
            for (int i = 1; i <= deltaSecs; i++) {
                rideFile->appendPoint(
                    last_time + i, cad, 0.0,
                    last_distance + (deltaDist * i/deltaSecs),
                    kph, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0,
                    RideFile::NA,RideFile::NA,
                    0.0, 0.0,0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,0.0, 0.0,
                    0.0, 0.0,0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0,
                    0);
            }
            last_time += deltaSecs;
            last_distance += deltaDist;
        }
    }

    /* weather broadcast as observed at weather station (undocumented) */
    void decodeWeather(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: // Timestamp
                          // ignored
                          break;
                case 8:   // Weather station name
                          // ignored
                        break;
                case 9:   // Weather observation timestamp
                          // ignored
                        break;
                case 10: // Weather station latitude
                         // ignored
                        break;
                case 11: // Weather station longitude
                         // ignored
                        break;
                case 3:  // Wind heading (0deg=North)
                        rideFile->setWindHeading(value / 180.0 * MATHCONST_PI);
                        break;
                case 4:  // Wind speed (mm/s)
                        rideFile->setWindSpeed(value * 0.0036);
                        break;
                case 1:  // Temperature
                         // ignored
                        break;
                case 7:  // Humidity
                         // ignored at present
                        break;
                default: ; // ignore it
            }
        }
    }

    void decodeDeviceSettings(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 0:  // Active timezone
                         // ignored
                        break;
                case 1:  // UTC offset
                         // ignored
                        break;
                case 5:  // timezone offset
                         // ignored
                        break;
                default: ; // ignore it
            }
        }
    }

    void decodeSegment(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        else
            time = last_time;

        int i = 0;
        time_t this_start_time = 0;
        ++interval;
        double total_elapsed_time = 0.0;
        double total_distance = 0.0;

        QString segment_name;
        foreach(const FitField &field, def.fields) {
            const FitValue& value = values[i++];

            if( value.type != StringValue && value.v == NA_VALUE )
                continue;

            if (FIT_DEBUG) {
                printf ("\tfield: num: %d ", field.num);
                DumpFitValue(value);
            }

            switch (field.num) {
                case 253: // Message timestamp
                    time = value.v + qbase_time.toTime_t();
                    break;
                case 2:   // start timestamp ?
                    this_start_time = value.v + qbase_time.toTime_t();
                    break;
                case 3:  // start latitude
                         // ignored
                        break;
                case 4:  // start longitude
                         // ignored
                        break;
                case 5:  // end latitude
                         // ignored
                        break;
                case 6:  // end longitude
                         // ignored
                        break;
                case 7:  // personal best (ms) ? segment elapsed time from this activity (ms) ?
                         // => depends on file / device / version ?
                         // FIXME: to be investigated/confirmed.
                    total_elapsed_time = round(value.v / 1000.0);
                    break;
                case 8:  // challenger best (ms) ? segment total timer time from this activity (ms) ?
                         // => depends on file / device / version ?
                         // FIXME: to be investigated/confirmed.
                         // ignored
                        break;
                case 9:  // leader best (ms) ? segment distance ? FIXME : to be investigated.
                         // => depends on file / device / version ?
                    total_distance = value.v / 100000.0;
                    break;
                case 10: // personal rank ? to be confirmed
                         // ignored
                        break;
                case 25:  // north-east latitude (bounding box)
                         // ignored
                        break;
                case 26:  // north-east longitude
                         // ignored
                        break;
                case 27:  // south-west latitude
                         // ignored
                        break;
                case 28:  // south-west longitude
                         // ignored
                        break;
                case 29:  // Segment name
                    segment_name = QString(value.s.c_str());
                    if (FIT_DEBUG)  {
                        printf("Found segment name: %s\n", segment_name.toStdString().c_str());
                    }
                    break;
                case 33:  /* undocumented, ignored */  break;
                case 71:  /* undocumented, ignored */  break;
                case 75:  /* undocumented, ignored */  break;
                case 76:  /* undocumented, ignored */  break;
                case 77:  /* undocumented, ignored */  break;
                case 78:  /* undocumented, ignored */  break;
                case 79:  /* undocumented, ignored */  break;
                case 80:  /* undocumented, ignored */  break;
                case 254:  /* message counter idx, ignored */  break;
                case 11:  /* undocumented, ignored */  break;
                case 12:  /* undocumented, ignored */  break;
                case 13:  /* undocumented, ignored */  break;
                case 14:  /* undocumented, ignored */  break;
                case 19:  /* undocumented, ignored */  break;
                case 20:  /* undocumented, ignored */  break;
                case 21:  /* total ascent ? ignored */  break;
                case 22:  /* total descent ? ignored */  break;
                case 30:  /* undocumented, ignored */  break;
                case 31:  /* undocumented, ignored */  break;
                case 69:  /* undocumented, ignored */  break;
                case 70:  /* undocumented, ignored */  break;
                case 72:  /* undocumented, ignored */  break;
                case 0:  /* undocumented, ignored */  break;
                case 1:  /* undocumented, ignored */  break;
                case 15:  /* undocumented (HR?), ignored */  break;
                case 16:  /* undocumented (HR?), ignored */  break;
                case 17:  /* undocumented (cadence?), ignored */  break;
                case 18:  /* undocumented (cadence?), ignored */  break;
                case 23:  /* undocumented, ignored */  break;
                case 24:  /* undocumented, ignored */  break;
                case 32:  /* undocumented, ignored */  break;
                case 58:  /* undocumented, ignored */  break;
                case 59:  /* undocumented, ignored */  break;
                case 60:  /* undocumented, ignored */  break;
                case 61:  /* undocumented, ignored */  break;
                case 62:  /* undocumented, ignored */  break;
                case 63:  /* undocumented, ignored */  break;
                case 64:  /* undocumented, ignored */  break;
                case 65:  // Segment UID
                         // ignored
                        break;
                case 66:  /* undocumented, ignored */  break;
                case 67:  /* undocumented, ignored */  break;
                case 68:  /* undocumented, ignored */  break;
                case 73:  /* undocumented, ignored */  break;
                case 74:  /* undocumented, ignored */  break;
                case 81:  /* undocumented, ignored */  break;
                case 82:  /* undocumented, ignored */  break;
                default: ; // ignore it
            }
        }

        if (LAPSWIM_DEBUG) qDebug() << "Lap" << interval << this_start_time - start_time << total_elapsed_time
                                    << time - this_start_time << total_distance;
        if (this_start_time == 0 || this_start_time-start_time < 0) {
            //errors << QString("lap %1 has invalid start time").arg(interval);
            this_start_time = start_time; // time was corrected after lap start

            if (time == 0 || time-start_time < 0) {
                errors << QString("lap %1 is ignored (invalid end time)").arg(interval);
                return;
            }
        }
        if (rideFile->dataPoints().count()) { // no samples means no laps..
            if (segment_name == "") {
                segment_name = QObject::tr("Lap %1").arg(interval);
            }
            if (isLapSwim && total_elapsed_time > 0.0) {
                rideFile->addInterval(RideFileInterval::DEVICE, this_start_time - start_time,
                                      this_start_time - start_time + total_elapsed_time, segment_name);
            } else {
                rideFile->addInterval(RideFileInterval::DEVICE, this_start_time - start_time, time - start_time,
                                      segment_name);
            }
        }

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

            if (FIT_DEBUG)  {
                printf("definition: local type=%d global=%d arch=%d fields=%d\n",
                       local_msg_type, def.global_msg_num, def.is_big_endian,
                       num_fields );
            }

            for (int i = 0; i < num_fields; ++i) {
                def.fields.push_back(FitField());
                FitField &field = def.fields.back();

                field.num = read_uint8(&count);
                field.size = read_uint8(&count);
                int base_type = read_uint8(&count);
                field.type = base_type & 0x1f;

                if (FIT_DEBUG)  {
                    printf("  field %d: %d bytes, num %d, type %d, size %d\n",
                           i, field.size, field.num, field.type, field.size );
                }
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

            if (FIT_DEBUG)  {
                printf( "read_record message local=%d global=%d\n", local_msg_type,
                    def.global_msg_num );
            }

            std::vector<FitValue> values;
            foreach(const FitField &field, def.fields) {
                FitValue value;
                int size;

                switch (field.type) {
                    case 0: value.type = SingleValue; value.v = read_uint8(&count); size = 1; break;
                    case 1: value.type = SingleValue; value.v = read_int8(&count); size = 1;  break;
                    case 2: value.type = SingleValue; value.v = read_uint8(&count); size = 1;
                        // Multi-values ?
                        if (field.size>size) {
                            value.type = DoubleValue;
                            value.v2 = read_uint8(&count);
                            size = 2;
                        }
                        break;
                    case 3: value.type = SingleValue; value.v = read_int16(def.is_big_endian, &count); size = 2;  break;
                    case 4: value.type = SingleValue; value.v = read_uint16(def.is_big_endian, &count); size = 2;
                        // Multi-values ?
                        if (field.size>size) {
                            value.type = DoubleValue;
                            value.v2 = read_uint16(def.is_big_endian, &count);
                            size = 4;
                        }
                        break;
                    case 5: value.type = SingleValue; value.v = read_int32(def.is_big_endian, &count); size = 4;  break;
                    case 6: value.type = SingleValue; value.v = read_uint32(def.is_big_endian, &count); size = 4;  break;
                    case 7:
                        value.type = StringValue;
                        value.s = read_text(field.size, &count);
                        size = field.size;
                        break;

                    //case 8: // FLOAT32
                    //case 9: // FLOAT64
                    case 10: value.type = SingleValue; value.v = read_uint8z(&count); size = 1; break;
                    case 11: value.type = SingleValue; value.v = read_uint16z(def.is_big_endian, &count); size = 2; break;
                    case 12: value.type = SingleValue; value.v = read_uint32z(def.is_big_endian, &count); size = 4; break;
                    //case 13: // BYTE

                    // we may need to add support for float, string + byte base types here
                    default:
                        if (FIT_DEBUG)  {
                            // TODO: Dump raw data.
                            printf("unknown type: %d size: %d \n", field.type,
                                   field.size);
                                  
                        }
                        read_unknown( field.size, &count );
                        value.type = SingleValue;
                        value.v = NA_VALUE;
                        unknown_base_type.insert(field.num);
                        size = field.size;
                }
                // Quick fix : we need to support multivalues
                if (size < field.size) {
                    if (FIT_DEBUG)  {
                         printf( "   warning : size=%d for size=%d (num=%d)\n",
                                 field.size, field.type, field.num);
                    }
                    read_unknown( field.size-size, &count );
                }

                values.push_back(value);

                if (FIT_DEBUG)  {
                    printf( " field: type=%d num=%d ",
                        field.type, field.num);
                    if (value.type == SingleValue)
                        printf( "value=%lld\n", value.v );
                    else if (value.type == DoubleValue)
                        printf( "value=%lld value2=%lld\n", value.v, value.v2 );
                    else if (value.type == StringValue)
                        printf( "value=%s\n", value.s.c_str() );
                }
            }
            // Most of the record types in the FIT format aren't actually all
            // that useful.  FileId, Lap, and Record clearly are.  The one
            // other one that might be useful is DeviceInfo, but it doesn't
            // seem to be filled in properly.  Sean's Cinqo, for example,
            // shows up as manufacturer #65535, even though it should be #7.
            switch (def.global_msg_num) {
                case 0:  decodeFileId(def, time_offset, values); break;
                case 18: decodeSession(def, time_offset, values); break; /* session */
                case LAP_TYPE: // #19
                    decodeLap(def, time_offset, values);
                    break;
                case RECORD_TYPE: // #20
                    decodeRecord(def, time_offset, values);
                    break;
                case 21: decodeEvent(def, time_offset, values); break;
                case 23: //decodeDeviceInfo(def, time_offset, values); /* device info */
                    break;
                case 101:
                    decodeLength(def, time_offset, values);
                    break; /* lap swimming */
                case 128: decodeWeather(def, time_offset, values); break; /* weather broadcast */

                case 1: /* capabilities, device settings and timezone */ break;
                case 2: decodeDeviceSettings(def, time_offset, values); break;
                case 3: /* USER_PROFILE */
                case 4: /* hrm profile */
                case 5: /* sdm profile */
                case 6: /* bike profile */
                case 7: /* ZONES_TARGET field#1 = MaxHR (bpm) */
                case 8: /* HR_ZONE */
                case 9: /* POWER_ZONE */
                case 10: /* MET_ZONE */
                case 12: /* SPORT */
                case 13: /* unknown */
                case 15: /* goal */
                case 22: /* source (undocumented) = sensors used for records ; see details below: */
                         /* #253: timestamp  /  #0: SPD/DIST  /  #1: SPD/DIST  /  #2: cadence  /  #4: HRM  /  #5: HRM */
                case 26: /* workout */
                case 27: /* workout step */
                case 28: /* schedule */
                case 29: /* location */
                case 30: /* weight scale */
                case 31: /* course */
                case 32: /* course point */
                case 33: /* totals */
                case 34: /* activity */
                case 35: /* software */
                case 37: /* file capabilities */
                case 38: /* message capabilities */
                case 39: /* field capabilities */
                case 49: /* file creator, software version ; see details below: */
                         /* #0: software version / #1: hardware version */
                case 51: /* blood pressure */
                case 53: /* speed zone */
                case 55: /* monitoring */
                case 72: /* training file (undocumented) : new since garmin 800 */
                case 78: /* hrv */
                case 79: /* HR zone (undocumented) ; see details below: */
                         /* #253: timestamp / #1: default Min HR / #2: default Max HR / #5: user Min HR / #6: user Max HR */
                case 103: /* monitoring info */
                case 104: /* battery */
                case 105: /* pad */
                case 106: /* salve device */
                case 113: /* unknown */

                case 125: /* unknown */
                case 131: /* cadence zone */

                case 140: /* unknown */
                case 141: /* unknown */
                    break;
                case SEGMENT_TYPE: // #142
                    decodeSegment(def, time_offset, values); /* segment data */
                    break;
                case 145: /* memo glob */
                case 147: /* equipment (undocumented) = sensors presets (sensor name, wheel circumference, etc.)  ; see details below: */
                          /* #0: equipment ID / #2: equipment name / #10: default wheel circ. value / #21: user wheel circ. value / #254: local eqt idx */
                case 148: /* segment description & metadata (undocumented) ; see details below: */
                          /* #0: segment name (string) / #1: segment UID (string) / #2: unknown, seems to be always 2 (enum) / #3: unknown, seems to be always 1 (enum)
                           / #4: exporting_user_id ? =user ID from connect ? (uint32) / #6: unknown, seems to be always 0 */
                case 149: /* segment leaderboard (undocumented) ; see details below: */
                          /* #1: who (0=segment leader, 1=personal best, 2=connection, 3=group leader, 4=challenger, 5+=H) 
                           / #3: ID of source garmin connect activity (uint32) ? OR ? timestamp ? / #4: time to finish (ms) / #254: message counter idx */
                case 150: /* segment trackpoint (undocumented) ; see details below: */
                          /* #1: latitude / #2: longitude / #3: distance from start point / #4: elevation / #5: timer since start (ms) / #6: message counter index */
                    break;

                default:
                    unknown_global_msg_nums.insert(def.global_msg_num);
            }
            last_msg_type = def.global_msg_num;
        }
        return count;
    }

    RideFile * run() {

        // get the Smart Recording parameters
        isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);
        GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
        if (GarminHWM.isNull() || GarminHWM.toInt() == 0) GarminHWM.setValue(25); // default to 25 seconds.

        // start
        rideFile = new RideFile;
        rideFile->setDeviceType("Garmin FIT");
        rideFile->setWindHeading(0.0);
        rideFile->setWindSpeed(0.0);
        rideFile->setRecIntSecs(1.0); // this is a terrible assumption!
        if (!file.open(QIODevice::ReadOnly)) {
            delete rideFile;
            return NULL;
        }

        int data_size = 0;
        try {

            // read the header
            int header_size = read_uint8();
            if (header_size != 12 && header_size != 14) {
                errors << QString("bad header size: %1").arg(header_size);
                file.close();
                delete rideFile;
                return NULL;
            }
            int protocol_version = read_uint8();
            (void) protocol_version;

            // if the header size is 14 we have profile minor then profile major
            // version. We still don't do anything with this information
            int profile_version = read_uint16(false); // always littleEndian
            (void) profile_version; // not sure what to do with this

            data_size = read_uint32(false); // always littleEndian
            char fit_str[5];
            if (file.read(fit_str, 4) != 4) {
                errors << "truncated header";
                file.close();
                delete rideFile;
                return NULL;
            }
            fit_str[4] = '\0';
            if (strcmp(fit_str, ".FIT") != 0) {
                errors << QString("bad header, expected \".FIT\" but got \"%1\"").arg(fit_str);
                file.close();
                delete rideFile;
                return NULL;
            }

            // read the rest of the header
            if (header_size == 14) read_uint16(false);

        } catch (TruncatedRead &e) {
            errors << "truncated file body";
            return NULL;
        }

        int bytes_read = 0;
        bool stop = false;
        bool truncated = false;
        try {
            while (!stop && (bytes_read < data_size))
                bytes_read += read_record(stop, errors);
        }
        catch (TruncatedRead &e) {
            errors << "truncated file body";
            //file.close();
            //delete rideFile;
            //return NULL;
            truncated = true;
        }
        if (stop) {
            file.close();
            delete rideFile;
            return NULL;
        }
        else {
            if (!truncated) {
                try {
                    int crc = read_uint16( false ); // always littleEndian
                    (void) crc;
                }
                catch (TruncatedRead &e) {
                    errors << "truncated file body";
                    return NULL;
                }
            }

            foreach(int num, unknown_global_msg_nums)
                qDebug() << QString("FitRideFile: unknown global message number %1; ignoring it").arg(num);
            foreach(int num, unknown_record_fields)
                qDebug() << QString("FitRideFile: unknown record field %1; ignoring it").arg(num);
            foreach(int num, unknown_base_type)
                qDebug() << QString("FitRideFile: unknown base type %1; skipped").arg(num);

            file.close();

            return rideFile;
        }
    }
};

RideFile *FitFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    QSharedPointer<FitFileReaderState> state(new FitFileReaderState(file, errors));
    return state->run();
}

// vi:expandtab tabstop=4 shiftwidth=4
