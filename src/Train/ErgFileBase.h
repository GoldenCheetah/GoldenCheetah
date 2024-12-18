/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2015 Vianney Boyer   (vlcvboyer@gmail.com)
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

#ifndef _ErgFileBase_h
#define _ErgFileBase_h

#include <QString>
#include <QStringList>

#define MAX_ZONES 10


enum class ErgFileFormat {
    unknown = 0,
    erg,
    mrc,
    crs,
    erg2,
    code
};

enum class ErgFileType {
    unknown = 0,
    erg,
    slp,
    code
};

enum class ErgFilePowerZone {
    unknown = 0,
    z1,
    z2,
    z3,
    z4,
    z5,
    z6,
    z7,
    z8,
    z9,
    z10
};


class ErgFilePoint
{
    public:

        ErgFilePoint() : x(0), y(0), val(0), lat(0), lon(0) {}
        ErgFilePoint(double x_, double y_, double val_) : x(x_), y(y_), val(val_), lat(0), lon(0) {}
        ErgFilePoint(double x_, double y_, double val_, double lat_, double lon_) : x(x_), y(y_), val(val_), lat(lat_), lon(lon_) {}

        double x;     // x axis - time in msecs or distance in meters
        double y;     // y axis - load in watts or altitude

        double val;   // the value to send to the device (watts/gradient)

        double lat, lon; // location (alt is y, above)
};


class ErgFileSection
{
    public:
        ErgFileSection() : duration(0), start(0), end(0) {}
        ErgFileSection(int duration, int start, int end) : duration(duration), start(start), end(end) {}

        int duration;
        double start, end;
};


class ErgFileZoneSection
: public ErgFileSection
{
    public:
        ErgFileZoneSection() : ErgFileSection(), startValue(0), endValue(0), zone(0) {}
        ErgFileZoneSection(int startMSecs, int startValue, int endMSecs, int endValue, int zone)
         : ErgFileSection(endMSecs - startMSecs, startMSecs, endMSecs), startValue(startValue), endValue(endValue), zone(zone)
        {}

        int startValue;
        int endValue;
        int zone;
};


class ErgFileText
{
    public:
        ErgFileText() : x(0), duration(0), text("") {}
        ErgFileText(double x, int duration, const QString &text) : x(x), duration(duration), text(text) {}

        double x;
        int duration;
        QString text;
};


class ErgFileLap
{
    public:
        ErgFileLap() : name(""), x(0), LapNum(0), lapRangeId(0), selected(false) {}
        ErgFileLap(double x, int LapNum, const QString& name) : name(name), x(x), LapNum(LapNum), lapRangeId(0), selected(false) {}
        ErgFileLap(double x, int LapNum, int lapRangeId, const QString& name) : name(name), x(x), LapNum(LapNum), lapRangeId(lapRangeId), selected(false) {}

        QString name;
        double x;      // when does this LAP marker occur? (time in msecs or distance in meters
        int LapNum;    // from 1 - n
        int lapRangeId;// for grouping lap markers into ranges. Value of zero is considered 'ungrouped'.
        bool selected; // used by the editor
};


class ErgFileBase
{
    public:
        ErgFileBase();
        ~ErgFileBase();

        static const QList<ErgFilePowerZone> &powerZoneList();

        bool hasGradient() const;
        bool hasWatts() const;
        bool hasAbsoluteWatts() const;
        bool hasRelativeWatts() const;
        bool hasGPS() const;

        ErgFileType type() const;
        QString typeString() const;
        QString ergSubTypeString() const;

        ErgFileFormat format() const;
        QString formatString() const;
        void format(ErgFileFormat format);

        QString version() const;
        void version(QString version);

        QString units() const;
        void units(QString units);

        QString filename() const;
        void filename(QString filename);

        QString originalFilename() const;
        void originalFilename(QString originalFilename);

        QString name() const;
        void name(QString name);

        QString description() const;
        void description(QString description);

        QString trainerDayId() const;
        void trainerDayId(QString trainerDayId);

        QString source() const;
        void source(QString source);

        QStringList tags() const;
        void tags(QStringList tags);

        QString category() const;
        void category(QString category);

        QString subcategory() const;
        void subcategory(QString subcategory);

        int categoryIndex() const;
        void categoryIndex(int categoryIndex);

        long duration() const;
        void duration(long duration);

        int ftp() const;
        void ftp(int ftp);

        int minWatts() const;
        void minWatts(int minWatts);

        int maxWatts() const;
        void maxWatts(int maxWatts);


        ErgFileFormat mode() const;
        void mode(ErgFileFormat mode);

        bool strictGradient() const;
        void strictGradient(bool strictGradient);

        bool fHasGPS() const;
        void fHasGPS(bool hasGPS);

        void resetMetrics();

        double minY() const;
        void minY(double minY);

        double maxY() const;
        void maxY(double maxY);

        double CP() const;
        void CP(double cp);

        double AP() const;
        void AP(double ap);

        double IsoPower() const;
        void IsoPower(double isoPower);

        double IF() const;
        void IF(double IF);

        double bikeStress() const;
        void bikeStress(double bikeStress);

        double VI() const;
        void VI(double vi);

        double XP() const;
        void XP(double xp);

        double RI() const;
        void RI(double ri);

        double BS() const;
        void BS(double bs);

        double SVI() const;
        void SVI(double svi);

        long distance() const;

        double minElevation() const;
        double maxElevation() const;

        double ele() const;
        void ele(double ele);

        double eleDist() const;
        void eleDist(double eleDist);

        double grade() const;
        void grade(double grade);

        double powerZonePC(ErgFilePowerZone zone) const;
        QList<double> powerZonesPC() const;
        void powerZonePC(ErgFilePowerZone zone, double pc);

        ErgFilePowerZone dominantZone() const;
        int dominantZoneInt() const;
        void dominantZone(ErgFilePowerZone zone);
        void dominantZone(int zone);

        int numZones() const;
        void numZones(int nz);

    private:
        static QList<ErgFilePowerZone> _powerZoneList;

        ErgFileFormat _format;

        QString _Version;        // version number / identifer

        QString _Units;          // units used
        QString _originalFilename; // filename from inside file
        QString _filename;       // filename on disk
        QString _Name;           // description in file
        QString _Description;    // long narrative for workout
        QString _TrainerDayId;   // if downloaded from TrainerDay
        QString _Source;         // where did this come from
        QStringList _Tags;       // tagged strings
        QString _Category;
        QString _Subcategory;
        int _CategoryIndex = -1;

        long _Duration = 0;      // Duration of this workout in msecs
        int _Ftp = 0;            // FTP this file was targetted at
        int _MinWatts = 0;       // minWatts in this ergfile (scaling)
        int _MaxWatts = 0;       // maxWatts in this ergfile (scaling)
        ErgFileFormat _mode = ErgFileFormat::unknown;
        bool _StrictGradient = false; // should gradient be strict or smoothed?
        bool _fHasGPS = false;   // has Lat/Lon?

        // Metrics for this workout
        double _minY = 0;
        double _maxY = 0;        // minimum and maximum Y value
        double _CP = 0;
        double _AP = 0;
        double _IsoPower = 0;
        double _IF = 0;
        double _BikeStress = 0;
        double _VI = 0;          // Coggan for erg / mrc
        double _XP = 0;
        double _RI = 0;
        double _BS = 0;
        double _SVI = 0;         // Skiba for erg / mrc
        double _ele = 0;
        double _eleDist = 0;
        double _grade = 0;       // crs
        double _powerZonesPC[11] = {}; // mrc
        ErgFilePowerZone _dominantZone = ErgFilePowerZone::unknown;
        int _numZones = 0;
};

#endif
