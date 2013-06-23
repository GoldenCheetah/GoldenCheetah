/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _RideFile_h
#define _RideFile_h
#include "GoldenCheetah.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QList>
#include <QMap>
#include <QVector>
#include <QObject>

class RideItem;
class RideFile;
struct RideFilePoint;
struct RideFileDataPresent;
struct RideFileInterval;
class EditorData;      // attached to a RideFile
class RideFileCommand; // for manipulating ride data
class MainWindow;      // for context; cyclist, homedir

// This file defines four classes:
//
// RideFile, as the name suggests, represents the data stored in a ride file,
// regardless of what type of file it is (.raw, .srm, .csv).
//
// RideFilePoint represents the data for a single sample in a RideFile.
//
// RideFileReader is an abstract base class for function-objects that take a
// filename and return a RideFile object representing the ride stored in the
// corresponding file.
//
// RideFileFactory is a singleton that maintains a mapping from ride file
// suffixes to the RideFileReader objects capable of converting those files
// into RideFile objects.

struct RideFileDataPresent
{
    bool secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, lrbalance, interval;
    // whether non-zero data of each field is present
    RideFileDataPresent():
        secs(false), cad(false), hr(false), km(false),
        kph(false), nm(false), watts(false), alt(false), lon(false), lat(false),
        headwind(false), slope(false), temp(false), lrbalance(false), interval(false) {}
};

struct RideFileInterval
{
    double start, stop;
    QString name;
    RideFileInterval() : start(0.0), stop(0.0) {}
    RideFileInterval(double start, double stop, QString name) :
        start(start), stop(stop), name(name) {}

    // order bu start time
    bool operator< (RideFileInterval right) const { return start < right.start; }
};

class RideFile : public QObject // QObject to emit signals
{
    Q_OBJECT
    G_OBJECT


    public:

        friend class RideFileCommand; // tells us we were modified
        friend class MainWindow; // tells us we were saved

        // Constructor / Destructor
        RideFile();
        RideFile(const QDateTime &startTime, double recIntSecs);
        virtual ~RideFile();

        // Working with DATASERIES
        enum seriestype { secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, interval, NP, xPower, vam, wattsKg, lrbalance, none };
        enum specialValues { noTemp = -255 };

        typedef enum seriestype SeriesType;
        static QString seriesName(SeriesType);
        static QString unitName(SeriesType, MainWindow *main);
        static int decimalsFor(SeriesType series);
        static double maximumFor(SeriesType series);
        static double minimumFor(SeriesType series);

        // Working with DATAPOINTS -- ***use command to modify***
        RideFileCommand *command;
        double getPointValue(int index, SeriesType series) const;
        QVariant getPoint(int index, SeriesType series) const;

        QVariant getMinPoint(SeriesType series) const;
        QVariant getAvgPoint(SeriesType series) const;
        QVariant getMaxPoint(SeriesType series) const;

        void appendPoint(double secs, double cad, double hr, double km,
                         double kph, double nm, double watts, double alt,
                         double lon, double lat, double headwind, double slope,
                         double temperature, double lrbalance, int interval);

        void appendPoint(const RideFilePoint &);
        const QVector<RideFilePoint*> &dataPoints() const { return dataPoints_; }

        // Working with DATAPRESENT flags
        inline const RideFileDataPresent *areDataPresent() const { return &dataPresent; }
        bool isDataPresent(SeriesType series);

        // Working with FIRST CLASS variables
        const QDateTime &startTime() const { return startTime_; }
        void setStartTime(const QDateTime &value) { startTime_ = value; }
        double recIntSecs() const { return recIntSecs_; }
        void setRecIntSecs(double value) { recIntSecs_ = value; }
        const QString &deviceType() const { return deviceType_; }
        void setDeviceType(const QString &value) { deviceType_ = value; }
        const QString &fileFormat() const { return fileFormat_; }
        void setFileFormat(const QString &value) { fileFormat_ = value; }
        const QString id() const { return id_; }
        void setId(const QString &value) { id_ = value; }

        // Working with INTERVALS
        const QList<RideFileInterval> &intervals() const { return intervals_; }
        void addInterval(double start, double stop, const QString &name) {
            intervals_.append(RideFileInterval(start, stop, name));
        }
        void clearIntervals();
        void fillInIntervals();
        int intervalBegin(const RideFileInterval &interval) const;

        // Index offset calculations
        double timeToDistance(double) const;  // get distance in km at time in secs
        int timeIndex(double) const;          // get index offset for time in secs
        int distanceIndex(double) const;      // get index offset for distance in KM

        // Working with the METADATA TAGS
        const QMap<QString,QString>& tags() const { return tags_; }
        QString getTag(QString name, QString fallback) const { return tags_.value(name, fallback); }
        void setTag(QString name, QString value) { tags_.insert(name, value); }

        MainWindow *mainwindow;
        double getWeight();

        // METRIC OVERRIDES
        QMap<QString,QMap<QString,QString> > metricOverrides;

        // editor data is held here and updated
        // as rows/columns are added/removed
        // this is a workaround to avoid holding
        // state data within each RideFilePoint structure
        // and avoiding impact on existing code
        EditorData *editorData() { return data; }
        void setEditorData(EditorData *x) { data = x; }

        // ************************ WARNING ***************************
        // you shouldn't use these routines directly
        // rather use the RideFileCommand *command
        // to manipulate the ride data
        void setPointValue(int index, SeriesType series, double value);
        void deletePoint(int index);
        void deletePoints(int index, int count);
        void insertPoint(int index, RideFilePoint *point);
        void appendPoints(QVector <struct RideFilePoint *> newRows);
        void setDataPresent(SeriesType, bool);
        // ************************************************************

    signals:
        void saved();
        void reverted();
        void modified();
        void deleted();

    protected:
        void emitSaved();
        void emitReverted();
        void emitModified();

    private:

        QString id_; // global uuid@goldencheetah.org
        QDateTime startTime_;  // time of day that the ride started
        double recIntSecs_;    // recording interval in seconds
        QVector<RideFilePoint*> dataPoints_;
        RideFilePoint* minPoint;
        RideFilePoint* maxPoint;
        RideFilePoint* avgPoint;
        RideFilePoint* totalPoint;
        RideFileDataPresent dataPresent;
        QString deviceType_;
        QString fileFormat_;
        QList<RideFileInterval> intervals_;
        QMap<QString,QString> tags_;
        EditorData *data;
        double weight_; // cached to save calls to getWeight();
        double totalCount;

        QVariant getPointFromValue(double value, SeriesType series) const;
        void updateMin(RideFilePoint* point);
        void updateMax(RideFilePoint* point);
        void updateAvg(RideFilePoint* point);
};

struct RideFilePoint
{
    double secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp, lrbalance;
    int interval;
    RideFilePoint() : secs(0.0), cad(0.0), hr(0.0), km(0.0), kph(0.0),
        nm(0.0), watts(0.0), alt(0.0), lon(0.0), lat(0.0), headwind(0.0), slope(0.0), temp(-255.0), lrbalance(0), interval(0) {}
    RideFilePoint(double secs, double cad, double hr, double km, double kph,
                  double nm, double watts, double alt, double lon, double lat,
                  double headwind, double slope, double temp, double lrbalance, int interval) :
        secs(secs), cad(cad), hr(hr), km(km), kph(kph), nm(nm),
        watts(watts), alt(alt), lon(lon), lat(lat), headwind(headwind), slope(slope), temp(temp), lrbalance(lrbalance), interval(interval) {}
    double value(RideFile::SeriesType series) const;
};

struct RideFileReader {
    virtual ~RideFileReader() {}
    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*>* = 0) const = 0;

    // if hasWrite capability should re-implement writeRideFile and hasWrite
    virtual bool hasWrite() const { return false; }
    virtual bool writeRideFile(MainWindow *, const RideFile *, QFile &) const { return false; }
};

class RideFileFactory {

    private:

        static RideFileFactory *instance_;
        QMap<QString,RideFileReader*> readFuncs_;
        QMap<QString,QString> descriptions_;

        RideFileFactory() {}

    public:

        static RideFileFactory &instance();

        int registerReader(const QString &suffix, const QString &description,
                           RideFileReader *reader);
        RideFile *openRideFile(MainWindow *main, QFile &file, QStringList &errors, QList<RideFile*>* = 0) const;
        bool writeRideFile(MainWindow *main, const RideFile *ride, QFile &file, QString format) const;
        QStringList listRideFiles(const QDir &dir) const;
        QStringList suffixes() const;
        QStringList writeSuffixes() const;
        QString description(const QString &suffix) const {
            return descriptions_[suffix];
        }
        QRegExp rideFileRegExp() const;
};

#endif // _RideFile_h
