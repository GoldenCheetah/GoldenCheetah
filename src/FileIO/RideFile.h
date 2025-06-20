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
#include <QRegExp>

class RideItem;
class RideCache;
class Specification;
class IntervalItem;
class WPrime;
class RideFile;
class XDataSeries;
class XDataPoint;
struct RideFilePoint;
struct RideFileDataPresent;
class RideFileInterval;
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
    bool lrbalance, lte, rte, lps, rps, lpco, rpco, lppb, rppb, lppe, rppe, lpppb, rpppb, lpppe, rpppe;
    bool smo2, thb, interval;

    // derived
    bool np,xp,apower,wprime,atiss,antiss,gear,hhb,o2hb,tcore;

    // running
    bool rvert, rcad, rcontact;

    // whether non-zero data of each field is present
    RideFileDataPresent():
        secs(false), cad(false), hr(false), km(false),
        kph(false), nm(false), watts(false), alt(false), lon(false), lat(false),
        headwind(false), slope(false), temp(false), 
        lrbalance(false), lte(false), rte(false), lps(false), rps(false),
        lpco(false), rpco(false), lppb(false), rppb(false), lppe(false), rppe(false),
        lpppb(false), rpppb(false), lpppe(false), rpppe(false),
        smo2(false), thb(false), interval(false),
        np(false), xp(false), apower(false), wprime(false), atiss(false), antiss(false),gear(false),
        hhb(false),o2hb(false), tcore(false),
        rvert(false), rcad(false), rcontact(false) {}

};

// Mappings for QIC/ANT fields info
struct CIQfield {
    CIQfield(QString message, QString name, int nativeid, int id, QString type, QString unit, int scale, int offset)
        : message(message), name(name), nativeid(nativeid), id(id), scale(scale), offset(offset), type(type), unit(unit) {}

    QString message;
    QString name;
    int     nativeid;
    int     id;
    int     scale;
    int     offset;
    QString type;
    QString unit;
//  int     size;
};

struct CIQinfo
{
    CIQinfo(QString appid, int id, int ver) : appid(appid), devid(id), ver(ver){}
    CIQinfo(const QJsonObject& obj);

    static QString listToJson(const QList<CIQinfo>& ciqList);
    static QList<CIQinfo> listFromJson(const QString& src);

    QString appid;
    int devid;
    int ver;
    QList<CIQfield> fields;
};

class RideFileInterval
{
    Q_DECLARE_TR_FUNCTIONS(RideFileInterval);

    public:
        enum intervaltype { DEVICE,                 // Came from Device (Calibration?)
                            USER,                   // User defined

                                                    // DISCOVERED ALWAYS AFTER USER BELOW
                            ALL,                    // Entire workout
                            PEAKPOWER,              // Peak Power incl. ranking 1-10 in ride
                            PEAKPACE,               // Peak Pace
                            EFFORT,                 // Sustained effort
                            ROUTE,                  // GPS Route
                            CLIMB                   // Hills and Cols

                                                    // ADD NEW ONES HERE AND UPDATE last() below

                           } types;

        static enum intervaltype last() { return CLIMB; } // update to last above!

        typedef enum intervaltype IntervalType;
        static QString typeDescription(IntervalType);                  // return a string to represent the type
        static QString typeDescriptionLong(IntervalType);              // return a longer string to represent the type
        static qint32 intervalTypeBits(IntervalType);                  // returns the bit value or'ed into GC_DISCOVERY

        const QMap<QString,QString>& tags() const { return tags_; }
        QString getTag(QString name, QString fallback) const { return tags_.value(name, fallback); }
        void setTag(QString name, QString value) { tags_.insert(name, value); }
        bool removeTag(QString name) { return tags_.remove(name); }

        QString typeString;
        IntervalType type;
        double start, stop;
        QString name;
        bool test;
        QColor color;
        QMap<QString,QString> tags_;

        RideFileInterval() : type(USER), start(0.0), stop(0.0), test(false), color(Qt::black) {}
        RideFileInterval(IntervalType type, double start, double stop, QString name, QColor color, bool test=false) :
        type(type), start(start), stop(stop), name(name), test(test), color(color) {}


        // order bu start time (and stop+name for QMap)
        bool operator< (RideFileInterval right) const { return start < right.start ||
                                                        (start == right.start && stop < right.stop) ||
                                                        (start == right.start && stop == right.stop && name < right.name); }
        bool operator== (RideFileInterval right) const { return start == right.start && stop == right.stop && name == right.name; }
        bool operator!= (RideFileInterval right) const { return start != right.start || stop != right.stop; }

        bool isPeak() const;
        bool isMatch() const;
        bool isClimb() const;
        bool isBest() const;
        bool isTest() const;
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
        friend class RideCache; // tells us if wbal is stale
        friend class RideItem; // derived/wbal stale
        friend class IntervalItem; // access intervals 
        friend class MainWindow; // tells us we were modified
        friend class ComparePane;
        friend class Context; // tells us we were saved
        friend class Athlete; // tells us we were saved

        // file format writers have more access
        friend class RideFileFactory;
        friend struct FitlogFileReader;
        friend struct GcFileReader;
        friend class TcxFileReader;
        friend struct PwxFileReader;
        friend struct JsonFileReader;
        friend class ManualActivityWizard;
        friend class PolarFileReader;
        friend class Strava;
        friend class ErgFile; // access to intervals
        // split and mergers
        friend class MergeActivityWizard;
        friend class SplitActivityWizard;
        friend class SplitConfirm;
        friend class SplitSelect;
        // fix tools
        friend class FixLapSwim;
        friend class Snippets;
        friend struct FitFileParser;

        // utility
        static unsigned int computeFileCRC(QString); 
        void updateDataTag();

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
                          alt, lon, lat, headwind, slope, temp, interval, IsoPower, xPower,
                          vam, wattsKg, lrbalance, lte, rte, lps, rps,
                          aPower, wprime, aTISS, anTISS, smo2, thb, 
                          rvert, rcad, rcontact, gear, o2hb, hhb,
                          lpco, rpco, lppb, rppb, lppe, rppe, lpppb, rpppb, lpppe, rpppe,
                          wbal, tcore, clength, aPowerKg, index,
                          none }; // none must ALWAYS be last
        typedef enum seriestype SeriesType;

        enum conversion { original, pace };

        enum xdatajoin { REPEAT, SPARSE, INTERPOLATE, RESAMPLE };
        typedef enum xdatajoin XDataJoin;

        // NA = Not Applicable - i.e Temperature no recorded value
        // NIL = Not available, but still set to zero for compatibility
        //       We should consider looking at code to handle NIL / NA 
        enum specialValues { NA = -255, NIL = 0 };
        typedef enum conversion Conversion;
        static SeriesType lastSeriesType() { return none; }

        static QStringList symbols(); // get a list of symbols for each series to use in a formula
        static SeriesType seriesForSymbol(QString symbol); // get series for symbol
        static QString symbolForSeries(SeriesType);

        static QString seriesName(SeriesType, bool compat=false);
        static QString unitName(SeriesType, Context *context);
        static QString formatValueWithUnit(double value, SeriesType series, Conversion conversion, Context *context, bool isSwim);

        static int decimalsFor(SeriesType series);
        static double maximumFor(SeriesType series);
        static double minimumFor(SeriesType series);
        static QColor colorFor(SeriesType series);
        static bool parseRideFileName(const QString &name, QDateTime *dt);

        static QString sportTag(QString sport);
        QString sport() const;
        bool isBike() const;
        bool isRun() const;
        bool isSwim() const;
        bool isXtrain() const;
        bool isAero() const;

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
                         double lpco, double rpco,
                         double lppb, double rppb, double lppe, double rppe,
                         double lpppb, double rpppb, double lpppe, double rpppe,
                         double smo2, double thb,
                         double rvert, double rcad, double rcontact, double tcore,
                         int interval);

        void appendOrUpdatePoint(double secs, double cad, double hr, double km,
                                 double kph, double nm, double watts, double alt,
                                 double lon, double lat, double headwind, double slope,
                                 double temperature, double lrbalance,
                                 double lte, double rte, double lps, double rps,
                                 double lpco, double rpco,
                                 double lppb, double rppb, double lppe, double rppe,
                                 double lpppb, double rpppb, double lpppe, double rpppe,
                                 double smo2, double thb,
                                 double rvert, double rcad, double rcontact, double tcore,
                                 int interval, bool forceAppend);

        void updatePointValue(QString valueName, QString valueStr);

        void appendPoint(const RideFilePoint &);

        void updatePoint(RideFilePoint *point, const RideFilePoint *oldPoint);
        void updatePoint(RideFilePoint *point, QString valueName, QString value);

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
        QVector<SeriesType> arePresent(); // list of what is present

        // Working with FIRST CLASS variables
        const QDateTime &startTime() const { return startTime_; }
        void setStartTime(const QDateTime &value);
    
        double recIntSecs() const { return recIntSecs_; }
        void setRecIntSecs(double value) { recIntSecs_ = value; }
        const QString deviceType() const { return getTag("Device", "unknown"); }
        void setDeviceType(const QString &value) { setTag("Device", value); }
        const QString &fileFormat() const { return fileFormat_; }
        void setFileFormat(const QString &value) { fileFormat_ = value; }
        const QString id() const { return id_; }
        void setId(const QString &value) { id_ = value; }

        // Working with INTERVALS
        void addInterval(RideFileInterval::IntervalType type, double start, double stop, const QString &name, QColor color=Qt::black, bool test=false) {
            intervals_.append(new RideFileInterval(type, start, stop, name, color, test));
        }
        int intervalBegin(const RideFileInterval &interval) const;
        int intervalBeginSecs(const double secs) const;
        bool removeInterval(RideFileInterval*);
        void moveInterval(int from, int to);
        RideFileInterval *newInterval(QString name, double start, double stop, QColor color, bool test) {
            RideFileInterval *add = new RideFileInterval(RideFileInterval::USER, start, stop, name, color, test);
            intervals_ << add;
            return add;
        }

        // Working with CAIBRATIONS
        const QList<RideFileCalibration*> &calibrations() const { return calibrations_; }
        void addCalibration(double start, int value, const QString &name) {
            calibrations_.append(new RideFileCalibration(start, value, name));
        }

        // Working with REFERENCES
        void appendReference(const RideFilePoint &);
        void removeReference(int index);
        void removeExhaustion(int index);
        const QVector<RideFilePoint*> &referencePoints() const { return referencePoints_; }

        // Index offset calculations
        double timeToDistance(double) const;  // get distance in km at time in secs
        double distanceToTime(double km) const; // det time from distance
        int timeIndex(double) const;          // get index offset for time in secs
        int distanceIndex(double) const;      // get index offset for distance in KM

        // Working with the METADATA TAGS -- these are ride metadata, there are similar in the RideFileInterval
        //                                   to store and manage interval metadata
        const QMap<QString,QString>& tags() const { return tags_; }
        QString getTag(QString name, QString fallback) const { return tags_.value(name, fallback); }
        void setTag(QString name, QString value) { tags_.insert(name, value); }
        bool removeTag(QString name) { return tags_.remove(name); }

        Context *context;
        double getWeight(); // legacy - moved to Athlete::getWeight
        double getHeight(); // legacy - moved to Athlete::getHeight
 
        WPrime *wprimeData(); // return wprime, init/refresh if needed

        // XDATA
        XDataSeries *xdata(QString name) const { return xdata_.value(name, NULL); }
        void addXData(QString name, XDataSeries *series);
        QMap<QString,XDataSeries*> &xdata() { return xdata_; }
        double xdataValue(const RideFilePoint *p, int &idx, QString xdata, QString series, RideFile::XDataJoin) const;
        void addCIQ(CIQinfo &ciqinfo);
        const QList<CIQinfo>& ciqinfo() const { return ciqinfo_; }

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
        // void setPointValue(int index, SeriesType series, int value);
        void setPointValue(double secs, SeriesType series, double value);
        // void setPointValue(double secs, SeriesType series, int value);
        void deletePoint(int index);
        void deletePoints(int index, int count);
        void insertPoint(int index, RideFilePoint *point);
        void appendPoints(QVector <struct RideFilePoint *> newRows);
        void setDataPresent(SeriesType, bool);
        void insertXDataPoint(QString xdata, int index, XDataPoint *point);
        void deleteXDataPoints(QString xdata, int index, int count);
        void appendXDataPoints(QString xdata, QVector<XDataPoint *> points);
        // ************************************************************

    signals:
        void saved();
        void reverted();
        void modified();
        void deleted();

    protected:

        //  should access via IntervalItem
        const QList<RideFileInterval*> &intervals() const { return intervals_; }

        // xdata series
        QMap<QString, XDataSeries*> xdata_;
        QList<CIQinfo> ciqinfo_; //ciq metadata

        void clearIntervals();
        void fillInIntervals();

        void emitSaved();
        void emitReverted();
        void emitModified();

        bool wstale;

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
        QString fileFormat_;
        QList<RideFileInterval*> intervals_;
        QList<RideFileCalibration*> calibrations_;
        QMap<QString,QString> tags_;
        EditorData *data;
        WPrime *wprime_;
        double weight_; // cached to save calls to getWeight();
        double totalCount, totalTemp;

        QVariant getPointFromValue(double value, SeriesType series) const;
        void updateMin(RideFilePoint* point);
        void updateMax(RideFilePoint* point);
        void updateAvg(SeriesType series, double value);
        void updateAvg(RideFilePoint* point);

        bool dstale; // is derived data up to date?

        // data required to compute headwind based on weather broadcast
        double windSpeed_, windHeading_;
};

struct RideFilePoint
{
    // recorded data
    double secs, cad, hr, km, kph, nm, watts, alt, lon, lat, headwind, slope, temp;

    // pedals
    double lrbalance, lte, rte, lps, rps;
    double lpco, rpco; // left and right platform center offset
    double lppb, rppb, lppe, rppe; // left and right power phase begin/end
    double lpppb, rpppb, lpppe, rpppe; // left and right begin and end peak power phase

    // oxy
    double smo2, thb;

    // acceleration in watts/s km/s
    double hrd, cadd, kphd, nmd, wattsd;

    // running data
    double rvert, rcad, rcontact;

    // core temperature
    double tcore;

    int interval;

    // derived data (we calculate it)
    // xPower, normalised power, aPower
    double xp, np, apower, atiss, antiss, gear, hhb, o2hb, clength;

    // create blank point
    RideFilePoint() : secs(0.0), cad(0.0), hr(0.0), km(0.0), kph(0.0), nm(0.0), 
                      watts(0.0), alt(0.0), lon(0.0), lat(0.0), headwind(0.0), 
                      slope(0.0), temp(-255.0),
                      lrbalance(-255.0),
                      lte(0.0), rte(0.0), lps(0.0), rps(0.0),
                      lpco(0.0), rpco(0.0),
                      lppb(0.0), rppb(0.0), lppe(0.0), rppe(0.0),
                      lpppb(0.0), rpppb(0.0), lpppe(0.0), rpppe(0.0),
                      smo2(0.0), thb(0.0),
                      hrd(0.0), cadd(0.0), kphd(0.0), nmd(0.0), wattsd(0.0),
                      rvert(0.0), rcad(0.0), rcontact(0.0), tcore(0.0),
                      interval(0), xp(0), np(0),
                      apower(0), atiss(0.0), antiss(0.0), gear(0.0), hhb(0.0), o2hb(0.0) {}

    // create point supplying all values
    RideFilePoint(double secs, double cad, double hr, double km, double kph,
                  double nm, double watts, double alt, double lon, double lat,
                  double headwind, double slope, double temp,
                  double lrbalance,
                  double lte, double rte, double lps, double rps,
                  double lpco, double rpco,
                  double lppb, double rppb, double lppe, double rppe,
                  double lpppb, double rpppb, double lpppe, double rpppe,
                  double smo2, double thb, 
                  double rvert, double rcad, double rcontact, double tcore,
                  int interval) :

        secs(secs), cad(cad), hr(hr), km(km), kph(kph), nm(nm), watts(watts), alt(alt), lon(lon), 
        lat(lat), headwind(headwind), slope(slope), temp(temp),
        lrbalance(lrbalance),
        lte(lte), rte(rte), lps(lps), rps(rps),
        lpco(lpco), rpco(rpco),
        lppb(lppb), rppb(rppb), lppe(lppe), rppe(rppe),
        lpppb(lpppb), rpppb(rpppb), lpppe(lpppe), rpppe(rpppe),
        smo2(smo2), thb(thb),
        hrd(0.0), cadd(0.0), kphd(0.0), nmd(0.0), wattsd(0.0), 
        rvert(rvert), rcad(rcad), rcontact(rcontact), tcore(tcore), interval(interval), 
        xp(0), np(0), apower(0), atiss(0.0), antiss(0.0), gear(0.0),hhb(0.0),o2hb(0.0) {}

    // get the value via the series type rather than access direct to the values
    double value(RideFile::SeriesType series) const;
    void setValue(RideFile::SeriesType series, double value);
};

class RideFileIterator {

    public:

        enum IterationSpec { Before=0, Sample=1, After=2 };

        RideFileIterator(RideFile *, Specification, IterationSpec mode=RideFileIterator::Sample);

        void toFront();
        void toBack();

        struct RideFilePoint *first();
        struct RideFilePoint *last();

        int firstIndex() { return start; }
        int lastIndex() { return stop; }

        bool hasNext() const;
        bool hasPrevious() const;

        struct RideFilePoint *next();
        struct RideFilePoint *previous();

    private:
        RideFile *f;
        int start, stop, index;
};

#define XDATA_MAXVALUES 64

class XDataPoint {
public:
    XDataPoint() {
        secs=km=0;
        for(int i=0; i<XDATA_MAXVALUES; i++) {
            number[i]=0;
            string[i]="";
        }
    }
    XDataPoint(const XDataPoint &other) {
        *this = other;
    }
    XDataPoint& operator=(const XDataPoint &other) {
        this->secs=other.secs;
        this->km=other.km;
        for(int i=0; i<XDATA_MAXVALUES; i++) {
            this->number[i]= other.number[i];
            this->string[i]= other.string[i];
        }
        return *this;
    }

    double secs, km;
    double number[XDATA_MAXVALUES];
    QString string[XDATA_MAXVALUES];
};

class XDataSeries {
public:
    XDataSeries() {}
    XDataSeries(const XDataSeries& other) { *this = other; }
    XDataSeries& operator=(const XDataSeries &other) {
        name = other.name;
        valuename = other.valuename;
        unitname = other.unitname;
        valuetype = other.valuetype;
        // we need to delete objects pointed by the assignment target
        foreach(XDataPoint *p, datapoints) delete p;
        datapoints.clear();
        // we need to create new objects since we are holding pointers to objects
        // otherwise we would end up w/ multiple frees or dangling ptrs!
        foreach (XDataPoint *p, other.datapoints) {
            datapoints.push_back(new XDataPoint(*p));
        }
        return *this;
    }

    ~XDataSeries() { foreach(XDataPoint *p, datapoints) delete p; }

    int timeIndex(double) const;          // get index offset for time in secs

    QString name;
    QStringList valuename;
    QStringList unitname;
    QList<RideFile::SeriesType> valuetype;
    QVector<XDataPoint*> datapoints;
};

struct RideFileReader {
    virtual ~RideFileReader() {}
    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*>* = 0) const = 0;

    // if hasWrite capability should re-implement writeRideFile and hasWrite
    virtual bool hasWrite() const { return false; }
    virtual bool writeRideFile(Context *, const RideFile *, QFile &) const { return false; }
};

class MetricAggregator;
class AthleteCard;
class RideFileFactory {

    private:

        static RideFileFactory *instance_;
        QMap<QString,RideFileReader*> readFuncs_;
        QMap<QString,QString> descriptions_;

        RideFileFactory() {}

    protected:

        friend class ::MetricAggregator;
        friend class ::RideCache;
        friend class ::AthleteCard;

        // will become private as code should work with
        // in memory representation not on disk .. but as we
        // migrate will add friends in here.
        // NOTE: DO NOT USE THIS, USE THE athlete->rideCache
        //       TO GET ACCESS TO THE RIDE LIST AND RIDE DATA
        QStringList listRideFiles(const QDir &dir) const;

    public:

        static RideFileFactory &instance();

        int registerReader(const QString &suffix, const QString &description,
                           RideFileReader *reader);
        RideFile *openRideFile(Context *context, QFile &file, QStringList &errors, QList<RideFile*>* = 0) const;
        bool writeRideFile(Context *context, const RideFile *ride, QFile &file, QString format) const;
        QStringList suffixes() const;
        QStringList writeSuffixes() const;
        bool supportedFormat(QString filename) const;
        QString description(const QString &suffix) const {
            return descriptions_[suffix];
        }
        QRegExp rideFileRegExp() const;
        RideFileReader *readerForSuffix(QString) const; 
};

#endif // _RideFile_h
