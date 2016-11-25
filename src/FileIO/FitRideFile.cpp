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

/* FIT has uint32 as largest integer type. So qint64 is large enough to
 * store all integer types - no matter if they're signed or not */

// this will need to change if float or other non-integer values are
// introduced into the file format
typedef qint64 fit_value_t;
#define NA_VALUE std::numeric_limits<fit_value_t>::max()
typedef std::string fit_string_value;
typedef float fit_float_value;

struct FitField {
    int num;
    int type; // FIT base_type
    int size; // in bytes
    int deve_idx; // Developer Data Index
};

struct FitDeveField {
    int dev_id; // Developer Data Index
    int num;
    int type; // FIT base_type
    int size; // in bytes
    int native; // native field number
    int scale;
    int offset;
    fit_string_value name;
    fit_string_value unit;
};

struct FitDefinition {
    int global_msg_num;
    bool is_big_endian;
    std::vector<FitField> fields;
};

enum fitValueType { SingleValue, ListValue, FloatValue, StringValue };
typedef enum fitValueType FitValueType;

struct FitValue
{
    FitValueType type;
    fit_value_t v;
    fit_string_value s;
    fit_float_value f;
    QList<fit_value_t> list;
    int size;
};

struct FitFileReaderState
{
    QFile &file;
    QStringList &errors;
    RideFile *rideFile;
    time_t start_time;
    time_t last_time;
    quint32 last_event_timestamp;
    double start_timestamp;
    double last_distance;
    QMap<int, FitDefinition> local_msg_types;
    QMap<QString, FitDeveField>  local_deve_fields; // All developer fields
    QMap<int, int> record_extra_fields;
    QMap<QString, int> record_deve_fields; // Developer fields in DEVELOPER XDATA or STANDARD DATA
    QMap<QString, int> record_deve_native_fields; // Developer fields with native values
    QSet<int> record_native_fields;
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
    double frac_time; // to carry sub-second length time in pool swimming
    double last_lap_end; // to align laps for drill mode in pool swimming
    QVariant isGarminSmartRecording;
    QVariant GarminHWM;
    XDataSeries *weatherXdata;
    XDataSeries *swimXdata;
    XDataSeries *deveXdata;
    XDataSeries *extraXdata;
    QMap<int, QString> deviceInfos;
    QList<QString> dataInfos;

    FitFileReaderState(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), start_time(0),
        last_time(0), last_distance(0.00f), interval(0), calibration(0),
        devices(0), stopped(true), isLapSwim(false), pool_length(0.0),
        last_event_type(-1), last_event(-1), last_msg_type(-1), frac_time(0.0),
        last_lap_end(0.0)
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

    fit_float_value read_float32(int *count = NULL) {
        float f;
        if (file.read(reinterpret_cast<char*>(&f), 4) != 4)
            throw TruncatedRead();
        if (count)
            (*count) += 4;

        return f;
    }

    void DumpFitValue(const FitValue& v) {
        printf("type: %d %llx %s\n", v.type, v.v, v.s.c_str());
    }

    void convert2Run() {
        if (rideFile->areDataPresent()->cad) {
            foreach(RideFilePoint *pt, rideFile->dataPoints()) {
                pt->rcad = pt->cad;
                pt->cad = 0;
            }
            rideFile->setDataPresent(RideFile::rcad, true);
            rideFile->setDataPresent(RideFile::cad, false);
        }
    }

    QString getManuProd(int manu, int prod) {
        if (manu == 1) {
            // Garmin
            // Product IDs can be found in c/fit_example.h in the FIT SDK.
            // Multiple product IDs refer to different regions e.g. China, Japan etc.
            switch (prod) {
                case 473: case 474: case 475: case 494: return "Garmin FR301";
                case 717: case 987: return "Garmin FR405";
                case 782: return "Garmin FR50";
                case 988: return "Garmin FR60";
                case 1018: return "Garmin FR310XT";
                case 1036: case 1199: case 1213: case 1387: return "Garmin Edge 500";
                case 1124: case 1274: return "Garmin FR110";
                case 1169: case 1333: case 1334: case 1386: return "Garmin Edge 800";
                case 1325: return "Garmin Edge 200";
                case 1328: return "Garmin FR910XT";
                case 1345: case 1410: return "Garmin FR610";
                case 1360: return "Garmin FR210";
                case 1436: return "Garmin FR70";
                case 1446: return "Garmin FR310XT 4T";
                case 1482: case 1688: return "Garmin FR10";
                case 1499: return "Garmin Swim";
                case 1551: return "Garmin Fenix";
                case 1561: case 1742: case 1821: return "Garmin Edge 510";
                case 1567: return "Garmin Edge 810";
                case 1623: case 2173: return "Garmin FR620";
                case 1632: case 2174: return "Garmin FR220";
                case 1765: case 2130: case 2131: case 2132: return "Garmin FR920XT";
                case 1836: case 2052: case 2053: case 2070: case 2100: return "Garmin Edge 1000";
                case 1903: return "Garmin FR15";
                case 1907: return "Garmin Vivoactive";
                case 1967: return "Garmin Fenix 2";
                case 2050: case 2188: case 2189: return "Garmin Fenix 3";
                case 2067: case 2260: return "Garmin Edge 520";
                case 2147: return "Garmin Edge 25";
                case 2153: case 2219: return "Garmin FR225";
                case 2156: return "Garmin FR630";
                case 2157: return "Garmin FR230";
                case 2204: return "Garmin Edge 1000 Explore";
                case 2238: return "Garmin Edge 20";
                case 2413: return "Garmin Fenix 3 HR";
                case 2431: return "Garmin FR235";
                case 2530: return "Garmin Edge 820";
                case 2531: return "Garmin Edge 820 Explore";
                case 20119: return "Garmin Training Center";
                case 65532: return "Android ANT+ Plugin";
                case 65534: return "Garmin Connect Website";
                default: return QString("Garmin %1").arg(prod);
            }
        } else if (manu == 6 ) {
            // SRM
            // powercontrol now uses FIT files from PC8
            switch (prod) {

            case 6: return "SRM PC6";
            case 7: return "SRM PC7";
            case 8: return "SRM PC8";
            default: return "SRM Powercontrol";
            }
        } else if (manu == 9 ) {
            // Powertap
            switch (prod) {
                case 14: return "Joule 2.0";
                case 18: return "Joule";
                case 19: return "Joule GPS";
                case 22: return "Joule GPS+";
                case 4096: return "Powertap G3";

                default: return QString("Powertap Device %1").arg(prod);
            }
        } else if (manu == 13 ) {
            // dynastream_oem
            switch (prod) {
                default: return QString("Dynastream %1").arg(prod);
            }
        } else if (manu == 29 ) {
            // saxonar
            switch (prod) {
                case 1031: return "Power2max S";
                default: return QString("Power2max %1").arg(prod);
            }
        } else if (manu == 32) {
            // wahoo
            switch (prod) {
                case 0: return "Wahoo fitness";
                default: return QString("Wahoo fitness %1").arg(prod);
            }
        } else if (manu == 38) {
            // o_synce
            switch (prod) {
                case 1: return "o_synce navi2coach";
                default: return QString("o_synce %1").arg(prod);
            }
        } else if (manu == 48) {
            // Pioneer
            switch (prod) {
                case 2: return "Pioneer SGX-CA500";
                default: return QString("Pioneer %1").arg(prod);
            }
        } else if (manu == 70) {
            // does not set product at this point
           return "Sigmasport ROX";
        } else if (manu == 76) {
            // Moxy
            return "Moxy Monitor";
        } else if (manu == 95) {
            // Stryd
            return "Stryd";
        } else if (manu == 98) {
            // BSX
            switch(prod) {
                  case 2: return "BSX Insight 2";
                  default: return QString("BSX %1").arg(prod);
            }
        } else if (manu == 260) {
            // Zwift!
            return "Zwift";
        } else if (manu == 267) {
            // Bryton!
            return "Bryton";
        } else {
            QString name = "Unknown FIT Device";
            return name + QString(" %1:%2").arg(manu).arg(prod);
        }
    }

    QString getDeviceType(int device_type) {
        switch (device_type) {
            case 4: return "Headunit"; // bike_power
            case 11: return "Powermeter"; // bike_power
            case 120: return "HR"; // heart_rate
            case 121: return "Speed-Cadence"; // bike_speed_cadence
            case 122: return "Cadence"; // bike_speed
            case 123: return "Speed"; // bike_speed
            case 124: return "Stride"; // stride_speed_distance

            default: return QString("Type %1").arg(device_type);
        }
    }

    RideFile::SeriesType getSeriesForNative(int native_num) {
        switch (native_num) {

            case 0: // POSITION_LAT
                    return RideFile::lat;
            case 1: // POSITION_LONG
                    return RideFile::lon;
            case 2: // ALTITUDE
                    return RideFile::alt;
            case 3: // HEART_RATE
                    return RideFile::hr;
            case 4: // CADENCE
                    return RideFile::cad;
            case 5: // DISTANCE
                    return RideFile::km;
            case 6: // SPEED
                    return RideFile::kph;
            case 7: // POWER
                    return RideFile::watts;
            case 9: // GRADE
                    return RideFile::slope;
            case 13: // TEMPERATURE
                    return RideFile::temp;
            case 30: //LEFT_RIGHT_BALANCE
                    return RideFile::lrbalance;
            case 39: // VERTICAL OSCILLATION
                    return RideFile::rvert;
            case 41: // GROUND CONTACT TIME
                    return RideFile::rcontact;
            case 54: // THb
                    return RideFile::thb;
            case 57: // SMO2
                    return RideFile::smo2;
            default:
                    return RideFile::none;
        }
    }

    void addRecordDeveField(QString key, FitDeveField deveField, bool xdata) {
        QString name = deveField.name.c_str();

        if (deveField.native>-1) {
            int i = 0;
            RideFile::SeriesType series = getSeriesForNative(deveField.native);
            QString nativeName = rideFile->symbolForSeries(series);

            if (nativeName.length() == 0)
                nativeName = QString("FIELD_%1").arg(deveField.native);
            else
                i++;

            QString typeName;
            if (xdata) {
                typeName = "DEVELOPER";
                do {
                    i++;
                    name = nativeName + (i>1?QString("-%1").arg(i):"");
                }
                while (deveXdata->valuename.contains(name));
            }
            else {
                typeName = "STANDARD";
                name = nativeName;
            }


            if (xdata && dataInfos.contains(QString("CIQ '%1' -> %2 %3").arg(deveField.name.c_str()).arg("STANDARD").arg(nativeName))) {
                int secs = last_time-start_time;
                int idx = dataInfos.indexOf(QString("CIQ '%1' -> %2 %3").arg(deveField.name.c_str()).arg("STANDARD").arg(nativeName));
                dataInfos.replace(idx, QString("CIQ '%1' -> %2 %3 (STANDARD until %4 secs)").arg(deveField.name.c_str()).arg(typeName).arg(name).arg(secs));
            } else
                dataInfos.append(QString("CIQ '%1' -> %2 %3").arg(deveField.name.c_str()).arg(typeName).arg(name));
        }

        if (xdata) {
            deveXdata->valuename << name;
            deveXdata->unitname << deveField.unit.c_str();

            record_deve_fields.insert(key, deveXdata->valuename.count()-1);
        } else
            record_deve_fields.insert(key, -1);
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
        rideFile->setDeviceType(getManuProd(manu, prod));
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
                            if (rideFile->dataPoints().count()>0)
                                convert2Run();
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
                    rideFile->setTag("Pool Length", // in meters
                                      QString("%1").arg(pool_length*1000.0));
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

        int index=-1;
        int manu = -1, prod = -1, version = -1, type = -1;
        fit_string_value name;

        QString deviceInfo;

        foreach(const FitField &field, def.fields) {
            FitValue value = values[i++];

            //qDebug() << field.num << value;

            switch (field.num) {
                case 0:   // device index
                     index = value.v;
                     break;
                case 1:   // ANT+ device type
                     type = value.v;
                     break;
                      // details: 0x78 = HRM, 0x79 = Spd&Cad, 0x7A = Cad, 0x7B = Speed
                case 2:   // manufacturer
                     manu = value.v;
                     break;
                case 4:   // product
                     prod = value.v;
                     break;
                case 5:   // software version
                     version = value.v;
                     break;
                case 27:   // product name
                     name = values[i++].s;
                 break;

                // all oher fields are ignored at present
                case 253: //timestamp
                case 3:   // serial number
                case 10:  // battery voltage
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
            //qDebug() << field.num << value.v;
        }

        //deviceInfo += QString("Device %1 ").arg(index);
        deviceInfo += QString("%1 ").arg(getDeviceType(type));
        if (manu>-1 && prod>-1)
            deviceInfo += getManuProd(manu, prod);
        if (name.length()>0)
            deviceInfo += QString(" %1").arg(name.c_str());
        if (version>0)
            deviceInfo += QString(" (v%1)").arg(version/100.0);

        // What is 7 and 0 ?
        // 3 for Moxy ?
        if (type>-1 && type != 0 && type != 7 && type != 3)
            deviceInfos.insert(index, deviceInfo);

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
                case 9:
                    total_distance = value.v / 100000.0;
                    break;

                // other data (ignored at present):
                case 254: // lap nbr
                case 3: // start_position_lat
                case 4: // start_position_lon
                case 5: // end_position_lat
                case 6: // end_position_lon
                case 7: // total_elapsed_time = value.v / 1000.0;
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
        if (this_start_time == 0 || this_start_time-start_time < 0) {
            //errors << QString("lap %1 has invalid start time").arg(interval);
            this_start_time = start_time; // time was corrected after lap start

            if (time == 0 || time-start_time < 0) {
                errors << QString("lap %1 is ignored (invalid end time)").arg(interval);
                return;
            }
        }
        if (isLapSwim) {
            // Fill empty laps due to false starts or pauses in some devices
            // s.t. Garmin 910xt
            double secs = time - start_time;
            if ((total_distance == 0.0) && (secs > last_time + 1) &&
                (isGarminSmartRecording.toInt() != 0) &&
                (secs - last_time < 100*GarminHWM.toInt())) {
                double deltaSecs = secs - last_time;
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
                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, interval);
                }
                last_time += deltaSecs;
            }
            ++interval;
        } else if (rideFile->dataPoints().count()) { // no samples means no laps
            ++interval;
            rideFile->addInterval(RideFileInterval::DEVICE,
                                  this_start_time - start_time,
                                  time - start_time,
                                  QObject::tr("Lap %1").arg(interval));
        }
    }

    void decodeRecord(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        if (isLapSwim) return; // We use the length message for Lap Swimming
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        double alt = 0, cad = 0, km = 0, hr = 0, lat = 0, lng = 0, badgps = 0, lrbalance = RideFile::NA;
        double kph = 0, temperature = RideFile::NA, watts = 0, slope = 0, headwind = 0;
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

        XDataPoint *p_deve = NULL;
        XDataPoint *p_extra = NULL;

        fit_value_t lati = NA_VALUE, lngi = NA_VALUE;
        int i = 0;
        foreach(const FitField &field, def.fields) {
            FitValue _values = values[i];
            fit_value_t value = values[i].v;
            QList<fit_value_t> valueList = values[i++].list;

            double deve_value = 0.0;

            if( value == NA_VALUE )
                continue;

            int native_num = field.num;
            bool native_profile = true;

            if (field.deve_idx>-1) {
                QString key = QString("%1.%2").arg(field.deve_idx).arg(field.num);
                //qDebug() << "deve_idx" << field.deve_idx << "num" << field.num << "type" << field.type;
                //qDebug() << "name" << local_deve_fields[key].name.c_str() << "unit" << local_deve_fields[key].unit.c_str() << local_deve_fields[key].offset << "(" << _values.v << _values.f << ")";

                if (record_deve_native_fields.contains(key) && !record_native_fields.contains(record_deve_native_fields[key])) {
                    native_num = record_deve_native_fields[key];

                    int scale = local_deve_fields[key].scale;
                    if (scale == -1)
                        scale = 1;
                    int offset = local_deve_fields[key].offset;
                    if (offset == -1)
                        offset = 0;

                    switch (_values.type) {
                        case SingleValue: deve_value=_values.v/(float)scale+offset; break;
                        case FloatValue: deve_value=_values.f/(float)scale+offset; break;
                        default: deve_value = 0.0; break;
                    }

                    // For compatibility with old Moxy deve Fields (with the native profile)
                    if (_values.type == SingleValue && (native_num == 54 || native_num == 57) )
                        native_profile = true;
                    else
                        native_profile = false;// Now Dynastream decided to use float values for CIQ

                    //qDebug() << "deve_value" << deve_value << native_profile;
                }
                else
                    native_num = -1;

                //qDebug()<< "native_num"<<native_num << time;
            } else {
                //qDebug()<< "native_num"<<native_num;
                if (!record_native_fields.contains(native_num)) {
                    record_native_fields.insert(native_num);
                }
            }

            if (native_num>-1) {

                switch (native_num) {
                    case 253: // TIMESTAMP
                              time = value + qbase_time.toTime_t();
                              // Time MUST NOT go backwards
                              // You canny break the laws of physics, Jim
                              if (time < last_time)
                                  time = last_time; // Not true for Bryton
                              break;
                    case 0: // POSITION_LAT
                            lati = value;
                            break;
                    case 1: // POSITION_LONG
                            lngi = value;
                            break;
                    case 2: // ALTITUDE
                            if (!native_profile && field.deve_idx>-1)
                                alt = deve_value;
                            else
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
                             if (!native_profile && field.deve_idx>-1)
                                rvert = deve_value;
                             else
                                rvert = value / 100.0f;
                             break;

                    //case 40: // GROUND CONTACT TIME PERCENT
                             //break;

                    case 41: // GROUND CONTACT TIME
                             if (!native_profile && field.deve_idx>-1)
                                 rcontact = deve_value;
                             else
                                rcontact = value / 10.0f;
                             break;

                    //case 42: // ACTIVITY_TYPE
                    //         // TODO We should know/test value for run
                    //         run = true;
                    //         break;

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
                             if (rideFile->getTag("Sport", "Bike") == "Run")
                                 rcad += value/128.0f;
                             else
                                 cad += value/128.0f;
                             break;
                    case 54: // tHb
                             if (!native_profile && field.deve_idx>-1) {
                                tHb = deve_value;
                             }
                             else
                                tHb= value/100.0f;
                             break;
                    case 57: // SMO2
                             if (!native_profile && field.deve_idx>-1) {
                                 smO2 = deve_value;
                             }
                             else
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
                             leftTopDeathCenter = round(valueList.at(0) * 360.0/256);
                             leftBottomDeathCenter = round(valueList.at(1) * 360.0/256);
                             break;
                    case 70: // ? Left Peak Phase  ?
                             leftTopPeakPowerPhase = round(valueList.at(0) * 360.0/256);
                             leftBottomPeakPowerPhase = round(valueList.at(1) * 360.0/256);
                             break;
                    case 71: // ? Right Power Phase ?
                             rightTopDeathCenter = round(valueList.at(0) * 360.0/256);
                             rightBottomDeathCenter = round(valueList.at(1) * 360.0/256);
                             break;
                    case 72: // ? Right Peak Phase  ?
                             rightTopPeakPowerPhase = round(valueList.at(0) * 360.0/256);
                             rightBottomPeakPowerPhase = round(valueList.at(1) * 360.0/256);
                             break;
                    case 84: // Left right balance
                             lrbalance = value/100.0;
                             break;

                    case 87: // ???
                             break;


                    default:
                            unknown_record_fields.insert(native_num);
                            native_num = -1;
                }
            }

            if (native_num == -1) {
                // native, deve_native or deve to record.

                int idx = -1;

                if (field.deve_idx>-1) {
                    QString key = QString("%1.%2").arg(field.deve_idx).arg(field.num);
                    FitDeveField deveField = local_deve_fields[key];

                    if (!record_deve_fields.contains(key)) {
                        addRecordDeveField(key, deveField, true);
                    } else {
                        if (record_deve_fields[key] == -1) {
                            addRecordDeveField(key, deveField, true);
                        }
                    }
                    idx = record_deve_fields[key];

                    if (idx>-1) {
                        if (p_deve == NULL &&
                                (_values.type == SingleValue ||
                                 _values.type == FloatValue ||
                                 _values.type == StringValue))
                           p_deve = new XDataPoint();

                        int scale = deveField.scale;
                        if (scale == -1)
                            scale = 1;
                        int offset = deveField.offset;
                        if (offset == -1)
                            offset = 0;

                        switch (_values.type) {
                            case SingleValue: p_deve->number[idx]=_values.v/(float)scale+offset; break;
                            case FloatValue: p_deve->number[idx]=_values.f/(float)scale+offset; break;
                            case StringValue: p_deve->string[idx]=_values.s.c_str(); break;
                            default: break;
                        }
                    }
                } else {
                    // Store standard native ignored
                    if (!record_extra_fields.contains(field.num)) {
                        RideFile::SeriesType series = getSeriesForNative(field.num);
                        QString nativeName = rideFile->symbolForSeries(series);

                        if (nativeName.length() == 0)
                            nativeName = QString("FIELD_%1").arg(field.num);

                        extraXdata->valuename << nativeName;
                        extraXdata->unitname << "";

                        //dataInfos.append(QString("EXTRA %1").arg(nativeName));

                        record_extra_fields.insert(field.num, record_extra_fields.count());
                    }
                    idx = record_extra_fields[field.num];

                    if (idx>-1) {
                        if (p_extra == NULL &&
                                (_values.type == SingleValue ||
                                 _values.type == FloatValue ||
                                 _values.type == StringValue))
                           p_extra = new XDataPoint();

                        switch (_values.type) {
                            case SingleValue: p_extra->number[idx]=_values.v; break;
                            case FloatValue: p_extra->number[idx]=_values.f; break;
                            case StringValue: p_extra->string[idx]=_values.s.c_str(); break;
                            default: break;
                        }
                    }

                }


            } else {
                if (field.deve_idx>-1) {
                    QString key = QString("%1.%2").arg(field.deve_idx).arg(field.num);
                    FitDeveField deveField = local_deve_fields[key];

                    if (!record_deve_fields.contains(key)) {
                        addRecordDeveField(key, deveField, false);
                    }
                }
            }
        }

        if (time == last_time)
            return; // Not true for Bryton

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
            // double deltaHeadwind = headwind - prevPoint->headwind;
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
                        0.0, // headwind
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
        rideFile->appendOrUpdatePoint(secs, cad, hr, km, kph, nm, watts, alt, lng, lat, headwind, slope, temperature,
                     lrbalance, leftTorqueEff, rightTorqueEff, leftPedalSmooth, rightPedalSmooth,
                     leftPedalCenterOffset, rightPedalCenterOffset,
                     leftTopDeathCenter, rightTopDeathCenter, leftBottomDeathCenter, rightBottomDeathCenter,
                     leftTopPeakPowerPhase, rightTopPeakPowerPhase, leftBottomPeakPowerPhase, rightBottomPeakPowerPhase,
                     smO2, tHb, rvert, rcad, rcontact, 0.0, interval, false);
        last_time = time;
        last_distance = km;

        if (p_deve != NULL) {
            p_deve->secs = secs;
            deveXdata->datapoints.append(p_deve);
        }
        if (p_extra != NULL) {
            p_extra->secs = secs;
            extraXdata->datapoints.append(p_extra);
        }
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
                interval = 1;
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

        int length_type = 0;
        int swim_stroke = 0;
        int total_strokes = 0;
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
                            time = last_time; // Not true for Bryton
                        break;
                case 3: // total elapsed time
                        length_duration = value / 1000.0;
                        break;
                case 4: // total timer time
                        if (FIT_DEBUG) qDebug() << " total_timer_time:" << value;
                        break;
                case 5: // total strokes
                        total_strokes = value;
                        break;
                case 6: // avg speed
                        kph = value * 3.6 / 1000.0;
                        break;
                case 7: // swim stroke: 0-free, 1-back, 2-breast, 3-fly,
                        //              4-drill, 5-mixed, 6-IM
                        swim_stroke = value;
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

        XDataPoint *p = new XDataPoint();
        p->secs = last_time;
        p->km = last_distance;
        p->number[0] = length_type + swim_stroke;
        p->number[1] = length_duration;
        p->number[2] = total_strokes;

        swimXdata->datapoints.append(p);

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
            interval = 1;
        }

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

        // Adjust length duration using fractional carry
        length_duration += frac_time;
        frac_time = modf(length_duration, &length_duration);

        // only fill 100x the maximal smart recording gap defined
        // in preferences - we don't want to crash / stall on bad
        // or corrupt files
        if ((isGarminSmartRecording.toInt() != 0) && length_duration > 0 && length_duration < 100*GarminHWM.toInt()) {
            double deltaSecs = length_duration;
            double deltaDist = km - last_distance;
            kph = 3600.0 * deltaDist / deltaSecs;
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
                    interval);
            }
            last_time += deltaSecs;
            last_distance += deltaDist;
        }
    }

    /* weather broadcast as observed at weather station (undocumented) */
    void decodeWeather(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);

        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        double windHeading = 0.0, windSpeed = 0.0, temp = 0.0, humidity = 0.0;

        int i = 0;
        foreach(const FitField &field, def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: // Timestamp
                          time = value + qbase_time.toTime_t();
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
                        windHeading = value ; // 180.0 * MATHCONST_PI;
                        break;
                case 4:  // Wind speed (mm/s)
                        windSpeed = value * 0.0036; // km/h
                        break;
                case 1:  // Temperature
                        temp = value;
                        break;
                case 7:  // Humidity
                        humidity = value;
                        break;
                default: ; // ignore it
            }
        }

        double secs = time - start_time;
        XDataPoint *p = new XDataPoint();
        p->secs = secs;
        p->km = last_distance;
        p->number[0] = windSpeed;
        p->number[1] = windHeading;
        p->number[2] = temp;
        p->number[3] = humidity;

        weatherXdata->datapoints.append(p);
    }

    void decodeHr(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        time_t time = 0;
        if (time_offset > 0) {
            time = last_time + time_offset;
        }

        QList<double> timestamps;
        QList<double> hr;

        int a = 0;
        int j = 0;
        foreach(const FitField &field, def.fields) {
            FitValue value = values[a++];

            if( value.type == SingleValue && value.v == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: // Timestamp
                          time = value.v + qbase_time.toTime_t();
                          break;
                case 0:   // fractional_timestamp
                          break;
                case 1:	  // time256
                          break;
                case 6:	  // filtered_bpm
                          if (value.type == SingleValue) {
                            hr.append(value.v);
                          }
                          else {
                              for (int i=0;i<value.list.size();i++) {
                                  hr.append(value.list.at(i));
                              }
                          }
                          break;

                case 9:   // event_timestamp
                          last_event_timestamp = value.v;
                          start_timestamp = time-last_event_timestamp/1024.0;
                          timestamps.append(last_event_timestamp/1024.0);
                          break;
                case 10:  // event_timestamp_12
                          j=0;
                          for (int i=0;i<value.list.size()-1;i++) {

                              qint16 last_event_timestamp12 = last_event_timestamp & 0xFFF;
                              qint16 next_event_timestamp12;

                              if (j%2 == 0) {
                                  next_event_timestamp12 = value.list.at(i) + ((value.list.at(i+1) & 0xF) << 8);
                                  last_event_timestamp = (last_event_timestamp & 0xFFFFF000) + next_event_timestamp12;
                              } else {
                                  next_event_timestamp12 = 16 * value.list.at(i+1) + ((value.list.at(i) & 0xF0) >> 4);
                                  last_event_timestamp = (last_event_timestamp & 0xFFFFF000) + next_event_timestamp12;
                                  i++;
                              }
                              if (next_event_timestamp12 < last_event_timestamp12)
                                  last_event_timestamp += 0x1000;

                              timestamps.append(last_event_timestamp/1024.0);
                              j++;
                          }

                          break;

                default: ; // ignore it
            }
        }

        for (int i=0;i<timestamps.count(); i++) {
            double secs = round(timestamps.at(i) + start_timestamp - start_time);
            if (secs>0) {
                int idx = rideFile->timeIndex(round(secs));

                if (rideFile->dataPoints().at(idx)->secs==secs)
                    rideFile->appendOrUpdatePoint(
                            secs, 0.0, hr.at(i),
                            0.0,
                            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, RideFile::NA, RideFile::NA,
                            0.0, 0.0,
                            0.0, 0.0,
                            0.0, 0.0,
                            0.0, 0.0,
                            0.0, 0.0,
                            0.0, 0.0,
                            0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, false);
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

        QString segment_name;
        bool fail = false;

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
                    //not used XXX total_distance = value.v / 100000.0;
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
                case 64:  // status
                    fail = (value.v == 1);
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

        if (fail) { // Segment started but not ended
            // no interval
            return;
        }

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

    void decodeDeveloperFieldDescription(const FitDefinition &def, int time_offset,
                      const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);
        int i = 0;

        FitDeveField fieldDef;
        fieldDef.scale = -1;
        fieldDef.offset = -1;
        fieldDef.native = -1;

        foreach(const FitField &field, def.fields) {
            FitValue value = values[i++];

            //qDebug() << "deve : num" << field.num  << value.v << value.s.c_str();

            switch (field.num) {
                case 0:  // developer_data_index
                        fieldDef.dev_id = value.v;
                        break;
                case 1:  // field_definition_number
                        fieldDef.num = value.v;
                        break;
                case 2:  // fit_base_type_id
                        fieldDef.type = value.v;
                        break;
                case 3:  // field_name
                        fieldDef.name = value.s;
                        break;
                case 4:  // array
                        break;
                case 5:  // components
                        break;
                case 6:  // scale
                        fieldDef.scale = value.v;
                        break;
                case 7:  // offset
                        fieldDef.offset = value.v;
                        break;
                case 8:  // units
                        fieldDef.unit = value.s;
                        break;
                case 9:  // bits
                case 10: // accumulate
                case 13: // fit_base_unit_id
                case 14: // native_mesg_num
                        break;
                case 15: // native field number
                        fieldDef.native = value.v;
                        break;
                default:
                        // ignore it
                        break;
            }
        }

        //qDebug() << "num" << fieldDef.num << "deve_idx" << fieldDef.dev_id << "type" << fieldDef.type << "native" << fieldDef.native << "name" << fieldDef.name.c_str() << "unit" << fieldDef.unit.c_str() << "scale" << fieldDef.scale << "offset" << fieldDef.offset;

        QString key = QString("%1.%2").arg(fieldDef.dev_id).arg(fieldDef.num);

        if (!local_deve_fields.contains(key)) {
            local_deve_fields.insert((key), fieldDef);
        }

        if (fieldDef.native > -1 && !record_deve_native_fields.values().contains(fieldDef.native)) {
            record_deve_native_fields.insert(key, fieldDef.native);

            /*RideFile::SeriesType series = getSeriesForNative(fieldDef.native);

            if (series != RideFile::none) {
                QString nativeName = rideFile->symbolForSeries(series);
                dataInfos.append(QString("NATIVE %1 : Field %2").arg(nativeName).arg(fieldDef.name.c_str()));
            }*/
        }
    }

    void read_header(bool &stop, QStringList &errors, int &data_size) {
        stop = false;
        try {
            // read the header
            int header_size = read_uint8();
            if (header_size != 12 && header_size != 14) {
                errors << QString("bad header size: %1").arg(header_size);
                stop = true;
            }
            int protocol_version = read_uint8();
            (void) protocol_version;

            // if the header size is 14 we have profile minor then profile major
            // version. We still don't do anything with this information
            int profile_version = read_uint16(false); // always littleEndian
            (void) profile_version;
            //qDebug() << "profile_version" << profile_version/100.0; // not sure what to do with this

            data_size = read_uint32(false); // always littleEndian
            char fit_str[5];
            if (file.read(fit_str, 4) != 4) {
                errors << "truncated header";
                stop = true;
            }
            fit_str[4] = '\0';
            if (strcmp(fit_str, ".FIT") != 0) {
                errors << QString("bad header, expected \".FIT\" but got \"%1\"").arg(fit_str);
                stop = true;
            }

            // read the rest of the header
            if (header_size == 14) read_uint16(false);
        } catch (TruncatedRead &e) {
            errors << "truncated file header";
            stop = true;
        }
    }

    int read_record(bool &stop, QStringList &errors) {
        stop = false;
        int count = 0;
        int header_byte = read_uint8(&count);
        if (!(header_byte & 0x80) && (header_byte & 0x40)) {
            // Definition record
            int local_msg_type = header_byte & 0xf;
            bool with_deve_data = (header_byte & 0x20) == 0x20 ;

            local_msg_types.insert(local_msg_type, FitDefinition());
            FitDefinition &def = local_msg_types[local_msg_type];

            int reserved = read_uint8(&count); (void) reserved; // unused
            def.is_big_endian = read_uint8(&count);
            def.global_msg_num = read_uint16(def.is_big_endian, &count);
            int num_fields = read_uint8(&count);

            if (FIT_DEBUG)  {
                //qDebug() << "definition: local type=" << local_msg_type << "global=" << def.global_msg_num << "arch=" << def.is_big_endian << "fields=" << num_fields;

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
                field.deve_idx = -1;

                if (FIT_DEBUG) {
                    printf("  field %d: %d bytes, num %d, type %d, size %d\n",
                           i, field.size, field.num, field.type, field.size );
                }
            }

            if (with_deve_data) {

                int num_fields = read_uint8(&count);

                for (int i = 0; i < num_fields; ++i) {
                    def.fields.push_back(FitField());
                    FitField &field = def.fields.back();

                    field.num = read_uint8(&count);
                    field.size = read_uint8(&count);
                    field.deve_idx = read_uint8(&count);

                    QString key = QString("%1.%2").arg(field.deve_idx).arg(field.num);
                    FitDeveField devField = local_deve_fields[key];
                    field.type = devField.type & 0x1f;

                    //qDebug() << "field" << field.num << "type" << field.type << "size" << field.size << "deve idx" << field.deve_idx;

                    if (FIT_DEBUG) {
                        printf("  field %d: %d bytes, num %d, type %d, size %d\n",
                               i, field.size, field.num, field.type, field.size );
                    }
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
                printf( "local type %d without previous definition\n", local_msg_type );
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
                    case 0: size = 1;
                            if (field.size==size) {
                                value.type = SingleValue; value.v = read_uint8(&count); size = 1;
                             } else { // Multi-values
                                value.type = ListValue;
                                value.list.clear();
                                for (int i=0;i<field.size/size;i++) {
                                    value.list.append(read_uint8(&count));
                                }
                                size = field.size;
                            }
                            break;
                    case 1: value.type = SingleValue; value.v = read_int8(&count); size = 1;  break;
                    case 2: size = 1;
                            if (field.size==size) {
                                value.type = SingleValue; value.v = read_uint8(&count);
                            } else { // Multi-values
                                value.type = ListValue;
                                value.list.clear();
                                for (int i=0;i<field.size/size;i++) {
                                    value.list.append(read_uint8(&count));
                                }
                                size = field.size;
                            }
                            break;
                    case 3: value.type = SingleValue; value.v = read_int16(def.is_big_endian, &count); size = 2;  break;
                    case 4: size = 2;
                            if (field.size==size) {
                                value.type = SingleValue; value.v = read_uint16(def.is_big_endian, &count);
                            } else { // Multi-values
                                value.type = ListValue;
                                value.list.clear();
                                for (int i=0;i<field.size/size;i++) {
                                    value.list.append(read_uint16(def.is_big_endian, &count));
                                }
                                size = field.size;
                            }
                            break;
                    case 5: value.type = SingleValue; value.v = read_int32(def.is_big_endian, &count); size = 4;  break;
                    case 6: size = 4;
                            if (field.size==size) {
                                value.type = SingleValue; value.v = read_uint32(def.is_big_endian, &count);
                            } else { // Multi-values
                                value.type = ListValue;
                                value.list.clear();
                                for (int i=0;i<field.size/size;i++) {
                                    value.list.append(read_uint32(def.is_big_endian, &count));
                                }
                                size = field.size;
                            }
                            break;
                    case 7:
                        value.type = StringValue;
                        value.s = read_text(field.size, &count);
                        size = field.size;
                        break;

                    case 8: // FLOAT32
                        size = 4;
                        value.type = FloatValue;
                        value.f = read_float32(&count);
                        size = field.size;

                        break;

                    //case 9: // FLOAT64

                    case 10: size = 1;
                             if (field.size==size) {
                                value.type = SingleValue; value.v = read_uint8z(&count); size = 1;
                             } else { // Multi-values
                                 value.type = ListValue;
                                 value.list.clear();
                                 for (int i=0;i<field.size/size;i++) {
                                     value.list.append(read_uint8z(&count));
                                 }
                                 size = field.size;
                             }
                             break;
                    case 11: value.type = SingleValue; value.v = read_uint16z(def.is_big_endian, &count); size = 2; break;
                    case 12: value.type = SingleValue; value.v = read_uint32z(def.is_big_endian, &count); size = 4; break;
                    case 13: // BYTE
                             value.type = ListValue;
                             value.list.clear();
                             for (int i=0;i<field.size;i++) {
                                value.list.append(read_uint8(&count));
                             }
                             size = value.list.size();
                             break;

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
                        unknown_base_type.insert(field.type);
                        size = field.size;
                }
                // Size is greater than expected
                if (size < field.size) {
                    if (FIT_DEBUG)  {
                         printf( "   warning : size=%d for type=%d (num=%d)\n",
                                 field.size, field.type, field.num);
                    }
                    read_unknown( field.size-size, &count );
                }

                values.push_back(value);

                if (FIT_DEBUG) {
                    printf( " field: type=%d num=%d size=%d(%d) ",
                        field.type, field.num, field.size, size);
                    if (value.type == SingleValue) {
                        if (value.v == NA_VALUE)
                            printf( "value=NA\n");
                        else
                            printf( "value=%lld\n", value.v );
                    }
                    else if (value.type == StringValue)
                        printf( "value=%s\n", value.s.c_str() );
                    else if (value.type == ListValue) {
                        printf( "values=");
                        for (int i=0;i<value.list.count();i++) {
                            if (value.v == NA_VALUE)
                                printf( "NA,");
                            else
                                printf( "%lld,", value.list.at(i) );
                        }
                        printf( "\n");
                    }
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
                case 23:
                    //decodeDeviceInfo(def, time_offset, values); /* device info */
                    break;
                case 101:
                    decodeLength(def, time_offset, values);
                    break; /* lap swimming */
                case 128:
                    decodeWeather(def, time_offset, values);
                    break; /* weather broadcast */
                case 132:
                    decodeHr(def, time_offset, values); /* HR */
                    break;
                case SEGMENT_TYPE: // #142
                    decodeSegment(def, time_offset, values); /* segment data */
                    break;

                case 206: // Developer Field Description
                    decodeDeveloperFieldDescription(def, time_offset, values);
                    break;


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
                case 207: /* Developer ID */
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
        weatherXdata = new XDataSeries();
        weatherXdata->name = "WEATHER";
        weatherXdata->valuename << "WINDSPEED";
        weatherXdata->unitname << "kph";
        weatherXdata->valuename << "WINDHEADING";
        weatherXdata->unitname << "";
        weatherXdata->valuename << "TEMPERATURE";
        weatherXdata->unitname << "C";
        weatherXdata->valuename << "HUMIDITY";
        weatherXdata->unitname << "relative humidity";

        swimXdata = new XDataSeries();
        swimXdata->name = "SWIM";
        swimXdata->valuename << "TYPE";
        swimXdata->unitname << "stroketype";
        swimXdata->valuename << "DURATION";
        swimXdata->unitname << "secs";
        swimXdata->valuename << "STROKES";
        swimXdata->unitname << "";

        deveXdata = new XDataSeries();
        deveXdata->name = "DEVELOPER";

        extraXdata = new XDataSeries();
        extraXdata->name = "EXTRA";

        bool stop = false;
        bool truncated = false;

        // read the header
        read_header(stop, errors, data_size);

        if (!stop) {

            int bytes_read = 0;

            try {
                while (!stop && (bytes_read < data_size)) {
                    bytes_read += read_record(stop, errors);
                }
            }
            catch (TruncatedRead &e) {
                errors << "truncated file body";
                //file.close();
                //delete rideFile;
                //return NULL;
                truncated = true;
            }
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

                // second file ?
                try {
                    while (file.canReadLine()) {
                        read_header(stop, errors, data_size);
                        if (!stop) {

                            int bytes_read = 0;

                            try {
                                while (!stop && (bytes_read < data_size)) {
                                    bytes_read += read_record(stop, errors);
                                }
                            }
                            catch (TruncatedRead &e) {
                                errors << "truncated second file body";
                            }
                        }
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
                    }
                }
                catch (TruncatedRead &e) {
                }

            }

            foreach(int num, unknown_global_msg_nums)
                qDebug() << QString("FitRideFile: unknown global message number %1; ignoring it").arg(num);
            foreach(int num, unknown_record_fields)
                qDebug() << QString("FitRideFile: unknown record field %1; ignoring it").arg(num);
            foreach(int num, unknown_base_type)
                qDebug() << QString("FitRideFile: unknown base type %1; skipped").arg(num);

            QString deviceInfo;
            foreach(QString info, deviceInfos) {
                deviceInfo += info + "\n";
            }
            if (deviceInfo.length()>0)
                rideFile->setTag("Device Info", deviceInfo);

            QString dataInfo;
            foreach(QString info, dataInfos) {
                dataInfo += info + "\n";
            }
            if (dataInfo.length()>0)
                rideFile->setTag("Data Info", dataInfo);

            file.close();

            if (weatherXdata->datapoints.count()>0)
                rideFile->addXData("WEATHER", weatherXdata);
            else
                delete weatherXdata;

            if (swimXdata->datapoints.count()>0)
                rideFile->addXData("SWIM", swimXdata);
            else
                delete swimXdata;

            if (deveXdata->datapoints.count()>0)
                rideFile->addXData("DEVELOPER", deveXdata);
            else
                delete deveXdata;

            if (extraXdata->datapoints.count()>0)
                rideFile->addXData("EXTRA", extraXdata);
            else
                delete extraXdata;

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
