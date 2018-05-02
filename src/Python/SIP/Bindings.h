#include <QString>
#include "RideFile.h"
#include "RideFileCache.h"

#undef slots
#include <Python.h>


class PythonDataSeries {

    public:
        PythonDataSeries(QString name, Py_ssize_t count);
        PythonDataSeries(PythonDataSeries*);
        PythonDataSeries();
        ~PythonDataSeries();

        QString name;
        Py_ssize_t count;
        double *data;
};

class Bindings {

    public:
        long threadid() const;
        long build() const;
        QString version() const;
        void result(double);

        // working with the web view
        int webpage(QString url) const;

        // working with athlete data
        PyObject* athlete() const;
        PyObject* athleteZones(PyObject* date=NULL, QString sport="") const;

        // working with activities
        PyObject* activities(QString filter=QString()) const;

        // working with seasons
        PyObject* season(bool all=false, bool compare=false) const;

        // working with data series
        bool seriesPresent(int type, PyObject* activity=NULL) const;
        int seriesLast() const;
        QString seriesName(int type) const;
        PythonDataSeries *series(int type, PyObject* activity=NULL) const;
        PythonDataSeries *activityWbal(PyObject* activity=NULL) const;
        PythonDataSeries *activityXdata(QString name, QString series, QString join="repeat", PyObject* activity=NULL) const;
        PythonDataSeries *activityXdataSeries(QString name, QString series, PyObject* activity=NULL) const;

        // working with metrics
        PyObject* activityMetrics(bool compare=false) const;
        PyObject* seasonMetrics(bool all=false, QString filter=QString(), bool compare=false) const;
        PythonDataSeries *metrics(QString metric, bool all=false, QString filter=QString()) const;
        PyObject* seasonPmc(bool all=false, QString metric=QString("TSS")) const;
        PyObject* seasonMeasures(bool all=false, QString group=QString("Body")) const;

        // working with meanmax data
        PyObject* activityMeanmax(bool compare=false) const;
        PyObject* seasonMeanmax(bool all=false, QString filter=QString(), bool compare=false) const;
        PyObject* seasonPeaks(QString series, int duration, bool all=false, QString filter=QString(), bool compare=false) const;

        // working with intervals
        PyObject* seasonIntervals(QString type=QString(), bool compare=false) const;
        PyObject* activityIntervals(QString type=QString(), PyObject* activity=NULL) const;

    private:
        // find a RideItem by DateTime
        RideItem* fromDateTime(PyObject* activity=NULL) const;

        // get a dict populated with metrics and metadata
        PyObject* activityMetrics(RideItem* item) const;
        PyObject* seasonMetrics(bool all, DateRange range, QString filter) const;
        PyObject* seasonIntervals(DateRange range, QString type) const;

        // get a dict populated with meanmax data
        PyObject* activityMeanmax(const RideItem* item) const;
        PyObject* seasonMeanmax(bool all, DateRange range, QString filter) const;
        PyObject* rideFileCacheMeanmax(RideFileCache* cache) const;
        PyObject* seasonPeaks(bool all, DateRange range, QString filter, QList<RideFile::SeriesType> series, QList<int> durations) const;

};
