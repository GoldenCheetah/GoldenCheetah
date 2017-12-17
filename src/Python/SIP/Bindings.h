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
        QString athlete() const;
        long build() const;
        QString version() const;

        // working with data series
        bool seriesPresent(int type) const;
        int seriesLast() const;
        QString seriesName(int type) const;
        PythonDataSeries *series(int type) const;

        // working with metrics
        PyObject* activityMetrics(bool compare=false) const;
        PyObject* seasonMetrics(bool all=false, QString filter=QString(), bool compare=false) const;
        PythonDataSeries *metrics(QString metric, bool all=false, QString filter=QString()) const;
        PyObject* pmc(bool all=false, QString metric=QString("TSS")) const;
        PyObject* measures(bool all=false, QString group=QString("Body")) const;

        // working with meanmax data
        PyObject* activityMeanmax(bool compare=false) const;
        PyObject* seasonMeanmax(bool all=false, QString filter=QString(), bool compare=false) const;

        // working with the web view
        int webpage(QString url) const;

    private:
        // get a dict populated with metrics and metadata
        PyObject* activityMetrics(RideItem* item) const;
        PyObject* seasonMetrics(bool all, DateRange range, QString filter) const;
        // get a dict populated with meanmax data
        PyObject* activityMeanmax(const RideItem* item) const;
        PyObject* seasonMeanmax(bool all, DateRange range, QString filter) const;
        PyObject* rideFileCacheMeanmax(RideFileCache* cache) const;

};
