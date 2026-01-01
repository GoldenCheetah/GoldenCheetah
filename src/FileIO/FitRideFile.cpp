/*
 * Copyright (c) 2007-2008 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2016-2017 Damien Grauser (Damien.Grauser@pev-geneve.ch)
 * Copyright (c) 2023 Mark Liversedge (liversedge@gmail.com)
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
#include "RideItem.h"
#include "Specification.h"
#include "DataProcessor.h"
#include "MainWindow.h" // for gcroot
#include "GcUpgrade.h" // for VERSION_CONFIG_PREFIX
#include <QSharedPointer>
#include <QMap>
#include <QSet>
#include <QtEndian>
#include <QDebug>
#include <QTime>
#include <cstdio>
#include <stdint.h>
#include <sstream>
#include <time.h>
#include <limits>
#include <cmath>

#define FIT_DEBUG               false // debug traces true/false- can generate many MB of logs
                                      //
                                      //               NOTE: fit records are the top level container for data
                                      //                     they contain messages of many types but the most numerous
                                      //                     are message type "record" which contain sample recording
                                      //                     its confusing terminology but we refer to "fit records"
                                      //                     and "record messages" in the comments to distinguish between
                                      //                     the two different data objects
                                      //
#define FIT_DEBUG_LEVEL         2     // debug level : 1 fit record processing (incl. local message definitions & developer definitions)
                                      //               2 message processing but not record messages
                                      //               3 message processing incl. record messages
                                      //               4 field level processing
                                      //               5 general information

static int fitFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "fit", "Garmin FIT", new FitFileReader());

// 1989-12-31 00:00:00 UTC
static const QDateTime qbase_time(QDate(1989, 12, 31), QTime(0, 0, 0), QTimeZone::UTC);


//
// SOURCE FILE CONTENTS
//
// This is a large source file that probably needs to be refactored.
//
// It has been organised into 4 sections, Section 1 is the majority
// of the file.
//
// Section 0 - loading and managing metadata
// Section 1 - FIT Parser associated methods and code
// Section 2 - RideFile entry point
// Section 3 - Write RideFile code
//

//
// Section 0 - Loading and managing metadata
//
//             All metadata is recored in FITmetadata.json, which is maintained via
//             the GoldenCheetah.org website and generated from the FIT sdk headers
//
//             Currently the metadata is mainly used to generate XDATA tabs for the
//             numerous message types in a FIT file (laps, sessions etc)
//

// metadata read from FITmetadata.json
QStringList FITbasetypes;
QList<FITproduct> FITproducts;
QList<FITmanufacturer> FITmanufacturers;
QList<FITmessage> FITmessages;
QList<FitFieldDefinition> FITstandardfields;

QStringList GenericDecodeList;

//Convert metadata typeids to fit ids
int typeToFIT(QString type)
{
    int id = FITbasetypes.indexOf(type);
    switch (id){
    case 1: //sint8
        return 1;
    case 2: //uint8
        return 2;
    case 3: //sint16
        return 131;
    case 4: //uint16
        return 132;
    case 5: //sint32
        return 133;
    case 6: //uint32
        return 134;
    case 8: //float32
        return 136;
    }
    return -1;
}

//Convert metadata typeids to fit lengths
int typeLength(QString type)
{
    int id = FITbasetypes.indexOf(type);
    switch (id){
    case 1: return 1;
    case 2: return 1;
    case 3: return 2;
    case 4: return 2;
    case 5: return 4;
    case 6: return 4;
    case 8: return 4;
    }
    return 0;
}

// load the FITmetadata file, called the first time a FIT file
// is parsed, so may not ever be called by the user
bool loaded=false;
static void loadMetadata()
{
    // only do it the first time
    if (loaded) return;
    loaded=true;

    // the metadata uses names for types not the value, these map to the values in fit.h
    // the index into the list is the type, so enum=0, sint8=1, uint8=2 and so on
    FITbasetypes << "enum" << "sint8" << "uint8" << "sint16" << "uint16" << "sint32" << "uint32" << "string"  << "float32"
                 << "float64" << "uint8z" << "uint16z" << "uint32z" << "byte" << "sint64" << "uint64" << "uint64z";

    // data is extracted generically using metadata and placed into XDATA, this is not performed
    // for all message types- we control the ones we want with this list:
    GenericDecodeList << "totals" << "session" << "lap";

    // lets try and download from the goldencheetah website- it makes
    // sense to do this here since we only get called when trying to
    // import a FIT file- which is only going to be post-activity not
    // every time the program is launched
    QNetworkAccessManager nam;
    QString content;
    QString localfilename = QDir(gcroot).canonicalPath()+"/FITmetadata.json";
    QString filename;

    // fetch from the goldencheetah.org website
    QString request = QString("%1/FITmetadata.json").arg(VERSION_CONFIG_PREFIX);

    QNetworkReply *reply = nam.get(QNetworkRequest(QUrl(request)));

    if (reply->error() == QNetworkReply::NoError) {

        // lets wait for a response with an event loop
        // it quits on a 5s timeout or reply coming back
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

        // 5 second timeout only hits hard for the very first FIT file
        // that is opened, which is during an import dialog, so not the end
        // of the world for folks who have disabled/slow internet connectivity
        timer.start(5000);

        // lets block until signal received
        loop.exec(QEventLoop::WaitForMoreEvents);

        // all good?
        if (reply->error() == QNetworkReply::NoError) {
            content = reply->readAll();

            // save away- will be read below as cached version
            QFile file(localfilename);
            if (file.open(QIODevice::Truncate|QIODevice::ReadWrite)) {
                file.write(content.toUtf8().constData());
                file.close();
            }
            if (FIT_DEBUG) { fprintf(stderr, "FITmetadata.json updated from online version\n"); fflush(stderr); }
        }
    }

    // get locally cached version- or fall back to baked in version
    // the baked in version will only be used by users that have
    // slow or disabled or no internet connection
    if (!QFile(localfilename).exists()) filename = ":/json/FITmetadata.json";
    else filename = localfilename;

    // read the file
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        content = file.readAll();
        file.close();
    } else return; // eek!

    // parse the content
    QJsonDocument metajson = QJsonDocument::fromJson(content.toUtf8());
    if (metajson.isEmpty() || metajson.isNull()) {
        fprintf(stderr, "Bad or missing FITmetadata.json\n"); fflush(stderr);
        goto badconfig;

    } else {

        // lets setup the structures
        QJsonObject root = metajson.object();

        if (!root.contains("PRODUCTS")) goto badconfig;
        QJsonArray PRODUCTS = root["PRODUCTS"].toArray();
        for(const QJsonValue &val : PRODUCTS) {

            // convert so we can inspect
            QJsonObject obj = val.toObject();

            FITproduct add;

            add.name = obj["name"].toString();
            add.prod = obj["prod"].toInt();
            add.manu = obj["manu"].toInt();

            FITproducts << add;
            if (FIT_DEBUG && FIT_DEBUG_LEVEL > 4) { fprintf(stderr, "FITprod: %d:%d %s\n", add.manu, add.prod, add.name.toStdString().c_str()); fflush(stderr); }
        }

        if (!root.contains("MANUFACTURERS")) goto badconfig;
        QJsonArray MANUFACTURERS = root["MANUFACTURERS"].toArray();
        for(const QJsonValue &val : MANUFACTURERS) {

            // convert so we can inspect
            QJsonObject obj = val.toObject();

            FITmanufacturer add;

            add.name = obj["name"].toString();
            add.manu = obj["manu"].toInt();

            FITmanufacturers << add;
            if (FIT_DEBUG && FIT_DEBUG_LEVEL > 4) { fprintf(stderr, "FITmanu: %d %s\n", add.manu, add.name.toStdString().c_str()); fflush(stderr); }
        }

        if (!root.contains("FIELDS")) goto badconfig;
        QJsonArray FIELDS = root["FIELDS"].toArray();
        for(const QJsonValue &val : FIELDS) {

            // Message:FieldNum lists
            //
            // remember field numbers are specific to a particular message- in the example below the
            // min_altitude value in a lap message has a field num of 62 and stored as an unsigned 16 bit
            // integer- scaled by 5, so to decode you need to divide by 5. Lastly it is in units "m" (meters)
            //
            // { "message":"lap","num":62,"name":"min_altitude","type":"uint16","scale":5,"offset":500,"units":"m" },

            // convert so we can inspect
            QJsonObject obj = val.toObject();

            FitFieldDefinition add;

            // we must have message section name, field number, field name and type for this to be useful
            // if we don't then it gets ignored, since this is the minimum information to decode and include in XDATA
            if (obj.contains("message") && obj.contains("num") && obj.contains("name") && obj.contains("type")) {

                // check the base type is supported
                QString type = obj["type"].toString();
                if (FITbasetypes.contains(type) || type == "date_time") {

                    add.dev_id = -1;
                    add.native = add.num = obj["num"].toInt();
                    add.message = obj["message"].toString().toStdString();
                    add.name = obj["name"].toString().toStdString();

                    if (type == "date_time") {
                        add.type = 6; // "uint32"
                        add.istime = true;
                    } else {
                        add.type = FITbasetypes.indexOf(type);
                        add.istime = false;
                    }
                    add.size = 0; // uses the value from the file.. for now

                    if (obj.contains("units")) add.unit = obj["unit"].toString().toStdString();
                    else add.unit="";

                    if (obj.contains("scale")) add.scale = obj["scale"].toString().toDouble();
                    else add.scale=-1;

                    if (obj.contains("offset")) add.offset = obj["offset"].toInt();
                    else add.offset=-1;

                    FITstandardfields << add;
                    if (FIT_DEBUG && FIT_DEBUG_LEVEL > 4) { fprintf(stderr, "Standard Field: [%s] %s (%s)\n", add.message.c_str(), add.name.c_str(), FITbasetypes[add.type].toStdString().c_str()); fflush(stderr); }
                }
            }
        }

        if (!root.contains("MESSAGES")) goto badconfig;
        QJsonArray MESSAGES = root["MESSAGES"].toArray();
        for(const QJsonValue &val : MESSAGES) {

            // convert so we can inspect
            QJsonObject obj = val.toObject();

            FITmessage add;

            add.desc = obj["desc"].toString();
            add.num = obj["num"].toInt();

            FITmessages << add;
            if (FIT_DEBUG && FIT_DEBUG_LEVEL > 4) { fprintf(stderr, "FITmessage: %d %s\n", add.num, add.desc.toStdString().c_str()); fflush(stderr); }
        }
        return;
    }

badconfig:
    { fprintf(stderr, "FITRideFile: FITmetadata.json parse error\n"); fflush(stderr); }
    return;
}

//
// Section 1 - The FIT Parser
//
//             The fit parser is managed via the FitFileParser structure
//             its a janky way of managing scope of functions and data and is
//             ripe for being refactored. For now we leave it untouched since it
//             is stable, if a little tedious to maintain. You will notice for
//             example that there is no FitFileParser header and all the methods
//             are defined and declared inline below. This makes navigation
//             harder and breaking into separate source files that little bit harder.
//
//             It is not clear why std c containers are used instead of Qt
//             ones; the use of std::string is quite annoying as it requires
//             conversion to align with the rest of the code. It is likely that
//             the original author was interested in learning about them rather
//             than writing code that aligned with the rest of the codebase.
//
//             The official FIT protocol docs are now hosted at Garmin and are
//             very useful, see: https://developer.garmin.com/fit/protocol/
//
//             We try to outline how the FIT protocol works below, but you may
//             get confused, 30 minutes perusing the URL above will save you a
//             lot of time if you are new to FIT files.
//
//             PARSING A FILE
//
//             A parser is instantiated with a filename, and is tied to that file
//             for the duration. A new parser is needed for each file parsed.
//             To start parsing the run() method is called
//
//             The run() method will read all *records* in a FIT file, there are
//             multiple record types; -definitions- these declare new types such as
//             developer fields and -data- these contain different types of data
//
//             The read_header() method is used to read the header record that must
//             exist at the top of every FIT file.
//
//             The read_record() method is used to read all the subsequent records
//             that follow the header and will extract the data within it. These are
//             either definition records or data records.
//
//             Definition records will create <FitFieldDefinitions>
//             whilst *data* records containg *messages* will create <FitMessage> structs
//
//             The <FitMessage> struct in turn contain a QList of <FitField> structs that
//             contain the data definitions whilst the value are stored stored in a QList
//             of <FitData> structs as extracted mechanically from the FIT file.
//
//             There are a wide range of message types from laps, sessions, events and so on.
//             Just to confuse you the message type 20 is called "record" and this is the message
//             that contains recorded data samples-- the bit we care about the most. Do not
//             confuse a "record message" that is decoded by the function decodeRecord() with
//             the FIT protocol top level records decoded by the function read_record() (!!!)
//
//             The last record in the file contains a CRC.
//
//             LOCAL MESSAGE TYPES vs GLOBAL MESSAGE TYPES
//
//             Every message containing a data payload will also be preceded at some point
//             in the file by a definition message that tells you how to decode it.
//
//             It is possible to read a FIT file without knowing anything about the FIT
//             profiles and the different message types. Just reading field level data.
//             But, of course, you won't know what the data means; just that it is called "x"
//             and contains the value y.
//
//             If you read the FIT docs you will see there are a number of message types
//             defined and they each have an associated number. The metadata loaded above
//             relates directly to this.
//
//             But when messages are placed into a FIT file they always have a LOCAL message
//             number- both the data record itself, and the definition record that tells you
//             how to read it.
//
//             To relate this back to the FIT profiles (so you get a clue what the fields
//             actually mean, the definition will define the local message number (that we
//             use to identify the record) and also a global message number (that we use
//             to relate back and process accordingly).
//
//             This means that although say a LAP message could have 50 fields in it, the
//             definition in the file says a LAP message is implemented locally with only
//             the 10 fields we have stored.
//
//             FITMESSAGE PROCESSING
//
//             As each <FitMessage> struct and QList of <FitField> is extracted by read_record()
//             it is passed to a decoding function that should iterate over the fields in the
//             message and add data to the rideFile being processed.
//
//             A generic decoder runs first and adds data to XDATA- the data it creates are not
//             used by standard GC code but are extracted for users to work with in user charts
//             and so on. It is entirely driven by the metadata loaded above. See:
//
//                    decodeGeneric()
//
//             After this not all of the standard messages are processed but the following are
//             processed and added to the rideFile being constructed, generally the data they
//             create IS used internally:
//
//                    decodeFileId()
//                    decodePhysiologicalMetrics()
//                    decodeSession()
//                    decodeDeviceInfo()
//                    decodeActivity()
//                    decodeEvent()
//                    decodeHRV()
//                    decodeLap()
//                    decodeRecord()
//                    decodeLength()
//                    decodeWeather()
//                    decodeHr()
//                    decodeDeviceSettings()
//                    decodeSegment()
//                    decodeUserProfile()
//                    decodeDeveloperID()
//                    decodeDeveloperFieldDescription()
//
//              Along the way there are large number of utility functions that
//              are used by the decode functions, these make up the rest of the
//              code within the parser
//
struct FitFileParser
{

    //
    // STATE INFORMATION
    //
    // Various state information, yes there is a crap ton of it. You will
    // see that across this source file there are a lot of aspects that have
    // grown organically as different people have worked on it.
    //
    // this is ripe for refactoring. *FIXME*
    //
    QFile &file;
    QStringList &errors;
    RideFile *rideFile;
    time_t start_time;
    time_t last_time;
    time_t last_reference_time;
    quint32 last_event_timestamp;
    double start_timestamp;
    double last_distance;
    QMap<int, FitMessage> local_msg_types;
    QMap<QString, FitFieldDefinition>  local_deve_fields; // All developer fields
    QMap<QString, FitDeveApp> local_deve_fields_app; // All developper apps
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
    double last_length;
    double last_RR;
    int last_event_type;
    int last_event;
    int last_msg_type;
    double last_altitude; // to avoid problems when records lacks altitude
    QVariant isGarminSmartRecording;
    QVariant GarminHWM;
    XDataSeries *weatherXdata;
    XDataSeries *gearsXdata;
    XDataSeries *positionXdata;
    XDataSeries *swimXdata;
    XDataSeries *deveXdata;
    XDataSeries *extraXdata;
    XDataSeries *hrvXdata;
    QMap<int, QString> deviceInfos, session_device_info_;
    QList<QString> dataInfos, session_data_info_;

    // cache tags for each session
    using SessionTagMap = QMap<QString, QVariant>;
    SessionTagMap active_session_;
    QList<SessionTagMap> session_tags_;
    QList<QMap<int, QString>> session_device_info_list_;
    QList<QList<QString>> session_data_info_list_;


    //
    // CONSTRUCTOR
    //
    // construct a parser passing the file by reference, any encountered
    // errors will go into the errors list also passed by reference
    //
    FitFileParser(QFile &file, QStringList &errors) :
        file(file), errors(errors), rideFile(NULL), start_time(0),
        last_time(0), last_distance(0.00f), interval(0), calibration(0),
        devices(0), stopped(true), isLapSwim(false), last_length(0.0),
        last_RR(0.0),
        last_event_type(-1), last_event(-1), last_msg_type(-1),
        last_altitude(0.0)
    {}


    // yay, lets crash the whole fucking program if the reader
    // has problems. sheesh. this is definitely a *FIXME*
    struct TruncatedRead {};
    void read_unknown( int size, int *count = NULL ) {
        if (!file.seek(file.pos() + size))
            throw TruncatedRead();
        if (count)
            (*count) += size;
    }

    //
    // FIT DATA TYPE READERS
    //
    // The FIT protocol is very focused on strongly typed data
    // that must be read/written quite particularly and the
    // names used uint16z etc are referred to as the FIT
    // base types across the FIT protocol documentation
    //
    // Ultimately all data is represented using these base
    // types, but the docs also uses semantic types to
    // indicate the type of data being stored. For example
    // timestamps are date_time types in the docs but
    // they map to the uint32 base type.
    //
    // This section of the code contains all the readers
    //              read_text
    //              read_uint8
    //              read_byte
    //              read_uint8z
    //              read_int16
    //              read_uint16
    //              read_uint16z
    //              read_int32
    //              read_uint32
    //              read_uint32z
    //
    // There are 16 base types and not all are represented
    // in our code, this should be fixed (e.g. floats)
    //
    // Additionally there are some functions here for
    // working with semantic types:
    //
    //              getSport
    //              getSportId
    //              getSubSport
    //              getSubSportId
    //

    // base types

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

    fit_value_t read_byte(int *count = NULL) {
        quint8 i;
        if (file.read(reinterpret_cast<char*>( &i), 1) != 1)
            throw TruncatedRead();
        if (count)
            (*count) += 1;

        return i;
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

    fit_float_value read_float32(bool is_big_endian, int *count = NULL) {
        float f;
        if (file.read(reinterpret_cast<char*>(&f), 4) != 4)
            throw TruncatedRead();
        if (count)
            (*count) += 4;

        if (is_big_endian) {
            f = qbswap(f);
        }

        return f;
    }

    // semantic types

    static QString getSport(quint8 sport_id) {
        switch (sport_id) {
            case 0: // Generic
                return "";
                break;
            case 1: // running:
                return "Run";
                break;
            case 2: // cycling
                return "Bike";
                break;
            case 3: // transition:
                return "Transition";
                break;
            case 4:
                return "Fitness equipment";
                break;
            case 5: // swimming
                return "Swim";
                break;
            case 6: // Basketball:
                return "Basketball";
                break;
            case 7:
                return "Soccer";
                break;
            case 8:
                return "Tennis";
                break;
            case 9:
                return "American fotball";
                break;
            case 10:
                return "Training";
                break;
            case 11:
                return "Walking";
                break;
            case 12:
                return "Cross country skiing";
                break;
            case 13:
                return "Alpine skiing";
                break;
            case 14:
                return "Snowboarding";
                break;
            case 15:
                return "Rowing";
                break;
            case 16:
                return "Mountaineering";
                break;
            case 17:
                return "Hiking";
                break;
            case 18:
                return "Multisport";
                break;
            case 19:
                return "Paddling";
                break;
            case 20: // flying
                return "flying";
                break;
            case 21: // e biking
                return "e-biking";
                break;
            case 22: // motorcycling
                return "motorcycling";
                break;
            case 23: // boating
                return "boating";
                break;
            case 24: // driving
                return "driving";
                break;
            case 25: // golf
                return "golf";
                break;
            case 26: // hang gliding
                return "hang gliding";
                break;
            case 27: // horseback riding
                return "horseback riding";
                break;
            case 28: // hunting
                return "hunting";
                break;
            case 29: // fishing
                return "fishing";
                break;
            case 30: // inline skating
                return "inline skating";
                break;
            case 31: // rock climbing
                return "rock climbing";
                break;
            case 32: // sailing
                return "sailing";
                break;
            case 33: // ice skating
                return "ice skating";
                break;
            case 34: // sky diving
                return "sky diving";
                break;
            case 35: // snowshoeing
                return "snowshoeing";
                break;
            case 36: // snowmobiling
                return "snowmobiling";
                break;
            case 37: // stand up paddleboarding
                return "stand up paddleboarding";
                break;
            case 38: // surfing
                return "surfing";
                break;
            case 39: // wakeboarding
                return "wakeboarding";
                break;
            case 40: // water skiing
                return "water skiing";
                break;
            case 41: // kayaking
                return "kayaking";
                break;
            case 42: // rafting
                return "rafting";
                break;
            case 43: // windsurfing
                return "windsurfing";
                break;
            case 44: // kitesurfing
                return "kitesurfing";
                break;
            case 45: // tactical
                return "tactical";
                break;
            case 46: // jumpmaster
                return "jumpmaster";
                break;
            case 47: // boxing
                return "boxing";
                break;
            case 48: // floor climbing
                return "floor climbing";
                break;
            case 49: // baseball
                return "baseball";
                break;
            case 50: // softball fast pitch
                return "softball fast pitch";
                break;
            case 51: // softball slow pitch
                return "softball slow pitch";
                break;
            case 56: // shooting
                return "shooting";
                break;
            case 57: // auto racing
                return "auto racing";
                break;
            case 58: // winter sport
                return "winter sport";
                break;
            case 59: // grinding
                return "grinding";
                break;
            case 60: // health monitoring
                return "health monitoring";
                break;
            case 61: // marine
                return "marine";
                break;
            case 62: // hiit
                return "hiit";
                break;
            case 63: // video gaming
                return "video gaming";
                break;
            case 64: // racket
                return "racket";
                break;
            case 65: // wheelchair push walk
                return "wheelchair push walk";
                break;
            case 66: // wheelchair push run
                return "wheelchair push run";
                break;
            case 67: // meditation
                return "meditation";
                break;
            case 68: // para sport
                return "para sport";
                break;
            case 69: // disc golf
                return "disc golf";
                break;
            case 70: // team sport
                return "team sport";
                break;
            case 71: // cricket
                return "cricket";
                break;
            case 72: // rugby
                return "rugby";
                break;
            case 73: // hockey
                return "hockey";
                break;
            case 74: // lacrosse
                return "lacrosse";
                break;
            case 75: // volleyball
                return "volleyball";
                break;
            case 76: // water tubing
                return "water tubing";
                break;
            case 77: // wakesurfing
                return "wakesurfing";
                break;
            default: // if we can't work it out, treat as Generic
                return ""; break;
        }
    }

    static quint8 getSportId(QString sport_descr) {
        if (sport_descr=="") {
            return 0;
        }
        for (quint16 i=0; i<=255; i++) {
            if (sport_descr.compare(FitFileParser::getSport(i), Qt::CaseInsensitive)==0) {
                return (quint8) i;
            }
        }
        return 0; // when not found return generic sport id
    }

    static QString getSubSport(quint8 subsport_id) {
        switch (subsport_id) {
            case 0:    // generic
                return "";
                break;
            case 1:    // treadmill
                return "treadmill";
                break;
            case 2:    // street
                return "street";
                break;
            case 3:    // trail
                return "trail";
                break;
            case 4:    // track
                return "track";
                break;
            case 5:    // spin
                return "spinning";
                break;
            case 6:    // home trainer
                return "home trainer";
                break;
            case 7:    // route
                return "route";
                break;
            case 8:    // mountain
                return "mountain";
                break;
            case 9:    // downhill
                return "downhill";
                break;
            case 10:    // recumbent
                return "recumbent";
                break;
            case 11:    // cyclocross
                return "cyclocross";
                break;
            case 12:    // hand_cycling
                return "hand cycling";
                break;
            case 13:    // piste
                return "piste";
                break;
            case 14:    // indoor_rowing
                return "indoor rowing";
                break;
            case 15:    // elliptical
                return "elliptical";
                break;
            case 16: // stair climbing
                return "stair climbing";
                break;
            case 17: // lap swimming
                return "lap swimming";
                break;
            case 18: // open water
                return "open water";
                break;
            case 19: // flexibility training
                return "flexibility training";
                break;
            case 20: // strength_training
                return "strength_training";
                break;
            case 21: // warm_up
                return "warm_up";
                break;
            case 22: // match
                return "match";
                break;
            case 23: // exercise
                return "exercise";
                break;
            case 24: // challenge
                return "challenge";
                break;
            case 25: // indoor_skiing
                return "indoor_skiing";
                break;
            case 26: // cardio_training
                return "cardio_training";
                break;
            case 27: // indoor_walking
                return "indoor_walking";
                break;
            case 28: // e_bike_fitness
                return "e_bike_fitness";
                break;
            case 29: // bmx
                return "bmx";
                break;
            case 30: // casual_walking
                return "casual_walking";
                break;
            case 31: // speed_walking
                return "speed_walking";
                break;
            case 32: // bike_to_run_transition
                return "bike_to_run_transition";
                break;
            case 33: // run_to_bike_transition
                return "run_to_bike_transition";
                break;
            case 34: // swim_to_bike_transition
                return "swim_to_bike_transition";
                break;
            case 35: // atv
                return "atv";
                break;
            case 36: // motocross
                return "motocross";
                break;
            case 37: // backcountry
                return "backcountry";
                break;
            case 38: // resort
                return "resort";
                break;
            case 39: // rc_drone
                return "rc_drone";
                break;
            case 40: // wingsuit
                return "wingsuit";
                break;
            case 41: // whitewater
                return "whitewater";
                break;
            case 42: // skate skiing
                return "skate skiing";
                break;
            case 43: // yoga
                return "yoga";
                break;
            case 44: // pilates
                return "pilates";
                break;
            case 45: // indoor running
                return "indoor running";
                break;
            case 46: // gravel cycling
                return "gravel cycling";
                break;
            case 47: // e bike mountain
                return "e-bike mountain";
                break;
            case 48: // commuting
                return "commuting";
                break;
            case 49: // mixed surface
                return "mixed surface";
                break;
            case 50: // navigate
                return "navigate";
                break;
            case 51: // track me
                return "track me";
                break;
            case 52: // map
                return "map";
                break;
            case 53: // single gas diving
                return "single gas diving";
                break;
            case 54: // multi gas diving
                return "multi gas diving";
                break;
            case 55: // gauge diving
                return "gauge diving";
                break;
            case 56: // apnea diving
                return "apnea diving";
                break;
            case 57: // apnea hunting
                return "apnea hunting";
                break;
            case 58: // virtual activity
                return "virtual activity";
                break;
            case 59: // obstacle
                return "obstacle";
                break;
            case 60: // assistance
                return "assistance";
                break;
            case 61: // incident detected
                return "incident detected";
                break;
            case 62: // breathing
                return "breathing";
                break;
            case 63: // ccr diving
                return "ccr diving";
                break;
            case 64: // area calc
                return "area calc";
                break;
            case 65: // sail race
                return "sail race";
                break;
            case 66: // expedition
                return "expedition";
                break;
            case 67: // ultra
                return "ultra";
                break;
            case 68: // indoor climbing
                return "indoor climbing";
                break;
            case 69: // bouldering
                return "bouldering";
                break;
            case 70: // hiit
                return "hiit";
                break;
            case 71: // indoor grinding
                return "indoor grinding";
                break;
            case 72: // hunting with dogs
                return "hunting with dogs";
                break;
            case 73: // amrap
                return "amrap";
                break;
            case 74: // emom
                return "emom";
                break;
            case 75: // tabata
                return "tabata";
                break;
            case 76: // fall detected
                return "fall detected";
                break;
            case 77: // esport
                return "esport";
                break;
            case 78: // triathlon
                return "triathlon";
                break;
            case 79: // duathlon
                return "duathlon";
                break;
            case 80: // brick
                return "brick";
                break;
            case 81: // swim run
                return "swim run";
                break;
            case 82: // adventure race
                return "adventure race";
                break;
            case 83: // trucker workout
                return "trucker workout";
                break;
            case 84: // pickleball
                return "pickleball";
                break;
            case 85: // padel
                return "padel";
                break;
            case 86: // indoor wheelchair walk
                return "indoor wheelchair walk";
                break;
            case 87: // indoor wheelchair run
                return "indoor wheelchair run";
                break;
            case 88: // indoor hand cycling
                return "indoor hand cycling";
                break;
            case 89: // anchor
                return "anchor";
                break;
            case 90: // field
                return "field";
                break;
            case 91: // ice
                return "ice";
                break;
            case 92: // ultimate
                return "ultimate";
                break;
            case 93: // platform
                return "platform";
                break;
            case 94: // squash
                return "squash";
                break;
            case 95: // badminton
                return "badminton";
                break;
            case 96: // racquetball
                return "racquetball";
                break;
            case 97: // table tennis
                return "table tennis";
                break;
            case 254: // all
            default:    // default, treat as Generic
                return "";
                break;
        }
    }

    static quint8 getSubSportId(QString subsport_descr) {
        if (subsport_descr=="") {
            return 0;
        }
        for (quint16 i=0; i<=255; i++) {
            if (subsport_descr.compare(FitFileParser::getSubSport(i), Qt::CaseInsensitive)==0) {
                return (quint8) i;
            }
        }
        // qDebug() << "getSubSportId(QString subsport_descr): subsport_descr was not found";
        return 0; // when not found return generic sport id
    }

    //
    // A number of utility and helper functions
    //
    // There is no particular rationale or pattern behind
    // these utilities and helpers they have increased in
    // number as different folks have worked on the code
    // and needed re-usable elements.
    //
    // Again, these are ripe for refactoring. *FIXME*
    //
    void DumpFitValue(const FitValue& v) {
        if (FIT_DEBUG && FIT_DEBUG_LEVEL>4) fprintf(stderr, "type: %d %llx %s\n", v.type, v.v, v.s.c_str());
    }

    void convert2Run( RideFile *rf) {
        if (rf->areDataPresent()->cad) {
            foreach(RideFilePoint *pt, rf->dataPoints()) {
                pt->rcad = pt->cad;
                pt->cad = 0;
            }
            rf->setDataPresent(RideFile::rcad, true);
            rf->setDataPresent(RideFile::cad, false);
        }
    }

    std::string fitTypeDesc(int type)
    {
        if (type >= 0 && type < FITbasetypes.count()) return FITbasetypes[type].toStdString();
        else return "Enumerated type";
    }

    std::string fitDeveloperFieldDescription(int idx, int num, QString &key)
    {
        key = QString("%1.%2").arg(idx).arg(num);
        if (local_deve_fields.contains(key))
            return local_deve_fields.value(key).name.c_str();
        key=""; // to indicate not found
        return "Unknown developer field";

    }

    // the field numbers are message specific so need to pass
    // the message name and field number to get the description
    std::string fitStandardFieldDescription(QString message, int num, int &idx)
    {
        int index=0;
        foreach(FitFieldDefinition x, FITstandardfields) {
            if (x.message == message.toStdString() && x.num == num) {
                idx=index;
                return x.name;
            }
            index++;
        }
        idx=-1;
        return "Unknown standard field";
    }

    QString fitBaseTypeDesc(int type)
    {
        if (type>=0 && type < FITbasetypes.count()) return FITbasetypes[type];
        return "Unknown";
    }

    QString fitMessageDesc(int message, bool lower)
    {
        QString returning = "Unknown message type";
        foreach(FITmessage x, FITmessages) {
            if (x.num == message) {
                returning= x.desc;
                break;
            }
        }
        if (lower) {
            returning=returning.toLower();
            returning=returning.replace(" ", "_");
        }
        return returning;
    }

    // return a name for the manufacturer, product combintion
    // uses the config from FITmetadata.json initialised above
    QString getManuProd(int manu, int prod) {

        QString returning;

        // is it a known or defaulted product?
        foreach(FITproduct x, FITproducts) {

            if (x.manu == manu && x.prod == prod) {
                // garmin devices special case (could also fix in FITmetadata.json)
                // we have been inconsistent on this in the past e.g. Powertap, not SARIS Powertap
                if (manu == 1) return QString("Garmin %1").arg(x.name);
                else return x.name;
            }
            if (x.manu == manu && x.prod == -1) returning = x.name;
        }
        if (returning != "") return returning;

        // ok, then lets just return manufacturer and prod number
        foreach(FITmanufacturer x, FITmanufacturers) {
            if (x.manu == manu) return QString ("%1 Device (%2)").arg(x.name).arg(prod);
        }

        // don't even recognise the manufacturer!
        return QString("Unknown FIT Device %1:%2").arg(manu).arg(prod);
    }

    QString getDeviceType(int device_type) {
        switch (device_type) {
            case 4: return "Headunit"; // bike_power
            case 11: return "Powermeter"; // bike_power
            case 12: case 25: return "Environment Sensor"; // s.t. temperature
            case 16: return "Remote Control"; // s.t. Edge remote
            case 17: return "Biketrainer"; // fitness equipment
            case 18: return "Blood Presure";
            case 30: return "Running Dynamics";
            case 31: return "Muscle Oxygen";
            case 34: return "Shifting";
            case 35: case 36: return "Bikelight"; // bike_light_main/shared
            case 40: return "Bike Radar"; // bike_radar
            case 46: return "Bike Aero"; // bike_aero
            case 120: return "HR"; // heart_rate
            case 121: return "Speed-Cadence"; // bike_speed_cadence
            case 122: return "Cadence"; // bike_speed
            case 123: return "Speed"; // bike_speed
            case 124: return "Stride"; // stride_speed_distance
            case 127: return "CoreTemp";

            default: return QString("Type %1").arg(device_type);
        }
    }

    QString getBatteryStatus(quint8 battery_status) {
        switch (battery_status) {
            case  1: return "new";
            case  2: return "good";
            case  3: return "ok";
            case  4: return "low";
            case  5: return "critical";
            case  6: return "charging";
            default: return "unknown";
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
            case 30: // LEFT_RIGHT_BALANCE
                    return RideFile::lrbalance;
            case 39: // VERTICAL OSCILLATION
                    return RideFile::rvert;
            case 41: // GROUND CONTACT TIME
                    return RideFile::rcontact;
            case 45: // LEFT_PEDAL_SMOOTHNESS
                    return RideFile::lps;
            case 46: // RIGHT_PEDAL_SMOOTHNESS
                    return RideFile::rps;
            //case 47: // COMBINED_PEDAL_SMOOTHNES
            //        return RideFile::cps;
            case 54: // THb
                    return RideFile::thb;
            case 57: // SMO2
                    return RideFile::smo2;

            case 253: // SECS
                return RideFile::secs;
            case 139: // CoreTemp
                return RideFile::tcore;
            default:
                return RideFile::none;
        }
    }

    QString getNameForExtraNative(int native_num) {
        switch (native_num) {

            case 32: // VERTICAL_SPEED
                    return "VERTICALSPEED"; // Vertical Speed

            case 40: // STANCE_TIME_PERCENT
                    return "STANCETIMEPERCENT"; // Stance Time Percent

            case 42: // ACTIVITY_TYPE
                    return "ACTIVITYTYPE"; // Activity Type

            case 47: // COMBINED_PEDAL_SMOOTHNES
                    return "COMBINEDSMOOTHNESS"; //Combined Pedal Smoothness

            case 81: // BATTERY_SOC
                    return "BATTERYSOC";

            case 83: // VERTICAL_RATIO
                    return "VERTICALRATIO"; // Vertical Ratio

            case 85: // STEP_LENGTH
                    return "STEPLENGTH"; // Step Length

            case 87: // CYCLE_LENGTH
                return "CYCLELENGTH"; // Cycle Length (Rowing, paddle)

            case 90: // PERFORMANCE_CONDITION
                    return "PERFORMANCECONDITION"; // Performance Contition

            case 108: // RESPIRATIONRATE
                return "RESPIRATIONRATE";

            case 114: // MTB Dynamics - Grit
                return "GRIT";

            case 115: // MTB Dynamics - Flow
                return "FLOW";

            case 116: // Stress
                return "STRESS";

            case 133: // Pulse Ox
                return "PULSEOX";

            case 136: // Wrist HR
                return "WRISTHR";

            case 137: // Potential Stamina
                return "POTENTIALSTAMINA";

            case 138: // Stamina
                return "STAMINA";

            case 139: // CoreTemp
                return "TCORE";

            case 140: // Gap
                return "GAP";

            case 143: // Body Battery
                return "BODYBATTERY";

            case 144: // External HR
                return "EXTERNALHR";

            default:
                return QString("FIELD_%1").arg(native_num);
        }
    }

    float getScaleForExtraNative(int native_num) {
        switch (native_num) {

            case 32: // VERTICAL_SPEED
            case 140: // GAP
                    return 1000.0;

            case 40: // STANCE_TIME_PERCENT
            case 83: // VERTICAL_RATIO
                    return 100.0;

            case 85: // STEP_LENGTH
                    return 10.0;

            case 87: // CYCLE_LENGTH
                return 100.0;

            case 47: // COMBINED_PEDAL_SMOOTHNES
            case 81: // BATTERY_SOC
                    return 2.0;

            case 108: // RESPIRATIONRATE
            case 116: // Stress
            case 139: // CoreTemp
                return 100.0;

            default:
                return 1.0;
        }
    }

    int getOffsetForExtraNative(int native_num) {
        switch (native_num) {
            case 0:
                return 0;
            default:
                return 0;
        }
    }

    void addRecordDeveField(QString key, FitFieldDefinition deveField, bool xdata) {
        QString name = deveField.name.c_str();

        //If field has a native type and doesnt have a name, generate one
        if (deveField.native>-1 && name.length()==0 ) {
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

            const QString contain_info = QString("CIQ '%1' -> %2 %3").arg(deveField.name.c_str()).arg("STANDARD").arg(nativeName);
            if (xdata && dataInfos.contains(contain_info)) {
                int secs = last_time-start_time;
                int idx = dataInfos.indexOf(contain_info);
                const QString info_replacement = QString("CIQ '%1' -> %2 %3 (STANDARD until %4 secs)").arg(deveField.name.c_str()).arg(typeName).arg(name).arg(secs);
                dataInfos.replace(idx, info_replacement);
                if (session_data_info_.contains(contain_info))
                    session_data_info_.replace(session_data_info_.indexOf(contain_info), info_replacement);
            } else {
                const QString the_info = QString("CIQ '%1' -> %2 %3").arg(deveField.name.c_str()).arg(typeName).arg(name);
                dataInfos.append(the_info);
                session_data_info_.append(the_info);
            }
        } else {
            int i = 0;
            do {
                i++;
                name = deveField.name.c_str() + (i>1?QString("-%1").arg(i):"");
            }
            while (deveXdata->valuename.contains(name));
        }

        if (xdata) {
            deveXdata->valuename << name;
            deveXdata->unitname << deveField.unit.c_str();

            record_deve_fields.insert(key, deveXdata->valuename.count()-1);
        } else
            record_deve_fields.insert(key, -1);

        // Add field for app
        QString appKey = QString("%1").arg(deveField.dev_id);
        if (local_deve_fields_app.contains(appKey)) {
            local_deve_fields_app[appKey].fields.append(deveField);
        }
    }

    void setRideFileDeviceInfo(RideFile *rf, QMap<int, QString> const& device_infos) {
        QStringList uniqueDevices(device_infos.values());
        uniqueDevices.removeDuplicates();
        QString deviceInfo = uniqueDevices.join("\n");
        if (! deviceInfo.isEmpty()) {
            rf->setTag("Device Info", deviceInfo);
        }
    }

    void setRideFileDataInfo(RideFile *rf, QList<QString> const& data_infos) {
        QString dataInfo;
        foreach(QString info, data_infos) {
            dataInfo += info + "\n";
        }
        if (dataInfo.length()>0) {
            rf->setTag("Data Info", dataInfo);
        }
    }

    void appendXData(RideFile *rf) {
        if (rf == nullptr) { return; }

        if (!weatherXdata->datapoints.empty())
            rf->addXData("WEATHER", weatherXdata);
        else
            delete weatherXdata;

        if (!swimXdata->datapoints.empty())
            rf->addXData("SWIM", swimXdata);
        else
            delete swimXdata;

        if (!hrvXdata->datapoints.empty())
            rf->addXData("HRV", hrvXdata);
        else
            delete hrvXdata;

        if (!gearsXdata->datapoints.empty())
            rf->addXData("GEARS", gearsXdata);
        else
            delete gearsXdata;

        if (!positionXdata->datapoints.empty())
            rf->addXData("POSITION", positionXdata);
        else
            delete positionXdata;

        if (!deveXdata->datapoints.empty())
            rf->addXData("DEVELOPER", deveXdata);
        else
            delete deveXdata;

        if (!extraXdata->datapoints.empty())
            rf->addXData("EXTRA", extraXdata);
        else
            delete extraXdata;
    }

    RideFile *splitSessions(QList<RideFile*> *rides) {
        // NOTES:
        // - start altitude of first transition not correct (zero), leads to too high climb figure
        //   i think this is not really a big deal yet

        // check multiple rides are expected and available, otherwise return just the whole activity
        if (!rides || session_tags_.size() < 2) {
            // just check if it was a run activity and adjust values, like it was done
            // in decoding the session before.
            if (rideFile->isRun()) {
                convert2Run(rideFile);
            }
            return rideFile;
        }

        // If there is more than one session parsed we
        // split the ride file just created into multiple
        // ones, each representing a single session.
        // BUT, we do not touch the original file.

        quint32 start = 0, start_time = 0;
        const QString deviceType = rideFile->deviceType();
        const QString fileFormat = rideFile->fileFormat();
        const double recIntSecs = rideFile->recIntSecs();
        // start date of an activity in ISO format is guaranteed to be unique in GC, see discussion in MR.
        const QString uniqueSessionCookie = rideFile->startTime().toUTC().toString(Qt::ISODate);
        QString sport_before;

        int session_idx = 0;
        for(auto s = session_tags_.begin(); s != session_tags_.end(); ++s, ++session_idx) {
            RideFile *rf = new RideFile;
            rf->setDeviceType(deviceType);
            rf->setFileFormat(fileFormat);
            rf->setRecIntSecs(recIntSecs);

            // each ride gets a unique cookie to allow filtering/grouping all sessions that belong together
            rf->setTag("Multisport", uniqueSessionCookie);

            // set tags and filter out session meta data
            quint32 stop = 0;
            for(auto sle = s->begin(); sle != s->end(); ++sle) {
                QString const& key = sle.key();
                // meta data is prefixed w/ '_', otherwise its a ride file tag
                if (key.startsWith('_')) {
                    if (0 == QString::compare(key, "_stop_time", Qt::CaseInsensitive)) {
                        stop = sle->toUInt();
                    } else if (start == 0 && 0 == QString::compare(key, "_start_time", Qt::CaseInsensitive)) {
                        // only do once
                        start = sle->toUInt();
                        start_time = start;
                    }
                } else {
                    rf->setTag(key, sle->toString());
                }
            }

            // adjust the start time
            rf->setStartTime(QDateTime::fromSecsSinceEpoch(start, QTimeZone::UTC));

            int idx_start = rideFile->timeIndex(start - start_time);
            int idx_stop = rideFile->timeIndex(stop - start_time);
            // add data points to the new file created.
            const QVector<RideFilePoint*> points_from_file = rideFile->dataPoints().mid(idx_start, idx_stop - idx_start);
            foreach(RideFilePoint *p, points_from_file) {
                rf->appendPoint(*p);
            }

            // fix subsport tag of transitions
            if (0 == QString::compare(rf->getTag("Sport", ""), "Transition", Qt::CaseInsensitive)) {
                QString sport_after;
                auto s_cpy = s + 1;
                if (s_cpy != session_tags_.end() && s_cpy->contains("Sport") && s_cpy->value("SubSport").toString().isEmpty()) {
                    sport_after = s_cpy->value("Sport").toString();
                }
                if (!sport_before.isEmpty() && !sport_after.isEmpty()) {
                     rf->setTag("SubSport", (QString("%1_to_%2_transition").arg(sport_before.toLower(), sport_after.toLower())));
                }
            }

            // add intervals
            //   Only intervals that fit in the range of the file we are recently creating
            //   are transferred.
            auto ride_intervals = rideFile->intervals();
            int int_ctr = 1;
            foreach (RideFileInterval *rfi, ride_intervals) {
                // is the recent interval in the range of our new file?
                if (rfi->start >= idx_start && rfi->start <= idx_stop && rfi->stop <= idx_stop) {   // is it possible that the a start is beyond stop and stop fits?
                    rf->addInterval(rfi->type, rfi->start, rfi->stop, QString("Lap %1").arg(int_ctr));
                    int_ctr++;
                }
            }

            // add XData
            const auto file_xdata = rideFile->xdata();
            for (auto it = file_xdata.begin(); it != file_xdata.end(); ++it) {
                XDataSeries *s = new XDataSeries(*(it.value()));
                s->datapoints.erase(std::remove_if(s->datapoints.begin(), s->datapoints.end(), [start_time, start, stop](XDataPoint *xdp){
                    return xdp && (xdp->secs >= (stop - start_time) || xdp->secs < (start - start_time));
                }), s->datapoints.end());
                // adjust their timing
                if (!s->datapoints.empty()) {
                    auto first_time = s->datapoints.first()->secs;
                    foreach (XDataPoint *p, s->datapoints) {
                        p->secs -= first_time;
                    }
                    // and append
                    rf->addXData(it.key(), s);
                }
            }

            // convert if necessary (run and transition)
            if (rf->isRun() || 0 == QString::compare(rf->getTag("Sport", ""), "Transition", Qt::CaseInsensitive)) {
                convert2Run(rf);
            }

            setRideFileDataInfo(rf, session_data_info_list_.at(session_idx));
            setRideFileDeviceInfo(rf, session_device_info_list_.at(session_idx));

            rides->append(rf);

            // remember sport tag
            sport_before = rf->getTag("Sport", "");

            // adjust timing and go on
            start = stop;
        }

        // return original file
        return rideFile;
    }

    //
    // FITMESSAGE DECODERS
    //
    // These form the majority of this code, running for the next 2k lines or so.
    //
    // Message decoding functions work with FitMessages and the decoded fields
    // passed as vectors of FitValues
    //
    //                    decodeGeneric()
    //
    //                    decodeFileId()
    //                    decodePhysiologicalMetrics()
    //                    decodeSession()
    //                    decodeDeviceInfo()
    //                    decodeActivity()
    //                    decodeEvent()
    //                    decodeHRV()
    //                    decodeLap()
    //                    decodeRecord()
    //                    decodeLength()
    //                    decodeWeather()
    //                    decodeHr()
    //                    decodeDeviceSettings()
    //                    decodeSegment()
    //                    decodeUserProfile()
    //                    decodeDeveloperID()
    //                    decodeDeveloperFieldDescription()
    //
    //

    // general purpose decoding of message via metadata and placing
    // the contents into an XDATA tab
    void decodeGeneric(QString message, const FitMessage &def, int time_offset,
                       const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);

        // we don't do it for all messages, it would get out of hand!
        if (!GenericDecodeList.contains(message)) return;

        // let those debuggers know we're doing a generic decode
        if (FIT_DEBUG && FIT_DEBUG_LEVEL>1) fprintf(stderr, "generic decode %s: %d fields to decode\n",
                                                            message.toStdString().c_str(), (int)values.size());

        // we look for a timestamp field as we go through
        double timestamp = -1;
        int index=0;
        int count=0;

        // lets add to XDATA if it is not already there
        // for example TOTAL and SESSION messages only occur once in general so will never exist
        // but LAP messages will be multiple, so we should reuse if we added it earlier
        int seriesindex=-1;
        XDataSeries *xdseries = rideFile->xdata(message.toUpper());
        if (xdseries == NULL) {
            xdseries = new XDataSeries();
            xdseries->name = message.toUpper();

            rideFile->addXData(message.toUpper(), xdseries);
        }

        // this point will be added for this message
        // with values appended as we go through the data below
        // and ultimately the secs value is derived at the end
        // before being added to the XDATA
        XDataPoint *add= new XDataPoint();

        for(const FitField &field : def.fields) {

            QString name;
            double value=0;
            double scaledvalue=0; //value after scaling is applied
            int idx; // standard field index
            QString key; // developer field key
            FitFieldDefinition metadata;

            // XDATA does not really support string data, but the code
            // is misleading on this, it should be fixed in the XDATA code *FIXME*
            if (field.type == 7) goto genericnext; // can't handle this

            switch(field.type) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                    // use the integer value v
                    if (values[index].v == NA_VALUE) goto genericnext;
                    value= values[index].v;
                    break;
            case 8:
                    // use the float value f
                    if (values[index].f == NA_VALUEF) goto genericnext;
                    value=values[index].f;
                    break;
            default:
                    if (values[index].v == NA_VALUE) goto genericnext;
                    value=values[index].v; // most likely drop through as type is unreliable
                break;
            }

            if (field.deve_idx == -1) {

                // find the metadata for a standard field
                name = fitStandardFieldDescription(message, field.num, idx).c_str(); // standard field
                if (idx < 0) goto genericnext; // not a field we support yet
                metadata = FITstandardfields[idx];

            } else if (field.deve_idx >=0) {

                // find the metadata for a developer field
                name = fitDeveloperFieldDescription(field.deve_idx, field.num, key).c_str();
                if (key == "") goto genericnext; // not found as a developer field, so skip
                metadata = local_deve_fields.value(key);

            }

            // if we get here we have metadata about the field in question
            // so lets apply scaling and add to the XDATA section
            scaledvalue = value;
            if (metadata.scale > 0) scaledvalue /= double(metadata.scale);

            // if we get here then the field is going to be transferred into
            // the XDATA section, so lets see if it is a start_time field?
            if (strcmp(metadata.name.c_str(), "start_time") == 0)  timestamp = value;

            // add the xdata series if not present yet, guarding against hitting the maximum
            if (xdseries->valuename.count() < XDATA_MAXVALUES) {
                seriesindex=xdseries->valuename.indexOf(metadata.name.c_str());
                if (seriesindex == -1) {
                    xdseries->valuename.append(metadata.name.c_str());
                    xdseries->unitname.append(metadata.unit.c_str());
                    xdseries->valuetype.append(RideFile::SeriesType::none); // makes no sense, if it was a series type it wouldn't be xdata
                    seriesindex=xdseries->valuename.indexOf(metadata.name.c_str());
                }
                add->number[seriesindex] = scaledvalue;
                count++;
            }

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>3) {
                fprintf(stderr, "decodeGeneric(%s) %d: %s field %d bytes, num %d, type %s [%s]=%f %s\n",
                    message.toStdString().c_str(),
                    index,
                    (field.deve_idx > -1 ? "developer" : "standard"), field.size,
                    field.num, fitTypeDesc(metadata.type).c_str(),
                    name.toStdString().c_str(),
                    scaledvalue, metadata.unit.c_str());
            }

genericnext:
            index++;
        }

        // what timestamp to use for this row if XDATA ?
        if (timestamp <= 0) timestamp=0;
        QDateTime time= qbase_time.addSecs(timestamp);
        QDateTime start= QDateTime::fromSecsSinceEpoch(start_time);
        if (timestamp <= 0 || time < start) time=start;

        // update the xdata series with the timestamp and add for our record
        // if we managed to extract any data from the file successfully
        if (count > 0) {
            add->secs = (float)(time.toUTC().toMSecsSinceEpoch() -  start.toUTC().toMSecsSinceEpoch()) / 1000.00;
            add->km = 0;
            xdseries->datapoints.append(add);
        }
    }

    void decodeFileId(const FitMessage &def, int,
                      const std::vector<FitValue>& values) {
        int i = 0;
        int manu = -1, prod = -1;
        for(const FitField &field : def.fields) {
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
        active_session_["_devicetype"] = getManuProd(manu, prod);
        rideFile->setDeviceType(getManuProd(manu, prod));
    }


    void decodePhysiologicalMetrics(const FitMessage &def, int,
                                    const std::vector<FitValue>& values) {
        int i = 0;

        for(const FitField &field : def.fields) {
            fit_value_t value = values[i++].v;


            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 7:   // METmax: 1 METmax = VO2max * 3.5, scale 65536
                    active_session_["VO2max detected"] = QString::number(round(value / 65536.0 * 3.5 * 10.0) / 10.0);
                    rideFile->setTag("VO2max detected", QString::number(round(value / 65536.0 * 3.5 * 10.0) / 10.0));
                    break;

                case 4:   // Aerobic Training Effect, scale 10
                    active_session_["Aerobic Training Effect"] = QString::number(value/10.0);
                    rideFile->setTag("Aerobic Training Effect", QString::number(value/10.0));
                    break;

                case 20:   // Anaerobic Training Effect, scale 10
                    active_session_["Anaerobic Training Effect"] = QString::number(value/10.0);
                    rideFile->setTag("Anaerobic Training Effect", QString::number(value/10.0));
                    break;

                case 9:   // Recovery Time, minutes
                    active_session_["Recovery Time"] = QString::number(round(value/60.0));
                    rideFile->setTag("Recovery Time", QString::number(round(value/60.0)));
                    break;

                case 17:   // Performance Condition
                    active_session_["Performance Condition"] = QString::number(value);
                    rideFile->setTag("Performance Condition", QString::number(value));
                    break;

                case 14:   // If watch detected Running Lactate Threshold Heart Rate, bpm
                    if(rideFile->isRun() && value > 0){
                        active_session_["LTHR detected"] = QString::number(value);
                        rideFile->setTag("LTHR detected", QString::number(value));
                    }
                    break;

                case 15:   // If watch detected Running Lactate Threshold Speed, m/s
                    if(rideFile->isRun() && value > 0){
                        active_session_["LTS detected"] = QString::number(value/100.0);
                        rideFile->setTag("LTS detected", QString::number(value/100.0));
                    }
                    break;
                default: break; // do nothing
            }


            if (FIT_DEBUG && FIT_DEBUG_LEVEL>3) {
                fprintf(stderr, "decodePhysiologicalMetrics  field %d: %d bytes, num %d, type %d\n", i, field.size, field.num, field.type );
            }
        }
    }

    void decodeSession(const FitMessage &def, int time_offset,
                       const std::vector<FitValue>& values) {
        time_t iniTime;
        if (time_offset > 0)
            iniTime = last_time + time_offset;
        else
            iniTime = last_time;

        int i = 0;
        time_t this_timestamp = 0, this_start_time = 0, this_elapsed_time = 0;
        QString sport, subsport;
        bool sport_found = false, subsport_found = false;
        QString prevSport = rideFile->getTag("Sport", "");
        double pool_length = 0.0;

        for(const FitField &field : def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 5: // sport field
                  if (sport_found == false) {
                    sport_found = true;
                    sport = FitFileParser::getSport(value);
                  }
                  break;
                case 6: // sub sport
                    if (subsport_found == false) {
                      subsport_found = true;
                      subsport = FitFileParser::getSubSport(value);
                    }
                    break;
                case 44: // pool_length
                    pool_length = value / 100000.0;
                    active_session_["Pool Length"] = QString("%1").arg(pool_length*1000.0);
                    rideFile->setTag("Pool Length", // in meters
                                      QString("%1").arg(pool_length*1000.0));
                    break;

                // other fields are ignored at present
                case 253: //timestamp
                    this_timestamp = value + qbase_time.toSecsSinceEpoch();
                    active_session_["_timestamp"] = static_cast<quint32>(this_timestamp);
                    break;
                case 168:   /* undocumented: Firstbeat EPOC based Exercise Load */
                    active_session_["EPOC"] = QString::number(round(value / 65536.0 ));
                    rideFile->setTag("EPOC", QString::number(round(value / 65536.0 )));
                    break;
                case 192:   /* undocumented: Feel manually entered after activity (0-25-50-75-100) */
                    active_session_["Feel"] = QString::number(round(value));
                    rideFile->setTag("Feel", QString::number(round(value)));
                    break;
                case 193:   /* undocumented: RPE manually entered after activity (0-10) */
                    active_session_["RPE"] = QString::number(value / 10.0 );
                    rideFile->setTag("RPE", QString::number(value / 10.0 ));
                    break;
                case 254: //index
                case 0:   //event
                case 1:    /* event_type */
                    break; // do nothing
                case 2:    /* start_time */
                    this_start_time = value + qbase_time.toSecsSinceEpoch();
                    active_session_["_start_time"] = static_cast<quint32>(this_start_time);
                    break;
                case 3:    /* start_position_lat */
                case 4:    /* start_position_long */
                    break; // do nothing
                case 7:    /* total elapsed time */
                    this_elapsed_time = value;
                    active_session_["_total_elapsed_time"] = static_cast<quint32>(this_elapsed_time);
                    break;
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
                default: break; // do nothing
            }

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>2) {
                printf("decodeSession  field %d: %d bytes, num %d, type %d\n", i, field.size, field.num, field.type );
            }
        }

        if (sport_found) {
            active_session_["Sport"] = sport;
            rideFile->setTag("Sport", sport);
        }
        if (subsport_found) {
            active_session_["SubSport"] = subsport;
            rideFile->setTag("SubSport", subsport);
        }


        // same procedure as for laps, code is c/p until a better solution is found
        if (this_timestamp == 0 && this_elapsed_time > 0) {
            this_timestamp = iniTime + this_elapsed_time - 1;
            active_session_["_timestamp"] = static_cast<quint32>(this_timestamp);
        }

        if (this_start_time == 0 || this_start_time-start_time < 0) {
            //errors << QString("lap %1 has invalid start time").arg(interval);
            this_start_time = start_time; // time was corrected after lap start
            active_session_["_start_time"] = static_cast<quint32>(this_start_time);

            if (this_timestamp == 0 || this_timestamp-start_time < 0) {
                active_session_.remove("_timestamp");
                errors << QString("lap %1 is ignored (invalid end time)").arg(interval);
                return;
            }
        }

        if (this_elapsed_time > 0) {
            time_t this_stop_time = this_start_time + round(this_elapsed_time/1000.0);
            active_session_["_stop_time"] = static_cast<quint32>(this_stop_time);
        }

        session_tags_.append(active_session_);
        session_device_info_list_.append(session_device_info_);
        session_data_info_list_.append(session_data_info_);
        active_session_.clear();
        session_device_info_.clear();
        session_data_info_.clear();
    }


    void decodeDeviceInfo(const FitMessage &def, int,
                          const std::vector<FitValue>& values) {
        int i = 0;

        int index=-1;
        int manu = -1, prod = -1, version = -1, type = -1;
        quint32 serial = 0;
        quint8 battery_status = 0;
        fit_string_value name;

        QString deviceInfo;

        for(const FitField &field : def.fields) {
            FitValue value = values[i++];

            //qDebug() << field.num << field.type << value.v;

            switch (field.num) {
                case 0:   // device index
                     index = value.v;
                     break;
                case 1:   // ANT+ device type
                     type = value.v;
                     break;
                case 2:   // manufacturer
                     manu = value.v;
                     break;
                case 3:   // serial number
                     serial = value.v;
                     break;
                case 4:   // product
                     prod = value.v;
                     break;
                case 5:   // software version
                     version = value.v;
                     break;
                case 11:  // battery status
                     battery_status = value.v;
                     break;
                case 27:   // product name
                     name = value.s;
                     break;

                // all other fields are ignored at present
                case 253: //timestamp
                case 10:  // battery voltage
                case 6:   // hardware version
                case 22:  // ANT network
                case 25:  // source type
                case 24:  // equipment ID
                default: ; // do nothing
            }

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>3) {
                fprintf(stderr, "decodeDeviceInfo  field %d: %d bytes, num %d, type %d\n", i, field.size, field.num, field.type );
            }
            //qDebug() << field.num << value.v;
        }

        //deviceInfo += QString("Device %1 ").arg(index);
        deviceInfo += QString("%1 ").arg(getDeviceType(type));
        if (manu>-1)
            deviceInfo += getManuProd(manu, prod);
        if (name.length()>0)
            deviceInfo += QString(" %1").arg(name.c_str());
        if (version>0)
            deviceInfo += QString(" (v%1)").arg(version/100.0);
        if (serial > 0 && serial < std::numeric_limits<quint32>::max())
            deviceInfo += QString(" ID:%1").arg(serial);
        if (battery_status > 0 && battery_status < 8)
            deviceInfo += QString(" BAT:%1").arg(getBatteryStatus(battery_status));

        // What is 7 and 0 ?
        // 3 for Moxy ?
        if (type>-1 && type != 0 && type != 7 && type != 3) {
            deviceInfos.insert(index, deviceInfo);
            session_device_info_.insert(index, deviceInfo);
        }
    }

    void decodeActivity(const FitMessage &def, int,
                        const std::vector<FitValue>& values) {
        int i = 0;

        const int delta = qbase_time.toSecsSinceEpoch();
        int event = -1, event_type = -1, local_timestamp = -1, timestamp = -1;

        for(const FitField &field : def.fields) {
            fit_value_t value = values[i++].v;

            if (value == NA_VALUE)
                continue;

            switch (field.num) {
                case 3: // event
                    event = value;
                    break;
                case 4: // event_type
                    event_type = value;
                    break;
                case 5: // local_timestamp
                    // adjust from seconds since 1989-12-31 00:00:00 UTC
                    if (0 != value)
                    {
                        local_timestamp = value + delta;
                    }
                    break;
                case 253: // timestamp
                    // adjust from seconds since 1989-12-31 00:00:00 UTC
                    if (0 != value)
                    {
                        timestamp = value + delta;
                    }
                    break;

                case 1: // num_sessions
                case 2: // type
                default:
                    break;
            }

            //qDebug() << field.num << value;
        }

        if (event != 26) // activity
            return;

        if (event_type != 1) // stop
            return;

        if (local_timestamp < 0 || timestamp < 0)
            return;
        
        if (0 == local_timestamp && 0 == timestamp)
            return;

        // In the new file structure activity comes first,
        // so we set start time when it is not already set.
        if (start_time == 0) {
            start_time = timestamp - 1; // recording interval?
            last_reference_time = start_time;
            QDateTime t;
            t.setSecsSinceEpoch(start_time);
            rideFile->setStartTime(t);
        }

        QDateTime t(rideFile->startTime().toUTC());
        if (0 == local_timestamp) {
            // ZWift FIT files are not reporting local timestamp
            rideFile->setStartTime(t.addSecs(timestamp));
        } else {
            // adjust start time to time zone of the ride
            rideFile->setStartTime(t.addSecs(local_timestamp - timestamp));
        }
    }

    void decodeEvent(const FitMessage &def, int,
                     const std::vector<FitValue>& values) {
        int time = -1;
        int event = -1;
        int event_type = -1;
        qint16 data16 = -1;
        qint32 data32 = -1;
        int i = 0;
        for(const FitField &field : def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: // timestamp field (s)
                    time = value + qbase_time.toSecsSinceEpoch();
                          break;
                case 0: // event field
                    event = value; break;
                case 1: // event_type field
                    event_type = value; break;
                case 2: // data16 field
                    data16 = value; break;
                case 3: //data32 field
                    data32 = value; break;

                // additional values (ignored at present):
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

            case 42: /* front_gear_change */
            case 43: /* rear_gear_change */
                {
                    int secs = (start_time==0?0:time-start_time);
                    XDataPoint *p = new XDataPoint();

                    switch (event_type) {
                        case 3:
                            p->secs = secs;
                            p->km = last_distance;
                            p->number[0] = ((data32 >> 24) & 255);
                            p->number[1] = ((data32 >> 8) & 255);
                            p->number[2] = ((data32 >> 16) & 255);
                            p->number[3] = (data32 & 255);
                            gearsXdata->datapoints.append(p);
                            break;
                        default:
                            errors << QString("Unknown gear change event %1 type %2 data %3").arg(event).arg(event_type).arg(data32);
                            break;
                    }
                }
                break;

            case 44: /* rider_position_change */
                {
                    int secs = (start_time==0?0:time-start_time);
                    XDataPoint *p = new XDataPoint();

                    switch (event_type) {
                        case 3:
                            p->secs = secs;
                            p->km = last_distance;
                            p->number[0] = (data32 & 255);
                            positionXdata->datapoints.append(p);
                            // qDebug() << QString("Rider position event received %1 type %2 data %3").arg(event).arg(event_type).arg(data32);
                            break;
                        default:
                            delete p;
                            errors << QString("Unknown rider position change event %1 type %2").arg(event).arg(event_type);
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
            case 45: /* elev_high_alert */
            case 46: /* elev_low_alert */
            case 47: /* comm_timeout */
            case 75: /* radar_threat_alert */
            default: ;
        }

        if (FIT_DEBUG && FIT_DEBUG_LEVEL>3) {
            fprintf(stderr, "event type %d\n", event_type);
        }
        last_event = event;
        last_event_type = event_type;
    }

    void decodeHRV(const FitMessage &def,
                   const std::vector<FitValue>& values) {
        int rrvalue;
        int i=0;
        double hrv_time=0.0;
        int n=hrvXdata->datapoints.count();

        if (n>0)
            hrv_time = hrvXdata->datapoints[n-1]->secs;

        for(const FitField &field : def.fields) {
            FitValue value = values[i++];
            if ( value.type == ListValue && field.num == 0){
                for (int j=0; j<value.list.size(); j++)
                {
                    rrvalue = int(value.list.at(j));
                    hrv_time += rrvalue/1000.0;

                    if (rrvalue == -1){
                        break;
                    }
                    XDataPoint *p = new XDataPoint();
                    p->secs = hrv_time;
                    p->number[0] = rrvalue;
                    hrvXdata->datapoints.append(p);
                }
            } else if (value.type == SingleValue)
            {
                rrvalue = int(value.v);
                hrv_time += rrvalue/1000.0;

                XDataPoint *p = new XDataPoint();
                p->secs = hrv_time;
                p->number[0] = rrvalue;
                hrvXdata->datapoints.append(p);
            }
        }
    }

    void decodeLap(const FitMessage &def, int time_offset,
                   const std::vector<FitValue>& values) {
        time_t iniTime;
        if (time_offset > 0)
            iniTime = last_time + time_offset;
        else
            iniTime = last_time;

        time_t time = 0;
        int i = 0;
        time_t this_start_time = 0;
        double total_distance = 0.0;
        double total_elapsed_time = 0.0;

        QString lap_name;

        if (FIT_DEBUG && FIT_DEBUG_LEVEL>1)  {
            fprintf(stderr, " FIT decode lap \n");
        }

        for(const FitField &field : def.fields) {
            const FitValue& value = values[i++];

            if( value.v == NA_VALUE )
                continue;

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>3) {
                fprintf (stderr, "\tfield: num: %d ", field.num);
                DumpFitValue(value);
            }

            // ignore any developer fields in laps
            if ( field.deve_idx > -1)
                continue;

            switch (field.num) {
                case 253:
                    time = value.v + qbase_time.toSecsSinceEpoch();
                    break;
                case 2:
                    this_start_time = value.v + qbase_time.toSecsSinceEpoch();
                    break;
                case 9:
                    total_distance = value.v / 100000.0;
                    break;
                case 7:
                    total_elapsed_time = value.v / 1000.0;
                    break;
                case 24:
                    //lap_trigger = value.v;


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

        if (time == 0 && total_elapsed_time > 0) {
            time = iniTime + total_elapsed_time - 1;
        }

        // In the new file format lap messages come first
        if (start_time == 0 && this_start_time >0) {
            start_time = this_start_time;
            last_reference_time = start_time;
            QDateTime t;
            t.setSecsSinceEpoch(start_time);
            rideFile->setStartTime(t);
        }
        // and timestamp doesn't match lap stop time anymore
        if (time <= this_start_time) {
            time = this_start_time + total_elapsed_time - 1;
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
            // Fill empty lengths due to false starts or pauses in some devices
            // s.t. Garmin 910xt
            double secs = time - start_time;
            if ((total_distance == 0.0) && (secs > last_length + 1)) {

                XDataPoint *p = new XDataPoint();
                p->secs = secs;
                p->km = last_distance;
                p->number[0] = 0;
                p->number[1] = secs-last_length;
                p->number[2] = 0;
                swimXdata->datapoints.append(p);

                last_length = secs;
            }
            ++interval;
        } else {
            // Lap messages can occur before record messages,
            // so we add them without checking for samples.
            ++interval;
            if (lap_name == "") {
                lap_name = QObject::tr("Lap %1").arg(interval);
            }
            rideFile->addInterval(RideFileInterval::DEVICE,
                                  this_start_time - start_time,
                                  time - start_time,
                                  lap_name);
        }
    }

    void decodeRecord(const FitMessage &def, int time_offset,
                      const std::vector<FitValue>& values) {
        time_t time = 0;
        if (time_offset == 0) // Damien : I have to confirm this...
            last_reference_time = last_time;
        if (time_offset > -1)
            time = last_reference_time + time_offset; // was last_time + time_offset

        double alt = last_altitude, cad = 0, km = 0, hr = 0, lat = 0, lng = 0, badgps = 0, lrbalance = RideFile::NA;
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
        double tcore = 0;
        //bool run=false;

        XDataPoint *p_deve = NULL;
        XDataPoint *p_extra = NULL;

        fit_value_t lati = NA_VALUE, lngi = NA_VALUE;
        int i = 0;
        for(const FitField &field : def.fields) {
            FitValue _values = values[i];
            fit_value_t value = values[i].v;
            QList<fit_value_t> valueList = values[i++].list;

            double deve_value = 0.0;

            if( _values.type == SingleValue && value == NA_VALUE )
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
                              time = value + qbase_time.toSecsSinceEpoch();
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
                    case 78:// ENHANCED ALTITUDE
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
                    case 73:// ENHANCED SPEED
                            if (field.deve_idx>-1)
                                 native_num = -1;
                            else
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
                             // When bit 7 is 1 value are right power contribution
                             // not '1' the location of the contribution is undefined
                             if (value & 0x80)
                                lrbalance = 100 - (value & 0x7F);
                             else
                                lrbalance = RideFile::NA;
                             break;
                    case 31: // GPS Accuracy
                             break;

                    case 32: // VERTICAL_SPEED
                             native_num = -1;
                             break;

                    case 39: // VERTICAL OSCILLATION
                             if (!native_profile && field.deve_idx>-1)
                                rvert = deve_value;
                             else
                                rvert = value / 100.0f;
                             break;

                    case 40: // GROUND CONTACT TIME PERCENT
                             native_num = -1;
                             break;

                    case 41: // GROUND CONTACT TIME
                             if (!native_profile && field.deve_idx>-1)
                                 rcontact = deve_value;
                             else
                                rcontact = value / 10.0f;
                             break;

                    case 42: // ACTIVITY_TYPE
                             native_num = -1;
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
                             // --> XDATA
                             native_num = -1;
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

                             if (leftTopDeathCenter>360) {
                                 leftTopDeathCenter = 0;
                             }
                             if (leftBottomDeathCenter>360) {
                                 leftBottomDeathCenter = 0;
                             }
                             break;
                    case 70: // ? Left Peak Phase  ?
                             leftTopPeakPowerPhase = round(valueList.at(0) * 360.0/256);
                             leftBottomPeakPowerPhase = round(valueList.at(1) * 360.0/256);

                             if (leftTopPeakPowerPhase>360) {
                                 leftTopPeakPowerPhase = 0;
                             }
                             if (leftBottomPeakPowerPhase>360) {
                                 leftBottomPeakPowerPhase = 0;
                             }
                             break;
                    case 71: // ? Right Power Phase ?
                             rightTopDeathCenter = round(valueList.at(0) * 360.0/256);
                             rightBottomDeathCenter = round(valueList.at(1) * 360.0/256);

                             if (rightTopDeathCenter>360) {
                                 rightTopDeathCenter = 0;
                             }
                             if (rightBottomDeathCenter>360) {
                                 rightBottomDeathCenter = 0;
                             }
                             break;
                    case 72: // ? Right Peak Phase  ?
                             rightTopPeakPowerPhase = round(valueList.at(0) * 360.0/256);
                             rightBottomPeakPowerPhase = round(valueList.at(1) * 360.0/256);

                             if (rightTopPeakPowerPhase>360) {
                                 rightTopPeakPowerPhase = 0;
                             }
                             if (rightBottomPeakPowerPhase>360) {
                                 rightBottomPeakPowerPhase = 0;
                             }
                             break;
                    case 81: // BATTERY_SOC
                             native_num = -1;
                             break;
                    case 83: // VERTICAL_RATIO
                             native_num = -1;
                             break;
                    case 84: // Left right balance
                             lrbalance = value/100.0;
                             break;
                    case 85: // STEP_LENGTH
                             native_num = -1;
                             break;
                    case 87: // Cycle length (16)
                             native_num = -1;
                             break;
                    case 90: // PERFORMANCE_CONDITION
                             native_num = -1;
                             break;
                    case 108: // to confirm : RESPIRATIONRATE
                             native_num = -1;
                             break;
                    case 114: // MTB Dynamics - Grit
                             native_num = -1;
                             break;
                    case 115: // MTB Dynamics - Flow
                             native_num = -1;
                             break;
                    case 116: // Stress
                             native_num = -1;
                             break;
                    case 133: // Pulse Ox
                             native_num = -1;
                             break;
                    case 139: // Core Temp
                             if (field.deve_idx>-1) {
                                 tcore = deve_value;
                             } else {
                                 tcore = value;
                                 native_num = -1;
                             }
                             break;
                    default:
                            unknown_record_fields.insert(native_num);
                            native_num = -1;
                }
            }

            if (native_num == -1 || field.deve_idx>-1) {
                // native, deve_native or deve to record.

                int idx = -1;

                if (field.deve_idx>-1) {
                    QString key = QString("%1.%2").arg(field.deve_idx).arg(field.num);
                    FitFieldDefinition deveField = local_deve_fields[key];

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
                            nativeName = getNameForExtraNative(field.num);

                        extraXdata->valuename << nativeName;
                        extraXdata->unitname << "";

                        //dataInfos.append(QString("EXTRA %1").arg(nativeName));

                        record_extra_fields.insert(field.num, record_extra_fields.count());
                    }
                    idx = record_extra_fields[field.num];

                    if (idx>-1) {
                        float scale = getScaleForExtraNative(field.num);
                        int offset = getOffsetForExtraNative(field.num);

                        if (p_extra == NULL &&
                                (_values.type == SingleValue ||
                                 _values.type == FloatValue ||
                                 _values.type == StringValue))
                           p_extra = new XDataPoint();

                        switch (_values.type) {
                            case SingleValue: p_extra->number[idx]=_values.v/scale+offset; break;
                            case FloatValue: p_extra->number[idx]=_values.f/scale+offset; break;
                            case StringValue: p_extra->string[idx]=_values.s.c_str(); break;
                            default: break;
                        }
                    }

                }


            } else {
                if (field.deve_idx>-1) {
                    QString key = QString("%1.%2").arg(field.deve_idx).arg(field.num);
                    FitFieldDefinition deveField = local_deve_fields[key];

                    if (!record_deve_fields.contains(key)) {
                        addRecordDeveField(key, deveField, false);
                    }
                }
            }
        }

        //if (time == last_time)
        //    return; // Not true for Bryton

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
            last_reference_time = start_time;
            QDateTime t;
            t.setSecsSinceEpoch(start_time);
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
                        (lrbalance!=RideFile::NA && prevPoint->lrbalance!=RideFile::NA) ? prevPoint->lrbalance + (deltaLeftRightBalance * weight) : RideFile::NA, // interpolate between valid values only
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
                        tcore, // tcore
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
                     smO2, tHb, rvert, rcad, rcontact, tcore, interval, false);

        last_time = time;
        last_distance = km;
        last_altitude = alt;

        if (p_deve != NULL) {
            p_deve->secs = secs;
            deveXdata->datapoints.append(p_deve);
        }
        if (p_extra != NULL) {
            p_extra->secs = secs;
            extraXdata->datapoints.append(p_extra);
        }
    }

    void decodeLength(const FitMessage &def, int time_offset,
                      const std::vector<FitValue>& values) {
        if (!isLapSwim) {
            isLapSwim = true;
            last_length = 0.0;
        }
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;

        int length_type = 0;
        int swim_stroke = 0;
        int total_strokes = 0;
        double length_duration = 0.0;

        int i = 0;
        for(const FitField &field : def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 0: // event
                        if (FIT_DEBUG && FIT_DEBUG_LEVEL>2) qDebug() << " event:" << value;
                        break;
                case 1: // event type
                        if (FIT_DEBUG && FIT_DEBUG_LEVEL>2) qDebug() << " event_type:" << value;
                        break;
                case 2: // start time
                        time = value + qbase_time.toSecsSinceEpoch();
                        // Time MUST NOT go backwards
                        // You canny break the laws of physics, Jim

                        if (time < last_length)
                            time = last_length;
                        break;
                case 3: // total elapsed time
                        if (FIT_DEBUG && FIT_DEBUG_LEVEL>2) qDebug() << " total_elapsed_time:" << value;
                        break;
                case 4: // total timer time
                        length_duration = value / 1000.0;
                        break;
                case 5: // total strokes
                        total_strokes = value;
                        break;
                case 6: // avg speed
                        // kph = value * 3.6 / 1000.0;
                        break;
                case 7: // swim stroke: 0-free, 1-back, 2-breast, 3-fly,
                        //              4-drill, 5-mixed, 6-IM
                        swim_stroke = value;
                        break;
                case 9: // cadence
                        // cad = value;
                        break;
                case 11: // total_calories
                        if (FIT_DEBUG && FIT_DEBUG_LEVEL>2) qDebug() << " total_calories:" << value;
                        break;
                case 12: // length type: 0-rest, 1-strokes
                        length_type = value;
                        break;
                case 254: // message_index
                        if (FIT_DEBUG && FIT_DEBUG_LEVEL>2) qDebug() << " message_index:" << value;
                        break;
                default:
                         unknown_record_fields.insert(field.num);
            }
        }

        if (length_duration > 0) {
            XDataPoint *p = new XDataPoint();
            p->secs = last_length;
            p->km = last_distance;
            p->number[0] = length_type + swim_stroke;
            p->number[1] = length_duration;
            p->number[2] = total_strokes;

            swimXdata->datapoints.append(p);
        }

        if (last_length == 0) {
            start_time = time - 1; // recording interval?
            last_reference_time = start_time;
            QDateTime t;
            t.setSecsSinceEpoch(start_time);
            rideFile->setStartTime(t);
            interval = 1;
        }

        last_length += length_duration;

    }

    /* weather broadcast as observed at weather station (undocumented) */
    void decodeWeather(const FitMessage &def, int time_offset,
                      const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);

        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        double windHeading = 0.0, windSpeed = 0.0, temp = 0.0, humidity = 0.0;

        int i = 0;
        for(const FitField &field : def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: // Timestamp
                          time = value + qbase_time.toSecsSinceEpoch();
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

    void decodeHr(const FitMessage &def, int time_offset,
                      const std::vector<FitValue>& values) {
        time_t time = 0;
        if (time_offset > 0) {
            time = last_time + time_offset;
        }

        QList<double> timestamps;
        QList<double> hr;
        QList<double> rr;

        int a = 0;
        int j = 0;
        for(const FitField &field : def.fields) {
            FitValue value = values[a++];

            if( value.type == SingleValue && value.v == NA_VALUE )
                continue;

            switch (field.num) {
                case 253: // Timestamp
                          time = value.v + qbase_time.toSecsSinceEpoch();
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
                          // update start_timestamp only if Timestamp (253) included for resyncs
                          if (time > 0) start_timestamp = time-last_event_timestamp/1024.0;
                          timestamps.append(last_event_timestamp/1024.0);
                          last_RR=last_event_timestamp;
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

                              rr.append((last_event_timestamp/1024.0-last_RR/1024.0)*1000); // R-R in ms
                              last_RR=last_event_timestamp;
                          }

                          break;

                default: ; // ignore it
            }
        }

        // Garmin HR-Swim belt with internal memory
        if (timestamps.count()>0 && timestamps.count()==rr.count() && timestamps.count()==hr.count()) {
            // start from i=1 as we cannot determine R-R for the very first heart beat
            for (int i=1;i<timestamps.count(); i++) {
                double secs = timestamps.at(i) + start_timestamp - start_time;
                if (secs>=0 && rr.at(i)!=0) {
                    XDataPoint *p = new XDataPoint();
                    p->secs = secs;
                    p->number[0] = rr.at(i);
                    hrvXdata->datapoints.append(p);
                }
            }
        }

        for (int i=0;i<timestamps.count(); i++) {
            double secs = round(timestamps.at(i) + start_timestamp - start_time);
            if (secs>0) {
                int idx = rideFile->timeIndex(round(secs));

                if (idx < 0 || rideFile->dataPoints().at(idx)->secs==secs)
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

    void decodeDeviceSettings(const FitMessage &def, int time_offset,
                      const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);
        int i = 0;
        for(const FitField &field : def.fields) {
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

    void decodeSegment(const FitMessage &def, int time_offset,
                      const std::vector<FitValue>& values) {
        time_t time = 0;
        if (time_offset > 0)
            time = last_time + time_offset;
        else
            time = last_time;

        int i = 0;
        time_t this_start_time = 0;
        ++interval;

        QString segment_name;
        bool fail = false;

        for(const FitField &field : def.fields) {
            const FitValue& value = values[i++];

            if( value.type != StringValue && value.v == NA_VALUE )
                continue;

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>3) {
                fprintf (stderr, "\tfield: num: %d ", field.num);
                DumpFitValue(value);
            }

            switch (field.num) {
                case 253: // Message timestamp
                    time = value.v + qbase_time.toSecsSinceEpoch();
                    break;
                case 2:   // start timestamp ?
                    this_start_time = value.v + qbase_time.toSecsSinceEpoch();
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
                         // total_elapsed_time = round(value.v / 1000.0);
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
                    if (FIT_DEBUG && FIT_DEBUG_LEVEL>4)  {
                        fprintf(stderr, "Found segment name: %s\n", segment_name.toStdString().c_str());
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
            if (segment_name == "") segment_name = QObject::tr("Lap %1").arg(interval);
            rideFile->addInterval(RideFileInterval::DEVICE, this_start_time - start_time, time - start_time, segment_name);
        }

    }

    void decodeUserProfile(const FitMessage &def, int time_offset,
                      const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);
        int i = 0;
        for(const FitField &field : def.fields) {
            fit_value_t value = values[i++].v;

            if( value == NA_VALUE )
                continue;

            switch (field.num) {
                case 4:  // Weight
                    active_session_["Weight"] = QString::number(value / 10.0 );
                    rideFile->setTag("Weight", QString::number(value / 10.0 ));
                    break;
                default: ; // ignore it
            }
        }
    }


    void decodeDeveloperID(const FitMessage &def, int time_offset,
                          const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);

        FitDeveApp deve;

        // 0	developer_id	byte
        // 1	application_id	byte
        // 2	manufacturer_id	manufacturer
        // 3	developer_data_index	uint8
        // 4	application_version	uint32

        int i = 0;
        for(const FitField &field : def.fields) {
            FitValue value = values[i++];

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>2)
                qDebug() << "deveID : num" << field.num  << value.v << value.s.c_str();

            fit_string_value dev_id = "";

            switch (field.num) {
                case 0:  // developer_id
                        dev_id = "";

                        foreach(fit_value_t val, value.list) {

                            if (val != NA_VALUE) {
                                std::stringstream sstream;
                                sstream << std::hex << val;
                                if (val<16)
                                    dev_id += "0";
                                dev_id += sstream.str();
                            }
                        }
                        if (dev_id.length()>=20) {
                            dev_id.insert(8, "-");
                            dev_id.insert(13, "-");
                            dev_id.insert(18, "-");
                            dev_id.insert(23, "-");
                        }
                        if (FIT_DEBUG && FIT_DEBUG_LEVEL>2)
                            qDebug() << "developer_id" << dev_id.c_str();
                        deve.dev_id = dev_id;
                        break;
                case 1:  // application_id
                        dev_id = "";

                        foreach(fit_value_t val, value.list) {

                            if (val != NA_VALUE) {
                                std::stringstream sstream;
                                sstream << std::hex << val;
                                if (val<16)
                                    dev_id += "0";
                                dev_id += sstream.str();
                            }
                        }
                        if (dev_id.length()>=20) {
                            dev_id.insert(8, "-");
                            dev_id.insert(13, "-");
                            dev_id.insert(18, "-");
                            dev_id.insert(23, "-");
                        }

                        if (FIT_DEBUG && FIT_DEBUG_LEVEL>2)
                            qDebug() << "application_id" << dev_id.c_str();
                        deve.app_id = dev_id;
                        break;
                case 2:  // manufacturer_id
                        deve.man_id = value.v;
                        break;
                case 3:  // developer_data_index
                        deve.dev_data_id = value.v;
                        break;
                case 4: deve.app_version = value.v;
                        break;
                default:
                        // ignore it
                        break;
            }
        }

        if (FIT_DEBUG && FIT_DEBUG_LEVEL>2)
            qDebug() << "DEVE ID" << deve.dev_id.c_str() << "app_id" << deve.app_id.c_str() << "man_id" << deve.man_id << "dev_data_id" << deve.dev_data_id << "app_version" << deve.app_version;

        QString appKey = QString("%1").arg(deve.dev_data_id);
        if (local_deve_fields_app.contains(appKey)) {
            FitDeveApp lastDeveApp = local_deve_fields_app[appKey];
            if (lastDeveApp.app_id != deve.app_id) {
                local_deve_fields_app.insert(deve.app_id.c_str(), lastDeveApp);
            }
        }
        local_deve_fields_app.insert(appKey, deve);


    }

    void decodeDeveloperFieldDescription(const FitMessage &def, int time_offset,
                      const std::vector<FitValue>& values) {
        Q_UNUSED(time_offset);
        int i = 0;

        FitFieldDefinition fieldDef;
        fieldDef.scale = -1;
        fieldDef.offset = -1;
        fieldDef.native = -1;
        int native_mesg_num = RECORD_MSG_NUM; // just in case it is missing, for backward compatibility

        for(const FitField &field : def.fields) {
            FitValue value = values[i++];

            switch (field.num) {
                case 0:  // developer_data_index
                        fieldDef.dev_id = value.v;
                        break;
                case 1:  // field_definition_number
                        fieldDef.num = value.v;
                        break;
                case 2:  // fit_base_type_id
                        fieldDef.type = value.v & 0x1F; // FIX applied in refactor
                        break;
                case 3:  // field_name
                        fieldDef.name = value.s;
                        break;
                case 4:  // array
                        break;
                case 5:  // components
                        break;
                case 6:  // scale
                        if (value.v == NA_VALUE) fieldDef.scale = -1; // FIX applied in refactor
                        else fieldDef.scale = value.v;
                        break;
                case 7:  // offset
                        if (value.v == NA_VALUE) fieldDef.offset = -1; // FIX applied in refactor
                        else fieldDef.offset = value.v;
                        break;
                case 8:  // units
                        fieldDef.unit = value.s;
                        break;
                case 9:  // bits
                case 10: // accumulate
                case 13: // fit_base_unit_id
                        break;
                case 14: // native_mesg_num
                        native_mesg_num = value.v;
                        break;
                case 15: // native field number
                        if (value.v == NA_VALUE) fieldDef.native = -1; // FIX applied in refactor
                        else fieldDef.native = value.v;
                        break;
                default:
                        // ignore it
                        break;
            }
        }
        fieldDef.message = fitMessageDesc(native_mesg_num,true).toStdString();

        if (FIT_DEBUG && FIT_DEBUG_LEVEL>0) {
            // always show these regardless of debug level
            fprintf(stderr,"** Developer Field Definition Added '%s' num %d Developer id %d type %s native %d unit %s scale %f offset %d\n",
                                                                                fieldDef.name.c_str(),
                                                                                fieldDef.num,
                                                                                fieldDef.dev_id,
                                                                                fitBaseTypeDesc(fieldDef.type).toStdString().c_str(),
                                                                                fieldDef.native,
                                                                                fieldDef.unit.c_str(),
                                                                                fieldDef.scale,
                                                                                fieldDef.offset);
        }

        QString key = QString("%1.%2").arg(fieldDef.dev_id).arg(fieldDef.num);

        // add or replace with new definition
        local_deve_fields.insert((key), fieldDef);

        if ((native_mesg_num == RECORD_MSG_NUM) && fieldDef.native > -1 &&
            !record_deve_native_fields.values().contains(fieldDef.native)) {
            record_deve_native_fields.insert(key, fieldDef.native);

            /*RideFile::SeriesType series = getSeriesForNative(fieldDef.native);

            if (series != RideFile::none) {
                QString nativeName = rideFile->symbolForSeries(series);
                dataInfos.append(QString("NATIVE %1 : Field %2").arg(nativeName).arg(fieldDef.name.c_str()));
            }*/
        }
    }

    //
    // FIT FILE EXECUTIVE FUNCTIONS AND RECORD READING
    //
    // The executive functions that read FIT protocol records and
    // create the top-level data structures used by the decoders
    // listed above
    //
    //          read_header()
    //          read_record()
    //          run() - start the parser !
    //
    //


    //
    //
    // The FIT header is either
    //
    // 12 bytes long- for the original FIT file protocol
    // 14 bytes long- for the current FIT file protocol (includes a header CRC)
    // >14 bytes long- it is for a later protocol
    //
    // As of November 2023 the FIT protocol is v32
    //
    void read_header(bool &stop, QStringList &errors, int &data_size) {

        stop = false;
        try {

            // read the header- make sure its 12 or 14 bytes long as that's all we know about
            int header_size = read_uint8();
            if (header_size != 12 && header_size != 14) {
                errors << QString("bad header size: %1 (we only support 12 or 14)").arg(header_size);
                stop = true;
            }

            // protocol version should be 1 or 2
            int protocol_version = read_uint8();
            (void) protocol_version;

            // if the header size is 14 we have profile minor then profile major
            // version. We still don't do anything with this information
            int profile_version = read_uint16(false); // always littleEndian
            (void) profile_version;
            //qDebug() << "profile_version" << profile_version/100.0; // not sure what to do with this

            // we always start with version info if debug is on
            if (FIT_DEBUG) fprintf(stderr, "FIT Protocol Version %d, Profile Version %d\n", protocol_version, profile_version);

            // length of data section excluding the header and trailing CRC record
            data_size = read_uint32(false); // always littleEndian

            // the chars ".FIT" for no other reason than it appears if you open
            // the file in a text editor (they haven't heard of magic numbers)
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
            int header_crc=0;
            if (header_size == 14) {
                header_crc = read_uint16(false);
            }
            Q_UNUSED(header_crc); // we don't check since its optional anyway

        } catch (TruncatedRead &e) {
            Q_UNUSED(e)
            errors << "truncated file header";
            stop = true;
        }
    }

    //
    // Records are either: A Definition Record (defining a local message type, optionally including developer fields)
    //                     A Data Record (as a local nessage type, stored in accordance with a previous definition record)
    //
    // This function parses definition records and stores the details in local_msg_types (Qmap)
    // Or it parses data records and calls the relevant decodeXXXX function to work with the data extracted
    //
    int read_record(bool &stop, QStringList &errors) {

        stop = false;
        int count = 0;

        // the header byte tells us what kind of record we are parsing
        //
        // Bit 7        0x80            0- Normal header 1- Compressed time record (only applies to data message)
        //     6        0x40            0- Data Message or 1- Definition Message
        //     5        0x20            When set in a definition message indicates that developer fields are included
        //     4        0x10            Reserved (should be ignored)
        //     0-3      0x0F (mask)     Local Message Number (also referred to as type)
        //
        int header_byte = read_uint8(&count);

        // For some reason the header byte is being parsed differently inside this if clause
        // this makes no sense but will not fix at this point *FIXME*
        //
        // The test for a definition record- is it normal (0x80) and a Definition message (0x40)
        if (!(header_byte & 0x80) && (header_byte & 0x40)) {

            //
            // LOCAL MESSAGE DEFINITION
            //

            // lets get the local message number
            int local_msg_num = header_byte & 0xf;

            // Do we also have developer fields in this definition ?
            bool hasDeveloperFields = (header_byte & 0x20) == 0x20 ;

            // If the definition already exists it will be replaced
            // with a blank new one, so re-defining as we go
            local_msg_types.insert(local_msg_num, FitMessage());

            // We get a reference to the newly created message definition to work with
            FitMessage &def = local_msg_types[local_msg_num];

            //
            // A local message definition is comprised:
            //
            // Byte 1           Reserved (ignored)
            //      2           Architecture 0- little-endian 1- big endian
            //      3-4         Global Message Nunber (for semantics)
            //      5           Number of fields that follow (n)
            //
            //      6 onwards   n x Field Definitions
            //
            int reserved = read_uint8(&count); (void) reserved; // unused
            def.is_big_endian = read_uint8(&count);
            def.global_msg_num = read_uint16(def.is_big_endian, &count);
            def.local_msg_num = local_msg_num;
            int num_fields = read_uint8(&count);

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>0)  fprintf(stderr, "message definition: local=%d global=%d (%s) big endian=%d fields=%d\n",
                                                                 def.local_msg_num, def.global_msg_num, fitMessageDesc(def.global_msg_num, false).toStdString().c_str(),
                                                                 def.is_big_endian, num_fields );


            //
            // Run through the field definitions that follow, comprised of
            //
            // Byte 1           Field Number
            //      2           Field Size (also inferred from the base type next)
            //      3           Base Type (see defn below)
            //
            // Base Type is comprised:
            //
            // Bit 7     0x80           Endianness (0- for single byte data, 1 for multi-byte)
            //     5-6   0x60 (mask)    Reserved (ignored)
            //     0-4   0x1F (mask)    Base type (max 32 types currently 17 defined in specs see FITBaseTypes above)
            //
            for (int i = 0; i < num_fields; ++i) {

                def.fields.push_back(FitField());
                FitField &field = def.fields.back();

                field.num = read_uint8(&count);
                field.size = read_uint8(&count);
                int base_type = read_uint8(&count);
                field.type = base_type & 0x1F;
                field.deve_idx = -1;

                if (FIT_DEBUG && FIT_DEBUG_LEVEL>3)  fprintf(stderr, "  field %d: %d bytes, num %d, type %d, size %d\n",
                                                                     i, field.size, field.num, field.type, field.size );
            }

            // Once all the standard field definitions have been read they will be followed
            // by optional developer fields, these are indicated in the header (see
            // hasDeveloperFields above)
            //
            //      1           Number of developer fields (m)
            //      2           m x Developer Field Definitions if flag set in header
            //
            if (hasDeveloperFields) {

                // first byte is the number of fields defined
                int num_fields = read_uint8(&count);

                //
                // Run through the developer field references that follow, comprised of
                //
                // Byte 1           Developer Field Number
                //      2           Field Size (also inferred from the base type next)
                //      3           Developer ID (they start from 0)
                //
                // NOTE: the developer ID and field is not defined here, it is referenced by developer id
                //       and field number. The definition is parsed as a separate message type (206).
                //       See decodeDeveloperID and decodeDeveloperFieldDescription() for the details
                //
                for (int i = 0; i < num_fields; ++i) {

                    def.fields.push_back(FitField());
                    FitField &field = def.fields.back();

                    field.num = read_uint8(&count);
                    field.size = read_uint8(&count);
                    field.deve_idx = read_uint8(&count);

                    QString key = QString("%1.%2").arg(field.deve_idx).arg(field.num);
                    FitFieldDefinition devField = local_deve_fields[key];
                    field.type = devField.type;

                    if (FIT_DEBUG && FIT_DEBUG_LEVEL>3)  fprintf(stderr, "  developer field %d: %d bytes, num %d, type %s, size %d\n",
                                                                            i, field.size, field.num, fitBaseTypeDesc(field.type).toStdString().c_str(), field.size);
                }
            }

        } else {

            //
            // LOCAL MESSAGE DATA
            //

            int local_msg_num = 0;
            int time_offset = -1;

            //
            // Data records can contain a compressed time record, in which case
            // ....
            //
            // In all cases, the rest of the record after the header byte
            // contains the data stored according to the definition we decoded
            // above
            //

            // compressed time record?
            if (header_byte & 0x80) {
                local_msg_num = (header_byte >> 5) & 0x3;
                time_offset = header_byte & 0x1f;

            } else {

                // normal data
                local_msg_num = header_byte & 0xf;
            }

            // lets check we have previously seen a definition record for
            // the message we are now going to process, because if we haven't
            // then there is no way we can deserialise the contents and will
            // need to abort.
            if (!local_msg_types.contains(local_msg_num)) {
                errors << QString("local type %1 without previous definition").arg(local_msg_num);
                stop = true;
                return count;
            }

            // lets get the previously stored definition
            const FitMessage &def = local_msg_types[local_msg_num];

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>1)  { fprintf(stderr, "read_record message local=%d global=%d offset=%d\n",
                                                                   local_msg_num, def.global_msg_num, time_offset ); }


            // now we just work through the definition and extract the field values
            // from the file directly
            std::vector<FitValue> values;
            for(const FitField &field : def.fields) {

                // we store the value into a struct that has members
                // for all the fit base types- so floats are in 'f' and
                // integers are in 'v' and strings are in 's'
                FitValue value;
                int size;

                // see FITbasetypes at the top of the file for the basic types
                // that are supported. Note that the read_XXXX routines will
                // check for FIT NA values and set to NA_VALUE where needed
                //
                // It is worth noting that size > the sizeof(type) indicates
                // a list or array of values
                switch (field.type) {

                    // Enumerated type (8 bit)
                    case 0: size = 1;

                            if (field.size==size) {
                                value.type = SingleValue; value.v = read_uint8(&count);;
                             } else { // Multi-values
                                value.type = ListValue;
                                value.list.clear();
                                for (int i=0;i<field.size/size;i++) {
                                    value.list.append(read_uint8(&count));
                                }
                                size = field.size;
                            }
                            break;

                    // Signed Int 8bit
                    case 1: value.type = SingleValue; value.v = read_int8(&count); size = 1;  break;


                    // Unsigned Int 8bit
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

                    // Signed Int 16
                    case 3: value.type = SingleValue; value.v = read_int16(def.is_big_endian, &count); size = 2;  break;

                    // Unsigned Int 16
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

                    // Signed Int 32
                    case 5: value.type = SingleValue; value.v = read_int32(def.is_big_endian, &count); size = 4;  break;

                    // Unsigned Int 32
                    case 6: size = 4;
                            if (field.size==size) {
                                value.type = SingleValue; value.v = read_uint32(def.is_big_endian, &count);
                            } else if (field.size<size) {
                                // Some device (eg Coros Pace 2) seems to declare uint32 with size 1
                                value.type = SingleValue;
                                if (field.size == 1)
                                    value.v = read_uint8(&count);
                                if (field.size == 2)
                                    value.v = read_uint16(def.is_big_endian, &count);
                                size = field.size;
                            } else { // Multi-values
                                value.type = ListValue;
                                value.list.clear();
                                for (int i=0;i<field.size/size;i++) {
                                    value.list.append(read_uint32(def.is_big_endian, &count));
                                }
                                size = field.size;
                            }
                            break;

                    // String
                    case 7:
                        value.type = StringValue;
                        value.s = read_text(field.size, &count);
                        size = field.size;
                        break;

                    // 32bit Float
                    case 8:
                        size = 4;
                        if (field.size==size) {
                            value.type = FloatValue;
                            value.f = read_float32(def.is_big_endian, &count);
                            if (value.f != value.f) // No NAN
                                value.f = 0;
                        } else { // Multi-values
                            value.type = ListValue;
                            value.list.clear();
                            for (int i=0;i<field.size/size;i++) {
                                value.list.append(read_float32(def.is_big_endian, &count));
                            }
                            size = field.size;
                        }
                        break;

                    // 64bit Float (unimplemented at present)
                    //case 9:

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

                    // Unsigned Int 16bit - A zero value signifies an invalid value (unimplemented)
                    case 11: value.type = SingleValue; value.v = read_uint16z(def.is_big_endian, &count); size = 2; break;

                    // Unsigned Int 32bit - A zero value signifies an invalid value (unimplemented)
                    case 12: value.type = SingleValue; value.v = read_uint32z(def.is_big_endian, &count); size = 4; break;

                    // 8 bit byte
                    case 13:
                             value.type = ListValue;
                             value.list.clear();
                             for (int i=0;i<field.size;i++) {
                                value.list.append(read_byte(&count));
                             }
                             size = value.list.size();
                             break;

                    // 64 bit integers are not yet implemented in the code
                    // case 14: Signed Int 64 bit
                    // case 15: Unsigned Int 64 bit
                    // case 16: Unsigned Int 64 bit Zero is an invalid value

                    // if in doubt just read the number of bytes for the
                    // data type and discard it
                    default:
                        if (FIT_DEBUG && FIT_DEBUG_LEVEL>3)  { fprintf(stderr, "unknown type: %d size: %d \n", field.type, field.size);  }

                        if (field.size > 0) read_unknown( field.size, &count );
                        value.type = SingleValue;
                        value.v = NA_VALUE;
                        unknown_base_type.insert(field.type);
                        size = field.size;
                        break;
                }

                // Size is greater than expected
                // Note we already check above for arrays
                // So this is likely a redundant check
                if (size < field.size) {
                    if (FIT_DEBUG && FIT_DEBUG_LEVEL>4)  { fprintf(stderr, "   warning : size=%d for type=%d (num=%d)\n",
                                                                           field.size, field.type, field.num); }

                    // jump over unaccounted for data
                    read_unknown( field.size-size, &count );
                }

                // add to the container
                values.push_back(value);

            }

            // we have now extracted the data stored in the message- so lets pass it to the decoders
            // to handle- in this case we use the global message number to decide since it indicates
            // the associated profile and semantics that would apply

            if (FIT_DEBUG && FIT_DEBUG_LEVEL>1) {
                if (def.global_msg_num != 20 || (FIT_DEBUG_LEVEL>2 && def.global_msg_num==20)) {

                    fprintf(stderr, "** Decoding message '%s' (local=%d global=%d)\n",
                                    fitMessageDesc(def.global_msg_num, false).toStdString().c_str(),
                                    def.local_msg_num, def.global_msg_num);
                }
            }

            // first lets run a generic decode for XDATA using metadata
            decodeGeneric(fitMessageDesc(def.global_msg_num,true), def, time_offset, values);

            // now for hand-crafted parsing that aligns to RideFile and its needs
            switch (def.global_msg_num) {
                case FILE_ID_MSG_NUM: // #0
                    decodeFileId(def, time_offset, values);
                    break;

                case SESSION_MSG_NUM: // #18
                    decodeSession(def, time_offset, values);
                    break;

                case LAP_MSG_NUM: // #19
                    decodeLap(def, time_offset, values);
                    break;

                case RECORD_MSG_NUM: // #20
                    decodeRecord(def, time_offset, values);
                    break;

                case EVENT_MSG_NUM: // #21
                    decodeEvent(def, time_offset, values);
                    break;

                case 23:
                    decodeDeviceInfo(def, time_offset, values); /* device info */
                    break;

                case ACTIVITY_MSG_NUM: // #34
                    decodeActivity(def, time_offset, values);
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

                case SEGMENT_MSG_NUM: // #142
                    decodeSegment(def, time_offset, values); /* segment data */
                    break;

                case 206: // Developer Field Description
                    decodeDeveloperFieldDescription(def, time_offset, values);
                    break;

                case 140: /* undocumented Garmin/Firstbeat specific metrics */
                    decodePhysiologicalMetrics(def, time_offset, values);
                    break;

                case 207: /* Developer ID */
                    decodeDeveloperID(def, time_offset, values);
                    break;

                case 1: /* capabilities, device settings and timezone */ break;
                case 2:
                    decodeDeviceSettings(def, time_offset, values);
                    break;

                case 3: /* USER_PROFILE */
                    decodeUserProfile(def, time_offset, values);
                    break;

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
                case 35: /* software */
                case 37: /* file capabilities */
                case 38: /* message capabilities */
                case 39: /* field capabilities */
                case 49: /* file creator */
                case 51: /* blood pressure */
                case 53: /* speed zone */
                case 55: /* monitoring */
                case 72: /* training file (undocumented) : new since garmin 800 */
                    break;

                case HRV_MSG_NUM:
                    decodeHRV(def, values);
                    break; /* hrv */

                case 79: /* HR zone (undocumented) ; see details below: */
                         /* #253: timestamp / #1: default Min HR / #2: default Max HR / #5: user Min HR / #6: user Max HR */
                case 103: /* monitoring info */
                case 104: /* battery */
                case 105: /* pad */
                case 106: /* salve device */
                case 113: /* unknown */

                case 125: /* unknown */
                case 131: /* cadence zone */

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
                    break;

                default:

                    // I'm not sure we really care if the global number is now known
                    // since they could be from a product profile - this is not
                    // as big an issue these days, but lets keep warning if we really
                    // don't recognise it
                    if (fitMessageDesc(def.global_msg_num,false) == "Unknown message type")
                        unknown_global_msg_nums.insert(def.global_msg_num);
                    break;
            }
            last_msg_type = def.global_msg_num;
        }
        return count;
    }

    //
    // MAIN ENTRY POINT INTO PARSER
    //
    // Kind of neat that you have to scroll down to line 4k or so to find it
    //
    //
    RideFile * run() {

        // get the Smart Recording parameters
        isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);
        GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
        if (GarminHWM.isNull() || GarminHWM.toInt() == 0) GarminHWM.setValue(25); // default to 25 seconds.

        // start
        rideFile = new RideFile;
        rideFile->setDeviceType("Garmin FIT");
        rideFile->setFileFormat("Flexible and Interoperable Data Transfer (FIT)");
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

        hrvXdata = new XDataSeries();
        hrvXdata->name = "HRV";
        hrvXdata->valuename << "R-R";
        hrvXdata->unitname << "msecs";

        gearsXdata = new XDataSeries();
        gearsXdata->name = "GEARS";
        gearsXdata->valuename << "FRONT";
        gearsXdata->unitname << "t";
        gearsXdata->valuename << "REAR";
        gearsXdata->unitname << "t";
        gearsXdata->valuename << "FRONT-NUM";
        gearsXdata->unitname << "";
        gearsXdata->valuename << "REAR-NUM";
        gearsXdata->unitname << "";

        positionXdata = new XDataSeries();
        positionXdata->name = "POSITION";
        positionXdata->valuename << "POSITION";
        positionXdata->unitname << "positiontype";

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
                Q_UNUSED(e)
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
                    Q_UNUSED(e)
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
                                Q_UNUSED(e)
                                errors << "truncated second file body";
                            }
                        }
                        if (!truncated) {
                            try {
                                int crc = read_uint16( false ); // always littleEndian
                                (void) crc;
                            }
                            catch (TruncatedRead &e) {
                                Q_UNUSED(e)
                                errors << "truncated file body";
                                return NULL;
                            }
                        }
                    }
                }
                catch (TruncatedRead &e) {
                    Q_UNUSED(e)
                }

            }

            foreach(int num, unknown_global_msg_nums)
                qDebug() << QString("FitRideFile: unknown global message number %1; ignoring it").arg(num);
            foreach(int num, unknown_record_fields)
                qDebug() << QString("FitRideFile: unknown record field %1; ignoring it").arg(num);
            foreach(int num, unknown_base_type)
                qDebug() << QString("FitRideFile: unknown base type %1; skipped").arg(num);

            setRideFileDeviceInfo(rideFile, deviceInfos);
            setRideFileDataInfo(rideFile, dataInfos);

            // CIQINFO
            if (local_deve_fields_app.count()>0) {

                foreach(FitDeveApp deve_app, local_deve_fields_app) {
                    CIQinfo info(deve_app.app_id.c_str(),
                                 deve_app.dev_data_id,
                                 deve_app.app_version);

                    foreach(FitFieldDefinition deve_field, deve_app.fields) {
                        info.fields << CIQfield(deve_field.message.c_str(), //always record atm
                                                deve_field.name.c_str(),
                                                deve_field.native,
                                                deve_field.num,
                                                FITbasetypes[deve_field.type],
                                                deve_field.unit.c_str(),
                                                deve_field.scale,
                                                deve_field.offset);
                    }
                    rideFile->addCIQ(info);
                }
                //add ciq infos as tag

                if (!rideFile->ciqinfo().empty() ) {

                    QString ciqString = CIQinfo::listToJson( rideFile->ciqinfo());

                    QString escaped = QJsonValue(ciqString).toVariant().toString();

                    rideFile->setTag("CIQ", escaped );
                }
            }

            file.close();

            appendXData(rideFile);

            if (rideFile->xdata("SWIM")) {
                // Build synthetic kph, km and cad sample data for Lap Swims
                DataProcessor* fixLapDP = DataProcessorFactory::instance().getProcessors(true).value("::FixLapSwim");
                if (fixLapDP) fixLapDP->postProcess(rideFile, NULL, "NEW");
                else qDebug()<<"Fix Lap Swim Data Processor not found.";
            }

            return rideFile;
        }
    }
};

// Section 2 - the main entry point for all ride file readers
//
//              openRideFile() is the main entry point used across the
//              codebase for filereaders
//
RideFile *FitFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*> *rides) const
{
    // prepare the metadata first
    loadMetadata();

    QSharedPointer<FitFileParser> state(new FitFileParser(file, errors));
    RideFile* ret = state->run();
    // Split sessions, only if we have a valid RideFile
    if (ret) ret = state->splitSessions(rides);
    return ret;
}


// Section 3 - FIT file writer
//
//              For exporting to FIT format, this is particularly useful
//              when sharing data with the cloud or other applications
//

void write_int8(QByteArray* array, fit_value_t value) {
    array->append(value);
}
void write_uint8(QByteArray* array, fit_value_t value) {
    array->append(value);
}

void write_int16(QByteArray *array, fit_value_t value,  bool is_big_endian) {
    value = is_big_endian
        ? qFromBigEndian<qint16>( value )
        : qFromLittleEndian<qint16>( value );

    for (int i=0; i<16; i=i+8) {
        array->append(value >> i);
    }
}

void write_uint16(QByteArray *array, fit_value_t value,  bool is_big_endian) {
    value = is_big_endian
        ? qFromBigEndian<quint16>( value )
        : qFromLittleEndian<quint16>( value );

    for (int i=0; i<16; i=i+8) {
        array->append(value >> i);
    }
}

void write_int32(QByteArray *array, fit_value_t value,  bool is_big_endian) {
    value = is_big_endian
        ? qFromBigEndian<qint32>( value )
        : qFromLittleEndian<qint32>( value );



    for (int i=0; i<32; i=i+8) {
        array->append(value >> i);
    }
}

void write_uint32(QByteArray *array, fit_value_t value,  bool is_big_endian) {
    value = is_big_endian
        ? qFromBigEndian<quint32>( value )
        : qFromLittleEndian<quint32>( value );



    for (int i=0; i<32; i=i+8) {
        array->append(value >> i);
    }
}

void write_float32(QByteArray* array, float f, bool is_big_endian) {
    if (is_big_endian) {
        f = qbswap(f);
    }

    uint32_t* p = (uint32_t*)&f;
    for (int i = 0; i < 32; i = i + 8) {
        array->append((*p) >> i);
    }
}

// Zero pad string to length of field (len)
void write_string(QByteArray* array, const char* str, int len) {
    size_t slen = strlen(str);
    array->append(str, slen);
    array->append(len - slen, 0);
}

uint16_t crc16(char *buf, int len)
{
  uint16_t crc = 0x0000;

  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos] & 0xff;

    for (int i = 8; i != 0; i--) {      // Each bit
        if ((crc & 0x0001) != 0) {      // LSB set
          crc >>= 1;                    // Shift right
          crc ^= 0xA001;                // XOR 0xA001
        }
        else
          crc >>= 1;                    // Shift right
     }
  }

  return crc;
}

// TODO check protocol version
void write_header(QByteArray* array, quint32 data_size) {
    quint8 header_size = 14;
    quint8 protocol_version = 16;
    quint16 profile_version = 1320; // always littleEndian

    write_int8(array, header_size);
    write_int8(array, protocol_version);
    write_int16(array, profile_version, false);
    write_int32(array, data_size, false);
    array->append(".FIT");

    uint16_t header_crc = crc16(array->data(), array->length());
    write_int16(array, header_crc, false);
}

void write_message_definition(QByteArray *array, int global_msg_num, int local_msg_typ, int num_fields) {
    // Definition ------
    write_int8(array, DEFINITION_MSG_HEADER + local_msg_typ); // definition_header
    write_int8(array, 0); // reserved
    write_int8(array, 1); // is_big_endian
    write_int16(array, global_msg_num, true);
    write_int8(array, num_fields);
}

void write_field_definition(QByteArray *array, int field_num, int field_size, int base_type) {
    write_int8(array, field_num);
    write_int8(array, field_size);
    write_int8(array, base_type);
}

void write_file_id(QByteArray *array, const RideFile *ride) {
    // 0	type
    // 1	manufacturer
    // 2	product/garmin_product
    // 3	serial_number
    // 4	time_created
    // 5	number
    // 8	product_name

    write_message_definition(array, FILE_ID_MSG_NUM, 0, 6); // global_msg_num, local_msg_type, num_fields

    write_field_definition(array, 0, 1, 0); // 1. type (0)
    write_field_definition(array, 1, 2, 132); // 2. manufacturer (1)
    write_field_definition(array, 2, 2, 132); // 3. product (2)
    write_field_definition(array, 4, 4, 134); // 4. time_created (4)

    write_field_definition(array, 3, 4, 134); // 5. serial_number (3)
    write_field_definition(array, 5, 2, 132); // 6. number (5)


    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    // 1. type
    int value = 4; //file:activity
    write_int8(array, value);

    // 2. manufacturer
    value = 0;
    write_int16(array, value, true);

    // 3. product
    value = 0;
    write_int16(array, value, true);

    // 4. time_created
    value = ride->startTime().toSecsSinceEpoch() - qbase_time.toSecsSinceEpoch();
    write_int32(array, value, true);

    // 5. serial
    value = 0;
    write_int32(array, value, true);

    // 6. number
    value = 65535; //NA
    write_int16(array, value, true);
}

void write_file_creator(QByteArray *array) {
    // 0	software_version	uint16
    // 1	hardware_version	uint8

    write_message_definition(array, FILE_CREATOR_MSG_NUM, 0, 2); // global_msg_num, local_msg_type, num_fields

    write_field_definition(array, 0, 2, 132); // 1. software_version (0)
    write_field_definition(array, 1, 1, 2); // 1. hardware_version (0)


    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    // 1. software_version
    int value = 100;
    write_int16(array, value, true);

    // 1. hardware_version
    value = 255; // NA
    write_int8(array, value);

}

void write_session(QByteArray* array, const RideFile* ride, QHash<QString, RideMetricPtr> computed) {

    write_message_definition(array, SESSION_MSG_NUM, 0, 10); // global_msg_num, local_msg_type, num_fields

    write_field_definition(array, 253, 4, 134); // timestamp (253)
    write_field_definition(array, 254, 2, 132); // message_index (254)
    write_field_definition(array, 0, 1, 2); // event (0)
    write_field_definition(array, 1, 1, 2); // event_type (1)
    write_field_definition(array, 2, 4, 134); // start_time (2)
    write_field_definition(array, 5, 1, 0); // sport (5)
    write_field_definition(array, 6, 1, 2); // subsport (6)
    write_field_definition(array, 7, 4, 134); // total_elapsed_time (7)
    write_field_definition(array, 9, 4, 134); // total_distance (9)
    write_field_definition(array, 28, 1, 0); // trigger (28)

    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    // 1. timestamp (253)
    int value = ride->startTime().toSecsSinceEpoch() - qbase_time.toSecsSinceEpoch();;
    if (ride->dataPoints().count() > 0) {
        value += ride->dataPoints().last()->secs+ride->recIntSecs();
    }
    write_int32(array, value, true);

    // 2. message_index (254)
    write_int16(array, 0, true);

    // 3. event (0)
    value = 9; // lap (8=session)
    write_int8(array, value);

    // 4. event_type (1)
    value = 1; // stop (4=stop_all)
    write_int8(array, value);

    // 5. start_time (4)
    value = ride->startTime().toSecsSinceEpoch() - qbase_time.toSecsSinceEpoch();
    write_int32(array, value, true);

    // 6. sport
    value=FitFileParser::getSportId(ride->sport());
    write_int8(array, value);

    // 7. sub sport
    value=FitFileParser::getSubSportId(ride->getTag("SubSport",""));
    write_int8(array, value);

    // 8. total_elapsed_time (7)
    value = computed.value("workout_time")->value(true) * 1000;
    write_int32(array, value, true);

    // 9. total_distance (9)
    value = computed.value("total_distance")->value(true) * 100000;
    write_int32(array, value, true);

    // 10. trigger
    write_int8(array, 0); // activity end

}

void write_lap(QByteArray *array, const RideFile *ride) {
    write_message_definition(array, LAP_MSG_NUM, 0, 6); // global_msg_num, local_msg_type, num_fields

    write_field_definition(array, 253, 4, 134); // timestamp (253)
    write_field_definition(array, 254, 2, 132); // message_index (254)
    write_field_definition(array, 0, 1, 2); // event (0)
    write_field_definition(array, 1, 1, 2); // event_type (1)
    write_field_definition(array, 2, 4, 134); // start_time (2)
    write_field_definition(array, 24, 1, 2); // trigger (24)


    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    // 1. timestamp
    int value = ride->startTime().toSecsSinceEpoch() - qbase_time.toSecsSinceEpoch();;
    if (ride->dataPoints().count() > 0) {
        value += ride->dataPoints().last()->secs+ride->recIntSecs();
    }
    write_int32(array, value, true);

    // 2. message_index (254)
    write_int16(array, 0, true);

    // 3. event
    value = 9; // session=8, lap=9
    write_int8(array, value);

    // 4. event_type
    value = 1; // stop_all=9, stop=1
    write_int8(array, value);

    // 5. start_time
    value = ride->startTime().toSecsSinceEpoch() - qbase_time.toSecsSinceEpoch();;
    write_int32(array, value, true);

    // 6. trigger
    value = 7; // session_end
    write_int8(array, value);

}

void write_start_event(QByteArray *array, const RideFile *ride) {

    write_message_definition(array, EVENT_MSG_NUM, 0, 5); // global_msg_num, local_msg_type, num_fields

    write_field_definition(array, 253, 4, 134); // timestamp (253)
    write_field_definition(array, 0, 1, 2); // event (0)
    write_field_definition(array, 1, 1, 2); // event_type (1)
    write_field_definition(array, 3, 4, 134); // data (3)
    write_field_definition(array, 4, 1, 2); // event_group (4)


    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    // 1. timestamp
    int value = ride->startTime().toSecsSinceEpoch() - qbase_time.toSecsSinceEpoch();
    write_int32(array, value, true);

    // 2. event
    value = 0;
    write_int8(array, value);

    // 3. event_type
    value = 0;
    write_int8(array, value);

    // 4. data
    write_int32(array, 0, true);

    // 5. event_group
    write_int8(array, 0);

}

void write_stop_event(QByteArray *array, const RideFile *ride) {

    write_message_definition(array, EVENT_MSG_NUM, 0, 5); // global_msg_num, local_msg_type, num_fields

    write_field_definition(array, 253, 4, 134); // timestamp (253)
    write_field_definition(array, 0, 1, 2); // event (0)
    write_field_definition(array, 1, 1, 2); // event_type (1)
    write_field_definition(array, 3, 4, 134); // data (3)
    write_field_definition(array, 4, 1, 2); // event_group (4)


    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    // 1. timestamp
    int value = ride->startTime().toSecsSinceEpoch() + 2 - qbase_time.toSecsSinceEpoch();
    write_int32(array, value, true);

    // 2. event
    value = 8; // timer=0
    write_int8(array, value);

    // 3. event_type
    value = 9; // stop_all=4
    write_int8(array, value);

    // 4. data
    write_int32(array, 1, true);

    // 5. event_group
    write_int8(array, 1);

}

void write_activity(QByteArray *array, const RideFile *ride, QHash<QString,RideMetricPtr> computed) {

    write_message_definition(array, ACTIVITY_MSG_NUM, 0, 6); // global_msg_num, local_msg_type, num_fields

    write_field_definition(array, 253, 4, 134); // timestamp (253)
    write_field_definition(array, 0, 4, 134); // total_timer_time (0)
    write_field_definition(array, 1, 2, 132); // num_sessions (1)
    write_field_definition(array, 2, 1, 2); // type (2)
    write_field_definition(array, 3, 1, 2); // event (3)
    write_field_definition(array, 4, 1, 2); // event_type (4)


    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    // 1. timestamp
    int value = ride->startTime().toSecsSinceEpoch() - qbase_time.toSecsSinceEpoch();
    if (ride->dataPoints().count() > 0) {
        value += ride->dataPoints().last()->secs+ride->recIntSecs();
    }
    write_int32(array, value, true);

    // 2. total_timer_time
    value = computed.value("workout_time")->value(true) * 1000;
    write_int32(array, value, true);

    // 3. num_sessions
    value = 1;
    write_int16(array, value, true);

    // 4. type
    value = 0; // manual
    write_int8(array, value);

    // 5. event
    value = 26; // activity
    write_int8(array, value);

    // 6. event_type
    value = 1; // stop
    write_int8(array, value);
}

// Add developer field definitions

int write_dev_fields(QByteArray* array, const RideFile* ride, int local_msg_type)
{
    int num_fields = 0;

    const QList<CIQinfo> &ciqlist = ride->ciqinfo();
    //Do we have developer field definitions?
    if (!ciqlist.empty()){
        foreach (CIQinfo ciqinfo, ciqlist)
        {
            int dev_idx = ciqinfo.devid;
            QByteArray* fields = new QByteArray();

            // Minimal developer header
            write_message_definition(array, 207, local_msg_type, 3); // global_msg_num, local_msg_type, num_fields
            write_field_definition(array, 1, 16, 13); // application id
            write_field_definition(array, 3, 1, 2);   // developer index
            write_field_definition(array, 4, 4, 6);   // app_version

            QString tmpid(ciqinfo.appid);
            tmpid.remove('-');//why bother adding the hyphens?
            QByteArray byteArray = QByteArray::fromHex(tmpid.toUtf8());

            write_int8(array, 0);
            array->append((const char*)byteArray.data(), 16);
            write_int8(array, dev_idx);
            write_int32(array, ciqinfo.ver, true);

            //Add field definitions
            write_field_definition(fields, 3, 64, 0x07); // field name
            write_field_definition(fields, 8, 16, 0x07); // units
            write_field_definition(fields, 14, 2, 0x84); // native-msg (20)
            write_field_definition(fields, 1, 1, 0x02);  // field_definition id
            write_field_definition(fields, 2, 1, 0x02);  // fit base type
            write_field_definition(fields, 15, 1, 0x02); // native field#
            write_field_definition(fields, 0, 1, 0x02);  // developer id            
            write_field_definition(fields, 6, 1, 0x02); // scale - uint
            write_field_definition(fields, 7, 1, 0x01); // offset - sint

            foreach (CIQfield field, ciqinfo.fields) {
                write_message_definition(array, FIELD_DESCRIPTION, local_msg_type, 9);
                array->append(fields->data(), fields->size());
                write_int8(array, 0);
                write_string(array, field.name.toStdString().c_str(), 64);
                write_string(array, field.unit.toStdString().c_str(), 16);
                write_int16(array, 20, true);                             // record type
                write_int8(array, field.id);                              // Local num
                write_int8(array, typeToFIT(field.type));                 // type
                write_int8(array, field.nativeid);                        // Native num
                write_int8(array, dev_idx);                               // developer id
                write_uint8(array, (field.scale < 0) ? 255 : field.scale);  // Scale
                write_int8(array, (field.offset < 0) ? 127 : field.offset); // Offset

                num_fields++;
            }
        }
    }
    return num_fields;
}

void write_record_definition(QByteArray *array, const RideFile *ride, QMap<int, int> *local_msg_type_for_record_type, bool withAlt, bool withWatts, bool withHr, bool withCad, int type) {
    int num_fields = 1;
    QByteArray *fields = new QByteArray();

    write_field_definition(fields, 253, 4, 134); // timestamp (253)

    if ( ride->areDataPresent()->lat ) {
        num_fields ++;
        write_field_definition(fields, 0, 4, 133); // position_lat (0)

        num_fields ++;
        write_field_definition(fields, 1, 4, 133); // position_long (2)
    }
    if ( withAlt && ride->areDataPresent()->alt ) {
        num_fields ++;
        write_field_definition(fields, 2, 2, 132); // altitude (2) 4 0x84
    }
    if ( withHr && ride->areDataPresent()->hr ) {
        num_fields ++;
        write_field_definition(fields, 3, 1, 2); // heart_rate (3)
    }
    if ( withCad && (ride->areDataPresent()->cad || (ride->isRun() && ride->areDataPresent()->rcad)) ) {
        num_fields ++;
        write_field_definition(fields, 4, 1, 2); // cadence (4)
    }
    if ( ride->areDataPresent()->km ) {
        num_fields ++;
        write_field_definition(fields, 5, 4, 134); // distance (5)
    }
    if ( ride->areDataPresent()->kph ) {
        num_fields ++;
        write_field_definition(fields, 6, 2, 132); // speed (6)
    }
    if ( withWatts && ride->areDataPresent()->watts ) {
        num_fields ++;
        write_field_definition(fields, 7, 2, 132); // power (7)
    }
    // can be NA...
    if ( (type&1)==1 ) {
        num_fields ++;
        write_field_definition(fields, 13, 1, 2); // temperature (13)
    }
    if ( (type&2)==2 ) {
        num_fields ++;
        write_field_definition(fields, 30, 1, 2); // left_right_balance (30)
    }

    int local_msg_type = local_msg_type_for_record_type->values().count()+1;

    // Do we need our developer fields adding?
    bool withDev = false;
    const QList<CIQinfo> &ciqlist = ride->ciqinfo();

    //Do we have developer field definitions?
    if (!ciqlist.empty()){
        withDev = true;
        int numfields = write_dev_fields(array, ride, 0); // add custom field definitions

        write_int8(fields, numfields);

        foreach (CIQinfo ciqinfo, ciqlist)
        {
            int dev_idx = ciqinfo.devid;

            foreach (CIQfield field, ciqinfo.fields)
            {
                write_field_definition(fields, field.id, typeLength(field.type), dev_idx);
            }
        }
    }
    
    // Add developer field flag to header
    write_message_definition(array, RECORD_MSG_NUM, local_msg_type | (withDev ? 32 : 0), num_fields); // global_msg_num, local_msg_type, num_fields

    array->append(fields->data(), fields->size());

    local_msg_type_for_record_type->insert(type, local_msg_type);
}

void write_record(QByteArray *array, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad ) {
    QMap<int, int> *local_msg_type_for_record_type = new QMap<int, int>();

    int xdata_cur=0; //cursor into core xdata
    // Record ------
    foreach (const RideFilePoint *point, ride->dataPoints()) {
        int type = 0;
        // Temperature and lrbalance can be NA
        if ( ride->areDataPresent()->temp && point->temp != RideFile::NA) {
            type += 1;
        }
        if ( ride->areDataPresent()->lrbalance && point->lrbalance != RideFile::NA) {
            type += 2;
        }

        // Add record definition for this type of record
        if (local_msg_type_for_record_type->value(type, -1)==-1)
            write_record_definition(array, ride, local_msg_type_for_record_type, withAlt, withWatts, withHr, withCad, type);
        int record_header = local_msg_type_for_record_type->value(type, 1);

        // RidePoint
        QByteArray *ridePoint = new QByteArray();
        write_int8(ridePoint, record_header);

        int value = point->secs + ride->startTime().toSecsSinceEpoch() - qbase_time.toSecsSinceEpoch();
        write_int32(ridePoint, value, true);

        if ( ride->areDataPresent()->lat ) {
            write_int32(ridePoint, point->lat * 0x7fffffff / 180, true);
            write_int32(ridePoint, point->lon * 0x7fffffff / 180, true);
        }
        if ( withAlt && ride->areDataPresent()->alt ) {
            write_int16(ridePoint, (point->alt+500) * 5, true);
        }
        if ( withHr && ride->areDataPresent()->hr ) {
            write_int8(ridePoint, point->hr);
        }
        if ( withCad && ride->areDataPresent()->cad ) {
            write_int8(ridePoint, point->cad);
        }
        // In runs, cadence is saved in 'rcad' instead of 'cad'
        else if (withCad && ride->isRun() && ride->areDataPresent()->rcad){
            write_int8(ridePoint, point->rcad);
        }
        if ( ride->areDataPresent()->km ) {
            write_int32(ridePoint, point->km * 100000, true);
        }
        if ( ride->areDataPresent()->kph ) {
            write_int16(ridePoint, point->kph / 3.6 * 1000, true);
        }
        if ( withWatts && ride->areDataPresent()->watts ) {
            write_int16(ridePoint, point->watts, true);
        }

        // temp and lrbalance can be NA... Not present for a point even if present in RideFile
        if ( (type&1)==1) {
            write_int8(ridePoint, point->temp);
        }
        if ( (type&2)==2 ) {
            // write right power contribution
            write_int8(ridePoint, 0x80 + (100-point->lrbalance));
        }

        const QList<CIQinfo> &ciqlist = ride->ciqinfo();
        //Do we have developer field definitions?
        if (!ciqlist.empty()){
            foreach (CIQinfo ciqinfo, ciqlist)
            {
                int dev_idx = ciqinfo.devid;
                static_cast<void>(dev_idx);

                foreach (CIQfield ciqfield, ciqinfo.fields)
                {
                    double val = ride->xdataValue(point, xdata_cur, "DEVELOPER", ciqfield.name, RideFile::REPEAT);
                    int id = FITbasetypes.indexOf(ciqfield.type);
                    switch (id){ //limited set supported
                    case 1:
                        write_int8(ridePoint, val);
                        break;
                    case 2:
                        write_uint8(ridePoint, val);
                        break;
                    case 3:
                        write_int16(ridePoint, val, true);
                        break;
                    case 4:
                        write_uint16(ridePoint, val, true);
                        break;
                    case 5:
                        write_int32(ridePoint, val, true);
                        break;
                    case 6:
                        write_uint32(ridePoint, val, true);
                        break;
                    case 8:
                        write_float32(ridePoint, val, true);
                        break;
                    default:
                        fprintf(stderr,"ERROR TYPE %s CURRENTLY UNSUPPORTED\n",ciqfield.type.toStdString().c_str()); fflush(stderr);
                    }
                }
            }
        }
        array->append(ridePoint->data(), ridePoint->size());
    }

    delete local_msg_type_for_record_type;
}

QByteArray
FitFileReader::toByteArray(Context *context, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad) const
{
    const char *metrics[] = {
        "total_distance",
        "workout_time",
        "total_work",
        "average_hr",
        "max_heartrate",
        "average_cad",
        "max_cadence",
        "average_power",
        "max_power",
        "max_speed",
        "average_speed",
        NULL
    };

    QStringList worklist = QStringList();
    for (int i=0; metrics[i];i++) worklist << metrics[i];

    QHash<QString,RideMetricPtr> computed;
    if (context) { // can't do this standalone
        RideItem *tempItem = new RideItem(const_cast<RideFile*>(ride), context);
        computed = RideMetric::computeMetrics(tempItem, Specification(), worklist);
    }

    QByteArray array;
    QByteArray data;

    // An activity file shall contain file_id, activity, session, and lap messages.
    // The file may also contain record, event, length and/or hrv messages.
    // All data messages in an activity file (other than hrv) are related by a timestamp.

    write_file_id(&data, ride); // file_id 0
    write_file_creator(&data); // file_creator 49
    write_start_event(&data, ride); // event 21 (x15)
    write_record(&data, ride, withAlt, withWatts, withHr, withCad); // record 20 (x14)  
    write_lap(&data, ride); // lap 19 (x11)
    write_stop_event(&data, ride); // event 21 (x15)
    write_session(&data, ride, computed); // session 18 (x12)
    write_activity(&data, ride, computed); // activity 34 (x22)

    write_header(&array, data.size());
    array += data;

    uint16_t array_crc = crc16(array.data(), array.length());
    write_int16(&array, array_crc, false);

    return array;
}

bool
FitFileReader::writeRideFile(Context *context, const RideFile *ride, QFile &file) const
{
    QByteArray content = toByteArray(context, ride, true, true, true, true);

    if (!file.open(QIODevice::WriteOnly)) return(false);
    file.resize(0);
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    file.write(content);
    file.close();
    return(true);
}


