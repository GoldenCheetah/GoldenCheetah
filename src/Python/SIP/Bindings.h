#include <QString>
#include "RideFile.h"

#include <pyport.h> // for Py_ssize_t

class PythonDataSeries {

    public:
        PythonDataSeries(const RideFile *f, RideFile::SeriesType series);
        PythonDataSeries(PythonDataSeries*);
        PythonDataSeries();
        ~PythonDataSeries();

        RideFile::SeriesType series;
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
};
