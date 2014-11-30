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
class WPrime;
class RideFile;
struct RideFilePoint;
struct RideFileDataPresent;
struct RideFileInterval;
class EditorData;      // attached to a RideFile
class RideFileCommand; // for manipulating ride data
class Context;      // for context; cyclist, homedir

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

extern const QChar deltaChar;

struct RideFileDataPresent
{
    // basic (te = torqueeffectiveness, ps = pedal smoothness)
    bool secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp;
    bool lrbalance, lte, rte, lps, rps, smo2, thb, interval;

    // derived
    bool np,xp,apower,wprime,atiss,antiss,gear,hhb,o2hb;

    // running
    bool rvert, rcad, rcontact;

    // whether non-zero data of each field is present
    RideFileDataPresent():
        secs(false), cad(false), hr(false), km(false),
        kph(false), nm(false), watts(false), alt(false), lon(false), lat(false),
        headwind(false), slope(false), temp(false), 
        lrbalance(false), lte(false), rte(false), lps(false), rps(false), smo2(false), thb(false), interval(false),
        np(false), xp(false), apower(false), wprime(false), atiss(false), antiss(false),gear(false),hhb(false),o2hb(false),
        rvert(false), rcad(false), rcontact(false) {}

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
    bool operator== (RideFileInterval right) const { return start == right.start && stop == right.stop; }
    bool operator!= (RideFileInterval right) const { return start != right.start || stop != right.stop; }
};

struct RideFileCalibration
{
    double start;
    int value;
    QString name;
    RideFileCalibration() : start(0.0), value(0) {}
    RideFileCalibration(double start, int value, QString name) :
        start(start), value(value), name(name) {}

    // order bu start time
    bool operator< (RideFileCalibration right) const { return start < right.start; }
};

class RideFile : public QObject // QObject to emit signals
{
    Q_OBJECT
    G_OBJECT


    public:

        friend class RideFileCommand; // tells us we were modified
        friend class MainWindow; // tells us we were modified
        friend class Context; // tells us we were saved

        // Constructor / Destructor
        RideFile();
        RideFile(RideFile*);
        RideFile(const QDateTime &startTime, double recIntSecs);
        virtual ~RideFile();

        // construct a new ridefile using the current one, but
        // resample the data and fill gaps in recording with
        // the max gap to interpolate also passed as a parameter
        RideFile *resample(double recIntSecs, int interpolate=30);

        // Working with DATASERIES
        enum seriestype { secs=0, cad, cadd, hr, hrd, km, kph, kphd, nm, nmd, watts, wattsd,
                          alt, lon, lat, headwind, slope, temp, interval, NP, xPower, 
                          vam, wattsKg, lrbalance, lte, rte, lps, rps, 
                          aPower, wprime, aTISS, anTISS, smo2, thb, 
                          rvert, rcad, rcontact, gear, o2hb, hhb, none };

        enum specialValues { NoTemp = -255 };
        typedef enum seriestype SeriesType;
        static SeriesType lastSeriesType() { return none; }

        static QString seriesName(SeriesType);
        static QString unitName(SeriesType, Context *context);
        static int decimalsFor(SeriesType series);
        static double maximumFor(SeriesType series);
        static double minimumFor(SeriesType series);
        static QColor colorFor(SeriesType series);
        static bool parseRideFileName(const QString &name, QDateTime *dt);
        bool isRun() const;

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
                         double temperature, double lrbalance,
                         double lte, double rte, double lps, double rps,
                         double smo2, double thb,
                         double rvert, double rcad, double rcontact,
                         int interval);

        void appendPoint(const RideFilePoint &);
        const QVector<RideFilePoint*> &dataPoints() const { return dataPoints_; }

        // recalculate all the derived data series
        // might want to move to a factory for these
        // at some point, but for now hard coded
        //
        // YOU MUST ALWAYS CALL THIS BEFORE ACESSING
        // THE DERIVED DATA. IT IS REFRESHED ON DEMAND.
        // STATE IS MAINTAINED IN 'bool dstale' BELOW
        // TO ENSURE IT IS ONLY REFRESHED IF NEEDED
        //
        void recalculateDerivedSeries(bool force=false);

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

        // Working with CAIBRATIONS
        const QList<RideFileCalibration> &calibrations() const { return calibrations_; }
        void addCalibration(double start, int value, const QString &name) {
            calibrations_.append(RideFileCalibration(start, value, name));
        }

        // Working with REFERENCES
        void appendReference(const RideFilePoint &);
        void removeReference(int index);
        const QVector<RideFilePoint*> &referencePoints() const { return referencePoints_; }

        // Index offset calculations
        double timeToDistance(double) const;  // get distance in km at time in secs
        double distanceToTime(double km) const; // det time from distance
        int timeIndex(double) const;          // get index offset for time in secs
        int distanceIndex(double) const;      // get index offset for distance in KM

        // Working with the METADATA TAGS
        const QMap<QString,QString>& tags() const { return tags_; }
        QString getTag(QString name, QString fallback) const { return tags_.value(name, fallback); }
        void setTag(QString name, QString value) { tags_.insert(name, value); }

        Context *context;
        double getWeight();
        void setWeight(double x);
        double getHeight();
 
        WPrime *wprimeData(); // return wprime, init/refresh if needed

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
        QVector<RideFilePoint*> referencePoints_;
        RideFilePoint* minPoint;
        RideFilePoint* maxPoint;
        RideFilePoint* avgPoint;
        RideFilePoint* totalPoint;
        RideFileDataPresent dataPresent;
        QString deviceType_;
        QString fileFormat_;
        QList<RideFileInterval> intervals_;
        QList<RideFileCalibration> calibrations_;
        QMap<QString,QString> tags_;
        EditorData *data;
        WPrime *wprime_;
        bool wstale;
        double weight_; // cached to save calls to getWeight();
        double totalCount, totalTemp;

        QVariant getPointFromValue(double value, SeriesType series) const;
        void updateMin(RideFilePoint* point);
        void updateMax(RideFilePoint* point);
        void updateAvg(RideFilePoint* point);

        bool dstale; // is derived data up to date?
};

struct RideFilePoint
{
    // recorded data
    double secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp;
    double lrbalance, lte, rte, lps, rps;
    double smo2, thb;
    double hrd, cadd, kphd, nmd, wattsd; // acceleration in watts/s km/s

    // running data
    double rvert, rcad, rcontact;

    int interval;

    // derived data (we calculate it)
    // xPower, normalised power, aPower
    double xp, np, apower, atiss, antiss, gear, hhb, o2hb;

    // create blank point
    RideFilePoint() : secs(0.0), cad(0.0), hr(0.0), km(0.0), kph(0.0), nm(0.0), 
                      watts(0.0), alt(0.0), lon(0.0), lat(0.0), headwind(0.0), 
                      slope(0.0), temp(-255.0), lrbalance(0), lte(0.0), rte(0.0),
                      lps(0.0), rps(0.0),
                      smo2(0.0), thb(0.0),
                      hrd(0.0), cadd(0.0), kphd(0.0), nmd(0.0), wattsd(0.0),
                      rvert(0.0), rcad(0.0), rcontact(0.0),
                      interval(0), xp(0), np(0),
                      apower(0), atiss(0.0), antiss(0.0), gear(0.0), hhb(0.0), o2hb(0.0) {}

    // create point supplying all values
    RideFilePoint(double secs, double cad, double hr, double km, double kph,
                  double nm, double watts, double alt, double lon, double lat,
                  double headwind, double slope, double temp, double lrbalance, 
                  double lte, double rte, double lps, double rps,
                  double smo2, double thb, 
                  double rvert, double rcad, double rcontact,
                  int interval) :

        secs(secs), cad(cad), hr(hr), km(km), kph(kph), nm(nm), watts(watts), alt(alt), lon(lon), 
        lat(lat), headwind(headwind), slope(slope), temp(temp),
        lrbalance(lrbalance), lte(lte), rte(rte), lps(lps), rps(rps),
        smo2(smo2), thb(thb),
        hrd(0.0), cadd(0.0), kphd(0.0), nmd(0.0), wattsd(0.0), 
        rvert(rvert), rcad(rcad), rcontact(rcontact), interval(interval), 
        xp(0), np(0), apower(0), atiss(0.0), antiss(0.0), gear(0.0),hhb(0.0),o2hb(0.0) {}

    // get the value via the series type rather than access direct to the values
    double value(RideFile::SeriesType series) const;
    void setValue(RideFile::SeriesType series, double value);
};

struct RideFileReader {
    virtual ~RideFileReader() {}
    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*>* = 0) const = 0;

    // if hasWrite capability should re-implement writeRideFile and hasWrite
    virtual bool hasWrite() const { return false; }
    virtual bool writeRideFile(Context *, const RideFile *, QFile &) const { return false; }
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
        RideFile *openRideFile(Context *context, QFile &file, QStringList &errors, QList<RideFile*>* = 0) const;
        bool writeRideFile(Context *context, const RideFile *ride, QFile &file, QString format) const;
        QStringList listRideFiles(const QDir &dir) const;
        QStringList suffixes() const;
        QStringList writeSuffixes() const;
        QString description(const QString &suffix) const {
            return descriptions_[suffix];
        }
        QRegExp rideFileRegExp() const;
        RideFileReader *readerForSuffix(QString) const; 
};

#endif // _RideFile_h
