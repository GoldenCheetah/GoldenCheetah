/*
* Copyright (c) 2020 Eric Christoffersen (impolexg@outlook.com)
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

// Data reader adapted to C++ from Wattzap Community Edition Java source code.

#include "TTSReader.h"
#include "LocationInterpolation.h"

#include <map>
#include <cstring>

// -------------------------------------------------------------
// LOG CONTROL
//
// Key is that it all goes away if disabled and optimized build.
// -------------------------------------------------------------

// Uncomment for debug logging
//#define DEBUG_PRINT

// Uncomment this too for more debug logging
//#define DEBUG_PRINT_VERBOSE

class log_disabled_output {};
static log_disabled_output log_disabled_output_instance;

template<typename T>
const log_disabled_output& operator << (const log_disabled_output& any, T const& thing) { Q_UNUSED(thing); return any; }

// std::endl simple, quick and dirty
const log_disabled_output& operator << (const log_disabled_output& any, const std::ostream&(*)(std::ostream&)) { return any; }

#ifdef DEBUG_PRINT
#include <QDebug>
#define DEBUG_LOG qDebug()
#else
#define DEBUG_LOG log_disabled_output_instance
#endif

#ifdef VERBOSE_DEBUG_PRINT
#define DEBUG_LOG_VERBOSE DEBUG_LOG
#else
#define DEBUG_LOG_VERBOSE log_disabled_output_instance
#endif

// -------------------------------------------------------------
// END LOG CONTROL
// -------------------------------------------------------------

using namespace NS_TTSReader;

// This is the static const string table that tts files may refer to by string key.
static const std::map<int, const char*> tts_string_table = {
    {1001, "route name" },
    {1002, "route description"},
    {1041, "segment name"},
    {1042, "segment description"},
    {2001, "company"},
    {2004, "serial"},
    {2005, "time"},
    {2007, "link"},
    {5001, "product"},
    {5002, "video name"},
    {6001, "infobox #1"}
};

/*
 * Utility Routines For TTS Reader
 */

 // Handy utils
float AsFloat(unsigned u) {
    float t;
    static_assert(sizeof(u) == sizeof(t), "Error: mismatched reinterpret sizes");
    memcpy(&t, &u, sizeof(u));
    return t;
}

float AsFloat(int i) {
    float t;
    static_assert(sizeof(i) == sizeof(t), "Error: mismatched reinterpret sizes");
    memcpy(&t, &i, sizeof(i));
    return t;
}

int toUInt(Byte b) {
    return reinterpret_cast<UByte&>(b);
}

int getUByte(ByteArray &buffer, int offset) {
    return (UByte)(buffer[offset]);
}

int getUShort(ByteArray buffer, size_t offset) {
    if((offset + 1) < buffer.size())
        return *(unsigned short*)&buffer[offset];    
    return getUByte(buffer, offset);
}

int getUInt(ByteArray &buffer, size_t offset) {
    if ((offset + 3) < buffer.size()) {
        return *(unsigned*)&buffer[offset];
    }
    return getUShort(buffer, offset);
}

bool isHeader(ByteArray &buffer) {
    if (buffer.size() < 2) {
        return false;
    }
    return getUShort(buffer, 0) <= 20;
}

void rehashKey(IntArray &A_0, int A_1, IntArray &outArray) {
    unsigned int i;
    CharArray chArray1;
    chArray1.resize(A_0.size() / 2);

    for (i = 0; i < chArray1.size(); i++) {
        chArray1[i] = (Char)(A_0[2 * i] + 256 * A_0[2 * i + 1]);
    }

    int num1 = 1000170181 + A_1;
    int num2 = 0;
    //int num3 = 1; ?
    while ((unsigned)num2 < chArray1.size()) {
        int index1 = num2;
        CharArray &chArray2 = chArray1;
        int index2 = index1;
        int num4 = (int)(short)chArray1[index1];
        int num5 = (int)255;
        int num6 = num4 & num5;
        int num7 = num1;
        int num8 = 1;
        int num9 = num7 + num8;
        Byte num10 = (Byte)(num6 ^ num7);
        int num11 = 8;
        int num12 = num4 >> num11;
        int num13 = num9;
        int num14 = 1;
        num1 = num13 + num14;
        int num15 = (int)(Byte)(num12 ^ num13);
        int num16 = (int)(toUInt(num10) << 8 | toUInt((Byte)num15)) & 0xffff;
        chArray2[index2] = (Char)num16;
        int num17 = 1;
        num2 += num17;
    }

    outArray.resize(chArray1.size());

    for (i = 0; i < outArray.size(); i++) {
        outArray[i] = (int)chArray1[i];
    }

    return;
}

bool encryptHeader(const IntArray &A_0, const IntArray &key2, IntArray &numArray) {

    const IntArray &bytes = key2;
    numArray.resize(bytes.size());

    unsigned int index1 = 0;
    unsigned int index2 = 0;
    unsigned int num = 6;

    while (true) {
        switch (num) {
        case 0:
            index1 = 0;
            num = 2;
            continue;
        case 1:
            if (index2 < bytes.size()) {
                numArray[index2] = (A_0[index1] ^ bytes[index2]);
                num = 5;
                continue;
            }
            else {
                num = 3;
                continue;
            }
        case 2:
            ++index2;
            num = 4;
            continue;
        case 3:
            return true;
        case 4:
        case 6:
            num = 1;
            continue;
        case 5:
            if (index1++ >= A_0.size() - 1) {
                num = 0;
                continue;
            }
            else {
                num = 2;
                continue;
            }
        default:
            return false;
        }
    }
}

bool decryptData(IntArray &A_0, IntArray &A_1, IntArray &numArray) {

    numArray.resize(A_0.size());

    size_t index = 0;
    int num1 = 5;
    size_t num2 = -1000;

    int e = 0; // set before each block
    while (true) {
        switch (num1) {
        case 0:
            return true;
        case 1:
            e = 0;
            num1 = 4;
            continue;

        case 2:
            if (index < A_0.size()) {
                numArray[index] = A_1[e] ^ A_0[index];
                num2 = e++;
                num1 = 6;
                continue;
            }
            else {
                num1 = 0;
                continue;
            }

        case 3:
        case 5:
            num1 = 2;
            continue;

        case 4:
            ++index;
            num1 = 3;
            continue;

        case 6:
            if (num2 >= A_1.size() - 1) {
                num1 = 1;
                continue;
            }
            else {
                num1 = 4;
                continue;
            }

        default:
            return false;
        }
    }
}

void iarr(const ByteArray &a, IntArray &r) {
    r.resize(0);
    for (unsigned int i = 0; i < a.size(); i++) {
        r.push_back((int)a[i]);
    }
}

void barr(IntArray &a, ByteArray &r) {
    r.resize(0);
    for (unsigned int i = 0; i < a.size(); i++) {
        r.push_back((Byte)a[i]);
    }
}

void videoInfo(int version, ByteArray & data) {
    // first three values could be shorts and then?
    Q_UNUSED(version);
    Q_UNUSED(data);
}

void TTSReader::segmentRange(int version, ByteArray &data) {

    Q_UNUSED(version);

    if (data.size() % 10 != 0) {
        DEBUG_LOG << "Segment Range data wrong length " << data.size() << "\n";
    }

    DEBUG_LOG << "[segment range token]";

    // In the files I've seen this segmentRange token is only set once and contains the route length.
    // Note this code seems to permit a segment to have multiple ranges. I have no example so not sure.
    for (unsigned int i = 0; i < data.size() / 10; i++) {

        double startKM = (getUInt(data, i * 10 + 0) / 100000.0);
        double endKM = (getUInt(data, i * 10 + 4) / 100000.0);

        DEBUG_LOG << " [" << i << "=" << startKM << "-" << endKM << "\n";

        if (getUShort(data, i * 10 + 6) != 0) {
            DEBUG_LOG << "/0x" << std::hex << (getUShort(data, i * 10 + 6)) << std::dec << "\n";
        }

        DEBUG_LOG << "]\n";
    }
}

// segment range; 548300 is 5.483km. What is short value in "old" files?
void TTSReader::segmentInfo(int version, ByteArray &data) {

    if ((version == 1104) && (data.size() == 8)) {
        unsigned startCM = getUInt(data, 0);
        unsigned endCM   = getUInt(data, 4);

        double startKM = (startCM / 100) / 1000.;
        double endKM   = (endCM   / 100) / 1000.;

        pendingSegment.startKM = startKM;
        pendingSegment.endKM   = endKM;

        DEBUG_LOG << "[segment range] " << startKM << "-" << endKM << "\n";
    }

    if ((version == 1000) && (data.size() == 10)) {
        unsigned startCM = getUInt(data, 2);
        unsigned endCM   = getUInt(data, 6);

        // NOTE: From the wattzapp debug print it looks like this 3rd value is a divisor.
        // In my test files its always 1. so no harm to divide - but beware I'm just guessing.
        double divisor = getUShort(data, 0);

        double startKM = ((startCM / 100) / 1000.) / divisor;
        double endKM   = ((endCM   / 100) / 1000.) / divisor;

        pendingSegment.startKM = startKM;
        pendingSegment.endKM   = endKM;

        DEBUG_LOG << "[segment range] " << (getUInt(data, 2) / 100000.0) << "-" << (getUInt(data, 6) / 100000.0) << "/" << getUShort(data, 0) << "\n";
    }
}

// 1 for "plain" RLV, 2 for ERGOs
void trainingType(int version, ByteArray &data) {

    if (version == 1004) {
        switch (data[5]) {
        case 1:
            DEBUG_LOG << "[video type] RLV\n";
            break; // ?
        case 2:
            DEBUG_LOG << "[video type] ERGO\n";
            break; // ?
        }
    }
}

void Point::print() const {
    DEBUG_LOG << "Point [latitude=" << latitude << ", longitude=" << longitude
              << ", elevation=" << elevation << ", distanceFromStart="
              << distanceFromStart << ", gradient=" << gradient << ", power="
              << power << ", speed=" << speed << ", time=" << time << "]\n";
}

//
// TTS Reader Method Impls
//

bool TTSReader::readData(QDataStream &is, ByteArray& buffer, bool copyPre) {
    size_t first = 0;
    if (copyPre) {
        buffer[0] = pre[0];
        buffer[1] = pre[1];
        first = 2;
    }

    if (buffer.size() != first) {
        size_t readSize = buffer.size() - first;

        size_t iBytesRead = is.readRawData((char*)&buffer[first], (int)buffer.size() - first);
        if (iBytesRead != readSize) return false;
    }

    return true;
}

const XYSeries &TTSReader::getSeries() const {

    // TODO Auto-generated method stub
    return series;
}

const std::vector<Point> & TTSReader::getPoints() const {
    return points;
}

const std::vector<Segment>& TTSReader::getSegments() const {
    return segmentList;
}

const std::wstring& TTSReader::getRouteName()        const {
    return routeName; 
}

const std::wstring& TTSReader::getRouteDescription() const {
    return routeDescription;
}

double TTSReader::getDistanceMeters() const {
    return totalDistance;
}

int TTSReader::routeType() const {
    return 0;// SLOPE;
}

double TTSReader::getMaxSlope() const {
    return maxSlope;
}

double TTSReader::getMinSlope() const {
    return minSlope;
}

bool TTSReader::deriveMinMaxSlopes(double &minSlope, double &maxSlope, double &variance) const {

    minSlope = 0;
    maxSlope = 0;

    double varianceSum = 0;

    if (fHasAlt) {
        for (unsigned u = 3; u < points.size(); u++) {
            double rise = points[u].getElevation() - points[u - 1].getElevation();
            double run = points[u].getDistanceFromStart() - points[u - 1].getDistanceFromStart();
            double s = run ? rise / run : 0;
            minSlope = std::min(s, minSlope);
            maxSlope = std::max(s, maxSlope);

            double deltaFromRecorded = run * (s - points[u].getGradient());

            varianceSum += (deltaFromRecorded * deltaFromRecorded);
        }

        variance = varianceSum / ((points.size() - 1) * points[points.size() - 1].getDistanceFromStart());
    }

    return true;
}

void TTSReader::recomputeAltitudeFromGradient() {

    if (points.size() < 3)
        return;

    // Some tts files I see have an altitude crisis around the start: Fixup altitude
    // of index 0 and 1 by linear interpolating.
    double d2d1delta = points[2].getDistanceFromStart() - points[1].getDistanceFromStart();
    points[1].setElevation(points[2].getElevation() - (0.01 * points[1].getGradient()*d2d1delta));
    points[0].setElevation(points[1].getElevation() - (0.01 * points[0].getGradient()*d2d1delta));

    // Wish there was a fix... TTS Files with no altitude will start at elevation 0...
    double alt = points[0].getElevation();

    for (unsigned u = 1; u < points.size(); u++) {

        const Point &curr = points[u];
        const Point &prev = points[u - 1];

        double gradient = prev.getGradient() / 100.;

        double hyp = curr.getDistanceFromStart() - prev.getDistanceFromStart();
        double run = hyp / sqrt(gradient * gradient + 1); // hyp*cos(atan(gradient))

        double rise = gradient * run;

        alt += rise;

        points[u].setElevation(alt);
    }

    // Even if tts file started without altitude, now that it is generated
    // we will claim to have it.
    fHasAlt = true;
}

bool TTSReader::parseFile(QDataStream &file) {
    pre.resize(2);
    int lastSize = -1;

    for (;;) {
        //    is = new FileInputStream(fileName);
        if (!readData(file, pre, false)) {
            break;
        }

        if (isHeader(pre)) {
            ByteArray header;
            header.resize(14);
            if (readData(file, header, true)) {
                content.push_back(header);
                lastSize = getUInt(header, 6) * getUInt(header, 10);
            }
            else {
                DEBUG_LOG << "Error: Cannot read header\n";
                return false;// new IllegalArgumentException("Cannot read header");
            }

            // one byte data.. unconditionally read as data, no-one is
            // able to check it
            if (lastSize < 2) {
                ByteArray data;
                data.resize(lastSize);
                if (readData(file, data, false)) {
                    content.push_back(data);
                }
                else {
                    DEBUG_LOG << "Cannot read " << lastSize << "b data" << "\n";
                    return false;
                }

                lastSize = -1;
            }
        }
        else {
            if (lastSize < 2) {
                DEBUG_LOG << "Data not allowed, header " << getUShort(pre, 0) << "\n";
                return false;
            }
            ByteArray data;
            data.resize(lastSize);
            if (readData(file, data, true)) {
                content.push_back(data);
            } else {
                DEBUG_LOG << "Cannot read " << lastSize << "b data\n";
                return false;
            }

            lastSize = -1;
        }
    }// for

    loadHeaders();

    // Clean up passes.

    // Recompute all altitude data using the recorded gradient.
    // This is necessary because generally the altitude data in tts
    // files is far far too noisy to use to compute slope. For example
    // see IT_LOMBARDY08.TTS.
    // Benefit of this is that altitude data now reflects slope
    // recorded by Tacx, so you won't be climbing a huge pass
    // without putting in the right amount of effort.
    recomputeAltitudeFromGradient();

    return true;
}

bool TTSReader::loadHeaders() {

    static const unsigned char s_key[] = { 0xD6, 0x9C, 0xD8, 0xBC, 0xDA, 0xA9, 0xDC,
                                           0xB0, 0xDE, 0xB6, 0xE0, 0x95, 0xE2, 0xC3, 0xE4, 0x97, 0xE6, 0x92,
                                           0xE8, 0x85, 0xEA, 0x8E, 0xEC, 0x9E, 0xEE, 0x91, 0xF0, 0xB1, 0xF2,
                                           0xD2, 0xF4, 0xD4, 0xF6, 0xD7, 0xF8, 0xB1, 0xFA, 0x9E, 0xFC, 0xDD,
                                           0xFE, 0x96, 0x00, 0x72, 0x02, 0x23, 0x04, 0x71, 0x06, 0x6F, 0x08,
                                           0x6C, 0x0A, 0x2B, 0x0C, 0x7E, 0x0E, 0x4E, 0x10, 0x67, 0x12, 0x7A,
                                           0x14, 0x7A, 0x16, 0x65, 0x18, 0x39, 0x1A, 0x74, 0x1C, 0x7B, 0x1E,
                                           0x3F, 0x20, 0x44, 0x22, 0x75, 0x24, 0x40, 0x26, 0x55, 0x28, 0x50,
                                           0x2A, 0x0B, 0x2C, 0x18, 0x2E, 0x19, 0x30, 0x08, 0x32, 0x0B, 0x34,
                                           0x02, 0x36, 0x1F, 0x38, 0x10, 0x3A, 0x1F, 0x3C, 0x17, 0x3E, 0x17,
                                           0x40, 0x62, 0x42, 0x65, 0x44, 0x65, 0x46, 0x6E, 0x48, 0x69, 0x4A,
                                           0x25, 0x4C, 0x28, 0x4E, 0x2A, 0x50, 0x35, 0x52, 0x36, 0x54, 0x31,
                                           0x56, 0x77, 0x58, 0x09, 0x5A, 0x29, 0x5C, 0x6D, 0x5E, 0x2B, 0x60,
                                           0x52, 0x62, 0x00, 0x64, 0x31, 0x66, 0x2E, 0x68, 0x26, 0x6A, 0x25,
                                           0x6C, 0x4D, 0x6E, 0x3C, 0x70, 0x28, 0x72, 0x20, 0x74, 0x21, 0x76,
                                           0x32, 0x78, 0x34, 0x7A, 0x55, 0x7C, 0x37, 0x7E, 0x0A, 0x80, 0xF2,
                                           0x82, 0xF7, 0x84, 0xC7, 0x86, 0xC2, 0x88, 0xDA, 0x8A, 0xDE, 0x8C,
                                           0xDF, 0x8E, 0xCA, 0x90, 0xE5, 0x92, 0xFC, 0x94, 0xB0, 0x96, 0xC9,
                                           0x98, 0xF4, 0x9A, 0xAF, 0x9C, 0xF6, 0x9E, 0xFA, 0xA0, 0xD5, 0xA2,
                                           0xCB, 0xA4, 0xCC, 0xA6, 0xD4, 0xA8, 0x83, 0xAA, 0x8F, 0xAC, 0xD9,
                                           0xAE, 0xDD, 0xB0, 0xD8, 0xB2, 0xDD, 0xB4, 0xD2, 0xB6, 0xDB, 0xB8,
                                           0xE6, 0xBA, 0xE4, 0xBC, 0xE2, 0xBE, 0xE0, 0xC0, 0xAF, 0xC2, 0xA4,
                                           0xC4, 0xE2, 0xC6, 0xA9, 0xC8, 0xBC, 0xCA, 0xAD, 0xCC, 0xAB, 0xCE,
                                           0xEE };

    IntArray iakey;
    for (unsigned u = 0; u < (sizeof(s_key) / sizeof(s_key[0])); u++)
        iakey.push_back(s_key[u]);

    IntArray key2;
    rehashKey(iakey, 17, key2);

    IntArray keyH;

    int blockType = -1;
    int version   = -1;
    int stringId  = -1;

    StringType stringType = StringType::BLOCK;

    //int fingerprint = 0; // not used

    int bytes = 0;

    for (unsigned cu = 0; cu < content.size(); cu++)
    {
        ByteArray &data = content[cu];

        if (isHeader(data)) {

            DEBUG_LOG << bytes << " [" << std::hex << (bytes) << std::dec << "]: "
                << getUShort(data, 0) << "." << getUShort(data, 2) << " v"
                << getUShort(data, 4) << " " << getUInt(data, 6) << "x"
                << getUInt(data, 10) << "\n";

            //fingerprint = getUInt(data, 6);

            IntArray ia;
            iarr(data, ia);
            bool success = encryptHeader(ia, key2, keyH);
            if (!success)
                return false;

            stringType = StringType::NONPRINTABLE;

            switch (getUShort(data, 2)) {

            case 10: // crc of the data?

                // I don't know how to compute it.. and to which data it
                // belongs..
                // for sure I'm not going to check these, I assume file is
                // not broken
                // (why it can be?)
                stringType = StringType::CRC;
                break;

            case 110: // UTF-16 string

                stringType = StringType::STRING;
                stringId = getUShort(data, 0);
                break;

            case 120: // image fingerprint

                stringId = getUShort(data, 0) + 1000;
                break;

            case 121: // imageType? always 01
                break;

            case 122: // image bytes, name is present in previous string
                      // from the block
                stringType = StringType::IMAGE;
                break;

            default:

                stringType = StringType::BLOCK;
                blockType = getUShort(data, 2);
                version = getUShort(data, 4);

                DEBUG_LOG << "\nblock type " << blockType << " version "
                          << version << "\n";

                stringId = -1;
                break;
            }
        }
        else {
            IntArray ia;
            iarr(data, ia);

            IntArray decrD;
            bool success = decryptData(ia, keyH, decrD);
            if (!success)
                return false;

            DEBUG_LOG << "::";

            switch (stringType) {

            case CRC:

                break;

            case IMAGE:

                //DEBUG_LOG << "[image " << blockType << "." << (stringId - 1000) + "]";

                /*
                 * try { result = currentFile + "." + (imageId++) + ".png";
                 * FileOutputStream file = new FileOutputStream(result);
                 * file.write(barr(decrD)); file.close(); } catch
                 * (IOException e) { result = "cannot create: " + e; }
                 */

                break;

            case STRING:

            {
                if (tts_string_table.count(blockType + stringId)) {
                    DEBUG_LOG << "[" << tts_string_table.at(blockType + stringId) << "]";
                } else {
                    DEBUG_LOG << "[" << blockType << "." << stringId << "]";
                }

                std::wstring result;

                for (size_t i = 0; i < decrD.size() / 2; i++) {
                    Char c = (Char)(decrD[2 * i] | (int)decrD[2 * i + 1] << 8);
                    result.append(1, c);
                }

                switch (blockType + stringId) {
                case 5002: // Video Name
                    ttsName = result;
                    break;
                case 1001: // Route Name
                    routeName = result;
                    break;
                case 1002: // Route Description
                    routeDescription = result;
                    break;
                case 1041: // Segment Name
                    // Push of pending segment to segment list is triggered by finding a new
                    // segment name. If pending segment is valid then push it, then clear
                    // pending to start a new one.
                    flushPendingSegment();
                    pendingSegment.name = result;
                    break;
                case 1042: // Segment Description
                    pendingSegment.description = result;
                    break;
                }

                DEBUG_LOG << "[" << result << "]";
            }
            break;

            case BLOCK:
            {
                ByteArray ba;
                barr(decrD, ba);
                blockProcessing(blockType, version, ba);
            }
            default:
            break;
            }
        }

        bytes += (int)data.size();
    }

    // At this... point...
    // - pointList holds framemapping info (if any)
    // - programList holds slope info
    // - gpsList holds gps info
    // - segmentList holds the segments and their names
    //
    // GPS Info is optional.
    // FrameMapping Info is optional.
    //
    // If this is a slope program then slope info is NOT optional.
    //
    // So we need to decide what is our base list.
    //
    // WattZap implementation based everything on framemapping info,
    // which doesn't work when framemapping isn't in the tts.
    //
    // One stream is chosen to be the basis for the interpolation of
    // the other streams. Choose whichever has the most data points,
    // then interpolate the other two onto it.

    // First thing, flush any pending segment.
    flushPendingSegment();

    size_t gpsCount = gpsPoints.size();
    size_t slopeCount = programList.size();
    size_t frameMapCount = pointList.size();

    enum Basis { NoBasis, FrameMap, GPS, Slope };

    Basis basis = NoBasis;

    // Copy basis stream onto points[].
    if (frameMapCount > gpsCount && frameMapCount > slopeCount)
    {
        // pointList basis. This holds frame mapping.
        // Is already correct type so just copy.
        points = pointList;
        basis = FrameMap;
    } else if (gpsCount >= slopeCount) {
        // GpsList basis    
        for (size_t i = 0; i < gpsCount; i++) {
            Point p;

            p.setDistanceFromStart(gpsPoints[i].distance);
            p.setLatitude(gpsPoints[i].lat);
            p.setLongitude(gpsPoints[i].lon);
            p.setElevation(gpsPoints[i].alt);

            points.push_back(p);
        }
        basis = GPS;
    } else {
        // slope basis
        for (size_t i = 0; i < slopeCount; i++) {
            Point p;

            p.setDistanceFromStart(programList[i].distance);
            p.setGradient(programList[i].slope);

            points.push_back(p);
        }
        basis = Slope;
    }

    if (basis == NoBasis) {
        return false;
    }

    GeoPointInterpolator gpi; // geo interpolation

    // Interpolate non-geo data in xyz.
    DistancePointInterpolator<LinearTwoPointInterpolator> ti;
    DistancePointInterpolator<LinearTwoPointInterpolator> si;

    // Interpolate non-basis streams onto points[].
    size_t frameMapIdx = 0;
    size_t gpsIdx      = 0;
    size_t slopeIdx    = 0;

    for (unsigned long i = 0; i < points.size(); i++) {

        fHasKM = true;

        Point &p = points[i];

        // Interpolate framemap
        if (basis != FrameMap && frameMapCount) {

            while (ti.WantsInput(p.getDistanceFromStart())) {
                if (frameMapIdx >= frameMapCount) {
                    ti.NotifyInputComplete();
                    break;
                }
                ti.Push(pointList[frameMapIdx].getDistanceFromStart(),
                        xyz(pointList[frameMapIdx].getTime(), 0, 0));
                frameMapIdx++;
            }

            xyz interp = ti.Location(p.getDistanceFromStart());
            double time = interp.x();

            p.setTime(time);

            fHasFrameMapping = true;
        }

        // Interpolate gps location
        if (basis != GPS && gpsCount) {
            while (gpi.WantsInput(p.getDistanceFromStart())) {
                if (gpsIdx >= gpsCount) {
                    gpi.NotifyInputComplete();
                    break;
                }
                gpi.Push(gpsPoints[gpsIdx].distance,
                         geolocation(gpsPoints[gpsIdx].lat,
                                     gpsPoints[gpsIdx].lon,
                                     gpsPoints[gpsIdx].alt));
                gpsIdx++;
            }

            geolocation geoloc = gpi.Location(p.getDistanceFromStart());

            p.setLatitude (geoloc.Lat());
            p.setLongitude(geoloc.Long());
            p.setElevation(geoloc.Alt());

            fHasGPS = true;
            fHasAlt = true;
        }

        // Interpolate gradient
        if (basis != Slope && slopeCount) {

            while (si.WantsInput(p.getDistanceFromStart())) {
                if (slopeIdx >= slopeCount) {
                    si.NotifyInputComplete();
                    break;
                }
                si.Push(programList[slopeIdx].distance,
                        xyz(programList[slopeIdx].slope, 0, 0));
                slopeIdx++;
            }

            xyz interp = si.Location(p.getDistanceFromStart());
            double slope = interp.x();

            p.setGradient(slope);

            fHasSlope = true;
        }
    }

    // Convet units for distance and time. Compute speed.
    Point *pPrevPoint = &(points[0]);

    unsigned int pointCount = 0;
    while (pointCount < points.size()) {
        Point &p = points[pointCount];

        // turn into meters
        p.setDistanceFromStart(p.getDistanceFromStart() / 100);

        // convert to mS
        p.setTime(p.getTime() * 1000);

        // speed = d/t
        if (pointCount > 0) {
            p.setSpeed((3600 * (p.getDistanceFromStart() - pPrevPoint->getDistanceFromStart())) / ((p.getTime() - pPrevPoint->getTime())));
        }

        pPrevPoint = &(points[pointCount]);

        // meters / meters
        p.print();

        series.push_back({ p.getDistanceFromStart() / 1000, p.getElevation() });

        pointCount++;
    }// while

    // fill in speed for first point

    totalDistance = 0;
    if (points.size() > 2) {
        points[0].setSpeed(points[1].getSpeed());
        totalDistance = points[points.size() - 1].getDistanceFromStart();
    }

    return true;
}

void TTSReader::blockProcessing(int blockType, int version, ByteArray &data) {
    switch (blockType) {
    case PROGRAM_DATA:
        programData(version, data);
        break;

    case DISTANCE_FRAME:
        distanceToFrame(version, data);
        break;

    case GPS_DATA:
        GPSData(version, data);
        break;

    case GENERAL_INFO:
        generalInfo(version, data);
        break;

    case TRAINING_TYPE:
        trainingType(version, data);
        break;

    case SEGMENT_INFO:
        segmentInfo(version, data);
        break;

    case SEGMENT_RANGE:
        segmentRange(version, data);
        break;

    case VIDEO_INFO:
        videoInfo(version, data);
        break;

    default:

        DEBUG_LOG << "Unidentified block type " << blockType << "\n";
        break;
    }
}

/*

 * Block routines

 */

// it looks like part of GENERALINFO, definition of type is included:
// DWORD WattSlopePulse;//0 = Watt program, 1 = Slope program, 2 = Pulse
// (HR) program
// DWORD TimeDist; //0 = Time based program, 1 = distance based program
// I'm not sure about the order.. but only slope/distance (0/) and
// power/time (1/1) pairs are handled..
void TTSReader::generalInfo(int version, ByteArray &data) {
    Q_UNUSED(version);

    const char* programType;

    switch (data[0]) {
    case 0:
        programType = "slope";
        break;
    case 1:
        programType = "watt";
        DEBUG_LOG << "Power files not currently supported\n";
        //throw new RuntimeException("Power files not currently supported");
        break;
    case 2:
        programType = "heartRate";
        break;

    default:
        programType = "unknown program";
        break;
    }

    const char* trainingType;

    switch (data[1]) {
    case 0:
        trainingType = "distance";
        break;
    case 1:
        trainingType = "time";
        break;
    default:
        trainingType = "unknown training";
        break;
    }

    DEBUG_LOG << "[program type] " << programType << " -> " << trainingType << "\n";

    return;
}

// It screams.. "I'm GPS position!". Distance followed by lat, lon,
// altitude
void TTSReader::GPSData(int version, ByteArray &data) {
    Q_UNUSED(version);

    if (data.size() % 16 != 0) {
        DEBUG_LOG << "GPS Points Data wrong length " << data.size() << "\n";
        return;
    }

    DEBUG_LOG << "[" << (data.size() / 16) << " gps points]\n";

    int gpsCount = (int)data.size() / 16;
    for (int i = 0; i < gpsCount; i++) {

        int distance = getUInt(data, i * 16);

        double lat = AsFloat(getUInt(data, i * 16 + 4));
        double lon = AsFloat(getUInt(data, i * 16 + 8));
        double altitude = AsFloat(getUInt(data, i * 16 + 12));

        if (lat != 0. || lon != 0.) {
            fHasGPS = true;
        }

        if (altitude != 0.) {
            fHasAlt = true;
        }

        bool fIsDuplicate = false;

        // Some tts files contain duplicate points. Remove these
        // since they only screw up interpolation during stream
        // merging.
        if (gpsPoints.size()) {
            GPSPoint &prevGPS = gpsPoints[gpsPoints.size() - 1];

            // Compare geo distance independent of altitude,
            // altitude noise is a separate problem.
            geolocation curGPS(lat, lon, 0);
            geolocation newGPS(prevGPS.lat, prevGPS.lon, 0);

            double distance = curGPS.DistanceFrom(newGPS);

            int distanceExp;
            std::frexp(distance, &distanceExp);

            // If outside of 2^-12 then they are not the same location.
            if (distanceExp < -12) {
                fIsDuplicate = true;
            }
        }

        if (!fIsDuplicate) {
            gpsPoints.push_back(GPSPoint(distance, lat, lon, altitude));
        }
    }

    return;
}

/*
 * Slope/Distance Program: Distance to Frame Mapping Format: 2102675
 * (cm)/77531 (frames)
 *
 * Watt/Time Program:
 */
void TTSReader::distanceToFrame(int version, ByteArray &data) {
    Q_UNUSED(version);
    if (data.size() % 8 != 0) {
        DEBUG_LOG << "Distance2Frame Data wrong length " << data.size() << "\n";
        return;
    }

    /*
     * We can calculate frame rate by dividing number of frames by no. of
     * data points. If value is less than or equal to 30 this is the
     * framerate, if between 30 and 100 divided by 2 (one datapoint every 2
     * seconds of film - typical with tacx TTS files), if the value is more
     * than 100 divide by 10 (typical with RLV files converted to TTS).
     */
    DEBUG_LOG << "[" << (data.size() / 8) << " video points][last frame "
        << getUInt(data, (int)data.size() - 4) << "]" << "\n";

    double dataPoints = ((int)data.size() / 8);
    double frames = getUInt(data, (int)data.size() - 4);

    frameRate = frames / dataPoints;

    if (frameRate > 30 && frameRate < 100) {
        frameRate = frameRate / 2.0;
    }
    else if (frameRate >= 100) {
        frameRate = frameRate / 10.0;
    }

    DEBUG_LOG << "Frame Rate " << frameRate << "\n";

    // System.out.println("Frame Rate " + frameRate);

    for (unsigned int i = 0; i < data.size() / 8; i++) {
        Point p;
        p.setDistanceFromStart(getUInt(data, i * 8));
        int frame = getUInt(data, i * 8 + 4);

        // System.out.println("frame " + frame + " dist " +
        // (p.getDistanceFromStart()/1000));

        p.setTime((int)(frame / frameRate));
        pointList.push_back(p);
    }

    // This reader was premised on all tts files having frame mapping, but
    // some don't...
    fHasFrameMapping = pointList.size() != 0;
}

/**
 * PROGRAM data
 *
 *
 * Format: slope/distance information or power/time
 *
 * short slope // FLOAT RollingFriction; // Usually 4.0 // Now it is integer
 * (/100=>[m], /100=>[s]) and short (/100=>[%], [W], // probably HR as
 * well..) // Value selector is in 1031.
 */

void TTSReader::programData(int version, ByteArray &data) {
    Q_UNUSED(version);

    if (data.size() % 6 != 0) {
        DEBUG_LOG << "Program data wrong length" << data.size() << "\n";
        return;
    }

    DEBUG_LOG << "[" << data.size() / 6 << " program points]";

    double distance = 0;
    int pointCount = (int)data.size() / 6;

    for (int i = 0; i < pointCount; i++) {
        int slope = getUShort(data, i * 6);
        if ((slope & 0x8000) != 0) {
            slope -= 0x10000;
        }

        // slope %, distance meters
        distance += (getUInt(data, i * 6 + 2));

        // System.out.println("slope " + slope + " distance " + distance);
        ProgramPoint p;

        p.slope = (double)slope / 100;
        p.distance = distance;

        if (i == 0) {
            // first time thru'
            minSlope = p.slope;
            maxSlope = p.slope;
        }

        if (p.slope < minSlope) {
            minSlope = p.slope;
        }

        if (p.slope > maxSlope) {
            maxSlope = p.slope;
        }

        // If first point isn't zero distance then create fake
        // first point. This matters for a few early tts rides
        // including NO_OCEAN from lunicus and IT_Mortirolo from
        // tacx.
        if (i == 0 && distance != 0.) {
            programList.push_back({0, 0});
        }

        programList.push_back(p);
    }
}
