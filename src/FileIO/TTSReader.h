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

// Adapted to C++ from Wattzap Community Edition Java source code.

#include <cstdint>
#include <iostream>
#include <vector>
#include <cctype>
#include <algorithm>
#include <QDataStream>

/* This file is part of Wattzap Community Edition.
 *
 * Wattzap Community Edtion is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wattzap Community Edition is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wattzap.  If not, see <http://www.gnu.org/licenses/>.
 */
namespace NS_TTSReader {

    // typedefs to ease conversion from Java
    typedef char                        Byte;
    typedef unsigned char               UByte;
    typedef std::vector<Byte>           ByteArray;
    typedef unsigned short              Char;
    typedef std::vector<Char>           CharArray;
    typedef std::vector<int>            IntArray;
    typedef std::int64_t                Long;

    struct Pair { double x; double y; };
    typedef std::vector<Pair> XYSeries;

    /**
     *
     * Represents a data point from a route or power file (gpx, rlv, pwr etc)
     *
     * @author David George (c) Copyright 2013-2016
     * @date 19 June 2013
     */

    struct Waypoint {
        double lat, lon, alt;

        Waypoint(double _lat, double _lon, double _alt) : lat(_lat), lon(_lon), alt(_alt) {}
    };

    class Point {
        double latitude;
        double longitude;
        double elevation;
        double distanceFromStart;
        double gradient;
        int    power;
        double speed;
        long   time;

    public:

        Point() : latitude(0), longitude(0), elevation(0), distanceFromStart(0), gradient(0), power(0), speed(0), time(0) {}

        double getSpeed() const {
            return speed;
        }

        void setSpeed(double _speed) {
            this->speed = _speed;
        }

        long getTime() const {
            return time;
        }

        void setTime(long _time) {
            this->time = _time;
        }

        double getGradient() const {
            return gradient;
        }

        int getPower() const {
            return power; // overload gradient with power
        }

        void setGradient(double _gradient) {
            this->gradient = _gradient;
        }

        void setPower(int _power) {
            this->power = _power;
        }

        double getLatitude() const {
            return latitude;
        }

        void setLatitude(double _latitude) {
            this->latitude = _latitude;
        }

        double getLongitude() const {
            return longitude;
        }

        void setLongitude(double _longitude) {
            this->longitude = _longitude;
        }

        double getElevation() const {
            return elevation;
        }

        void setElevation(double _elevation) {
            this->elevation = _elevation;
        }

        double getDistanceFromStart() const {
            return distanceFromStart;
        }

        void setDistanceFromStart(double _distanceFromStart) {
            this->distanceFromStart = _distanceFromStart;
        }

        void print() const;
    };

    /**
     * Tacx TTS file reader.
     *
     * @author JaroslawP
     * @author David George
     * @date 1 April 2015
     */

    struct ProgramPoint {
        long distance;
        double slope;
    };

    class TTSReader {

        double frameRate;
        std::vector<Point> pointList;

        std::vector<ProgramPoint> programList;
        std::vector<ByteArray> content;
        ByteArray pre;

        int imageId;

        double   maxSlope;
        double   minSlope;
        XYSeries series;

        std::vector<Point> points;

        double totalDistance;

        bool fHasKM;
        bool fHasLat;
        bool fHasLon;
        bool fHasAlt;
        bool fHasSlope;

        // Block Types
        static const int PROGRAM_DATA = 1032;
        static const int DISTANCE_FRAME = 5020;
        static const int GPS_DATA = 5050;
        static const int GENERAL_INFO = 1031;
        static const int TRAINING_TYPE = 5010;
        static const int SEGMENT_INFO = 1041;
        static const int SEGMENT_RANGE = 1050;
        static const int VIDEO_INFO = 2010;

    public:

        TTSReader() : maxSlope(0), minSlope(0), totalDistance(0), fHasKM(false), fHasLat(false), fHasLon(false), fHasAlt(false), fHasSlope(false) {}

        enum StringType {
            NONPRINTABLE, BLOCK, STRING, IMAGE, CRC
        };

        const XYSeries           & getSeries() const;
        const std::vector<Point> & getPoints() const;
        double                     getDistanceMeters() const;
        int                        routeType() const;
        double                     getMaxSlope() const;
        double                     getMinSlope() const;

        bool                       parseFile(QDataStream &file);

        bool                       deriveMinMaxSlopes(double &minSlope, double &maxSlope, double &variance) const;

        unsigned                   findNextNonDuplicateGeolocation(unsigned u) const;
        unsigned                   interpolateLocationsWithinSpan(unsigned u0, unsigned u1);
        unsigned                   interpolateDuplicateLocations();
        void                       recomputeAltitudeFromGradient();

#if 0
        static final Map<Integer, String> strings = new HashMap<Integer, String>();

        static {
            strings.put(1001, "route name");
            strings.put(1002, "route description");
            strings.put(1041, "segment name");
            strings.put(1042, "segment description");
            strings.put(2001, "company");
            strings.put(2004, "serial");
            strings.put(2005, "time");
            strings.put(2007, "link");
            strings.put(5001, "product");
            strings.put(5002, "video name");
            strings.put(6001, "infobox #1");
        }
#endif

        bool hasKm()        { return fHasKM; }
        bool hasGradient()  { return fHasSlope; }
        bool hasLatitude()  { return fHasLat; }
        bool hasLongitude() { return fHasLon; }
        bool hasElevation() { return fHasAlt; }


        bool loadHeaders();
        void blockProcessing(int blockType, int version, ByteArray &data);

        // it looks like part of GENERALINFO, definition of type is included:
        // DWORD WattSlopePulse;//0 = Watt program, 1 = Slope program, 2 = Pulse
        // (HR) program
        // DWORD TimeDist; //0 = Time based program, 1 = distance based program
        // I'm not sure about the order.. but only slope/distance (0/) and
        // power/time (1/1) pairs are handled..
        void generalInfo(int version, ByteArray &data);

        bool readData(QDataStream &is, ByteArray& buffer, bool copyPre);

        // It screams.. "I'm GPS position!". Distance followed by lat, lon,
        // altitude
        void GPSData(int version, ByteArray &data);

        /*
         * Slope/Distance Program: Distance to Frame Mapping Format: 2102675
         * (cm)/77531 (frames)
         *
         * Watt/Time Program:
         */

        void distanceToFrame(int version, ByteArray &data);

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

        void programData(int version, ByteArray &data);
    }; // End Class TTSReader
}; // End namespace NS_TTSReader
