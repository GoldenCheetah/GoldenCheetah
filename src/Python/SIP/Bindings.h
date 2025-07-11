#ifndef _Bindings_h
#define _Bindings_h

#include <QString>
#include "RideFile.h"
#include "RideFileCache.h"
#include "RideFileCommand.h"

#undef slots
#include <Python.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wcast-function-type" // shut gcc up
#endif

class PythonDataSeries {

    public:
        PythonDataSeries(QString name, Py_ssize_t count, bool readOnly, RideFile::SeriesType seriesType, RideFile *rideFile);
        PythonDataSeries(QString name, Py_ssize_t count);
        PythonDataSeries(PythonDataSeries*);
        PythonDataSeries();
        ~PythonDataSeries();

        QString name;
        Py_ssize_t count;
        double *data;

        bool readOnly;
        int seriesType;
        RideFile *rideFile;
};

class PythonXDataSeries {
    public:
        PythonXDataSeries(QString xdata, QString series, QString unit, int count, bool readOnly, RideFile *rideFile);
        PythonXDataSeries(PythonXDataSeries*);
        PythonXDataSeries();

        QString name() const { return QString("%1_%2").arg(xdata).arg(series); }

        double get(int i) const { return data[i]; }
        bool set(int i, double value);
        bool add(double value);
        bool remove(int i);

        void *rawDataPtr() { return (void*) data.data(); }
        int count() const { return data.count(); }

        QString xdata;
        QString series;
        int colIdx;
        QString unit;

        bool readOnly;
        RideFile *rideFile;

        QVector<Py_ssize_t> shape;

    private:
        bool setColIdx();

        QVector<double> data;
};

class Bindings {

    public:
        long threadid() const;
        long build() const;
        QString version() const;
        void result(double);

        // working with the web view
        bool webpage(QString url) const;

        // working with athlete data
        PyObject* athlete() const;
        PyObject* athleteZones(PyObject* date=NULL, QString sport="") const;

        // working with activities
        PyObject* activities(QString filter=QString()) const;

        // working with seasons
        PyObject* season(bool all=false, bool compare=false) const;

        // working with data series
        bool seriesPresent(int type, PyObject* activity=NULL, int compareindex=-1) const;
        int seriesLast() const;
        QString seriesName(int type) const;
        PythonDataSeries *series(int type, PyObject* activity=NULL, int compareindex=-1) const;
        PythonDataSeries *activityWbal(PyObject* activity=NULL, int compareindex=-1) const;
        PythonDataSeries *xdata(QString name, QString series, QString join="repeat", PyObject* activity=NULL, int compareindex=-1) const;
        PythonXDataSeries *xdataSeries(QString name, QString series, PyObject* activity=NULL, int compareindex=-1) const;
        PyObject* xdataNames(QString name=QString(), PyObject* activity=NULL, int compareindex=-1) const;

        // working with metrics
        PyObject* activityMetrics(bool compare=false) const;
        PyObject* seasonMetrics(bool all=false, QString filter=QString(), bool compare=false) const;
        PythonDataSeries *metrics(QString metric, bool all=false, QString filter=QString()) const;
        PyObject* seasonPmc(bool all=false, QString metric=QString("BikeStress"), QString type=QString("Actual")) const;
        PyObject* seasonMeasures(bool all=false, QString group=QString("Body")) const;

        // working with meanmax data
        PyObject* activityMeanmax(bool compare=false) const;
        PyObject* seasonMeanmax(bool all=false, QString filter=QString(), bool compare=false) const;
        PyObject* seasonPeaks(QString series, int duration, bool all=false, QString filter=QString(), bool compare=false) const;

        // working with intervals
        QString intervalType(int type) const;
        PyObject* seasonIntervals(QString type=QString(), bool compare=false) const;
        PyObject* activityIntervals(QString type=QString(), PyObject* activity=NULL) const;

        // editing data
        bool createXDataSeries(QString name, QString series, QString seriesUnit, PyObject *activity=NULL) const;
        bool deleteXDataSeries(QString name, QString series, PyObject *activity=NULL) const;
        bool deleteXData(QString names, PyObject *activity=NULL) const;
        bool deleteActivitySample(int index = -1, PyObject *activity = NULL) const;
        bool deleteSeries(int type, PyObject *activity = NULL) const;
        bool postProcess(QString processor, PyObject *activity = NULL) const;
        bool setTag(QString name, QString value, PyObject *activity = NULL) const;
        bool delTag(QString name, PyObject *activity = NULL) const;
        bool hasTag(QString name, PyObject *activity = NULL) const;
        QString getTag(QString name, PyObject *activity = NULL) const;

        // working with charts
        bool configChart(QString title, int type, bool animate, int pos, bool stack, int orientation) const;
        bool setCurve(QString name, PyObject *xseries, PyObject *yseries, QStringList fseries, QString xname, QString yname,
                      QStringList labels,  QStringList colors,
                      int line, int symbol, int symbolsize, QString color, int opacity, bool opengl, bool legend, bool datalabels, bool fill) const;
        bool configAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories);
        bool addAnnotation(QString, QString, QString, double);

    private:
        // find a RideItem by DateTime
        RideItem* fromDateTime(PyObject* activity=NULL) const;
        RideFile *selectRideFile(PyObject *activity = nullptr, int compareindex=-1) const;

        // get a dict populated with metrics and metadata
        PyObject* activityMetrics(RideItem* item) const;
        PyObject* seasonMetrics(bool all, DateRange range, QString filter) const;
        PyObject* seasonIntervals(DateRange range, QString type) const;

        // get a dict populated with meanmax data
        PyObject* activityMeanmax(const RideItem* item) const;
        PyObject* seasonMeanmax(bool all, DateRange range, QString filter) const;
        PyObject* rideFileCacheMeanmax(RideFileCache* cache) const;
        PyObject* seasonPeaks(bool all, DateRange range, QString filter, QList<RideFile::SeriesType> series, QList<int> durations) const;

        int PyDict_SetItemString_Steal(PyObject *p, const char *key, PyObject *val) const;
};

#endif // _Bindings_h
