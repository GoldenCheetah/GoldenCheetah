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
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <limits>
#include <cmath>
#define FIT_DEBUG false // debug traces

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
        last_time(0), last_distance(0.00f), interval(0), calibration(0), devices(0), stopped(true), isLapSwim(false), pool_length(0.0),
        last_event_type(-1), last_event(-1), last_msg_type(-1)
    {
    }

    struct TruncatedRead {};

    void read_unknown( int size, int *count = NULL ){
        char c[size+1];

        if (file.read(c, size ) != size)
            throw TruncatedRead();
        if (count)
            (*count) += size;
    }

    fit_string_value read_text(int len, int *count = NULL)
    {
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

    void decodeFileId(const FitDefinition &def, int, const std::vector<FitValue> values) {
        int i = 0;
        int manu = -1, prod = -1;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 1: manu = value; break;
                case 2: prod = value; break;
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
                case 1765: case 2130: case 2131: case 2132: rideFile->setDeviceType("Garmin FR920XT"); break;
                case 1836: case 2052: case 2053: case 2070: case 2100: rideFile->setDeviceType("Garmin Edge 1000"); break;
                case 1903: rideFile->setDeviceType("Garmin FR15"); break;
                case 1967: rideFile->setDeviceType("Garmin Fenix2"); break;
                case 2050: case 2188: case 2189: rideFile->setDeviceType("Garmin Fenix3"); break;
                case 2067: rideFile->setDeviceType("Garmin Edge 520"); break;
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

    void decodeSession(const FitDefinition &def, int, const std::vector<FitValue> values) {
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
                    }
                    break;
                case 44: // pool_length
                    pool_length = value/100000;
                    break;
                default: ; // do nothing
            }

            if (FIT_DEBUG) {
                printf("decodeSession  field %d: %d bytes, num %d, type %d\n", i, field.size, field.num, field.type );
            }
        }
    }

    void decodeDeviceInfo(const FitDefinition &def, int, const std::vector<FitValue> values) {
        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            if (FIT_DEBUG) {
                printf("decodeDeviceInfo  field %d: %d bytes, num %d, type %d\n", i, field.size, field.num, field.type );
            }
        }
    }

    void decodeEvent(const FitDefinition &def, int, const std::vector<FitValue> values) {
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
                    errors << QString("Unknown timer event type %1").arg(event_type);
            }
        }
        else if (event == 36) { // Calibration event
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

        if (FIT_DEBUG) {
            printf("event type %d\n", event_type);
        }
        last_event = event;
        last_event_type = event_type;
    }

    void decodeLap(const FitDefinition &def, int time_offset, const std::vector<FitValue> values) {
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        else
            time = last_time;
        int i = 0;
        time_t this_start_time = 0;
        ++interval;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: time = value + qbase_time.toTime_t(); break;
                case 2: this_start_time = value + qbase_time.toTime_t(); break;
                default: ; // ignore it
            }
        }
        if (this_start_time == 0 || this_start_time-start_time < 0) {
            //errors << QString("lap %1 has invalid start time").arg(interval);
            this_start_time = start_time; // time was corrected after lap start

            if (time == 0 || time-start_time < 0) {
                errors << QString("lap %1 is ignored (invalid end time)").arg(interval);
                return;
            }
        }
        if (rideFile->dataPoints().count()) // no samples means no laps..
            rideFile->addInterval(RideFileInterval::DEVICE, this_start_time - start_time, time - start_time, QString(QObject::tr("Lap %1")).arg(interval));
    }

    void decodeRecord(const FitDefinition &def, int time_offset, const std::vector<FitValue> values) {
        if (isLapSwim) return; // We use the length message for Lap Swimming
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        double alt = 0, cad = 0, km = 0, hr = 0, lat = 0, lng = 0, badgps = 0, lrbalance = 0;
        double kph = 0, temperature = RideFile::NoTemp, watts = 0, slope = 0;
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
                case 61: // ? GPS Altitude ?
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
        double headwind = 0.0;
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

    void decodeLength(const FitDefinition &def, int time_offset, const std::vector<FitValue> values) {
        if (!isLapSwim) {
            isLapSwim = true;
            // reset rideFile if not empty
            if (!rideFile->dataPoints().empty()) {
                start_time = 0;
                last_time = 0;
                last_distance = 0.00f;
                interval = 0;
                delete rideFile;
                rideFile = new RideFile;
                rideFile->setDeviceType("Garmin FIT");
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
            else if (fabs(pool_length - 0.025) < 0.002) pool_length = 0.025;
            else if (fabs(pool_length - 0.025*METERS_PER_YARD) < 0.002) pool_length = 0.025*METERS_PER_YARD;
            else if (fabs(pool_length - 0.020) < 0.002) pool_length = 0.020;
        }

        // another pool length or pause
        km = last_distance + (kph > 0.0 ? pool_length : 0.0);

        if ((secs > last_time + 1) && (isGarminSmartRecording.toInt() != 0) && (secs - last_time < 10*GarminHWM.toInt())) {
            double deltaSecs = secs - last_time;
            for (int i = 1; i <= deltaSecs; i++) {
                rideFile->appendPoint(
                    last_time+i, 0.0, 0.0,
                    last_distance,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, RideFile::NoTemp,
                    0.0, 0.0, 0.0,
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
        double deltaSecs = length_duration;
        double deltaDist = km - last_distance;

        // only fill 10x the maximal smart recording gap defined
        // in preferences - we don't want to crash / stall on bad
        // or corrupt files
        if ((isGarminSmartRecording.toInt() != 0) && deltaSecs > 0 && deltaSecs < 10*GarminHWM.toInt()) {

            for (int i = 1; i <= deltaSecs; i++) {
                double weight = i /deltaSecs;
                rideFile->appendPoint(
                    last_time + (deltaSecs * weight), cad, 0.0,
                    last_distance + (deltaDist * weight),
                    kph, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, RideFile::NoTemp,
                    0.0, 0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0);
            }
            last_time += deltaSecs;
            last_distance += deltaDist;
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
                    case 7: value.type = StringValue; value.s = read_text(field.size, &count); size = field.size; break;


                    //case 8: // FLOAT32
                    //case 9: // FLOAT64
                    case 10: value.type = SingleValue; value.v = read_uint8z(&count); size = 1; break;
                    case 11: value.type = SingleValue; value.v = read_uint16z(def.is_big_endian, &count); size = 2; break;
                    case 12: value.type = SingleValue; value.v = read_uint32z(def.is_big_endian, &count); size = 4; break;
                    //case 13: // BYTE

                    // we may need to add support for float, string + byte base types here
                    default:
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
                        printf( "salue=%s\n",value.s.c_str() );
                }
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

                case 23: //decodeDeviceInfo(def, time_offset, values); break; /* device info */
                case 18: decodeSession(def, time_offset, values); break; /* session */

                case 101: decodeLength(def, time_offset, values); break; /* lap swimming */

                case 2: /* DEVICE_SETTINGS */
                case 3: /* USER_PROFILE */
                case 7: /* ZONES_TARGET12 */
                case 8: /* HR_ZONE */
                case 9: /* POWER_ZONE */
                case 12: /* SPORT */
                case 13: /* unknown */
                case 22: /* undocumented */
                case 72: /* undocumented  - new for garmin 800*/
                case 34: /* activity */
                case 49: /* file creator */
                case 79: /* unknown */
                case 104: /* battery */
                case 113: /* unknown */
                case 125: /* unknown */
                case 128: /* unknown */
                case 140: /* unknown */
                case 141: /* unknown */
                case 147: /* unknown */

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
