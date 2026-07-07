/*
 * Copyright (c) 2010 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _FitRideFile_h
#define _FitRideFile_h
#include "GoldenCheetah.h"

#include "RideFile.h"

struct FitFileReader : public RideFileReader {

    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*> *rides = 0) const;

    QByteArray toByteArray(Context *context, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad) const;
    bool writeRideFile(Context *context, const RideFile *ride, QFile &file) const;
    bool hasWrite() const { return true; }

};

#ifndef MATHCONST_PI
#define MATHCONST_PI            3.141592653589793238462643383279502884L /* pi */
#endif

#define DEFINITION_MSG_HEADER   64
#define FILE_ID_MSG_NUM         0
#define SESSION_MSG_NUM         18
#define LAP_MSG_NUM             19
#define RECORD_MSG_NUM          20
#define EVENT_MSG_NUM           21
#define ACTIVITY_MSG_NUM        34
#define FILE_CREATOR_MSG_NUM    49
#define HRV_MSG_NUM             78
#define SEGMENT_MSG_NUM         142
#define FIELD_DESCRIPTION       206

/* FIT has uint32 as largest integer type. So qint64 is large enough to
 * store all integer types - no matter if they're signed or not */

// this will need to change if float or other non-integer values are
// introduced into the file format *FIXME*
typedef qint64 fit_value_t;

#define NA_VALUE std::numeric_limits<fit_value_t>::max()
#define NA_VALUEF  (double)(0xFFFFFFFF)

typedef std::string fit_string_value;
typedef float fit_float_value;

struct FitField {
    int num;
    int type; // FIT base_type
    int size; // in bytes
    int deve_idx; // Developer Data Index
};

struct FitFieldDefinition {
    int dev_id; // Developer Data Index (for developer fields)
    int num;
    int type; // FIT base_type
    bool istime; // for date_time fields with base type uint32
    int size; // in bytes
    int native; // native field number
    double scale; // should be a double not an int
    int offset;
    fit_string_value message; // what message section does this belong to
    fit_string_value name; // what is the name of the field
    fit_string_value unit; // what units does the field use
};

struct FitDeveApp {
    fit_string_value dev_id; // Developer Data Index
    fit_string_value app_id; // application_id
    int man_id; // manufacturer_id
    int dev_data_id; // developer_data_index
    qint64 app_version; // application_version

    QList<FitFieldDefinition> fields;
};

struct FitMessage {
    int global_msg_num;
    int local_msg_num;
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

// Fit types metadata
struct FITproduct { int manu, prod; QString name; };
struct FITmanufacturer { int manu; QString name; };
struct FITmessage { int num; QString desc; };

#endif // _FitRideFile_h

